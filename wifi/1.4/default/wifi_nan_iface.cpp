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
namespace V1_4 {
namespace implementation {
using hidl_return_util::validateAndCall;

WifiNanIface::WifiNanIface(
    const std::string& ifname, bool is_dedicated_iface,
    const std::weak_ptr<legacy_hal::WifiLegacyHal> legacy_hal,
    const std::weak_ptr<iface_util::WifiIfaceUtil> iface_util)
    : ifname_(ifname),
      is_dedicated_iface_(is_dedicated_iface),
      legacy_hal_(legacy_hal),
      iface_util_(iface_util),
      is_valid_(true) {
    if (is_dedicated_iface_) {
        // If using a dedicated iface, set the iface up first.
        if (!iface_util_.lock()->setUpState(ifname_, true)) {
            // Fatal failure, invalidate the iface object.
            invalidate();
            return;
        }
    }
    // Register all the callbacks here. these should be valid for the lifetime
    // of the object. Whenever the mode changes legacy HAL will remove
    // all of these callbacks.
    legacy_hal::NanCallbackHandlers callback_handlers;
    android::wp<WifiNanIface> weak_ptr_this(this);

    // Callback for response.
    callback_handlers
        .on_notify_response = [weak_ptr_this](
                                  legacy_hal::transaction_id id,
                                  const legacy_hal::NanResponseMsg& msg) {
        const auto shared_ptr_this = weak_ptr_this.promote();
        if (!shared_ptr_this.get() || !shared_ptr_this->isValid()) {
            LOG(ERROR) << "Callback invoked on an invalid object";
            return;
        }
        WifiNanStatus wifiNanStatus;
        if (!hidl_struct_util::convertLegacyNanResponseHeaderToHidl(
                msg, &wifiNanStatus)) {
            LOG(ERROR) << "Failed to convert nan response header";
            return;
        }

        switch (msg.response_type) {
            case legacy_hal::NAN_RESPONSE_ENABLED: {
                for (const auto& callback :
                     shared_ptr_this->getEventCallbacks()) {
                    if (!callback->notifyEnableResponse(id, wifiNanStatus)
                             .isOk()) {
                        LOG(ERROR) << "Failed to invoke the callback";
                    }
                }
                break;
            }
            case legacy_hal::NAN_RESPONSE_DISABLED: {
                for (const auto& callback :
                     shared_ptr_this->getEventCallbacks()) {
                    if (!callback->notifyDisableResponse(id, wifiNanStatus)
                             .isOk()) {
                        LOG(ERROR) << "Failed to invoke the callback";
                    }
                }
                break;
            }
            case legacy_hal::NAN_RESPONSE_PUBLISH: {
                for (const auto& callback :
                     shared_ptr_this->getEventCallbacks()) {
                    if (!callback
                             ->notifyStartPublishResponse(
                                 id, wifiNanStatus,
                                 msg.body.publish_response.publish_id)
                             .isOk()) {
                        LOG(ERROR) << "Failed to invoke the callback";
                    }
                }
                break;
            }
            case legacy_hal::NAN_RESPONSE_PUBLISH_CANCEL: {
                for (const auto& callback :
                     shared_ptr_this->getEventCallbacks()) {
                    if (!callback->notifyStopPublishResponse(id, wifiNanStatus)
                             .isOk()) {
                        LOG(ERROR) << "Failed to invoke the callback";
                    }
                }
                break;
            }
            case legacy_hal::NAN_RESPONSE_TRANSMIT_FOLLOWUP: {
                for (const auto& callback :
                     shared_ptr_this->getEventCallbacks()) {
                    if (!callback
                             ->notifyTransmitFollowupResponse(id, wifiNanStatus)
                             .isOk()) {
                        LOG(ERROR) << "Failed to invoke the callback";
                    }
                }
                break;
            }
            case legacy_hal::NAN_RESPONSE_SUBSCRIBE: {
                for (const auto& callback :
                     shared_ptr_this->getEventCallbacks()) {
                    if (!callback
                             ->notifyStartSubscribeResponse(
                                 id, wifiNanStatus,
                                 msg.body.subscribe_response.subscribe_id)
                             .isOk()) {
                        LOG(ERROR) << "Failed to invoke the callback";
                    }
                }
                break;
            }
            case legacy_hal::NAN_RESPONSE_SUBSCRIBE_CANCEL: {
                for (const auto& callback :
                     shared_ptr_this->getEventCallbacks()) {
                    if (!callback
                             ->notifyStopSubscribeResponse(id, wifiNanStatus)
                             .isOk()) {
                        LOG(ERROR) << "Failed to invoke the callback";
                    }
                }
                break;
            }
            case legacy_hal::NAN_RESPONSE_CONFIG: {
                for (const auto& callback :
                     shared_ptr_this->getEventCallbacks()) {
                    if (!callback->notifyConfigResponse(id, wifiNanStatus)
                             .isOk()) {
                        LOG(ERROR) << "Failed to invoke the callback";
                    }
                }
                break;
            }
            case legacy_hal::NAN_GET_CAPABILITIES: {
                NanCapabilities hidl_struct;
                if (!hidl_struct_util::
                        convertLegacyNanCapabilitiesResponseToHidl(
                            msg.body.nan_capabilities, &hidl_struct)) {
                    LOG(ERROR) << "Failed to convert nan capabilities response";
                    return;
                }
                for (const auto& callback :
                     shared_ptr_this->getEventCallbacks()) {
                    if (!callback
                             ->notifyCapabilitiesResponse(id, wifiNanStatus,
                                                          hidl_struct)
                             .isOk()) {
                        LOG(ERROR) << "Failed to invoke the callback";
                    }
                }
                break;
            }
            case legacy_hal::NAN_DP_INTERFACE_CREATE: {
                for (const auto& callback :
                     shared_ptr_this->getEventCallbacks()) {
                    if (!callback
                             ->notifyCreateDataInterfaceResponse(id,
                                                                 wifiNanStatus)
                             .isOk()) {
                        LOG(ERROR) << "Failed to invoke the callback";
                    }
                }
                break;
            }
            case legacy_hal::NAN_DP_INTERFACE_DELETE: {
                for (const auto& callback :
                     shared_ptr_this->getEventCallbacks()) {
                    if (!callback
                             ->notifyDeleteDataInterfaceResponse(id,
                                                                 wifiNanStatus)
                             .isOk()) {
                        LOG(ERROR) << "Failed to invoke the callback";
                    }
                }
                break;
            }
            case legacy_hal::NAN_DP_INITIATOR_RESPONSE: {
                for (const auto& callback :
                     shared_ptr_this->getEventCallbacks()) {
                    if (!callback
                             ->notifyInitiateDataPathResponse(
                                 id, wifiNanStatus,
                                 msg.body.data_request_response.ndp_instance_id)
                             .isOk()) {
                        LOG(ERROR) << "Failed to invoke the callback";
                    }
                }
                break;
            }
            case legacy_hal::NAN_DP_RESPONDER_RESPONSE: {
                for (const auto& callback :
                     shared_ptr_this->getEventCallbacks()) {
                    if (!callback
                             ->notifyRespondToDataPathIndicationResponse(
                                 id, wifiNanStatus)
                             .isOk()) {
                        LOG(ERROR) << "Failed to invoke the callback";
                    }
                }
                break;
            }
            case legacy_hal::NAN_DP_END: {
                for (const auto& callback :
                     shared_ptr_this->getEventCallbacks()) {
                    if (!callback
                             ->notifyTerminateDataPathResponse(id,
                                                               wifiNanStatus)
                             .isOk()) {
                        LOG(ERROR) << "Failed to invoke the callback";
                    }
                }
                break;
            }
            case legacy_hal::NAN_RESPONSE_BEACON_SDF_PAYLOAD:
            /* fall through */
            case legacy_hal::NAN_RESPONSE_TCA:
            /* fall through */
            case legacy_hal::NAN_RESPONSE_STATS:
            /* fall through */
            case legacy_hal::NAN_RESPONSE_ERROR:
            /* fall through */
            default:
                LOG(ERROR) << "Unknown or unhandled response type: "
                           << msg.response_type;
                return;
        }
    };

    callback_handlers.on_event_disc_eng_event =
        [weak_ptr_this](const legacy_hal::NanDiscEngEventInd& msg) {
            const auto shared_ptr_this = weak_ptr_this.promote();
            if (!shared_ptr_this.get() || !shared_ptr_this->isValid()) {
                LOG(ERROR) << "Callback invoked on an invalid object";
                return;
            }
            NanClusterEventInd hidl_struct;
            // event types defined identically - hence can be cast
            hidl_struct.eventType = (NanClusterEventType)msg.event_type;
            hidl_struct.addr = msg.data.mac_addr.addr;

            for (const auto& callback : shared_ptr_this->getEventCallbacks()) {
                if (!callback->eventClusterEvent(hidl_struct).isOk()) {
                    LOG(ERROR) << "Failed to invoke the callback";
                }
            }
        };

    callback_handlers.on_event_disabled =
        [weak_ptr_this](const legacy_hal::NanDisabledInd& msg) {
            const auto shared_ptr_this = weak_ptr_this.promote();
            if (!shared_ptr_this.get() || !shared_ptr_this->isValid()) {
                LOG(ERROR) << "Callback invoked on an invalid object";
                return;
            }
            WifiNanStatus status;
            hidl_struct_util::convertToWifiNanStatus(
                msg.reason, msg.nan_reason, sizeof(msg.nan_reason), &status);

            for (const auto& callback : shared_ptr_this->getEventCallbacks()) {
                if (!callback->eventDisabled(status).isOk()) {
                    LOG(ERROR) << "Failed to invoke the callback";
                }
            }
        };

    callback_handlers.on_event_publish_terminated =
        [weak_ptr_this](const legacy_hal::NanPublishTerminatedInd& msg) {
            const auto shared_ptr_this = weak_ptr_this.promote();
            if (!shared_ptr_this.get() || !shared_ptr_this->isValid()) {
                LOG(ERROR) << "Callback invoked on an invalid object";
                return;
            }
            WifiNanStatus status;
            hidl_struct_util::convertToWifiNanStatus(
                msg.reason, msg.nan_reason, sizeof(msg.nan_reason), &status);

            for (const auto& callback : shared_ptr_this->getEventCallbacks()) {
                if (!callback->eventPublishTerminated(msg.publish_id, status)
                         .isOk()) {
                    LOG(ERROR) << "Failed to invoke the callback";
                }
            }
        };

    callback_handlers.on_event_subscribe_terminated =
        [weak_ptr_this](const legacy_hal::NanSubscribeTerminatedInd& msg) {
            const auto shared_ptr_this = weak_ptr_this.promote();
            if (!shared_ptr_this.get() || !shared_ptr_this->isValid()) {
                LOG(ERROR) << "Callback invoked on an invalid object";
                return;
            }
            WifiNanStatus status;
            hidl_struct_util::convertToWifiNanStatus(
                msg.reason, msg.nan_reason, sizeof(msg.nan_reason), &status);

            for (const auto& callback : shared_ptr_this->getEventCallbacks()) {
                if (!callback
                         ->eventSubscribeTerminated(msg.subscribe_id, status)
                         .isOk()) {
                    LOG(ERROR) << "Failed to invoke the callback";
                }
            }
        };

    callback_handlers.on_event_match =
        [weak_ptr_this](const legacy_hal::NanMatchInd& msg) {
            const auto shared_ptr_this = weak_ptr_this.promote();
            if (!shared_ptr_this.get() || !shared_ptr_this->isValid()) {
                LOG(ERROR) << "Callback invoked on an invalid object";
                return;
            }
            NanMatchInd hidl_struct;
            if (!hidl_struct_util::convertLegacyNanMatchIndToHidl(
                    msg, &hidl_struct)) {
                LOG(ERROR) << "Failed to convert nan capabilities response";
                return;
            }

            for (const auto& callback : shared_ptr_this->getEventCallbacks()) {
                if (!callback->eventMatch(hidl_struct).isOk()) {
                    LOG(ERROR) << "Failed to invoke the callback";
                }
            }
        };

    callback_handlers.on_event_match_expired =
        [weak_ptr_this](const legacy_hal::NanMatchExpiredInd& msg) {
            const auto shared_ptr_this = weak_ptr_this.promote();
            if (!shared_ptr_this.get() || !shared_ptr_this->isValid()) {
                LOG(ERROR) << "Callback invoked on an invalid object";
                return;
            }
            for (const auto& callback : shared_ptr_this->getEventCallbacks()) {
                if (!callback
                         ->eventMatchExpired(msg.publish_subscribe_id,
                                             msg.requestor_instance_id)
                         .isOk()) {
                    LOG(ERROR) << "Failed to invoke the callback";
                }
            }
        };

    callback_handlers.on_event_followup =
        [weak_ptr_this](const legacy_hal::NanFollowupInd& msg) {
            const auto shared_ptr_this = weak_ptr_this.promote();
            if (!shared_ptr_this.get() || !shared_ptr_this->isValid()) {
                LOG(ERROR) << "Callback invoked on an invalid object";
                return;
            }
            NanFollowupReceivedInd hidl_struct;
            if (!hidl_struct_util::convertLegacyNanFollowupIndToHidl(
                    msg, &hidl_struct)) {
                LOG(ERROR) << "Failed to convert nan capabilities response";
                return;
            }

            for (const auto& callback : shared_ptr_this->getEventCallbacks()) {
                if (!callback->eventFollowupReceived(hidl_struct).isOk()) {
                    LOG(ERROR) << "Failed to invoke the callback";
                }
            }
        };

    callback_handlers.on_event_transmit_follow_up =
        [weak_ptr_this](const legacy_hal::NanTransmitFollowupInd& msg) {
            const auto shared_ptr_this = weak_ptr_this.promote();
            if (!shared_ptr_this.get() || !shared_ptr_this->isValid()) {
                LOG(ERROR) << "Callback invoked on an invalid object";
                return;
            }
            WifiNanStatus status;
            hidl_struct_util::convertToWifiNanStatus(
                msg.reason, msg.nan_reason, sizeof(msg.nan_reason), &status);

            for (const auto& callback : shared_ptr_this->getEventCallbacks()) {
                if (!callback->eventTransmitFollowup(msg.id, status).isOk()) {
                    LOG(ERROR) << "Failed to invoke the callback";
                }
            }
        };

    callback_handlers.on_event_data_path_request =
        [weak_ptr_this](const legacy_hal::NanDataPathRequestInd& msg) {
            const auto shared_ptr_this = weak_ptr_this.promote();
            if (!shared_ptr_this.get() || !shared_ptr_this->isValid()) {
                LOG(ERROR) << "Callback invoked on an invalid object";
                return;
            }
            NanDataPathRequestInd hidl_struct;
            if (!hidl_struct_util::convertLegacyNanDataPathRequestIndToHidl(
                    msg, &hidl_struct)) {
                LOG(ERROR) << "Failed to convert nan capabilities response";
                return;
            }

            for (const auto& callback : shared_ptr_this->getEventCallbacks()) {
                if (!callback->eventDataPathRequest(hidl_struct).isOk()) {
                    LOG(ERROR) << "Failed to invoke the callback";
                }
            }
        };

    callback_handlers.on_event_data_path_confirm =
        [weak_ptr_this](const legacy_hal::NanDataPathConfirmInd& msg) {
            const auto shared_ptr_this = weak_ptr_this.promote();
            if (!shared_ptr_this.get() || !shared_ptr_this->isValid()) {
                LOG(ERROR) << "Callback invoked on an invalid object";
                return;
            }
            V1_2::NanDataPathConfirmInd hidl_struct;
            if (!hidl_struct_util::convertLegacyNanDataPathConfirmIndToHidl(
                    msg, &hidl_struct)) {
                LOG(ERROR) << "Failed to convert nan capabilities response";
                return;
            }

            for (const auto& callback :
                 shared_ptr_this->getEventCallbacks_1_2()) {
                if (!callback->eventDataPathConfirm_1_2(hidl_struct).isOk()) {
                    LOG(ERROR) << "Failed to invoke the callback";
                }
            }
        };

    callback_handlers.on_event_data_path_end =
        [weak_ptr_this](const legacy_hal::NanDataPathEndInd& msg) {
            const auto shared_ptr_this = weak_ptr_this.promote();
            if (!shared_ptr_this.get() || !shared_ptr_this->isValid()) {
                LOG(ERROR) << "Callback invoked on an invalid object";
                return;
            }
            for (const auto& callback : shared_ptr_this->getEventCallbacks()) {
                for (int i = 0; i < msg.num_ndp_instances; ++i) {
                    if (!callback
                             ->eventDataPathTerminated(msg.ndp_instance_id[i])
                             .isOk()) {
                        LOG(ERROR) << "Failed to invoke the callback";
                    }
                }
            }
        };

    callback_handlers.on_event_beacon_sdf_payload =
        [weak_ptr_this](const legacy_hal::NanBeaconSdfPayloadInd& /* msg */) {
            LOG(ERROR) << "on_event_beacon_sdf_payload - should not be called";
        };

    callback_handlers.on_event_range_request =
        [weak_ptr_this](const legacy_hal::NanRangeRequestInd& /* msg */) {
            LOG(ERROR) << "on_event_range_request - should not be called";
        };

    callback_handlers.on_event_range_report =
        [weak_ptr_this](const legacy_hal::NanRangeReportInd& /* msg */) {
            LOG(ERROR) << "on_event_range_report - should not be called";
        };

    callback_handlers
        .on_event_schedule_update = [weak_ptr_this](
                                        const legacy_hal::
                                            NanDataPathScheduleUpdateInd& msg) {
        const auto shared_ptr_this = weak_ptr_this.promote();
        if (!shared_ptr_this.get() || !shared_ptr_this->isValid()) {
            LOG(ERROR) << "Callback invoked on an invalid object";
            return;
        }
        V1_2::NanDataPathScheduleUpdateInd hidl_struct;
        if (!hidl_struct_util::convertLegacyNanDataPathScheduleUpdateIndToHidl(
                msg, &hidl_struct)) {
            LOG(ERROR) << "Failed to convert nan capabilities response";
            return;
        }

        for (const auto& callback : shared_ptr_this->getEventCallbacks_1_2()) {
            if (!callback->eventDataPathScheduleUpdate(hidl_struct).isOk()) {
                LOG(ERROR) << "Failed to invoke the callback";
            }
        }
    };

    legacy_hal::wifi_error legacy_status =
        legacy_hal_.lock()->nanRegisterCallbackHandlers(ifname_,
                                                        callback_handlers);
    if (legacy_status != legacy_hal::WIFI_SUCCESS) {
        LOG(ERROR) << "Failed to register nan callbacks. Invalidating object";
        invalidate();
    }

    // Register for iface state toggle events.
    iface_util::IfaceEventHandlers event_handlers = {};
    event_handlers.on_state_toggle_off_on =
        [weak_ptr_this](const std::string& /* iface_name */) {
            const auto shared_ptr_this = weak_ptr_this.promote();
            if (!shared_ptr_this.get() || !shared_ptr_this->isValid()) {
                LOG(ERROR) << "Callback invoked on an invalid object";
                return;
            }
            // Tell framework that NAN has been disabled.
            WifiNanStatus status = {
                NanStatusType::UNSUPPORTED_CONCURRENCY_NAN_DISABLED, ""};
            for (const auto& callback : shared_ptr_this->getEventCallbacks()) {
                if (!callback->eventDisabled(status).isOk()) {
                    LOG(ERROR) << "Failed to invoke the callback";
                }
            }
        };
    iface_util_.lock()->registerIfaceEventHandlers(ifname_, event_handlers);
}

void WifiNanIface::invalidate() {
    if (!isValid()) {
        return;
    }
    // send commands to HAL to actually disable and destroy interfaces
    legacy_hal_.lock()->nanDisableRequest(ifname_, 0xFFFF);
    legacy_hal_.lock()->nanDataInterfaceDelete(ifname_, 0xFFFE, "aware_data0");
    legacy_hal_.lock()->nanDataInterfaceDelete(ifname_, 0xFFFD, "aware_data1");
    iface_util_.lock()->unregisterIfaceEventHandlers(ifname_);
    legacy_hal_.reset();
    event_cb_handler_.invalidate();
    event_cb_handler_1_2_.invalidate();
    is_valid_ = false;
    if (is_dedicated_iface_) {
        // If using a dedicated iface, set the iface down.
        iface_util_.lock()->setUpState(ifname_, false);
    }
}

bool WifiNanIface::isValid() { return is_valid_; }

std::string WifiNanIface::getName() { return ifname_; }

std::set<sp<V1_0::IWifiNanIfaceEventCallback>>
WifiNanIface::getEventCallbacks() {
    return event_cb_handler_.getCallbacks();
}

std::set<sp<V1_2::IWifiNanIfaceEventCallback>>
WifiNanIface::getEventCallbacks_1_2() {
    return event_cb_handler_1_2_.getCallbacks();
}

Return<void> WifiNanIface::getName(getName_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                           &WifiNanIface::getNameInternal, hidl_status_cb);
}

Return<void> WifiNanIface::getType(getType_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                           &WifiNanIface::getTypeInternal, hidl_status_cb);
}

Return<void> WifiNanIface::registerEventCallback(
    const sp<V1_0::IWifiNanIfaceEventCallback>& callback,
    registerEventCallback_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                           &WifiNanIface::registerEventCallbackInternal,
                           hidl_status_cb, callback);
}

Return<void> WifiNanIface::getCapabilitiesRequest(
    uint16_t cmd_id, getCapabilitiesRequest_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                           &WifiNanIface::getCapabilitiesRequestInternal,
                           hidl_status_cb, cmd_id);
}

Return<void> WifiNanIface::enableRequest(uint16_t cmd_id,
                                         const V1_0::NanEnableRequest& msg,
                                         enableRequest_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                           &WifiNanIface::enableRequestInternal, hidl_status_cb,
                           cmd_id, msg);
}

Return<void> WifiNanIface::configRequest(uint16_t cmd_id,
                                         const V1_0::NanConfigRequest& msg,
                                         configRequest_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                           &WifiNanIface::configRequestInternal, hidl_status_cb,
                           cmd_id, msg);
}

Return<void> WifiNanIface::disableRequest(uint16_t cmd_id,
                                          disableRequest_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                           &WifiNanIface::disableRequestInternal,
                           hidl_status_cb, cmd_id);
}

Return<void> WifiNanIface::startPublishRequest(
    uint16_t cmd_id, const NanPublishRequest& msg,
    startPublishRequest_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                           &WifiNanIface::startPublishRequestInternal,
                           hidl_status_cb, cmd_id, msg);
}

Return<void> WifiNanIface::stopPublishRequest(
    uint16_t cmd_id, uint8_t sessionId, stopPublishRequest_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                           &WifiNanIface::stopPublishRequestInternal,
                           hidl_status_cb, cmd_id, sessionId);
}

Return<void> WifiNanIface::startSubscribeRequest(
    uint16_t cmd_id, const NanSubscribeRequest& msg,
    startSubscribeRequest_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                           &WifiNanIface::startSubscribeRequestInternal,
                           hidl_status_cb, cmd_id, msg);
}

Return<void> WifiNanIface::stopSubscribeRequest(
    uint16_t cmd_id, uint8_t sessionId,
    stopSubscribeRequest_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                           &WifiNanIface::stopSubscribeRequestInternal,
                           hidl_status_cb, cmd_id, sessionId);
}

Return<void> WifiNanIface::transmitFollowupRequest(
    uint16_t cmd_id, const NanTransmitFollowupRequest& msg,
    transmitFollowupRequest_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                           &WifiNanIface::transmitFollowupRequestInternal,
                           hidl_status_cb, cmd_id, msg);
}

Return<void> WifiNanIface::createDataInterfaceRequest(
    uint16_t cmd_id, const hidl_string& iface_name,
    createDataInterfaceRequest_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                           &WifiNanIface::createDataInterfaceRequestInternal,
                           hidl_status_cb, cmd_id, iface_name);
}

Return<void> WifiNanIface::deleteDataInterfaceRequest(
    uint16_t cmd_id, const hidl_string& iface_name,
    deleteDataInterfaceRequest_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                           &WifiNanIface::deleteDataInterfaceRequestInternal,
                           hidl_status_cb, cmd_id, iface_name);
}

Return<void> WifiNanIface::initiateDataPathRequest(
    uint16_t cmd_id, const NanInitiateDataPathRequest& msg,
    initiateDataPathRequest_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                           &WifiNanIface::initiateDataPathRequestInternal,
                           hidl_status_cb, cmd_id, msg);
}

Return<void> WifiNanIface::respondToDataPathIndicationRequest(
    uint16_t cmd_id, const NanRespondToDataPathIndicationRequest& msg,
    respondToDataPathIndicationRequest_cb hidl_status_cb) {
    return validateAndCall(
        this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
        &WifiNanIface::respondToDataPathIndicationRequestInternal,
        hidl_status_cb, cmd_id, msg);
}

Return<void> WifiNanIface::terminateDataPathRequest(
    uint16_t cmd_id, uint32_t ndpInstanceId,
    terminateDataPathRequest_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                           &WifiNanIface::terminateDataPathRequestInternal,
                           hidl_status_cb, cmd_id, ndpInstanceId);
}

Return<void> WifiNanIface::registerEventCallback_1_2(
    const sp<V1_2::IWifiNanIfaceEventCallback>& callback,
    registerEventCallback_1_2_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                           &WifiNanIface::registerEventCallback_1_2Internal,
                           hidl_status_cb, callback);
}

Return<void> WifiNanIface::enableRequest_1_2(
    uint16_t cmd_id, const V1_0::NanEnableRequest& msg1,
    const V1_2::NanConfigRequestSupplemental& msg2,
    enableRequest_1_2_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                           &WifiNanIface::enableRequest_1_2Internal,
                           hidl_status_cb, cmd_id, msg1, msg2);
}

Return<void> WifiNanIface::configRequest_1_2(
    uint16_t cmd_id, const V1_0::NanConfigRequest& msg1,
    const V1_2::NanConfigRequestSupplemental& msg2,
    configRequest_1_2_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                           &WifiNanIface::configRequest_1_2Internal,
                           hidl_status_cb, cmd_id, msg1, msg2);
}

Return<void> WifiNanIface::enableRequest_1_4(
    uint16_t cmd_id, const NanEnableRequest& msg1,
    const V1_2::NanConfigRequestSupplemental& msg2,
    enableRequest_1_4_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                           &WifiNanIface::enableRequest_1_4Internal,
                           hidl_status_cb, cmd_id, msg1, msg2);
}

Return<void> WifiNanIface::configRequest_1_4(
    uint16_t cmd_id, const NanConfigRequest& msg1,
    const V1_2::NanConfigRequestSupplemental& msg2,
    configRequest_1_4_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                           &WifiNanIface::configRequest_1_4Internal,
                           hidl_status_cb, cmd_id, msg1, msg2);
}

std::pair<WifiStatus, std::string> WifiNanIface::getNameInternal() {
    return {createWifiStatus(WifiStatusCode::SUCCESS), ifname_};
}

std::pair<WifiStatus, IfaceType> WifiNanIface::getTypeInternal() {
    return {createWifiStatus(WifiStatusCode::SUCCESS), IfaceType::NAN};
}

WifiStatus WifiNanIface::registerEventCallbackInternal(
    const sp<V1_0::IWifiNanIfaceEventCallback>& callback) {
    if (!event_cb_handler_.addCallback(callback)) {
        return createWifiStatus(WifiStatusCode::ERROR_UNKNOWN);
    }
    return createWifiStatus(WifiStatusCode::SUCCESS);
}

WifiStatus WifiNanIface::getCapabilitiesRequestInternal(uint16_t cmd_id) {
    legacy_hal::wifi_error legacy_status =
        legacy_hal_.lock()->nanGetCapabilities(ifname_, cmd_id);
    return createWifiStatusFromLegacyError(legacy_status);
}

WifiStatus WifiNanIface::enableRequestInternal(
    uint16_t /* cmd_id */, const V1_0::NanEnableRequest& /* msg */) {
    return createWifiStatus(WifiStatusCode::ERROR_NOT_SUPPORTED);
}

WifiStatus WifiNanIface::configRequestInternal(
    uint16_t /* cmd_id */, const V1_0::NanConfigRequest& /* msg */) {
    return createWifiStatus(WifiStatusCode::ERROR_NOT_SUPPORTED);
}

WifiStatus WifiNanIface::disableRequestInternal(uint16_t cmd_id) {
    legacy_hal::wifi_error legacy_status =
        legacy_hal_.lock()->nanDisableRequest(ifname_, cmd_id);
    return createWifiStatusFromLegacyError(legacy_status);
}

WifiStatus WifiNanIface::startPublishRequestInternal(
    uint16_t cmd_id, const NanPublishRequest& msg) {
    legacy_hal::NanPublishRequest legacy_msg;
    if (!hidl_struct_util::convertHidlNanPublishRequestToLegacy(msg,
                                                                &legacy_msg)) {
        return createWifiStatus(WifiStatusCode::ERROR_INVALID_ARGS);
    }
    legacy_hal::wifi_error legacy_status =
        legacy_hal_.lock()->nanPublishRequest(ifname_, cmd_id, legacy_msg);
    return createWifiStatusFromLegacyError(legacy_status);
}

WifiStatus WifiNanIface::stopPublishRequestInternal(uint16_t cmd_id,
                                                    uint8_t sessionId) {
    legacy_hal::NanPublishCancelRequest legacy_msg;
    legacy_msg.publish_id = sessionId;
    legacy_hal::wifi_error legacy_status =
        legacy_hal_.lock()->nanPublishCancelRequest(ifname_, cmd_id,
                                                    legacy_msg);
    return createWifiStatusFromLegacyError(legacy_status);
}

WifiStatus WifiNanIface::startSubscribeRequestInternal(
    uint16_t cmd_id, const NanSubscribeRequest& msg) {
    legacy_hal::NanSubscribeRequest legacy_msg;
    if (!hidl_struct_util::convertHidlNanSubscribeRequestToLegacy(
            msg, &legacy_msg)) {
        return createWifiStatus(WifiStatusCode::ERROR_INVALID_ARGS);
    }
    legacy_hal::wifi_error legacy_status =
        legacy_hal_.lock()->nanSubscribeRequest(ifname_, cmd_id, legacy_msg);
    return createWifiStatusFromLegacyError(legacy_status);
}

WifiStatus WifiNanIface::stopSubscribeRequestInternal(uint16_t cmd_id,
                                                      uint8_t sessionId) {
    legacy_hal::NanSubscribeCancelRequest legacy_msg;
    legacy_msg.subscribe_id = sessionId;
    legacy_hal::wifi_error legacy_status =
        legacy_hal_.lock()->nanSubscribeCancelRequest(ifname_, cmd_id,
                                                      legacy_msg);
    return createWifiStatusFromLegacyError(legacy_status);
}

WifiStatus WifiNanIface::transmitFollowupRequestInternal(
    uint16_t cmd_id, const NanTransmitFollowupRequest& msg) {
    legacy_hal::NanTransmitFollowupRequest legacy_msg;
    if (!hidl_struct_util::convertHidlNanTransmitFollowupRequestToLegacy(
            msg, &legacy_msg)) {
        return createWifiStatus(WifiStatusCode::ERROR_INVALID_ARGS);
    }
    legacy_hal::wifi_error legacy_status =
        legacy_hal_.lock()->nanTransmitFollowupRequest(ifname_, cmd_id,
                                                       legacy_msg);
    return createWifiStatusFromLegacyError(legacy_status);
}

WifiStatus WifiNanIface::createDataInterfaceRequestInternal(
    uint16_t cmd_id, const std::string& iface_name) {
    legacy_hal::wifi_error legacy_status =
        legacy_hal_.lock()->nanDataInterfaceCreate(ifname_, cmd_id, iface_name);
    return createWifiStatusFromLegacyError(legacy_status);
}
WifiStatus WifiNanIface::deleteDataInterfaceRequestInternal(
    uint16_t cmd_id, const std::string& iface_name) {
    legacy_hal::wifi_error legacy_status =
        legacy_hal_.lock()->nanDataInterfaceDelete(ifname_, cmd_id, iface_name);
    return createWifiStatusFromLegacyError(legacy_status);
}
WifiStatus WifiNanIface::initiateDataPathRequestInternal(
    uint16_t cmd_id, const NanInitiateDataPathRequest& msg) {
    legacy_hal::NanDataPathInitiatorRequest legacy_msg;
    if (!hidl_struct_util::convertHidlNanDataPathInitiatorRequestToLegacy(
            msg, &legacy_msg)) {
        return createWifiStatus(WifiStatusCode::ERROR_INVALID_ARGS);
    }
    legacy_hal::wifi_error legacy_status =
        legacy_hal_.lock()->nanDataRequestInitiator(ifname_, cmd_id,
                                                    legacy_msg);
    return createWifiStatusFromLegacyError(legacy_status);
}
WifiStatus WifiNanIface::respondToDataPathIndicationRequestInternal(
    uint16_t cmd_id, const NanRespondToDataPathIndicationRequest& msg) {
    legacy_hal::NanDataPathIndicationResponse legacy_msg;
    if (!hidl_struct_util::convertHidlNanDataPathIndicationResponseToLegacy(
            msg, &legacy_msg)) {
        return createWifiStatus(WifiStatusCode::ERROR_INVALID_ARGS);
    }
    legacy_hal::wifi_error legacy_status =
        legacy_hal_.lock()->nanDataIndicationResponse(ifname_, cmd_id,
                                                      legacy_msg);
    return createWifiStatusFromLegacyError(legacy_status);
}
WifiStatus WifiNanIface::terminateDataPathRequestInternal(
    uint16_t cmd_id, uint32_t ndpInstanceId) {
    legacy_hal::wifi_error legacy_status =
        legacy_hal_.lock()->nanDataEnd(ifname_, cmd_id, ndpInstanceId);
    return createWifiStatusFromLegacyError(legacy_status);
}

WifiStatus WifiNanIface::registerEventCallback_1_2Internal(
    const sp<V1_2::IWifiNanIfaceEventCallback>& callback) {
    sp<V1_0::IWifiNanIfaceEventCallback> callback_1_0 = callback;
    if (!event_cb_handler_.addCallback(callback_1_0)) {
        return createWifiStatus(WifiStatusCode::ERROR_UNKNOWN);
    }
    if (!event_cb_handler_1_2_.addCallback(callback)) {
        return createWifiStatus(WifiStatusCode::ERROR_UNKNOWN);
    }
    return createWifiStatus(WifiStatusCode::SUCCESS);
}

WifiStatus WifiNanIface::enableRequest_1_2Internal(
    uint16_t /* cmd_id */, const V1_0::NanEnableRequest& /* msg1 */,
    const V1_2::NanConfigRequestSupplemental& /*msg2 */) {
    return createWifiStatus(WifiStatusCode::ERROR_NOT_SUPPORTED);
}

WifiStatus WifiNanIface::configRequest_1_2Internal(
    uint16_t /* cmd_id */, const V1_0::NanConfigRequest& /* msg1 */,
    const V1_2::NanConfigRequestSupplemental& /* msg2 */) {
    return createWifiStatus(WifiStatusCode::ERROR_NOT_SUPPORTED);
}

WifiStatus WifiNanIface::enableRequest_1_4Internal(
    uint16_t cmd_id, const NanEnableRequest& msg1,
    const V1_2::NanConfigRequestSupplemental& msg2) {
    legacy_hal::NanEnableRequest legacy_msg;
    if (!hidl_struct_util::convertHidlNanEnableRequest_1_4ToLegacy(
            msg1, msg2, &legacy_msg)) {
        return createWifiStatus(WifiStatusCode::ERROR_INVALID_ARGS);
    }
    legacy_hal::wifi_error legacy_status =
        legacy_hal_.lock()->nanEnableRequest(ifname_, cmd_id, legacy_msg);
    return createWifiStatusFromLegacyError(legacy_status);
}

WifiStatus WifiNanIface::configRequest_1_4Internal(
    uint16_t cmd_id, const NanConfigRequest& msg1,
    const V1_2::NanConfigRequestSupplemental& msg2) {
    legacy_hal::NanConfigRequest legacy_msg;
    if (!hidl_struct_util::convertHidlNanConfigRequest_1_4ToLegacy(
            msg1, msg2, &legacy_msg)) {
        return createWifiStatus(WifiStatusCode::ERROR_INVALID_ARGS);
    }
    legacy_hal::wifi_error legacy_status =
        legacy_hal_.lock()->nanConfigRequest(ifname_, cmd_id, legacy_msg);
    return createWifiStatusFromLegacyError(legacy_status);
}

}  // namespace implementation
}  // namespace V1_4
}  // namespace wifi
}  // namespace hardware
}  // namespace android
