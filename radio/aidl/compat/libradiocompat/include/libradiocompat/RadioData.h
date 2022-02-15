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

#include "RadioCompatBase.h"

#include <aidl/android/hardware/radio/data/BnRadioData.h>

namespace android::hardware::radio::compat {

class RadioData : public RadioCompatBase, public aidl::android::hardware::radio::data::BnRadioData {
    ::ndk::ScopedAStatus allocatePduSessionId(int32_t serial) override;
    ::ndk::ScopedAStatus cancelHandover(int32_t serial, int32_t callId) override;
    ::ndk::ScopedAStatus deactivateDataCall(
            int32_t serial, int32_t cid,
            ::aidl::android::hardware::radio::data::DataRequestReason reason) override;
    ::ndk::ScopedAStatus getDataCallList(int32_t serial) override;
    ::ndk::ScopedAStatus getSlicingConfig(int32_t serial) override;
    ::ndk::ScopedAStatus releasePduSessionId(int32_t serial, int32_t id) override;
    ::ndk::ScopedAStatus responseAcknowledgement() override;
    ::ndk::ScopedAStatus setDataAllowed(int32_t serial, bool allow) override;
    ::ndk::ScopedAStatus setDataProfile(
            int32_t serial,
            const std::vector<::aidl::android::hardware::radio::data::DataProfileInfo>& profiles)
            override;
    ::ndk::ScopedAStatus setDataThrottling(
            int32_t serial,
            ::aidl::android::hardware::radio::data::DataThrottlingAction dataThrottlingAction,
            int64_t completionDurationMillis) override;
    ::ndk::ScopedAStatus setInitialAttachApn(
            int32_t serial,
            const std::optional<::aidl::android::hardware::radio::data::DataProfileInfo>& dpInfo)
            override;
    ::ndk::ScopedAStatus setResponseFunctions(
            const std::shared_ptr<::aidl::android::hardware::radio::data::IRadioDataResponse>&
                    radioDataResponse,
            const std::shared_ptr<::aidl::android::hardware::radio::data::IRadioDataIndication>&
                    radioDataIndication) override;
    ::ndk::ScopedAStatus setupDataCall(
            int32_t serial, ::aidl::android::hardware::radio::AccessNetwork accessNetwork,
            const ::aidl::android::hardware::radio::data::DataProfileInfo& dataProfileInfo,
            bool roamingAllowed, ::aidl::android::hardware::radio::data::DataRequestReason reason,
            const std::vector<::aidl::android::hardware::radio::data::LinkAddress>& addresses,
            const std::vector<std::string>& dnses, int32_t pduSessionId,
            const std::optional<::aidl::android::hardware::radio::data::SliceInfo>& sliceInfo,
            bool matchAllRuleAllowed) override;
    ::ndk::ScopedAStatus startHandover(int32_t serial, int32_t callId) override;
    ::ndk::ScopedAStatus startKeepalive(
            int32_t serial,
            const ::aidl::android::hardware::radio::data::KeepaliveRequest& keepalive) override;
    ::ndk::ScopedAStatus stopKeepalive(int32_t serial, int32_t sessionHandle) override;

  protected:
    std::shared_ptr<::aidl::android::hardware::radio::data::IRadioDataResponse> respond();

  public:
    using RadioCompatBase::RadioCompatBase;
};

}  // namespace android::hardware::radio::compat
