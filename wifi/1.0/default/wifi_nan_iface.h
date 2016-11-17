/*
 * Copyright (C) 2016 The Android Open Source Project
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

#include <android-base/macros.h>
#include <android/hardware/wifi/1.0/IWifiNanIface.h>
#include <android/hardware/wifi/1.0/IWifiNanIfaceEventCallback.h>

#include "wifi_legacy_hal.h"

namespace android {
namespace hardware {
namespace wifi {
namespace V1_0 {
namespace implementation {

/**
 * HIDL interface object used to control a NAN Iface instance.
 */
class WifiNanIface : public IWifiNanIface {
 public:
  WifiNanIface(const std::string& ifname,
               const std::weak_ptr<WifiLegacyHal> legacy_hal);
  // Refer to |WifiChip::invalidate()|.
  void invalidate();
  bool isValid();

  // HIDL methods exposed.
  Return<void> getName(getName_cb hidl_status_cb) override;
  Return<void> getType(getType_cb hidl_status_cb) override;
  Return<void> registerEventCallback(
      const sp<IWifiNanIfaceEventCallback>& callback,
      registerEventCallback_cb hidl_status_cb) override;
  Return<void> enableRequest(uint32_t cmd_id,
                             const NanEnableRequest& msg,
                             enableRequest_cb hidl_status_cb) override;
  Return<void> disableRequest(uint32_t cmd_id,
                              disableRequest_cb hidl_status_cb) override;
  Return<void> publishRequest(uint32_t cmd_id,
                              const NanPublishRequest& msg,
                              publishRequest_cb hidl_status_cb) override;
  Return<void> publishCancelRequest(
      uint32_t cmd_id,
      const NanPublishCancelRequest& msg,
      publishCancelRequest_cb hidl_status_cb) override;
  Return<void> subscribeRequest(uint32_t cmd_id,
                                const NanSubscribeRequest& msg,
                                subscribeRequest_cb hidl_status_cb) override;
  Return<void> subscribeCancelRequest(
      uint32_t cmd_id,
      const NanSubscribeCancelRequest& msg,
      subscribeCancelRequest_cb hidl_status_cb) override;
  Return<void> transmitFollowupRequest(
      uint32_t cmd_id,
      const NanTransmitFollowupRequest& msg,
      transmitFollowupRequest_cb hidl_status_cb) override;
  Return<void> configRequest(uint32_t cmd_id,
                             const NanConfigRequest& msg,
                             configRequest_cb hidl_status_cb) override;
  Return<void> beaconSdfPayloadRequest(
      uint32_t cmd_id,
      const NanBeaconSdfPayloadRequest& msg,
      beaconSdfPayloadRequest_cb hidl_status_cb) override;
  Return<void> getVersion(getVersion_cb hidl_status_cb) override;
  Return<void> getCapabilities(uint32_t cmd_id,
                               getCapabilities_cb hidl_status_cb) override;
  Return<void> dataInterfaceCreate(
      uint32_t cmd_id,
      const hidl_string& iface_name,
      dataInterfaceCreate_cb hidl_status_cb) override;
  Return<void> dataInterfaceDelete(
      uint32_t cmd_id,
      const hidl_string& iface_name,
      dataInterfaceDelete_cb hidl_status_cb) override;
  Return<void> dataRequestInitiator(
      uint32_t cmd_id,
      const NanDataPathInitiatorRequest& msg,
      dataRequestInitiator_cb hidl_status_cb) override;
  Return<void> dataIndicationResponse(
      uint32_t cmd_id,
      const NanDataPathIndicationResponse& msg,
      dataIndicationResponse_cb hidl_status_cb) override;
  Return<void> dataEnd(uint32_t cmd_id,
                       const NanDataPathEndRequest& msg,
                       dataEnd_cb hidl_status_cb) override;

 private:
  // Corresponding worker functions for the HIDL methods.
  std::pair<WifiStatus, std::string> getNameInternal();
  std::pair<WifiStatus, IfaceType> getTypeInternal();
  WifiStatus registerEventCallbackInternal(
      const sp<IWifiNanIfaceEventCallback>& callback);
  WifiStatus enableRequestInternal(uint32_t cmd_id,
                                   const NanEnableRequest& msg);
  WifiStatus disableRequestInternal(uint32_t cmd_id);
  WifiStatus publishRequestInternal(uint32_t cmd_id,
                                    const NanPublishRequest& msg);
  WifiStatus publishCancelRequestInternal(uint32_t cmd_id,
                                          const NanPublishCancelRequest& msg);
  WifiStatus subscribeRequestInternal(uint32_t cmd_id,
                                      const NanSubscribeRequest& msg);
  WifiStatus subscribeCancelRequestInternal(
      uint32_t cmd_id, const NanSubscribeCancelRequest& msg);
  WifiStatus transmitFollowupRequestInternal(
      uint32_t cmd_id, const NanTransmitFollowupRequest& msg);
  WifiStatus configRequestInternal(uint32_t cmd_id,
                                   const NanConfigRequest& msg);
  WifiStatus beaconSdfPayloadRequestInternal(
      uint32_t cmd_id, const NanBeaconSdfPayloadRequest& msg);
  std::pair<WifiStatus, NanVersion> getVersionInternal();
  WifiStatus getCapabilitiesInternal(uint32_t cmd_id);
  WifiStatus dataInterfaceCreateInternal(uint32_t cmd_id,
                                         const std::string& iface_name);
  WifiStatus dataInterfaceDeleteInternal(uint32_t cmd_id,
                                         const std::string& iface_name);
  WifiStatus dataRequestInitiatorInternal(
      uint32_t cmd_id, const NanDataPathInitiatorRequest& msg);
  WifiStatus dataIndicationResponseInternal(
      uint32_t cmd_id, const NanDataPathIndicationResponse& msg);
  WifiStatus dataEndInternal(uint32_t cmd_id, const NanDataPathEndRequest& msg);

  std::string ifname_;
  std::weak_ptr<WifiLegacyHal> legacy_hal_;
  std::vector<sp<IWifiNanIfaceEventCallback>> event_callbacks_;
  bool is_valid_;

  DISALLOW_COPY_AND_ASSIGN(WifiNanIface);
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace wifi
}  // namespace hardware
}  // namespace android

#endif  // WIFI_NAN_IFACE_H_
