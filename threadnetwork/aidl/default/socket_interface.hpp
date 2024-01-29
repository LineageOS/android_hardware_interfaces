/*
 * Copyright (C) 2024 The Android Open Source Project
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

/**
 * @file
 *   This file includes definitions for the Socket interface interface to radio
 * (RCP).
 */

#include "lib/spinel/spinel_interface.hpp"
#include "lib/url/url.hpp"

namespace aidl {
namespace android {
namespace hardware {
namespace threadnetwork {

/**
 * Defines a Socket interface to the Radio Co-processor (RCP)
 *
 */
class SocketInterface : public ot::Spinel::SpinelInterface {
  public:
    /**
     * Initializes the object.
     *
     * @param[in] aRadioUrl  RadioUrl parsed from radio url.
     *
     */
    explicit SocketInterface(const ot::Url::Url& aRadioUrl);

    /**
     * This destructor deinitializes the object.
     *
     */
    ~SocketInterface();

    /**
     * Initializes the interface to the Radio Co-processor (RCP)
     *
     * @note This method should be called before reading and sending Spinel
     * frames to the interface.
     *
     * @param[in] aCallback         Callback on frame received
     * @param[in] aCallbackContext  Callback context
     * @param[in] aFrameBuffer      A reference to a `RxFrameBuffer` object.
     *
     * @retval OT_ERROR_NONE       The interface is initialized successfully
     * @retval OT_ERROR_ALREADY    The interface is already initialized.
     * @retval OT_ERROR_FAILED     Failed to initialize the interface.
     *
     */
    otError Init(ReceiveFrameCallback aCallback, void* aCallbackContext,
                 RxFrameBuffer& aFrameBuffer);

    /**
     * Deinitializes the interface to the RCP.
     *
     */
    void Deinit(void);

    /**
     * Sends a Spinel frame to Radio Co-processor (RCP) over the
     * socket.
     *
     * @param[in] aFrame     A pointer to buffer containing the Spinel frame to
     * send.
     * @param[in] aLength    The length (number of bytes) in the frame.
     *
     * @retval OT_ERROR_NONE     Successfully sent the Spinel frame.
     * @retval OT_ERROR_FAILED   Failed to send a frame.
     *
     */
    otError SendFrame(const uint8_t* aFrame, uint16_t aLength);

    /**
     * Waits for receiving part or all of Spinel frame within specified
     * interval.
     *
     * @param[in]  aTimeout  The timeout value in microseconds.
     *
     * @retval OT_ERROR_NONE             Part or all of Spinel frame is
     * received.
     * @retval OT_ERROR_RESPONSE_TIMEOUT No Spinel frame is received within @p
     * aTimeout.
     * @retval OT_EXIT_FAILURE           RCP error
     *
     */
    otError WaitForFrame(uint64_t aTimeoutUs);

    /**
     * Updates the file descriptor sets with file descriptors used by the radio
     * driver.
     *
     * @param[in,out]   aMainloopContext  A pointer to the mainloop context
     * containing fd_sets.
     *
     */
    void UpdateFdSet(void* aMainloopContext);

    /**
     * Performs radio driver processing.
     *
     * @param[in]   aMainloopContext  A pointer to the mainloop context
     * containing fd_sets.
     *
     */
    void Process(const void* aMainloopContext);

    /**
     * Returns the bus speed between the host and the radio.
     *
     * @return   Bus speed in bits/second.
     *
     */
    uint32_t GetBusSpeed(void) const { return 1000000; }

    /**
     * Hardware resets the RCP.
     *
     * @retval OT_ERROR_NONE            Successfully reset the RCP.
     * @retval OT_ERROR_NOT_IMPLEMENT   The hardware reset is not implemented.
     *
     */
    otError HardwareReset(void) { return OT_ERROR_NOT_IMPLEMENTED; }

    /**
     * Returns the RCP interface metrics.
     *
     * @return The RCP interface metrics.
     *
     */
    const otRcpInterfaceMetrics* GetRcpInterfaceMetrics(void) const { return &mInterfaceMetrics; }

    /**
     * Indicates whether or not the given interface matches this interface name.
     *
     * @param[in] aInterfaceName A pointer to the interface name.
     *
     * @retval TRUE   The given interface name matches this interface name.
     * @retval FALSE  The given interface name doesn't match this interface
     * name.
     */
    static bool IsInterfaceNameMatch(const char* aInterfaceName) {
        static const char kInterfaceName[] = "spinel+socket";
        return (strncmp(aInterfaceName, kInterfaceName, strlen(kInterfaceName)) == 0);
    }

  private:
    /**
     * Instructs `SocketInterface` to read data from radio over the
     * socket.
     *
     * If a full Spinel frame is received, this method invokes the
     * `HandleSocketFrame()` (on the `aCallback` object from constructor) to
     * pass the received frame to be processed.
     *
     */
    void Read(void);

    /**
     * Writes a given frame to the socket.
     *
     * @param[in] aFrame  A pointer to buffer containing the frame to write.
     * @param[in] aLength The length (number of bytes) in the frame.
     *
     */
    void Write(const uint8_t* aFrame, uint16_t aLength);

    /**
     * Process received data.
     *
     * If a full frame is finished processing and we obtain the raw Spinel
     * frame, this method invokes the `HandleSocketFrame()` (on the `aCallback`
     * object from constructor) to pass the received frame to be processed.
     *
     * @param[in] aBuffer  A pointer to buffer containing data.
     * @param[in] aLength  The length (number of bytes) in the buffer.
     *
     */
    void ProcessReceivedData(const uint8_t* aBuffer, uint16_t aLength);

    static void HandleSocketFrame(void* aContext, otError aError);
    void HandleSocketFrame(otError aError);

    /**
     * Opens file specified by aRadioUrl.
     *
     * @param[in] aRadioUrl  A reference to object containing path to file and
     * data for configuring the connection with tty type file.
     *
     * @retval The file descriptor of newly opened file.
     * @retval -1 Fail to open file.
     *
     */
    int OpenFile(const ot::Url::Url& aRadioUrl);

    /**
     * Closes file associated with the file descriptor.
     *
     */
    void CloseFile(void);

    /**
     * Check if socket file is created.
     *
     * @param[in] aPath  Socket file path name.
     *
     * @retval TRUE The required socket file is created.
     * @retval FALSE The required socket file is not created.
     *
     */
    bool IsSocketFileExisted(const char* aPath);

    /**
     * Wait until the socket file is created.
     *
     * @param[in] aPath  Socket file path name.
     *
     */
    void WaitForSocketFileCreated(const char* aPath);

    enum {
        kMaxSelectTimeMs = 2000,  ///< Maximum wait time in Milliseconds for file
                                  ///< descriptor to become available.
    };

    ReceiveFrameCallback mReceiveFrameCallback;
    void* mReceiveFrameContext;
    RxFrameBuffer* mReceiveFrameBuffer;

    int mSockFd;
    const ot::Url::Url& mRadioUrl;

    otRcpInterfaceMetrics mInterfaceMetrics;

    // Non-copyable, intentionally not implemented.
    SocketInterface(const SocketInterface&);
    SocketInterface& operator=(const SocketInterface&);
};

}  // namespace threadnetwork
}  // namespace hardware
}  // namespace android
}  // namespace aidl
