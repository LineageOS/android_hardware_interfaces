/*
 * Copyright (C) 2021 The Android Open Source Project
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

#include "InterceptorRelay.h"

#include <android-base/logging.h>
#include <libnl++/printer.h>
#include <poll.h>

#include <chrono>

#include "util.h"

namespace android::nlinterceptor {
using namespace std::chrono_literals;

static constexpr std::chrono::milliseconds kPollTimeout = 300ms;
static constexpr bool kSuperVerbose = false;

InterceptorRelay::InterceptorRelay(uint32_t nlFamily, uint32_t clientNlPid,
                                   const std::string& clientName)
    : mClientName(clientName),
      mNlSocket(std::make_optional<nl::Socket>(nlFamily, 0, 0)),
      mClientNlPid(clientNlPid) {}

InterceptorRelay::~InterceptorRelay() {
    mRunning = false;
    if (mRelayThread.joinable()) mRelayThread.join();
}

uint32_t InterceptorRelay::getPid() {
    auto pidMaybe = mNlSocket->getPid();
    CHECK(pidMaybe.has_value()) << "Failed to get pid of nl::Socket!";
    return *pidMaybe;
}

void InterceptorRelay::relayMessages() {
    pollfd fds[] = {
        mNlSocket->preparePoll(POLLIN),
    };
    while (mRunning) {
        if (poll(fds, countof(fds), kPollTimeout.count()) < 0) {
            PLOG(FATAL) << "poll failed";
            return;
        }
        const auto nlsockEvents = fds[0].revents;

        if (isSocketBad(nlsockEvents)) {
            LOG(ERROR) << "Netlink socket is bad";
            mRunning = false;
            return;
        }
        if (!isSocketReadable(nlsockEvents)) continue;

        const auto [msgMaybe, sa] = mNlSocket->receiveFrom();
        if (!msgMaybe.has_value()) {
            LOG(ERROR) << "Failed to receive Netlink data!";
            mRunning = false;
            return;
        }
        const auto msg = *msgMaybe;
        if (!msg.firstOk()) {
            LOG(ERROR) << "Netlink packet is malformed!";
            // Test messages might be empty, this isn't fatal.
            continue;
        }
        if constexpr (kSuperVerbose) {
            LOG(VERBOSE) << "[" << mClientName
                         << "] nlMsg: " << nl::toString(msg, NETLINK_GENERIC);
        }

        uint32_t destinationPid = 0;
        if (sa.nl_pid == 0) {
            destinationPid = mClientNlPid;
        }

        if (!mNlSocket->send(msg, destinationPid)) {
            LOG(ERROR) << "Failed to send Netlink message!";
            mRunning = false;
            return;
        }
    }
    LOG(VERBOSE) << "[" << mClientName << "] Exiting relay thread!";
}

bool InterceptorRelay::start() {
    if (mRunning) {
        LOG(ERROR)
            << "Can't relay messages: InterceptorRelay is already running!";
        return false;
    }
    if (mRelayThread.joinable()) {
        LOG(ERROR) << "relay thread is already running!";
        return false;
    }
    if (!mNlSocket.has_value()) {
        LOG(ERROR) << "Netlink socket not initialized!";
        return false;
    }

    mRunning = true;
    mRelayThread = std::thread(&InterceptorRelay::relayMessages, this);

    LOG(VERBOSE) << "Relay threads initialized";
    return true;
}

bool InterceptorRelay::subscribeGroup(uint32_t nlGroup) {
    return mNlSocket->addMembership(nlGroup);
}

bool InterceptorRelay::unsubscribeGroup(uint32_t nlGroup) {
    return mNlSocket->dropMembership(nlGroup);
}

}  // namespace android::nlinterceptor
