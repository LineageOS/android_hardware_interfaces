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

    Bus(sp<ICanController> controller, const ICanController::BusConfiguration& config)
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

static CanMessage makeMessage(CanMessageId id) {
    CanMessage msg = {};
    msg.id = id;
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

    ICanController::BusConfiguration config = {};
    config.name = mBusNames[idx];
    config.iftype = InterfaceType::VIRTUAL;
    config.interfaceId.address("vcan50");

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

TEST_F(CanBusVirtualHalTest, Filter) {
    if (mBusNames.size() < 2u) GTEST_SKIP() << "Not testable with less than two CAN buses.";
    auto bus1 = makeBus();
    auto bus2 = makeBus();

    hidl_vec<CanMessageFilter> filterPositive = {
            {0x101, 0x100, false, FilterFlag::DONT_CARE, FilterFlag::DONT_CARE},
            {0x010, 0x0F0, false, FilterFlag::DONT_CARE, FilterFlag::DONT_CARE},
    };
    auto listenerPositive = bus2.listen(filterPositive);

    hidl_vec<CanMessageFilter> filterNegative = {
            {0x123, 0x0FF, true, FilterFlag::DONT_CARE, FilterFlag::DONT_CARE},
            {0x004, 0x00F, true, FilterFlag::DONT_CARE, FilterFlag::DONT_CARE},
    };
    auto listenerNegative = bus2.listen(filterNegative);

    bus1.send(makeMessage(0));
    bus1.send(makeMessage(0x1A0));
    bus1.send(makeMessage(0x1A1));
    bus1.send(makeMessage(0x2A0));
    bus1.send(makeMessage(0x3A0));
    bus1.send(makeMessage(0x010));
    bus1.send(makeMessage(0x123));
    bus1.send(makeMessage(0x023));
    bus1.send(makeMessage(0x124));

    std::vector<can::V1_0::CanMessage> expectedPositive{
            makeMessage(0x1A0),  //
            makeMessage(0x1A1),  //
            makeMessage(0x3A0),  //
            makeMessage(0x010),  //
            makeMessage(0x123),  //
            makeMessage(0x124),  //
    };
    std::vector<can::V1_0::CanMessage> expectedNegative{
            makeMessage(0),      //
            makeMessage(0x1A0),  //
            makeMessage(0x1A1),  //
            makeMessage(0x2A0),  //
            makeMessage(0x3A0),  //
            makeMessage(0x010),  //
    };

    auto messagesNegative = listenerNegative->fetchMessages(100ms, expectedNegative.size());
    auto messagesPositive = listenerPositive->fetchMessages(100ms, expectedPositive.size());
    clearTimestamps(messagesNegative);
    clearTimestamps(messagesPositive);
    ASSERT_EQ(expectedNegative, messagesNegative);
    ASSERT_EQ(expectedPositive, messagesPositive);
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
