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
#include <android/hardware/neuralnetworks/1.2/types.h>
#include <fmq/MessageQueue.h>
#include <hidl/MQDescriptor.h>
#include <nnapi/Result.h>
#include <nnapi/Types.h>
#include <nnapi/hal/ProtectCallback.h>

#include <atomic>
#include <chrono>
#include <memory>
#include <tuple>
#include <utility>
#include <vector>

namespace android::hardware::neuralnetworks::V1_2::utils {

/**
 * Number of elements in the FMQ.
 */
constexpr const size_t kExecutionBurstChannelLength = 1024;

/**
 * Get how long the burst controller should poll while waiting for results to be returned.
 *
 * This time can be affected by the property "debug.nn.burst-controller-polling-window".
 *
 * @return Polling time in microseconds.
 */
std::chrono::microseconds getBurstControllerPollingTimeWindow();

/**
 * Get how long the burst server should poll while waiting for a request to be received.
 *
 * This time can be affected by the property "debug.nn.burst-server-polling-window".
 *
 * @return Polling time in microseconds.
 */
std::chrono::microseconds getBurstServerPollingTimeWindow();

/**
 * Function to serialize a request.
 *
 * @param request Request object without the pool information.
 * @param measure Whether to collect timing information for the execution.
 * @param memoryIds Slot identifiers corresponding to memory resources for the request.
 * @return Serialized FMQ request data.
 */
std::vector<FmqRequestDatum> serialize(const V1_0::Request& request, MeasureTiming measure,
                                       const std::vector<int32_t>& slots);

/**
 * Deserialize the FMQ request data.
 *
 * The three resulting fields are the Request object (where Request::pools is empty), slot
 * identifiers (which are stand-ins for Request::pools), and whether timing information must be
 * collected for the run.
 *
 * @param data Serialized FMQ request data.
 * @return Request object if successfully deserialized, otherwise an error message.
 */
nn::Result<std::tuple<V1_0::Request, std::vector<int32_t>, MeasureTiming>> deserialize(
        const std::vector<FmqRequestDatum>& data);

/**
 * Function to serialize results.
 *
 * @param errorStatus Status of the execution.
 * @param outputShapes Dynamic shapes of the output tensors.
 * @param timing Timing information of the execution.
 * @return Serialized FMQ result data.
 */
std::vector<FmqResultDatum> serialize(V1_0::ErrorStatus errorStatus,
                                      const std::vector<OutputShape>& outputShapes, Timing timing);

/**
 * Deserialize the FMQ result data.
 *
 * The three resulting fields are the status of the execution, the dynamic shapes of the output
 * tensors, and the timing information of the execution.
 *
 * @param data Serialized FMQ result data.
 * @return Result object if successfully deserialized, otherwise an error message.
 */
nn::Result<std::tuple<V1_0::ErrorStatus, std::vector<OutputShape>, Timing>> deserialize(
        const std::vector<FmqResultDatum>& data);

/**
 * RequestChannelSender is responsible for serializing the result packet of information, sending it
 * on the result channel, and signaling that the data is available.
 */
class RequestChannelSender final : public neuralnetworks::utils::IProtectedCallback {
    struct PrivateConstructorTag {};

  public:
    /**
     * Create the sending end of a request channel.
     *
     * @param channelLength Number of elements in the FMQ.
     * @return A pair of ResultChannelReceiver and the FMQ descriptor on successful creation,
     *     GeneralError otherwise.
     */
    static nn::GeneralResult<std::pair<std::unique_ptr<RequestChannelSender>,
                                       const MQDescriptorSync<FmqRequestDatum>*>>
    create(size_t channelLength);

    /**
     * Send the request to the channel.
     *
     * @param request Request object without the pool information.
     * @param measure Whether to collect timing information for the execution.
     * @param slots Slot identifiers corresponding to memory resources for the request.
     * @return An empty `Result` on successful send, otherwise an error message.
     */
    nn::Result<void> send(const V1_0::Request& request, MeasureTiming measure,
                          const std::vector<int32_t>& slots);

    /**
     * Method to mark the channel as invalid, causing all future calls to RequestChannelSender::send
     * to immediately return false without attempting to send a message across the FMQ.
     */
    void notifyAsDeadObject() override;

    // prefer calling RequestChannelSender::send
    nn::Result<void> sendPacket(const std::vector<FmqRequestDatum>& packet);

    RequestChannelSender(PrivateConstructorTag tag, size_t channelLength);

  private:
    MessageQueue<FmqRequestDatum, kSynchronizedReadWrite> mFmqRequestChannel;
    std::atomic<bool> mValid{true};
};

/**
 * RequestChannelReceiver is responsible for waiting on the channel until the packet is available,
 * extracting the packet from the channel, and deserializing the packet.
 *
 * Because the receiver can wait on a packet that may never come (e.g., because the sending side of
 * the packet has been closed), this object can be invalidated, unblocking the receiver.
 */
class RequestChannelReceiver final {
    struct PrivateConstructorTag {};

  public:
    /**
     * Create the receiving end of a request channel.
     *
     * @param requestChannel Descriptor for the request channel.
     * @param pollingTimeWindow How much time (in microseconds) the RequestChannelReceiver is
     *     allowed to poll the FMQ before waiting on the blocking futex. Polling may result in lower
     *     latencies at the potential cost of more power usage.
     * @return RequestChannelReceiver on successful creation, nullptr otherwise.
     */
    static nn::GeneralResult<std::unique_ptr<RequestChannelReceiver>> create(
            const MQDescriptorSync<FmqRequestDatum>& requestChannel,
            std::chrono::microseconds pollingTimeWindow);

    /**
     * Get the request from the channel.
     *
     * This method will block until either:
     * 1) The packet has been retrieved, or
     * 2) The receiver has been invalidated
     *
     * @return Request object if successfully received, an appropriate message if error or if the
     *     receiver object was invalidated.
     */
    nn::Result<std::tuple<V1_0::Request, std::vector<int32_t>, MeasureTiming>> getBlocking();

    /**
     * Method to mark the channel as invalid, unblocking any current or future calls to
     * RequestChannelReceiver::getBlocking.
     */
    void invalidate();

    RequestChannelReceiver(PrivateConstructorTag tag,
                           const MQDescriptorSync<FmqRequestDatum>& requestChannel,
                           std::chrono::microseconds pollingTimeWindow);

  private:
    nn::Result<std::vector<FmqRequestDatum>> getPacketBlocking();

    MessageQueue<FmqRequestDatum, kSynchronizedReadWrite> mFmqRequestChannel;
    std::atomic<bool> mTeardown{false};
    const std::chrono::microseconds kPollingTimeWindow;
};

/**
 * ResultChannelSender is responsible for serializing the result packet of information, sending it
 * on the result channel, and signaling that the data is available.
 */
class ResultChannelSender final {
    struct PrivateConstructorTag {};

  public:
    /**
     * Create the sending end of a result channel.
     *
     * @param resultChannel Descriptor for the result channel.
     * @return ResultChannelSender on successful creation, nullptr otherwise.
     */
    static nn::GeneralResult<std::unique_ptr<ResultChannelSender>> create(
            const MQDescriptorSync<FmqResultDatum>& resultChannel);

    /**
     * Send the result to the channel.
     *
     * @param errorStatus Status of the execution.
     * @param outputShapes Dynamic shapes of the output tensors.
     * @param timing Timing information of the execution.
     */
    void send(V1_0::ErrorStatus errorStatus, const std::vector<OutputShape>& outputShapes,
              Timing timing);

    // prefer calling ResultChannelSender::send
    void sendPacket(const std::vector<FmqResultDatum>& packet);

    ResultChannelSender(PrivateConstructorTag tag,
                        const MQDescriptorSync<FmqResultDatum>& resultChannel);

  private:
    MessageQueue<FmqResultDatum, kSynchronizedReadWrite> mFmqResultChannel;
};

/**
 * ResultChannelReceiver is responsible for waiting on the channel until the packet is available,
 * extracting the packet from the channel, and deserializing the packet.
 *
 * Because the receiver can wait on a packet that may never come (e.g., because the sending side of
 * the packet has been closed), this object can be invalidated, unblocking the receiver.
 */
class ResultChannelReceiver final : public neuralnetworks::utils::IProtectedCallback {
    struct PrivateConstructorTag {};

  public:
    /**
     * Create the receiving end of a result channel.
     *
     * @param channelLength Number of elements in the FMQ.
     * @param pollingTimeWindow How much time (in microseconds) the ResultChannelReceiver is allowed
     *     to poll the FMQ before waiting on the blocking futex. Polling may result in lower
     *     latencies at the potential cost of more power usage.
     * @return A pair of ResultChannelReceiver and the FMQ descriptor on successful creation, or
     *     GeneralError otherwise.
     */
    static nn::GeneralResult<std::pair<std::unique_ptr<ResultChannelReceiver>,
                                       const MQDescriptorSync<FmqResultDatum>*>>
    create(size_t channelLength, std::chrono::microseconds pollingTimeWindow);

    /**
     * Get the result from the channel.
     *
     * This method will block until either:
     * 1) The packet has been retrieved, or
     * 2) The receiver has been invalidated
     *
     * @return Result object if successfully received, otherwise an appropriate message if error or
     *     if the receiver object was invalidated.
     */
    nn::Result<std::tuple<V1_0::ErrorStatus, std::vector<OutputShape>, Timing>> getBlocking();

    /**
     * Method to mark the channel as invalid, unblocking any current or future calls to
     * ResultChannelReceiver::getBlocking.
     */
    void notifyAsDeadObject() override;

    // prefer calling ResultChannelReceiver::getBlocking
    nn::Result<std::vector<FmqResultDatum>> getPacketBlocking();

    ResultChannelReceiver(PrivateConstructorTag tag, size_t channelLength,
                          std::chrono::microseconds pollingTimeWindow);

  private:
    MessageQueue<FmqResultDatum, kSynchronizedReadWrite> mFmqResultChannel;
    std::atomic<bool> mValid{true};
    const std::chrono::microseconds kPollingTimeWindow;
};

}  // namespace android::hardware::neuralnetworks::V1_2::utils

#endif  // ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_1_2_UTILS_EXECUTION_BURST_UTILS_H
