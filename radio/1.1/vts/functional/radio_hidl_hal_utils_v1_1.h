/*
 * Copyright (C) 2017 The Android Open Source Project
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

#include <android/hardware/radio/1.1/IRadio.h>
#include <android/hardware/radio/1.1/IRadioIndication.h>
#include <android/hardware/radio/1.1/IRadioResponse.h>
#include <android/hardware/radio/1.1/types.h>

#include "radio_hidl_hal_utils_v1_0.h"

using namespace ::android::hardware::radio::V1_1;

class RadioHidlTest_v1_1;

/* Callback class for radio response v1_1*/
class RadioResponse_v1_1 : public RadioResponse {
   protected:
    RadioHidlTest_v1_1& parent_v1_1;

   public:
    RadioResponse_v1_1(RadioHidlTest_v1_1& parent_v1_1);
    virtual ~RadioResponse_v1_1() = default;

    /* 1.1 Api */
    Return<void> setCarrierInfoForImsiEncryptionResponse(const RadioResponseInfo& info);

    Return<void> setSimCardPowerResponse_1_1(const RadioResponseInfo& info);

    Return<void> startNetworkScanResponse(const RadioResponseInfo& info);

    Return<void> stopNetworkScanResponse(const RadioResponseInfo& info);
};

// The main test class for Radio HIDL.
class RadioHidlTest_v1_1 : public RadioHidlTest {
   public:
    virtual void SetUp() override;
    sp<::android::hardware::radio::V1_1::IRadio> radio_v1_1;
    sp<RadioResponse_v1_1> radioRsp_v1_1;
    sp<::android::hardware::radio::V1_1::IRadioIndication> radioInd_v1_1;
};