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

#define LOG_TAG "ExecutionBurstServer"

#include "ExecutionBurstServer.h"

#include <android-base/logging.h>

#include <algorithm>
#include <cstring>
#include <limits>
#include <map>
#include <memory>
#include <tuple>
#include <utility>
#include <vector>

#include "HalInterfaces.h"
#include "Tracing.h"

namespace android::nn {
namespace {

using hardware::MQDescriptorSync;
using V1_2::FmqRequestDatum;
using V1_2::FmqResultDatum;
using V1_2::IBurstCallback;
using V1_2::IBurstContext;

constexpr V1_2::Timing kNoTiming = {std::numeric_limits<uint64_t>::max(),
                                    std::numeric_limits<uint64_t>::max()};

// DefaultBurstExecutorWithCache adapts an IPreparedModel so that it can be
// used as an IBurstExecutorWithCache. Specifically, the cache simply stores the
// hidl_memory object, and the execution forwards calls to the provided
// IPreparedModel's "executeSynchronously" method. With this class, hidl_memory
// must be mapped and unmapped for each execution.
class DefaultBurstExecutorWithCache : public ExecutionBurstServer::IBurstExecutorWithCache {
  public:
    DefaultBurstExecutorWithCache(V1_2::IPreparedModel* preparedModel)
        : mpPreparedModel(preparedModel) {}

    bool isCacheEntryPresent(int32_t slot) const override {
        const auto it = mMemoryCache.find(slot);
        return (it != mMemoryCache.end()) && it->second.valid();
    }

    void addCacheEntry(const hardware::hidl_memory& memory, int32_t slot) override {
        mMemoryCache[slot] = memory;
    }

    void removeCacheEntry(int32_t slot) override { mMemoryCache.erase(slot); }

    std::tuple<V1_0::ErrorStatus, hardware::hidl_vec<V1_2::OutputShape>, V1_2::Timing> execute(
            const V1_0::Request& request, const std::vector<int32_t>& slots,
            V1_2::MeasureTiming measure) override {
        // convert slots to pools
        hardware::hidl_vec<hardware::hidl_memory> pools(slots.size());
        std::transform(slots.begin(), slots.end(), pools.begin(),
                       [this](int32_t slot) { return mMemoryCache[slot]; });

        // create full request
        V1_0::Request fullRequest = request;
        fullRequest.pools = std::move(pools);

        // setup execution
        V1_0::ErrorStatus returnedStatus = V1_0::ErrorStatus::GENERAL_FAILURE;
        hardware::hidl_vec<V1_2::OutputShape> returnedOutputShapes;
        V1_2::Timing returnedTiming;
        auto cb = [&returnedStatus, &returnedOutputShapes, &returnedTiming](
                          V1_0::ErrorStatus status,
                          const hardware::hidl_vec<V1_2::OutputShape>& outputShapes,
                          const V1_2::Timing& timing) {
            returnedStatus = status;
            returnedOutputShapes = outputShapes;
            returnedTiming = timing;
        };

        // execute
        const hardware::Return<void> ret =
                mpPreparedModel->executeSynchronously(fullRequest, measure, cb);
        if (!ret.isOk() || returnedStatus != V1_0::ErrorStatus::NONE) {
            LOG(ERROR) << "IPreparedModelAdapter::execute -- Error executing";
            return {returnedStatus, std::move(returnedOutputShapes), kNoTiming};
        }

        return std::make_tuple(returnedStatus, std::move(returnedOutputShapes), returnedTiming);
    }

  private:
    V1_2::IPreparedModel* const mpPreparedModel;
    std::map<int32_t, hardware::hidl_memory> mMemoryCache;
};

}  // anonymous namespace

// serialize result
std::vector<FmqResultDatum> serialize(V1_0::ErrorStatus errorStatus,
                                      const std::vector<V1_2::OutputShape>& outputShapes,
                                      V1_2::Timing timing) {
    // count how many elements need to be sent for a request
    size_t count = 2 + outputShapes.size();
    for (const auto& outputShape : outputShapes) {
        count += outputShape.dimensions.size();
    }

    // create buffer to temporarily store elements
    std::vector<FmqResultDatum> data;
    data.reserve(count);

    // package packetInfo
    {
        FmqResultDatum datum;
        datum.packetInformation({/*.packetSize=*/static_cast<uint32_t>(count),
                                 /*.errorStatus=*/errorStatus,
                                 /*.numberOfOperands=*/static_cast<uint32_t>(outputShapes.size())});
        data.push_back(datum);
    }

    // package output shape data
    for (const auto& operand : outputShapes) {
        // package operand information
        FmqResultDatum::OperandInformation info{};
        info.isSufficient = operand.isSufficient;
        info.numberOfDimensions = static_cast<uint32_t>(operand.dimensions.size());

        FmqResultDatum datum;
        datum.operandInformation(info);
        data.push_back(datum);

        // package operand dimensions
        for (uint32_t dimension : operand.dimensions) {
            FmqResultDatum datum;
            datum.operandDimensionValue(dimension);
            data.push_back(datum);
        }
    }

    // package executionTiming
    {
        FmqResultDatum datum;
        datum.executionTiming(timing);
        data.push_back(datum);
    }

    // return result
    return data;
}

// deserialize request
std::optional<std::tuple<V1_0::Request, std::vector<int32_t>, V1_2::MeasureTiming>> deserialize(
        const std::vector<FmqRequestDatum>& data) {
    using discriminator = FmqRequestDatum::hidl_discriminator;

    size_t index = 0;

    // validate packet information
    if (data.size() == 0 || data[index].getDiscriminator() != discriminator::packetInformation) {
        LOG(ERROR) << "FMQ Request packet ill-formed";
        return std::nullopt;
    }

    // unpackage packet information
    const FmqRequestDatum::PacketInformation& packetInfo = data[index].packetInformation();
    index++;
    const uint32_t packetSize = packetInfo.packetSize;
    const uint32_t numberOfInputOperands = packetInfo.numberOfInputOperands;
    const uint32_t numberOfOutputOperands = packetInfo.numberOfOutputOperands;
    const uint32_t numberOfPools = packetInfo.numberOfPools;

    // verify packet size
    if (data.size() != packetSize) {
        LOG(ERROR) << "FMQ Request packet ill-formed";
        return std::nullopt;
    }

    // unpackage input operands
    std::vector<V1_0::RequestArgument> inputs;
    inputs.reserve(numberOfInputOperands);
    for (size_t operand = 0; operand < numberOfInputOperands; ++operand) {
        // validate input operand information
        if (data[index].getDiscriminator() != discriminator::inputOperandInformation) {
            LOG(ERROR) << "FMQ Request packet ill-formed";
            return std::nullopt;
        }

        // unpackage operand information
        const FmqRequestDatum::OperandInformation& operandInfo =
                data[index].inputOperandInformation();
        index++;
        const bool hasNoValue = operandInfo.hasNoValue;
        const V1_0::DataLocation location = operandInfo.location;
        const uint32_t numberOfDimensions = operandInfo.numberOfDimensions;

        // unpackage operand dimensions
        std::vector<uint32_t> dimensions;
        dimensions.reserve(numberOfDimensions);
        for (size_t i = 0; i < numberOfDimensions; ++i) {
            // validate dimension
            if (data[index].getDiscriminator() != discriminator::inputOperandDimensionValue) {
                LOG(ERROR) << "FMQ Request packet ill-formed";
                return std::nullopt;
            }

            // unpackage dimension
            const uint32_t dimension = data[index].inputOperandDimensionValue();
            index++;

            // store result
            dimensions.push_back(dimension);
        }

        // store result
        inputs.push_back(
                {/*.hasNoValue=*/hasNoValue, /*.location=*/location, /*.dimensions=*/dimensions});
    }

    // unpackage output operands
    std::vector<V1_0::RequestArgument> outputs;
    outputs.reserve(numberOfOutputOperands);
    for (size_t operand = 0; operand < numberOfOutputOperands; ++operand) {
        // validate output operand information
        if (data[index].getDiscriminator() != discriminator::outputOperandInformation) {
            LOG(ERROR) << "FMQ Request packet ill-formed";
            return std::nullopt;
        }

        // unpackage operand information
        const FmqRequestDatum::OperandInformation& operandInfo =
                data[index].outputOperandInformation();
        index++;
        const bool hasNoValue = operandInfo.hasNoValue;
        const V1_0::DataLocation location = operandInfo.location;
        const uint32_t numberOfDimensions = operandInfo.numberOfDimensions;

        // unpackage operand dimensions
        std::vector<uint32_t> dimensions;
        dimensions.reserve(numberOfDimensions);
        for (size_t i = 0; i < numberOfDimensions; ++i) {
            // validate dimension
            if (data[index].getDiscriminator() != discriminator::outputOperandDimensionValue) {
                LOG(ERROR) << "FMQ Request packet ill-formed";
                return std::nullopt;
            }

            // unpackage dimension
            const uint32_t dimension = data[index].outputOperandDimensionValue();
            index++;

            // store result
            dimensions.push_back(dimension);
        }

        // store result
        outputs.push_back(
                {/*.hasNoValue=*/hasNoValue, /*.location=*/location, /*.dimensions=*/dimensions});
    }

    // unpackage pools
    std::vector<int32_t> slots;
    slots.reserve(numberOfPools);
    for (size_t pool = 0; pool < numberOfPools; ++pool) {
        // validate input operand information
        if (data[index].getDiscriminator() != discriminator::poolIdentifier) {
            LOG(ERROR) << "FMQ Request packet ill-formed";
            return std::nullopt;
        }

        // unpackage operand information
        const int32_t poolId = data[index].poolIdentifier();
        index++;

        // store result
        slots.push_back(poolId);
    }

    // validate measureTiming
    if (data[index].getDiscriminator() != discriminator::measureTiming) {
        LOG(ERROR) << "FMQ Request packet ill-formed";
        return std::nullopt;
    }

    // unpackage measureTiming
    const V1_2::MeasureTiming measure = data[index].measureTiming();
    index++;

    // validate packet information
    if (index != packetSize) {
        LOG(ERROR) << "FMQ Result packet ill-formed";
        return std::nullopt;
    }

    // return request
    V1_0::Request request = {/*.inputs=*/inputs, /*.outputs=*/outputs, /*.pools=*/{}};
    return std::make_tuple(std::move(request), std::move(slots), measure);
}

// RequestChannelReceiver methods

std::unique_ptr<RequestChannelReceiver> RequestChannelReceiver::create(
        const FmqRequestDescriptor& requestChannel, std::chrono::microseconds pollingTimeWindow) {
    std::unique_ptr<FmqRequestChannel> fmqRequestChannel =
            std::make_unique<FmqRequestChannel>(requestChannel);

    if (!fmqRequestChannel->isValid()) {
        LOG(ERROR) << "Unable to create RequestChannelReceiver";
        return nullptr;
    }
    if (fmqRequestChannel->getEventFlagWord() == nullptr) {
        LOG(ERROR)
                << "RequestChannelReceiver::create was passed an MQDescriptor without an EventFlag";
        return nullptr;
    }

    return std::make_unique<RequestChannelReceiver>(std::move(fmqRequestChannel),
                                                    pollingTimeWindow);
}

RequestChannelReceiver::RequestChannelReceiver(std::unique_ptr<FmqRequestChannel> fmqRequestChannel,
                                               std::chrono::microseconds pollingTimeWindow)
    : mFmqRequestChannel(std::move(fmqRequestChannel)), kPollingTimeWindow(pollingTimeWindow) {}

std::optional<std::tuple<V1_0::Request, std::vector<int32_t>, V1_2::MeasureTiming>>
RequestChannelReceiver::getBlocking() {
    const auto packet = getPacketBlocking();
    if (!packet) {
        return std::nullopt;
    }

    return deserialize(*packet);
}

void RequestChannelReceiver::invalidate() {
    mTeardown = true;

    // force unblock
    // ExecutionBurstServer is by default waiting on a request packet. If the
    // client process destroys its burst object, the server may still be waiting
    // on the futex. This force unblock wakes up any thread waiting on the
    // futex.
    // TODO: look for a different/better way to signal/notify the futex to wake
    // up any thread waiting on it
    FmqRequestDatum datum;
    datum.packetInformation({/*.packetSize=*/0, /*.numberOfInputOperands=*/0,
                             /*.numberOfOutputOperands=*/0, /*.numberOfPools=*/0});
    mFmqRequestChannel->writeBlocking(&datum, 1);
}

std::optional<std::vector<FmqRequestDatum>> RequestChannelReceiver::getPacketBlocking() {
    if (mTeardown) {
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
        if (mTeardown.load(std::memory_order_relaxed)) {
            return std::nullopt;
        }

        // Check if data is available. If it is, immediately retrieve it and
        // return.
        const size_t available = mFmqRequestChannel->availableToRead();
        if (available > 0) {
            // This is the first point when we know an execution is occurring,
            // so begin to collect systraces. Note that a similar systrace does
            // not exist at the corresponding point in
            // ResultChannelReceiver::getPacketBlocking because the execution is
            // already in flight.
            NNTRACE_FULL(NNTRACE_LAYER_IPC, NNTRACE_PHASE_EXECUTION,
                         "ExecutionBurstServer getting packet");
            std::vector<FmqRequestDatum> packet(available);
            const bool success = mFmqRequestChannel->read(packet.data(), available);
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

    // wait for request packet and read first element of request packet
    FmqRequestDatum datum;
    bool success = mFmqRequestChannel->readBlocking(&datum, 1);

    // This is the first point when we know an execution is occurring, so begin
    // to collect systraces. Note that a similar systrace does not exist at the
    // corresponding point in ResultChannelReceiver::getPacketBlocking because
    // the execution is already in flight.
    NNTRACE_FULL(NNTRACE_LAYER_IPC, NNTRACE_PHASE_EXECUTION, "ExecutionBurstServer getting packet");

    // retrieve remaining elements
    // NOTE: all of the data is already available at this point, so there's no
    // need to do a blocking wait to wait for more data. This is known because
    // in FMQ, all writes are published (made available) atomically. Currently,
    // the producer always publishes the entire packet in one function call, so
    // if the first element of the packet is available, the remaining elements
    // are also available.
    const size_t count = mFmqRequestChannel->availableToRead();
    std::vector<FmqRequestDatum> packet(count + 1);
    std::memcpy(&packet.front(), &datum, sizeof(datum));
    success &= mFmqRequestChannel->read(packet.data() + 1, count);

    // terminate loop
    if (mTeardown) {
        return std::nullopt;
    }

    // ensure packet was successfully received
    if (!success) {
        LOG(ERROR) << "Error receiving packet";
        return std::nullopt;
    }

    return std::make_optional(std::move(packet));
}

// ResultChannelSender methods

std::unique_ptr<ResultChannelSender> ResultChannelSender::create(
        const FmqResultDescriptor& resultChannel) {
    std::unique_ptr<FmqResultChannel> fmqResultChannel =
            std::make_unique<FmqResultChannel>(resultChannel);

    if (!fmqResultChannel->isValid()) {
        LOG(ERROR) << "Unable to create RequestChannelSender";
        return nullptr;
    }
    if (fmqResultChannel->getEventFlagWord() == nullptr) {
        LOG(ERROR) << "ResultChannelSender::create was passed an MQDescriptor without an EventFlag";
        return nullptr;
    }

    return std::make_unique<ResultChannelSender>(std::move(fmqResultChannel));
}

ResultChannelSender::ResultChannelSender(std::unique_ptr<FmqResultChannel> fmqResultChannel)
    : mFmqResultChannel(std::move(fmqResultChannel)) {}

bool ResultChannelSender::send(V1_0::ErrorStatus errorStatus,
                               const std::vector<V1_2::OutputShape>& outputShapes,
                               V1_2::Timing timing) {
    const std::vector<FmqResultDatum> serialized = serialize(errorStatus, outputShapes, timing);
    return sendPacket(serialized);
}

bool ResultChannelSender::sendPacket(const std::vector<FmqResultDatum>& packet) {
    if (packet.size() > mFmqResultChannel->availableToWrite()) {
        LOG(ERROR)
                << "ResultChannelSender::sendPacket -- packet size exceeds size available in FMQ";
        const std::vector<FmqResultDatum> errorPacket =
                serialize(V1_0::ErrorStatus::GENERAL_FAILURE, {}, kNoTiming);

        // Always send the packet with "blocking" because this signals the futex
        // and unblocks the consumer if it is waiting on the futex.
        return mFmqResultChannel->writeBlocking(errorPacket.data(), errorPacket.size());
    }

    // Always send the packet with "blocking" because this signals the futex and
    // unblocks the consumer if it is waiting on the futex.
    return mFmqResultChannel->writeBlocking(packet.data(), packet.size());
}

// ExecutionBurstServer methods

sp<ExecutionBurstServer> ExecutionBurstServer::create(
        const sp<IBurstCallback>& callback, const MQDescriptorSync<FmqRequestDatum>& requestChannel,
        const MQDescriptorSync<FmqResultDatum>& resultChannel,
        std::shared_ptr<IBurstExecutorWithCache> executorWithCache,
        std::chrono::microseconds pollingTimeWindow) {
    // check inputs
    if (callback == nullptr || executorWithCache == nullptr) {
        LOG(ERROR) << "ExecutionBurstServer::create passed a nullptr";
        return nullptr;
    }

    // create FMQ objects
    std::unique_ptr<RequestChannelReceiver> requestChannelReceiver =
            RequestChannelReceiver::create(requestChannel, pollingTimeWindow);
    std::unique_ptr<ResultChannelSender> resultChannelSender =
            ResultChannelSender::create(resultChannel);

    // check FMQ objects
    if (!requestChannelReceiver || !resultChannelSender) {
        LOG(ERROR) << "ExecutionBurstServer::create failed to create FastMessageQueue";
        return nullptr;
    }

    // make and return context
    return new ExecutionBurstServer(callback, std::move(requestChannelReceiver),
                                    std::move(resultChannelSender), std::move(executorWithCache));
}

sp<ExecutionBurstServer> ExecutionBurstServer::create(
        const sp<IBurstCallback>& callback, const MQDescriptorSync<FmqRequestDatum>& requestChannel,
        const MQDescriptorSync<FmqResultDatum>& resultChannel, V1_2::IPreparedModel* preparedModel,
        std::chrono::microseconds pollingTimeWindow) {
    // check relevant input
    if (preparedModel == nullptr) {
        LOG(ERROR) << "ExecutionBurstServer::create passed a nullptr";
        return nullptr;
    }

    // adapt IPreparedModel to have caching
    const std::shared_ptr<DefaultBurstExecutorWithCache> preparedModelAdapter =
            std::make_shared<DefaultBurstExecutorWithCache>(preparedModel);

    // make and return context
    return ExecutionBurstServer::create(callback, requestChannel, resultChannel,
                                        preparedModelAdapter, pollingTimeWindow);
}

ExecutionBurstServer::ExecutionBurstServer(
        const sp<IBurstCallback>& callback, std::unique_ptr<RequestChannelReceiver> requestChannel,
        std::unique_ptr<ResultChannelSender> resultChannel,
        std::shared_ptr<IBurstExecutorWithCache> executorWithCache)
    : mCallback(callback),
      mRequestChannelReceiver(std::move(requestChannel)),
      mResultChannelSender(std::move(resultChannel)),
      mExecutorWithCache(std::move(executorWithCache)) {
    // TODO: highly document the threading behavior of this class
    mWorker = std::thread([this] { task(); });
}

ExecutionBurstServer::~ExecutionBurstServer() {
    // set teardown flag
    mTeardown = true;
    mRequestChannelReceiver->invalidate();

    // wait for task thread to end
    mWorker.join();
}

hardware::Return<void> ExecutionBurstServer::freeMemory(int32_t slot) {
    std::lock_guard<std::mutex> hold(mMutex);
    mExecutorWithCache->removeCacheEntry(slot);
    return hardware::Void();
}

void ExecutionBurstServer::ensureCacheEntriesArePresentLocked(const std::vector<int32_t>& slots) {
    const auto slotIsKnown = [this](int32_t slot) {
        return mExecutorWithCache->isCacheEntryPresent(slot);
    };

    // find unique unknown slots
    std::vector<int32_t> unknownSlots = slots;
    auto unknownSlotsEnd = unknownSlots.end();
    std::sort(unknownSlots.begin(), unknownSlotsEnd);
    unknownSlotsEnd = std::unique(unknownSlots.begin(), unknownSlotsEnd);
    unknownSlotsEnd = std::remove_if(unknownSlots.begin(), unknownSlotsEnd, slotIsKnown);
    unknownSlots.erase(unknownSlotsEnd, unknownSlots.end());

    // quick-exit if all slots are known
    if (unknownSlots.empty()) {
        return;
    }

    V1_0::ErrorStatus errorStatus = V1_0::ErrorStatus::GENERAL_FAILURE;
    std::vector<hardware::hidl_memory> returnedMemories;
    auto cb = [&errorStatus, &returnedMemories](
                      V1_0::ErrorStatus status,
                      const hardware::hidl_vec<hardware::hidl_memory>& memories) {
        errorStatus = status;
        returnedMemories = memories;
    };

    const hardware::Return<void> ret = mCallback->getMemories(unknownSlots, cb);

    if (!ret.isOk() || errorStatus != V1_0::ErrorStatus::NONE ||
        returnedMemories.size() != unknownSlots.size()) {
        LOG(ERROR) << "Error retrieving memories";
        return;
    }

    // add memories to unknown slots
    for (size_t i = 0; i < unknownSlots.size(); ++i) {
        mExecutorWithCache->addCacheEntry(returnedMemories[i], unknownSlots[i]);
    }
}

void ExecutionBurstServer::task() {
    // loop until the burst object is being destroyed
    while (!mTeardown) {
        // receive request
        auto arguments = mRequestChannelReceiver->getBlocking();

        // if the request packet was not properly received, return a generic
        // error and skip the execution
        //
        // if the burst is being torn down, skip the execution so the "task"
        // function can end
        if (!arguments) {
            if (!mTeardown) {
                mResultChannelSender->send(V1_0::ErrorStatus::GENERAL_FAILURE, {}, kNoTiming);
            }
            continue;
        }

        // otherwise begin tracing execution
        NNTRACE_FULL(NNTRACE_LAYER_IPC, NNTRACE_PHASE_EXECUTION,
                     "ExecutionBurstServer getting memory, executing, and returning results");

        // unpack the arguments; types are Request, std::vector<int32_t>, and
        // MeasureTiming, respectively
        const auto [requestWithoutPools, slotsOfPools, measure] = std::move(*arguments);

        // ensure executor with cache has required memory
        std::lock_guard<std::mutex> hold(mMutex);
        ensureCacheEntriesArePresentLocked(slotsOfPools);

        // perform computation; types are ErrorStatus, hidl_vec<OutputShape>,
        // and Timing, respectively
        const auto [errorStatus, outputShapes, returnedTiming] =
                mExecutorWithCache->execute(requestWithoutPools, slotsOfPools, measure);

        // return result
        mResultChannelSender->send(errorStatus, outputShapes, returnedTiming);
    }
}

}  // namespace android::nn
