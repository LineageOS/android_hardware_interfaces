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

#include "wifi_nan_iface.h"

#include <android-base/logging.h>

#include "aidl_return_util.h"
#include "aidl_struct_util.h"
#include "wifi_status_util.h"

namespace aidl {
namespace android {
namespace hardware {
namespace wifi {
using aidl_return_util::validateAndCall;

WifiNanIface::WifiNanIface(const std::string& ifname, bool is_dedicated_iface,
                           const std::weak_ptr<legacy_hal::WifiLegacyHal> legacy_hal,
                           const std::weak_ptr<iface_util::WifiIfaceUtil> iface_util)
    : ifname_(ifname),
      is_dedicated_iface_(is_dedicated_iface),
      legacy_hal_(legacy_hal),
      iface_util_(iface_util),
      is_valid_(true) {}

std::shared_ptr<WifiNanIface> WifiNanIface::create(
        const std::string& ifname, bool is_dedicated_iface,
        const std::weak_ptr<legacy_hal::WifiLegacyHal> legacy_hal,
        const std::weak_ptr<iface_util::WifiIfaceUtil> iface_util) {
    std::shared_ptr<WifiNanIface> ptr = ndk::SharedRefBase::make<WifiNanIface>(
            ifname, is_dedicated_iface, legacy_hal, iface_util);
    if (is_dedicated_iface) {
        // If using a dedicated iface, set the iface up first.
        if (!iface_util.lock()->setUpState(ifname, true)) {
            // Fatal failure, invalidate the iface object.
            ptr->invalidate();
            return nullptr;
        }
    }
    std::weak_ptr<WifiNanIface> weak_ptr_this(ptr);
    ptr->setWeakPtr(weak_ptr_this);
    ptr->registerCallbackHandlers();
    return ptr;
}

void WifiNanIface::registerCallbackHandlers() {
    // Register all the callbacks here. These should be valid for the lifetime
    // of the object. Whenever the mode changes legacy HAL will remove
    // all of these callbacks.
    legacy_hal::NanCallbackHandlers callback_handlers;
    std::weak_ptr<WifiNanIface> weak_ptr_this = weak_ptr_this_;

    // Callback for response.
    callback_handlers.on_notify_response = [weak_ptr_this](legacy_hal::transaction_id id,
                                                           const legacy_hal::NanResponseMsg& msg) {
        const auto shared_ptr_this = weak_ptr_this.lock();
        if (!shared_ptr_this.get() || !shared_ptr_this->isValid()) {
            LOG(ERROR) << "Callback invoked on an invalid object";
            return;
        }
        NanStatus nanStatus;
        if (!aidl_struct_util::convertLegacyNanResponseHeaderToAidl(msg, &nanStatus)) {
            LOG(ERROR) << "Failed to convert nan response header";
            return;
        }

        switch (msg.response_type) {
            case legacy_hal::NAN_RESPONSE_ENABLED: {
                for (const auto& callback : shared_ptr_this->getEventCallbacks()) {
                    if (!callback->notifyEnableResponse(id, nanStatus).isOk()) {
                        LOG(ERROR) << "Failed to invoke the callback";
                    }
                }
                break;
            }
            case legacy_hal::NAN_RESPONSE_DISABLED: {
                for (const auto& callback : shared_ptr_this->getEventCallbacks()) {
                    if (!callback->notifyDisableResponse(id, nanStatus).isOk()) {
                        LOG(ERROR) << "Failed to invoke the callback";
                    }
                }
                break;
            }
            case legacy_hal::NAN_RESPONSE_PUBLISH: {
                for (const auto& callback : shared_ptr_this->getEventCallbacks()) {
                    if (!callback->notifyStartPublishResponse(id, nanStatus,
                                                              msg.body.publish_response.publish_id)
                                 .isOk()) {
                        LOG(ERROR) << "Failed to invoke the callback";
                    }
                }
                break;
            }
            case legacy_hal::NAN_RESPONSE_PUBLISH_CANCEL: {
                for (const auto& callback : shared_ptr_this->getEventCallbacks()) {
                    if (!callback->notifyStopPublishResponse(id, nanStatus).isOk()) {
                        LOG(ERROR) << "Failed to invoke the callback";
                    }
                }
                break;
            }
            case legacy_hal::NAN_RESPONSE_TRANSMIT_FOLLOWUP: {
                for (const auto& callback : shared_ptr_this->getEventCallbacks()) {
                    if (!callback->notifyTransmitFollowupResponse(id, nanStatus).isOk()) {
                        LOG(ERROR) << "Failed to invoke the callback";
                    }
                }
                break;
            }
            case legacy_hal::NAN_RESPONSE_SUBSCRIBE: {
                for (const auto& callback : shared_ptr_this->getEventCallbacks()) {
                    if (!callback->notifyStartSubscribeResponse(
                                         id, nanStatus, msg.body.subscribe_response.subscribe_id)
                                 .isOk()) {
                        LOG(ERROR) << "Failed to invoke the callback";
                    }
                }
                break;
            }
            case legacy_hal::NAN_RESPONSE_SUBSCRIBE_CANCEL: {
                for (const auto& callback : shared_ptr_this->getEventCallbacks()) {
                    if (!callback->notifyStopSubscribeResponse(id, nanStatus).isOk()) {
                        LOG(ERROR) << "Failed to invoke the callback";
                    }
                }
                break;
            }
            case legacy_hal::NAN_RESPONSE_CONFIG: {
                for (const auto& callback : shared_ptr_this->getEventCallbacks()) {
                    if (!callback->notifyConfigResponse(id, nanStatus).isOk()) {
                        LOG(ERROR) << "Failed to invoke the callback";
                    }
                }
                break;
            }
            case legacy_hal::NAN_GET_CAPABILITIES: {
                NanCapabilities aidl_struct;
                if (!aidl_struct_util::convertLegacyNanCapabilitiesResponseToAidl(
                            msg.body.nan_capabilities, &aidl_struct)) {
                    LOG(ERROR) << "Failed to convert nan capabilities response";
                    return;
                }
                for (const auto& callback : shared_ptr_this->getEventCallbacks()) {
                    if (!callback->notifyCapabilitiesResponse(id, nanStatus, aidl_struct).isOk()) {
                        LOG(ERROR) << "Failed to invoke the callback";
                    }
                }
                break;
            }
            case legacy_hal::NAN_DP_INTERFACE_CREATE: {
                for (const auto& callback : shared_ptr_this->getEventCallbacks()) {
                    if (!callback->notifyCreateDataInterfaceResponse(id, nanStatus).isOk()) {
                        LOG(ERROR) << "Failed to invoke the callback";
                    }
                }
                break;
            }
            case legacy_hal::NAN_DP_INTERFACE_DELETE: {
                for (const auto& callback : shared_ptr_this->getEventCallbacks()) {
                    if (!callback->notifyDeleteDataInterfaceResponse(id, nanStatus).isOk()) {
                        LOG(ERROR) << "Failed to invoke the callback";
                    }
                }
                break;
            }
            case legacy_hal::NAN_DP_INITIATOR_RESPONSE: {
                for (const auto& callback : shared_ptr_this->getEventCallbacks()) {
                    if (!callback->notifyInitiateDataPathResponse(
                                         id, nanStatus,
                                         msg.body.data_request_response.ndp_instance_id)
                                 .isOk()) {
                        LOG(ERROR) << "Failed to invoke the callback";
                    }
                }
                break;
            }
            case legacy_hal::NAN_DP_RESPONDER_RESPONSE: {
                for (const auto& callback : shared_ptr_this->getEventCallbacks()) {
                    if (!callback->notifyRespondToDataPathIndicationResponse(id, nanStatus)
                                 .isOk()) {
                        LOG(ERROR) << "Failed to invoke the callback";
                    }
                }
                break;
            }
            case legacy_hal::NAN_DP_END: {
                for (const auto& callback : shared_ptr_this->getEventCallbacks()) {
                    if (!callback->notifyTerminateDataPathResponse(id, nanStatus).isOk()) {
                        LOG(ERROR) << "Failed to invoke the callback";
                    }
                }
                break;
            }
            case legacy_hal::NAN_PAIRING_INITIATOR_RESPONSE: {
                for (const auto& callback : shared_ptr_this->getEventCallbacks()) {
                    if (!callback->notifyInitiatePairingResponse(
                                         id, nanStatus,
                                         msg.body.pairing_request_response.paring_instance_id)
                                 .isOk()) {
                        LOG(ERROR) << "Failed to invoke the callback";
                    }
                }
                break;
            }
            case legacy_hal::NAN_PAIRING_RESPONDER_RESPONSE: {
                for (const auto& callback : shared_ptr_this->getEventCallbacks()) {
                    if (!callback->notifyRespondToPairingIndicationResponse(id, nanStatus).isOk()) {
                        LOG(ERROR) << "Failed to invoke the callback";
                    }
                }
                break;
            }
            case legacy_hal::NAN_PAIRING_END: {
                for (const auto& callback : shared_ptr_this->getEventCallbacks()) {
                    if (!callback->notifyTerminatePairingResponse(id, nanStatus).isOk()) {
                        LOG(ERROR) << "Failed to invoke the callback";
                    }
                }
                break;
            }
            case legacy_hal::NAN_BOOTSTRAPPING_INITIATOR_RESPONSE: {
                for (const auto& callback : shared_ptr_this->getEventCallbacks()) {
                    if (!callback->notifyInitiateBootstrappingResponse(
                                         id, nanStatus,
                                         msg.body.bootstrapping_request_response
                                                 .bootstrapping_instance_id)
                                 .isOk()) {
                        LOG(ERROR) << "Failed to invoke the callback";
                    }
                }
                break;
            }
            case legacy_hal::NAN_BOOTSTRAPPING_RESPONDER_RESPONSE: {
                for (const auto& callback : shared_ptr_this->getEventCallbacks()) {
                    if (!callback->notifyRespondToBootstrappingIndicationResponse(id, nanStatus)
                                 .isOk()) {
                        LOG(ERROR) << "Failed to invoke the callback";
                    }
                }
                break;
            }
            case legacy_hal::NAN_SUSPEND_REQUEST_RESPONSE: {
                for (const auto& callback : shared_ptr_this->getEventCallbacks()) {
                    if (!callback->notifySuspendResponse(id, nanStatus).isOk()) {
                        LOG(ERROR) << "Failed to invoke the callback";
                    }
                }
                break;
            }
            case legacy_hal::NAN_RESUME_REQUEST_RESPONSE: {
                for (const auto& callback : shared_ptr_this->getEventCallbacks()) {
                    if (!callback->notifyResumeResponse(id, nanStatus).isOk()) {
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
                LOG(ERROR) << "Unknown or unhandled response type: " << msg.response_type;
                return;
        }
    };

    callback_handlers.on_event_disc_eng_event = [weak_ptr_this](
                                                        const legacy_hal::NanDiscEngEventInd& msg) {
        const auto shared_ptr_this = weak_ptr_this.lock();
        if (!shared_ptr_this.get() || !shared_ptr_this->isValid()) {
            LOG(ERROR) << "Callback invoked on an invalid object";
            return;
        }
        NanClusterEventInd aidl_struct;
        // event types defined identically - hence can be cast
        aidl_struct.eventType = (NanClusterEventType)msg.event_type;
        aidl_struct.addr = std::array<uint8_t, 6>();
        std::copy(msg.data.mac_addr.addr, msg.data.mac_addr.addr + 6, std::begin(aidl_struct.addr));

        for (const auto& callback : shared_ptr_this->getEventCallbacks()) {
            if (!callback->eventClusterEvent(aidl_struct).isOk()) {
                LOG(ERROR) << "Failed to invoke the callback";
            }
        }
    };

    callback_handlers.on_event_disabled = [weak_ptr_this](const legacy_hal::NanDisabledInd& msg) {
        const auto shared_ptr_this = weak_ptr_this.lock();
        if (!shared_ptr_this.get() || !shared_ptr_this->isValid()) {
            LOG(ERROR) << "Callback invoked on an invalid object";
            return;
        }
        NanStatus status;
        aidl_struct_util::convertToNanStatus(msg.reason, msg.nan_reason, sizeof(msg.nan_reason),
                                             &status);

        for (const auto& callback : shared_ptr_this->getEventCallbacks()) {
            if (!callback->eventDisabled(status).isOk()) {
                LOG(ERROR) << "Failed to invoke the callback";
            }
        }
    };

    callback_handlers.on_event_publish_terminated =
            [weak_ptr_this](const legacy_hal::NanPublishTerminatedInd& msg) {
                const auto shared_ptr_this = weak_ptr_this.lock();
                if (!shared_ptr_this.get() || !shared_ptr_this->isValid()) {
                    LOG(ERROR) << "Callback invoked on an invalid object";
                    return;
                }
                NanStatus status;
                aidl_struct_util::convertToNanStatus(msg.reason, msg.nan_reason,
                                                     sizeof(msg.nan_reason), &status);

                for (const auto& callback : shared_ptr_this->getEventCallbacks()) {
                    if (!callback->eventPublishTerminated(msg.publish_id, status).isOk()) {
                        LOG(ERROR) << "Failed to invoke the callback";
                    }
                }
            };

    callback_handlers.on_event_subscribe_terminated =
            [weak_ptr_this](const legacy_hal::NanSubscribeTerminatedInd& msg) {
                const auto shared_ptr_this = weak_ptr_this.lock();
                if (!shared_ptr_this.get() || !shared_ptr_this->isValid()) {
                    LOG(ERROR) << "Callback invoked on an invalid object";
                    return;
                }
                NanStatus status;
                aidl_struct_util::convertToNanStatus(msg.reason, msg.nan_reason,
                                                     sizeof(msg.nan_reason), &status);

                for (const auto& callback : shared_ptr_this->getEventCallbacks()) {
                    if (!callback->eventSubscribeTerminated(msg.subscribe_id, status).isOk()) {
                        LOG(ERROR) << "Failed to invoke the callback";
                    }
                }
            };

    callback_handlers.on_event_match = [weak_ptr_this](const legacy_hal::NanMatchInd& msg) {
        const auto shared_ptr_this = weak_ptr_this.lock();
        if (!shared_ptr_this.get() || !shared_ptr_this->isValid()) {
            LOG(ERROR) << "Callback invoked on an invalid object";
            return;
        }
        NanMatchInd aidl_struct;
        if (!aidl_struct_util::convertLegacyNanMatchIndToAidl(msg, &aidl_struct)) {
            LOG(ERROR) << "Failed to convert nan capabilities response";
            return;
        }

        for (const auto& callback : shared_ptr_this->getEventCallbacks()) {
            if (!callback->eventMatch(aidl_struct).isOk()) {
                LOG(ERROR) << "Failed to invoke the callback";
            }
        }
    };

    callback_handlers.on_event_match_expired = [weak_ptr_this](
                                                       const legacy_hal::NanMatchExpiredInd& msg) {
        const auto shared_ptr_this = weak_ptr_this.lock();
        if (!shared_ptr_this.get() || !shared_ptr_this->isValid()) {
            LOG(ERROR) << "Callback invoked on an invalid object";
            return;
        }
        for (const auto& callback : shared_ptr_this->getEventCallbacks()) {
            if (!callback->eventMatchExpired(msg.publish_subscribe_id, msg.requestor_instance_id)
                         .isOk()) {
                LOG(ERROR) << "Failed to invoke the callback";
            }
        }
    };

    callback_handlers.on_event_followup = [weak_ptr_this](const legacy_hal::NanFollowupInd& msg) {
        const auto shared_ptr_this = weak_ptr_this.lock();
        if (!shared_ptr_this.get() || !shared_ptr_this->isValid()) {
            LOG(ERROR) << "Callback invoked on an invalid object";
            return;
        }
        NanFollowupReceivedInd aidl_struct;
        if (!aidl_struct_util::convertLegacyNanFollowupIndToAidl(msg, &aidl_struct)) {
            LOG(ERROR) << "Failed to convert nan capabilities response";
            return;
        }

        for (const auto& callback : shared_ptr_this->getEventCallbacks()) {
            if (!callback->eventFollowupReceived(aidl_struct).isOk()) {
                LOG(ERROR) << "Failed to invoke the callback";
            }
        }
    };

    callback_handlers.on_event_transmit_follow_up =
            [weak_ptr_this](const legacy_hal::NanTransmitFollowupInd& msg) {
                const auto shared_ptr_this = weak_ptr_this.lock();
                if (!shared_ptr_this.get() || !shared_ptr_this->isValid()) {
                    LOG(ERROR) << "Callback invoked on an invalid object";
                    return;
                }
                NanStatus status;
                aidl_struct_util::convertToNanStatus(msg.reason, msg.nan_reason,
                                                     sizeof(msg.nan_reason), &status);

                for (const auto& callback : shared_ptr_this->getEventCallbacks()) {
                    if (!callback->eventTransmitFollowup(msg.id, status).isOk()) {
                        LOG(ERROR) << "Failed to invoke the callback";
                    }
                }
            };

    callback_handlers.on_event_data_path_request =
            [weak_ptr_this](const legacy_hal::NanDataPathRequestInd& msg) {
                const auto shared_ptr_this = weak_ptr_this.lock();
                if (!shared_ptr_this.get() || !shared_ptr_this->isValid()) {
                    LOG(ERROR) << "Callback invoked on an invalid object";
                    return;
                }
                NanDataPathRequestInd aidl_struct;
                if (!aidl_struct_util::convertLegacyNanDataPathRequestIndToAidl(msg,
                                                                                &aidl_struct)) {
                    LOG(ERROR) << "Failed to convert nan capabilities response";
                    return;
                }

                for (const auto& callback : shared_ptr_this->getEventCallbacks()) {
                    if (!callback->eventDataPathRequest(aidl_struct).isOk()) {
                        LOG(ERROR) << "Failed to invoke the callback";
                    }
                }
            };

    callback_handlers.on_event_data_path_confirm =
            [weak_ptr_this](const legacy_hal::NanDataPathConfirmInd& msg) {
                const auto shared_ptr_this = weak_ptr_this.lock();
                if (!shared_ptr_this.get() || !shared_ptr_this->isValid()) {
                    LOG(ERROR) << "Callback invoked on an invalid object";
                    return;
                }
                NanDataPathConfirmInd aidl_struct;
                if (!aidl_struct_util::convertLegacyNanDataPathConfirmIndToAidl(msg,
                                                                                &aidl_struct)) {
                    LOG(ERROR) << "Failed to convert nan capabilities response";
                    return;
                }

                for (const auto& callback : shared_ptr_this->getEventCallbacks()) {
                    if (!callback->eventDataPathConfirm(aidl_struct).isOk()) {
                        LOG(ERROR) << "Failed to invoke the callback";
                    }
                }
            };

    callback_handlers.on_event_data_path_end =
            [weak_ptr_this](const legacy_hal::NanDataPathEndInd& msg) {
                const auto shared_ptr_this = weak_ptr_this.lock();
                if (!shared_ptr_this.get() || !shared_ptr_this->isValid()) {
                    LOG(ERROR) << "Callback invoked on an invalid object";
                    return;
                }
                for (const auto& callback : shared_ptr_this->getEventCallbacks()) {
                    for (int i = 0; i < msg.num_ndp_instances; ++i) {
                        if (!callback->eventDataPathTerminated(msg.ndp_instance_id[i]).isOk()) {
                            LOG(ERROR) << "Failed to invoke the callback";
                        }
                    }
                }
            };

    callback_handlers.on_event_pairing_request =
            [weak_ptr_this](const legacy_hal::NanPairingRequestInd& msg) {
                const auto shared_ptr_this = weak_ptr_this.lock();
                if (!shared_ptr_this.get() || !shared_ptr_this->isValid()) {
                    LOG(ERROR) << "Callback invoked on an invalid object";
                    return;
                }
                NanPairingRequestInd aidl_struct;
                if (!aidl_struct_util::convertLegacyNanPairingRequestIndToAidl(msg, &aidl_struct)) {
                    LOG(ERROR) << "Failed to convert nan capabilities response";
                    return;
                }

                for (const auto& callback : shared_ptr_this->getEventCallbacks()) {
                    if (!callback->eventPairingRequest(aidl_struct).isOk()) {
                        LOG(ERROR) << "Failed to invoke the callback";
                    }
                }
            };
    callback_handlers.on_event_pairing_confirm =
            [weak_ptr_this](const legacy_hal::NanPairingConfirmInd& msg) {
                const auto shared_ptr_this = weak_ptr_this.lock();
                if (!shared_ptr_this.get() || !shared_ptr_this->isValid()) {
                    LOG(ERROR) << "Callback invoked on an invalid object";
                    return;
                }
                NanPairingConfirmInd aidl_struct;
                if (!aidl_struct_util::convertLegacyNanPairingConfirmIndToAidl(msg, &aidl_struct)) {
                    LOG(ERROR) << "Failed to convert nan capabilities response";
                    return;
                }

                for (const auto& callback : shared_ptr_this->getEventCallbacks()) {
                    if (!callback->eventPairingConfirm(aidl_struct).isOk()) {
                        LOG(ERROR) << "Failed to invoke the callback";
                    }
                }
            };
    callback_handlers.on_event_bootstrapping_request =
            [weak_ptr_this](const legacy_hal::NanBootstrappingRequestInd& msg) {
                const auto shared_ptr_this = weak_ptr_this.lock();
                if (!shared_ptr_this.get() || !shared_ptr_this->isValid()) {
                    LOG(ERROR) << "Callback invoked on an invalid object";
                    return;
                }
                NanBootstrappingRequestInd aidl_struct;
                if (!aidl_struct_util::convertLegacyNanBootstrappingRequestIndToAidl(
                            msg, &aidl_struct)) {
                    LOG(ERROR) << "Failed to convert nan capabilities response";
                    return;
                }

                for (const auto& callback : shared_ptr_this->getEventCallbacks()) {
                    if (!callback->eventBootstrappingRequest(aidl_struct).isOk()) {
                        LOG(ERROR) << "Failed to invoke the callback";
                    }
                }
            };
    callback_handlers.on_event_bootstrapping_confirm =
            [weak_ptr_this](const legacy_hal::NanBootstrappingConfirmInd& msg) {
                const auto shared_ptr_this = weak_ptr_this.lock();
                if (!shared_ptr_this.get() || !shared_ptr_this->isValid()) {
                    LOG(ERROR) << "Callback invoked on an invalid object";
                    return;
                }
                NanBootstrappingConfirmInd aidl_struct;
                if (!aidl_struct_util::convertLegacyNanBootstrappingConfirmIndToAidl(
                            msg, &aidl_struct)) {
                    LOG(ERROR) << "Failed to convert nan capabilities response";
                    return;
                }

                for (const auto& callback : shared_ptr_this->getEventCallbacks()) {
                    if (!callback->eventBootstrappingConfirm(aidl_struct).isOk()) {
                        LOG(ERROR) << "Failed to invoke the callback";
                    }
                }
            };

    callback_handlers.on_event_beacon_sdf_payload =
            [](const legacy_hal::NanBeaconSdfPayloadInd& /* msg */) {
                LOG(ERROR) << "on_event_beacon_sdf_payload - should not be called";
            };

    callback_handlers.on_event_range_request = [](const legacy_hal::NanRangeRequestInd& /* msg */) {
        LOG(ERROR) << "on_event_range_request - should not be called";
    };

    callback_handlers.on_event_range_report = [](const legacy_hal::NanRangeReportInd& /* msg */) {
        LOG(ERROR) << "on_event_range_report - should not be called";
    };

    callback_handlers.on_event_schedule_update =
            [weak_ptr_this](const legacy_hal::NanDataPathScheduleUpdateInd& msg) {
                const auto shared_ptr_this = weak_ptr_this.lock();
                if (!shared_ptr_this.get() || !shared_ptr_this->isValid()) {
                    LOG(ERROR) << "Callback invoked on an invalid object";
                    return;
                }
                NanDataPathScheduleUpdateInd aidl_struct;
                if (!aidl_struct_util::convertLegacyNanDataPathScheduleUpdateIndToAidl(
                            msg, &aidl_struct)) {
                    LOG(ERROR) << "Failed to convert nan capabilities response";
                    return;
                }

                for (const auto& callback : shared_ptr_this->getEventCallbacks()) {
                    if (!callback->eventDataPathScheduleUpdate(aidl_struct).isOk()) {
                        LOG(ERROR) << "Failed to invoke the callback";
                    }
                }
            };
    callback_handlers.on_event_suspension_mode_change =
            [weak_ptr_this](const legacy_hal::NanSuspensionModeChangeInd& msg) {
                const auto shared_ptr_this = weak_ptr_this.lock();
                if (!shared_ptr_this.get() || !shared_ptr_this->isValid()) {
                    LOG(ERROR) << "Callback invoked on an invalid object";
                    return;
                }
                NanSuspensionModeChangeInd aidl_struct;
                aidl_struct.isSuspended = msg.is_suspended;

                for (const auto& callback : shared_ptr_this->getEventCallbacks()) {
                    if (!callback->eventSuspensionModeChanged(aidl_struct).isOk()) {
                        LOG(ERROR) << "Failed to invoke the callback";
                    }
                }
            };

    legacy_hal::wifi_error legacy_status =
            legacy_hal_.lock()->nanRegisterCallbackHandlers(ifname_, callback_handlers);
    if (legacy_status != legacy_hal::WIFI_SUCCESS) {
        LOG(ERROR) << "Failed to register nan callbacks. Invalidating object";
        invalidate();
    }

    // Register for iface state toggle events.
    iface_util::IfaceEventHandlers event_handlers = {};
#ifndef WIFI_SKIP_STATE_TOGGLE_OFF_ON_FOR_NAN
    event_handlers.on_state_toggle_off_on = [weak_ptr_this](const std::string& /* iface_name */) {
        const auto shared_ptr_this = weak_ptr_this.lock();
        if (!shared_ptr_this.get() || !shared_ptr_this->isValid()) {
            LOG(ERROR) << "Callback invoked on an invalid object";
            return;
        }
        // Tell framework that NAN has been disabled.
        NanStatus status = {NanStatusCode::UNSUPPORTED_CONCURRENCY_NAN_DISABLED, ""};
        for (const auto& callback : shared_ptr_this->getEventCallbacks()) {
            if (!callback->eventDisabled(status).isOk()) {
                LOG(ERROR) << "Failed to invoke the callback";
            }
        }
    };
#endif
    iface_util_.lock()->registerIfaceEventHandlers(ifname_, event_handlers);
}

void WifiNanIface::setWeakPtr(std::weak_ptr<WifiNanIface> ptr) {
    weak_ptr_this_ = ptr;
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
    is_valid_ = false;
    if (is_dedicated_iface_) {
        // If using a dedicated iface, set the iface down.
        iface_util_.lock()->setUpState(ifname_, false);
    }
}

bool WifiNanIface::isValid() {
    return is_valid_;
}

std::string WifiNanIface::getName() {
    return ifname_;
}

std::set<std::shared_ptr<IWifiNanIfaceEventCallback>> WifiNanIface::getEventCallbacks() {
    LOG(ERROR) << "Using original getEventCallbacks";
    return event_cb_handler_.getCallbacks();
}

ndk::ScopedAStatus WifiNanIface::getName(std::string* _aidl_return) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                           &WifiNanIface::getNameInternal, _aidl_return);
}

ndk::ScopedAStatus WifiNanIface::registerEventCallback(
        const std::shared_ptr<IWifiNanIfaceEventCallback>& callback) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                           &WifiNanIface::registerEventCallbackInternal, callback);
}

ndk::ScopedAStatus WifiNanIface::getCapabilitiesRequest(char16_t in_cmdId) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                           &WifiNanIface::getCapabilitiesRequestInternal, in_cmdId);
}

ndk::ScopedAStatus WifiNanIface::enableRequest(char16_t in_cmdId, const NanEnableRequest& in_msg1,
                                               const NanConfigRequestSupplemental& in_msg2) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                           &WifiNanIface::enableRequestInternal, in_cmdId, in_msg1, in_msg2);
}

ndk::ScopedAStatus WifiNanIface::configRequest(char16_t in_cmdId, const NanConfigRequest& in_msg1,
                                               const NanConfigRequestSupplemental& in_msg2) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                           &WifiNanIface::configRequestInternal, in_cmdId, in_msg1, in_msg2);
}

ndk::ScopedAStatus WifiNanIface::disableRequest(char16_t in_cmdId) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                           &WifiNanIface::disableRequestInternal, in_cmdId);
}

ndk::ScopedAStatus WifiNanIface::startPublishRequest(char16_t in_cmdId,
                                                     const NanPublishRequest& in_msg) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                           &WifiNanIface::startPublishRequestInternal, in_cmdId, in_msg);
}

ndk::ScopedAStatus WifiNanIface::stopPublishRequest(char16_t in_cmdId, int8_t in_sessionId) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                           &WifiNanIface::stopPublishRequestInternal, in_cmdId, in_sessionId);
}

ndk::ScopedAStatus WifiNanIface::startSubscribeRequest(char16_t in_cmdId,
                                                       const NanSubscribeRequest& in_msg) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                           &WifiNanIface::startSubscribeRequestInternal, in_cmdId, in_msg);
}

ndk::ScopedAStatus WifiNanIface::stopSubscribeRequest(char16_t in_cmdId, int8_t in_sessionId) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                           &WifiNanIface::stopSubscribeRequestInternal, in_cmdId, in_sessionId);
}

ndk::ScopedAStatus WifiNanIface::transmitFollowupRequest(char16_t in_cmdId,
                                                         const NanTransmitFollowupRequest& in_msg) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                           &WifiNanIface::transmitFollowupRequestInternal, in_cmdId, in_msg);
}

ndk::ScopedAStatus WifiNanIface::createDataInterfaceRequest(char16_t in_cmdId,
                                                            const std::string& in_ifaceName) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                           &WifiNanIface::createDataInterfaceRequestInternal, in_cmdId,
                           in_ifaceName);
}

ndk::ScopedAStatus WifiNanIface::deleteDataInterfaceRequest(char16_t in_cmdId,
                                                            const std::string& in_ifaceName) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                           &WifiNanIface::deleteDataInterfaceRequestInternal, in_cmdId,
                           in_ifaceName);
}

ndk::ScopedAStatus WifiNanIface::initiateDataPathRequest(char16_t in_cmdId,
                                                         const NanInitiateDataPathRequest& in_msg) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                           &WifiNanIface::initiateDataPathRequestInternal, in_cmdId, in_msg);
}

ndk::ScopedAStatus WifiNanIface::respondToDataPathIndicationRequest(
        char16_t in_cmdId, const NanRespondToDataPathIndicationRequest& in_msg) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                           &WifiNanIface::respondToDataPathIndicationRequestInternal, in_cmdId,
                           in_msg);
}

ndk::ScopedAStatus WifiNanIface::terminateDataPathRequest(char16_t in_cmdId,
                                                          int32_t in_ndpInstanceId) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                           &WifiNanIface::terminateDataPathRequestInternal, in_cmdId,
                           in_ndpInstanceId);
}

ndk::ScopedAStatus WifiNanIface::initiatePairingRequest(char16_t in_cmdId,
                                                        const NanPairingRequest& in_msg) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                           &WifiNanIface::initiatePairingRequestInternal, in_cmdId, in_msg);
}

ndk::ScopedAStatus WifiNanIface::respondToPairingIndicationRequest(
        char16_t in_cmdId, const NanRespondToPairingIndicationRequest& in_msg) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                           &WifiNanIface::respondToPairingIndicationRequestInternal, in_cmdId,
                           in_msg);
}

ndk::ScopedAStatus WifiNanIface::terminatePairingRequest(char16_t in_cmdId,
                                                         int32_t in_ndpInstanceId) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                           &WifiNanIface::terminatePairingRequestInternal, in_cmdId,
                           in_ndpInstanceId);
}

ndk::ScopedAStatus WifiNanIface::initiateBootstrappingRequest(
        char16_t in_cmdId, const NanBootstrappingRequest& in_msg) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                           &WifiNanIface::initiateBootstrappingRequestInternal, in_cmdId, in_msg);
}

ndk::ScopedAStatus WifiNanIface::respondToBootstrappingIndicationRequest(
        char16_t in_cmdId, const NanBootstrappingResponse& in_msg) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                           &WifiNanIface::respondToBootstrappingIndicationRequestInternal, in_cmdId,
                           in_msg);
}

ndk::ScopedAStatus WifiNanIface::suspendRequest(char16_t in_cmdId, int8_t in_sessionId) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                           &WifiNanIface::suspendRequestInternal, in_cmdId, in_sessionId);
}

ndk::ScopedAStatus WifiNanIface::resumeRequest(char16_t in_cmdId, int8_t in_sessionId) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                           &WifiNanIface::resumeRequestInternal, in_cmdId, in_sessionId);
}

std::pair<std::string, ndk::ScopedAStatus> WifiNanIface::getNameInternal() {
    return {ifname_, ndk::ScopedAStatus::ok()};
}

ndk::ScopedAStatus WifiNanIface::registerEventCallbackInternal(
        const std::shared_ptr<IWifiNanIfaceEventCallback>& callback) {
    if (!event_cb_handler_.addCallback(callback)) {
        return createWifiStatus(WifiStatusCode::ERROR_UNKNOWN);
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus WifiNanIface::getCapabilitiesRequestInternal(char16_t cmd_id) {
    legacy_hal::wifi_error legacy_status = legacy_hal_.lock()->nanGetCapabilities(ifname_, cmd_id);
    return createWifiStatusFromLegacyError(legacy_status);
}

ndk::ScopedAStatus WifiNanIface::enableRequestInternal(char16_t cmd_id,
                                                       const NanEnableRequest& msg1,
                                                       const NanConfigRequestSupplemental& msg2) {
    legacy_hal::NanEnableRequest legacy_msg;
    if (!aidl_struct_util::convertAidlNanEnableRequestToLegacy(msg1, msg2, &legacy_msg)) {
        return createWifiStatus(WifiStatusCode::ERROR_INVALID_ARGS);
    }
    legacy_hal::wifi_error legacy_status =
            legacy_hal_.lock()->nanEnableRequest(ifname_, cmd_id, legacy_msg);
    return createWifiStatusFromLegacyError(legacy_status);
}

ndk::ScopedAStatus WifiNanIface::configRequestInternal(char16_t cmd_id,
                                                       const NanConfigRequest& msg1,
                                                       const NanConfigRequestSupplemental& msg2) {
    legacy_hal::NanConfigRequest legacy_msg;
    if (!aidl_struct_util::convertAidlNanConfigRequestToLegacy(msg1, msg2, &legacy_msg)) {
        return createWifiStatus(WifiStatusCode::ERROR_INVALID_ARGS);
    }
    legacy_hal::wifi_error legacy_status =
            legacy_hal_.lock()->nanConfigRequest(ifname_, cmd_id, legacy_msg);
    return createWifiStatusFromLegacyError(legacy_status);
}

ndk::ScopedAStatus WifiNanIface::disableRequestInternal(char16_t cmd_id) {
    legacy_hal::wifi_error legacy_status = legacy_hal_.lock()->nanDisableRequest(ifname_, cmd_id);
    return createWifiStatusFromLegacyError(legacy_status);
}

ndk::ScopedAStatus WifiNanIface::startPublishRequestInternal(char16_t cmd_id,
                                                             const NanPublishRequest& msg) {
    legacy_hal::NanPublishRequest legacy_msg;
    if (!aidl_struct_util::convertAidlNanPublishRequestToLegacy(msg, &legacy_msg)) {
        return createWifiStatus(WifiStatusCode::ERROR_INVALID_ARGS);
    }
    legacy_hal::wifi_error legacy_status =
            legacy_hal_.lock()->nanPublishRequest(ifname_, cmd_id, legacy_msg);
    return createWifiStatusFromLegacyError(legacy_status);
}

ndk::ScopedAStatus WifiNanIface::stopPublishRequestInternal(char16_t cmd_id, int8_t sessionId) {
    legacy_hal::NanPublishCancelRequest legacy_msg;
    legacy_msg.publish_id = sessionId;
    legacy_hal::wifi_error legacy_status =
            legacy_hal_.lock()->nanPublishCancelRequest(ifname_, cmd_id, legacy_msg);
    return createWifiStatusFromLegacyError(legacy_status);
}

ndk::ScopedAStatus WifiNanIface::startSubscribeRequestInternal(char16_t cmd_id,
                                                               const NanSubscribeRequest& msg) {
    legacy_hal::NanSubscribeRequest legacy_msg;
    if (!aidl_struct_util::convertAidlNanSubscribeRequestToLegacy(msg, &legacy_msg)) {
        return createWifiStatus(WifiStatusCode::ERROR_INVALID_ARGS);
    }
    legacy_hal::wifi_error legacy_status =
            legacy_hal_.lock()->nanSubscribeRequest(ifname_, cmd_id, legacy_msg);
    return createWifiStatusFromLegacyError(legacy_status);
}

ndk::ScopedAStatus WifiNanIface::stopSubscribeRequestInternal(char16_t cmd_id, int8_t sessionId) {
    legacy_hal::NanSubscribeCancelRequest legacy_msg;
    legacy_msg.subscribe_id = sessionId;
    legacy_hal::wifi_error legacy_status =
            legacy_hal_.lock()->nanSubscribeCancelRequest(ifname_, cmd_id, legacy_msg);
    return createWifiStatusFromLegacyError(legacy_status);
}

ndk::ScopedAStatus WifiNanIface::transmitFollowupRequestInternal(
        char16_t cmd_id, const NanTransmitFollowupRequest& msg) {
    legacy_hal::NanTransmitFollowupRequest legacy_msg;
    if (!aidl_struct_util::convertAidlNanTransmitFollowupRequestToLegacy(msg, &legacy_msg)) {
        return createWifiStatus(WifiStatusCode::ERROR_INVALID_ARGS);
    }
    legacy_hal::wifi_error legacy_status =
            legacy_hal_.lock()->nanTransmitFollowupRequest(ifname_, cmd_id, legacy_msg);
    return createWifiStatusFromLegacyError(legacy_status);
}

ndk::ScopedAStatus WifiNanIface::createDataInterfaceRequestInternal(char16_t cmd_id,
                                                                    const std::string& iface_name) {
    legacy_hal::wifi_error legacy_status =
            legacy_hal_.lock()->nanDataInterfaceCreate(ifname_, cmd_id, iface_name);
    return createWifiStatusFromLegacyError(legacy_status);
}
ndk::ScopedAStatus WifiNanIface::deleteDataInterfaceRequestInternal(char16_t cmd_id,
                                                                    const std::string& iface_name) {
    legacy_hal::wifi_error legacy_status =
            legacy_hal_.lock()->nanDataInterfaceDelete(ifname_, cmd_id, iface_name);
    return createWifiStatusFromLegacyError(legacy_status);
}
ndk::ScopedAStatus WifiNanIface::initiateDataPathRequestInternal(
        char16_t cmd_id, const NanInitiateDataPathRequest& msg) {
    legacy_hal::NanDataPathInitiatorRequest legacy_msg;
    if (!aidl_struct_util::convertAidlNanDataPathInitiatorRequestToLegacy(msg, &legacy_msg)) {
        return createWifiStatus(WifiStatusCode::ERROR_INVALID_ARGS);
    }
    legacy_hal::wifi_error legacy_status =
            legacy_hal_.lock()->nanDataRequestInitiator(ifname_, cmd_id, legacy_msg);
    return createWifiStatusFromLegacyError(legacy_status);
}
ndk::ScopedAStatus WifiNanIface::respondToDataPathIndicationRequestInternal(
        char16_t cmd_id, const NanRespondToDataPathIndicationRequest& msg) {
    legacy_hal::NanDataPathIndicationResponse legacy_msg;
    if (!aidl_struct_util::convertAidlNanDataPathIndicationResponseToLegacy(msg, &legacy_msg)) {
        return createWifiStatus(WifiStatusCode::ERROR_INVALID_ARGS);
    }
    legacy_hal::wifi_error legacy_status =
            legacy_hal_.lock()->nanDataIndicationResponse(ifname_, cmd_id, legacy_msg);
    return createWifiStatusFromLegacyError(legacy_status);
}
ndk::ScopedAStatus WifiNanIface::terminateDataPathRequestInternal(char16_t cmd_id,
                                                                  int32_t ndpInstanceId) {
    legacy_hal::wifi_error legacy_status =
            legacy_hal_.lock()->nanDataEnd(ifname_, cmd_id, ndpInstanceId);
    return createWifiStatusFromLegacyError(legacy_status);
}
ndk::ScopedAStatus WifiNanIface::initiatePairingRequestInternal(char16_t cmd_id,
                                                                const NanPairingRequest& msg) {
    legacy_hal::NanPairingRequest legacy_msg;
    if (!aidl_struct_util::convertAidlNanPairingInitiatorRequestToLegacy(msg, &legacy_msg)) {
        return createWifiStatus(WifiStatusCode::ERROR_INVALID_ARGS);
    }
    legacy_hal::wifi_error legacy_status =
            legacy_hal_.lock()->nanPairingRequest(ifname_, cmd_id, legacy_msg);
    return createWifiStatusFromLegacyError(legacy_status);
}
ndk::ScopedAStatus WifiNanIface::respondToPairingIndicationRequestInternal(
        char16_t cmd_id, const NanRespondToPairingIndicationRequest& msg) {
    legacy_hal::NanPairingIndicationResponse legacy_msg;
    if (!aidl_struct_util::convertAidlNanPairingIndicationResponseToLegacy(msg, &legacy_msg)) {
        return createWifiStatus(WifiStatusCode::ERROR_INVALID_ARGS);
    }
    legacy_hal::wifi_error legacy_status =
            legacy_hal_.lock()->nanPairingIndicationResponse(ifname_, cmd_id, legacy_msg);
    return createWifiStatusFromLegacyError(legacy_status);
}
ndk::ScopedAStatus WifiNanIface::terminatePairingRequestInternal(char16_t cmd_id,
                                                                 int32_t ndpInstanceId) {
    legacy_hal::wifi_error legacy_status =
            legacy_hal_.lock()->nanPairingEnd(ifname_, cmd_id, ndpInstanceId);
    return createWifiStatusFromLegacyError(legacy_status);
}
ndk::ScopedAStatus WifiNanIface::initiateBootstrappingRequestInternal(
        char16_t cmd_id, const NanBootstrappingRequest& msg) {
    legacy_hal::NanBootstrappingRequest legacy_msg;
    if (!aidl_struct_util::convertAidlNanBootstrappingInitiatorRequestToLegacy(msg, &legacy_msg)) {
        return createWifiStatus(WifiStatusCode::ERROR_INVALID_ARGS);
    }
    legacy_hal::wifi_error legacy_status =
            legacy_hal_.lock()->nanBootstrappingRequest(ifname_, cmd_id, legacy_msg);
    return createWifiStatusFromLegacyError(legacy_status);
}
ndk::ScopedAStatus WifiNanIface::respondToBootstrappingIndicationRequestInternal(
        char16_t cmd_id, const NanBootstrappingResponse& msg) {
    legacy_hal::NanBootstrappingIndicationResponse legacy_msg;
    if (!aidl_struct_util::convertAidlNanBootstrappingIndicationResponseToLegacy(msg,
                                                                                 &legacy_msg)) {
        return createWifiStatus(WifiStatusCode::ERROR_INVALID_ARGS);
    }
    legacy_hal::wifi_error legacy_status =
            legacy_hal_.lock()->nanBootstrappingIndicationResponse(ifname_, cmd_id, legacy_msg);
    return createWifiStatusFromLegacyError(legacy_status);
}
ndk::ScopedAStatus WifiNanIface::suspendRequestInternal(char16_t cmd_id, int8_t sessionId) {
    legacy_hal::NanSuspendRequest legacy_msg;
    legacy_msg.publish_subscribe_id = sessionId;
    legacy_hal::wifi_error legacy_status =
            legacy_hal_.lock()->nanSuspendRequest(ifname_, cmd_id, legacy_msg);
    return createWifiStatusFromLegacyError(legacy_status);
}
ndk::ScopedAStatus WifiNanIface::resumeRequestInternal(char16_t cmd_id, int8_t sessionId) {
    legacy_hal::NanResumeRequest legacy_msg;
    legacy_msg.publish_subscribe_id = sessionId;
    legacy_hal::wifi_error legacy_status =
            legacy_hal_.lock()->nanResumeRequest(ifname_, cmd_id, legacy_msg);
    return createWifiStatusFromLegacyError(legacy_status);
}
}  // namespace wifi
}  // namespace hardware
}  // namespace android
}  // namespace aidl
