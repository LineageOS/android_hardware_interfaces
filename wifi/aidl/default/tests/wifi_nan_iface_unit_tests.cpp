/*
 * Copyright (C) 2022 The Android Open Source Project
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

namespace aidl {
namespace android {
namespace hardware {
namespace wifi {

bool CaptureIfaceEventHandlers(const std::string& /* iface_name*/,
                               iface_util::IfaceEventHandlers in_iface_event_handlers,
                               iface_util::IfaceEventHandlers* out_iface_event_handlers) {
    *out_iface_event_handlers = in_iface_event_handlers;
    return true;
}

class MockNanIface : public WifiNanIface {
  public:
    MockNanIface(const std::string& ifname, bool is_dedicated_iface,
                 const std::weak_ptr<legacy_hal::WifiLegacyHal> legacy_hal,
                 const std::weak_ptr<iface_util::WifiIfaceUtil> iface_util)
        : WifiNanIface(ifname, is_dedicated_iface, legacy_hal, iface_util) {}

    static std::shared_ptr<MockNanIface> createMock(
            const std::string& ifname, bool is_dedicated_iface,
            const std::weak_ptr<legacy_hal::WifiLegacyHal> legacy_hal,
            const std::weak_ptr<iface_util::WifiIfaceUtil> iface_util) {
        std::shared_ptr<MockNanIface> ptr = ndk::SharedRefBase::make<MockNanIface>(
                ifname, is_dedicated_iface, legacy_hal, iface_util);
        std::weak_ptr<MockNanIface> weak_ptr_this(ptr);
        ptr->setWeakPtr(weak_ptr_this);
        ptr->registerCallbackHandlers();
        return ptr;
    }

    // Override getEventCallbacks() so that we can return a mocked callback object.
    std::set<std::shared_ptr<IWifiNanIfaceEventCallback>> getEventCallbacks() override {
        return {callback_};
    }

    void setMockCallback(std::shared_ptr<IWifiNanIfaceEventCallback> cb) { callback_ = cb; }

  private:
    std::shared_ptr<IWifiNanIfaceEventCallback> callback_;
};

class MockNanIfaceEventCallback : public IWifiNanIfaceEventCallback {
  public:
    ndk::SpAIBinder asBinder() override { return ::ndk::SpAIBinder{}; }
    bool isRemote() override { return false; }

    ::ndk::ScopedAStatus getInterfaceVersion(int32_t* _aidl_return) override {
        *_aidl_return = 1;
        return ndk::ScopedAStatus::ok();
    }
    ::ndk::ScopedAStatus getInterfaceHash(std::string* _aidl_return) override {
        *_aidl_return = "some_hash";
        return ndk::ScopedAStatus::ok();
    }

    MOCK_METHOD3(notifyCapabilitiesResponse,
                 ndk::ScopedAStatus(char16_t, const NanStatus&, const NanCapabilities&));
    MOCK_METHOD2(notifyEnableResponse, ndk::ScopedAStatus(char16_t, const NanStatus&));
    MOCK_METHOD2(notifyConfigResponse, ndk::ScopedAStatus(char16_t, const NanStatus&));
    MOCK_METHOD2(notifyDisableResponse, ndk::ScopedAStatus(char16_t, const NanStatus&));
    MOCK_METHOD3(notifyStartPublishResponse,
                 ndk::ScopedAStatus(char16_t, const NanStatus&, int8_t));
    MOCK_METHOD2(notifyStopPublishResponse, ndk::ScopedAStatus(char16_t, const NanStatus&));
    MOCK_METHOD3(notifyStartSubscribeResponse,
                 ndk::ScopedAStatus(char16_t, const NanStatus&, int8_t));
    MOCK_METHOD2(notifyStopSubscribeResponse, ndk::ScopedAStatus(char16_t, const NanStatus&));
    MOCK_METHOD2(notifyTransmitFollowupResponse, ndk::ScopedAStatus(char16_t, const NanStatus&));
    MOCK_METHOD2(notifyCreateDataInterfaceResponse, ndk::ScopedAStatus(char16_t, const NanStatus&));
    MOCK_METHOD2(notifyDeleteDataInterfaceResponse, ndk::ScopedAStatus(char16_t, const NanStatus&));
    MOCK_METHOD3(notifyInitiateDataPathResponse,
                 ndk::ScopedAStatus(char16_t, const NanStatus&, int32_t));
    MOCK_METHOD2(notifyRespondToDataPathIndicationResponse,
                 ndk::ScopedAStatus(char16_t, const NanStatus&));
    MOCK_METHOD2(notifyTerminateDataPathResponse, ndk::ScopedAStatus(char16_t, const NanStatus&));
    MOCK_METHOD1(eventClusterEvent, ndk::ScopedAStatus(const NanClusterEventInd&));
    MOCK_METHOD1(eventDisabled, ndk::ScopedAStatus(const NanStatus&));
    MOCK_METHOD2(eventPublishTerminated, ndk::ScopedAStatus(int8_t, const NanStatus&));
    MOCK_METHOD2(eventSubscribeTerminated, ndk::ScopedAStatus(int8_t, const NanStatus&));
    MOCK_METHOD1(eventMatch, ndk::ScopedAStatus(const NanMatchInd&));
    MOCK_METHOD2(eventMatchExpired, ndk::ScopedAStatus(int8_t, int32_t));
    MOCK_METHOD1(eventFollowupReceived, ndk::ScopedAStatus(const NanFollowupReceivedInd&));
    MOCK_METHOD2(eventTransmitFollowup, ndk::ScopedAStatus(char16_t, const NanStatus&));
    MOCK_METHOD1(eventDataPathRequest, ndk::ScopedAStatus(const NanDataPathRequestInd&));
    MOCK_METHOD1(eventDataPathConfirm, ndk::ScopedAStatus(const NanDataPathConfirmInd&));
    MOCK_METHOD1(eventDataPathTerminated, ndk::ScopedAStatus(int32_t));
    MOCK_METHOD1(eventDataPathScheduleUpdate,
                 ndk::ScopedAStatus(const NanDataPathScheduleUpdateInd&));
    MOCK_METHOD1(eventPairingConfirm, ndk::ScopedAStatus(const NanPairingConfirmInd&));
    MOCK_METHOD1(eventPairingRequest, ndk::ScopedAStatus(const NanPairingRequestInd&));
    MOCK_METHOD1(eventBootstrappingConfirm, ndk::ScopedAStatus(const NanBootstrappingConfirmInd&));
    MOCK_METHOD1(eventBootstrappingRequest, ndk::ScopedAStatus(const NanBootstrappingRequestInd&));
    MOCK_METHOD3(notifyInitiatePairingResponse,
                 ndk::ScopedAStatus(char16_t, const NanStatus&, int32_t));
    MOCK_METHOD2(notifyRespondToPairingIndicationResponse,
                 ndk::ScopedAStatus(char16_t, const NanStatus&));
    MOCK_METHOD3(notifyInitiateBootstrappingResponse,
                 ndk::ScopedAStatus(char16_t, const NanStatus&, int32_t));
    MOCK_METHOD2(notifyRespondToBootstrappingIndicationResponse,
                 ndk::ScopedAStatus(char16_t, const NanStatus&));
    MOCK_METHOD2(notifySuspendResponse, ndk::ScopedAStatus(char16_t, const NanStatus&));
    MOCK_METHOD2(notifyResumeResponse, ndk::ScopedAStatus(char16_t, const NanStatus&));
    MOCK_METHOD2(notifyTerminatePairingResponse, ndk::ScopedAStatus(char16_t, const NanStatus&));
    MOCK_METHOD1(eventSuspensionModeChanged, ndk::ScopedAStatus(const NanSuspensionModeChangeInd&));
};

class WifiNanIfaceTest : public Test {
  protected:
    legacy_hal::wifi_hal_fn fake_func_table_;
    std::shared_ptr<NiceMock<::android::wifi_system::MockInterfaceTool>> iface_tool_{
            new NiceMock<::android::wifi_system::MockInterfaceTool>};
    std::shared_ptr<NiceMock<legacy_hal::MockWifiLegacyHal>> legacy_hal_{
            new NiceMock<legacy_hal::MockWifiLegacyHal>(iface_tool_, fake_func_table_, true)};
    std::shared_ptr<NiceMock<iface_util::MockWifiIfaceUtil>> iface_util_{
            new NiceMock<iface_util::MockWifiIfaceUtil>(iface_tool_, legacy_hal_)};
};

TEST_F(WifiNanIfaceTest, IfacEventHandlers_OnStateToggleOffOn) {
    // Ensure that event handlers are registered during nan iface creation.
    iface_util::IfaceEventHandlers captured_iface_event_handlers = {};
    EXPECT_CALL(*legacy_hal_, nanRegisterCallbackHandlers(testing::_, testing::_))
            .WillOnce(testing::Return(legacy_hal::WIFI_SUCCESS));
    EXPECT_CALL(*iface_util_, registerIfaceEventHandlers(testing::_, testing::_))
            .WillOnce(testing::Invoke(bind(CaptureIfaceEventHandlers, std::placeholders::_1,
                                           std::placeholders::_2, &captured_iface_event_handlers)));

    // Create nan iface and register a callback.
    // Note: Since we can't register a callback directly (gTest fails on
    //       AIBinder_linkToDeath), simulate the registration by overriding
    //       getEventCallbacks() to return our mock callback object.
    std::shared_ptr<MockNanIface> mock_nan_iface =
            MockNanIface::createMock(kIfaceName, false, legacy_hal_, iface_util_);
    std::shared_ptr<MockNanIfaceEventCallback> mock_event_callback =
            ndk::SharedRefBase::make<MockNanIfaceEventCallback>();
    mock_nan_iface->setMockCallback(mock_event_callback);

    // Ensure that the eventDisabled() function in the mock callback will be invoked.
    NanStatus expected_nan_status = {NanStatusCode::UNSUPPORTED_CONCURRENCY_NAN_DISABLED, ""};
    EXPECT_CALL(*mock_event_callback, eventDisabled(expected_nan_status)).Times(1);

    // Trigger the iface state toggle callback.
    captured_iface_event_handlers.on_state_toggle_off_on(kIfaceName);
}

}  // namespace wifi
}  // namespace hardware
}  // namespace android
}  // namespace aidl
