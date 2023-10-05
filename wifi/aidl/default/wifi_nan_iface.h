/*
 * Copyright (C) 2022 The Android Open Source Project
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

#ifndef WIFI_NAN_IFACE_H_
#define WIFI_NAN_IFACE_H_

#include <aidl/android/hardware/wifi/BnWifiNanIface.h>
#include <aidl/android/hardware/wifi/IWifiNanIfaceEventCallback.h>
#include <android-base/macros.h>

#include "aidl_callback_util.h"
#include "wifi_iface_util.h"
#include "wifi_legacy_hal.h"

namespace aidl {
namespace android {
namespace hardware {
namespace wifi {

/**
 * AIDL interface object used to control a NAN Iface instance.
 */
class WifiNanIface : public BnWifiNanIface {
  public:
    WifiNanIface(const std::string& ifname, bool is_dedicated_iface,
                 const std::weak_ptr<legacy_hal::WifiLegacyHal> legacy_hal,
                 const std::weak_ptr<iface_util::WifiIfaceUtil> iface_util);

    // Factory method - use instead of default constructor.
    static std::shared_ptr<WifiNanIface> create(
            const std::string& ifname, bool is_dedicated_iface,
            const std::weak_ptr<legacy_hal::WifiLegacyHal> legacy_hal,
            const std::weak_ptr<iface_util::WifiIfaceUtil> iface_util);

    // Refer to |WifiChip::invalidate()|.
    void invalidate();
    bool isValid();
    std::string getName();

    // AIDL methods exposed.
    ndk::ScopedAStatus getName(std::string* _aidl_return) override;
    ndk::ScopedAStatus registerEventCallback(
            const std::shared_ptr<IWifiNanIfaceEventCallback>& in_callback) override;
    ndk::ScopedAStatus getCapabilitiesRequest(char16_t in_cmdId) override;
    ndk::ScopedAStatus enableRequest(char16_t in_cmdId, const NanEnableRequest& in_msg1,
                                     const NanConfigRequestSupplemental& in_msg2) override;
    ndk::ScopedAStatus configRequest(char16_t in_cmdId, const NanConfigRequest& in_msg1,
                                     const NanConfigRequestSupplemental& in_msg2) override;
    ndk::ScopedAStatus disableRequest(char16_t in_cmdId) override;
    ndk::ScopedAStatus startPublishRequest(char16_t in_cmdId,
                                           const NanPublishRequest& in_msg) override;
    ndk::ScopedAStatus stopPublishRequest(char16_t in_cmdId, int8_t in_sessionId) override;
    ndk::ScopedAStatus startSubscribeRequest(char16_t in_cmdId,
                                             const NanSubscribeRequest& in_msg) override;
    ndk::ScopedAStatus stopSubscribeRequest(char16_t in_cmdId, int8_t in_sessionId) override;
    ndk::ScopedAStatus transmitFollowupRequest(char16_t in_cmdId,
                                               const NanTransmitFollowupRequest& in_msg) override;
    ndk::ScopedAStatus createDataInterfaceRequest(char16_t in_cmdId,
                                                  const std::string& in_ifaceName) override;
    ndk::ScopedAStatus deleteDataInterfaceRequest(char16_t in_cmdId,
                                                  const std::string& in_ifaceName) override;
    ndk::ScopedAStatus initiateDataPathRequest(char16_t in_cmdId,
                                               const NanInitiateDataPathRequest& in_msg) override;
    ndk::ScopedAStatus respondToDataPathIndicationRequest(
            char16_t in_cmdId, const NanRespondToDataPathIndicationRequest& in_msg) override;
    ndk::ScopedAStatus terminateDataPathRequest(char16_t in_cmdId,
                                                int32_t in_ndpInstanceId) override;
    ndk::ScopedAStatus initiatePairingRequest(char16_t in_cmdId,
                                              const NanPairingRequest& in_msg) override;
    ndk::ScopedAStatus respondToPairingIndicationRequest(
            char16_t in_cmdId, const NanRespondToPairingIndicationRequest& in_msg) override;
    ndk::ScopedAStatus terminatePairingRequest(char16_t in_cmdId, int32_t in_pairingId) override;
    ndk::ScopedAStatus initiateBootstrappingRequest(char16_t in_cmdId,
                                                    const NanBootstrappingRequest& in_msg) override;
    ndk::ScopedAStatus respondToBootstrappingIndicationRequest(
            char16_t in_cmdId, const NanBootstrappingResponse& in_msg) override;
    ndk::ScopedAStatus suspendRequest(char16_t in_cmdId, int8_t sessionId) override;
    ndk::ScopedAStatus resumeRequest(char16_t in_cmdId, int8_t sessionId) override;

  protected:
    // Accessible to child class in the gTest suite.
    void setWeakPtr(std::weak_ptr<WifiNanIface> ptr);
    void registerCallbackHandlers();

  private:
    // Corresponding worker functions for the AIDL methods.
    std::pair<std::string, ndk::ScopedAStatus> getNameInternal();
    ndk::ScopedAStatus registerEventCallbackInternal(
            const std::shared_ptr<IWifiNanIfaceEventCallback>& callback);
    ndk::ScopedAStatus getCapabilitiesRequestInternal(char16_t cmd_id);
    ndk::ScopedAStatus enableRequestInternal(char16_t cmd_id, const NanEnableRequest& msg1,
                                             const NanConfigRequestSupplemental& msg2);
    ndk::ScopedAStatus configRequestInternal(char16_t cmd_id, const NanConfigRequest& msg1,
                                             const NanConfigRequestSupplemental& msg2);
    ndk::ScopedAStatus disableRequestInternal(char16_t cmd_id);
    ndk::ScopedAStatus startPublishRequestInternal(char16_t cmd_id, const NanPublishRequest& msg);
    ndk::ScopedAStatus stopPublishRequestInternal(char16_t cmd_id, int8_t sessionId);
    ndk::ScopedAStatus startSubscribeRequestInternal(char16_t cmd_id,
                                                     const NanSubscribeRequest& msg);
    ndk::ScopedAStatus stopSubscribeRequestInternal(char16_t cmd_id, int8_t sessionId);
    ndk::ScopedAStatus transmitFollowupRequestInternal(char16_t cmd_id,
                                                       const NanTransmitFollowupRequest& msg);
    ndk::ScopedAStatus createDataInterfaceRequestInternal(char16_t cmd_id,
                                                          const std::string& iface_name);
    ndk::ScopedAStatus deleteDataInterfaceRequestInternal(char16_t cmd_id,
                                                          const std::string& iface_name);
    ndk::ScopedAStatus initiateDataPathRequestInternal(char16_t cmd_id,
                                                       const NanInitiateDataPathRequest& msg);
    ndk::ScopedAStatus respondToDataPathIndicationRequestInternal(
            char16_t cmd_id, const NanRespondToDataPathIndicationRequest& msg);
    ndk::ScopedAStatus terminateDataPathRequestInternal(char16_t cmd_id, int32_t ndpInstanceId);
    ndk::ScopedAStatus initiatePairingRequestInternal(char16_t cmd_id,
                                                      const NanPairingRequest& msg);
    ndk::ScopedAStatus respondToPairingIndicationRequestInternal(
            char16_t cmd_id, const NanRespondToPairingIndicationRequest& msg);
    ndk::ScopedAStatus terminatePairingRequestInternal(char16_t cmd_id, int32_t pairingId);
    ndk::ScopedAStatus initiateBootstrappingRequestInternal(char16_t cmd_id,
                                                            const NanBootstrappingRequest& msg);
    ndk::ScopedAStatus respondToBootstrappingIndicationRequestInternal(
            char16_t cmd_id, const NanBootstrappingResponse& msg);
    ndk::ScopedAStatus suspendRequestInternal(char16_t in_cmdId, int8_t sessionId);
    ndk::ScopedAStatus resumeRequestInternal(char16_t in_cmdId, int8_t sessionId);

    // Overridden in the gTest suite.
    virtual std::set<std::shared_ptr<IWifiNanIfaceEventCallback>> getEventCallbacks();

    std::string ifname_;
    bool is_dedicated_iface_;
    std::weak_ptr<legacy_hal::WifiLegacyHal> legacy_hal_;
    std::weak_ptr<iface_util::WifiIfaceUtil> iface_util_;
    bool is_valid_;
    std::weak_ptr<WifiNanIface> weak_ptr_this_;
    aidl_callback_util::AidlCallbackHandler<IWifiNanIfaceEventCallback> event_cb_handler_;

    DISALLOW_COPY_AND_ASSIGN(WifiNanIface);
};

}  // namespace wifi
}  // namespace hardware
}  // namespace android
}  // namespace aidl

#endif  // WIFI_NAN_IFACE_H_
