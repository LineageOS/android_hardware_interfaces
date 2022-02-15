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

#include <libradiocompat/RadioIndication.h>

#include "commonStructs.h"
#include "debug.h"
#include "structs.h"

#include "collections.h"

#define RADIO_MODULE "DataIndication"

namespace android::hardware::radio::compat {

namespace aidl = ::aidl::android::hardware::radio::data;

void RadioIndication::setResponseFunction(std::shared_ptr<aidl::IRadioDataIndication> dataCb) {
    mDataCb = dataCb;
}

std::shared_ptr<aidl::IRadioDataIndication> RadioIndication::dataCb() {
    return mDataCb.get();
}

Return<void> RadioIndication::dataCallListChanged(V1_0::RadioIndicationType type,
                                                  const hidl_vec<V1_0::SetupDataCallResult>&) {
    LOG_CALL << type;
    LOG(ERROR) << "IRadio HAL 1.0 not supported";
    return {};
}

Return<void> RadioIndication::dataCallListChanged_1_4(V1_0::RadioIndicationType type,
                                                      const hidl_vec<V1_4::SetupDataCallResult>&) {
    LOG_CALL << type;
    LOG(ERROR) << "IRadio HAL 1.4 not supported";
    return {};
}

Return<void> RadioIndication::dataCallListChanged_1_5(
        V1_0::RadioIndicationType type, const hidl_vec<V1_5::SetupDataCallResult>& dcList) {
    LOG_CALL << type;
    dataCb()->dataCallListChanged(toAidl(type), toAidl(dcList));
    return {};
}

Return<void> RadioIndication::dataCallListChanged_1_6(
        V1_0::RadioIndicationType type, const hidl_vec<V1_6::SetupDataCallResult>& dcList) {
    LOG_CALL << type;
    dataCb()->dataCallListChanged(toAidl(type), toAidl(dcList));
    return {};
}

Return<void> RadioIndication::keepaliveStatus(V1_0::RadioIndicationType type,
                                              const V1_1::KeepaliveStatus& status) {
    LOG_CALL << type;
    dataCb()->keepaliveStatus(toAidl(type), toAidl(status));
    return {};
}

Return<void> RadioIndication::pcoData(V1_0::RadioIndicationType type,
                                      const V1_0::PcoDataInfo& pco) {
    LOG_CALL << type;
    dataCb()->pcoData(toAidl(type), toAidl(pco));
    return {};
}

Return<void> RadioIndication::unthrottleApn(V1_0::RadioIndicationType type,
                                            const hidl_string& apn) {
    LOG_CALL << type;
    dataCb()->unthrottleApn(toAidl(type), mContext->getDataProfile(apn));
    return {};
}

Return<void> RadioIndication::slicingConfigChanged(V1_0::RadioIndicationType type,
                                                   const V1_6::SlicingConfig& slicingConfig) {
    LOG_CALL << type;
    dataCb()->slicingConfigChanged(toAidl(type), toAidl(slicingConfig));
    return {};
}

}  // namespace android::hardware::radio::compat
