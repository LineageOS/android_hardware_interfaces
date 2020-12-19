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

#define LOG_TAG "ExecutionBurstController"

#include "ExecutionBurstController.h"

#include <android-base/logging.h>

#include <algorithm>
#include <cstring>
#include <limits>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "HalInterfaces.h"
#include "Tracing.h"
#include "Utils.h"

namespace android::nn {
namespace {

using V1_2::FmqRequestDatum;
using V1_2::FmqResultDatum;
using V1_2::IBurstCallback;
using V1_2::IBurstContext;
using FmqRequestDescriptor = hardware::MQDescriptorSync<FmqRequestDatum>;
using FmqResultDescriptor = hardware::MQDescriptorSync<FmqResultDatum>;

constexpr V1_2::Timing kNoTiming12 = {std::numeric_limits<uint64_t>::max(),
                                      std::numeric_limits<uint64_t>::max()};

class BurstContextDeathHandler : public hardware::hidl_death_recipient {
  public:
    using Callback = std::function<void()>;

    BurstContextDeathHandler(const Callback& onDeathCallback) : mOnDeathCallback(onDeathCallback) {
        CHECK(onDeathCallback != nullptr);
    }

    void serviceDied(uint64_t /*cookie*/, const wp<hidl::base::V1_0::IBase>& /*who*/) override {
        LOG(ERROR) << "BurstContextDeathHandler::serviceDied -- service unexpectedly died!";
        mOnDeathCallback();
    }

  private:
    const Callback mOnDeathCallback;
};

}  // anonymous namespace

// serialize a request into a packet
std::vector<FmqRequestDatum> serialize(const V1_0::Request& request, V1_2::MeasureTiming measure,
                                       const std::vector<int32_t>& slots) {
    // count how many elements need to be sent for a request
    size_t count = 2 + request.inputs.size() + request.outputs.size() + request.pools.size();
    for (const auto& input : request.inputs) {
        count += input.dimensions.size();
    }
    for (const auto& output : request.outputs) {
        count += output.dimensions.size();
    }

    // create buffer to temporarily store elements
    std::vector<FmqRequestDatum> data;
    data.reserve(count);

    // package packetInfo
    {
        FmqRequestDatum datum;
        datum.packetInformation(
                {/*.packetSize=*/static_cast<uint32_t>(count),
                 /*.numberOfInputOperands=*/static_cast<uint32_t>(request.inputs.size()),
                 /*.numberOfOutputOperands=*/static_cast<uint32_t>(request.outputs.size()),
                 /*.numberOfPools=*/static_cast<uint32_t>(request.pools.size())});
        data.push_back(datum);
    }

    // package input data
    for (const auto& input : request.inputs) {
        // package operand information
        FmqRequestDatum datum;
        datum.inputOperandInformation(
                {/*.hasNoValue=*/input.hasNoValue,
                 /*.location=*/input.location,
                 /*.numberOfDimensions=*/static_cast<uint32_t>(input.dimensions.size())});
        data.push_back(datum);

        // package operand dimensions
        for (uint32_t dimension : input.dimensions) {
            FmqRequestDatum datum;
            datum.inputOperandDimensionValue(dimension);
            data.push_back(datum);
        }
    }

    // package output data
    for (const auto& output : request.outputs) {
        // package operand information
        FmqRequestDatum datum;
        datum.outputOperandInformation(
                {/*.hasNoValue=*/output.hasNoValue,
                 /*.location=*/output.location,
                 /*.numberOfDimensions=*/static_cast<uint32_t>(output.dimensions.size())});
        data.push_back(datum);

        // package operand dimensions
        for (uint32_t dimension : output.dimensions) {
            FmqRequestDatum datum;
            datum.outputOperandDimensionValue(dimension);
            data.push_back(datum);
        }
    }

    // package pool identifier
    for (int32_t slot : slots) {
        FmqRequestDatum datum;
        datum.poolIdentifier(slot);
        data.push_back(datum);
    }

    // package measureTiming
    {
        FmqRequestDatum datum;
        datum.measureTiming(measure);
        data.push_back(datum);
    }

    // return packet
    return data;
}

// deserialize a packet into the result
std::optional<std::tuple<V1_0::ErrorStatus, std::vector<V1_2::OutputShape>, V1_2::Timing>>
deserialize(const std::vector<FmqResultDatum>& data) {
    using discriminator = FmqResultDatum::hidl_discriminator;

    std::vector<V1_2::OutputShape> outputShapes;
    size_t index = 0;

    // validate packet information
    if (data.size() == 0 || data[index].getDiscriminator() != discriminator::packetInformation) {
        LOG(ERROR) << "FMQ Result packet ill-formed";
        return std::nullopt;
    }

    // unpackage packet information
    const FmqResultDatum::PacketInformation& packetInfo = data[index].packetInformation();
    index++;
    const uint32_t packetSize = packetInfo.packetSize;
    const V1_0::ErrorStatus errorStatus = packetInfo.errorStatus;
    const uint32_t numberOfOperands = packetInfo.numberOfOperands;

    // verify packet size
    if (data.size() != packetSize) {
        LOG(ERROR) << "FMQ Result packet ill-formed";
        return std::nullopt;
    }

    // unpackage operands
    for (size_t operand = 0; operand < numberOfOperands; ++operand) {
        // validate operand information
        if (data[index].getDiscriminator() != discriminator::operandInformation) {
            LOG(ERROR) << "FMQ Result packet ill-formed";
            return std::nullopt;
        }

        // unpackage operand information
        const FmqResultDatum::OperandInformation& operandInfo = data[index].operandInformation();
        index++;
        const bool isSufficient = operandInfo.isSufficient;
        const uint32_t numberOfDimensions = operandInfo.numberOfDimensions;

        // unpackage operand dimensions
        std::vector<uint32_t> dimensions;
        dimensions.reserve(numberOfDimensions);
        for (size_t i = 0; i < numberOfDimensions; ++i) {
            // validate dimension
            if (data[index].getDiscriminator() != discriminator::operandDimensionValue) {
                LOG(ERROR) << "FMQ Result packet ill-formed";
                return std::nullopt;
            }

            // unpackage dimension
            const uint32_t dimension = data[index].operandDimensionValue();
            index++;

            // store result
            dimensions.push_back(dimension);
        }

        // store result
        outputShapes.push_back({/*.dimensions=*/dimensions, /*.isSufficient=*/isSufficient});
    }

    // validate execution timing
    if (data[index].getDiscriminator() != discriminator::executionTiming) {
        LOG(ERROR) << "FMQ Result packet ill-formed";
        return std::nullopt;
    }

    // unpackage execution timing
    const V1_2::Timing timing = data[index].executionTiming();
    index++;

    // validate packet information
    if (index != packetSize) {
        LOG(ERROR) << "FMQ Result packet ill-formed";
        return std::nullopt;
    }

    // return result
    return std::make_tuple(errorStatus, std::move(outputShapes), timing);
}

V1_0::ErrorStatus legacyConvertResultCodeToErrorStatus(int resultCode) {
    return convertToV1_0(convertResultCodeToErrorStatus(resultCode));
}

std::pair<std::unique_ptr<ResultChannelReceiver>, const FmqResultDescriptor*>
ResultChannelReceiver::create(size_t channelLength, std::chrono::microseconds pollingTimeWindow) {
    std::unique_ptr<FmqResultChannel> fmqResultChannel =
            std::make_unique<FmqResultChannel>(channelLength, /*confEventFlag=*/true);
    if (!fmqResultChannel->isValid()) {
        LOG(ERROR) << "Unable to create ResultChannelReceiver";
        return {nullptr, nullptr};
    }

    const FmqResultDescriptor* descriptor = fmqResultChannel->getDesc();
    return std::make_pair(
            std::make_unique<ResultChannelReceiver>(std::move(fmqResultChannel), pollingTimeWindow),
            descriptor);
}

ResultChannelReceiver::ResultChannelReceiver(std::unique_ptr<FmqResultChannel> fmqResultChannel,
                                             std::chrono::microseconds pollingTimeWindow)
    : mFmqResultChannel(std::move(fmqResultChannel)), kPollingTimeWindow(pollingTimeWindow) {}

std::optional<std::tuple<V1_0::ErrorStatus, std::vector<V1_2::OutputShape>, V1_2::Timing>>
ResultChannelReceiver::getBlocking() {
    const auto packet = getPacketBlocking();
    if (!packet) {
        return std::nullopt;
    }

    return deserialize(*packet);
}

void ResultChannelReceiver::invalidate() {
    mValid = false;

    // force unblock
    // ExecutionBurstController waits on a result packet after sending a
    // request. If the driver containing ExecutionBurstServer crashes, the
    // controller may be waiting on the futex. This force unblock wakes up any
    // thread waiting on the futex.
    // TODO: look for a different/better way to signal/notify the futex to
    // wake up any thread waiting on it
    FmqResultDatum datum;
    datum.packetInformation({/*.packetSize=*/0,
                             /*.errorStatus=*/V1_0::ErrorStatus::GENERAL_FAILURE,
                             /*.numberOfOperands=*/0});
    mFmqResultChannel->writeBlocking(&datum, 1);
}

std::optional<std::vector<FmqResultDatum>> ResultChannelReceiver::getPacketBlocking() {
    if (!mValid) {
        return std::nullopt;
    }

    // First spend time polling if results are available in FMQ instead of
    // waiting on the futex. Polling is more responsive (yielding lower
    // latencies), but can take up more power, so only poll for a limited period
    // of time.

    auto& getCurrentTime = std::chrono::high_resolution_clock::now;
    const auto timeToStopPolling = getCurrentTime() + kPollingTimeWindow;

    while (getCurrentTime() < timeToStopPolling) {
        // if class is being torn down, immediately return
        if (!mValid.load(std::memory_order_relaxed)) {
            return std::nullopt;
        }

        // Check if data is available. If it is, immediately retrieve it and
        // return.
        const size_t available = mFmqResultChannel->availableToRead();
        if (available > 0) {
            std::vector<FmqResultDatum> packet(available);
            const bool success = mFmqResultChannel->read(packet.data(), available);
            if (!success) {
                LOG(ERROR) << "Error receiving packet";
                return std::nullopt;
            }
            return std::make_optional(std::move(packet));
        }
    }

    // If we get to this point, we either stopped polling because it was taking
    // too long or polling was not allowed. Instead, perform a blocking call
    // which uses a futex to save power.

    // wait for result packet and read first element of result packet
    FmqResultDatum datum;
    bool success = mFmqResultChannel->readBlocking(&datum, 1);

    // retrieve remaining elements
    // NOTE: all of the data is already available at this point, so there's no
    // need to do a blocking wait to wait for more data. This is known because
    // in FMQ, all writes are published (made available) atomically. Currently,
    // the producer always publishes the entire packet in one function call, so
    // if the first element of the packet is available, the remaining elements
    // are also available.
    const size_t count = mFmqResultChannel->availableToRead();
    std::vector<FmqResultDatum> packet(count + 1);
    std::memcpy(&packet.front(), &datum, sizeof(datum));
    success &= mFmqResultChannel->read(packet.data() + 1, count);

    if (!mValid) {
        return std::nullopt;
    }

    // ensure packet was successfully received
    if (!success) {
        LOG(ERROR) << "Error receiving packet";
        return std::nullopt;
    }

    return std::make_optional(std::move(packet));
}

std::pair<std::unique_ptr<RequestChannelSender>, const FmqRequestDescriptor*>
RequestChannelSender::create(size_t channelLength) {
    std::unique_ptr<FmqRequestChannel> fmqRequestChannel =
            std::make_unique<FmqRequestChannel>(channelLength, /*confEventFlag=*/true);
    if (!fmqRequestChannel->isValid()) {
        LOG(ERROR) << "Unable to create RequestChannelSender";
        return {nullptr, nullptr};
    }

    const FmqRequestDescriptor* descriptor = fmqRequestChannel->getDesc();
    return std::make_pair(std::make_unique<RequestChannelSender>(std::move(fmqRequestChannel)),
                          descriptor);
}

RequestChannelSender::RequestChannelSender(std::unique_ptr<FmqRequestChannel> fmqRequestChannel)
    : mFmqRequestChannel(std::move(fmqRequestChannel)) {}

bool RequestChannelSender::send(const V1_0::Request& request, V1_2::MeasureTiming measure,
                                const std::vector<int32_t>& slots) {
    const std::vector<FmqRequestDatum> serialized = serialize(request, measure, slots);
    return sendPacket(serialized);
}

bool RequestChannelSender::sendPacket(const std::vector<FmqRequestDatum>& packet) {
    if (!mValid) {
        return false;
    }

    if (packet.size() > mFmqRequestChannel->availableToWrite()) {
        LOG(ERROR)
                << "RequestChannelSender::sendPacket -- packet size exceeds size available in FMQ";
        return false;
    }

    // Always send the packet with "blocking" because this signals the futex and
    // unblocks the consumer if it is waiting on the futex.
    return mFmqRequestChannel->writeBlocking(packet.data(), packet.size());
}

void RequestChannelSender::invalidate() {
    mValid = false;
}

hardware::Return<void> ExecutionBurstController::ExecutionBurstCallback::getMemories(
        const hardware::hidl_vec<int32_t>& slots, getMemories_cb cb) {
    std::lock_guard<std::mutex> guard(mMutex);

    // get all memories
    hardware::hidl_vec<hardware::hidl_memory> memories(slots.size());
    std::transform(slots.begin(), slots.end(), memories.begin(), [this](int32_t slot) {
        return slot < mMemoryCache.size() ? mMemoryCache[slot] : hardware::hidl_memory{};
    });

    // ensure all memories are valid
    if (!std::all_of(memories.begin(), memories.end(),
                     [](const hardware::hidl_memory& memory) { return memory.valid(); })) {
        cb(V1_0::ErrorStatus::INVALID_ARGUMENT, {});
        return hardware::Void();
    }

    // return successful
    cb(V1_0::ErrorStatus::NONE, std::move(memories));
    return hardware::Void();
}

std::vector<int32_t> ExecutionBurstController::ExecutionBurstCallback::getSlots(
        const hardware::hidl_vec<hardware::hidl_memory>& memories,
        const std::vector<intptr_t>& keys) {
    std::lock_guard<std::mutex> guard(mMutex);

    // retrieve (or bind) all slots corresponding to memories
    std::vector<int32_t> slots;
    slots.reserve(memories.size());
    for (size_t i = 0; i < memories.size(); ++i) {
        slots.push_back(getSlotLocked(memories[i], keys[i]));
    }
    return slots;
}

std::pair<bool, int32_t> ExecutionBurstController::ExecutionBurstCallback::freeMemory(
        intptr_t key) {
    std::lock_guard<std::mutex> guard(mMutex);

    auto iter = mMemoryIdToSlot.find(key);
    if (iter == mMemoryIdToSlot.end()) {
        return {false, 0};
    }
    const int32_t slot = iter->second;
    mMemoryIdToSlot.erase(key);
    mMemoryCache[slot] = {};
    mFreeSlots.push(slot);
    return {true, slot};
}

int32_t ExecutionBurstController::ExecutionBurstCallback::getSlotLocked(
        const hardware::hidl_memory& memory, intptr_t key) {
    auto iter = mMemoryIdToSlot.find(key);
    if (iter == mMemoryIdToSlot.end()) {
        const int32_t slot = allocateSlotLocked();
        mMemoryIdToSlot[key] = slot;
        mMemoryCache[slot] = memory;
        return slot;
    } else {
        const int32_t slot = iter->second;
        return slot;
    }
}

int32_t ExecutionBurstController::ExecutionBurstCallback::allocateSlotLocked() {
    constexpr size_t kMaxNumberOfSlots = std::numeric_limits<int32_t>::max();

    // if there is a free slot, use it
    if (mFreeSlots.size() > 0) {
        const int32_t slot = mFreeSlots.top();
        mFreeSlots.pop();
        return slot;
    }

    // otherwise use a slot for the first time
    CHECK(mMemoryCache.size() < kMaxNumberOfSlots) << "Exceeded maximum number of slots!";
    const int32_t slot = static_cast<int32_t>(mMemoryCache.size());
    mMemoryCache.emplace_back();

    return slot;
}

std::unique_ptr<ExecutionBurstController> ExecutionBurstController::create(
        const sp<V1_2::IPreparedModel>& preparedModel,
        std::chrono::microseconds pollingTimeWindow) {
    // check inputs
    if (preparedModel == nullptr) {
        LOG(ERROR) << "ExecutionBurstController::create passed a nullptr";
        return nullptr;
    }

    // create callback object
    sp<ExecutionBurstCallback> callback = new ExecutionBurstCallback();

    // create FMQ objects
    auto [requestChannelSenderTemp, requestChannelDescriptor] =
            RequestChannelSender::create(kExecutionBurstChannelLength);
    auto [resultChannelReceiverTemp, resultChannelDescriptor] =
            ResultChannelReceiver::create(kExecutionBurstChannelLength, pollingTimeWindow);
    std::shared_ptr<RequestChannelSender> requestChannelSender =
            std::move(requestChannelSenderTemp);
    std::shared_ptr<ResultChannelReceiver> resultChannelReceiver =
            std::move(resultChannelReceiverTemp);

    // check FMQ objects
    if (!requestChannelSender || !resultChannelReceiver || !requestChannelDescriptor ||
        !resultChannelDescriptor) {
        LOG(ERROR) << "ExecutionBurstController::create failed to create FastMessageQueue";
        return nullptr;
    }

    // configure burst
    V1_0::ErrorStatus errorStatus;
    sp<IBurstContext> burstContext;
    const hardware::Return<void> ret = preparedModel->configureExecutionBurst(
            callback, *requestChannelDescriptor, *resultChannelDescriptor,
            [&errorStatus, &burstContext](V1_0::ErrorStatus status,
                                          const sp<IBurstContext>& context) {
                errorStatus = status;
                burstContext = context;
            });

    // check burst
    if (!ret.isOk()) {
        LOG(ERROR) << "IPreparedModel::configureExecutionBurst failed with description "
                   << ret.description();
        return nullptr;
    }
    if (errorStatus != V1_0::ErrorStatus::NONE) {
        LOG(ERROR) << "IPreparedModel::configureExecutionBurst failed with status "
                   << toString(errorStatus);
        return nullptr;
    }
    if (burstContext == nullptr) {
        LOG(ERROR) << "IPreparedModel::configureExecutionBurst returned nullptr for burst";
        return nullptr;
    }

    // create death handler object
    BurstContextDeathHandler::Callback onDeathCallback = [requestChannelSender,
                                                          resultChannelReceiver] {
        requestChannelSender->invalidate();
        resultChannelReceiver->invalidate();
    };
    const sp<BurstContextDeathHandler> deathHandler = new BurstContextDeathHandler(onDeathCallback);

    // linkToDeath registers a callback that will be invoked on service death to
    // proactively handle service crashes. If the linkToDeath call fails,
    // asynchronous calls are susceptible to hangs if the service crashes before
    // providing the response.
    const hardware::Return<bool> deathHandlerRet = burstContext->linkToDeath(deathHandler, 0);
    if (!deathHandlerRet.isOk() || deathHandlerRet != true) {
        LOG(ERROR) << "ExecutionBurstController::create -- Failed to register a death recipient "
                      "for the IBurstContext object.";
        return nullptr;
    }

    // make and return controller
    return std::make_unique<ExecutionBurstController>(requestChannelSender, resultChannelReceiver,
                                                      burstContext, callback, deathHandler);
}

ExecutionBurstController::ExecutionBurstController(
        const std::shared_ptr<RequestChannelSender>& requestChannelSender,
        const std::shared_ptr<ResultChannelReceiver>& resultChannelReceiver,
        const sp<IBurstContext>& burstContext, const sp<ExecutionBurstCallback>& callback,
        const sp<hardware::hidl_death_recipient>& deathHandler)
    : mRequestChannelSender(requestChannelSender),
      mResultChannelReceiver(resultChannelReceiver),
      mBurstContext(burstContext),
      mMemoryCache(callback),
      mDeathHandler(deathHandler) {}

ExecutionBurstController::~ExecutionBurstController() {
    // It is safe to ignore any errors resulting from this unlinkToDeath call
    // because the ExecutionBurstController object is already being destroyed
    // and its underlying IBurstContext object is no longer being used by the NN
    // runtime.
    if (mDeathHandler) {
        mBurstContext->unlinkToDeath(mDeathHandler).isOk();
    }
}

static std::tuple<int, std::vector<V1_2::OutputShape>, V1_2::Timing, bool> getExecutionResult(
        V1_0::ErrorStatus status, std::vector<V1_2::OutputShape> outputShapes, V1_2::Timing timing,
        bool fallback) {
    auto [n, checkedOutputShapes, checkedTiming] =
            getExecutionResult(convertToV1_3(status), std::move(outputShapes), timing);
    return {n, convertToV1_2(checkedOutputShapes), convertToV1_2(checkedTiming), fallback};
}

std::tuple<int, std::vector<V1_2::OutputShape>, V1_2::Timing, bool>
ExecutionBurstController::compute(const V1_0::Request& request, V1_2::MeasureTiming measure,
                                  const std::vector<intptr_t>& memoryIds) {
    // This is the first point when we know an execution is occurring, so begin
    // to collect systraces. Note that the first point we can begin collecting
    // systraces in ExecutionBurstServer is when the RequestChannelReceiver
    // realizes there is data in the FMQ, so ExecutionBurstServer collects
    // systraces at different points in the code.
    NNTRACE_FULL(NNTRACE_LAYER_IPC, NNTRACE_PHASE_EXECUTION, "ExecutionBurstController::compute");

    std::lock_guard<std::mutex> guard(mMutex);

    // send request packet
    const std::vector<int32_t> slots = mMemoryCache->getSlots(request.pools, memoryIds);
    const bool success = mRequestChannelSender->send(request, measure, slots);
    if (!success) {
        LOG(ERROR) << "Error sending FMQ packet";
        // only use fallback execution path if the packet could not be sent
        return getExecutionResult(V1_0::ErrorStatus::GENERAL_FAILURE, {}, kNoTiming12,
                                  /*fallback=*/true);
    }

    // get result packet
    const auto result = mResultChannelReceiver->getBlocking();
    if (!result) {
        LOG(ERROR) << "Error retrieving FMQ packet";
        // only use fallback execution path if the packet could not be sent
        return getExecutionResult(V1_0::ErrorStatus::GENERAL_FAILURE, {}, kNoTiming12,
                                  /*fallback=*/false);
    }

    // unpack results and return (only use fallback execution path if the
    // packet could not be sent)
    auto [status, outputShapes, timing] = std::move(*result);
    return getExecutionResult(status, std::move(outputShapes), timing, /*fallback=*/false);
}

void ExecutionBurstController::freeMemory(intptr_t key) {
    std::lock_guard<std::mutex> guard(mMutex);

    bool valid;
    int32_t slot;
    std::tie(valid, slot) = mMemoryCache->freeMemory(key);
    if (valid) {
        mBurstContext->freeMemory(slot).isOk();
    }
}

}  // namespace android::nn
