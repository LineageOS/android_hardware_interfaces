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

#include <android-base/logging.h>

#include "hidl_return_util.h"
#include "hidl_struct_util.h"
#include "wifi_nan_iface.h"
#include "wifi_status_util.h"

namespace android {
namespace hardware {
namespace wifi {
namespace V1_0 {
namespace implementation {
using hidl_return_util::validateAndCall;

WifiNanIface::WifiNanIface(
    const std::string& ifname,
    const std::weak_ptr<legacy_hal::WifiLegacyHal> legacy_hal)
    : ifname_(ifname), legacy_hal_(legacy_hal), is_valid_(true) {
  // Register all the callbacks here. these should be valid for the lifetime
  // of the object. Whenever the mode changes legacy HAL will remove
  // all of these callbacks.
  legacy_hal::NanCallbackHandlers callback_handlers;

  // Callback for response.
  callback_handlers.on_notify_response = [&](
      legacy_hal::transaction_id id, const legacy_hal::NanResponseMsg& msg) {
    NanResponseMsgHeader hidl_header;
    if (!hidl_struct_util::convertLegacyNanResponseHeaderToHidl(msg,
                                                                &hidl_header)) {
      LOG(ERROR) << "Failed to convert nan response header";
      return;
    }
    // TODO: This is a union in the legacy HAL. Need to convert to appropriate
    // callback based on type.
    // Assuming |NanPublishResponseMsg| type here.
    NanPublishResponse hidl_body;
    if (!hidl_struct_util::convertLegacyNanPublishResponseToHidl(
            msg.body.publish_response, &hidl_body)) {
      LOG(ERROR) << "Failed to convert nan publish response";
      return;
    }
    NanPublishResponseMsg hidl_msg;
    hidl_msg.header = hidl_header;
    hidl_msg.body = hidl_body;
    for (const auto& callback : event_callbacks_) {
      if (!callback->notifyPublishResponse(id, hidl_msg).isOk()) {
        LOG(ERROR) << "Failed to invoke the callback";
      }
    }
  };
  // TODO: Register the remaining callbacks.
  legacy_hal::wifi_error legacy_status =
      legacy_hal_.lock()->nanRegisterCallbackHandlers(callback_handlers);
  if (legacy_status != legacy_hal::WIFI_SUCCESS) {
    LOG(ERROR) << "Failed to register nan callbacks. Invalidating object";
    invalidate();
  }
}

void WifiNanIface::invalidate() {
  legacy_hal_.reset();
  event_callbacks_.clear();
  is_valid_ = false;
}

bool WifiNanIface::isValid() {
  return is_valid_;
}

Return<void> WifiNanIface::getName(getName_cb hidl_status_cb) {
  return validateAndCall(this,
                         WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                         &WifiNanIface::getNameInternal,
                         hidl_status_cb);
}

Return<void> WifiNanIface::registerEventCallback(
    const sp<IWifiNanIfaceEventCallback>& callback,
    registerEventCallback_cb hidl_status_cb) {
  return validateAndCall(this,
                         WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                         &WifiNanIface::registerEventCallbackInternal,
                         hidl_status_cb,
                         callback);
}

Return<void> WifiNanIface::enableRequest(uint32_t cmd_id,
                                         const NanEnableRequest& msg,
                                         enableRequest_cb hidl_status_cb) {
  return validateAndCall(this,
                         WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                         &WifiNanIface::enableRequestInternal,
                         hidl_status_cb,
                         cmd_id,
                         msg);
}

Return<void> WifiNanIface::disableRequest(uint32_t cmd_id,
                                          disableRequest_cb hidl_status_cb) {
  return validateAndCall(this,
                         WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                         &WifiNanIface::disableRequestInternal,
                         hidl_status_cb,
                         cmd_id);
}

Return<void> WifiNanIface::publishRequest(uint32_t cmd_id,
                                          const NanPublishRequest& msg,
                                          publishRequest_cb hidl_status_cb) {
  return validateAndCall(this,
                         WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                         &WifiNanIface::publishRequestInternal,
                         hidl_status_cb,
                         cmd_id,
                         msg);
}

Return<void> WifiNanIface::publishCancelRequest(
    uint32_t cmd_id,
    const NanPublishCancelRequest& msg,
    publishCancelRequest_cb hidl_status_cb) {
  return validateAndCall(this,
                         WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                         &WifiNanIface::publishCancelRequestInternal,
                         hidl_status_cb,
                         cmd_id,
                         msg);
}

Return<void> WifiNanIface::subscribeRequest(
    uint32_t cmd_id,
    const NanSubscribeRequest& msg,
    subscribeRequest_cb hidl_status_cb) {
  return validateAndCall(this,
                         WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                         &WifiNanIface::subscribeRequestInternal,
                         hidl_status_cb,
                         cmd_id,
                         msg);
}

Return<void> WifiNanIface::subscribeCancelRequest(
    uint32_t cmd_id,
    const NanSubscribeCancelRequest& msg,
    subscribeCancelRequest_cb hidl_status_cb) {
  return validateAndCall(this,
                         WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                         &WifiNanIface::subscribeCancelRequestInternal,
                         hidl_status_cb,
                         cmd_id,
                         msg);
}

Return<void> WifiNanIface::transmitFollowupRequest(
    uint32_t cmd_id,
    const NanTransmitFollowupRequest& msg,
    transmitFollowupRequest_cb hidl_status_cb) {
  return validateAndCall(this,
                         WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                         &WifiNanIface::transmitFollowupRequestInternal,
                         hidl_status_cb,
                         cmd_id,
                         msg);
}

Return<void> WifiNanIface::configRequest(uint32_t cmd_id,
                                         const NanConfigRequest& msg,
                                         configRequest_cb hidl_status_cb) {
  return validateAndCall(this,
                         WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                         &WifiNanIface::configRequestInternal,
                         hidl_status_cb,
                         cmd_id,
                         msg);
}

Return<void> WifiNanIface::beaconSdfPayloadRequest(
    uint32_t cmd_id,
    const NanBeaconSdfPayloadRequest& msg,
    beaconSdfPayloadRequest_cb hidl_status_cb) {
  return validateAndCall(this,
                         WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                         &WifiNanIface::beaconSdfPayloadRequestInternal,
                         hidl_status_cb,
                         cmd_id,
                         msg);
}

Return<void> WifiNanIface::getVersion(getVersion_cb hidl_status_cb) {
  return validateAndCall(this,
                         WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                         &WifiNanIface::getVersionInternal,
                         hidl_status_cb);
}

Return<void> WifiNanIface::getCapabilities(uint32_t cmd_id,
                                           getCapabilities_cb hidl_status_cb) {
  return validateAndCall(this,
                         WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                         &WifiNanIface::getCapabilitiesInternal,
                         hidl_status_cb,
                         cmd_id);
}

Return<void> WifiNanIface::dataInterfaceCreate(
    uint32_t cmd_id,
    const hidl_string& iface_name,
    dataInterfaceCreate_cb hidl_status_cb) {
  return validateAndCall(this,
                         WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                         &WifiNanIface::dataInterfaceCreateInternal,
                         hidl_status_cb,
                         cmd_id,
                         iface_name);
}

Return<void> WifiNanIface::dataInterfaceDelete(
    uint32_t cmd_id,
    const hidl_string& iface_name,
    dataInterfaceDelete_cb hidl_status_cb) {
  return validateAndCall(this,
                         WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                         &WifiNanIface::dataInterfaceDeleteInternal,
                         hidl_status_cb,
                         cmd_id,
                         iface_name);
}

Return<void> WifiNanIface::dataRequestInitiator(
    uint32_t cmd_id,
    const NanDataPathInitiatorRequest& msg,
    dataRequestInitiator_cb hidl_status_cb) {
  return validateAndCall(this,
                         WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                         &WifiNanIface::dataRequestInitiatorInternal,
                         hidl_status_cb,
                         cmd_id,
                         msg);
}

Return<void> WifiNanIface::dataIndicationResponse(
    uint32_t cmd_id,
    const NanDataPathIndicationResponse& msg,
    dataIndicationResponse_cb hidl_status_cb) {
  return validateAndCall(this,
                         WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                         &WifiNanIface::dataIndicationResponseInternal,
                         hidl_status_cb,
                         cmd_id,
                         msg);
}

Return<void> WifiNanIface::dataEnd(uint32_t cmd_id,
                                   const NanDataPathEndRequest& msg,
                                   dataEnd_cb hidl_status_cb) {
  return validateAndCall(this,
                         WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                         &WifiNanIface::dataEndInternal,
                         hidl_status_cb,
                         cmd_id,
                         msg);
}

Return<void> WifiNanIface::getType(getType_cb hidl_status_cb) {
  return validateAndCall(this,
                         WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                         &WifiNanIface::getTypeInternal,
                         hidl_status_cb);
}

std::pair<WifiStatus, std::string> WifiNanIface::getNameInternal() {
  return {createWifiStatus(WifiStatusCode::SUCCESS), ifname_};
}

std::pair<WifiStatus, IfaceType> WifiNanIface::getTypeInternal() {
  return {createWifiStatus(WifiStatusCode::SUCCESS), IfaceType::NAN};
}

WifiStatus WifiNanIface::registerEventCallbackInternal(
    const sp<IWifiNanIfaceEventCallback>& callback) {
  // TODO(b/31632518): remove the callback when the client is destroyed
  event_callbacks_.emplace_back(callback);
  return createWifiStatus(WifiStatusCode::SUCCESS);
}

WifiStatus WifiNanIface::enableRequestInternal(uint32_t cmd_id,
                                               const NanEnableRequest& msg) {
  legacy_hal::NanEnableRequest legacy_msg;
  if (!hidl_struct_util::convertHidlNanEnableRequestToLegacy(msg,
                                                             &legacy_msg)) {
    return createWifiStatus(WifiStatusCode::ERROR_INVALID_ARGS);
  }
  legacy_hal::wifi_error legacy_status =
      legacy_hal_.lock()->nanEnableRequest(cmd_id, legacy_msg);
  return createWifiStatusFromLegacyError(legacy_status);
}

WifiStatus WifiNanIface::disableRequestInternal(uint32_t cmd_id) {
  legacy_hal::wifi_error legacy_status =
      legacy_hal_.lock()->nanDisableRequest(cmd_id);
  return createWifiStatusFromLegacyError(legacy_status);
}

WifiStatus WifiNanIface::publishRequestInternal(uint32_t cmd_id,
                                                const NanPublishRequest& msg) {
  legacy_hal::NanPublishRequest legacy_msg;
  if (!hidl_struct_util::convertHidlNanPublishRequestToLegacy(msg,
                                                              &legacy_msg)) {
    return createWifiStatus(WifiStatusCode::ERROR_INVALID_ARGS);
  }
  legacy_hal::wifi_error legacy_status =
      legacy_hal_.lock()->nanPublishRequest(cmd_id, legacy_msg);
  return createWifiStatusFromLegacyError(legacy_status);
}

WifiStatus WifiNanIface::publishCancelRequestInternal(
    uint32_t cmd_id, const NanPublishCancelRequest& msg) {
  legacy_hal::NanPublishCancelRequest legacy_msg;
  if (!hidl_struct_util::convertHidlNanPublishCancelRequestToLegacy(
          msg, &legacy_msg)) {
    return createWifiStatus(WifiStatusCode::ERROR_INVALID_ARGS);
  }
  legacy_hal::wifi_error legacy_status =
      legacy_hal_.lock()->nanPublishCancelRequest(cmd_id, legacy_msg);
  return createWifiStatusFromLegacyError(legacy_status);
}

WifiStatus WifiNanIface::subscribeRequestInternal(
    uint32_t cmd_id, const NanSubscribeRequest& msg) {
  legacy_hal::NanSubscribeRequest legacy_msg;
  if (!hidl_struct_util::convertHidlNanSubscribeRequestToLegacy(msg,
                                                                &legacy_msg)) {
    return createWifiStatus(WifiStatusCode::ERROR_INVALID_ARGS);
  }
  legacy_hal::wifi_error legacy_status =
      legacy_hal_.lock()->nanSubscribeRequest(cmd_id, legacy_msg);
  return createWifiStatusFromLegacyError(legacy_status);
}

WifiStatus WifiNanIface::subscribeCancelRequestInternal(
    uint32_t /* cmd_id */, const NanSubscribeCancelRequest& /* msg */) {
  // TODO implement
  return createWifiStatus(WifiStatusCode::SUCCESS);
}
WifiStatus WifiNanIface::transmitFollowupRequestInternal(
    uint32_t /* cmd_id */, const NanTransmitFollowupRequest& /* msg */) {
  // TODO implement
  return createWifiStatus(WifiStatusCode::SUCCESS);
}
WifiStatus WifiNanIface::configRequestInternal(
    uint32_t /* cmd_id */, const NanConfigRequest& /* msg */) {
  // TODO implement
  return createWifiStatus(WifiStatusCode::SUCCESS);
}
WifiStatus WifiNanIface::beaconSdfPayloadRequestInternal(
    uint32_t /* cmd_id */, const NanBeaconSdfPayloadRequest& /* msg */) {
  // TODO implement
  return createWifiStatus(WifiStatusCode::SUCCESS);
}
std::pair<WifiStatus, NanVersion> WifiNanIface::getVersionInternal() {
  // TODO implement
  return {createWifiStatus(WifiStatusCode::SUCCESS), 0};
}
WifiStatus WifiNanIface::getCapabilitiesInternal(uint32_t /* cmd_id */) {
  // TODO implement
  return createWifiStatus(WifiStatusCode::SUCCESS);
}
WifiStatus WifiNanIface::dataInterfaceCreateInternal(
    uint32_t /* cmd_id */, const std::string& /* iface_name */) {
  // TODO implement
  return createWifiStatus(WifiStatusCode::SUCCESS);
}
WifiStatus WifiNanIface::dataInterfaceDeleteInternal(
    uint32_t /* cmd_id */, const std::string& /* iface_name */) {
  // TODO implement
  return createWifiStatus(WifiStatusCode::SUCCESS);
}
WifiStatus WifiNanIface::dataRequestInitiatorInternal(
    uint32_t /* cmd_id */, const NanDataPathInitiatorRequest& /* msg */) {
  // TODO implement
  return createWifiStatus(WifiStatusCode::SUCCESS);
}
WifiStatus WifiNanIface::dataIndicationResponseInternal(
    uint32_t /* cmd_id */, const NanDataPathIndicationResponse& /* msg */) {
  // TODO implement
  return createWifiStatus(WifiStatusCode::SUCCESS);
}
WifiStatus WifiNanIface::dataEndInternal(
    uint32_t /* cmd_id */, const NanDataPathEndRequest& /* msg */) {
  // TODO implement
  return createWifiStatus(WifiStatusCode::SUCCESS);
}
}  // namespace implementation
}  // namespace V1_0
}  // namespace wifi
}  // namespace hardware
}  // namespace android
