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

#ifndef ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_1_2_UTILS_EXECUTION_BURST_UTILS_H
#define ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_1_2_UTILS_EXECUTION_BURST_UTILS_H

#include <android/hardware/neuralnetworks/1.0/types.h>
#include <android/hardware/neuralnetworks/1.1/types.h>
#include <android/hardware/neuralnetworks/1.2/types.h>
#include <fmq/MessageQueue.h>
#include <hidl/MQDescriptor.h>

#include <atomic>
#include <chrono>
#include <memory>
#include <optional>
#include <tuple>
#include <utility>
#include <vector>

namespace android::hardware::neuralnetworks::V1_2::utils {

/**
 * Number of elements in the FMQ.
 */
constexpr const size_t kExecutionBurstChannelLength = 1024;

using FmqRequestDescriptor = MQDescriptorSync<FmqRequestDatum>;
using FmqResultDescriptor = MQDescriptorSync<FmqResultDatum>;

/**
 * Function to serialize a request.
 *
 * Prefer calling RequestChannelSender::send.
 *
 * @param request Request object without the pool information.
 * @param measure Whether to collect timing information for the execution.
 * @param memoryIds Slot identifiers corresponding to memory resources for the
 *     request.
 * @return Serialized FMQ request data.
 */
std::vector<hardware::neuralnetworks::V1_2::FmqRequestDatum> serialize(
        const hardware::neuralnetworks::V1_0::Request& request,
        hardware::neuralnetworks::V1_2::MeasureTiming measure, const std::vector<int32_t>& slots);

/**
 * Deserialize the FMQ request data.
 *
 * The three resulting fields are the Request object (where Request::pools is
 * empty), slot identifiers (which are stand-ins for Request::pools), and
 * whether timing information must be collected for the run.
 *
 * @param data Serialized FMQ request data.
 * @return Request object if successfully deserialized, std::nullopt otherwise.
 */
std::optional<std::tuple<hardware::neuralnetworks::V1_0::Request, std::vector<int32_t>,
                         hardware::neuralnetworks::V1_2::MeasureTiming>>
deserialize(const std::vector<hardware::neuralnetworks::V1_2::FmqRequestDatum>& data);

/**
 * Function to serialize results.
 *
 * Prefer calling ResultChannelSender::send.
 *
 * @param errorStatus Status of the execution.
 * @param outputShapes Dynamic shapes of the output tensors.
 * @param timing Timing information of the execution.
 * @return Serialized FMQ result data.
 */
std::vector<hardware::neuralnetworks::V1_2::FmqResultDatum> serialize(
        hardware::neuralnetworks::V1_0::ErrorStatus errorStatus,
        const std::vector<hardware::neuralnetworks::V1_2::OutputShape>& outputShapes,
        hardware::neuralnetworks::V1_2::Timing timing);

/**
 * Deserialize the FMQ result data.
 *
 * The three resulting fields are the status of the execution, the dynamic
 * shapes of the output tensors, and the timing information of the execution.
 *
 * @param data Serialized FMQ result data.
 * @return Result object if successfully deserialized, std::nullopt otherwise.
 */
std::optional<std::tuple<hardware::neuralnetworks::V1_0::ErrorStatus,
                         std::vector<hardware::neuralnetworks::V1_2::OutputShape>,
                         hardware::neuralnetworks::V1_2::Timing>>
deserialize(const std::vector<hardware::neuralnetworks::V1_2::FmqResultDatum>& data);

/**
 * Convert result code to error status.
 *
 * @param resultCode Result code to be converted.
 * @return ErrorStatus Resultant error status.
 */
hardware::neuralnetworks::V1_0::ErrorStatus legacyConvertResultCodeToErrorStatus(int resultCode);

/**
 * RequestChannelSender is responsible for serializing the result packet of
 * information, sending it on the result channel, and signaling that the data is
 * available.
 */
class RequestChannelSender {
    using FmqRequestDescriptor =
            hardware::MQDescriptorSync<hardware::neuralnetworks::V1_2::FmqRequestDatum>;
    using FmqRequestChannel =
            hardware::MessageQueue<hardware::neuralnetworks::V1_2::FmqRequestDatum,
                                   hardware::kSynchronizedReadWrite>;

  public:
    /**
     * Create the sending end of a request channel.
     *
     * Prefer this call over the constructor.
     *
     * @param channelLength Number of elements in the FMQ.
     * @return A pair of ResultChannelReceiver and the FMQ descriptor on
     *     successful creation, both nullptr otherwise.
     */
    static std::pair<std::unique_ptr<RequestChannelSender>, const FmqRequestDescriptor*> create(
            size_t channelLength);

    /**
     * Send the request to the channel.
     *
     * @param request Request object without the pool information.
     * @param measure Whether to collect timing information for the execution.
     * @param memoryIds Slot identifiers corresponding to memory resources for
     *     the request.
     * @return 'true' on successful send, 'false' otherwise.
     */
    bool send(const hardware::neuralnetworks::V1_0::Request& request,
              hardware::neuralnetworks::V1_2::MeasureTiming measure,
              const std::vector<int32_t>& slots);

    /**
     * Method to mark the channel as invalid, causing all future calls to
     * RequestChannelSender::send to immediately return false without attempting
     * to send a message across the FMQ.
     */
    void invalidate();

    // prefer calling RequestChannelSender::send
    bool sendPacket(const std::vector<hardware::neuralnetworks::V1_2::FmqRequestDatum>& packet);

    RequestChannelSender(std::unique_ptr<FmqRequestChannel> fmqRequestChannel);

  private:
    const std::unique_ptr<FmqRequestChannel> mFmqRequestChannel;
    std::atomic<bool> mValid{true};
};

/**
 * RequestChannelReceiver is responsible for waiting on the channel until the
 * packet is available, extracting the packet from the channel, and
 * deserializing the packet.
 *
 * Because the receiver can wait on a packet that may never come (e.g., because
 * the sending side of the packet has been closed), this object can be
 * invalidated, unblocking the receiver.
 */
class RequestChannelReceiver {
    using FmqRequestChannel =
            hardware::MessageQueue<hardware::neuralnetworks::V1_2::FmqRequestDatum,
                                   hardware::kSynchronizedReadWrite>;

  public:
    /**
     * Create the receiving end of a request channel.
     *
     * Prefer this call over the constructor.
     *
     * @param requestChannel Descriptor for the request channel.
     * @param pollingTimeWindow How much time (in microseconds) the
     *     RequestChannelReceiver is allowed to poll the FMQ before waiting on
     *     the blocking futex. Polling may result in lower latencies at the
     *     potential cost of more power usage.
     * @return RequestChannelReceiver on successful creation, nullptr otherwise.
     */
    static std::unique_ptr<RequestChannelReceiver> create(
            const FmqRequestDescriptor& requestChannel,
            std::chrono::microseconds pollingTimeWindow);

    /**
     * Get the request from the channel.
     *
     * This method will block until either:
     * 1) The packet has been retrieved, or
     * 2) The receiver has been invalidated
     *
     * @return Request object if successfully received, std::nullopt if error or
     *     if the receiver object was invalidated.
     */
    std::optional<std::tuple<hardware::neuralnetworks::V1_0::Request, std::vector<int32_t>,
                             hardware::neuralnetworks::V1_2::MeasureTiming>>
    getBlocking();

    /**
     * Method to mark the channel as invalid, unblocking any current or future
     * calls to RequestChannelReceiver::getBlocking.
     */
    void invalidate();

    RequestChannelReceiver(std::unique_ptr<FmqRequestChannel> fmqRequestChannel,
                           std::chrono::microseconds pollingTimeWindow);

  private:
    std::optional<std::vector<hardware::neuralnetworks::V1_2::FmqRequestDatum>> getPacketBlocking();

    const std::unique_ptr<FmqRequestChannel> mFmqRequestChannel;
    std::atomic<bool> mTeardown{false};
    const std::chrono::microseconds kPollingTimeWindow;
};

/**
 * ResultChannelSender is responsible for serializing the result packet of
 * information, sending it on the result channel, and signaling that the data is
 * available.
 */
class ResultChannelSender {
    using FmqResultChannel = hardware::MessageQueue<hardware::neuralnetworks::V1_2::FmqResultDatum,
                                                    hardware::kSynchronizedReadWrite>;

  public:
    /**
     * Create the sending end of a result channel.
     *
     * Prefer this call over the constructor.
     *
     * @param resultChannel Descriptor for the result channel.
     * @return ResultChannelSender on successful creation, nullptr otherwise.
     */
    static std::unique_ptr<ResultChannelSender> create(const FmqResultDescriptor& resultChannel);

    /**
     * Send the result to the channel.
     *
     * @param errorStatus Status of the execution.
     * @param outputShapes Dynamic shapes of the output tensors.
     * @param timing Timing information of the execution.
     * @return 'true' on successful send, 'false' otherwise.
     */
    bool send(hardware::neuralnetworks::V1_0::ErrorStatus errorStatus,
              const std::vector<hardware::neuralnetworks::V1_2::OutputShape>& outputShapes,
              hardware::neuralnetworks::V1_2::Timing timing);

    // prefer calling ResultChannelSender::send
    bool sendPacket(const std::vector<hardware::neuralnetworks::V1_2::FmqResultDatum>& packet);

    ResultChannelSender(std::unique_ptr<FmqResultChannel> fmqResultChannel);

  private:
    const std::unique_ptr<FmqResultChannel> mFmqResultChannel;
};

/**
 * ResultChannelReceiver is responsible for waiting on the channel until the
 * packet is available, extracting the packet from the channel, and
 * deserializing the packet.
 *
 * Because the receiver can wait on a packet that may never come (e.g., because
 * the sending side of the packet has been closed), this object can be
 * invalidated, unblocking the receiver.
 */
class ResultChannelReceiver {
    using FmqResultDescriptor =
            hardware::MQDescriptorSync<hardware::neuralnetworks::V1_2::FmqResultDatum>;
    using FmqResultChannel = hardware::MessageQueue<hardware::neuralnetworks::V1_2::FmqResultDatum,
                                                    hardware::kSynchronizedReadWrite>;

  public:
    /**
     * Create the receiving end of a result channel.
     *
     * Prefer this call over the constructor.
     *
     * @param channelLength Number of elements in the FMQ.
     * @param pollingTimeWindow How much time (in microseconds) the
     *     ResultChannelReceiver is allowed to poll the FMQ before waiting on
     *     the blocking futex. Polling may result in lower latencies at the
     *     potential cost of more power usage.
     * @return A pair of ResultChannelReceiver and the FMQ descriptor on
     *     successful creation, both nullptr otherwise.
     */
    static std::pair<std::unique_ptr<ResultChannelReceiver>, const FmqResultDescriptor*> create(
            size_t channelLength, std::chrono::microseconds pollingTimeWindow);

    /**
     * Get the result from the channel.
     *
     * This method will block until either:
     * 1) The packet has been retrieved, or
     * 2) The receiver has been invalidated
     *
     * @return Result object if successfully received, std::nullopt if error or
     *     if the receiver object was invalidated.
     */
    std::optional<std::tuple<hardware::neuralnetworks::V1_0::ErrorStatus,
                             std::vector<hardware::neuralnetworks::V1_2::OutputShape>,
                             hardware::neuralnetworks::V1_2::Timing>>
    getBlocking();

    /**
     * Method to mark the channel as invalid, unblocking any current or future
     * calls to ResultChannelReceiver::getBlocking.
     */
    void invalidate();

    // prefer calling ResultChannelReceiver::getBlocking
    std::optional<std::vector<hardware::neuralnetworks::V1_2::FmqResultDatum>> getPacketBlocking();

    ResultChannelReceiver(std::unique_ptr<FmqResultChannel> fmqResultChannel,
                          std::chrono::microseconds pollingTimeWindow);

  private:
    const std::unique_ptr<FmqResultChannel> mFmqResultChannel;
    std::atomic<bool> mValid{true};
    const std::chrono::microseconds kPollingTimeWindow;
};

}  // namespace android::hardware::neuralnetworks::V1_2::utils

#endif  // ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_1_2_UTILS_EXECUTION_BURST_UTILS_H
