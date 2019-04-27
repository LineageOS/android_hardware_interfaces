/*
 * Copyright (C) 2018 The Android Open Source Project
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

#define LOG_TAG "neuralnetworks_hidl_hal_test"

#include "VtsHalNeuralnetworks.h"

#include "Callbacks.h"
#include "ExecutionBurstController.h"
#include "TestHarness.h"
#include "Utils.h"

#include <android-base/logging.h>
#include <android/hidl/memory/1.0/IMemory.h>
#include <hidlmemory/mapping.h>

namespace android {
namespace hardware {
namespace neuralnetworks {
namespace V1_2 {
namespace vts {
namespace functional {

using ::android::hardware::neuralnetworks::V1_2::implementation::ExecutionCallback;
using ::android::hidl::memory::V1_0::IMemory;
using test_helper::for_all;
using test_helper::MixedTyped;
using test_helper::MixedTypedExample;

///////////////////////// UTILITY FUNCTIONS /////////////////////////

static bool badTiming(Timing timing) {
    return timing.timeOnDevice == UINT64_MAX && timing.timeInDriver == UINT64_MAX;
}

// Primary validation function. This function will take a valid request, apply a
// mutation to it to invalidate the request, then pass it to interface calls
// that use the request. Note that the request here is passed by value, and any
// mutation to the request does not leave this function.
static void validate(const sp<IPreparedModel>& preparedModel, const std::string& message,
                     Request request, const std::function<void(Request*)>& mutation) {
    mutation(&request);

    // We'd like to test both with timing requested and without timing
    // requested. Rather than running each test both ways, we'll decide whether
    // to request timing by hashing the message. We do not use std::hash because
    // it is not guaranteed stable across executions.
    char hash = 0;
    for (auto c : message) {
        hash ^= c;
    };
    MeasureTiming measure = (hash & 1) ? MeasureTiming::YES : MeasureTiming::NO;

    // asynchronous
    {
        SCOPED_TRACE(message + " [execute_1_2]");

        sp<ExecutionCallback> executionCallback = new ExecutionCallback();
        ASSERT_NE(nullptr, executionCallback.get());
        Return<ErrorStatus> executeLaunchStatus =
                preparedModel->execute_1_2(request, measure, executionCallback);
        ASSERT_TRUE(executeLaunchStatus.isOk());
        ASSERT_EQ(ErrorStatus::INVALID_ARGUMENT, static_cast<ErrorStatus>(executeLaunchStatus));

        executionCallback->wait();
        ErrorStatus executionReturnStatus = executionCallback->getStatus();
        const auto& outputShapes = executionCallback->getOutputShapes();
        Timing timing = executionCallback->getTiming();
        ASSERT_EQ(ErrorStatus::INVALID_ARGUMENT, executionReturnStatus);
        ASSERT_EQ(outputShapes.size(), 0);
        ASSERT_TRUE(badTiming(timing));
    }

    // synchronous
    {
        SCOPED_TRACE(message + " [executeSynchronously]");

        Return<void> executeStatus = preparedModel->executeSynchronously(
                request, measure,
                [](ErrorStatus error, const hidl_vec<OutputShape>& outputShapes,
                   const Timing& timing) {
                    ASSERT_EQ(ErrorStatus::INVALID_ARGUMENT, error);
                    EXPECT_EQ(outputShapes.size(), 0);
                    EXPECT_TRUE(badTiming(timing));
                });
        ASSERT_TRUE(executeStatus.isOk());
    }

    // burst
    {
        SCOPED_TRACE(message + " [burst]");

        // create burst
        std::shared_ptr<::android::nn::ExecutionBurstController> burst =
                ::android::nn::ExecutionBurstController::create(preparedModel, /*blocking=*/true);
        ASSERT_NE(nullptr, burst.get());

        // create memory keys
        std::vector<intptr_t> keys(request.pools.size());
        for (size_t i = 0; i < keys.size(); ++i) {
            keys[i] = reinterpret_cast<intptr_t>(&request.pools[i]);
        }

        // execute and verify
        ErrorStatus error;
        std::vector<OutputShape> outputShapes;
        Timing timing;
        std::tie(error, outputShapes, timing) = burst->compute(request, measure, keys);
        EXPECT_EQ(ErrorStatus::INVALID_ARGUMENT, error);
        EXPECT_EQ(outputShapes.size(), 0);
        EXPECT_TRUE(badTiming(timing));

        // additional burst testing
        if (request.pools.size() > 0) {
            // valid free
            burst->freeMemory(keys.front());

            // negative test: invalid free of unknown (blank) memory
            burst->freeMemory(intptr_t{});

            // negative test: double free of memory
            burst->freeMemory(keys.front());
        }
    }
}

// Delete element from hidl_vec. hidl_vec doesn't support a "remove" operation,
// so this is efficiently accomplished by moving the element to the end and
// resizing the hidl_vec to one less.
template <typename Type>
static void hidl_vec_removeAt(hidl_vec<Type>* vec, uint32_t index) {
    if (vec) {
        std::rotate(vec->begin() + index, vec->begin() + index + 1, vec->end());
        vec->resize(vec->size() - 1);
    }
}

template <typename Type>
static uint32_t hidl_vec_push_back(hidl_vec<Type>* vec, const Type& value) {
    // assume vec is valid
    const uint32_t index = vec->size();
    vec->resize(index + 1);
    (*vec)[index] = value;
    return index;
}

///////////////////////// REMOVE INPUT ////////////////////////////////////

static void removeInputTest(const sp<IPreparedModel>& preparedModel, const Request& request) {
    for (size_t input = 0; input < request.inputs.size(); ++input) {
        const std::string message = "removeInput: removed input " + std::to_string(input);
        validate(preparedModel, message, request,
                 [input](Request* request) { hidl_vec_removeAt(&request->inputs, input); });
    }
}

///////////////////////// REMOVE OUTPUT ////////////////////////////////////

static void removeOutputTest(const sp<IPreparedModel>& preparedModel, const Request& request) {
    for (size_t output = 0; output < request.outputs.size(); ++output) {
        const std::string message = "removeOutput: removed Output " + std::to_string(output);
        validate(preparedModel, message, request,
                 [output](Request* request) { hidl_vec_removeAt(&request->outputs, output); });
    }
}

///////////////////////////// ENTRY POINT //////////////////////////////////

std::vector<Request> createRequests(const std::vector<MixedTypedExample>& examples) {
    const uint32_t INPUT = 0;
    const uint32_t OUTPUT = 1;

    std::vector<Request> requests;

    for (auto& example : examples) {
        const MixedTyped& inputs = example.operands.first;
        const MixedTyped& outputs = example.operands.second;

        std::vector<RequestArgument> inputs_info, outputs_info;
        uint32_t inputSize = 0, outputSize = 0;

        // This function only partially specifies the metadata (vector of RequestArguments).
        // The contents are copied over below.
        for_all(inputs, [&inputs_info, &inputSize](int index, auto, auto s) {
            if (inputs_info.size() <= static_cast<size_t>(index)) inputs_info.resize(index + 1);
            RequestArgument arg = {
                .location = {.poolIndex = INPUT, .offset = 0, .length = static_cast<uint32_t>(s)},
                .dimensions = {},
            };
            RequestArgument arg_empty = {
                .hasNoValue = true,
            };
            inputs_info[index] = s ? arg : arg_empty;
            inputSize += s;
        });
        // Compute offset for inputs 1 and so on
        {
            size_t offset = 0;
            for (auto& i : inputs_info) {
                if (!i.hasNoValue) i.location.offset = offset;
                offset += i.location.length;
            }
        }

        // Go through all outputs, initialize RequestArgument descriptors
        for_all(outputs, [&outputs_info, &outputSize](int index, auto, auto s) {
            if (outputs_info.size() <= static_cast<size_t>(index)) outputs_info.resize(index + 1);
            RequestArgument arg = {
                .location = {.poolIndex = OUTPUT, .offset = 0, .length = static_cast<uint32_t>(s)},
                .dimensions = {},
            };
            outputs_info[index] = arg;
            outputSize += s;
        });
        // Compute offset for outputs 1 and so on
        {
            size_t offset = 0;
            for (auto& i : outputs_info) {
                i.location.offset = offset;
                offset += i.location.length;
            }
        }
        std::vector<hidl_memory> pools = {nn::allocateSharedMemory(inputSize),
                                          nn::allocateSharedMemory(outputSize)};
        if (pools[INPUT].size() == 0 || pools[OUTPUT].size() == 0) {
            return {};
        }

        // map pool
        sp<IMemory> inputMemory = mapMemory(pools[INPUT]);
        if (inputMemory == nullptr) {
            return {};
        }
        char* inputPtr = reinterpret_cast<char*>(static_cast<void*>(inputMemory->getPointer()));
        if (inputPtr == nullptr) {
            return {};
        }

        // initialize pool
        inputMemory->update();
        for_all(inputs, [&inputs_info, inputPtr](int index, auto p, auto s) {
            char* begin = (char*)p;
            char* end = begin + s;
            // TODO: handle more than one input
            std::copy(begin, end, inputPtr + inputs_info[index].location.offset);
        });
        inputMemory->commit();

        requests.push_back({.inputs = inputs_info, .outputs = outputs_info, .pools = pools});
    }

    return requests;
}

void ValidationTest::validateRequests(const sp<IPreparedModel>& preparedModel,
                                      const std::vector<Request>& requests) {
    // validate each request
    for (const Request& request : requests) {
        removeInputTest(preparedModel, request);
        removeOutputTest(preparedModel, request);
    }
}

}  // namespace functional
}  // namespace vts
}  // namespace V1_2
}  // namespace neuralnetworks
}  // namespace hardware
}  // namespace android
