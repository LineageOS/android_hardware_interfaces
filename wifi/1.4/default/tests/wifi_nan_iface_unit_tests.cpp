/*
 * Copyright (C) 2019, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <android-base/logging.h>
#include <android-base/macros.h>
#include <cutils/properties.h>
#include <gmock/gmock.h>

#undef NAN  // This is weird, NAN is defined in bionic/libc/include/math.h:38
#include "wifi_nan_iface.h"

#include "mock_interface_tool.h"
#include "mock_wifi_feature_flags.h"
#include "mock_wifi_iface_util.h"
#include "mock_wifi_legacy_hal.h"

using testing::NiceMock;
using testing::Return;
using testing::Test;

namespace {
constexpr char kIfaceName[] = "mockWlan0";
}  // namespace

namespace android {
namespace hardware {
namespace wifi {
namespace V1_4 {
namespace implementation {

using android::hardware::wifi::V1_2::IWifiNanIfaceEventCallback;
using android::hardware::wifi::V1_2::NanDataPathConfirmInd;

bool CaptureIfaceEventHandlers(
    const std::string& /* iface_name*/,
    iface_util::IfaceEventHandlers in_iface_event_handlers,
    iface_util::IfaceEventHandlers* out_iface_event_handlers) {
    *out_iface_event_handlers = in_iface_event_handlers;
    return true;
}

class MockNanIfaceEventCallback : public IWifiNanIfaceEventCallback {
   public:
    MockNanIfaceEventCallback() = default;

    MOCK_METHOD3(notifyCapabilitiesResponse,
                 Return<void>(uint16_t, const WifiNanStatus&,
                              const NanCapabilities&));
    MOCK_METHOD2(notifyEnableResponse,
                 Return<void>(uint16_t, const WifiNanStatus&));
    MOCK_METHOD2(notifyConfigResponse,
                 Return<void>(uint16_t, const WifiNanStatus&));
    MOCK_METHOD2(notifyDisableResponse,
                 Return<void>(uint16_t, const WifiNanStatus&));
    MOCK_METHOD3(notifyStartPublishResponse,
                 Return<void>(uint16_t, const WifiNanStatus&, uint8_t));
    MOCK_METHOD2(notifyStopPublishResponse,
                 Return<void>(uint16_t, const WifiNanStatus&));
    MOCK_METHOD3(notifyStartSubscribeResponse,
                 Return<void>(uint16_t, const WifiNanStatus&, uint8_t));
    MOCK_METHOD2(notifyStopSubscribeResponse,
                 Return<void>(uint16_t, const WifiNanStatus&));
    MOCK_METHOD2(notifyTransmitFollowupResponse,
                 Return<void>(uint16_t, const WifiNanStatus&));
    MOCK_METHOD2(notifyCreateDataInterfaceResponse,
                 Return<void>(uint16_t, const WifiNanStatus&));
    MOCK_METHOD2(notifyDeleteDataInterfaceResponse,
                 Return<void>(uint16_t, const WifiNanStatus&));
    MOCK_METHOD3(notifyInitiateDataPathResponse,
                 Return<void>(uint16_t, const WifiNanStatus&, uint32_t));
    MOCK_METHOD2(notifyRespondToDataPathIndicationResponse,
                 Return<void>(uint16_t, const WifiNanStatus&));
    MOCK_METHOD2(notifyTerminateDataPathResponse,
                 Return<void>(uint16_t, const WifiNanStatus&));
    MOCK_METHOD1(eventClusterEvent, Return<void>(const NanClusterEventInd&));
    MOCK_METHOD1(eventDisabled, Return<void>(const WifiNanStatus&));
    MOCK_METHOD2(eventPublishTerminated,
                 Return<void>(uint8_t, const WifiNanStatus&));
    MOCK_METHOD2(eventSubscribeTerminated,
                 Return<void>(uint8_t, const WifiNanStatus&));
    MOCK_METHOD1(eventMatch, Return<void>(const NanMatchInd&));
    MOCK_METHOD2(eventMatchExpired, Return<void>(uint8_t, uint32_t));
    MOCK_METHOD1(eventFollowupReceived,
                 Return<void>(const NanFollowupReceivedInd&));
    MOCK_METHOD2(eventTransmitFollowup,
                 Return<void>(uint16_t, const WifiNanStatus&));
    MOCK_METHOD1(eventDataPathRequest,
                 Return<void>(const NanDataPathRequestInd&));
    MOCK_METHOD1(
        eventDataPathConfirm,
        Return<void>(
            const android::hardware::wifi::V1_0::NanDataPathConfirmInd&));
    MOCK_METHOD1(eventDataPathTerminated, Return<void>(uint32_t));
    MOCK_METHOD1(eventDataPathConfirm_1_2,
                 Return<void>(const NanDataPathConfirmInd&));
    MOCK_METHOD1(eventDataPathScheduleUpdate,
                 Return<void>(const NanDataPathScheduleUpdateInd&));
};

class WifiNanIfaceTest : public Test {
   protected:
    std::shared_ptr<NiceMock<wifi_system::MockInterfaceTool>> iface_tool_{
        new NiceMock<wifi_system::MockInterfaceTool>};
    std::shared_ptr<NiceMock<legacy_hal::MockWifiLegacyHal>> legacy_hal_{
        new NiceMock<legacy_hal::MockWifiLegacyHal>(iface_tool_)};
    std::shared_ptr<NiceMock<iface_util::MockWifiIfaceUtil>> iface_util_{
        new NiceMock<iface_util::MockWifiIfaceUtil>(iface_tool_)};
};

TEST_F(WifiNanIfaceTest, IfacEventHandlers_OnStateToggleOffOn) {
    iface_util::IfaceEventHandlers captured_iface_event_handlers = {};
    EXPECT_CALL(*legacy_hal_,
                nanRegisterCallbackHandlers(testing::_, testing::_))
        .WillOnce(testing::Return(legacy_hal::WIFI_SUCCESS));
    EXPECT_CALL(*iface_util_,
                registerIfaceEventHandlers(testing::_, testing::_))
        .WillOnce(testing::Invoke(
            bind(CaptureIfaceEventHandlers, std::placeholders::_1,
                 std::placeholders::_2, &captured_iface_event_handlers)));
    sp<WifiNanIface> nan_iface =
        new WifiNanIface(kIfaceName, false, legacy_hal_, iface_util_);

    // Register a mock nan event callback.
    sp<NiceMock<MockNanIfaceEventCallback>> mock_event_callback{
        new NiceMock<MockNanIfaceEventCallback>};
    nan_iface->registerEventCallback(
        mock_event_callback, [](const WifiStatus& status) {
            ASSERT_EQ(WifiStatusCode::SUCCESS, status.code);
        });
    // Ensure that the eventDisabled() function in mock callback will be
    // invoked.
    WifiNanStatus expected_nan_status = {
        NanStatusType::UNSUPPORTED_CONCURRENCY_NAN_DISABLED, ""};
    EXPECT_CALL(*mock_event_callback, eventDisabled(expected_nan_status))
        .Times(1);

    // Trigger the iface state toggle callback.
    captured_iface_event_handlers.on_state_toggle_off_on(kIfaceName);
}
}  // namespace implementation
}  // namespace V1_4
}  // namespace wifi
}  // namespace hardware
}  // namespace android
