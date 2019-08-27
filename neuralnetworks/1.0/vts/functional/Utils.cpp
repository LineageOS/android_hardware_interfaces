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
#include <android/hidl/allocator/1.0/IAllocator.h>
#include <android/hidl/memory/1.0/IMemory.h>
#include <hidlmemory/mapping.h>

#include <vector>

namespace android {
namespace hardware {
namespace neuralnetworks {

using namespace test_helper;
using ::android::hardware::neuralnetworks::V1_0::DataLocation;
using ::android::hardware::neuralnetworks::V1_0::Request;
using ::android::hardware::neuralnetworks::V1_0::RequestArgument;
using ::android::hidl::memory::V1_0::IMemory;

constexpr uint32_t kInputPoolIndex = 0;
constexpr uint32_t kOutputPoolIndex = 1;

Request createRequest(const TestModel& testModel) {
    // Model inputs.
    hidl_vec<RequestArgument> inputs(testModel.inputIndexes.size());
    size_t inputSize = 0;
    for (uint32_t i = 0; i < testModel.inputIndexes.size(); i++) {
        const auto& op = testModel.operands[testModel.inputIndexes[i]];
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
    hidl_vec<RequestArgument> outputs(testModel.outputIndexes.size());
    size_t outputSize = 0;
    for (uint32_t i = 0; i < testModel.outputIndexes.size(); i++) {
        const auto& op = testModel.operands[testModel.outputIndexes[i]];
        size_t dataSize = op.data.size();
        DataLocation loc = {.poolIndex = kOutputPoolIndex,
                            .offset = static_cast<uint32_t>(outputSize),
                            .length = static_cast<uint32_t>(dataSize)};
        outputSize += op.data.alignedSize();
        outputs[i] = {.hasNoValue = false, .location = loc, .dimensions = {}};
    }

    // Allocate memory pools.
    hidl_vec<hidl_memory> pools = {nn::allocateSharedMemory(inputSize),
                                   nn::allocateSharedMemory(outputSize)};
    CHECK_NE(pools[kInputPoolIndex].size(), 0u);
    CHECK_NE(pools[kOutputPoolIndex].size(), 0u);
    sp<IMemory> inputMemory = mapMemory(pools[kInputPoolIndex]);
    CHECK(inputMemory.get() != nullptr);
    uint8_t* inputPtr = static_cast<uint8_t*>(static_cast<void*>(inputMemory->getPointer()));
    CHECK(inputPtr != nullptr);

    // Copy input data to the memory pool.
    for (uint32_t i = 0; i < testModel.inputIndexes.size(); i++) {
        const auto& op = testModel.operands[testModel.inputIndexes[i]];
        if (op.data.size() > 0) {
            const uint8_t* begin = op.data.get<uint8_t>();
            const uint8_t* end = begin + op.data.size();
            std::copy(begin, end, inputPtr + inputs[i].location.offset);
        }
    }

    return {.inputs = std::move(inputs), .outputs = std::move(outputs), .pools = std::move(pools)};
}

std::vector<TestBuffer> getOutputBuffers(const Request& request) {
    sp<IMemory> outputMemory = mapMemory(request.pools[kOutputPoolIndex]);
    CHECK(outputMemory.get() != nullptr);
    uint8_t* outputPtr = static_cast<uint8_t*>(static_cast<void*>(outputMemory->getPointer()));
    CHECK(outputPtr != nullptr);

    // Copy out output results.
    std::vector<TestBuffer> outputBuffers;
    for (const auto& output : request.outputs) {
        outputBuffers.emplace_back(output.location.length, outputPtr + output.location.offset);
    }

    return outputBuffers;
}

}  // namespace neuralnetworks
}  // namespace hardware
}  // namespace android
