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

#include <VtsHalHidlTargetTestBase.h>
#include <android-base/logging.h>
#include <android-base/strings.h>
#include <android/hardware/automotive/can/1.0/ICanBus.h>
#include <android/hardware/automotive/can/1.0/ICanController.h>
#include <android/hardware/automotive/can/1.0/types.h>
#include <android/hidl/manager/1.2/IServiceManager.h>
#include <can-vts-utils/bus-enumerator.h>
#include <can-vts-utils/can-hal-printers.h>
#include <can-vts-utils/environment-utils.h>
#include <gmock/gmock.h>
#include <hidl-utils/hidl-utils.h>
#include <utils/Mutex.h>
#include <utils/SystemClock.h>

#include <chrono>
#include <thread>

namespace android::hardware::automotive::can::V1_0::vts {

using namespace std::chrono_literals;

using hardware::hidl_vec;
using InterfaceType = ICanController::InterfaceType;

static utils::SimpleHidlEnvironment<ICanController>* gEnv = nullptr;

struct CanMessageListener : public can::V1_0::ICanMessageListener {
    DISALLOW_COPY_AND_ASSIGN(CanMessageListener);

    CanMessageListener() {}

    virtual Return<void> onReceive(const can::V1_0::CanMessage& msg) override {
        std::unique_lock<std::mutex> lk(mMessagesGuard);
        mMessages.push_back(msg);
        mMessagesUpdated.notify_one();
        return {};
    }

    virtual ~CanMessageListener() { mCloseHandle->close(); }

    void assignCloseHandle(sp<ICloseHandle> closeHandle) {
        EXPECT_TRUE(closeHandle);
        EXPECT_FALSE(mCloseHandle);
        mCloseHandle = closeHandle;
    }

    std::vector<can::V1_0::CanMessage> fetchMessages(std::chrono::milliseconds timeout,
                                                     unsigned atLeast = 1) {
        std::unique_lock<std::mutex> lk(mMessagesGuard);
        mMessagesUpdated.wait_for(lk, timeout, [&] { return mMessages.size() >= atLeast; });
        const auto messages = mMessages;
        mMessages.clear();
        return messages;
    }

  private:
    sp<ICloseHandle> mCloseHandle;

    std::mutex mMessagesGuard;
    std::condition_variable mMessagesUpdated GUARDED_BY(mMessagesGuard);
    std::vector<can::V1_0::CanMessage> mMessages GUARDED_BY(mMessagesGuard);
};

struct Bus {
    DISALLOW_COPY_AND_ASSIGN(Bus);

    Bus(sp<ICanController> controller, const ICanController::BusConfig& config)
        : mIfname(config.name), mController(controller) {
        const auto result = controller->upInterface(config);
        EXPECT_EQ(ICanController::Result::OK, result);

        /* Not using ICanBus::getService here, since it ignores interfaces not in the manifest
         * file -- this is a test, so we don't want to add dummy services to a device manifest. */
        auto manager = hidl::manager::V1_2::IServiceManager::getService();
        auto service = manager->get(ICanBus::descriptor, config.name);
        mBus = ICanBus::castFrom(service);
        EXPECT_TRUE(mBus) << "ICanBus/" << config.name << " failed to register";
    }

    virtual ~Bus() { reset(); }

    void reset() {
        mBus.clear();
        if (mController) {
            const auto res = mController->downInterface(mIfname);
            EXPECT_TRUE(res);
            mController.clear();
        }
    }

    ICanBus* operator->() const { return mBus.get(); }
    sp<ICanBus> get() { return mBus; }

    sp<CanMessageListener> listen(const hidl_vec<CanMessageFilter>& filter) {
        sp<CanMessageListener> listener = new CanMessageListener();

        Result result;
        sp<ICloseHandle> closeHandle;
        mBus->listen(filter, listener, hidl_utils::fill(&result, &closeHandle)).assertOk();
        EXPECT_EQ(Result::OK, result);
        listener->assignCloseHandle(closeHandle);

        return listener;
    }

    void send(const CanMessage& msg) {
        EXPECT_NE(mBus, nullptr);
        if (!mBus) return;
        const auto result = mBus->send(msg);
        EXPECT_EQ(Result::OK, result);
    }

  private:
    const std::string mIfname;
    sp<ICanController> mController;
    sp<ICanBus> mBus;
};

class CanBusVirtualHalTest : public ::testing::VtsHalHidlTargetTestBase {
  protected:
    virtual void SetUp() override;

    static void SetUpTestCase();
    static void TearDownTestCase();

    Bus makeBus();

  protected:
    static hidl_vec<hidl_string> mBusNames;

  private:
    unsigned mLastIface = 0;
    static sp<ICanController> mCanController;
    static bool mVirtualSupported;
    static bool mTestCaseInitialized;
};

sp<ICanController> CanBusVirtualHalTest::mCanController = nullptr;
bool CanBusVirtualHalTest::mVirtualSupported;
hidl_vec<hidl_string> CanBusVirtualHalTest::mBusNames;
bool CanBusVirtualHalTest::mTestCaseInitialized = false;

static CanMessage makeMessage(CanMessageId id, bool rtr, bool extended) {
    CanMessage msg = {};
    msg.id = id;
    msg.remoteTransmissionRequest = rtr;
    msg.isExtendedId = extended;
    return msg;
}

static void clearTimestamps(std::vector<CanMessage>& messages) {
    std::for_each(messages.begin(), messages.end(), [](auto& msg) { msg.timestamp = 0; });
}

void CanBusVirtualHalTest::SetUp() {
    if (!mVirtualSupported) GTEST_SKIP();
    ASSERT_TRUE(mTestCaseInitialized);
}

void CanBusVirtualHalTest::SetUpTestCase() {
    const auto serviceName = gEnv->getServiceName<ICanController>();
    mCanController = getService<ICanController>(serviceName);
    ASSERT_TRUE(mCanController) << "Couldn't open CAN Controller: " << serviceName;

    hidl_vec<InterfaceType> supported;
    mCanController->getSupportedInterfaceTypes(hidl_utils::fill(&supported)).assertOk();
    mVirtualSupported = supported.contains(InterfaceType::VIRTUAL);

    mBusNames = utils::getBusNames();
    ASSERT_NE(0u, mBusNames.size()) << "No ICanBus HALs defined in device manifest";

    mTestCaseInitialized = true;
}

void CanBusVirtualHalTest::TearDownTestCase() {
    mCanController.clear();
}

Bus CanBusVirtualHalTest::makeBus() {
    const auto idx = mLastIface++;
    EXPECT_LT(idx, mBusNames.size());

    ICanController::BusConfig config = {};
    config.name = mBusNames[idx];
    config.interfaceId.virtualif({"vcan50"});

    return Bus(mCanController, config);
}

TEST_F(CanBusVirtualHalTest, Send) {
    auto bus = makeBus();

    CanMessage msg = {};
    msg.id = 0x123;
    msg.payload = {1, 2, 3};

    bus.send(msg);
}

TEST_F(CanBusVirtualHalTest, SendAfterClose) {
    auto bus = makeBus();
    auto zombie = bus.get();
    bus.reset();

    const auto result = zombie->send({});
    ASSERT_EQ(Result::INTERFACE_DOWN, result);
}

TEST_F(CanBusVirtualHalTest, SendAndRecv) {
    if (mBusNames.size() < 2u) GTEST_SKIP() << "Not testable with less than two CAN buses.";
    auto bus1 = makeBus();
    auto bus2 = makeBus();

    auto listener = bus2.listen({});

    CanMessage msg = {};
    msg.id = 0x123;
    msg.payload = {1, 2, 3};
    bus1.send(msg);

    auto messages = listener->fetchMessages(100ms);
    ASSERT_EQ(1u, messages.size());
    ASSERT_NEAR(uint64_t(elapsedRealtimeNano()), messages[0].timestamp,
                std::chrono::nanoseconds(100ms).count());
    clearTimestamps(messages);
    ASSERT_EQ(msg, messages[0]);
}

TEST_F(CanBusVirtualHalTest, DownOneOfTwo) {
    if (mBusNames.size() < 2u) GTEST_SKIP() << "Not testable with less than two CAN buses.";

    auto bus1 = makeBus();
    auto bus2 = makeBus();

    bus2.reset();

    bus1.send({});
}

TEST_F(CanBusVirtualHalTest, FilterPositive) {
    if (mBusNames.size() < 2u) GTEST_SKIP() << "Not testable with less than two CAN buses.";
    auto bus1 = makeBus();
    auto bus2 = makeBus();

    /* clang-format off */
    /*        id,            mask,           rtr,                   eff,          exclude */
    hidl_vec<CanMessageFilter> filterPositive = {
            {0x334,           0x73F, FilterFlag::DONT_CARE, FilterFlag::DONT_CARE, false},
            {0x49D,           0x700, FilterFlag::DONT_CARE, FilterFlag::DONT_CARE, false},
            {0x325,           0x7FC, FilterFlag::DONT_CARE, FilterFlag::NOT_SET,   false},
            {0x246,           0x7FF, FilterFlag::SET,       FilterFlag::DONT_CARE, false},
            {0x1A2,           0x7FB, FilterFlag::SET,       FilterFlag::NOT_SET,   false},
            {0x607,           0x7C9, FilterFlag::NOT_SET,   FilterFlag::DONT_CARE, false},
            {0x7F4,           0x777, FilterFlag::NOT_SET,   FilterFlag::NOT_SET,   false},
            {0x1BF19EAF, 0x10F0F0F0, FilterFlag::DONT_CARE, FilterFlag::DONT_CARE, false},
            {0x12E99200, 0x1FFFFFFF, FilterFlag::DONT_CARE, FilterFlag::SET,       false},
            {0x06B70270, 0x1FFFFFFF, FilterFlag::SET,       FilterFlag::DONT_CARE, false},
            {0x096CFD2B, 0x1FFFFFFF, FilterFlag::SET,       FilterFlag::SET,       false},
            {0x1BDCB008, 0x0F0F0F0F, FilterFlag::NOT_SET,   FilterFlag::DONT_CARE, false},
            {0x08318B46, 0x10F0F0F0, FilterFlag::NOT_SET,   FilterFlag::SET,       false},
            {0x06B,           0x70F, FilterFlag::DONT_CARE, FilterFlag::SET,       false},
            {0x750,           0x70F, FilterFlag::SET,       FilterFlag::SET,       false},
            {0x5CF,           0x70F, FilterFlag::NOT_SET,   FilterFlag::SET,       false},
    };
    /* clang-format on */
    auto listenerPositive = bus2.listen(filterPositive);

    // 334:73F, DNC, DNC
    bus1.send(makeMessage(0x3F4, false, false));
    bus1.send(makeMessage(0x334, false, true));
    bus1.send(makeMessage(0x374, true, false));
    bus1.send(makeMessage(0x3F4, true, true));

    // 49D:700, DNC, DNC
    bus1.send(makeMessage(0x404, false, false));
    bus1.send(makeMessage(0x4A5, false, true));
    bus1.send(makeMessage(0x4FF, true, false));
    bus1.send(makeMessage(0x46B, true, true));

    // 325:7FC, DNC, NS
    bus1.send(makeMessage(0x324, false, false));
    bus1.send(makeMessage(0x325, false, true));  // filtered out
    bus1.send(makeMessage(0x326, true, false));
    bus1.send(makeMessage(0x327, true, true));  // filtered out

    // 246:7FF, SET, DNC
    bus1.send(makeMessage(0x246, false, false));  // filtered out
    bus1.send(makeMessage(0x246, false, true));   // filtered out
    bus1.send(makeMessage(0x246, true, false));
    bus1.send(makeMessage(0x246, true, true));

    // 1A2:7FB, SET, NS
    bus1.send(makeMessage(0x1A2, false, false));  // filtered out
    bus1.send(makeMessage(0x1A6, false, true));   // filtered out
    bus1.send(makeMessage(0x1A2, true, false));
    bus1.send(makeMessage(0x1A6, true, true));  // filtered out

    // 607:7C9, NS, DNC
    bus1.send(makeMessage(0x607, false, false));
    bus1.send(makeMessage(0x613, false, true));
    bus1.send(makeMessage(0x625, true, false));  // filtered out
    bus1.send(makeMessage(0x631, true, true));   // filtered out

    // 7F4:777, NS, NS
    bus1.send(makeMessage(0x774, false, false));
    bus1.send(makeMessage(0x7F4, false, true));  // filtered out
    bus1.send(makeMessage(0x77C, true, false));  // filtered out
    bus1.send(makeMessage(0x7FC, true, false));  // filtered out

    // 1BF19EAF:10F0F0F0, DNC, DNC
    bus1.send(makeMessage(0x11F293A4, false, false));
    bus1.send(makeMessage(0x15F697A8, false, true));
    bus1.send(makeMessage(0x19FA9BAC, true, false));
    bus1.send(makeMessage(0x1DFE9FA0, true, true));

    // 12E99200:1FFFFFFF, DNC, SET
    bus1.send(makeMessage(0x12E99200, false, false));  // filtered out
    bus1.send(makeMessage(0x12E99200, false, true));
    bus1.send(makeMessage(0x12E99200, true, false));  // filtered out
    bus1.send(makeMessage(0x12E99200, true, true));

    // 06B70270:1FFFFFFF, SET, DNC
    bus1.send(makeMessage(0x06B70270, false, false));  // filtered out
    bus1.send(makeMessage(0x06B70270, false, true));   // filtered out
    bus1.send(makeMessage(0x06B70270, true, false));
    bus1.send(makeMessage(0x06B70270, true, true));

    // 096CFD2B:1FFFFFFF, SET, SET
    bus1.send(makeMessage(0x096CFD2B, false, false));  // filtered out
    bus1.send(makeMessage(0x096CFD2B, false, true));   // filtered out
    bus1.send(makeMessage(0x096CFD2B, true, false));   // filtered out
    bus1.send(makeMessage(0x096CFD2B, true, true));

    // 1BDCB008:0F0F0F0F, NS, DNC
    bus1.send(makeMessage(0x1B2C3048, false, false));
    bus1.send(makeMessage(0x0B5C6078, false, true));
    bus1.send(makeMessage(0x1B8C90A8, true, false));  // filtered out
    bus1.send(makeMessage(0x0BBCC0D8, true, true));   // filtered out

    // 08318B46:10F0F0F0, NS, SET
    bus1.send(makeMessage(0x0F3E8D4C, false, false));  // filtered out
    bus1.send(makeMessage(0x0B3A8948, false, true));
    bus1.send(makeMessage(0x07368544, true, false));  // filtered out
    bus1.send(makeMessage(0x03328140, true, true));   // filtered out

    // 06B:70F, DNC, SET
    bus1.send(makeMessage(0x00B, false, false));  // filtered out
    bus1.send(makeMessage(0x04B, false, true));
    bus1.send(makeMessage(0x08B, true, false));  // filtered out
    bus1.send(makeMessage(0x0FB, true, true));

    // 750:70F, SET, SET
    bus1.send(makeMessage(0x7F0, false, false));  // filtered out
    bus1.send(makeMessage(0x780, false, true));   // filtered out
    bus1.send(makeMessage(0x740, true, false));   // filtered out
    bus1.send(makeMessage(0x700, true, true));

    // 5CF:70F, NS, SET
    bus1.send(makeMessage(0x51F, false, false));  // filtered out
    bus1.send(makeMessage(0x53F, false, true));
    bus1.send(makeMessage(0x57F, true, false));  // filtered out
    bus1.send(makeMessage(0x5FF, true, true));   // filtered out

    std::vector<can::V1_0::CanMessage> expectedPositive{
            makeMessage(0x3F4, false, false),       // 334:73F, DNC, DNC
            makeMessage(0x334, false, true),        // 334:73F, DNC, DNC
            makeMessage(0x374, true, false),        // 334:73F, DNC, DNC
            makeMessage(0x3F4, true, true),         // 334:73F, DNC, DNC
            makeMessage(0x404, false, false),       // 49D:700, DNC, DNC
            makeMessage(0x4A5, false, true),        // 49D:700, DNC, DNC
            makeMessage(0x4FF, true, false),        // 49D:700, DNC, DNC
            makeMessage(0x46B, true, true),         // 49D:700, DNC, DNC
            makeMessage(0x324, false, false),       // 325:7FC, DNC, NS
            makeMessage(0x326, true, false),        // 325:7FC, DNC, NS
            makeMessage(0x246, true, false),        // 246:7FF, SET, DNC
            makeMessage(0x246, true, true),         // 246:7FF, SET, DNC
            makeMessage(0x1A2, true, false),        // 1A2:7FB, SET, NS
            makeMessage(0x607, false, false),       // 607:7C9, NS, DNC
            makeMessage(0x613, false, true),        // 607:7C9, NS, DNC
            makeMessage(0x774, false, false),       // 7F4:777, NS, NS
            makeMessage(0x11F293A4, false, false),  // 1BF19EAF:10F0F0F0, DNC, DNC
            makeMessage(0x15F697A8, false, true),   // 1BF19EAF:10F0F0F0, DNC, DNC
            makeMessage(0x19FA9BAC, true, false),   // 1BF19EAF:10F0F0F0, DNC, DNC
            makeMessage(0x1DFE9FA0, true, true),    // 1BF19EAF:10F0F0F0, DNC, DNC
            makeMessage(0x12E99200, false, true),   // 12E99200:1FFFFFFF, DNC, SET
            makeMessage(0x12E99200, true, true),    // 12E99200:1FFFFFFF, DNC, SET
            makeMessage(0x06B70270, true, false),   // 06B70270:1FFFFFFF, SET, DNC
            makeMessage(0x06B70270, true, true),    // 06B70270:1FFFFFFF, SET, DNC
            makeMessage(0x096CFD2B, true, true),    // 096CFD2B:1FFFFFFF, SET, SET
            makeMessage(0x1B2C3048, false, false),  // 1BDCB008:0F0F0F0F, NS, DNC
            makeMessage(0x0B5C6078, false, true),   // 1BDCB008:0F0F0F0F, NS, DNC
            makeMessage(0x0B3A8948, false, true),   // 08318B46:10F0F0F0, NS, SET
            makeMessage(0x04B, false, true),        // 06B:70F, DNC, SET
            makeMessage(0x0FB, true, true),         // 06B:70F, DNC, SET
            makeMessage(0x700, true, true),         // 750:70F, SET, SET
            makeMessage(0x53F, false, true),        // 5CF:70F, NS, SET
    };

    auto messagesPositive = listenerPositive->fetchMessages(100ms, expectedPositive.size());
    clearTimestamps(messagesPositive);
    ASSERT_EQ(expectedPositive, messagesPositive);
}

TEST_F(CanBusVirtualHalTest, FilterNegative) {
    if (mBusNames.size() < 2u) GTEST_SKIP() << "Not testable with less than two CAN buses.";
    auto bus1 = makeBus();
    auto bus2 = makeBus();

    /* clang-format off */
    /*        id,             mask,           rtr,                   eff          exclude */
    hidl_vec<CanMessageFilter> filterNegative = {
            {0x063,           0x7F3, FilterFlag::DONT_CARE, FilterFlag::DONT_CARE, true},
            {0x0A1,           0x78F, FilterFlag::DONT_CARE, FilterFlag::DONT_CARE, true},
            {0x18B,           0x7E3, FilterFlag::DONT_CARE, FilterFlag::NOT_SET,   true},
            {0x1EE,           0x7EC, FilterFlag::SET,       FilterFlag::DONT_CARE, true},
            {0x23F,           0x7A5, FilterFlag::SET,       FilterFlag::NOT_SET,   true},
            {0x31F,           0x77F, FilterFlag::NOT_SET,   FilterFlag::DONT_CARE, true},
            {0x341,           0x77F, FilterFlag::NOT_SET,   FilterFlag::NOT_SET,   true},
            {0x196573DB, 0x1FFFFF7F, FilterFlag::DONT_CARE, FilterFlag::DONT_CARE, true},
            {0x1CFCB417, 0x1FFFFFEC, FilterFlag::DONT_CARE, FilterFlag::SET,       true},
            {0x17CCC433, 0x1FFFFFEC, FilterFlag::SET,       FilterFlag::DONT_CARE, true},
            {0x0BC2F508, 0x1FFFFFC3, FilterFlag::SET,       FilterFlag::SET,       true},
            {0x1179B5D2, 0x1FFFFFC3, FilterFlag::NOT_SET,   FilterFlag::DONT_CARE, true},
            {0x082AF63D, 0x1FFFFFFF, FilterFlag::NOT_SET,   FilterFlag::SET,       true},
            {0x66D,           0x76F, FilterFlag::DONT_CARE, FilterFlag::SET,       true},
            {0x748,           0x7CC, FilterFlag::SET,       FilterFlag::SET,       true},
            {0x784,           0x7CC, FilterFlag::NOT_SET,   FilterFlag::SET,       true},
    };
    /* clang-format on */

    auto listenerNegative = bus2.listen(filterNegative);

    // 063:7F3, DNC, DNC: ~06[3,7,B,F]
    bus1.send(makeMessage(0x063, false, false));  // filtered out
    bus1.send(makeMessage(0x060, false, true));
    bus1.send(makeMessage(0x05B, true, false));
    bus1.send(makeMessage(0x06F, true, true));  // filtered out

    // 0A1:78F, DNC, DNC: ~0[8-F]1
    bus1.send(makeMessage(0x081, false, false));  // filtered out
    bus1.send(makeMessage(0x031, false, true));
    bus1.send(makeMessage(0x061, true, false));
    bus1.send(makeMessage(0x071, true, true));

    // 18B:7E3, DNC, NS: ~1[8-9][7,B,F]
    bus1.send(makeMessage(0x18B, false, false));  // filtered out
    bus1.send(makeMessage(0x188, false, true));
    bus1.send(makeMessage(0x123, true, false));
    bus1.send(makeMessage(0x1D5, true, true));

    // 1EE:7EC, SET, DNC: ~1[E-F][C-F]
    bus1.send(makeMessage(0x17E, false, false));
    bus1.send(makeMessage(0x138, false, true));
    bus1.send(makeMessage(0x123, true, false));
    bus1.send(makeMessage(0x1EC, true, true));  // filtered out

    // 23F:7A5, SET, NS: ~2[2,3,6,7][5,7,D,F]
    bus1.send(makeMessage(0x222, false, false));
    bus1.send(makeMessage(0x275, false, true));
    bus1.send(makeMessage(0x23f, true, false));  // filtered out
    bus1.send(makeMessage(0x241, true, false));
    bus1.send(makeMessage(0x2FF, true, true));

    // 31F:77F, NS, DNC: ~3[1,9]F
    bus1.send(makeMessage(0x32F, false, false));
    bus1.send(makeMessage(0x31F, false, true));  // filtered out
    bus1.send(makeMessage(0x36F, false, true));
    bus1.send(makeMessage(0x31F, true, false));
    bus1.send(makeMessage(0x3F3, true, true));

    // 341:77F, NS, NS: ~3[4,C]1
    bus1.send(makeMessage(0x341, false, false));  // filtered out
    bus1.send(makeMessage(0x352, false, false));
    bus1.send(makeMessage(0x3AA, false, true));
    bus1.send(makeMessage(0x3BC, true, false));
    bus1.send(makeMessage(0x3FF, true, true));

    // 196573DB:1FFFFF7F, DNC, DNC: ~196573[5,D]B
    bus1.send(makeMessage(0x1965733B, false, false));
    bus1.send(makeMessage(0x1965734B, false, true));
    bus1.send(makeMessage(0x1965735B, true, false));  // filtered out
    bus1.send(makeMessage(0x1965736B, true, true));

    // 1CFCB417:1FFFFFEC, DNC, SET: ~1CFCB4[0-1][4-7]
    bus1.send(makeMessage(0x1CFCB407, false, false));
    bus1.send(makeMessage(0x1CFCB4FF, false, true));
    bus1.send(makeMessage(0x1CFCB414, true, false));
    bus1.send(makeMessage(0x1CFCB407, true, true));  // filtered out

    // 17CCC433:1FFFFFEC, SET, DNC: ~17CCC4[2-3][0-3]
    bus1.send(makeMessage(0x17CCC430, false, false));
    bus1.send(makeMessage(0x17CCC423, false, true));
    bus1.send(makeMessage(0x17CCC420, true, false));  // filtered out
    bus1.send(makeMessage(0x17CCC444, true, true));

    // 0BC2F508:1FFFFFC3, SET, SET: ~5[0-3][0,4,8,C]
    bus1.send(makeMessage(0x0BC2F504, false, false));
    bus1.send(makeMessage(0x0BC2F518, false, true));
    bus1.send(makeMessage(0x0BC2F52C, true, false));
    bus1.send(makeMessage(0x0BC2F500, true, true));  // filtered out
    bus1.send(makeMessage(0x0BC2F543, true, true));

    // 1179B5D2:1FFFFFC3, NS, DNC: ~5[C-F][2,6,A,E]
    bus1.send(makeMessage(0x1179B5BB, false, false));
    bus1.send(makeMessage(0x1179B5EA, false, true));  // filtered out
    bus1.send(makeMessage(0x1179B5C2, true, false));
    bus1.send(makeMessage(0x1179B5DA, true, true));

    // 082AF63D:1FFFFF6F, NS, SET: ~6[2,3,A,B]D
    bus1.send(makeMessage(0x082AF62D, false, false));
    bus1.send(makeMessage(0x082AF63D, false, true));  // filtered out
    bus1.send(makeMessage(0x082AF60D, false, true));
    bus1.send(makeMessage(0x082AF6AD, true, false));
    bus1.send(makeMessage(0x082AF6BD, true, true));

    // 66D:76F, DNC, SET: ~6[6,7,E,F]D
    bus1.send(makeMessage(0x66D, false, false));
    bus1.send(makeMessage(0x68D, false, true));
    bus1.send(makeMessage(0x67D, true, false));
    bus1.send(makeMessage(0x6ED, true, true));  // filtered out

    // 748:7CC, SET, SET: ~0x7[4-7][8-F]
    bus1.send(makeMessage(0x749, false, false));
    bus1.send(makeMessage(0x75A, false, true));
    bus1.send(makeMessage(0x76B, true, false));
    bus1.send(makeMessage(0x748, true, true));  // filtered out
    bus1.send(makeMessage(0x788, true, true));

    // 784:7CC, NS, SET: ~0x7[8-F][4-7]
    bus1.send(makeMessage(0x795, false, false));
    bus1.send(makeMessage(0x784, false, true));  // filtered out
    bus1.send(makeMessage(0x71B, false, true));
    bus1.send(makeMessage(0x769, true, false));
    bus1.send(makeMessage(0x784, true, true));

    std::vector<can::V1_0::CanMessage> expectedNegative{
            makeMessage(0x060, false, true),        // 063:7F3, DNC, DNC
            makeMessage(0x05B, true, false),        // 063:7F3, DNC, DNC
            makeMessage(0x031, false, true),        // 0A1:78F, DNC, DNC
            makeMessage(0x061, true, false),        // 0A1:78F, DNC, DNC
            makeMessage(0x071, true, true),         // 0A1:78F, DNC, DNC
            makeMessage(0x188, false, true),        // 18B:7E3, DNC, NS
            makeMessage(0x123, true, false),        // 18B:7E3, DNC, NS
            makeMessage(0x1D5, true, true),         // 18B:7E3, DNC, NS
            makeMessage(0x17E, false, false),       // 1EE:7EC, SET, DNC
            makeMessage(0x138, false, true),        // 1EE:7EC, SET, DNC
            makeMessage(0x123, true, false),        // 1EE:7EC, SET, DNC
            makeMessage(0x222, false, false),       // 23F:7A5, SET, NS
            makeMessage(0x275, false, true),        // 23F:7A5, SET, NS
            makeMessage(0x241, true, false),        // 23F:7A5, SET, NS
            makeMessage(0x2FF, true, true),         // 23F:7A5, SET, NS
            makeMessage(0x32F, false, false),       // 31F:77F, NS, DNC
            makeMessage(0x36F, false, true),        // 31F:77F, NS, DNC
            makeMessage(0x31F, true, false),        // 31F:77F, NS, DNC
            makeMessage(0x3F3, true, true),         // 31F:77F, NS, DNC
            makeMessage(0x352, false, false),       // 341:77F, NS, NS
            makeMessage(0x3AA, false, true),        // 341:77F, NS, NS
            makeMessage(0x3BC, true, false),        // 341:77F, NS, NS
            makeMessage(0x3FF, true, true),         // 341:77F, NS, NS
            makeMessage(0x1965733B, false, false),  // 196573DB:1FFFFF7F, DNC, DNC
            makeMessage(0x1965734B, false, true),   // 196573DB:1FFFFF7F, DNC, DNC
            makeMessage(0x1965736B, true, true),    // 196573DB:1FFFFF7F, DNC, DNC
            makeMessage(0x1CFCB407, false, false),  // 1CFCB417:1FFFFFEC, DNC, SET
            makeMessage(0x1CFCB4FF, false, true),   // 1CFCB417:1FFFFFEC, DNC, SET
            makeMessage(0x1CFCB414, true, false),   // 1CFCB417:1FFFFFEC, DNC, SET
            makeMessage(0x17CCC430, false, false),  // 17CCC433:1FFFFFEC, SET, DNC
            makeMessage(0x17CCC423, false, true),   // 17CCC433:1FFFFFEC, SET, DNC
            makeMessage(0x17CCC444, true, true),    // 17CCC433:1FFFFFEC, SET, DNC
            makeMessage(0x0BC2F504, false, false),  // 0BC2F508:1FFFFFC3, SET, SET
            makeMessage(0x0BC2F518, false, true),   // 0BC2F508:1FFFFFC3, SET, SET
            makeMessage(0x0BC2F52C, true, false),   // 0BC2F508:1FFFFFC3, SET, SET
            makeMessage(0x0BC2F543, true, true),    // 0BC2F508:1FFFFFC3, SET, SET
            makeMessage(0x1179B5BB, false, false),  // 1179B5D2:1FFFFFC3, NS, DNC
            makeMessage(0x1179B5C2, true, false),   // 1179B5D2:1FFFFFC3, NS, DNC
            makeMessage(0x1179B5DA, true, true),    // 1179B5D2:1FFFFFC3, NS, DNC
            makeMessage(0x082AF62D, false, false),  // 082AF63D:1FFFFF6F, NS, SET
            makeMessage(0x082AF60D, false, true),   // 082AF63D:1FFFFF6F, NS, SET
            makeMessage(0x082AF6AD, true, false),   // 082AF63D:1FFFFF6F, NS, SET
            makeMessage(0x082AF6BD, true, true),    // 082AF63D:1FFFFF6F, NS, SET
            makeMessage(0x66D, false, false),       // 66D:76F, DNC, SET
            makeMessage(0x68D, false, true),        // 66D:76F, DNC, SET
            makeMessage(0x67D, true, false),        // 66D:76F, DNC, SET
            makeMessage(0x749, false, false),       // 748:7CC, SET, SET
            makeMessage(0x75A, false, true),        // 748:7CC, SET, SET
            makeMessage(0x76B, true, false),        // 748:7CC, SET, SET
            makeMessage(0x788, true, true),         // 748:7CC, SET, SET
            makeMessage(0x795, false, false),       // 784:7CC, NS, SET
            makeMessage(0x71B, false, true),        // 784:7CC, NS, SET
            makeMessage(0x769, true, false),        // 784:7CC, NS, SET
            makeMessage(0x784, true, true),         // 784:7CC, NS, SET
    };

    auto messagesNegative = listenerNegative->fetchMessages(100ms, expectedNegative.size());
    clearTimestamps(messagesNegative);
    ASSERT_EQ(expectedNegative, messagesNegative);
}

TEST_F(CanBusVirtualHalTest, FilterMixed) {
    if (mBusNames.size() < 2u) GTEST_SKIP() << "Not testable with less than two CAN buses.";
    auto bus1 = makeBus();
    auto bus2 = makeBus();

    /* clang-format off */
    /*        id,           mask,             rtr,                   eff          exclude */
    hidl_vec<CanMessageFilter> filterMixed = {
            {0x000,      0x700,      FilterFlag::DONT_CARE, FilterFlag::DONT_CARE, false},
            {0x0D5,      0x7FF,      FilterFlag::DONT_CARE, FilterFlag::DONT_CARE, true},
            {0x046,      0x7FF,      FilterFlag::DONT_CARE, FilterFlag::NOT_SET,   true},
            {0x11D89097, 0x1FFFFFFF, FilterFlag::DONT_CARE, FilterFlag::SET,       true},
            {0x0AB,      0x7FF,      FilterFlag::NOT_SET,   FilterFlag::DONT_CARE, true},
            {0x00D,      0x7FF,      FilterFlag::NOT_SET,   FilterFlag::NOT_SET,   true},
            {0x0F82400E, 0x1FFFFFFF, FilterFlag::NOT_SET,   FilterFlag::SET,       true},
            {0x08F,      0x7FF,      FilterFlag::SET,       FilterFlag::DONT_CARE, true},
            {0x0BE,      0x7FF,      FilterFlag::SET,       FilterFlag::NOT_SET,   true},
            {0x0A271011, 0x1FFFFFFF, FilterFlag::SET,       FilterFlag::SET,       true},
            {0x0BE,      0x7FF,      FilterFlag::DONT_CARE, FilterFlag::DONT_CARE, false},

            {0x100,      0x700,      FilterFlag::DONT_CARE, FilterFlag::NOT_SET,   false},
            {0x138,      0x7FF,      FilterFlag::DONT_CARE, FilterFlag::DONT_CARE, true},
            {0x1BF,      0x7FF,      FilterFlag::DONT_CARE, FilterFlag::NOT_SET,   true},
            {0x13AB6165, 0x1FFFFFFF, FilterFlag::DONT_CARE, FilterFlag::SET,       true},
            {0x17A,      0x7FF,      FilterFlag::NOT_SET,   FilterFlag::DONT_CARE, true},
            {0x13C,      0x7FF,      FilterFlag::NOT_SET,   FilterFlag::NOT_SET,   true},
            {0x102C5197, 0x1FFFFFFF, FilterFlag::NOT_SET,   FilterFlag::SET,       true},
            {0x19B,      0x7FF,      FilterFlag::SET,       FilterFlag::DONT_CARE, true},
            {0x1B8,      0x7FF,      FilterFlag::SET,       FilterFlag::NOT_SET,   true},
            {0x0D6D5185, 0x1FFFFFFF, FilterFlag::SET,       FilterFlag::SET,       true},
            {0x1B8,      0x7FF,      FilterFlag::DONT_CARE, FilterFlag::DONT_CARE, false},

            {0x096A2200, 0x1FFFFF00, FilterFlag::DONT_CARE, FilterFlag::SET,       false},
            {0x201,      0x7FF,      FilterFlag::DONT_CARE, FilterFlag::DONT_CARE, true},
            {0x22A,      0x7FF,      FilterFlag::DONT_CARE, FilterFlag::NOT_SET,   true},
            {0x1D1C3238, 0x1FFFFFFF, FilterFlag::DONT_CARE, FilterFlag::SET,       true},
            {0x2C0,      0x7FF,      FilterFlag::NOT_SET,   FilterFlag::DONT_CARE, true},
            {0x23C,      0x7FF,      FilterFlag::NOT_SET,   FilterFlag::NOT_SET,   true},
            {0x016182C6, 0x1FFFFFFF, FilterFlag::NOT_SET,   FilterFlag::SET,       true},
            {0x27B,      0x7FF,      FilterFlag::SET,       FilterFlag::DONT_CARE, true},
            {0x2A5,      0x7FF,      FilterFlag::SET,       FilterFlag::NOT_SET,   true},
            {0x160EB24B, 0x1FFFFFFF, FilterFlag::SET,       FilterFlag::SET,       true},
            {0x2A5,      0x7FF,      FilterFlag::DONT_CARE, FilterFlag::DONT_CARE, false},

            {0x300,      0x700,      FilterFlag::NOT_SET,   FilterFlag::DONT_CARE, false},
            {0x339,      0x7FF,      FilterFlag::DONT_CARE, FilterFlag::DONT_CARE, true},
            {0x3D4,      0x7FF,      FilterFlag::DONT_CARE, FilterFlag::NOT_SET,   true},
            {0x182263BE, 0x1FFFFFFF, FilterFlag::DONT_CARE, FilterFlag::SET,       true},
            {0x327,      0x7FF,      FilterFlag::NOT_SET,   FilterFlag::DONT_CARE, true},
            {0x36B,      0x7FF,      FilterFlag::NOT_SET,   FilterFlag::NOT_SET,   true},
            {0x1A1D8374, 0x1FFFFFFF, FilterFlag::NOT_SET,   FilterFlag::SET,       true},
            {0x319,      0x7FF,      FilterFlag::SET,       FilterFlag::DONT_CARE, true},
            {0x39E,      0x7FF,      FilterFlag::SET,       FilterFlag::NOT_SET,   true},
            {0x1B657332, 0x1FFFFFFF, FilterFlag::SET,       FilterFlag::SET,       true},
            {0x39E,      0x7FF,      FilterFlag::DONT_CARE, FilterFlag::DONT_CARE, false},

            {0x06C5D400, 0x1FFFFF00, FilterFlag::NOT_SET,   FilterFlag::SET,       false},
            {0x492,      0x7FF,      FilterFlag::DONT_CARE, FilterFlag::DONT_CARE, true},
            {0x4EE,      0x7FF,      FilterFlag::DONT_CARE, FilterFlag::NOT_SET,   true},
            {0x07725454, 0x1FFFFFFF, FilterFlag::DONT_CARE, FilterFlag::SET,       true},
            {0x4D5,      0x7FF,      FilterFlag::NOT_SET,   FilterFlag::DONT_CARE, true},
            {0x402,      0x7FF,      FilterFlag::NOT_SET,   FilterFlag::NOT_SET,   true},
            {0x139714A7, 0x1FFFFFFF, FilterFlag::NOT_SET,   FilterFlag::SET,       true},
            {0x464,      0x7FF,      FilterFlag::SET,       FilterFlag::DONT_CARE, true},
            {0x454,      0x7FF,      FilterFlag::SET,       FilterFlag::NOT_SET,   true},
            {0x0EF4B46F, 0x1FFFFFFF, FilterFlag::SET,       FilterFlag::SET,       true},
            {0x454,      0x7FF,      FilterFlag::DONT_CARE, FilterFlag::DONT_CARE, false},

            {0x500,      0x700,      FilterFlag::SET,       FilterFlag::DONT_CARE, false},
            {0x503,      0x7FF,      FilterFlag::DONT_CARE, FilterFlag::DONT_CARE, true},
            {0x566,      0x7FF,      FilterFlag::DONT_CARE, FilterFlag::NOT_SET,   true},
            {0x137605E7, 0x1FFFFFFF, FilterFlag::DONT_CARE, FilterFlag::SET,       true},
            {0x564,      0x7FF,      FilterFlag::NOT_SET,   FilterFlag::DONT_CARE, true},
            {0x58E,      0x7FF,      FilterFlag::NOT_SET,   FilterFlag::NOT_SET,   true},
            {0x05F9052D, 0x1FFFFFFF, FilterFlag::NOT_SET,   FilterFlag::SET,       true},
            {0x595,      0x7FF,      FilterFlag::SET,       FilterFlag::DONT_CARE, true},
            {0x563,      0x7FF,      FilterFlag::SET,       FilterFlag::NOT_SET,   true},
            {0x13358537, 0x1FFFFFFF, FilterFlag::SET,       FilterFlag::SET,       true},
            {0x563,      0x7FF,      FilterFlag::DONT_CARE, FilterFlag::DONT_CARE, false},

            {0x600,      0x700,      FilterFlag::SET,       FilterFlag::NOT_SET,   false},
            {0x64D,      0x7FF,      FilterFlag::DONT_CARE, FilterFlag::DONT_CARE, true},
            {0x620,      0x7FF,      FilterFlag::DONT_CARE, FilterFlag::NOT_SET,   true},
            {0x1069A676, 0x1FFFFFFF, FilterFlag::DONT_CARE, FilterFlag::SET,       true},
            {0x62D,      0x7FF,      FilterFlag::NOT_SET,   FilterFlag::DONT_CARE, true},
            {0x6C4,      0x7FF,      FilterFlag::NOT_SET,   FilterFlag::NOT_SET,   true},
            {0x14C76629, 0x1FFFFFFF, FilterFlag::NOT_SET,   FilterFlag::SET,       true},
            {0x689,      0x7FF,      FilterFlag::SET,       FilterFlag::DONT_CARE, true},
            {0x6A4,      0x7FF,      FilterFlag::SET,       FilterFlag::NOT_SET,   true},
            {0x0BCCA6C2, 0x1FFFFFFF, FilterFlag::SET,       FilterFlag::SET,       true},
            {0x6A4,      0x7FF,      FilterFlag::DONT_CARE, FilterFlag::DONT_CARE, false},

            {0x04BB1700, 0x1FFFFF00, FilterFlag::SET,       FilterFlag::SET,       false},
            {0x784,      0x7FF,      FilterFlag::DONT_CARE, FilterFlag::DONT_CARE, true},
            {0x7F9,      0x7FF,      FilterFlag::DONT_CARE, FilterFlag::NOT_SET,   true},
            {0x0200F77D, 0x1FFFFFFF, FilterFlag::DONT_CARE, FilterFlag::SET,       true},
            {0x783,      0x7FF,      FilterFlag::NOT_SET,   FilterFlag::DONT_CARE, true},
            {0x770,      0x7FF,      FilterFlag::NOT_SET,   FilterFlag::NOT_SET,   true},
            {0x06602719, 0x1FFFFFFF, FilterFlag::NOT_SET,   FilterFlag::SET,       true},
            {0x76B,      0x7FF,      FilterFlag::SET,       FilterFlag::DONT_CARE, true},
            {0x7DF,      0x7FF,      FilterFlag::SET,       FilterFlag::NOT_SET,   true},
            {0x1939E736, 0x1FFFFFFF, FilterFlag::SET,       FilterFlag::SET,       true},
            {0x7DF,      0x7FF,      FilterFlag::DONT_CARE, FilterFlag::DONT_CARE, false},
    };
    /* clang-format on */

    auto listenerMixed = bus2.listen(filterMixed);

    bus1.send(makeMessage(0x000, true, true));  // positive filter
    bus1.send(makeMessage(0x0D5, false, false));
    bus1.send(makeMessage(0x046, true, false));
    bus1.send(makeMessage(0x046, false, false));
    bus1.send(makeMessage(0x11D89097, true, true));
    bus1.send(makeMessage(0x11D89097, false, true));
    bus1.send(makeMessage(0x0AB, false, false));
    bus1.send(makeMessage(0x0AB, false, true));
    bus1.send(makeMessage(0x00D, false, false));
    bus1.send(makeMessage(0x0F82400E, false, true));
    bus1.send(makeMessage(0x08F, true, false));
    bus1.send(makeMessage(0x08F, true, true));
    bus1.send(makeMessage(0x0BE, true, false));
    bus1.send(makeMessage(0x0A271011, true, true));
    bus1.send(makeMessage(0x0BE, false, true));   // not filtered
    bus1.send(makeMessage(0x100, false, false));  // positive filter
    bus1.send(makeMessage(0x138, false, true));
    bus1.send(makeMessage(0x138, true, false));
    bus1.send(makeMessage(0x1BF, false, false));
    bus1.send(makeMessage(0x1BF, true, false));
    bus1.send(makeMessage(0x13AB6165, false, true));
    bus1.send(makeMessage(0x13AB6165, true, true));
    bus1.send(makeMessage(0x17A, false, false));
    bus1.send(makeMessage(0x17A, false, true));
    bus1.send(makeMessage(0x13C, false, false));
    bus1.send(makeMessage(0x102C5197, false, true));
    bus1.send(makeMessage(0x19B, true, false));
    bus1.send(makeMessage(0x19B, true, true));
    bus1.send(makeMessage(0x1B8, true, false));
    bus1.send(makeMessage(0x0D6D5185, true, true));
    bus1.send(makeMessage(0x1B8, false, true));       // not filtered
    bus1.send(makeMessage(0x096A2200, false, true));  // positive filter
    bus1.send(makeMessage(0x201, false, true));
    bus1.send(makeMessage(0x201, true, false));
    bus1.send(makeMessage(0x22A, false, false));
    bus1.send(makeMessage(0x22A, true, false));
    bus1.send(makeMessage(0x1D1C3238, false, true));
    bus1.send(makeMessage(0x1D1C3238, true, true));
    bus1.send(makeMessage(0x2C0, false, false));
    bus1.send(makeMessage(0x2C0, false, true));
    bus1.send(makeMessage(0x23C, false, false));
    bus1.send(makeMessage(0x016182C6, false, true));
    bus1.send(makeMessage(0x27B, true, false));
    bus1.send(makeMessage(0x27B, true, true));
    bus1.send(makeMessage(0x2A5, true, false));
    bus1.send(makeMessage(0x160EB24B, true, true));
    bus1.send(makeMessage(0x2A5, false, true));   // not filtereed
    bus1.send(makeMessage(0x300, false, false));  // positive filter
    bus1.send(makeMessage(0x339, false, true));
    bus1.send(makeMessage(0x339, false, false));
    bus1.send(makeMessage(0x3D4, true, false));
    bus1.send(makeMessage(0x182263BE, false, true));
    bus1.send(makeMessage(0x182263BE, true, true));
    bus1.send(makeMessage(0x327, false, false));
    bus1.send(makeMessage(0x327, false, true));
    bus1.send(makeMessage(0x36B, false, false));
    bus1.send(makeMessage(0x1A1D8374, false, true));
    bus1.send(makeMessage(0x319, true, false));
    bus1.send(makeMessage(0x319, true, true));
    bus1.send(makeMessage(0x39E, true, false));
    bus1.send(makeMessage(0x1B657332, true, true));
    bus1.send(makeMessage(0x39E, false, true));       // not filtered
    bus1.send(makeMessage(0x06C5D400, false, true));  // positive filter
    bus1.send(makeMessage(0x492, false, true));
    bus1.send(makeMessage(0x492, true, false));
    bus1.send(makeMessage(0x4EE, false, false));
    bus1.send(makeMessage(0x4EE, true, false));
    bus1.send(makeMessage(0x07725454, false, true));
    bus1.send(makeMessage(0x07725454, true, true));
    bus1.send(makeMessage(0x4D5, false, false));
    bus1.send(makeMessage(0x4D5, false, true));
    bus1.send(makeMessage(0x402, false, false));
    bus1.send(makeMessage(0x139714A7, false, true));
    bus1.send(makeMessage(0x464, true, false));
    bus1.send(makeMessage(0x464, true, true));
    bus1.send(makeMessage(0x454, true, false));
    bus1.send(makeMessage(0x0EF4B46F, true, true));
    bus1.send(makeMessage(0x454, false, true));  // not filtered
    bus1.send(makeMessage(0x500, true, false));  // positive filter
    bus1.send(makeMessage(0x503, false, true));
    bus1.send(makeMessage(0x503, true, false));
    bus1.send(makeMessage(0x566, false, false));
    bus1.send(makeMessage(0x566, true, false));
    bus1.send(makeMessage(0x137605E7, false, true));
    bus1.send(makeMessage(0x137605E7, true, true));
    bus1.send(makeMessage(0x564, false, false));
    bus1.send(makeMessage(0x564, false, true));
    bus1.send(makeMessage(0x58E, false, false));
    bus1.send(makeMessage(0x05F9052D, false, true));
    bus1.send(makeMessage(0x595, true, false));
    bus1.send(makeMessage(0x595, true, true));
    bus1.send(makeMessage(0x563, true, false));
    bus1.send(makeMessage(0x13358537, true, true));
    bus1.send(makeMessage(0x563, false, true));  // not filtered
    bus1.send(makeMessage(0x600, true, false));  // positive filter
    bus1.send(makeMessage(0x64D, false, true));
    bus1.send(makeMessage(0x64D, true, false));
    bus1.send(makeMessage(0x620, false, false));
    bus1.send(makeMessage(0x620, true, false));
    bus1.send(makeMessage(0x1069A676, false, true));
    bus1.send(makeMessage(0x1069A676, true, true));
    bus1.send(makeMessage(0x62D, false, false));
    bus1.send(makeMessage(0x62D, false, true));
    bus1.send(makeMessage(0x6C4, false, false));
    bus1.send(makeMessage(0x14C76629, false, true));
    bus1.send(makeMessage(0x689, true, false));
    bus1.send(makeMessage(0x689, true, true));
    bus1.send(makeMessage(0x6A4, true, false));
    bus1.send(makeMessage(0x0BCCA6C2, true, true));
    bus1.send(makeMessage(0x6A4, false, true));      // not filtered
    bus1.send(makeMessage(0x04BB1700, true, true));  // positive filter
    bus1.send(makeMessage(0x784, false, true));
    bus1.send(makeMessage(0x784, true, false));
    bus1.send(makeMessage(0x7F9, false, false));
    bus1.send(makeMessage(0x7F9, true, false));
    bus1.send(makeMessage(0x0200F77D, false, true));
    bus1.send(makeMessage(0x0200F77D, true, true));
    bus1.send(makeMessage(0x783, false, false));
    bus1.send(makeMessage(0x783, false, true));
    bus1.send(makeMessage(0x770, false, false));
    bus1.send(makeMessage(0x06602719, false, true));
    bus1.send(makeMessage(0x76B, true, false));
    bus1.send(makeMessage(0x76B, true, true));
    bus1.send(makeMessage(0x7DF, true, false));
    bus1.send(makeMessage(0x1939E736, true, true));
    bus1.send(makeMessage(0x7DF, false, true));  // not filtered

    std::vector<can::V1_0::CanMessage> expectedMixed{
            makeMessage(0x000, true, true),  // 0x000:0x700, DONT_CARE, DONT_CARE
            makeMessage(0x0BE, false, true),
            makeMessage(0x100, false, false),  // 0x100:0x700, DONT_CARE, NOT_SET
            makeMessage(0x1B8, false, true),
            makeMessage(0x096A2200, false, true),  // 0x096A2200:0x1FFFFF00, DONT_CARE, SET
            makeMessage(0x2A5, false, true),
            makeMessage(0x300, false, false),  // 0x300:0x700, NOT_SET, DONT_CARE
            makeMessage(0x39E, false, true),
            makeMessage(0x06C5D400, false, true),  // 0x06C5D400:0x1FFFFF00, NOT_SET, SET
            makeMessage(0x454, false, true),
            makeMessage(0x500, true, false),  // 0x500:0x700, SET, DONT_CARE
            makeMessage(0x563, false, true),
            makeMessage(0x600, true, false),  // 0x600:0x700, SET, NOT_SET
            makeMessage(0x6A4, false, true),
            makeMessage(0x04BB1700, true, true),  // 0x04BB1700:0x1FFFFF00, SET, SET
            makeMessage(0x7DF, false, true),
    };

    auto messagesMixed = listenerMixed->fetchMessages(100ms, expectedMixed.size());
    clearTimestamps(messagesMixed);
    ASSERT_EQ(expectedMixed, messagesMixed);
}

}  // namespace android::hardware::automotive::can::V1_0::vts

/**
 * Example manual invocation:
 * adb shell /data/nativetest64/VtsHalCanBusVirtualV1_0TargetTest/VtsHalCanBusVirtualV1_0TargetTest\
 *     --hal_service_instance=android.hardware.automotive.can@1.0::ICanController/socketcan
 */
int main(int argc, char** argv) {
    using android::hardware::automotive::can::V1_0::ICanController;
    using android::hardware::automotive::can::V1_0::vts::gEnv;
    using android::hardware::automotive::can::V1_0::vts::utils::SimpleHidlEnvironment;
    android::base::SetDefaultTag("CanBusVirtualVts");
    android::base::SetMinimumLogSeverity(android::base::VERBOSE);
    gEnv = new SimpleHidlEnvironment<ICanController>;
    ::testing::AddGlobalTestEnvironment(gEnv);
    ::testing::InitGoogleTest(&argc, argv);
    gEnv->init(&argc, argv);
    return RUN_ALL_TESTS();
}
