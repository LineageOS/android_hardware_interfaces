/*
 * Copyright (C) 2021 The Android Open Source Project
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

#include "Utils.h"

#include <aidl/android/hardware/neuralnetworks/IPreparedModelParcel.h>
#include <aidl/android/hardware/neuralnetworks/Operand.h>
#include <aidl/android/hardware/neuralnetworks/OperandType.h>
#include <android-base/logging.h>
#include <android/binder_status.h>
#include <android/hardware_buffer.h>

#include <sys/mman.h>
#include <iostream>
#include <limits>
#include <numeric>

#include <MemoryUtils.h>
#include <nnapi/SharedMemory.h>
#include <nnapi/hal/aidl/Conversions.h>
#include <nnapi/hal/aidl/Utils.h>

namespace aidl::android::hardware::neuralnetworks {

using test_helper::TestBuffer;
using test_helper::TestModel;

uint32_t sizeOfData(OperandType type) {
    switch (type) {
        case OperandType::FLOAT32:
        case OperandType::INT32:
        case OperandType::UINT32:
        case OperandType::TENSOR_FLOAT32:
        case OperandType::TENSOR_INT32:
            return 4;
        case OperandType::TENSOR_QUANT16_SYMM:
        case OperandType::TENSOR_FLOAT16:
        case OperandType::FLOAT16:
        case OperandType::TENSOR_QUANT16_ASYMM:
            return 2;
        case OperandType::TENSOR_QUANT8_ASYMM:
        case OperandType::BOOL:
        case OperandType::TENSOR_BOOL8:
        case OperandType::TENSOR_QUANT8_SYMM_PER_CHANNEL:
        case OperandType::TENSOR_QUANT8_SYMM:
        case OperandType::TENSOR_QUANT8_ASYMM_SIGNED:
            return 1;
        case OperandType::SUBGRAPH:
            return 0;
        default:
            CHECK(false) << "Invalid OperandType " << static_cast<uint32_t>(type);
            return 0;
    }
}

static bool isTensor(OperandType type) {
    switch (type) {
        case OperandType::FLOAT32:
        case OperandType::INT32:
        case OperandType::UINT32:
        case OperandType::FLOAT16:
        case OperandType::BOOL:
        case OperandType::SUBGRAPH:
            return false;
        case OperandType::TENSOR_FLOAT32:
        case OperandType::TENSOR_INT32:
        case OperandType::TENSOR_QUANT16_SYMM:
        case OperandType::TENSOR_FLOAT16:
        case OperandType::TENSOR_QUANT16_ASYMM:
        case OperandType::TENSOR_QUANT8_ASYMM:
        case OperandType::TENSOR_BOOL8:
        case OperandType::TENSOR_QUANT8_SYMM_PER_CHANNEL:
        case OperandType::TENSOR_QUANT8_SYMM:
        case OperandType::TENSOR_QUANT8_ASYMM_SIGNED:
            return true;
        default:
            CHECK(false) << "Invalid OperandType " << static_cast<uint32_t>(type);
            return false;
    }
}

uint32_t sizeOfData(const Operand& operand) {
    const uint32_t dataSize = sizeOfData(operand.type);
    if (isTensor(operand.type) && operand.dimensions.size() == 0) return 0;
    return std::accumulate(operand.dimensions.begin(), operand.dimensions.end(), dataSize,
                           std::multiplies<>{});
}

std::unique_ptr<TestAshmem> TestAshmem::create(uint32_t size, bool aidlReadonly) {
    auto ashmem = std::make_unique<TestAshmem>(size, aidlReadonly);
    return ashmem->mIsValid ? std::move(ashmem) : nullptr;
}

// This function will create a readonly shared memory with PROT_READ only.
// The input shared memory must be either Ashmem or mapped-FD.
static nn::SharedMemory convertSharedMemoryToReadonly(const nn::SharedMemory& sharedMemory) {
    if (std::holds_alternative<nn::Memory::Ashmem>(sharedMemory->handle)) {
        const auto& memory = std::get<nn::Memory::Ashmem>(sharedMemory->handle);
        return nn::createSharedMemoryFromFd(memory.size, PROT_READ, memory.fd.get(), /*offset=*/0)
                .value();
    } else if (std::holds_alternative<nn::Memory::Fd>(sharedMemory->handle)) {
        const auto& memory = std::get<nn::Memory::Fd>(sharedMemory->handle);
        return nn::createSharedMemoryFromFd(memory.size, PROT_READ, memory.fd.get(), memory.offset)
                .value();
    }
    CHECK(false) << "Unexpected shared memory type";
    return sharedMemory;
}

void TestAshmem::initialize(uint32_t size, bool aidlReadonly) {
    mIsValid = false;
    ASSERT_GT(size, 0);
    const auto sharedMemory = nn::createSharedMemory(size).value();
    mMappedMemory = nn::map(sharedMemory).value();
    mPtr = static_cast<uint8_t*>(std::get<void*>(mMappedMemory.pointer));
    CHECK_NE(mPtr, nullptr);
    if (aidlReadonly) {
        mAidlMemory = utils::convert(convertSharedMemoryToReadonly(sharedMemory)).value();
    } else {
        mAidlMemory = utils::convert(sharedMemory).value();
    }
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

    AHardwareBuffer* ahwb = nullptr;
    ASSERT_EQ(AHardwareBuffer_allocate(&desc, &ahwb), 0);
    ASSERT_NE(ahwb, nullptr);

    mMemory = nn::createSharedMemoryFromAHWB(ahwb, /*takeOwnership=*/true).value();
    mMapping = nn::map(mMemory).value();
    mPtr = static_cast<uint8_t*>(std::get<void*>(mMapping.pointer));
    CHECK_NE(mPtr, nullptr);
    mAidlMemory = utils::convert(mMemory).value();

    mIsValid = true;
}

std::string gtestCompliantName(std::string name) {
    // gtest test names must only contain alphanumeric characters
    std::replace_if(
            name.begin(), name.end(), [](char c) { return !std::isalnum(c); }, '_');
    return name;
}

::std::ostream& operator<<(::std::ostream& os, ErrorStatus errorStatus) {
    return os << toString(errorStatus);
}

Request ExecutionContext::createRequest(const TestModel& testModel, MemoryType memoryType) {
    CHECK(memoryType == MemoryType::ASHMEM || memoryType == MemoryType::BLOB_AHWB);

    // Model inputs.
    std::vector<RequestArgument> inputs(testModel.main.inputIndexes.size());
    size_t inputSize = 0;
    for (uint32_t i = 0; i < testModel.main.inputIndexes.size(); i++) {
        const auto& op = testModel.main.operands[testModel.main.inputIndexes[i]];
        if (op.data.size() == 0) {
            // Omitted input.
            inputs[i] = {.hasNoValue = true};
        } else {
            DataLocation loc = {.poolIndex = kInputPoolIndex,
                                .offset = static_cast<int64_t>(inputSize),
                                .length = static_cast<int64_t>(op.data.size())};
            inputSize += op.data.alignedSize();
            inputs[i] = {.hasNoValue = false, .location = loc, .dimensions = {}};
        }
    }

    // Model outputs.
    std::vector<RequestArgument> outputs(testModel.main.outputIndexes.size());
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
                            .offset = static_cast<int64_t>(outputSize),
                            .length = static_cast<int64_t>(bufferSize)};
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
    CHECK_NE(mInputMemory, nullptr);
    CHECK_NE(mOutputMemory, nullptr);

    auto copiedInputMemory = utils::clone(*mInputMemory->getAidlMemory());
    CHECK(copiedInputMemory.has_value()) << copiedInputMemory.error().message;
    auto copiedOutputMemory = utils::clone(*mOutputMemory->getAidlMemory());
    CHECK(copiedOutputMemory.has_value()) << copiedOutputMemory.error().message;

    std::vector<RequestMemoryPool> pools;
    pools.push_back(RequestMemoryPool::make<RequestMemoryPool::Tag::pool>(
            std::move(copiedInputMemory).value()));
    pools.push_back(RequestMemoryPool::make<RequestMemoryPool::Tag::pool>(
            std::move(copiedOutputMemory).value()));

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

}  // namespace aidl::android::hardware::neuralnetworks
