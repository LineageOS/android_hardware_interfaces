/*
 * Copyright (C) 2017 The Android Open Source Project
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

#define LOG_TAG "SocketComm"

#include <android/hardware/automotive/vehicle/2.0/IVehicle.h>
#include <android/log.h>
#include <arpa/inet.h>
#include <log/log.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "SocketComm.h"

// Socket to use when communicating with Host PC
static constexpr int DEBUG_SOCKET = 33452;

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace V2_0 {

namespace impl {

SocketComm::SocketComm(MessageProcessor* messageProcessor)
    : mListenFd(-1), mMessageProcessor(messageProcessor) {}

SocketComm::~SocketComm() {
}

void SocketComm::start() {
    if (!listen()) {
        return;
    }

    mListenThread = std::make_unique<std::thread>(std::bind(&SocketComm::listenThread, this));
}

void SocketComm::stop() {
    if (mListenFd > 0) {
        ::close(mListenFd);
        if (mListenThread->joinable()) {
            mListenThread->join();
        }
        mListenFd = -1;
    }
}

void SocketComm::sendMessage(vhal_proto::EmulatorMessage const& msg) {
    std::lock_guard<std::mutex> lock(mMutex);
    for (std::unique_ptr<SocketConn> const& conn : mOpenConnections) {
        conn->sendMessage(msg);
    }
}

bool SocketComm::listen() {
    int retVal;
    struct sockaddr_in servAddr;

    mListenFd = socket(AF_INET, SOCK_STREAM, 0);
    if (mListenFd < 0) {
        ALOGE("%s: socket() failed, mSockFd=%d, errno=%d", __FUNCTION__, mListenFd, errno);
        mListenFd = -1;
        return false;
    }

    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = INADDR_ANY;
    servAddr.sin_port = htons(DEBUG_SOCKET);

    retVal = bind(mListenFd, reinterpret_cast<struct sockaddr*>(&servAddr), sizeof(servAddr));
    if(retVal < 0) {
        ALOGE("%s: Error on binding: retVal=%d, errno=%d", __FUNCTION__, retVal, errno);
        close(mListenFd);
        mListenFd = -1;
        return false;
    }

    ALOGI("%s: Listening for connections on port %d", __FUNCTION__, DEBUG_SOCKET);
    if (::listen(mListenFd, 1) == -1) {
        ALOGE("%s: Error on listening: errno: %d: %s", __FUNCTION__, errno, strerror(errno));
        return false;
    }
    return true;
}

SocketConn* SocketComm::accept() {
    sockaddr_in cliAddr;
    socklen_t cliLen = sizeof(cliAddr);
    int sfd = ::accept(mListenFd, reinterpret_cast<struct sockaddr*>(&cliAddr), &cliLen);

    if (sfd > 0) {
        char addr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &cliAddr.sin_addr, addr, INET_ADDRSTRLEN);

        ALOGD("%s: Incoming connection received from %s:%d", __FUNCTION__, addr, cliAddr.sin_port);
        return new SocketConn(mMessageProcessor, sfd);
    }

    return nullptr;
}

void SocketComm::listenThread() {
    while (true) {
        SocketConn* conn = accept();
        if (conn == nullptr) {
            return;
        }

        conn->start();
        {
            std::lock_guard<std::mutex> lock(mMutex);
            mOpenConnections.push_back(std::unique_ptr<SocketConn>(conn));
        }
    }
}

/**
 * Called occasionally to clean up connections that have been closed.
 */
void SocketComm::removeClosedConnections() {
    std::lock_guard<std::mutex> lock(mMutex);
    std::remove_if(mOpenConnections.begin(), mOpenConnections.end(),
                   [](std::unique_ptr<SocketConn> const& c) { return !c->isOpen(); });
}

SocketConn::SocketConn(MessageProcessor* messageProcessor, int sfd)
    : CommConn(messageProcessor), mSockFd(sfd) {}

/**
 * Reads, in a loop, exactly numBytes from the given fd. If the connection is closed, returns
 * an empty buffer, otherwise will return exactly the given number of bytes.
 */
std::vector<uint8_t> readExactly(int fd, int numBytes) {
    std::vector<uint8_t> buffer(numBytes);
    int totalRead = 0;
    int offset = 0;
    while (totalRead < numBytes) {
        int numRead = ::read(fd, &buffer.data()[offset], numBytes - offset);
        if (numRead == 0) {
            buffer.resize(0);
            return buffer;
        }

        totalRead += numRead;
    }
    return buffer;
}

/**
 * Reads an int, guaranteed to be non-zero, from the given fd. If the connection is closed, returns
 * -1.
 */
int32_t readInt(int fd) {
    std::vector<uint8_t> buffer = readExactly(fd, sizeof(int32_t));
    if (buffer.size() == 0) {
        return -1;
    }

    int32_t value = *reinterpret_cast<int32_t*>(buffer.data());
    return ntohl(value);
}

std::vector<uint8_t> SocketConn::read() {
    int32_t msgSize = readInt(mSockFd);
    if (msgSize <= 0) {
        ALOGD("%s: Connection terminated on socket %d", __FUNCTION__, mSockFd);
        return std::vector<uint8_t>();
    }

    return readExactly(mSockFd, msgSize);
}

void SocketConn::stop() {
    if (mSockFd > 0) {
        close(mSockFd);
        mSockFd = -1;
    }
}

int SocketConn::write(const std::vector<uint8_t>& data) {
    static constexpr int MSG_HEADER_LEN = 4;
    int retVal = 0;
    union {
        uint32_t msgLen;
        uint8_t msgLenBytes[MSG_HEADER_LEN];
    };

    // Prepare header for the message
    msgLen = static_cast<uint32_t>(data.size());
    msgLen = htonl(msgLen);

    if (mSockFd > 0) {
        retVal = ::write(mSockFd, msgLenBytes, MSG_HEADER_LEN);

        if (retVal == MSG_HEADER_LEN) {
            retVal = ::write(mSockFd, data.data(), data.size());
        }
    }

    return retVal;
}

}  // impl

}  // namespace V2_0
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android

