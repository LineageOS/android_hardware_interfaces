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

#include "GeneratedTestHarness.h"

#include <aidl/android/hardware/neuralnetworks/ErrorStatus.h>
#include <aidl/android/hardware/neuralnetworks/RequestMemoryPool.h>
#include <android-base/logging.h>
#include <android/binder_auto_utils.h>
#include <android/sync.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <iostream>
#include <iterator>
#include <numeric>
#include <vector>

#include <MemoryUtils.h>
#include <android/binder_status.h>
#include <nnapi/Result.h>
#include <nnapi/SharedMemory.h>
#include <nnapi/Types.h>
#include <nnapi/hal/aidl/Conversions.h>
#include <nnapi/hal/aidl/Utils.h>

#include "Callbacks.h"
#include "TestHarness.h"
#include "Utils.h"
#include "VtsHalNeuralnetworks.h"

namespace aidl::android::hardware::neuralnetworks::vts::functional {

namespace nn = ::android::nn;
using namespace test_helper;
using implementation::PreparedModelCallback;

namespace {

enum class OutputType { FULLY_SPECIFIED, UNSPECIFIED, INSUFFICIENT, MISSED_DEADLINE };

struct TestConfig {
    Executor executor;
    bool measureTiming;
    OutputType outputType;
    MemoryType memoryType;
    // `reportSkipping` indicates if a test should print an info message in case
    // it is skipped. The field is set to true by default and is set to false in
    // quantization coupling tests to suppress skipping a test
    bool reportSkipping;
    TestConfig(Executor executor, bool measureTiming, OutputType outputType, MemoryType memoryType)
        : executor(executor),
          measureTiming(measureTiming),
          outputType(outputType),
          memoryType(memoryType),
          reportSkipping(true) {}
    TestConfig(Executor executor, bool measureTiming, OutputType outputType, MemoryType memoryType,
               bool reportSkipping)
        : executor(executor),
          measureTiming(measureTiming),
          outputType(outputType),
          memoryType(memoryType),
          reportSkipping(reportSkipping) {}
};

enum class IOType { INPUT, OUTPUT };

class DeviceMemoryAllocator {
  public:
    DeviceMemoryAllocator(const std::shared_ptr<IDevice>& device,
                          const std::shared_ptr<IPreparedModel>& preparedModel,
                          const TestModel& testModel)
        : kDevice(device), kPreparedModel(preparedModel), kTestModel(testModel) {}

    // Allocate device memory for a target input/output operand.
    // Return {IBuffer object, token} if successful.
    // Return {nullptr, 0} if device memory is not supported.
    template <IOType ioType>
    std::pair<std::shared_ptr<IBuffer>, int32_t> allocate(uint32_t index) {
        std::pair<std::shared_ptr<IBuffer>, int32_t> buffer;
        allocateInternal<ioType>(index, &buffer);
        return buffer;
    }

  private:
    template <IOType ioType>
    void allocateInternal(int32_t index, std::pair<std::shared_ptr<IBuffer>, int32_t>* result) {
        ASSERT_NE(result, nullptr);

        // Prepare arguments.
        BufferRole role = {.modelIndex = 0, .ioIndex = index, .probability = 1.0f};
        std::vector<BufferRole> inputRoles, outputRoles;
        if constexpr (ioType == IOType::INPUT) {
            inputRoles = {role};
        } else {
            outputRoles = {role};
        }

        // Allocate device memory.
        DeviceBuffer buffer;
        IPreparedModelParcel parcel;
        parcel.preparedModel = kPreparedModel;
        const auto ret = kDevice->allocate({}, {parcel}, inputRoles, outputRoles, &buffer);

        // Check allocation results.
        if (ret.isOk()) {
            ASSERT_NE(buffer.buffer, nullptr);
            ASSERT_GT(buffer.token, 0);
        } else {
            ASSERT_EQ(ret.getExceptionCode(), EX_SERVICE_SPECIFIC);
            ASSERT_EQ(static_cast<ErrorStatus>(ret.getServiceSpecificError()),
                      ErrorStatus::GENERAL_FAILURE);
            buffer.buffer = nullptr;
            buffer.token = 0;
        }

        // Initialize input data from TestBuffer.
        if constexpr (ioType == IOType::INPUT) {
            if (buffer.buffer != nullptr) {
                // TestBuffer -> Shared memory.
                const auto& testBuffer =
                        kTestModel.main.operands[kTestModel.main.inputIndexes[index]].data;
                ASSERT_GT(testBuffer.size(), 0);
                const auto sharedMemory = nn::createSharedMemory(testBuffer.size()).value();
                const auto memory = utils::convert(sharedMemory).value();
                const auto mapping = nn::map(sharedMemory).value();
                uint8_t* inputPtr = static_cast<uint8_t*>(std::get<void*>(mapping.pointer));
                ASSERT_NE(inputPtr, nullptr);
                const uint8_t* begin = testBuffer.get<uint8_t>();
                const uint8_t* end = begin + testBuffer.size();
                std::copy(begin, end, inputPtr);

                // Shared memory -> IBuffer.
                auto ret = buffer.buffer->copyFrom(memory, {});
                ASSERT_TRUE(ret.isOk());
            }
        }
        *result = {std::move(buffer.buffer), buffer.token};
    }

    const std::shared_ptr<IDevice> kDevice;
    const std::shared_ptr<IPreparedModel> kPreparedModel;
    const TestModel& kTestModel;
};

Subgraph createSubgraph(const TestSubgraph& testSubgraph, uint32_t* constCopySize,
                        std::vector<const TestBuffer*>* constCopies, uint32_t* constRefSize,
                        std::vector<const TestBuffer*>* constReferences) {
    CHECK(constCopySize != nullptr);
    CHECK(constCopies != nullptr);
    CHECK(constRefSize != nullptr);
    CHECK(constReferences != nullptr);

    // Operands.
    std::vector<Operand> operands(testSubgraph.operands.size());
    for (uint32_t i = 0; i < testSubgraph.operands.size(); i++) {
        const auto& op = testSubgraph.operands[i];

        DataLocation loc = {};
        if (op.lifetime == TestOperandLifeTime::CONSTANT_COPY) {
            loc = {
                    .poolIndex = 0,
                    .offset = *constCopySize,
                    .length = static_cast<int64_t>(op.data.size()),
            };
            constCopies->push_back(&op.data);
            *constCopySize += op.data.alignedSize();
        } else if (op.lifetime == TestOperandLifeTime::CONSTANT_REFERENCE) {
            loc = {
                    .poolIndex = 0,
                    .offset = *constRefSize,
                    .length = static_cast<int64_t>(op.data.size()),
            };
            constReferences->push_back(&op.data);
            *constRefSize += op.data.alignedSize();
        } else if (op.lifetime == TestOperandLifeTime::SUBGRAPH) {
            loc = {
                    .poolIndex = 0,
                    .offset = *op.data.get<uint32_t>(),
                    .length = 0,
            };
        }

        std::optional<OperandExtraParams> extraParams;
        if (op.type == TestOperandType::TENSOR_QUANT8_SYMM_PER_CHANNEL) {
            using Tag = OperandExtraParams::Tag;
            extraParams = OperandExtraParams::make<Tag::channelQuant>(SymmPerChannelQuantParams{
                    .scales = op.channelQuant.scales,
                    .channelDim = static_cast<int32_t>(op.channelQuant.channelDim)});
        }

        operands[i] = {.type = static_cast<OperandType>(op.type),
                       .dimensions = utils::toSigned(op.dimensions).value(),
                       .scale = op.scale,
                       .zeroPoint = op.zeroPoint,
                       .lifetime = static_cast<OperandLifeTime>(op.lifetime),
                       .location = loc,
                       .extraParams = std::move(extraParams)};
    }

    // Operations.
    std::vector<Operation> operations(testSubgraph.operations.size());
    std::transform(testSubgraph.operations.begin(), testSubgraph.operations.end(),
                   operations.begin(), [](const TestOperation& op) -> Operation {
                       return {.type = static_cast<OperationType>(op.type),
                               .inputs = utils::toSigned(op.inputs).value(),
                               .outputs = utils::toSigned(op.outputs).value()};
                   });

    return {.operands = std::move(operands),
            .operations = std::move(operations),
            .inputIndexes = utils::toSigned(testSubgraph.inputIndexes).value(),
            .outputIndexes = utils::toSigned(testSubgraph.outputIndexes).value()};
}

void copyTestBuffers(const std::vector<const TestBuffer*>& buffers, uint8_t* output) {
    uint32_t offset = 0;
    for (const TestBuffer* buffer : buffers) {
        const uint8_t* begin = buffer->get<uint8_t>();
        const uint8_t* end = begin + buffer->size();
        std::copy(begin, end, output + offset);
        offset += buffer->alignedSize();
    }
}

}  // namespace

void waitForSyncFence(int syncFd) {
    constexpr int kInfiniteTimeout = -1;
    ASSERT_GT(syncFd, 0);
    int r = sync_wait(syncFd, kInfiniteTimeout);
    ASSERT_GE(r, 0);
}

Model createModel(const TestModel& testModel) {
    uint32_t constCopySize = 0;
    uint32_t constRefSize = 0;
    std::vector<const TestBuffer*> constCopies;
    std::vector<const TestBuffer*> constReferences;

    Subgraph mainSubgraph = createSubgraph(testModel.main, &constCopySize, &constCopies,
                                           &constRefSize, &constReferences);
    std::vector<Subgraph> refSubgraphs(testModel.referenced.size());
    std::transform(testModel.referenced.begin(), testModel.referenced.end(), refSubgraphs.begin(),
                   [&constCopySize, &constCopies, &constRefSize,
                    &constReferences](const TestSubgraph& testSubgraph) {
                       return createSubgraph(testSubgraph, &constCopySize, &constCopies,
                                             &constRefSize, &constReferences);
                   });

    // Constant copies.
    std::vector<uint8_t> operandValues(constCopySize);
    copyTestBuffers(constCopies, operandValues.data());

    // Shared memory.
    std::vector<nn::SharedMemory> pools = {};
    if (constRefSize > 0) {
        const auto pool = nn::createSharedMemory(constRefSize).value();
        pools.push_back(pool);

        // load data
        const auto mappedMemory = nn::map(pool).value();
        uint8_t* mappedPtr = static_cast<uint8_t*>(std::get<void*>(mappedMemory.pointer));
        CHECK(mappedPtr != nullptr);

        copyTestBuffers(constReferences, mappedPtr);
    }

    std::vector<Memory> aidlPools;
    aidlPools.reserve(pools.size());
    for (auto& pool : pools) {
        auto aidlPool = utils::convert(pool).value();
        aidlPools.push_back(std::move(aidlPool));
    }

    return {.main = std::move(mainSubgraph),
            .referenced = std::move(refSubgraphs),
            .operandValues = std::move(operandValues),
            .pools = std::move(aidlPools),
            .relaxComputationFloat32toFloat16 = testModel.isRelaxed};
}

static bool isOutputSizeGreaterThanOne(const TestModel& testModel, uint32_t index) {
    const auto byteSize = testModel.main.operands[testModel.main.outputIndexes[index]].data.size();
    return byteSize > 1u;
}

static void makeOutputInsufficientSize(uint32_t outputIndex, Request* request) {
    auto& loc = request->outputs[outputIndex].location;
    ASSERT_GT(loc.length, 1u);
    loc.length -= 1u;
    // Test that the padding is not used for output data.
    loc.padding += 1u;
}

static void makeOutputDimensionsUnspecified(Model* model) {
    for (auto i : model->main.outputIndexes) {
        auto& dims = model->main.operands[i].dimensions;
        std::fill(dims.begin(), dims.end(), 0);
    }
}

// Manages the lifetime of memory resources used in an execution.
class ExecutionContext {
  public:
    ExecutionContext(std::shared_ptr<IDevice> device, std::shared_ptr<IPreparedModel> preparedModel)
        : kDevice(std::move(device)), kPreparedModel(std::move(preparedModel)) {}

    std::optional<Request> createRequest(const TestModel& testModel, MemoryType memoryType);
    std::vector<TestBuffer> getOutputBuffers(const TestModel& testModel,
                                             const Request& request) const;

  private:
    // Get a TestBuffer with data copied from an IBuffer object.
    void getBuffer(const std::shared_ptr<IBuffer>& buffer, size_t size,
                   TestBuffer* testBuffer) const;

    static constexpr uint32_t kInputPoolIndex = 0;
    static constexpr uint32_t kOutputPoolIndex = 1;
    static constexpr uint32_t kDeviceMemoryBeginIndex = 2;

    const std::shared_ptr<IDevice> kDevice;
    const std::shared_ptr<IPreparedModel> kPreparedModel;
    std::unique_ptr<TestMemoryBase> mInputMemory, mOutputMemory;
    std::vector<std::shared_ptr<IBuffer>> mBuffers;
};

// Returns the number of bytes needed to round up "size" to the nearest multiple of "multiple".
static uint32_t roundUpBytesNeeded(uint32_t size, uint32_t multiple) {
    CHECK(multiple != 0);
    return ((size + multiple - 1) / multiple) * multiple - size;
}

std::optional<Request> ExecutionContext::createRequest(const TestModel& testModel,
                                                       MemoryType memoryType) {
    // Memory pools are organized as:
    // - 0: Input shared memory pool
    // - 1: Output shared memory pool
    // - [2, 2+i): Input device memories
    // - [2+i, 2+i+o): Output device memories
    DeviceMemoryAllocator allocator(kDevice, kPreparedModel, testModel);
    std::vector<int32_t> tokens;
    mBuffers.clear();

    // Model inputs.
    std::vector<RequestArgument> inputs(testModel.main.inputIndexes.size());
    size_t inputSize = 0;
    for (uint32_t i = 0; i < testModel.main.inputIndexes.size(); i++) {
        const auto& op = testModel.main.operands[testModel.main.inputIndexes[i]];
        if (op.data.size() == 0) {
            // Omitted input.
            inputs[i] = {.hasNoValue = true};
            continue;
        } else if (memoryType == MemoryType::DEVICE) {
            SCOPED_TRACE("Input index = " + std::to_string(i));
            auto [buffer, token] = allocator.allocate<IOType::INPUT>(i);
            if (buffer != nullptr) {
                DataLocation loc = {.poolIndex = static_cast<int32_t>(mBuffers.size() +
                                                                      kDeviceMemoryBeginIndex)};
                mBuffers.push_back(std::move(buffer));
                tokens.push_back(token);
                inputs[i] = {.hasNoValue = false, .location = loc, .dimensions = {}};
                continue;
            }
        }

        // Reserve shared memory for input.
        inputSize += roundUpBytesNeeded(inputSize, nn::kDefaultRequestMemoryAlignment);
        const auto padding = roundUpBytesNeeded(op.data.size(), nn::kDefaultRequestMemoryPadding);
        DataLocation loc = {.poolIndex = kInputPoolIndex,
                            .offset = static_cast<int64_t>(inputSize),
                            .length = static_cast<int64_t>(op.data.size()),
                            .padding = static_cast<int64_t>(padding)};
        inputSize += (op.data.size() + padding);
        inputs[i] = {.hasNoValue = false, .location = loc, .dimensions = {}};
    }

    // Model outputs.
    std::vector<RequestArgument> outputs(testModel.main.outputIndexes.size());
    size_t outputSize = 0;
    for (uint32_t i = 0; i < testModel.main.outputIndexes.size(); i++) {
        const auto& op = testModel.main.operands[testModel.main.outputIndexes[i]];
        if (memoryType == MemoryType::DEVICE) {
            SCOPED_TRACE("Output index = " + std::to_string(i));
            auto [buffer, token] = allocator.allocate<IOType::OUTPUT>(i);
            if (buffer != nullptr) {
                DataLocation loc = {.poolIndex = static_cast<int32_t>(mBuffers.size() +
                                                                      kDeviceMemoryBeginIndex)};
                mBuffers.push_back(std::move(buffer));
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
        outputSize += roundUpBytesNeeded(outputSize, nn::kDefaultRequestMemoryAlignment);
        const auto padding = roundUpBytesNeeded(bufferSize, nn::kDefaultRequestMemoryPadding);
        DataLocation loc = {.poolIndex = kOutputPoolIndex,
                            .offset = static_cast<int64_t>(outputSize),
                            .length = static_cast<int64_t>(bufferSize),
                            .padding = static_cast<int64_t>(padding)};
        outputSize += (bufferSize + padding);
        outputs[i] = {.hasNoValue = false, .location = loc, .dimensions = {}};
    }

    if (memoryType == MemoryType::DEVICE && mBuffers.empty()) {
        return std::nullopt;
    }

    // Memory pools.
    if (memoryType == MemoryType::BLOB_AHWB) {
        mInputMemory = TestBlobAHWB::create(std::max<size_t>(inputSize, 1));
        mOutputMemory = TestBlobAHWB::create(std::max<size_t>(outputSize, 1));
    } else {
        mInputMemory = TestAshmem::create(std::max<size_t>(inputSize, 1), /*aidlReadonly=*/true);
        mOutputMemory = TestAshmem::create(std::max<size_t>(outputSize, 1), /*aidlReadonly=*/false);
    }
    CHECK_NE(mInputMemory, nullptr);
    CHECK_NE(mOutputMemory, nullptr);
    std::vector<RequestMemoryPool> pools;
    pools.reserve(kDeviceMemoryBeginIndex + mBuffers.size());

    auto copiedInputMemory = utils::clone(*mInputMemory->getAidlMemory());
    CHECK(copiedInputMemory.has_value()) << copiedInputMemory.error().message;
    auto copiedOutputMemory = utils::clone(*mOutputMemory->getAidlMemory());
    CHECK(copiedOutputMemory.has_value()) << copiedOutputMemory.error().message;

    pools.push_back(RequestMemoryPool::make<RequestMemoryPool::Tag::pool>(
            std::move(copiedInputMemory).value()));
    pools.push_back(RequestMemoryPool::make<RequestMemoryPool::Tag::pool>(
            std::move(copiedOutputMemory).value()));
    for (const auto& token : tokens) {
        pools.push_back(RequestMemoryPool::make<RequestMemoryPool::Tag::token>(token));
    }

    // Copy input data to the input shared memory pool.
    uint8_t* inputPtr = mInputMemory->getPointer();
    for (uint32_t i = 0; i < testModel.main.inputIndexes.size(); i++) {
        if (!inputs[i].hasNoValue && inputs[i].location.poolIndex == kInputPoolIndex) {
            const auto& op = testModel.main.operands[testModel.main.inputIndexes[i]];
            const uint8_t* begin = op.data.get<uint8_t>();
            const uint8_t* end = begin + op.data.size();
            std::copy(begin, end, inputPtr + inputs[i].location.offset);
        }
    }
    return Request{
            .inputs = std::move(inputs), .outputs = std::move(outputs), .pools = std::move(pools)};
}

std::vector<TestBuffer> ExecutionContext::getOutputBuffers(const TestModel& testModel,
                                                           const Request& request) const {
    // Copy out output results.
    uint8_t* outputPtr = mOutputMemory->getPointer();
    std::vector<TestBuffer> outputBuffers;
    for (uint32_t i = 0; i < request.outputs.size(); i++) {
        const auto& outputLoc = request.outputs[i].location;
        if (outputLoc.poolIndex == kOutputPoolIndex) {
            outputBuffers.emplace_back(outputLoc.length, outputPtr + outputLoc.offset);
        } else {
            const auto& op = testModel.main.operands[testModel.main.outputIndexes[i]];
            if (op.data.size() == 0) {
                outputBuffers.emplace_back(0, nullptr);
            } else {
                SCOPED_TRACE("Output index = " + std::to_string(i));
                const uint32_t bufferIndex = outputLoc.poolIndex - kDeviceMemoryBeginIndex;
                TestBuffer buffer;
                getBuffer(mBuffers[bufferIndex], op.data.size(), &buffer);
                outputBuffers.push_back(std::move(buffer));
            }
        }
    }
    return outputBuffers;
}

// Get a TestBuffer with data copied from an IBuffer object.
void ExecutionContext::getBuffer(const std::shared_ptr<IBuffer>& buffer, size_t size,
                                 TestBuffer* testBuffer) const {
    // IBuffer -> Shared memory.
    auto sharedMemory = nn::createSharedMemory(size).value();
    auto aidlMemory = utils::convert(sharedMemory).value();
    const auto ret = buffer->copyTo(aidlMemory);
    ASSERT_TRUE(ret.isOk());

    // Shared memory -> TestBuffer.
    const auto outputMemory = nn::map(sharedMemory).value();
    const uint8_t* outputPtr = std::visit(
            [](auto* ptr) { return static_cast<const uint8_t*>(ptr); }, outputMemory.pointer);
    ASSERT_NE(outputPtr, nullptr);
    ASSERT_NE(testBuffer, nullptr);
    *testBuffer = TestBuffer(size, outputPtr);
}

static bool hasZeroSizedOutput(const TestModel& testModel) {
    return std::any_of(testModel.main.outputIndexes.begin(), testModel.main.outputIndexes.end(),
                       [&testModel](uint32_t index) {
                           return testModel.main.operands[index].data.size() == 0;
                       });
}

void EvaluatePreparedModel(const std::shared_ptr<IDevice>& device,
                           const std::shared_ptr<IPreparedModel>& preparedModel,
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

    ExecutionContext context(device, preparedModel);
    auto maybeRequest = context.createRequest(testModel, testConfig.memoryType);
    // Skip if testing memory domain but no device memory has been allocated.
    if (!maybeRequest.has_value()) {
        return;
    }

    Request request = std::move(maybeRequest).value();

    constexpr uint32_t kInsufficientOutputIndex = 0;
    if (testConfig.outputType == OutputType::INSUFFICIENT) {
        makeOutputInsufficientSize(kInsufficientOutputIndex, &request);
    }

    int64_t loopTimeoutDurationNs = kOmittedTimeoutDuration;
    // OutputType::MISSED_DEADLINE is only used by
    // TestKind::INTINITE_LOOP_TIMEOUT tests to verify that an infinite loop is
    // aborted after a timeout.
    if (testConfig.outputType == OutputType::MISSED_DEADLINE) {
        // Override the default loop timeout duration with a small value to
        // speed up test execution.
        constexpr int64_t kMillisecond = 1'000'000;
        loopTimeoutDurationNs = 1 * kMillisecond;
    }

    ErrorStatus executionStatus;
    std::vector<OutputShape> outputShapes;
    Timing timing = kNoTiming;
    switch (testConfig.executor) {
        case Executor::SYNC: {
            SCOPED_TRACE("synchronous");

            ExecutionResult executionResult;
            // execute
            const auto ret = preparedModel->executeSynchronously(request, testConfig.measureTiming,
                                                                 kNoDeadline, loopTimeoutDurationNs,
                                                                 &executionResult);
            ASSERT_TRUE(ret.isOk() || ret.getExceptionCode() == EX_SERVICE_SPECIFIC)
                    << ret.getDescription();
            if (ret.isOk()) {
                executionStatus = executionResult.outputSufficientSize
                                          ? ErrorStatus::NONE
                                          : ErrorStatus::OUTPUT_INSUFFICIENT_SIZE;
                outputShapes = std::move(executionResult.outputShapes);
                timing = executionResult.timing;
            } else {
                executionStatus = static_cast<ErrorStatus>(ret.getServiceSpecificError());
            }
            break;
        }
        case Executor::BURST: {
            SCOPED_TRACE("burst");

            // create burst
            std::shared_ptr<IBurst> burst;
            auto ret = preparedModel->configureExecutionBurst(&burst);
            ASSERT_TRUE(ret.isOk()) << ret.getDescription();
            ASSERT_NE(nullptr, burst.get());

            // associate a unique slot with each memory pool
            int64_t currentSlot = 0;
            std::vector<int64_t> slots;
            slots.reserve(request.pools.size());
            for (const auto& pool : request.pools) {
                if (pool.getTag() == RequestMemoryPool::Tag::pool) {
                    slots.push_back(currentSlot++);
                } else {
                    EXPECT_EQ(pool.getTag(), RequestMemoryPool::Tag::token);
                    slots.push_back(-1);
                }
            }

            ExecutionResult executionResult;
            // execute
            ret = burst->executeSynchronously(request, slots, testConfig.measureTiming, kNoDeadline,
                                              loopTimeoutDurationNs, &executionResult);
            ASSERT_TRUE(ret.isOk() || ret.getExceptionCode() == EX_SERVICE_SPECIFIC)
                    << ret.getDescription();
            if (ret.isOk()) {
                executionStatus = executionResult.outputSufficientSize
                                          ? ErrorStatus::NONE
                                          : ErrorStatus::OUTPUT_INSUFFICIENT_SIZE;
                outputShapes = std::move(executionResult.outputShapes);
                timing = executionResult.timing;
            } else {
                executionStatus = static_cast<ErrorStatus>(ret.getServiceSpecificError());
            }

            // Mark each slot as unused after the execution. This is unnecessary because the burst
            // is freed after this scope ends, but this is here to test the functionality.
            for (int64_t slot : slots) {
                ret = burst->releaseMemoryResource(slot);
                ASSERT_TRUE(ret.isOk()) << ret.getDescription();
            }

            break;
        }
        case Executor::FENCED: {
            SCOPED_TRACE("fenced");
            ErrorStatus result = ErrorStatus::NONE;
            FencedExecutionResult executionResult;
            auto ret = preparedModel->executeFenced(request, {}, testConfig.measureTiming,
                                                    kNoDeadline, loopTimeoutDurationNs, kNoDuration,
                                                    &executionResult);
            ASSERT_TRUE(ret.isOk() || ret.getExceptionCode() == EX_SERVICE_SPECIFIC)
                    << ret.getDescription();
            if (!ret.isOk()) {
                result = static_cast<ErrorStatus>(ret.getServiceSpecificError());
                executionStatus = result;
            } else if (executionResult.syncFence.get() != -1) {
                std::vector<ndk::ScopedFileDescriptor> waitFor;
                auto dupFd = dup(executionResult.syncFence.get());
                ASSERT_NE(dupFd, -1);
                waitFor.emplace_back(dupFd);
                // If a sync fence is returned, try start another run waiting for the sync fence.
                ret = preparedModel->executeFenced(request, waitFor, testConfig.measureTiming,
                                                   kNoDeadline, loopTimeoutDurationNs, kNoDuration,
                                                   &executionResult);
                ASSERT_TRUE(ret.isOk());
                waitForSyncFence(executionResult.syncFence.get());
            }
            if (result == ErrorStatus::NONE) {
                ASSERT_NE(executionResult.callback, nullptr);
                Timing timingFenced;
                auto ret = executionResult.callback->getExecutionInfo(&timing, &timingFenced,
                                                                      &executionStatus);
                ASSERT_TRUE(ret.isOk());
            }
            break;
        }
        default: {
            FAIL() << "Unsupported execution mode for AIDL interface.";
        }
    }

    if (testConfig.outputType != OutputType::FULLY_SPECIFIED &&
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
    if (!testConfig.measureTiming) {
        EXPECT_EQ(timing, kNoTiming);
    } else {
        if (timing.timeOnDeviceNs != -1 && timing.timeInDriverNs != -1) {
            EXPECT_LE(timing.timeOnDeviceNs, timing.timeInDriverNs);
        }
    }

    switch (testConfig.outputType) {
        case OutputType::FULLY_SPECIFIED:
            if (testConfig.executor == Executor::FENCED && hasZeroSizedOutput(testModel)) {
                // Executor::FENCED does not support zero-sized output.
                ASSERT_EQ(ErrorStatus::INVALID_ARGUMENT, executionStatus);
                return;
            }
            // If the model output operands are fully specified, outputShapes must be either
            // either empty, or have the same number of elements as the number of outputs.
            ASSERT_EQ(ErrorStatus::NONE, executionStatus);
            ASSERT_TRUE(outputShapes.size() == 0 ||
                        outputShapes.size() == testModel.main.outputIndexes.size());
            break;
        case OutputType::UNSPECIFIED:
            if (testConfig.executor == Executor::FENCED) {
                // For Executor::FENCED, the output shape must be fully specified.
                ASSERT_EQ(ErrorStatus::INVALID_ARGUMENT, executionStatus);
                return;
            }
            // If the model output operands are not fully specified, outputShapes must have
            // the same number of elements as the number of outputs.
            ASSERT_EQ(ErrorStatus::NONE, executionStatus);
            ASSERT_EQ(outputShapes.size(), testModel.main.outputIndexes.size());
            break;
        case OutputType::INSUFFICIENT:
            if (testConfig.executor == Executor::FENCED) {
                // For Executor::FENCED, the output shape must be fully specified.
                ASSERT_EQ(ErrorStatus::INVALID_ARGUMENT, executionStatus);
                return;
            }
            ASSERT_EQ(ErrorStatus::OUTPUT_INSUFFICIENT_SIZE, executionStatus);
            ASSERT_EQ(outputShapes.size(), testModel.main.outputIndexes.size());
            // Check that all returned output dimensions are at least as fully specified as the
            // union of the information about the corresponding operand in the model and in the
            // request. In this test, all model outputs have known rank with all dimensions
            // unspecified, and no dimensional information is provided in the request.
            for (uint32_t i = 0; i < outputShapes.size(); i++) {
                ASSERT_EQ(outputShapes[i].isSufficient, i != kInsufficientOutputIndex);
                const auto& actual = outputShapes[i].dimensions;
                const auto& golden =
                        testModel.main.operands[testModel.main.outputIndexes[i]].dimensions;
                ASSERT_EQ(actual.size(), golden.size());
                for (uint32_t j = 0; j < actual.size(); j++) {
                    if (actual[j] == 0) continue;
                    EXPECT_EQ(actual[j], golden[j]) << "index: " << j;
                }
            }
            return;
        case OutputType::MISSED_DEADLINE:
            ASSERT_TRUE(executionStatus == ErrorStatus::MISSED_DEADLINE_TRANSIENT ||
                        executionStatus == ErrorStatus::MISSED_DEADLINE_PERSISTENT)
                    << "executionStatus = " << executionStatus;
            return;
    }

    // Go through all outputs, check returned output shapes.
    for (uint32_t i = 0; i < outputShapes.size(); i++) {
        EXPECT_TRUE(outputShapes[i].isSufficient);
        const auto& expect = testModel.main.operands[testModel.main.outputIndexes[i]].dimensions;
        const auto unsignedActual = nn::toUnsigned(outputShapes[i].dimensions);
        ASSERT_TRUE(unsignedActual.has_value());
        const std::vector<uint32_t>& actual = unsignedActual.value();
        EXPECT_EQ(expect, actual);
    }

    // Retrieve execution results.
    const std::vector<TestBuffer> outputs = context.getOutputBuffers(testModel, request);

    // We want "close-enough" results.
    checkResults(testModel, outputs);
}

void EvaluatePreparedModel(const std::shared_ptr<IDevice>& device,
                           const std::shared_ptr<IPreparedModel>& preparedModel,
                           const TestModel& testModel, TestKind testKind) {
    std::vector<OutputType> outputTypesList;
    std::vector<bool> measureTimingList;
    std::vector<Executor> executorList;
    std::vector<MemoryType> memoryTypeList;

    switch (testKind) {
        case TestKind::GENERAL: {
            outputTypesList = {OutputType::FULLY_SPECIFIED};
            measureTimingList = {false, true};
            executorList = {Executor::SYNC, Executor::BURST};
            memoryTypeList = {MemoryType::ASHMEM};
        } break;
        case TestKind::DYNAMIC_SHAPE: {
            outputTypesList = {OutputType::UNSPECIFIED, OutputType::INSUFFICIENT};
            measureTimingList = {false, true};
            executorList = {Executor::SYNC, Executor::BURST, Executor::FENCED};
            memoryTypeList = {MemoryType::ASHMEM};
        } break;
        case TestKind::MEMORY_DOMAIN: {
            outputTypesList = {OutputType::FULLY_SPECIFIED};
            measureTimingList = {false};
            executorList = {Executor::SYNC, Executor::BURST, Executor::FENCED};
            memoryTypeList = {MemoryType::BLOB_AHWB, MemoryType::DEVICE};
        } break;
        case TestKind::FENCED_COMPUTE: {
            outputTypesList = {OutputType::FULLY_SPECIFIED};
            measureTimingList = {false, true};
            executorList = {Executor::FENCED};
            memoryTypeList = {MemoryType::ASHMEM};
        } break;
        case TestKind::QUANTIZATION_COUPLING: {
            LOG(FATAL) << "Wrong TestKind for EvaluatePreparedModel";
            return;
        } break;
        case TestKind::INTINITE_LOOP_TIMEOUT: {
            outputTypesList = {OutputType::MISSED_DEADLINE};
            measureTimingList = {false, true};
            executorList = {Executor::SYNC, Executor::BURST, Executor::FENCED};
            memoryTypeList = {MemoryType::ASHMEM};
        } break;
    }

    for (const OutputType outputType : outputTypesList) {
        for (const bool measureTiming : measureTimingList) {
            for (const Executor executor : executorList) {
                for (const MemoryType memoryType : memoryTypeList) {
                    const TestConfig testConfig(executor, measureTiming, outputType, memoryType);
                    EvaluatePreparedModel(device, preparedModel, testModel, testConfig);
                }
            }
        }
    }
}

void EvaluatePreparedCoupledModels(const std::shared_ptr<IDevice>& device,
                                   const std::shared_ptr<IPreparedModel>& preparedModel,
                                   const TestModel& testModel,
                                   const std::shared_ptr<IPreparedModel>& preparedCoupledModel,
                                   const TestModel& coupledModel) {
    const std::vector<OutputType> outputTypesList = {OutputType::FULLY_SPECIFIED};
    const std::vector<bool> measureTimingList = {false, true};
    const std::vector<Executor> executorList = {Executor::SYNC, Executor::BURST, Executor::FENCED};

    for (const OutputType outputType : outputTypesList) {
        for (const bool measureTiming : measureTimingList) {
            for (const Executor executor : executorList) {
                const TestConfig testConfig(executor, measureTiming, outputType, MemoryType::ASHMEM,
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

void Execute(const std::shared_ptr<IDevice>& device, const TestModel& testModel,
             TestKind testKind) {
    Model model = createModel(testModel);
    if (testKind == TestKind::DYNAMIC_SHAPE) {
        makeOutputDimensionsUnspecified(&model);
    }

    std::shared_ptr<IPreparedModel> preparedModel;
    switch (testKind) {
        case TestKind::GENERAL:
        case TestKind::DYNAMIC_SHAPE:
        case TestKind::MEMORY_DOMAIN:
        case TestKind::FENCED_COMPUTE:
        case TestKind::INTINITE_LOOP_TIMEOUT: {
            createPreparedModel(device, model, &preparedModel);
            if (preparedModel == nullptr) return;
            EvaluatePreparedModel(device, preparedModel, testModel, testKind);
        } break;
        case TestKind::QUANTIZATION_COUPLING: {
            ASSERT_TRUE(testModel.hasQuant8CoupledOperands());
            createPreparedModel(device, model, &preparedModel,
                                /*reportSkipping*/ false);
            TestModel signedQuantizedModel = convertQuant8AsymmOperandsToSigned(testModel);
            std::shared_ptr<IPreparedModel> preparedCoupledModel;
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
    const bool deviceIsResponsive =
            ndk::ScopedAStatus::fromStatus(AIBinder_ping(kDevice->asBinder().get())).isOk();
    ASSERT_TRUE(deviceIsResponsive);
}

std::vector<NamedModel> getNamedModels(const FilterFn& filter) {
    return TestModelManager::get().getTestModels(filter);
}

std::vector<NamedModel> getNamedModels(const FilterNameFn& filter) {
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

// Tag for the loop timeout tests
class InfiniteLoopTimeoutTest : public GeneratedTest {};

TEST_P(GeneratedTest, Test) {
    Execute(kDevice, kTestModel, TestKind::GENERAL);
}

TEST_P(DynamicOutputShapeTest, Test) {
    Execute(kDevice, kTestModel, TestKind::DYNAMIC_SHAPE);
}

TEST_P(MemoryDomainTest, Test) {
    Execute(kDevice, kTestModel, TestKind::MEMORY_DOMAIN);
}

TEST_P(FencedComputeTest, Test) {
    Execute(kDevice, kTestModel, TestKind::FENCED_COMPUTE);
}

TEST_P(QuantizationCouplingTest, Test) {
    Execute(kDevice, kTestModel, TestKind::QUANTIZATION_COUPLING);
}

TEST_P(InfiniteLoopTimeoutTest, Test) {
    Execute(kDevice, kTestModel, TestKind::INTINITE_LOOP_TIMEOUT);
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
    return !testModel.expectFailure && testModel.hasQuant8CoupledOperands() &&
           testModel.main.operations.size() == 1;
});

INSTANTIATE_GENERATED_TEST(InfiniteLoopTimeoutTest, [](const TestModel& testModel) {
    return testModel.isInfiniteLoopTimeoutTest();
});

}  // namespace aidl::android::hardware::neuralnetworks::vts::functional
