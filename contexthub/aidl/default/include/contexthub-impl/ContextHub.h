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
#include <aidl/android/hardware/contexthub/BnEndpointCommunication.h>

#include <array>
#include <atomic>
#include <memory>
#include <mutex>
#include <optional>
#include <unordered_map>
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
    ::ndk::ScopedAStatus registerEndpointHub(
            const std::shared_ptr<IEndpointCallback>& in_callback, const HubInfo& in_hubInfo,
            std::shared_ptr<IEndpointCommunication>* _aidl_return) override;

  private:
    class HubInterface : public BnEndpointCommunication {
      public:
        HubInterface(ContextHub& hal, const std::shared_ptr<IEndpointCallback>& in_callback,
                     const HubInfo& in_hubInfo)
            : mHal(hal), mEndpointCallback(in_callback), kInfo(in_hubInfo) {}
        ~HubInterface() = default;

        ::ndk::ScopedAStatus registerEndpoint(const EndpointInfo& in_endpoint) override;
        ::ndk::ScopedAStatus unregisterEndpoint(const EndpointInfo& in_endpoint) override;
        ::ndk::ScopedAStatus requestSessionIdRange(int32_t in_size,
                                                   std::array<int32_t, 2>* _aidl_return) override;
        ::ndk::ScopedAStatus openEndpointSession(
                int32_t in_sessionId, const EndpointId& in_destination,
                const EndpointId& in_initiator,
                const std::optional<std::string>& in_serviceDescriptor) override;
        ::ndk::ScopedAStatus sendMessageToEndpoint(int32_t in_sessionId,
                                                   const Message& in_msg) override;
        ::ndk::ScopedAStatus sendMessageDeliveryStatusToEndpoint(
                int32_t in_sessionId, const MessageDeliveryStatus& in_msgStatus) override;
        ::ndk::ScopedAStatus closeEndpointSession(int32_t in_sessionId, Reason in_reason) override;
        ::ndk::ScopedAStatus endpointSessionOpenComplete(int32_t in_sessionId) override;
        ::ndk::ScopedAStatus unregister() override;

      private:
        friend class ContextHub;

        struct EndpointSession {
            int32_t sessionId;
            EndpointId initiator;
            EndpointId peer;
            std::optional<std::string> serviceDescriptor;
        };

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

        //! Endpoint storage and information
        ContextHub& mHal;
        std::shared_ptr<IEndpointCallback> mEndpointCallback;
        const HubInfo kInfo;

        std::atomic<bool> mActive = true;

        std::mutex mEndpointMutex;
        std::vector<EndpointInfo> mEndpoints;
        std::vector<EndpointSession> mEndpointSessions;
        uint16_t mBaseSessionId;
        uint16_t mMaxSessionId;
    };

    static constexpr uint32_t kMockHubId = 0;

    std::shared_ptr<IContextHubCallback> mCallback;

    std::unordered_set<char16_t> mConnectedHostEndpoints;

    std::mutex mHostHubsLock;
    std::unordered_map<int64_t, std::shared_ptr<HubInterface>> mIdToHostHub;
    int32_t mNextSessionIdBase = 0;
};

}  // namespace contexthub
}  // namespace hardware
}  // namespace android
}  // namespace aidl
