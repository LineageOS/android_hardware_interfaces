/*
 * Copyright (C) 2019 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "1.0/Utils.h"

#include "MemoryUtils.h"
#include "TestHarness.h"

#include <android-base/logging.h>
#include <android/hardware/neuralnetworks/1.0/types.h>
#include <android/hardware_buffer.h>
#include <android/hidl/allocator/1.0/IAllocator.h>
#include <android/hidl/memory/1.0/IMemory.h>
#include <hidlmemory/mapping.h>
#include <vndk/hardware_buffer.h>

#include <gtest/gtest.h>
#include <algorithm>
#include <iostream>
#include <vector>

namespace android::hardware::neuralnetworks {

using namespace test_helper;
using hidl::memory::V1_0::IMemory;
using V1_0::DataLocation;
using V1_0::Request;
using V1_0::RequestArgument;

std::unique_ptr<TestAshmem> TestAshmem::create(uint32_t size) {
    auto ashmem = std::make_unique<TestAshmem>(size);
    return ashmem->mIsValid ? std::move(ashmem) : nullptr;
}

void TestAshmem::initialize(uint32_t size) {
    mIsValid = false;
    ASSERT_GT(size, 0);
    mHidlMemory = nn::allocateSharedMemory(size);
    ASSERT_TRUE(mHidlMemory.valid());
    mMappedMemory = mapMemory(mHidlMemory);
    ASSERT_NE(mMappedMemory, nullptr);
    mPtr = static_cast<uint8_t*>(static_cast<void*>(mMappedMemory->getPointer()));
    ASSERT_NE(mPtr, nullptr);
    mIsValid = true;
}

std::unique_ptr<TestBlobAHWB> TestBlobAHWB::create(uint32_t size) {
    auto ahwb = std::make_unique<TestBlobAHWB>(size);
    return ahwb->mIsValid ? std::move(ahwb) : nullptr;
}

void TestBlobAHWB::initialize(uint32_t size) {
    mIsValid = false;
    ASSERT_GT(size, 0);
    const auto usage = AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN | AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN;
    const AHardwareBuffer_Desc desc = {
            .width = size,
            .height = 1,
            .layers = 1,
            .format = AHARDWAREBUFFER_FORMAT_BLOB,
            .usage = usage,
            .stride = size,
    };
    ASSERT_EQ(AHardwareBuffer_allocate(&desc, &mAhwb), 0);
    ASSERT_NE(mAhwb, nullptr);

    void* buffer = nullptr;
    ASSERT_EQ(AHardwareBuffer_lock(mAhwb, usage, -1, nullptr, &buffer), 0);
    ASSERT_NE(buffer, nullptr);
    mPtr = static_cast<uint8_t*>(buffer);

    const native_handle_t* handle = AHardwareBuffer_getNativeHandle(mAhwb);
    ASSERT_NE(handle, nullptr);
    mHidlMemory = hidl_memory("hardware_buffer_blob", handle, desc.width);
    mIsValid = true;
}

TestBlobAHWB::~TestBlobAHWB() {
    if (mAhwb) {
        AHardwareBuffer_unlock(mAhwb, nullptr);
        AHardwareBuffer_release(mAhwb);
    }
}

Request ExecutionContext::createRequest(const TestModel& testModel, MemoryType memoryType) {
    CHECK(memoryType == MemoryType::ASHMEM || memoryType == MemoryType::BLOB_AHWB);

    // Model inputs.
    hidl_vec<RequestArgument> inputs(testModel.main.inputIndexes.size());
    size_t inputSize = 0;
    for (uint32_t i = 0; i < testModel.main.inputIndexes.size(); i++) {
        const auto& op = testModel.main.operands[testModel.main.inputIndexes[i]];
        if (op.data.size() == 0) {
            // Omitted input.
            inputs[i] = {.hasNoValue = true};
        } else {
            DataLocation loc = {.poolIndex = kInputPoolIndex,
                                .offset = static_cast<uint32_t>(inputSize),
                                .length = static_cast<uint32_t>(op.data.size())};
            inputSize += op.data.alignedSize();
            inputs[i] = {.hasNoValue = false, .location = loc, .dimensions = {}};
        }
    }

    // Model outputs.
    hidl_vec<RequestArgument> outputs(testModel.main.outputIndexes.size());
    size_t outputSize = 0;
    for (uint32_t i = 0; i < testModel.main.outputIndexes.size(); i++) {
        const auto& op = testModel.main.operands[testModel.main.outputIndexes[i]];

        // In the case of zero-sized output, we should at least provide a one-byte buffer.
        // This is because zero-sized tensors are only supported internally to the driver, or
        // reported in output shapes. It is illegal for the client to pre-specify a zero-sized
        // tensor as model output. Otherwise, we will have two semantic conflicts:
        // - "Zero dimension" conflicts with "unspecified dimension".
        // - "Omitted operand buffer" conflicts with "zero-sized operand buffer".
        size_t bufferSize = std::max<size_t>(op.data.size(), 1);

        DataLocation loc = {.poolIndex = kOutputPoolIndex,
                            .offset = static_cast<uint32_t>(outputSize),
                            .length = static_cast<uint32_t>(bufferSize)};
        outputSize += op.data.size() == 0 ? TestBuffer::kAlignment : op.data.alignedSize();
        outputs[i] = {.hasNoValue = false, .location = loc, .dimensions = {}};
    }

    // Allocate memory pools.
    if (memoryType == MemoryType::ASHMEM) {
        mInputMemory = TestAshmem::create(inputSize);
        mOutputMemory = TestAshmem::create(outputSize);
    } else {
        mInputMemory = TestBlobAHWB::create(inputSize);
        mOutputMemory = TestBlobAHWB::create(outputSize);
    }
    EXPECT_NE(mInputMemory, nullptr);
    EXPECT_NE(mOutputMemory, nullptr);
    hidl_vec<hidl_memory> pools = {mInputMemory->getHidlMemory(), mOutputMemory->getHidlMemory()};

    // Copy input data to the memory pool.
    uint8_t* inputPtr = mInputMemory->getPointer();
    for (uint32_t i = 0; i < testModel.main.inputIndexes.size(); i++) {
        const auto& op = testModel.main.operands[testModel.main.inputIndexes[i]];
        if (op.data.size() > 0) {
            const uint8_t* begin = op.data.get<uint8_t>();
            const uint8_t* end = begin + op.data.size();
            std::copy(begin, end, inputPtr + inputs[i].location.offset);
        }
    }

    return {.inputs = std::move(inputs), .outputs = std::move(outputs), .pools = std::move(pools)};
}

std::vector<TestBuffer> ExecutionContext::getOutputBuffers(const Request& request) const {
    // Copy out output results.
    uint8_t* outputPtr = mOutputMemory->getPointer();
    std::vector<TestBuffer> outputBuffers;
    for (const auto& output : request.outputs) {
        outputBuffers.emplace_back(output.location.length, outputPtr + output.location.offset);
    }
    return outputBuffers;
}

std::string gtestCompliantName(std::string name) {
    // gtest test names must only contain alphanumeric characters
    std::replace_if(
            name.begin(), name.end(), [](char c) { return !std::isalnum(c); }, '_');
    return name;
}

}  // namespace android::hardware::neuralnetworks

namespace android::hardware::neuralnetworks::V1_0 {

::std::ostream& operator<<(::std::ostream& os, ErrorStatus errorStatus) {
    return os << toString(errorStatus);
}

::std::ostream& operator<<(::std::ostream& os, DeviceStatus deviceStatus) {
    return os << toString(deviceStatus);
}

}  // namespace android::hardware::neuralnetworks::V1_0
