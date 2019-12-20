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

#pragma once

#include <android-base/macros.h>
#include <android-base/unique_fd.h>
#include <linux/can.h>

#include <atomic>
#include <chrono>
#include <thread>

namespace android::hardware::automotive::can::V1_0::implementation {

/** Wrapper around SocketCAN socket. */
struct CanSocket {
    using ReadCallback = std::function<void(const struct canfd_frame&, std::chrono::nanoseconds)>;
    using ErrorCallback = std::function<void(int errnoVal)>;

    /**
     * Open and bind SocketCAN socket.
     *
     * \param ifname SocketCAN network interface name (such as can0)
     * \param rdcb Callback on received messages
     * \param errcb Callback on socket failure
     * \return Socket instance, or nullptr if it wasn't possible to open one
     */
    static std::unique_ptr<CanSocket> open(const std::string& ifname, ReadCallback rdcb,
                                           ErrorCallback errcb);
    virtual ~CanSocket();

    /**
     * Send CAN frame.
     *
     * \param frame Frame to send
     * \return true in case of success, false otherwise
     */
    bool send(const struct canfd_frame& frame);

  private:
    CanSocket(base::unique_fd socket, ReadCallback rdcb, ErrorCallback errcb);
    void readerThread();

    ReadCallback mReadCallback;
    ErrorCallback mErrorCallback;

    const base::unique_fd mSocket;
    std::thread mReaderThread;
    std::atomic<bool> mStopReaderThread = false;
    std::atomic<bool> mReaderThreadFinished = false;

    DISALLOW_COPY_AND_ASSIGN(CanSocket);
};

}  // namespace android::hardware::automotive::can::V1_0::implementation
