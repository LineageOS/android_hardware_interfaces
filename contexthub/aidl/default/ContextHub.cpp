/*
 * Copyright (C) 2021 The Android Open Source Project
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

#include "contexthub-impl/ContextHub.h"

#ifndef LOG_TAG
#define LOG_TAG "CHRE"
#endif

#include <inttypes.h>
#include <log/log.h>

using ::ndk::ScopedAStatus;

namespace aidl::android::hardware::contexthub {

namespace {

constexpr uint64_t kMockVendorHubId = 0x1234567812345678;
constexpr uint64_t kMockVendorHub2Id = 0x0EADBEEFDEADBEEF;

// Mock endpoints for the default implementation.
// These endpoints just echo back any messages sent to them.
constexpr size_t kMockEndpointCount = 4;
const EndpointInfo kMockEndpointInfos[kMockEndpointCount] = {
        {
                .id = {.hubId = kMockVendorHubId, .id = UINT64_C(0x1)},
                .type = EndpointInfo::EndpointType::GENERIC,
                .name = "Mock Endpoint 1",
                .version = 1,
        },
        {
                .id = {.hubId = kMockVendorHubId, .id = UINT64_C(0x2)},
                .type = EndpointInfo::EndpointType::GENERIC,
                .name = "Mock Endpoint 2",
                .version = 2,
        },
        {
                .id = {.hubId = kMockVendorHub2Id, .id = UINT64_C(0x1)},
                .type = EndpointInfo::EndpointType::GENERIC,
                .name = "Mock Endpoint 3",
                .version = 1,
        },
        {
                .id = {.hubId = kMockVendorHub2Id, .id = UINT64_C(0x2)},
                .type = EndpointInfo::EndpointType::GENERIC,
                .name = "Mock Endpoint 4",
                .version = 2,
        },
};

}  // anonymous namespace

ScopedAStatus ContextHub::getContextHubs(std::vector<ContextHubInfo>* out_contextHubInfos) {
    ContextHubInfo hub = {};
    hub.name = "Mock Context Hub";
    hub.vendor = "AOSP";
    hub.toolchain = "n/a";
    hub.id = kMockHubId;
    hub.peakMips = 1;
    hub.maxSupportedMessageLengthBytes = 4096;
    hub.chrePlatformId = UINT64_C(0x476f6f6754000000);
    hub.chreApiMajorVersion = 1;
    hub.chreApiMinorVersion = 6;
    hub.supportsReliableMessages = false;

    out_contextHubInfos->push_back(hub);

    return ScopedAStatus::ok();
}

// We don't expose any nanoapps for the default impl, therefore all nanoapp-related APIs fail.
ScopedAStatus ContextHub::loadNanoapp(int32_t /* in_contextHubId */,
                                      const NanoappBinary& /* in_appBinary */,
                                      int32_t /* in_transactionId */) {
    return ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ScopedAStatus ContextHub::unloadNanoapp(int32_t /* in_contextHubId */, int64_t /* in_appId */,
                                        int32_t /* in_transactionId */) {
    return ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ScopedAStatus ContextHub::disableNanoapp(int32_t /* in_contextHubId */, int64_t /* in_appId */,
                                         int32_t /* in_transactionId */) {
    return ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ScopedAStatus ContextHub::enableNanoapp(int32_t /* in_contextHubId */, int64_t /* in_appId */,
                                        int32_t /* in_transactionId */) {
    return ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ScopedAStatus ContextHub::onSettingChanged(Setting /* in_setting */, bool /*in_enabled */) {
    return ScopedAStatus::ok();
}

ScopedAStatus ContextHub::queryNanoapps(int32_t in_contextHubId) {
    if (in_contextHubId == kMockHubId && mCallback != nullptr) {
        std::vector<NanoappInfo> nanoapps;
        mCallback->handleNanoappInfo(nanoapps);
        return ScopedAStatus::ok();
    } else {
        return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
}

ScopedAStatus ContextHub::getPreloadedNanoappIds(int32_t /* in_contextHubId */,
                                                 std::vector<int64_t>* out_preloadedNanoappIds) {
    if (out_preloadedNanoappIds == nullptr) {
        return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }

    for (uint64_t i = 0; i < 10; ++i) {
        out_preloadedNanoappIds->push_back(i);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus ContextHub::onNanSessionStateChanged(const NanSessionStateUpdate& /*in_update*/) {
    return ScopedAStatus::ok();
}

ScopedAStatus ContextHub::registerCallback(int32_t in_contextHubId,
                                           const std::shared_ptr<IContextHubCallback>& in_cb) {
    if (in_contextHubId == kMockHubId) {
        mCallback = in_cb;
        return ScopedAStatus::ok();
    } else {
        return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
}

ScopedAStatus ContextHub::sendMessageToHub(int32_t in_contextHubId,
                                           const ContextHubMessage& /* in_message */) {
    if (in_contextHubId == kMockHubId) {
        // Return true here to indicate that the HAL has accepted the message.
        // Successful delivery of the message to a nanoapp should be handled at
        // a higher level protocol.
        return ScopedAStatus::ok();
    } else {
        return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
}

ScopedAStatus ContextHub::setTestMode(bool enable) {
    if (enable) {
        std::unique_lock<std::mutex> lock(mEndpointMutex);
        mEndpoints.clear();
        mEndpointSessions.clear();
        mEndpointCallback = nullptr;
    }
    return ScopedAStatus::ok();
}

ScopedAStatus ContextHub::onHostEndpointConnected(const HostEndpointInfo& in_info) {
    mConnectedHostEndpoints.insert(in_info.hostEndpointId);

    return ScopedAStatus::ok();
}

ScopedAStatus ContextHub::onHostEndpointDisconnected(char16_t in_hostEndpointId) {
    if (mConnectedHostEndpoints.count(in_hostEndpointId) > 0) {
        mConnectedHostEndpoints.erase(in_hostEndpointId);
    }

    return ScopedAStatus::ok();
}

ScopedAStatus ContextHub::sendMessageDeliveryStatusToHub(
        int32_t /* in_contextHubId */,
        const MessageDeliveryStatus& /* in_messageDeliveryStatus */) {
    return ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ScopedAStatus ContextHub::getHubs(std::vector<HubInfo>* _aidl_return) {
    if (_aidl_return == nullptr) {
        return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }

    ContextHubInfo hub = {};
    hub.name = "Mock Context Hub";
    hub.vendor = "AOSP";
    hub.toolchain = "n/a";
    hub.id = kMockHubId;
    hub.peakMips = 1;
    hub.maxSupportedMessageLengthBytes = 4096;
    hub.chrePlatformId = UINT64_C(0x476f6f6754000000);
    hub.chreApiMajorVersion = 1;
    hub.chreApiMinorVersion = 6;
    hub.supportsReliableMessages = false;

    HubInfo hubInfo1 = {};
    hubInfo1.hubId = hub.chrePlatformId;
    hubInfo1.hubDetails = HubInfo::HubDetails::make<HubInfo::HubDetails::Tag::contextHubInfo>(hub);

    VendorHubInfo vendorHub = {};
    vendorHub.name = "Mock Vendor Hub";
    vendorHub.version = 42;

    HubInfo hubInfo2 = {};
    hubInfo2.hubId = kMockVendorHubId;
    hubInfo2.hubDetails =
            HubInfo::HubDetails::make<HubInfo::HubDetails::Tag::vendorHubInfo>(vendorHub);

    VendorHubInfo vendorHub2 = {};
    vendorHub2.name = "Mock Vendor Hub 2";
    vendorHub2.version = 24;

    HubInfo hubInfo3 = {};
    hubInfo3.hubId = kMockVendorHub2Id;
    hubInfo3.hubDetails =
            HubInfo::HubDetails::make<HubInfo::HubDetails::Tag::vendorHubInfo>(vendorHub2);

    _aidl_return->push_back(hubInfo1);
    _aidl_return->push_back(hubInfo2);
    _aidl_return->push_back(hubInfo3);

    return ScopedAStatus::ok();
};

ScopedAStatus ContextHub::getEndpoints(std::vector<EndpointInfo>* _aidl_return) {
    if (_aidl_return == nullptr) {
        return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }

    Service echoService;
    echoService.format = Service::RpcFormat::CUSTOM;
    echoService.serviceDescriptor = "ECHO";
    echoService.majorVersion = 1;
    echoService.minorVersion = 0;

    for (const EndpointInfo& endpoint : kMockEndpointInfos) {
        EndpointInfo endpointWithService(endpoint);
        endpointWithService.services.push_back(echoService);
        _aidl_return->push_back(std::move(endpointWithService));
    }

    return ScopedAStatus::ok();
};

ScopedAStatus ContextHub::registerEndpoint(const EndpointInfo& in_endpoint) {
    std::unique_lock<std::mutex> lock(mEndpointMutex);

    for (const EndpointInfo& endpoint : mEndpoints) {
        if ((endpoint.id.id == in_endpoint.id.id && endpoint.id.hubId == in_endpoint.id.hubId) ||
            endpoint.name == in_endpoint.name) {
            return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
        }
    }
    mEndpoints.push_back(in_endpoint);
    return ScopedAStatus::ok();
};

ScopedAStatus ContextHub::unregisterEndpoint(const EndpointInfo& in_endpoint) {
    std::unique_lock<std::mutex> lock(mEndpointMutex);

    for (auto it = mEndpoints.begin(); it != mEndpoints.end(); ++it) {
        if (it->id.id == in_endpoint.id.id && it->id.hubId == in_endpoint.id.hubId) {
            mEndpoints.erase(it);
            return ScopedAStatus::ok();
        }
    }
    return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
};

ScopedAStatus ContextHub::registerEndpointCallback(
        const std::shared_ptr<IEndpointCallback>& in_callback) {
    std::unique_lock<std::mutex> lock(mEndpointMutex);

    mEndpointCallback = in_callback;
    return ScopedAStatus::ok();
};

ScopedAStatus ContextHub::requestSessionIdRange(int32_t in_size,
                                                std::vector<int32_t>* _aidl_return) {
    constexpr int32_t kMaxSize = 1024;
    if (in_size > kMaxSize || _aidl_return == nullptr) {
        return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }

    {
        std::lock_guard<std::mutex> lock(mEndpointMutex);
        mMaxValidSessionId = in_size;
    }

    _aidl_return->push_back(0);
    _aidl_return->push_back(in_size);
    return ScopedAStatus::ok();
};

ScopedAStatus ContextHub::openEndpointSession(
        int32_t in_sessionId, const EndpointId& in_destination, const EndpointId& in_initiator,
        const std::optional<std::string>& in_serviceDescriptor) {
    // We are not calling onCloseEndpointSession on failure because the remote endpoints (our
    // mock endpoints) always accept the session.

    std::shared_ptr<IEndpointCallback> callback = nullptr;
    {
        std::unique_lock<std::mutex> lock(mEndpointMutex);
        if (in_sessionId > mMaxValidSessionId) {
            ALOGE("openEndpointSession: session ID %" PRId32 " is invalid", in_sessionId);
            return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
        }

        for (const EndpointSession& session : mEndpointSessions) {
            bool sessionAlreadyExists =
                    (session.initiator == in_destination && session.peer == in_initiator) ||
                    (session.peer == in_destination && session.initiator == in_initiator);
            if (sessionAlreadyExists) {
                ALOGD("openEndpointSession: session ID %" PRId32 " already exists", in_sessionId);
                return (session.sessionId == in_sessionId &&
                        session.serviceDescriptor == in_serviceDescriptor)
                               ? ScopedAStatus::ok()
                               : ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
            } else if (session.sessionId == in_sessionId) {
                ALOGE("openEndpointSession: session ID %" PRId32 " is invalid: endpoint mismatch",
                      in_sessionId);
                return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
            }
        }

        // Verify the initiator and destination are valid endpoints
        bool initiatorIsValid = findEndpoint(in_initiator, mEndpoints.begin(), mEndpoints.end());
        if (!initiatorIsValid) {
            ALOGE("openEndpointSession: initiator %" PRIu64 ":%" PRIu64 " is invalid",
                  in_initiator.id, in_initiator.hubId);
            return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
        }
        bool destinationIsValid = findEndpoint(in_destination, &kMockEndpointInfos[0],
                                               &kMockEndpointInfos[kMockEndpointCount]);
        if (!destinationIsValid) {
            ALOGE("openEndpointSession: destination %" PRIu64 ":%" PRIu64 " is invalid",
                  in_destination.id, in_destination.hubId);
            return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
        }

        mEndpointSessions.push_back({
                .sessionId = in_sessionId,
                .initiator = in_initiator,
                .peer = in_destination,
                .serviceDescriptor = in_serviceDescriptor,
        });

        if (mEndpointCallback != nullptr) {
            callback = mEndpointCallback;
        }
    }

    if (callback != nullptr) {
        callback->onEndpointSessionOpenComplete(in_sessionId);
    }
    return ScopedAStatus::ok();
};

ScopedAStatus ContextHub::sendMessageToEndpoint(int32_t in_sessionId, const Message& in_msg) {
    bool foundSession = false;
    std::shared_ptr<IEndpointCallback> callback = nullptr;
    {
        std::unique_lock<std::mutex> lock(mEndpointMutex);

        for (const EndpointSession& session : mEndpointSessions) {
            if (session.sessionId == in_sessionId) {
                foundSession = true;
                break;
            }
        }

        if (mEndpointCallback != nullptr) {
            callback = mEndpointCallback;
        }
    }

    if (!foundSession) {
        ALOGE("sendMessageToEndpoint: session ID %" PRId32 " is invalid", in_sessionId);
        return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }

    if (callback != nullptr) {
        if (in_msg.flags & Message::FLAG_REQUIRES_DELIVERY_STATUS) {
            MessageDeliveryStatus msgStatus = {};
            msgStatus.messageSequenceNumber = in_msg.sequenceNumber;
            msgStatus.errorCode = ErrorCode::OK;
            callback->onMessageDeliveryStatusReceived(in_sessionId, msgStatus);
        }

        // Echo the message back
        callback->onMessageReceived(in_sessionId, in_msg);
    }
    return ScopedAStatus::ok();
};

ScopedAStatus ContextHub::sendMessageDeliveryStatusToEndpoint(
        int32_t /* in_sessionId */, const MessageDeliveryStatus& /* in_msgStatus */) {
    return ScopedAStatus::ok();
};

ScopedAStatus ContextHub::closeEndpointSession(int32_t in_sessionId, Reason /* in_reason */) {
    std::unique_lock<std::mutex> lock(mEndpointMutex);

    for (auto it = mEndpointSessions.begin(); it != mEndpointSessions.end(); ++it) {
        if (it->sessionId == in_sessionId) {
            mEndpointSessions.erase(it);
            return ScopedAStatus::ok();
        }
    }
    ALOGE("closeEndpointSession: session ID %" PRId32 " is invalid", in_sessionId);
    return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
};

ScopedAStatus ContextHub::endpointSessionOpenComplete(int32_t /* in_sessionId */) {
    return ScopedAStatus::ok();
};

}  // namespace aidl::android::hardware::contexthub
