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

#include "BurstUtils.h"

#include <android-base/logging.h>
#include <android-base/properties.h>
#include <android/hardware/neuralnetworks/1.0/types.h>
#include <android/hardware/neuralnetworks/1.1/types.h>
#include <android/hardware/neuralnetworks/1.2/types.h>
#include <fmq/MessageQueue.h>
#include <hidl/MQDescriptor.h>
#include <nnapi/Result.h>
#include <nnapi/Types.h>
#include <nnapi/hal/1.0/ProtectCallback.h>

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

std::chrono::microseconds getPollingTimeWindow(const std::string& property) {
    constexpr int32_t kDefaultPollingTimeWindow = 0;
#ifdef NN_DEBUGGABLE
    constexpr int32_t kMinPollingTimeWindow = 0;
    const int32_t selectedPollingTimeWindow =
            base::GetIntProperty(property, kDefaultPollingTimeWindow, kMinPollingTimeWindow);
    return std::chrono::microseconds(selectedPollingTimeWindow);
#else
    (void)property;
    return std::chrono::microseconds(kDefaultPollingTimeWindow);
#endif  // NN_DEBUGGABLE
}

}  // namespace

std::chrono::microseconds getBurstControllerPollingTimeWindow() {
    return getPollingTimeWindow("debug.nn.burst-controller-polling-window");
}

std::chrono::microseconds getBurstServerPollingTimeWindow() {
    return getPollingTimeWindow("debug.nn.burst-server-polling-window");
}

// serialize a request into a packet
std::vector<FmqRequestDatum> serialize(const V1_0::Request& request, V1_2::MeasureTiming measure,
                                       const std::vector<int32_t>& slots) {
    // count how many elements need to be sent for a request
    size_t count = 2 + request.inputs.size() + request.outputs.size() + slots.size();
    for (const auto& input : request.inputs) {
        count += input.dimensions.size();
    }
    for (const auto& output : request.outputs) {
        count += output.dimensions.size();
    }
    CHECK_LE(count, std::numeric_limits<uint32_t>::max());

    // create buffer to temporarily store elements
    std::vector<FmqRequestDatum> data;
    data.reserve(count);

    // package packetInfo
    data.emplace_back();
    data.back().packetInformation(
            {.packetSize = static_cast<uint32_t>(count),
             .numberOfInputOperands = static_cast<uint32_t>(request.inputs.size()),
             .numberOfOutputOperands = static_cast<uint32_t>(request.outputs.size()),
             .numberOfPools = static_cast<uint32_t>(slots.size())});

    // package input data
    for (const auto& input : request.inputs) {
        // package operand information
        data.emplace_back();
        data.back().inputOperandInformation(
                {.hasNoValue = input.hasNoValue,
                 .location = input.location,
                 .numberOfDimensions = static_cast<uint32_t>(input.dimensions.size())});

        // package operand dimensions
        for (uint32_t dimension : input.dimensions) {
            data.emplace_back();
            data.back().inputOperandDimensionValue(dimension);
        }
    }

    // package output data
    for (const auto& output : request.outputs) {
        // package operand information
        data.emplace_back();
        data.back().outputOperandInformation(
                {.hasNoValue = output.hasNoValue,
                 .location = output.location,
                 .numberOfDimensions = static_cast<uint32_t>(output.dimensions.size())});

        // package operand dimensions
        for (uint32_t dimension : output.dimensions) {
            data.emplace_back();
            data.back().outputOperandDimensionValue(dimension);
        }
    }

    // package pool identifier
    for (int32_t slot : slots) {
        data.emplace_back();
        data.back().poolIdentifier(slot);
    }

    // package measureTiming
    data.emplace_back();
    data.back().measureTiming(measure);

    CHECK_EQ(data.size(), count);

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
    data.emplace_back();
    data.back().packetInformation({.packetSize = static_cast<uint32_t>(count),
                                   .errorStatus = errorStatus,
                                   .numberOfOperands = static_cast<uint32_t>(outputShapes.size())});

    // package output shape data
    for (const auto& operand : outputShapes) {
        // package operand information
        data.emplace_back();
        data.back().operandInformation(
                {.isSufficient = operand.isSufficient,
                 .numberOfDimensions = static_cast<uint32_t>(operand.dimensions.size())});

        // package operand dimensions
        for (uint32_t dimension : operand.dimensions) {
            data.emplace_back();
            data.back().operandDimensionValue(dimension);
        }
    }

    // package executionTiming
    data.emplace_back();
    data.back().executionTiming(timing);

    CHECK_EQ(data.size(), count);

    // return result
    return data;
}

// deserialize request
nn::Result<std::tuple<V1_0::Request, std::vector<int32_t>, V1_2::MeasureTiming>> deserialize(
        const std::vector<FmqRequestDatum>& data) {
    using discriminator = FmqRequestDatum::hidl_discriminator;

    size_t index = 0;

    // validate packet information
    if (index >= data.size() ||
        data.at(index).getDiscriminator() != discriminator::packetInformation) {
        return NN_ERROR() << "FMQ Request packet ill-formed";
    }

    // unpackage packet information
    const FmqRequestDatum::PacketInformation& packetInfo = data.at(index).packetInformation();
    index++;
    const uint32_t packetSize = packetInfo.packetSize;
    const uint32_t numberOfInputOperands = packetInfo.numberOfInputOperands;
    const uint32_t numberOfOutputOperands = packetInfo.numberOfOutputOperands;
    const uint32_t numberOfPools = packetInfo.numberOfPools;

    // verify packet size
    if (data.size() != packetSize) {
        return NN_ERROR() << "FMQ Request packet ill-formed";
    }

    // unpackage input operands
    std::vector<V1_0::RequestArgument> inputs;
    inputs.reserve(numberOfInputOperands);
    for (size_t operand = 0; operand < numberOfInputOperands; ++operand) {
        // validate input operand information
        if (index >= data.size() ||
            data.at(index).getDiscriminator() != discriminator::inputOperandInformation) {
            return NN_ERROR() << "FMQ Request packet ill-formed";
        }

        // unpackage operand information
        const FmqRequestDatum::OperandInformation& operandInfo =
                data.at(index).inputOperandInformation();
        index++;
        const bool hasNoValue = operandInfo.hasNoValue;
        const V1_0::DataLocation location = operandInfo.location;
        const uint32_t numberOfDimensions = operandInfo.numberOfDimensions;

        // unpackage operand dimensions
        std::vector<uint32_t> dimensions;
        dimensions.reserve(numberOfDimensions);
        for (size_t i = 0; i < numberOfDimensions; ++i) {
            // validate dimension
            if (index >= data.size() ||
                data.at(index).getDiscriminator() != discriminator::inputOperandDimensionValue) {
                return NN_ERROR() << "FMQ Request packet ill-formed";
            }

            // unpackage dimension
            const uint32_t dimension = data.at(index).inputOperandDimensionValue();
            index++;

            // store result
            dimensions.push_back(dimension);
        }

        // store result
        inputs.push_back(
                {.hasNoValue = hasNoValue, .location = location, .dimensions = dimensions});
    }

    // unpackage output operands
    std::vector<V1_0::RequestArgument> outputs;
    outputs.reserve(numberOfOutputOperands);
    for (size_t operand = 0; operand < numberOfOutputOperands; ++operand) {
        // validate output operand information
        if (index >= data.size() ||
            data.at(index).getDiscriminator() != discriminator::outputOperandInformation) {
            return NN_ERROR() << "FMQ Request packet ill-formed";
        }

        // unpackage operand information
        const FmqRequestDatum::OperandInformation& operandInfo =
                data.at(index).outputOperandInformation();
        index++;
        const bool hasNoValue = operandInfo.hasNoValue;
        const V1_0::DataLocation location = operandInfo.location;
        const uint32_t numberOfDimensions = operandInfo.numberOfDimensions;

        // unpackage operand dimensions
        std::vector<uint32_t> dimensions;
        dimensions.reserve(numberOfDimensions);
        for (size_t i = 0; i < numberOfDimensions; ++i) {
            // validate dimension
            if (index >= data.size() ||
                data.at(index).getDiscriminator() != discriminator::outputOperandDimensionValue) {
                return NN_ERROR() << "FMQ Request packet ill-formed";
            }

            // unpackage dimension
            const uint32_t dimension = data.at(index).outputOperandDimensionValue();
            index++;

            // store result
            dimensions.push_back(dimension);
        }

        // store result
        outputs.push_back(
                {.hasNoValue = hasNoValue, .location = location, .dimensions = dimensions});
    }

    // unpackage pools
    std::vector<int32_t> slots;
    slots.reserve(numberOfPools);
    for (size_t pool = 0; pool < numberOfPools; ++pool) {
        // validate input operand information
        if (index >= data.size() ||
            data.at(index).getDiscriminator() != discriminator::poolIdentifier) {
            return NN_ERROR() << "FMQ Request packet ill-formed";
        }

        // unpackage operand information
        const int32_t poolId = data.at(index).poolIdentifier();
        index++;

        // store result
        slots.push_back(poolId);
    }

    // validate measureTiming
    if (index >= data.size() || data.at(index).getDiscriminator() != discriminator::measureTiming) {
        return NN_ERROR() << "FMQ Request packet ill-formed";
    }

    // unpackage measureTiming
    const V1_2::MeasureTiming measure = data.at(index).measureTiming();
    index++;

    // validate packet information
    if (index != packetSize) {
        return NN_ERROR() << "FMQ Request packet ill-formed";
    }

    // return request
    V1_0::Request request = {.inputs = inputs, .outputs = outputs, .pools = {}};
    return std::make_tuple(std::move(request), std::move(slots), measure);
}

// deserialize a packet into the result
nn::Result<std::tuple<V1_0::ErrorStatus, std::vector<V1_2::OutputShape>, V1_2::Timing>> deserialize(
        const std::vector<FmqResultDatum>& data) {
    using discriminator = FmqResultDatum::hidl_discriminator;
    size_t index = 0;

    // validate packet information
    if (index >= data.size() ||
        data.at(index).getDiscriminator() != discriminator::packetInformation) {
        return NN_ERROR() << "FMQ Result packet ill-formed";
    }

    // unpackage packet information
    const FmqResultDatum::PacketInformation& packetInfo = data.at(index).packetInformation();
    index++;
    const uint32_t packetSize = packetInfo.packetSize;
    const V1_0::ErrorStatus errorStatus = packetInfo.errorStatus;
    const uint32_t numberOfOperands = packetInfo.numberOfOperands;

    // verify packet size
    if (data.size() != packetSize) {
        return NN_ERROR() << "FMQ Result packet ill-formed";
    }

    // unpackage operands
    std::vector<V1_2::OutputShape> outputShapes;
    outputShapes.reserve(numberOfOperands);
    for (size_t operand = 0; operand < numberOfOperands; ++operand) {
        // validate operand information
        if (index >= data.size() ||
            data.at(index).getDiscriminator() != discriminator::operandInformation) {
            return NN_ERROR() << "FMQ Result packet ill-formed";
        }

        // unpackage operand information
        const FmqResultDatum::OperandInformation& operandInfo = data.at(index).operandInformation();
        index++;
        const bool isSufficient = operandInfo.isSufficient;
        const uint32_t numberOfDimensions = operandInfo.numberOfDimensions;

        // unpackage operand dimensions
        std::vector<uint32_t> dimensions;
        dimensions.reserve(numberOfDimensions);
        for (size_t i = 0; i < numberOfDimensions; ++i) {
            // validate dimension
            if (index >= data.size() ||
                data.at(index).getDiscriminator() != discriminator::operandDimensionValue) {
                return NN_ERROR() << "FMQ Result packet ill-formed";
            }

            // unpackage dimension
            const uint32_t dimension = data.at(index).operandDimensionValue();
            index++;

            // store result
            dimensions.push_back(dimension);
        }

        // store result
        outputShapes.push_back({.dimensions = dimensions, .isSufficient = isSufficient});
    }

    // validate execution timing
    if (index >= data.size() ||
        data.at(index).getDiscriminator() != discriminator::executionTiming) {
        return NN_ERROR() << "FMQ Result packet ill-formed";
    }

    // unpackage execution timing
    const V1_2::Timing timing = data.at(index).executionTiming();
    index++;

    // validate packet information
    if (index != packetSize) {
        return NN_ERROR() << "FMQ Result packet ill-formed";
    }

    // return result
    return std::make_tuple(errorStatus, std::move(outputShapes), timing);
}

// RequestChannelSender methods

nn::GeneralResult<
        std::pair<std::unique_ptr<RequestChannelSender>, const MQDescriptorSync<FmqRequestDatum>*>>
RequestChannelSender::create(size_t channelLength) {
    auto requestChannelSender =
            std::make_unique<RequestChannelSender>(PrivateConstructorTag{}, channelLength);
    if (!requestChannelSender->mFmqRequestChannel.isValid()) {
        return NN_ERROR() << "Unable to create RequestChannelSender";
    }

    const MQDescriptorSync<FmqRequestDatum>* descriptor =
            requestChannelSender->mFmqRequestChannel.getDesc();
    return std::make_pair(std::move(requestChannelSender), descriptor);
}

RequestChannelSender::RequestChannelSender(PrivateConstructorTag /*tag*/, size_t channelLength)
    : mFmqRequestChannel(channelLength, /*configureEventFlagWord=*/true) {}

nn::Result<void> RequestChannelSender::send(const V1_0::Request& request,
                                            V1_2::MeasureTiming measure,
                                            const std::vector<int32_t>& slots) {
    const std::vector<FmqRequestDatum> serialized = serialize(request, measure, slots);
    return sendPacket(serialized);
}

nn::Result<void> RequestChannelSender::sendPacket(const std::vector<FmqRequestDatum>& packet) {
    if (!mValid) {
        return NN_ERROR() << "FMQ object is invalid";
    }

    if (packet.size() > mFmqRequestChannel.availableToWrite()) {
        return NN_ERROR()
               << "RequestChannelSender::sendPacket -- packet size exceeds size available in FMQ";
    }

    // Always send the packet with "blocking" because this signals the futex and unblocks the
    // consumer if it is waiting on the futex.
    const bool success = mFmqRequestChannel.writeBlocking(packet.data(), packet.size());
    if (!success) {
        return NN_ERROR()
               << "RequestChannelSender::sendPacket -- FMQ's writeBlocking returned an error";
    }

    return {};
}

void RequestChannelSender::notifyAsDeadObject() {
    mValid = false;
}

// RequestChannelReceiver methods

nn::GeneralResult<std::unique_ptr<RequestChannelReceiver>> RequestChannelReceiver::create(
        const MQDescriptorSync<FmqRequestDatum>& requestChannel,
        std::chrono::microseconds pollingTimeWindow) {
    auto requestChannelReceiver = std::make_unique<RequestChannelReceiver>(
            PrivateConstructorTag{}, requestChannel, pollingTimeWindow);

    if (!requestChannelReceiver->mFmqRequestChannel.isValid()) {
        return NN_ERROR() << "Unable to create RequestChannelReceiver";
    }
    if (requestChannelReceiver->mFmqRequestChannel.getEventFlagWord() == nullptr) {
        return NN_ERROR()
               << "RequestChannelReceiver::create was passed an MQDescriptor without an EventFlag";
    }

    return requestChannelReceiver;
}

RequestChannelReceiver::RequestChannelReceiver(
        PrivateConstructorTag /*tag*/, const MQDescriptorSync<FmqRequestDatum>& requestChannel,
        std::chrono::microseconds pollingTimeWindow)
    : mFmqRequestChannel(requestChannel), kPollingTimeWindow(pollingTimeWindow) {}

nn::Result<std::tuple<V1_0::Request, std::vector<int32_t>, V1_2::MeasureTiming>>
RequestChannelReceiver::getBlocking() {
    const auto packet = NN_TRY(getPacketBlocking());
    return deserialize(packet);
}

void RequestChannelReceiver::invalidate() {
    mTeardown = true;

    // force unblock
    // ExecutionBurstServer is by default waiting on a request packet. If the client process
    // destroys its burst object, the server may still be waiting on the futex. This force unblock
    // wakes up any thread waiting on the futex.
    const auto data = serialize(V1_0::Request{}, V1_2::MeasureTiming::NO, {});
    mFmqRequestChannel.writeBlocking(data.data(), data.size());
}

nn::Result<std::vector<FmqRequestDatum>> RequestChannelReceiver::getPacketBlocking() {
    if (mTeardown) {
        return NN_ERROR() << "FMQ object is being torn down";
    }

    // First spend time polling if results are available in FMQ instead of waiting on the futex.
    // Polling is more responsive (yielding lower latencies), but can take up more power, so only
    // poll for a limited period of time.

    auto& getCurrentTime = std::chrono::high_resolution_clock::now;
    const auto timeToStopPolling = getCurrentTime() + kPollingTimeWindow;

    while (getCurrentTime() < timeToStopPolling) {
        // if class is being torn down, immediately return
        if (mTeardown.load(std::memory_order_relaxed)) {
            return NN_ERROR() << "FMQ object is being torn down";
        }

        // Check if data is available. If it is, immediately retrieve it and return.
        const size_t available = mFmqRequestChannel.availableToRead();
        if (available > 0) {
            std::vector<FmqRequestDatum> packet(available);
            const bool success = mFmqRequestChannel.readBlocking(packet.data(), available);
            if (!success) {
                return NN_ERROR() << "Error receiving packet";
            }
            return packet;
        }

        std::this_thread::yield();
    }

    // If we get to this point, we either stopped polling because it was taking too long or polling
    // was not allowed. Instead, perform a blocking call which uses a futex to save power.

    // wait for request packet and read first element of request packet
    FmqRequestDatum datum;
    bool success = mFmqRequestChannel.readBlocking(&datum, 1);

    // retrieve remaining elements
    // NOTE: all of the data is already available at this point, so there's no need to do a blocking
    // wait to wait for more data. This is known because in FMQ, all writes are published (made
    // available) atomically. Currently, the producer always publishes the entire packet in one
    // function call, so if the first element of the packet is available, the remaining elements are
    // also available.
    const size_t count = mFmqRequestChannel.availableToRead();
    std::vector<FmqRequestDatum> packet(count + 1);
    std::memcpy(&packet.front(), &datum, sizeof(datum));
    success &= mFmqRequestChannel.read(packet.data() + 1, count);

    // terminate loop
    if (mTeardown) {
        return NN_ERROR() << "FMQ object is being torn down";
    }

    // ensure packet was successfully received
    if (!success) {
        return NN_ERROR() << "Error receiving packet";
    }

    return packet;
}

// ResultChannelSender methods

nn::GeneralResult<std::unique_ptr<ResultChannelSender>> ResultChannelSender::create(
        const MQDescriptorSync<FmqResultDatum>& resultChannel) {
    auto resultChannelSender =
            std::make_unique<ResultChannelSender>(PrivateConstructorTag{}, resultChannel);

    if (!resultChannelSender->mFmqResultChannel.isValid()) {
        return NN_ERROR() << "Unable to create RequestChannelSender";
    }
    if (resultChannelSender->mFmqResultChannel.getEventFlagWord() == nullptr) {
        return NN_ERROR()
               << "ResultChannelSender::create was passed an MQDescriptor without an EventFlag";
    }

    return resultChannelSender;
}

ResultChannelSender::ResultChannelSender(PrivateConstructorTag /*tag*/,
                                         const MQDescriptorSync<FmqResultDatum>& resultChannel)
    : mFmqResultChannel(resultChannel) {}

void ResultChannelSender::send(V1_0::ErrorStatus errorStatus,
                               const std::vector<V1_2::OutputShape>& outputShapes,
                               V1_2::Timing timing) {
    const std::vector<FmqResultDatum> serialized = serialize(errorStatus, outputShapes, timing);
    sendPacket(serialized);
}

void ResultChannelSender::sendPacket(const std::vector<FmqResultDatum>& packet) {
    if (packet.size() > mFmqResultChannel.availableToWrite()) {
        LOG(ERROR)
                << "ResultChannelSender::sendPacket -- packet size exceeds size available in FMQ";
        const std::vector<FmqResultDatum> errorPacket =
                serialize(V1_0::ErrorStatus::GENERAL_FAILURE, {}, kNoTiming);

        // Always send the packet with "blocking" because this signals the futex and unblocks the
        // consumer if it is waiting on the futex.
        mFmqResultChannel.writeBlocking(errorPacket.data(), errorPacket.size());
    } else {
        // Always send the packet with "blocking" because this signals the futex and unblocks the
        // consumer if it is waiting on the futex.
        mFmqResultChannel.writeBlocking(packet.data(), packet.size());
    }
}

// ResultChannelReceiver methods

nn::GeneralResult<
        std::pair<std::unique_ptr<ResultChannelReceiver>, const MQDescriptorSync<FmqResultDatum>*>>
ResultChannelReceiver::create(size_t channelLength, std::chrono::microseconds pollingTimeWindow) {
    auto resultChannelReceiver = std::make_unique<ResultChannelReceiver>(
            PrivateConstructorTag{}, channelLength, pollingTimeWindow);
    if (!resultChannelReceiver->mFmqResultChannel.isValid()) {
        return NN_ERROR() << "Unable to create ResultChannelReceiver";
    }

    const MQDescriptorSync<FmqResultDatum>* descriptor =
            resultChannelReceiver->mFmqResultChannel.getDesc();
    return std::make_pair(std::move(resultChannelReceiver), descriptor);
}

ResultChannelReceiver::ResultChannelReceiver(PrivateConstructorTag /*tag*/, size_t channelLength,
                                             std::chrono::microseconds pollingTimeWindow)
    : mFmqResultChannel(channelLength, /*configureEventFlagWord=*/true),
      kPollingTimeWindow(pollingTimeWindow) {}

nn::Result<std::tuple<V1_0::ErrorStatus, std::vector<V1_2::OutputShape>, V1_2::Timing>>
ResultChannelReceiver::getBlocking() {
    const auto packet = NN_TRY(getPacketBlocking());
    return deserialize(packet);
}

void ResultChannelReceiver::notifyAsDeadObject() {
    mValid = false;

    // force unblock
    // ExecutionBurstController waits on a result packet after sending a request. If the driver
    // containing ExecutionBurstServer crashes, the controller may be waiting on the futex. This
    // force unblock wakes up any thread waiting on the futex.
    const auto data = serialize(V1_0::ErrorStatus::GENERAL_FAILURE, {}, kNoTiming);
    mFmqResultChannel.writeBlocking(data.data(), data.size());
}

nn::Result<std::vector<FmqResultDatum>> ResultChannelReceiver::getPacketBlocking() {
    if (!mValid) {
        return NN_ERROR() << "FMQ object is invalid";
    }

    // First spend time polling if results are available in FMQ instead of waiting on the futex.
    // Polling is more responsive (yielding lower latencies), but can take up more power, so only
    // poll for a limited period of time.

    auto& getCurrentTime = std::chrono::high_resolution_clock::now;
    const auto timeToStopPolling = getCurrentTime() + kPollingTimeWindow;

    while (getCurrentTime() < timeToStopPolling) {
        // if class is being torn down, immediately return
        if (!mValid.load(std::memory_order_relaxed)) {
            return NN_ERROR() << "FMQ object is invalid";
        }

        // Check if data is available. If it is, immediately retrieve it and return.
        const size_t available = mFmqResultChannel.availableToRead();
        if (available > 0) {
            std::vector<FmqResultDatum> packet(available);
            const bool success = mFmqResultChannel.readBlocking(packet.data(), available);
            if (!success) {
                return NN_ERROR() << "Error receiving packet";
            }
            return packet;
        }

        std::this_thread::yield();
    }

    // If we get to this point, we either stopped polling because it was taking too long or polling
    // was not allowed. Instead, perform a blocking call which uses a futex to save power.

    // wait for result packet and read first element of result packet
    FmqResultDatum datum;
    bool success = mFmqResultChannel.readBlocking(&datum, 1);

    // retrieve remaining elements
    // NOTE: all of the data is already available at this point, so there's no need to do a blocking
    // wait to wait for more data. This is known because in FMQ, all writes are published (made
    // available) atomically. Currently, the producer always publishes the entire packet in one
    // function call, so if the first element of the packet is available, the remaining elements are
    // also available.
    const size_t count = mFmqResultChannel.availableToRead();
    std::vector<FmqResultDatum> packet(count + 1);
    std::memcpy(&packet.front(), &datum, sizeof(datum));
    success &= mFmqResultChannel.read(packet.data() + 1, count);

    if (!mValid) {
        return NN_ERROR() << "FMQ object is invalid";
    }

    // ensure packet was successfully received
    if (!success) {
        return NN_ERROR() << "Error receiving packet";
    }

    return packet;
}

}  // namespace android::hardware::neuralnetworks::V1_2::utils
