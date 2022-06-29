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

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <aidl/android/hardware/net/nlinterceptor/IInterceptor.h>
#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <gtest/gtest.h>
#include <libnetdevice/libnetdevice.h>
#include <libnl++/MessageFactory.h>
#include <libnl++/Socket.h>
#include <libnl++/printer.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>

#include <chrono>
#include <thread>

using aidl::android::hardware::net::nlinterceptor::IInterceptor;
using AidlInterceptedSocket =
    ::aidl::android::hardware::net::nlinterceptor::InterceptedSocket;
using namespace std::chrono_literals;
using namespace std::string_literals;

class InterceptorAidlTest : public ::testing::TestWithParam<std::string> {
   public:
    virtual void SetUp() override {
        android::base::SetDefaultTag("InterceptorAidlTest");
        android::base::SetMinimumLogSeverity(android::base::VERBOSE);
        const auto instance = IInterceptor::descriptor + "/default"s;
        mNlInterceptorService = IInterceptor::fromBinder(
            ndk::SpAIBinder(AServiceManager_getService(instance.c_str())));

        ASSERT_NE(mNlInterceptorService, nullptr);
        mSocket = std::make_unique<android::nl::Socket>(NETLINK_ROUTE);
        ASSERT_TRUE(mSocket->getPid().has_value());

        // If the test broke last run, clean up our mess, don't worry about "no
        // such device".
        if (android::netdevice::del(mTestIfaceName)) {
            LOG(WARNING) << "Test interface wasn't cleaned up on previous run!";
        }
    }

    void multicastReceiver();

    std::shared_ptr<IInterceptor> mNlInterceptorService;
    std::unique_ptr<android::nl::Socket> mSocket;
    bool mRunning;
    bool mGotMulticast;
    const std::string mTestIfaceName = "interceptorvts0";
};

TEST_P(InterceptorAidlTest, createSocketTest) {
    // Ask IInterceptor for a socket.
    AidlInterceptedSocket interceptedSocket;
    auto aidlStatus = mNlInterceptorService->createSocket(
        NETLINK_ROUTE, *(mSocket->getPid()), "createSocketTest",
        &interceptedSocket);
    ASSERT_TRUE(aidlStatus.isOk());
    ASSERT_NE(interceptedSocket.portId, 0);
    uint32_t interceptorPid = interceptedSocket.portId;

    // Ask the kernel to tell us what interfaces are available.
    android::nl::MessageFactory<rtgenmsg> req(RTM_GETLINK,
                                              NLM_F_REQUEST | NLM_F_DUMP);
    req->rtgen_family = AF_PACKET;
    sockaddr_nl sa = {.nl_family = AF_NETLINK,
                      .nl_pad = 0,
                      .nl_pid = interceptorPid,
                      .nl_groups = 0};
    EXPECT_TRUE(mSocket->send(req, sa));

    // We'll likely get back several messages, as indicated by the MULTI flag.
    unsigned received = 0;
    for (const auto msg : *mSocket) {
        ASSERT_NE(msg->nlmsg_type, NLMSG_ERROR);
        ++received;
        break;
        if (msg->nlmsg_type == NLMSG_DONE) {
            // TODO(202548749): NLMSG_DONE on NETLINK_ROUTE doesn't work?
            break;
        }
    }
    ASSERT_GE(received, 1);

    // Close the socket and make sure it's stopped working.
    aidlStatus = mNlInterceptorService->closeSocket(interceptedSocket);
    EXPECT_TRUE(aidlStatus.isOk());
    EXPECT_FALSE(mSocket->send(req, sa));
}

static bool isSocketReadable(const short revents) {
    return 0 != (revents & POLLIN);
}

static bool isSocketBad(const short revents) {
    return 0 != (revents & (POLLERR | POLLHUP | POLLNVAL));
}

void InterceptorAidlTest::multicastReceiver() {
    pollfd fds[] = {
        mSocket->preparePoll(POLLIN),
    };
    while (mRunning) {
        if (poll(fds, 1, 300) < 0) {
            PLOG(FATAL) << "poll failed";
            return;
        }
        const auto nlsockEvents = fds[0].revents;
        ASSERT_FALSE(isSocketBad(nlsockEvents));
        if (!isSocketReadable(nlsockEvents)) continue;

        const auto [msgMaybe, sa] = mSocket->receiveFrom();
        ASSERT_TRUE(msgMaybe.has_value());
        auto msg = *msgMaybe;

        // Multicast messages have 0 for their pid and sequence number.
        if (msg->nlmsg_pid == 0 && msg->nlmsg_seq == 0) {
            mGotMulticast = true;
        }
    }
}

TEST_P(InterceptorAidlTest, subscribeGroupTest) {
    // Ask IInterceptor for a socket.
    AidlInterceptedSocket interceptedSocket;
    auto aidlStatus = mNlInterceptorService->createSocket(
        NETLINK_ROUTE, *(mSocket->getPid()), "subscribeGroupTest",
        &interceptedSocket);
    ASSERT_TRUE(aidlStatus.isOk());
    ASSERT_TRUE(interceptedSocket.portId != 0);

    // Listen for interface up/down events.
    aidlStatus =
        mNlInterceptorService->subscribeGroup(interceptedSocket, RTNLGRP_LINK);
    ASSERT_TRUE(aidlStatus.isOk());

    // Start a thread to receive a multicast
    mRunning = true;
    mGotMulticast = false;
    std::thread successfulReceiver(&InterceptorAidlTest::multicastReceiver,
                                   this);

    // TODO(201695162): use futures with wait_for instead of a sleep_for().
    std::this_thread::sleep_for(50ms);
    // create a network interface and bring it up to trigger a multicast event.
    ASSERT_TRUE(android::netdevice::add(mTestIfaceName, /*type=*/"dummy"));
    ASSERT_TRUE(android::netdevice::up(mTestIfaceName));
    std::this_thread::sleep_for(50ms);
    EXPECT_TRUE(mGotMulticast);
    mRunning = false;
    successfulReceiver.join();

    // Stop listening to interface up/down events.
    aidlStatus = mNlInterceptorService->unsubscribeGroup(interceptedSocket,
                                                         RTNLGRP_LINK);
    ASSERT_TRUE(aidlStatus.isOk());

    // This time, we should hear nothing.
    mGotMulticast = false;
    mRunning = true;
    std::thread unsuccessfulReceiver(&InterceptorAidlTest::multicastReceiver,
                                     this);
    std::this_thread::sleep_for(50ms);
    ASSERT_TRUE(android::netdevice::down(mTestIfaceName));
    ASSERT_TRUE(android::netdevice::del(mTestIfaceName));
    std::this_thread::sleep_for(50ms);
    EXPECT_FALSE(mGotMulticast);
    mRunning = false;
    unsuccessfulReceiver.join();

    aidlStatus = mNlInterceptorService->closeSocket(interceptedSocket);
    EXPECT_TRUE(aidlStatus.isOk());
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(InterceptorAidlTest);
INSTANTIATE_TEST_SUITE_P(PerInstance, InterceptorAidlTest,
                         testing::ValuesIn(android::getAidlHalInstanceNames(
                             IInterceptor::descriptor)),
                         android::PrintInstanceNameToString);
