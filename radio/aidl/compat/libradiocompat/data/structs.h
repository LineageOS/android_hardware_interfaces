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

#include <aidl/android/hardware/radio/data/DataProfileInfo.h>
#include <aidl/android/hardware/radio/data/KeepaliveRequest.h>
#include <aidl/android/hardware/radio/data/KeepaliveStatus.h>
#include <aidl/android/hardware/radio/data/LinkAddress.h>
#include <aidl/android/hardware/radio/data/OsAppId.h>
#include <aidl/android/hardware/radio/data/PcoDataInfo.h>
#include <aidl/android/hardware/radio/data/RouteSelectionDescriptor.h>
#include <aidl/android/hardware/radio/data/SetupDataCallResult.h>
#include <aidl/android/hardware/radio/data/SliceInfo.h>
#include <aidl/android/hardware/radio/data/SlicingConfig.h>
#include <aidl/android/hardware/radio/data/TrafficDescriptor.h>
#include <aidl/android/hardware/radio/data/UrspRule.h>
#include <android/hardware/radio/1.6/types.h>

namespace android::hardware::radio::compat {

V1_5::DataProfileInfo toHidl(const ::aidl::android::hardware::radio::data::DataProfileInfo& info);

V1_5::LinkAddress toHidl(const ::aidl::android::hardware::radio::data::LinkAddress& addr);

::aidl::android::hardware::radio::data::SliceInfo toAidl(const V1_6::SliceInfo& info);
V1_6::SliceInfo toHidl(const ::aidl::android::hardware::radio::data::SliceInfo& info);

::aidl::android::hardware::radio::data::TrafficDescriptor toAidl(const V1_6::TrafficDescriptor& td);
V1_6::TrafficDescriptor toHidl(const ::aidl::android::hardware::radio::data::TrafficDescriptor& td);

V1_1::KeepaliveRequest toHidl(const ::aidl::android::hardware::radio::data::KeepaliveRequest& keep);

::aidl::android::hardware::radio::data::OsAppId toAidl(const V1_6::OsAppId& appId);
V1_6::OsAppId toHidl(const ::aidl::android::hardware::radio::data::OsAppId& appId);

::aidl::android::hardware::radio::data::SetupDataCallResult  //
toAidl(const V1_5::SetupDataCallResult& res);
::aidl::android::hardware::radio::data::SetupDataCallResult  //
toAidl(const V1_6::SetupDataCallResult& res);

::aidl::android::hardware::radio::data::LinkAddress toAidl(const V1_5::LinkAddress& addr);

::aidl::android::hardware::radio::data::QosSession toAidl(const V1_6::QosSession& session);

::aidl::android::hardware::radio::data::QosFilter toAidl(const V1_6::QosFilter& filter);

::aidl::android::hardware::radio::data::KeepaliveStatus toAidl(const V1_1::KeepaliveStatus& status);

::aidl::android::hardware::radio::data::PcoDataInfo toAidl(const V1_0::PcoDataInfo& info);

::aidl::android::hardware::radio::data::SlicingConfig toAidl(const V1_6::SlicingConfig& cfg);

::aidl::android::hardware::radio::data::UrspRule toAidl(const V1_6::UrspRule& rule);

::aidl::android::hardware::radio::data::RouteSelectionDescriptor  //
toAidl(const V1_6::RouteSelectionDescriptor& descr);

}  // namespace android::hardware::radio::compat
