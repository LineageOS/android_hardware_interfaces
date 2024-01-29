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
 *   This file includes the implementation for the Socket interface to radio
 * (RCP).
 */

#include "socket_interface.hpp"

#include <errno.h>
#include <openthread/logging.h>
#include <sys/inotify.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

#include <string>

#include "common/code_utils.hpp"
#include "openthread/openthread-system.h"
#include "platform-posix.h"

namespace aidl {
namespace android {
namespace hardware {
namespace threadnetwork {

SocketInterface::SocketInterface(const ot::Url::Url& aRadioUrl)
    : mReceiveFrameCallback(nullptr),
      mReceiveFrameContext(nullptr),
      mReceiveFrameBuffer(nullptr),
      mSockFd(-1),
      mRadioUrl(aRadioUrl) {
    memset(&mInterfaceMetrics, 0, sizeof(mInterfaceMetrics));
    mInterfaceMetrics.mRcpInterfaceType = kSpinelInterfaceTypeVendor;
}

otError SocketInterface::Init(ReceiveFrameCallback aCallback, void* aCallbackContext,
                              RxFrameBuffer& aFrameBuffer) {
    otError error = OT_ERROR_NONE;

    VerifyOrExit(mSockFd == -1, error = OT_ERROR_ALREADY);

    WaitForSocketFileCreated(mRadioUrl.GetPath());

    mSockFd = OpenFile(mRadioUrl);
    VerifyOrExit(mSockFd != -1, error = OT_ERROR_FAILED);

    mReceiveFrameCallback = aCallback;
    mReceiveFrameContext = aCallbackContext;
    mReceiveFrameBuffer = &aFrameBuffer;

exit:
    return error;
}

SocketInterface::~SocketInterface(void) {
    Deinit();
}

void SocketInterface::Deinit(void) {
    CloseFile();

    mReceiveFrameCallback = nullptr;
    mReceiveFrameContext = nullptr;
    mReceiveFrameBuffer = nullptr;
}

otError SocketInterface::SendFrame(const uint8_t* aFrame, uint16_t aLength) {
    Write(aFrame, aLength);

    return OT_ERROR_NONE;
}

otError SocketInterface::WaitForFrame(uint64_t aTimeoutUs) {
    otError error = OT_ERROR_NONE;
    struct timeval timeout;
    timeout.tv_sec = static_cast<time_t>(aTimeoutUs / US_PER_S);
    timeout.tv_usec = static_cast<suseconds_t>(aTimeoutUs % US_PER_S);

    fd_set readFds;
    fd_set errorFds;
    int rval;

    FD_ZERO(&readFds);
    FD_ZERO(&errorFds);
    FD_SET(mSockFd, &readFds);
    FD_SET(mSockFd, &errorFds);

    rval = TEMP_FAILURE_RETRY(select(mSockFd + 1, &readFds, nullptr, &errorFds, &timeout));

    if (rval > 0) {
        if (FD_ISSET(mSockFd, &readFds)) {
            Read();
        } else if (FD_ISSET(mSockFd, &errorFds)) {
            DieNowWithMessage("RCP error", OT_EXIT_FAILURE);
        } else {
            DieNow(OT_EXIT_FAILURE);
        }
    } else if (rval == 0) {
        ExitNow(error = OT_ERROR_RESPONSE_TIMEOUT);
    } else {
        DieNowWithMessage("wait response", OT_EXIT_FAILURE);
    }

exit:
    return error;
}

void SocketInterface::UpdateFdSet(void* aMainloopContext) {
    otSysMainloopContext* context = reinterpret_cast<otSysMainloopContext*>(aMainloopContext);

    assert(context != nullptr);

    FD_SET(mSockFd, &context->mReadFdSet);

    if (context->mMaxFd < mSockFd) {
        context->mMaxFd = mSockFd;
    }
}

void SocketInterface::Process(const void* aMainloopContext) {
    const otSysMainloopContext* context =
            reinterpret_cast<const otSysMainloopContext*>(aMainloopContext);

    assert(context != nullptr);

    if (FD_ISSET(mSockFd, &context->mReadFdSet)) {
        Read();
    }
}

void SocketInterface::Read(void) {
    uint8_t buffer[kMaxFrameSize];

    ssize_t rval = TEMP_FAILURE_RETRY(read(mSockFd, buffer, sizeof(buffer)));

    if (rval > 0) {
        ProcessReceivedData(buffer, static_cast<uint16_t>(rval));
    } else if (rval < 0) {
        DieNow(OT_EXIT_ERROR_ERRNO);
    } else {
        otLogCritPlat("Socket connection is closed by remote.");
        exit(OT_EXIT_FAILURE);
    }
}

void SocketInterface::Write(const uint8_t* aFrame, uint16_t aLength) {
    ssize_t rval = TEMP_FAILURE_RETRY(write(mSockFd, aFrame, aLength));
    VerifyOrDie(rval >= 0, OT_EXIT_ERROR_ERRNO);
    VerifyOrDie(rval > 0, OT_EXIT_FAILURE);
}

void SocketInterface::ProcessReceivedData(const uint8_t* aBuffer, uint16_t aLength) {
    while (aLength--) {
        uint8_t byte = *aBuffer++;
        if (mReceiveFrameBuffer->CanWrite(sizeof(uint8_t))) {
            IgnoreError(mReceiveFrameBuffer->WriteByte(byte));
        } else {
            HandleSocketFrame(this, OT_ERROR_NO_BUFS);
            return;
        }
    }
    HandleSocketFrame(this, OT_ERROR_NONE);
}

void SocketInterface::HandleSocketFrame(void* aContext, otError aError) {
    static_cast<SocketInterface*>(aContext)->HandleSocketFrame(aError);
}

void SocketInterface::HandleSocketFrame(otError aError) {
    VerifyOrExit((mReceiveFrameCallback != nullptr) && (mReceiveFrameBuffer != nullptr));

    if (aError == OT_ERROR_NONE) {
        mReceiveFrameCallback(mReceiveFrameContext);
    } else {
        mReceiveFrameBuffer->DiscardFrame();
        otLogWarnPlat("Process socket frame failed: %s", otThreadErrorToString(aError));
    }

exit:
    return;
}

int SocketInterface::OpenFile(const ot::Url::Url& aRadioUrl) {
    int fd = -1;
    sockaddr_un serverAddress;

    VerifyOrExit(sizeof(serverAddress.sun_path) > strlen(aRadioUrl.GetPath()),
                 otLogCritPlat("Invalid file path length"));
    strncpy(serverAddress.sun_path, aRadioUrl.GetPath(), sizeof(serverAddress.sun_path));
    serverAddress.sun_family = AF_UNIX;

    fd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    VerifyOrExit(fd != -1, otLogCritPlat("open(): errno=%s", strerror(errno)));

    if (connect(fd, reinterpret_cast<struct sockaddr*>(&serverAddress), sizeof(serverAddress)) ==
        -1) {
        otLogCritPlat("connect(): errno=%s", strerror(errno));
        close(fd);
        fd = -1;
    }

exit:
    return fd;
}

void SocketInterface::CloseFile(void) {
    VerifyOrExit(mSockFd != -1);

    VerifyOrExit(0 == close(mSockFd), otLogCritPlat("close(): errno=%s", strerror(errno)));
    VerifyOrExit(wait(nullptr) != -1 || errno == ECHILD,
                 otLogCritPlat("wait(): errno=%s", strerror(errno)));

    mSockFd = -1;

exit:
    return;
}

void SocketInterface::WaitForSocketFileCreated(const char* aPath) {
    int inotifyFd;
    int wd;
    int lastSlashIdx;
    std::string folderPath;
    std::string socketPath(aPath);

    VerifyOrExit(!IsSocketFileExisted(aPath));

    inotifyFd = inotify_init();
    VerifyOrDie(inotifyFd != -1, OT_EXIT_ERROR_ERRNO);

    lastSlashIdx = socketPath.find_last_of('/');
    VerifyOrDie(lastSlashIdx != std::string::npos, OT_EXIT_ERROR_ERRNO);

    folderPath = socketPath.substr(0, lastSlashIdx);
    wd = inotify_add_watch(inotifyFd, folderPath.c_str(), IN_CREATE);
    VerifyOrDie(wd != -1, OT_EXIT_ERROR_ERRNO);

    otLogInfoPlat("Waiting for socket file %s be created...", aPath);

    while (true) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(inotifyFd, &fds);
        struct timeval timeout = {kMaxSelectTimeMs / MS_PER_S,
                                  (kMaxSelectTimeMs % MS_PER_S) * MS_PER_S};

        int rval = select(inotifyFd + 1, &fds, nullptr, nullptr, &timeout);
        VerifyOrDie(rval >= 0, OT_EXIT_ERROR_ERRNO);

        if (rval == 0 && IsSocketFileExisted(aPath)) {
            break;
        }

        if (FD_ISSET(inotifyFd, &fds)) {
            char buffer[sizeof(struct inotify_event)];
            ssize_t bytesRead = read(inotifyFd, buffer, sizeof(buffer));

            VerifyOrDie(bytesRead >= 0, OT_EXIT_ERROR_ERRNO);

            struct inotify_event* event = reinterpret_cast<struct inotify_event*>(buffer);
            if ((event->mask & IN_CREATE) && IsSocketFileExisted(aPath)) {
                break;
            }
        }
    }

    close(inotifyFd);

exit:
    otLogInfoPlat("Socket file: %s is created", aPath);
    return;
}

bool SocketInterface::IsSocketFileExisted(const char* aPath) {
    struct stat st;
    return stat(aPath, &st) == 0 && S_ISSOCK(st.st_mode);
}

}  // namespace threadnetwork
}  // namespace hardware
}  // namespace android
}  // namespace aidl
