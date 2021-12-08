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

#include <aidl/android/hardware/radio/sim/AppStatus.h>
#include <aidl/android/hardware/radio/sim/CardStatus.h>
#include <aidl/android/hardware/radio/sim/Carrier.h>
#include <aidl/android/hardware/radio/sim/CarrierRestrictions.h>
#include <aidl/android/hardware/radio/sim/IccIo.h>
#include <aidl/android/hardware/radio/sim/IccIoResult.h>
#include <aidl/android/hardware/radio/sim/ImsiEncryptionInfo.h>
#include <aidl/android/hardware/radio/sim/PhonebookCapacity.h>
#include <aidl/android/hardware/radio/sim/PhonebookRecordInfo.h>
#include <aidl/android/hardware/radio/sim/SelectUiccSub.h>
#include <aidl/android/hardware/radio/sim/SimApdu.h>
#include <aidl/android/hardware/radio/sim/SimRefreshResult.h>
#include <android/hardware/radio/1.6/types.h>

namespace android::hardware::radio::compat {

V1_0::IccIo toHidl(const ::aidl::android::hardware::radio::sim::IccIo& icc);

V1_0::SimApdu toHidl(const ::aidl::android::hardware::radio::sim::SimApdu& apdu);

::aidl::android::hardware::radio::sim::Carrier toAidl(const V1_0::Carrier& carrier);
V1_0::Carrier toHidl(const ::aidl::android::hardware::radio::sim::Carrier& carrier);

::aidl::android::hardware::radio::sim::CarrierRestrictions  //
toAidl(const V1_0::CarrierRestrictions& cr);
::aidl::android::hardware::radio::sim::CarrierRestrictions  //
toAidl(const V1_4::CarrierRestrictionsWithPriority& cr);
V1_4::CarrierRestrictionsWithPriority  //
toHidl(const ::aidl::android::hardware::radio::sim::CarrierRestrictions& cr);

V1_1::ImsiEncryptionInfo  //
toHidl(const ::aidl::android::hardware::radio::sim::ImsiEncryptionInfo& info);
V1_6::ImsiEncryptionInfo  //
toHidl_1_6(const ::aidl::android::hardware::radio::sim::ImsiEncryptionInfo& info);

V1_0::SelectUiccSub toHidl(const ::aidl::android::hardware::radio::sim::SelectUiccSub& sub);

::aidl::android::hardware::radio::sim::PhonebookRecordInfo  //
toAidl(const V1_6::PhonebookRecordInfo& info);
V1_6::PhonebookRecordInfo  //
toHidl(const ::aidl::android::hardware::radio::sim::PhonebookRecordInfo& info);

::aidl::android::hardware::radio::sim::SimRefreshResult  //
toAidl(const V1_0::SimRefreshResult& res);

::aidl::android::hardware::radio::sim::CardStatus toAidl(const V1_0::CardStatus& status);
::aidl::android::hardware::radio::sim::CardStatus toAidl(const V1_2::CardStatus& status);
::aidl::android::hardware::radio::sim::CardStatus toAidl(const V1_4::CardStatus& status);
::aidl::android::hardware::radio::sim::CardStatus toAidl(const V1_5::CardStatus& status);

::aidl::android::hardware::radio::sim::AppStatus toAidl(const V1_0::AppStatus& status);
::aidl::android::hardware::radio::sim::AppStatus toAidl(const V1_5::AppStatus& status);

::aidl::android::hardware::radio::sim::PhonebookCapacity toAidl(const V1_6::PhonebookCapacity& c);

::aidl::android::hardware::radio::sim::IccIoResult toAidl(const V1_0::IccIoResult& iir);

}  // namespace android::hardware::radio::compat
