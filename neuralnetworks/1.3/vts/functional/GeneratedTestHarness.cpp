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

#include "GeneratedTestHarness.h"

#include <android-base/logging.h>
#include <android/hardware/neuralnetworks/1.0/IDevice.h>
#include <android/hardware/neuralnetworks/1.0/IExecutionCallback.h>
#include <android/hardware/neuralnetworks/1.0/IPreparedModel.h>
#include <android/hardware/neuralnetworks/1.0/IPreparedModelCallback.h>
#include <android/hardware/neuralnetworks/1.0/types.h>
#include <android/hardware/neuralnetworks/1.1/IDevice.h>
#include <android/hardware/neuralnetworks/1.2/IDevice.h>
#include <android/hardware/neuralnetworks/1.2/IExecutionCallback.h>
#include <android/hardware/neuralnetworks/1.2/IPreparedModel.h>
#include <android/hardware/neuralnetworks/1.2/IPreparedModelCallback.h>
#include <android/hardware/neuralnetworks/1.2/types.h>
#include <android/hardware/neuralnetworks/1.3/IDevice.h>
#include <android/hardware/neuralnetworks/1.3/IFencedExecutionCallback.h>
#include <android/hardware/neuralnetworks/1.3/IPreparedModel.h>
#include <android/hardware/neuralnetworks/1.3/IPreparedModelCallback.h>
#include <android/hardware/neuralnetworks/1.3/types.h>
#include <android/hidl/allocator/1.0/IAllocator.h>
#include <android/hidl/memory/1.0/IMemory.h>
#include <android/sync.h>
#include <gtest/gtest.h>
#include <hidlmemory/mapping.h>

#include <algorithm>
#include <chrono>
#include <iostream>
#include <numeric>
#include <vector>

#include "1.0/Utils.h"
#include "1.3/Callbacks.h"
#include "1.3/Utils.h"
#include "ExecutionBurstController.h"
#include "MemoryUtils.h"
#include "TestHarness.h"
#include "Utils.h"
#include "VtsHalNeuralnetworks.h"

namespace android::hardware::neuralnetworks::V1_3::vts::functional {

using namespace test_helper;
using hidl::memory::V1_0::IMemory;
using implementation::ExecutionCallback;
using implementation::PreparedModelCallback;
using V1_0::DataLocation;
using V1_0::RequestArgument;
using V1_1::ExecutionPreference;
using V1_2::Constant;
using V1_2::MeasureTiming;
using V1_2::OutputShape;
using V1_2::SymmPerChannelQuantParams;
using V1_2::Timing;
using HidlToken = hidl_array<uint8_t, static_cast<uint32_t>(Constant::BYTE_SIZE_OF_CACHE_TOKEN)>;

namespace {

enum class Executor { ASYNC, SYNC, BURST, FENCED };

enum class OutputType { FULLY_SPECIFIED, UNSPECIFIED, INSUFFICIENT };

enum class MemoryType { SHARED, DEVICE };

enum class IOType { INPUT, OUTPUT };

static void waitForSyncFence(int syncFd) {
    constexpr int kInfiniteTimeout = -1;
    ASSERT_GT(syncFd, 0);
    int r = sync_wait(syncFd, kInfiniteTimeout);
    ASSERT_GE(r, 0);
}

struct TestConfig {
    Executor executor;
    MeasureTiming measureTiming;
    OutputType outputType;
    MemoryType memoryType;
    // `reportSkipping` indicates if a test should print an info message in case
    // it is skipped. The field is set to true by default and is set to false in
    // quantization coupling tests to suppress skipping a test
    bool reportSkipping;
    TestConfig(Executor executor, MeasureTiming measureTiming, OutputType outputType,
               MemoryType memoryType)
        : executor(executor),
          measureTiming(measureTiming),
          outputType(outputType),
          memoryType(memoryType),
          reportSkipping(true) {}
    TestConfig(Executor executor, MeasureTiming measureTiming, OutputType outputType,
               MemoryType memoryType, bool reportSkipping)
        : executor(executor),
          measureTiming(measureTiming),
          outputType(outputType),
          memoryType(memoryType),
          reportSkipping(reportSkipping) {}
};

class DeviceMemoryAllocator {
  public:
    DeviceMemoryAllocator(const sp<IDevice>& device, const sp<IPreparedModel>& preparedModel,
                          const TestModel& testModel)
        : kDevice(device), kPreparedModel(preparedModel), kTestModel(testModel) {}

    // Allocate device memory for a target input/output operand.
    // Return {IBuffer object, token} if successful.
    // Return {nullptr, 0} if device memory is not supported.
    template <IOType ioType>
    std::pair<sp<IBuffer>, uint32_t> allocate(uint32_t index) {
        std::pair<sp<IBuffer>, uint32_t> buffer;
        allocateInternal<ioType>(index, &buffer);
        return buffer;
    }

  private:
    template <IOType ioType>
    void allocateInternal(uint32_t index, std::pair<sp<IBuffer>, uint32_t>* result) {
        ASSERT_NE(result, nullptr);

        // Prepare arguments.
        BufferRole role = {.modelIndex = 0, .ioIndex = index, .frequency = 1.0f};
        hidl_vec<BufferRole> inputRoles, outputRoles;
        if constexpr (ioType == IOType::INPUT) {
            inputRoles = {role};
        } else {
            outputRoles = {role};
        }

        // Allocate device memory.
        ErrorStatus status;
        sp<IBuffer> buffer;
        uint32_t token;
        auto cb = [&status, &buffer, &token](ErrorStatus error, const sp<IBuffer>& buf,
                                             uint32_t tok) {
            status = error;
            buffer = buf;
            token = tok;
        };
        const auto ret = kDevice->allocate({}, {kPreparedModel}, inputRoles, outputRoles, cb);

        // Check allocation results.
        ASSERT_TRUE(ret.isOk());
        if (status == ErrorStatus::NONE) {
            ASSERT_NE(buffer, nullptr);
            ASSERT_GT(token, 0);
        } else {
            ASSERT_EQ(status, ErrorStatus::GENERAL_FAILURE);
            ASSERT_EQ(buffer, nullptr);
            ASSERT_EQ(token, 0);
        }

        // Initialize input data from TestBuffer.
        if constexpr (ioType == IOType::INPUT) {
            if (buffer != nullptr) {
                // TestBuffer -> Shared memory.
                const auto& testBuffer = kTestModel.operands[kTestModel.inputIndexes[index]].data;
                ASSERT_GT(testBuffer.size(), 0);
                hidl_memory tmp = nn::allocateSharedMemory(testBuffer.size());
                sp<IMemory> inputMemory = mapMemory(tmp);
                ASSERT_NE(inputMemory.get(), nullptr);
                uint8_t* inputPtr =
                        static_cast<uint8_t*>(static_cast<void*>(inputMemory->getPointer()));
                ASSERT_NE(inputPtr, nullptr);
                const uint8_t* begin = testBuffer.get<uint8_t>();
                const uint8_t* end = begin + testBuffer.size();
                std::copy(begin, end, inputPtr);

                // Shared memory -> IBuffer.
                auto ret = buffer->copyFrom(tmp, {});
                ASSERT_TRUE(ret.isOk());
                ASSERT_EQ(static_cast<ErrorStatus>(ret), ErrorStatus::NONE);
            }
        }
        *result = {std::move(buffer), token};
    }

    const sp<IDevice> kDevice;
    const sp<IPreparedModel> kPreparedModel;
    const TestModel& kTestModel;
};

}  // namespace

Model createModel(const TestModel& testModel) {
    // Model operands.
    hidl_vec<Operand> operands(testModel.operands.size());
    size_t constCopySize = 0, constRefSize = 0;
    for (uint32_t i = 0; i < testModel.operands.size(); i++) {
        const auto& op = testModel.operands[i];

        DataLocation loc = {};
        if (op.lifetime == TestOperandLifeTime::CONSTANT_COPY) {
            loc = {.poolIndex = 0,
                   .offset = static_cast<uint32_t>(constCopySize),
                   .length = static_cast<uint32_t>(op.data.size())};
            constCopySize += op.data.alignedSize();
        } else if (op.lifetime == TestOperandLifeTime::CONSTANT_REFERENCE) {
            loc = {.poolIndex = 0,
                   .offset = static_cast<uint32_t>(constRefSize),
                   .length = static_cast<uint32_t>(op.data.size())};
            constRefSize += op.data.alignedSize();
        }

        V1_2::Operand::ExtraParams extraParams;
        if (op.type == TestOperandType::TENSOR_QUANT8_SYMM_PER_CHANNEL) {
            extraParams.channelQuant(SymmPerChannelQuantParams{
                    .scales = op.channelQuant.scales, .channelDim = op.channelQuant.channelDim});
        }

        operands[i] = {.type = static_cast<OperandType>(op.type),
                       .dimensions = op.dimensions,
                       .numberOfConsumers = op.numberOfConsumers,
                       .scale = op.scale,
                       .zeroPoint = op.zeroPoint,
                       .lifetime = static_cast<OperandLifeTime>(op.lifetime),
                       .location = loc,
                       .extraParams = std::move(extraParams)};
    }

    // Model operations.
    hidl_vec<Operation> operations(testModel.operations.size());
    std::transform(testModel.operations.begin(), testModel.operations.end(), operations.begin(),
                   [](const TestOperation& op) -> Operation {
                       return {.type = static_cast<OperationType>(op.type),
                               .inputs = op.inputs,
                               .outputs = op.outputs};
                   });

    // Constant copies.
    hidl_vec<uint8_t> operandValues(constCopySize);
    for (uint32_t i = 0; i < testModel.operands.size(); i++) {
        const auto& op = testModel.operands[i];
        if (op.lifetime == TestOperandLifeTime::CONSTANT_COPY) {
            const uint8_t* begin = op.data.get<uint8_t>();
            const uint8_t* end = begin + op.data.size();
            std::copy(begin, end, operandValues.data() + operands[i].location.offset);
        }
    }

    // Shared memory.
    hidl_vec<hidl_memory> pools = {};
    if (constRefSize > 0) {
        hidl_vec_push_back(&pools, nn::allocateSharedMemory(constRefSize));
        CHECK_NE(pools[0].size(), 0u);

        // load data
        sp<IMemory> mappedMemory = mapMemory(pools[0]);
        CHECK(mappedMemory.get() != nullptr);
        uint8_t* mappedPtr =
                reinterpret_cast<uint8_t*>(static_cast<void*>(mappedMemory->getPointer()));
        CHECK(mappedPtr != nullptr);

        for (uint32_t i = 0; i < testModel.operands.size(); i++) {
            const auto& op = testModel.operands[i];
            if (op.lifetime == TestOperandLifeTime::CONSTANT_REFERENCE) {
                const uint8_t* begin = op.data.get<uint8_t>();
                const uint8_t* end = begin + op.data.size();
                std::copy(begin, end, mappedPtr + operands[i].location.offset);
            }
        }
    }

    return {.main = {.operands = std::move(operands),
                     .operations = std::move(operations),
                     .inputIndexes = testModel.inputIndexes,
                     .outputIndexes = testModel.outputIndexes},
            .operandValues = std::move(operandValues),
            .pools = std::move(pools),
            .relaxComputationFloat32toFloat16 = testModel.isRelaxed};
}

static bool isOutputSizeGreaterThanOne(const TestModel& testModel, uint32_t index) {
    const auto byteSize = testModel.operands[testModel.outputIndexes[index]].data.size();
    return byteSize > 1u;
}

static void makeOutputInsufficientSize(uint32_t outputIndex, Request* request) {
    auto& length = request->outputs[outputIndex].location.length;
    ASSERT_GT(length, 1u);
    length -= 1u;
}

static void makeOutputDimensionsUnspecified(Model* model) {
    for (auto i : model->main.outputIndexes) {
        auto& dims = model->main.operands[i].dimensions;
        std::fill(dims.begin(), dims.end(), 0);
    }
}

constexpr uint32_t kInputPoolIndex = 0;
constexpr uint32_t kOutputPoolIndex = 1;
constexpr uint32_t kDeviceMemoryBeginIndex = 2;

static std::pair<Request, std::vector<sp<IBuffer>>> createRequest(
        const sp<IDevice>& device, const sp<IPreparedModel>& preparedModel,
        const TestModel& testModel, bool preferDeviceMemory) {
    // Memory pools are organized as:
    // - 0: Input shared memory pool
    // - 1: Output shared memory pool
    // - [2, 2+i): Input device memories
    // - [2+i, 2+i+o): Output device memories
    DeviceMemoryAllocator allocator(device, preparedModel, testModel);
    std::vector<sp<IBuffer>> buffers;
    std::vector<uint32_t> tokens;

    // Model inputs.
    hidl_vec<RequestArgument> inputs(testModel.inputIndexes.size());
    size_t inputSize = 0;
    for (uint32_t i = 0; i < testModel.inputIndexes.size(); i++) {
        const auto& op = testModel.operands[testModel.inputIndexes[i]];
        if (op.data.size() == 0) {
            // Omitted input.
            inputs[i] = {.hasNoValue = true};
            continue;
        } else if (preferDeviceMemory) {
            SCOPED_TRACE("Input index = " + std::to_string(i));
            auto [buffer, token] = allocator.allocate<IOType::INPUT>(i);
            if (buffer != nullptr) {
                DataLocation loc = {.poolIndex = static_cast<uint32_t>(buffers.size() +
                                                                       kDeviceMemoryBeginIndex)};
                buffers.push_back(std::move(buffer));
                tokens.push_back(token);
                inputs[i] = {.hasNoValue = false, .location = loc, .dimensions = {}};
                continue;
            }
        }

        // Reserve shared memory for input.
        DataLocation loc = {.poolIndex = kInputPoolIndex,
                            .offset = static_cast<uint32_t>(inputSize),
                            .length = static_cast<uint32_t>(op.data.size())};
        inputSize += op.data.alignedSize();
        inputs[i] = {.hasNoValue = false, .location = loc, .dimensions = {}};
    }

    // Model outputs.
    hidl_vec<RequestArgument> outputs(testModel.outputIndexes.size());
    size_t outputSize = 0;
    for (uint32_t i = 0; i < testModel.outputIndexes.size(); i++) {
        const auto& op = testModel.operands[testModel.outputIndexes[i]];
        if (preferDeviceMemory) {
            SCOPED_TRACE("Output index = " + std::to_string(i));
            auto [buffer, token] = allocator.allocate<IOType::OUTPUT>(i);
            if (buffer != nullptr) {
                DataLocation loc = {.poolIndex = static_cast<uint32_t>(buffers.size() +
                                                                       kDeviceMemoryBeginIndex)};
                buffers.push_back(std::move(buffer));
                tokens.push_back(token);
                outputs[i] = {.hasNoValue = false, .location = loc, .dimensions = {}};
                continue;
            }
        }

        // In the case of zero-sized output, we should at least provide a one-byte buffer.
        // This is because zero-sized tensors are only supported internally to the driver, or
        // reported in output shapes. It is illegal for the client to pre-specify a zero-sized
        // tensor as model output. Otherwise, we will have two semantic conflicts:
        // - "Zero dimension" conflicts with "unspecified dimension".
        // - "Omitted operand buffer" conflicts with "zero-sized operand buffer".
        size_t bufferSize = std::max<size_t>(op.data.size(), 1);

        // Reserve shared memory for output.
        DataLocation loc = {.poolIndex = kOutputPoolIndex,
                            .offset = static_cast<uint32_t>(outputSize),
                            .length = static_cast<uint32_t>(bufferSize)};
        outputSize += op.data.size() == 0 ? TestBuffer::kAlignment : op.data.alignedSize();
        outputs[i] = {.hasNoValue = false, .location = loc, .dimensions = {}};
    }

    // Memory pools.
    hidl_vec<Request::MemoryPool> pools(kDeviceMemoryBeginIndex + buffers.size());
    pools[kInputPoolIndex].hidlMemory(nn::allocateSharedMemory(std::max<size_t>(inputSize, 1)));
    pools[kOutputPoolIndex].hidlMemory(nn::allocateSharedMemory(std::max<size_t>(outputSize, 1)));
    CHECK_NE(pools[kInputPoolIndex].hidlMemory().size(), 0u);
    CHECK_NE(pools[kOutputPoolIndex].hidlMemory().size(), 0u);
    for (uint32_t i = 0; i < buffers.size(); i++) {
        pools[kDeviceMemoryBeginIndex + i].token(tokens[i]);
    }

    // Copy input data to the input shared memory pool.
    sp<IMemory> inputMemory = mapMemory(pools[kInputPoolIndex].hidlMemory());
    CHECK(inputMemory.get() != nullptr);
    uint8_t* inputPtr = static_cast<uint8_t*>(static_cast<void*>(inputMemory->getPointer()));
    CHECK(inputPtr != nullptr);
    for (uint32_t i = 0; i < testModel.inputIndexes.size(); i++) {
        if (!inputs[i].hasNoValue && inputs[i].location.poolIndex == kInputPoolIndex) {
            const auto& op = testModel.operands[testModel.inputIndexes[i]];
            const uint8_t* begin = op.data.get<uint8_t>();
            const uint8_t* end = begin + op.data.size();
            std::copy(begin, end, inputPtr + inputs[i].location.offset);
        }
    }

    Request request = {
            .inputs = std::move(inputs), .outputs = std::move(outputs), .pools = std::move(pools)};
    return {std::move(request), std::move(buffers)};
}

// Get a TestBuffer with data copied from an IBuffer object.
static void getBuffer(const sp<IBuffer>& buffer, size_t size, TestBuffer* testBuffer) {
    // IBuffer -> Shared memory.
    hidl_memory tmp = nn::allocateSharedMemory(size);
    const auto ret = buffer->copyTo(tmp);
    ASSERT_TRUE(ret.isOk());
    ASSERT_EQ(static_cast<ErrorStatus>(ret), ErrorStatus::NONE);

    // Shared memory -> TestBuffer.
    sp<IMemory> outputMemory = mapMemory(tmp);
    ASSERT_NE(outputMemory.get(), nullptr);
    uint8_t* outputPtr = static_cast<uint8_t*>(static_cast<void*>(outputMemory->getPointer()));
    ASSERT_NE(outputPtr, nullptr);
    ASSERT_NE(testBuffer, nullptr);
    *testBuffer = TestBuffer(size, outputPtr);
}

static std::vector<TestBuffer> getOutputBuffers(const TestModel& testModel, const Request& request,
                                                const std::vector<sp<IBuffer>>& buffers) {
    sp<IMemory> outputMemory = mapMemory(request.pools[kOutputPoolIndex].hidlMemory());
    CHECK(outputMemory.get() != nullptr);
    uint8_t* outputPtr = static_cast<uint8_t*>(static_cast<void*>(outputMemory->getPointer()));
    CHECK(outputPtr != nullptr);

    // Copy out output results.
    std::vector<TestBuffer> outputBuffers;
    for (uint32_t i = 0; i < request.outputs.size(); i++) {
        const auto& outputLoc = request.outputs[i].location;
        if (outputLoc.poolIndex == kOutputPoolIndex) {
            outputBuffers.emplace_back(outputLoc.length, outputPtr + outputLoc.offset);
        } else {
            const auto& op = testModel.operands[testModel.outputIndexes[i]];
            if (op.data.size() == 0) {
                outputBuffers.emplace_back();
            } else {
                SCOPED_TRACE("Output index = " + std::to_string(i));
                const uint32_t bufferIndex = outputLoc.poolIndex - kDeviceMemoryBeginIndex;
                TestBuffer buffer;
                getBuffer(buffers[bufferIndex], op.data.size(), &buffer);
                outputBuffers.push_back(std::move(buffer));
            }
        }
    }
    return outputBuffers;
}

static Return<ErrorStatus> ExecutePreparedModel(const sp<IPreparedModel>& preparedModel,
                                                const Request& request, MeasureTiming measure,
                                                sp<ExecutionCallback>& callback) {
    return preparedModel->execute_1_3(request, measure, {}, callback);
}
static Return<ErrorStatus> ExecutePreparedModel(const sp<IPreparedModel>& preparedModel,
                                                const Request& request, MeasureTiming measure,
                                                hidl_vec<OutputShape>* outputShapes,
                                                Timing* timing) {
    ErrorStatus result;
    Return<void> ret = preparedModel->executeSynchronously_1_3(
            request, measure, {},
            [&result, outputShapes, timing](ErrorStatus error, const hidl_vec<OutputShape>& shapes,
                                            const Timing& time) {
                result = error;
                *outputShapes = shapes;
                *timing = time;
            });
    if (!ret.isOk()) {
        return ErrorStatus::GENERAL_FAILURE;
    }
    return result;
}
static std::shared_ptr<::android::nn::ExecutionBurstController> CreateBurst(
        const sp<IPreparedModel>& preparedModel) {
    return android::nn::ExecutionBurstController::create(preparedModel,
                                                         std::chrono::microseconds{0});
}

void EvaluatePreparedModel(const sp<IDevice>& device, const sp<IPreparedModel>& preparedModel,
                           const TestModel& testModel, const TestConfig& testConfig,
                           bool* skipped = nullptr) {
    if (skipped != nullptr) {
        *skipped = false;
    }
    // If output0 does not have size larger than one byte, we can not test with insufficient buffer.
    if (testConfig.outputType == OutputType::INSUFFICIENT &&
        !isOutputSizeGreaterThanOne(testModel, 0)) {
        return;
    }

    auto [request, buffers] =
            createRequest(device, preparedModel, testModel,
                          /*preferDeviceMemory=*/testConfig.memoryType == MemoryType::DEVICE);
    // Skip if testing memory domain but no device memory has been allocated.
    if (testConfig.memoryType == MemoryType::DEVICE && buffers.empty()) {
        return;
    }
    if (testConfig.outputType == OutputType::INSUFFICIENT) {
        makeOutputInsufficientSize(/*outputIndex=*/0, &request);
    }

    ErrorStatus executionStatus;
    hidl_vec<OutputShape> outputShapes;
    Timing timing;
    switch (testConfig.executor) {
        case Executor::ASYNC: {
            SCOPED_TRACE("asynchronous");

            // launch execution
            sp<ExecutionCallback> executionCallback = new ExecutionCallback();
            Return<ErrorStatus> executionLaunchStatus = ExecutePreparedModel(
                    preparedModel, request, testConfig.measureTiming, executionCallback);
            ASSERT_TRUE(executionLaunchStatus.isOk());
            EXPECT_EQ(ErrorStatus::NONE, static_cast<ErrorStatus>(executionLaunchStatus));

            // retrieve execution status
            executionCallback->wait();
            executionStatus = executionCallback->getStatus();
            outputShapes = executionCallback->getOutputShapes();
            timing = executionCallback->getTiming();

            break;
        }
        case Executor::SYNC: {
            SCOPED_TRACE("synchronous");

            // execute
            Return<ErrorStatus> executionReturnStatus = ExecutePreparedModel(
                    preparedModel, request, testConfig.measureTiming, &outputShapes, &timing);
            ASSERT_TRUE(executionReturnStatus.isOk());
            executionStatus = static_cast<ErrorStatus>(executionReturnStatus);

            break;
        }
        case Executor::BURST: {
            // TODO(butlermichael): Check if we need to test burst in V1_3 if the interface remains
            //                      V1_2.
            SCOPED_TRACE("burst");

            // check compliance
            ASSERT_TRUE(nn::compliantWithV1_0(request));
            V1_0::Request request10 = nn::convertToV1_0(request);

            // create burst
            const std::shared_ptr<::android::nn::ExecutionBurstController> controller =
                    CreateBurst(preparedModel);
            ASSERT_NE(nullptr, controller.get());

            // create memory keys
            std::vector<intptr_t> keys(request10.pools.size());
            for (size_t i = 0; i < keys.size(); ++i) {
                keys[i] = reinterpret_cast<intptr_t>(&request10.pools[i]);
            }

            // execute burst
            int n;
            std::tie(n, outputShapes, timing, std::ignore) =
                    controller->compute(request10, testConfig.measureTiming, keys);
            executionStatus = nn::convertResultCodeToErrorStatus(n);

            break;
        }
        case Executor::FENCED: {
            SCOPED_TRACE("fenced");
            ErrorStatus result;
            hidl_handle syncFenceHandle;
            sp<IFencedExecutionCallback> fencedCallback;
            Return<void> ret = preparedModel->executeFenced(
                    request, {}, testConfig.measureTiming, {}, {},
                    [&result, &syncFenceHandle, &fencedCallback](
                            ErrorStatus error, const hidl_handle& handle,
                            const sp<IFencedExecutionCallback>& callback) {
                        result = error;
                        syncFenceHandle = handle;
                        fencedCallback = callback;
                    });
            ASSERT_TRUE(ret.isOk());
            if (result != ErrorStatus::NONE) {
                ASSERT_EQ(syncFenceHandle.getNativeHandle(), nullptr);
                ASSERT_EQ(fencedCallback, nullptr);
                executionStatus = ErrorStatus::GENERAL_FAILURE;
            } else if (syncFenceHandle.getNativeHandle()) {
                waitForSyncFence(syncFenceHandle.getNativeHandle()->data[0]);
            }
            if (result == ErrorStatus::NONE) {
                ASSERT_NE(fencedCallback, nullptr);
                Return<void> ret = fencedCallback->getExecutionInfo(
                        [&executionStatus, &timing](ErrorStatus error, Timing t, Timing) {
                            executionStatus = error;
                            timing = t;
                        });
                ASSERT_TRUE(ret.isOk());
            }
            break;
        }
    }

    // The driver is allowed to reject executeFenced, and if they do, we should skip.
    if ((testConfig.outputType != OutputType::FULLY_SPECIFIED ||
         testConfig.executor == Executor::FENCED) &&
        executionStatus == ErrorStatus::GENERAL_FAILURE) {
        if (skipped != nullptr) {
            *skipped = true;
        }
        if (!testConfig.reportSkipping) {
            return;
        }
        LOG(INFO) << "NN VTS: Early termination of test because vendor service cannot "
                     "execute model that it does not support.";
        std::cout << "[          ]   Early termination of test because vendor service cannot "
                     "execute model that it does not support."
                  << std::endl;
        GTEST_SKIP();
    }
    if (testConfig.measureTiming == MeasureTiming::NO) {
        EXPECT_EQ(UINT64_MAX, timing.timeOnDevice);
        EXPECT_EQ(UINT64_MAX, timing.timeInDriver);
    } else {
        if (timing.timeOnDevice != UINT64_MAX && timing.timeInDriver != UINT64_MAX) {
            EXPECT_LE(timing.timeOnDevice, timing.timeInDriver);
        }
    }

    switch (testConfig.outputType) {
        case OutputType::FULLY_SPECIFIED:
            // If the model output operands are fully specified, outputShapes must be either
            // either empty, or have the same number of elements as the number of outputs.
            ASSERT_EQ(ErrorStatus::NONE, executionStatus);
            ASSERT_TRUE(outputShapes.size() == 0 ||
                        outputShapes.size() == testModel.outputIndexes.size());
            break;
        case OutputType::UNSPECIFIED:
            // If the model output operands are not fully specified, outputShapes must have
            // the same number of elements as the number of outputs.
            ASSERT_EQ(ErrorStatus::NONE, executionStatus);
            ASSERT_EQ(outputShapes.size(), testModel.outputIndexes.size());
            break;
        case OutputType::INSUFFICIENT:
            ASSERT_EQ(ErrorStatus::OUTPUT_INSUFFICIENT_SIZE, executionStatus);
            ASSERT_EQ(outputShapes.size(), testModel.outputIndexes.size());
            ASSERT_FALSE(outputShapes[0].isSufficient);
            return;
    }

    // Go through all outputs, check returned output shapes.
    for (uint32_t i = 0; i < outputShapes.size(); i++) {
        EXPECT_TRUE(outputShapes[i].isSufficient);
        const auto& expect = testModel.operands[testModel.outputIndexes[i]].dimensions;
        const std::vector<uint32_t> actual = outputShapes[i].dimensions;
        EXPECT_EQ(expect, actual);
    }

    // Retrieve execution results.
    const std::vector<TestBuffer> outputs = getOutputBuffers(testModel, request, buffers);

    // We want "close-enough" results.
    checkResults(testModel, outputs);
}

void EvaluatePreparedModel(const sp<IDevice>& device, const sp<IPreparedModel>& preparedModel,
                           const TestModel& testModel, TestKind testKind) {
    std::vector<OutputType> outputTypesList;
    std::vector<MeasureTiming> measureTimingList;
    std::vector<Executor> executorList;
    MemoryType memoryType = MemoryType::SHARED;

    switch (testKind) {
        case TestKind::GENERAL: {
            outputTypesList = {OutputType::FULLY_SPECIFIED};
            measureTimingList = {MeasureTiming::NO, MeasureTiming::YES};
            executorList = {Executor::ASYNC, Executor::SYNC, Executor::BURST};
        } break;
        case TestKind::DYNAMIC_SHAPE: {
            outputTypesList = {OutputType::UNSPECIFIED, OutputType::INSUFFICIENT};
            measureTimingList = {MeasureTiming::NO, MeasureTiming::YES};
            executorList = {Executor::ASYNC, Executor::SYNC, Executor::BURST};
        } break;
        case TestKind::MEMORY_DOMAIN: {
            outputTypesList = {OutputType::FULLY_SPECIFIED};
            measureTimingList = {MeasureTiming::NO};
            executorList = {Executor::ASYNC, Executor::SYNC};
            memoryType = MemoryType::DEVICE;
        } break;
        case TestKind::FENCED_COMPUTE: {
            outputTypesList = {OutputType::FULLY_SPECIFIED};
            measureTimingList = {MeasureTiming::NO, MeasureTiming::YES};
            executorList = {Executor::FENCED};
        } break;
        case TestKind::QUANTIZATION_COUPLING: {
            LOG(FATAL) << "Wrong TestKind for EvaluatePreparedModel";
            return;
        } break;
    }

    for (const OutputType outputType : outputTypesList) {
        for (const MeasureTiming measureTiming : measureTimingList) {
            for (const Executor executor : executorList) {
                const TestConfig testConfig(executor, measureTiming, outputType, memoryType);
                EvaluatePreparedModel(device, preparedModel, testModel, testConfig);
            }
        }
    }
}

void EvaluatePreparedCoupledModels(const sp<IDevice>& device,
                                   const sp<IPreparedModel>& preparedModel,
                                   const TestModel& testModel,
                                   const sp<IPreparedModel>& preparedCoupledModel,
                                   const TestModel& coupledModel) {
    const std::vector<OutputType> outputTypesList = {OutputType::FULLY_SPECIFIED};
    const std::vector<MeasureTiming> measureTimingList = {MeasureTiming::NO, MeasureTiming::YES};
    const std::vector<Executor> executorList = {Executor::ASYNC, Executor::SYNC, Executor::BURST,
                                                Executor::FENCED};

    for (const OutputType outputType : outputTypesList) {
        for (const MeasureTiming measureTiming : measureTimingList) {
            for (const Executor executor : executorList) {
                const TestConfig testConfig(executor, measureTiming, outputType, MemoryType::SHARED,
                                            /*reportSkipping=*/false);
                bool baseSkipped = false;
                EvaluatePreparedModel(device, preparedModel, testModel, testConfig, &baseSkipped);
                bool coupledSkipped = false;
                EvaluatePreparedModel(device, preparedCoupledModel, coupledModel, testConfig,
                                      &coupledSkipped);
                ASSERT_EQ(baseSkipped, coupledSkipped);
                if (baseSkipped) {
                    LOG(INFO) << "NN VTS: Early termination of test because vendor service cannot "
                                 "execute model that it does not support.";
                    std::cout << "[          ]   Early termination of test because vendor service "
                                 "cannot "
                                 "execute model that it does not support."
                              << std::endl;
                    GTEST_SKIP();
                }
            }
        }
    }
}

void Execute(const sp<IDevice>& device, const TestModel& testModel, TestKind testKind) {
    Model model = createModel(testModel);
    if (testKind == TestKind::DYNAMIC_SHAPE) {
        makeOutputDimensionsUnspecified(&model);
    }

    sp<IPreparedModel> preparedModel;
    switch (testKind) {
        case TestKind::GENERAL:
        case TestKind::DYNAMIC_SHAPE:
        case TestKind::MEMORY_DOMAIN:
        case TestKind::FENCED_COMPUTE: {
            createPreparedModel(device, model, &preparedModel);
            if (preparedModel == nullptr) return;
            EvaluatePreparedModel(device, preparedModel, testModel, testKind);
        } break;
        case TestKind::QUANTIZATION_COUPLING: {
            ASSERT_TRUE(testModel.hasQuant8CoupledOperands());
            createPreparedModel(device, model, &preparedModel,
                                /*reportSkipping*/ false);
            TestModel signedQuantizedModel = convertQuant8AsymmOperandsToSigned(testModel);
            sp<IPreparedModel> preparedCoupledModel;
            createPreparedModel(device, createModel(signedQuantizedModel), &preparedCoupledModel,
                                /*reportSkipping*/ false);
            // If we couldn't prepare a model with unsigned quantization, we must
            // fail to prepare a model with signed quantization as well.
            if (preparedModel == nullptr) {
                ASSERT_EQ(preparedCoupledModel, nullptr);
                // If we failed to prepare both of the models, we can safely skip
                // the test.
                LOG(INFO) << "NN VTS: Early termination of test because vendor service cannot "
                             "prepare model that it does not support.";
                std::cout
                        << "[          ]   Early termination of test because vendor service cannot "
                           "prepare model that it does not support."
                        << std::endl;
                GTEST_SKIP();
            }
            ASSERT_NE(preparedCoupledModel, nullptr);
            EvaluatePreparedCoupledModels(device, preparedModel, testModel, preparedCoupledModel,
                                          signedQuantizedModel);
        } break;
    }
}

void GeneratedTestBase::SetUp() {
    testing::TestWithParam<GeneratedTestParam>::SetUp();
    ASSERT_NE(kDevice, nullptr);

    const Return<void> ret =
            kDevice->supportsDeadlines([this](bool prepareModelDeadline, bool executionDeadline) {
                mSupportsDeadlines = {prepareModelDeadline, executionDeadline};
            });
    ASSERT_TRUE(ret.isOk());
}

std::vector<NamedModel> getNamedModels(const FilterFn& filter) {
    return TestModelManager::get().getTestModels(filter);
}

std::string printGeneratedTest(const testing::TestParamInfo<GeneratedTestParam>& info) {
    const auto& [namedDevice, namedModel] = info.param;
    return gtestCompliantName(getName(namedDevice) + "_" + getName(namedModel));
}

// Tag for the generated tests
class GeneratedTest : public GeneratedTestBase {};

// Tag for the dynamic output shape tests
class DynamicOutputShapeTest : public GeneratedTest {};

// Tag for the memory domain tests
class MemoryDomainTest : public GeneratedTest {};

// Tag for the fenced compute tests
class FencedComputeTest : public GeneratedTest {};

// Tag for the dynamic output shape tests
class QuantizationCouplingTest : public GeneratedTest {};

TEST_P(GeneratedTest, Test) {
    Execute(kDevice, kTestModel, /*testKind=*/TestKind::GENERAL);
}

TEST_P(DynamicOutputShapeTest, Test) {
    Execute(kDevice, kTestModel, /*testKind=*/TestKind::DYNAMIC_SHAPE);
}

TEST_P(MemoryDomainTest, Test) {
    Execute(kDevice, kTestModel, /*testKind=*/TestKind::MEMORY_DOMAIN);
}

TEST_P(FencedComputeTest, Test) {
    Execute(kDevice, kTestModel, /*testKind=*/TestKind::FENCED_COMPUTE);
}

TEST_P(QuantizationCouplingTest, Test) {
    Execute(kDevice, kTestModel, /*testKind=*/TestKind::QUANTIZATION_COUPLING);
}

INSTANTIATE_GENERATED_TEST(GeneratedTest,
                           [](const TestModel& testModel) { return !testModel.expectFailure; });

INSTANTIATE_GENERATED_TEST(DynamicOutputShapeTest, [](const TestModel& testModel) {
    return !testModel.expectFailure && !testModel.hasScalarOutputs();
});

INSTANTIATE_GENERATED_TEST(MemoryDomainTest,
                           [](const TestModel& testModel) { return !testModel.expectFailure; });

INSTANTIATE_GENERATED_TEST(FencedComputeTest,
                           [](const TestModel& testModel) { return !testModel.expectFailure; });

INSTANTIATE_GENERATED_TEST(QuantizationCouplingTest, [](const TestModel& testModel) {
    return testModel.hasQuant8CoupledOperands() && testModel.operations.size() == 1;
});

}  // namespace android::hardware::neuralnetworks::V1_3::vts::functional
