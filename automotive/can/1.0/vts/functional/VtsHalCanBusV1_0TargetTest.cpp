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

#include <android-base/logging.h>
#include <android-base/strings.h>
#include <android/hardware/automotive/can/1.0/ICanBus.h>
#include <android/hardware/automotive/can/1.0/types.h>
#include <can-vts-utils/bus-enumerator.h>
#include <can-vts-utils/can-hal-printers.h>
#include <gmock/gmock.h>
#include <hidl-utils/hidl-utils.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>

namespace android::hardware::automotive::can::V1_0::vts {

using hardware::hidl_vec;
using InterfaceType = ICanController::InterfaceType;
using IfId = ICanController::BusConfig::InterfaceId;

struct CanMessageListener : public can::V1_0::ICanMessageListener {
    virtual Return<void> onReceive(const can::V1_0::CanMessage&) override { return {}; }
};

struct CanErrorListener : public can::V1_0::ICanErrorListener {
    virtual Return<void> onError(ErrorEvent, bool) override { return {}; }
};

class CanBusHalTest : public ::testing::TestWithParam<std::string> {
  protected:
    virtual void SetUp() override;
    virtual void TearDown() override;

    bool up(InterfaceType iftype, const std::string& srvname, const std::string& ifname);

    std::tuple<Result, sp<ICloseHandle>> listen(const hidl_vec<CanMessageFilter>& filter,
                                                const sp<ICanMessageListener>& listener);
    sp<ICloseHandle> listenForErrors(const sp<ICanErrorListener>& listener);

    sp<ICanBus> mCanBus;
    sp<ICanController> mCanController;
};

void CanBusHalTest::SetUp() {
    mCanBus = ICanBus::getService(GetParam());
    ASSERT_TRUE(mCanBus) << "Couldn't open CAN Bus: " << GetParam();
    const auto controllers = getAllHalInstanceNames(ICanController::descriptor);
    ASSERT_GT(controllers.size(), 0u);
    // just grab the first one
    mCanController = ICanController::getService(controllers[0]);
    ASSERT_TRUE(mCanController) << "Couldn't open CAN Controller: " << controllers[0];

    // this will throw an error if the bus is already up, but we have to try.
    up(InterfaceType::VIRTUAL, GetParam(), "vcan0");
}

void CanBusHalTest::TearDown() {
    mCanBus.clear();
}

bool CanBusHalTest::up(InterfaceType iftype, const std::string& srvname,
                       const std::string& ifname) {
    ICanController::BusConfig config = {};
    config.name = srvname;

    // TODO(b/146214370): move interfaceId constructors to a library
    if (iftype == InterfaceType::SOCKETCAN) {
        IfId::Socketcan socketcan = {};
        socketcan.ifname(ifname);
        config.interfaceId.socketcan(socketcan);
    } else if (iftype == InterfaceType::SLCAN) {
        IfId::Slcan slcan = {};
        slcan.ttyname(ifname);
        config.interfaceId.slcan(slcan);
    } else if (iftype == InterfaceType::VIRTUAL) {
        config.interfaceId.virtualif({ifname});
    } else {
        ADD_FAILURE() << "Unexpected iftype: " << toString(iftype);
    }

    const auto upresult = mCanController->upInterface(config);
    if (upresult != ICanController::Result::OK) {
        // upInterface returns INVALID_STATE if the interface is already up (which is fine).
        EXPECT_EQ(ICanController::Result::INVALID_STATE, upresult)
                << ifname << " can't be brought up!";
    }
    return true;
}

std::tuple<Result, sp<ICloseHandle>> CanBusHalTest::listen(
        const hidl_vec<CanMessageFilter>& filter, const sp<ICanMessageListener>& listener) {
    Result halResult;
    sp<ICloseHandle> closeHandle;
    mCanBus->listen(filter, listener, hidl_utils::fill(&halResult, &closeHandle)).assertOk();

    return {halResult, closeHandle};
}

sp<ICloseHandle> CanBusHalTest::listenForErrors(const sp<ICanErrorListener>& listener) {
    const auto res = mCanBus->listenForErrors(listener);
    res.assertOk();
    return res;
}

TEST_P(CanBusHalTest, SendNoPayload) {
    CanMessage msg = {};
    msg.id = 0x123;
    ASSERT_NE(mCanBus, nullptr);
    const auto result = mCanBus->send(msg);
    ASSERT_EQ(Result::OK, result);
}

TEST_P(CanBusHalTest, Send8B) {
    CanMessage msg = {};
    msg.id = 0x234;
    msg.payload = {1, 2, 3, 4, 5, 6, 7, 8};

    const auto result = mCanBus->send(msg);
    ASSERT_EQ(Result::OK, result);
}

TEST_P(CanBusHalTest, SendZeroId) {
    CanMessage msg = {};
    msg.payload = {1, 2, 3};

    const auto result = mCanBus->send(msg);
    ASSERT_EQ(Result::OK, result);
}

TEST_P(CanBusHalTest, SendTooLong) {
    CanMessage msg = {};
    msg.id = 0x123;
    msg.payload = hidl_vec<uint8_t>(102400);  // 100kiB

    const auto result = mCanBus->send(msg);
    ASSERT_EQ(Result::PAYLOAD_TOO_LONG, result);
}

TEST_P(CanBusHalTest, ListenNoFilter) {
    const auto [result, closeHandle] = listen({}, new CanMessageListener());
    ASSERT_EQ(Result::OK, result);

    closeHandle->close().assertOk();
}

TEST_P(CanBusHalTest, ListenSomeFilter) {
    hidl_vec<CanMessageFilter> filters = {
            {0x123, 0x1FF, FilterFlag::DONT_CARE, FilterFlag::DONT_CARE, false},
            {0x001, 0x00F, FilterFlag::DONT_CARE, FilterFlag::DONT_CARE, true},
            {0x200, 0x100, FilterFlag::DONT_CARE, FilterFlag::DONT_CARE, false},
    };

    const auto [result, closeHandle] = listen(filters, new CanMessageListener());
    ASSERT_EQ(Result::OK, result);

    closeHandle->close().assertOk();
}

TEST_P(CanBusHalTest, ListenNull) {
    const auto [result, closeHandle] = listen({}, nullptr);
    ASSERT_EQ(Result::INVALID_ARGUMENTS, result);
}

TEST_P(CanBusHalTest, DoubleCloseListener) {
    const auto [result, closeHandle] = listen({}, new CanMessageListener());
    ASSERT_EQ(Result::OK, result);

    closeHandle->close().assertOk();
    closeHandle->close().assertOk();
}

TEST_P(CanBusHalTest, DontCloseListener) {
    const auto [result, closeHandle] = listen({}, new CanMessageListener());
    ASSERT_EQ(Result::OK, result);
}

TEST_P(CanBusHalTest, DoubleCloseErrorListener) {
    auto closeHandle = listenForErrors(new CanErrorListener());
    ASSERT_NE(nullptr, closeHandle.get());

    closeHandle->close().assertOk();
    closeHandle->close().assertOk();
}

TEST_P(CanBusHalTest, DoubleCloseNullErrorListener) {
    auto closeHandle = listenForErrors(nullptr);
    ASSERT_NE(nullptr, closeHandle.get());

    closeHandle->close().assertOk();
    closeHandle->close().assertOk();
}

TEST_P(CanBusHalTest, DontCloseErrorListener) {
    auto closeHandle = listenForErrors(new CanErrorListener());
    ASSERT_NE(nullptr, closeHandle.get());
}

/**
 * This test requires that you bring up a valid bus first.
 *
 * Before running:
 * mma -j && adb root && adb remount && adb sync
 *
 * Example manual invocation:
 * adb shell canhalctrl up <NAME_OF_VALID_BUS> socketcan can0 125000
 * adb shell /data/nativetest64/VtsHalCanBusV1_0TargetTest/VtsHalCanBusV1_0TargetTest\
 *     --gtest_filter=*_<NAME_OF_VALID_BUS>
 */
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(CanBusHalTest);
INSTANTIATE_TEST_SUITE_P(PerInstance, CanBusHalTest,
                         testing::ValuesIn(getAllHalInstanceNames(ICanBus::descriptor)),
                         PrintInstanceNameToString);

}  // namespace android::hardware::automotive::can::V1_0::vts
