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

#define LOG_TAG "ExecutionBurstUtils"

#include "ExecutionBurstUtils.h"

#include <android-base/logging.h>
#include <android/hardware/neuralnetworks/1.0/types.h>
#include <android/hardware/neuralnetworks/1.1/types.h>
#include <android/hardware/neuralnetworks/1.2/types.h>
#include <fmq/MessageQueue.h>
#include <hidl/MQDescriptor.h>

#include <atomic>
#include <chrono>
#include <memory>
#include <thread>
#include <tuple>
#include <utility>
#include <vector>

namespace android::hardware::neuralnetworks::V1_2::utils {
namespace {

constexpr V1_2::Timing kNoTiming = {std::numeric_limits<uint64_t>::max(),
                                    std::numeric_limits<uint64_t>::max()};

}

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

// RequestChannelSender methods

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

// ResultChannelReceiver methods

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

}  // namespace android::hardware::neuralnetworks::V1_2::utils
