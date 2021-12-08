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

#include <aidl/android/hardware/radio/voice/Call.h>
#include <aidl/android/hardware/radio/voice/CallForwardInfo.h>
#include <aidl/android/hardware/radio/voice/CdmaCallWaiting.h>
#include <aidl/android/hardware/radio/voice/CdmaDisplayInfoRecord.h>
#include <aidl/android/hardware/radio/voice/CdmaInformationRecord.h>
#include <aidl/android/hardware/radio/voice/CdmaLineControlInfoRecord.h>
#include <aidl/android/hardware/radio/voice/CdmaNumberInfoRecord.h>
#include <aidl/android/hardware/radio/voice/CdmaRedirectingNumberInfoRecord.h>
#include <aidl/android/hardware/radio/voice/CdmaSignalInfoRecord.h>
#include <aidl/android/hardware/radio/voice/CdmaT53AudioControlInfoRecord.h>
#include <aidl/android/hardware/radio/voice/CdmaT53ClirInfoRecord.h>
#include <aidl/android/hardware/radio/voice/CfData.h>
#include <aidl/android/hardware/radio/voice/Dial.h>
#include <aidl/android/hardware/radio/voice/EmergencyNumber.h>
#include <aidl/android/hardware/radio/voice/LastCallFailCauseInfo.h>
#include <aidl/android/hardware/radio/voice/SsInfoData.h>
#include <aidl/android/hardware/radio/voice/StkCcUnsolSsResult.h>
#include <aidl/android/hardware/radio/voice/UusInfo.h>
#include <android/hardware/radio/1.6/types.h>

namespace android::hardware::radio::compat {

V1_0::Dial toHidl(const ::aidl::android::hardware::radio::voice::Dial& info);

V1_0::UusInfo toHidl(const ::aidl::android::hardware::radio::voice::UusInfo& info);

::aidl::android::hardware::radio::voice::CallForwardInfo toAidl(const V1_0::CallForwardInfo& info);
V1_0::CallForwardInfo toHidl(const ::aidl::android::hardware::radio::voice::CallForwardInfo& info);

::aidl::android::hardware::radio::voice::CdmaSignalInfoRecord  //
toAidl(const V1_0::CdmaSignalInfoRecord& record);

::aidl::android::hardware::radio::voice::CdmaCallWaiting toAidl(const V1_0::CdmaCallWaiting& call);

::aidl::android::hardware::radio::voice::CdmaInformationRecord  //
toAidl(const V1_0::CdmaInformationRecord& record);

::aidl::android::hardware::radio::voice::CdmaDisplayInfoRecord  //
toAidl(const V1_0::CdmaDisplayInfoRecord& record);

::aidl::android::hardware::radio::voice::CdmaNumberInfoRecord  //
toAidl(const V1_0::CdmaNumberInfoRecord& record);

::aidl::android::hardware::radio::voice::CdmaRedirectingNumberInfoRecord  //
toAidl(const V1_0::CdmaRedirectingNumberInfoRecord& record);

::aidl::android::hardware::radio::voice::CdmaLineControlInfoRecord  //
toAidl(const V1_0::CdmaLineControlInfoRecord& record);

::aidl::android::hardware::radio::voice::CdmaT53ClirInfoRecord  //
toAidl(const V1_0::CdmaT53ClirInfoRecord& record);

::aidl::android::hardware::radio::voice::CdmaT53AudioControlInfoRecord  //
toAidl(const V1_0::CdmaT53AudioControlInfoRecord& record);

::aidl::android::hardware::radio::voice::EmergencyNumber toAidl(const V1_4::EmergencyNumber& num);

::aidl::android::hardware::radio::voice::StkCcUnsolSsResult  //
toAidl(const V1_0::StkCcUnsolSsResult& res);

::aidl::android::hardware::radio::voice::SsInfoData toAidl(const V1_0::SsInfoData& info);

::aidl::android::hardware::radio::voice::CfData toAidl(const V1_0::CfData& data);

::aidl::android::hardware::radio::voice::Call toAidl(const V1_0::Call& call);
::aidl::android::hardware::radio::voice::Call toAidl(const V1_2::Call& call);
::aidl::android::hardware::radio::voice::Call toAidl(const V1_6::Call& call);

::aidl::android::hardware::radio::voice::UusInfo toAidl(const V1_0::UusInfo& info);

::aidl::android::hardware::radio::voice::LastCallFailCauseInfo  //
toAidl(const V1_0::LastCallFailCauseInfo& info);

}  // namespace android::hardware::radio::compat
