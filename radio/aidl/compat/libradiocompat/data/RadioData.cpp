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

#include <libradiocompat/RadioData.h>

#include "commonStructs.h"
#include "debug.h"
#include "structs.h"

#include "collections.h"

#define RADIO_MODULE "Data"

namespace android::hardware::radio::compat {

using ::ndk::ScopedAStatus;
namespace aidl = ::aidl::android::hardware::radio::data;
namespace aidlCommon = ::aidl::android::hardware::radio;
constexpr auto ok = &ScopedAStatus::ok;

std::shared_ptr<aidl::IRadioDataResponse> RadioData::respond() {
    return mCallbackManager->response().dataCb();
}

ScopedAStatus RadioData::allocatePduSessionId(int32_t serial) {
    LOG_CALL << serial;
    if (mHal1_6) {
        mHal1_6->allocatePduSessionId(serial);
    } else {
        respond()->allocatePduSessionIdResponse(notSupported(serial), 0);
    }
    return ok();
}

ScopedAStatus RadioData::cancelHandover(int32_t serial, int32_t callId) {
    LOG_CALL << serial;
    if (mHal1_6) {
        mHal1_6->cancelHandover(serial, callId);
    } else {
        respond()->cancelHandoverResponse(notSupported(serial));
    }
    return ok();
}

ScopedAStatus RadioData::deactivateDataCall(int32_t serial, int32_t cid,
                                            aidl::DataRequestReason reason) {
    LOG_CALL << serial;
    mHal1_5->deactivateDataCall_1_2(serial, cid, V1_2::DataRequestReason(reason));
    return ok();
}

ScopedAStatus RadioData::getDataCallList(int32_t serial) {
    LOG_CALL << serial;
    if (mHal1_6) {
        mHal1_6->getDataCallList_1_6(serial);
    } else {
        mHal1_5->getDataCallList(serial);
    }
    return ok();
}

ScopedAStatus RadioData::getSlicingConfig(int32_t serial) {
    LOG_CALL << serial;
    if (mHal1_6) {
        mHal1_6->getSlicingConfig(serial);
    } else {
        respond()->getSlicingConfigResponse(notSupported(serial), {});
    }
    return ok();
}

ScopedAStatus RadioData::releasePduSessionId(int32_t serial, int32_t id) {
    LOG_CALL << serial;
    if (mHal1_6) {
        mHal1_6->releasePduSessionId(serial, id);
    } else {
        respond()->releasePduSessionIdResponse(notSupported(serial));
    }
    return ok();
}

ScopedAStatus RadioData::responseAcknowledgement() {
    LOG_CALL;
    mHal1_5->responseAcknowledgement();
    return ok();
}

ScopedAStatus RadioData::setDataAllowed(int32_t serial, bool allow) {
    LOG_CALL << serial;
    mHal1_5->setDataAllowed(serial, allow);
    return ok();
}

ScopedAStatus RadioData::setDataProfile(int32_t serial,
                                        const std::vector<aidl::DataProfileInfo>& profiles) {
    LOG_CALL << serial;
    mHal1_5->setDataProfile_1_5(serial, toHidl(profiles));
    return ok();
}

ScopedAStatus RadioData::setDataThrottling(int32_t serial, aidl::DataThrottlingAction dta,
                                           int64_t completionDurationMs) {
    LOG_CALL << serial;
    if (mHal1_6) {
        mHal1_6->setDataThrottling(serial, V1_6::DataThrottlingAction(dta), completionDurationMs);
    } else {
        respond()->setDataThrottlingResponse(notSupported(serial));
    }
    return ok();
}

ScopedAStatus RadioData::setInitialAttachApn(int32_t serial,
                                             const std::optional<aidl::DataProfileInfo>& info) {
    LOG_CALL << serial;
    mHal1_5->setInitialAttachApn_1_5(serial, toHidl(info.value()));
    return ok();
}

ScopedAStatus RadioData::setResponseFunctions(
        const std::shared_ptr<aidl::IRadioDataResponse>& response,
        const std::shared_ptr<aidl::IRadioDataIndication>& indication) {
    LOG_CALL << response << ' ' << indication;
    mCallbackManager->setResponseFunctions(response, indication);
    return ok();
}

ScopedAStatus RadioData::setupDataCall(int32_t serial, aidlCommon::AccessNetwork accessNetwork,
                                       const aidl::DataProfileInfo& dataProfileInfo,
                                       bool roamingAllowed, aidl::DataRequestReason reason,
                                       const std::vector<aidl::LinkAddress>& addresses,
                                       const std::vector<std::string>& dnses, int32_t pduSessId,
                                       const std::optional<aidl::SliceInfo>& sliceInfo,
                                       bool matchAllRuleAllowed) {
    if (mHal1_6) {
        mHal1_6->setupDataCall_1_6(
                serial, V1_5::AccessNetwork(accessNetwork), toHidl(dataProfileInfo), roamingAllowed,
                V1_2::DataRequestReason(reason), toHidl(addresses), toHidl(dnses), pduSessId,
                toHidl<V1_6::OptionalSliceInfo>(sliceInfo),
                toHidl<V1_6::OptionalTrafficDescriptor>(dataProfileInfo.trafficDescriptor),
                matchAllRuleAllowed);
        mContext->addDataProfile(dataProfileInfo);
    } else {
        mHal1_5->setupDataCall_1_5(
                serial, V1_5::AccessNetwork(accessNetwork), toHidl(dataProfileInfo), roamingAllowed,
                V1_2::DataRequestReason(reason), toHidl(addresses), toHidl(dnses));
    }
    return ok();
}

ScopedAStatus RadioData::startHandover(int32_t serial, int32_t callId) {
    LOG_CALL << serial;
    if (mHal1_6) {
        mHal1_6->startHandover(serial, callId);
    } else {
        respond()->startHandoverResponse(notSupported(serial));
    }
    return ok();
}

ScopedAStatus RadioData::startKeepalive(int32_t serial, const aidl::KeepaliveRequest& keepalive) {
    LOG_CALL << serial;
    mHal1_5->startKeepalive(serial, toHidl(keepalive));
    return ok();
}

ScopedAStatus RadioData::stopKeepalive(int32_t serial, int32_t sessionHandle) {
    LOG_CALL << serial;
    mHal1_5->stopKeepalive(serial, sessionHandle);
    return ok();
}

}  // namespace android::hardware::radio::compat
