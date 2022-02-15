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

#include <libradiocompat/RadioResponse.h>

#include "commonStructs.h"
#include "debug.h"
#include "structs.h"

#include "collections.h"

#define RADIO_MODULE "DataResponse"

namespace android::hardware::radio::compat {

namespace aidl = ::aidl::android::hardware::radio::data;

void RadioResponse::setResponseFunction(std::shared_ptr<aidl::IRadioDataResponse> dataCb) {
    mDataCb = dataCb;
}

std::shared_ptr<aidl::IRadioDataResponse> RadioResponse::dataCb() {
    return mDataCb.get();
}

Return<void> RadioResponse::allocatePduSessionIdResponse(const V1_6::RadioResponseInfo& info,
                                                         int32_t id) {
    LOG_CALL << info.serial;
    dataCb()->allocatePduSessionIdResponse(toAidl(info), id);
    return {};
}

Return<void> RadioResponse::cancelHandoverResponse(const V1_6::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    dataCb()->cancelHandoverResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::deactivateDataCallResponse(const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    dataCb()->deactivateDataCallResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::getDataCallListResponse(const V1_0::RadioResponseInfo& info,
                                                    const hidl_vec<V1_0::SetupDataCallResult>&) {
    LOG_CALL << info.serial;
    LOG(ERROR) << "IRadio HAL 1.0 not supported";
    return {};
}

Return<void> RadioResponse::getDataCallListResponse_1_4(
        const V1_0::RadioResponseInfo& info, const hidl_vec<V1_4::SetupDataCallResult>&) {
    LOG_CALL << info.serial;
    LOG(ERROR) << "IRadio HAL 1.4 not supported";
    return {};
}

Return<void> RadioResponse::getDataCallListResponse_1_5(
        const V1_0::RadioResponseInfo& info,
        const hidl_vec<V1_5::SetupDataCallResult>& dcResponse) {
    LOG_CALL << info.serial;
    dataCb()->getDataCallListResponse(toAidl(info), toAidl(dcResponse));
    return {};
}

Return<void> RadioResponse::getDataCallListResponse_1_6(
        const V1_6::RadioResponseInfo& info,
        const hidl_vec<V1_6::SetupDataCallResult>& dcResponse) {
    LOG_CALL << info.serial;
    dataCb()->getDataCallListResponse(toAidl(info), toAidl(dcResponse));
    return {};
}

Return<void> RadioResponse::getSlicingConfigResponse(const V1_6::RadioResponseInfo& info,
                                                     const V1_6::SlicingConfig& slicingConfig) {
    LOG_CALL << info.serial;
    dataCb()->getSlicingConfigResponse(toAidl(info), toAidl(slicingConfig));
    return {};
}

Return<void> RadioResponse::releasePduSessionIdResponse(const V1_6::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    dataCb()->releasePduSessionIdResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::setDataAllowedResponse(const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    dataCb()->setDataAllowedResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::setDataProfileResponse(const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    dataCb()->setDataProfileResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::setDataProfileResponse_1_5(const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    dataCb()->setDataProfileResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::setDataThrottlingResponse(const V1_6::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    dataCb()->setDataThrottlingResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::setInitialAttachApnResponse(const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    dataCb()->setInitialAttachApnResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::setInitialAttachApnResponse_1_5(const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    dataCb()->setInitialAttachApnResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::setupDataCallResponse(const V1_0::RadioResponseInfo& info,
                                                  const V1_0::SetupDataCallResult&) {
    LOG_CALL << info.serial;
    LOG(ERROR) << "IRadio HAL 1.0 not supported";
    return {};
}

Return<void> RadioResponse::setupDataCallResponse_1_4(const V1_0::RadioResponseInfo& info,
                                                      const V1_4::SetupDataCallResult&) {
    LOG_CALL << info.serial;
    LOG(ERROR) << "IRadio HAL 1.0 not supported";
    return {};
}

Return<void> RadioResponse::setupDataCallResponse_1_5(const V1_0::RadioResponseInfo& info,
                                                      const V1_5::SetupDataCallResult& dcResponse) {
    LOG_CALL << info.serial;
    dataCb()->setupDataCallResponse(toAidl(info), toAidl(dcResponse));
    return {};
}

Return<void> RadioResponse::setupDataCallResponse_1_6(const V1_6::RadioResponseInfo& info,
                                                      const V1_6::SetupDataCallResult& dcResponse) {
    LOG_CALL << info.serial;
    dataCb()->setupDataCallResponse(toAidl(info), toAidl(dcResponse));
    return {};
}

Return<void> RadioResponse::startHandoverResponse(const V1_6::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    dataCb()->startHandoverResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::startKeepaliveResponse(const V1_0::RadioResponseInfo& info,
                                                   const V1_1::KeepaliveStatus& status) {
    LOG_CALL << info.serial;
    dataCb()->startKeepaliveResponse(toAidl(info), toAidl(status));
    return {};
}

Return<void> RadioResponse::stopKeepaliveResponse(const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    dataCb()->stopKeepaliveResponse(toAidl(info));
    return {};
}

}  // namespace android::hardware::radio::compat
