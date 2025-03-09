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

#pragma once

#include <aidl/android/hardware/contexthub/BnContextHub.h>

#include <mutex>
#include <unordered_set>
#include <vector>

namespace aidl {
namespace android {
namespace hardware {
namespace contexthub {

class ContextHub : public BnContextHub {
    ::ndk::ScopedAStatus getContextHubs(std::vector<ContextHubInfo>* out_contextHubInfos) override;
    ::ndk::ScopedAStatus loadNanoapp(int32_t in_contextHubId, const NanoappBinary& in_appBinary,
                                     int32_t in_transactionId) override;
    ::ndk::ScopedAStatus unloadNanoapp(int32_t in_contextHubId, int64_t in_appId,
                                       int32_t in_transactionId) override;
    ::ndk::ScopedAStatus disableNanoapp(int32_t in_contextHubId, int64_t in_appId,
                                        int32_t in_transactionId) override;
    ::ndk::ScopedAStatus enableNanoapp(int32_t in_contextHubId, int64_t in_appId,
                                       int32_t in_transactionId) override;
    ::ndk::ScopedAStatus onSettingChanged(Setting in_setting, bool in_enabled) override;
    ::ndk::ScopedAStatus queryNanoapps(int32_t in_contextHubId) override;
    ::ndk::ScopedAStatus getPreloadedNanoappIds(
            int32_t in_contextHubId, std::vector<int64_t>* out_preloadedNanoappIds) override;
    ::ndk::ScopedAStatus registerCallback(
            int32_t in_contextHubId, const std::shared_ptr<IContextHubCallback>& in_cb) override;
    ::ndk::ScopedAStatus sendMessageToHub(int32_t in_contextHubId,
                                          const ContextHubMessage& in_message) override;
    ::ndk::ScopedAStatus setTestMode(bool enable) override;
    ::ndk::ScopedAStatus onHostEndpointConnected(const HostEndpointInfo& in_info) override;

    ::ndk::ScopedAStatus onHostEndpointDisconnected(char16_t in_hostEndpointId) override;
    ::ndk::ScopedAStatus onNanSessionStateChanged(const NanSessionStateUpdate& in_update) override;
    ::ndk::ScopedAStatus sendMessageDeliveryStatusToHub(
            int32_t in_contextHubId,
            const MessageDeliveryStatus& in_messageDeliveryStatus) override;

    ::ndk::ScopedAStatus getHubs(std::vector<HubInfo>* _aidl_return) override;
    ::ndk::ScopedAStatus getEndpoints(std::vector<EndpointInfo>* _aidl_return) override;
    ::ndk::ScopedAStatus registerEndpoint(const EndpointInfo& in_endpoint) override;
    ::ndk::ScopedAStatus unregisterEndpoint(const EndpointInfo& in_endpoint) override;
    ::ndk::ScopedAStatus registerEndpointCallback(
            const std::shared_ptr<IEndpointCallback>& in_callback) override;
    ::ndk::ScopedAStatus requestSessionIdRange(int32_t in_size,
                                               std::vector<int32_t>* _aidl_return) override;
    ::ndk::ScopedAStatus openEndpointSession(
            int32_t in_sessionId, const EndpointId& in_destination, const EndpointId& in_initiator,
            const std::optional<std::string>& in_serviceDescriptor) override;
    ::ndk::ScopedAStatus sendMessageToEndpoint(int32_t in_sessionId,
                                               const Message& in_msg) override;
    ::ndk::ScopedAStatus sendMessageDeliveryStatusToEndpoint(
            int32_t in_sessionId, const MessageDeliveryStatus& in_msgStatus) override;
    ::ndk::ScopedAStatus closeEndpointSession(int32_t in_sessionId, Reason in_reason) override;
    ::ndk::ScopedAStatus endpointSessionOpenComplete(int32_t in_sessionId) override;

  private:
    struct EndpointSession {
        int32_t sessionId;
        EndpointId initiator;
        EndpointId peer;
        std::optional<std::string> serviceDescriptor;
    };

    static constexpr uint32_t kMockHubId = 0;

    //! Finds an endpoint in the range defined by the endpoints
    //! @return whether the endpoint was found
    template <typename Iter>
    bool findEndpoint(const EndpointId& target, const Iter& begin, const Iter& end) {
        for (auto iter = begin; iter != end; ++iter) {
            if (iter->id.id == target.id && iter->id.hubId == target.hubId) {
                return true;
            }
        }
        return false;
    }

    std::shared_ptr<IContextHubCallback> mCallback;

    std::unordered_set<char16_t> mConnectedHostEndpoints;

    //! Endpoint storage and information
    std::mutex mEndpointMutex;
    std::vector<EndpointInfo> mEndpoints;
    std::vector<EndpointSession> mEndpointSessions;
    std::shared_ptr<IEndpointCallback> mEndpointCallback;
    int32_t mMaxValidSessionId = 0;
};

}  // namespace contexthub
}  // namespace hardware
}  // namespace android
}  // namespace aidl
