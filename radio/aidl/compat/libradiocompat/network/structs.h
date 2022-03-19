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

#include <aidl/android/hardware/radio/network/BarringInfo.h>
#include <aidl/android/hardware/radio/network/CellIdentity.h>
#include <aidl/android/hardware/radio/network/CellInfo.h>
#include <aidl/android/hardware/radio/network/LceDataInfo.h>
#include <aidl/android/hardware/radio/network/LinkCapacityEstimate.h>
#include <aidl/android/hardware/radio/network/NetworkScanRequest.h>
#include <aidl/android/hardware/radio/network/NetworkScanResult.h>
#include <aidl/android/hardware/radio/network/OperatorInfo.h>
#include <aidl/android/hardware/radio/network/PhysicalChannelConfig.h>
#include <aidl/android/hardware/radio/network/RadioAccessSpecifier.h>
#include <aidl/android/hardware/radio/network/RadioBandMode.h>
#include <aidl/android/hardware/radio/network/RegStateResult.h>
#include <aidl/android/hardware/radio/network/SignalStrength.h>
#include <aidl/android/hardware/radio/network/SignalThresholdInfo.h>
#include <aidl/android/hardware/radio/network/SuppSvcNotification.h>
#include <android/hardware/radio/1.6/types.h>

namespace android::hardware::radio::compat {

::aidl::android::hardware::radio::network::RadioBandMode toAidl(V1_0::RadioBandMode mode);
::aidl::android::hardware::radio::network::GeranBands toAidl(V1_1::GeranBands band);
V1_1::GeranBands toHidl(::aidl::android::hardware::radio::network::GeranBands band);
::aidl::android::hardware::radio::network::UtranBands toAidl(V1_5::UtranBands band);
V1_5::UtranBands toHidl(::aidl::android::hardware::radio::network::UtranBands band);
::aidl::android::hardware::radio::network::EutranBands toAidl(V1_5::EutranBands band);
V1_5::EutranBands toHidl(::aidl::android::hardware::radio::network::EutranBands band);
::aidl::android::hardware::radio::network::NgranBands toAidl(V1_5::NgranBands band);
V1_5::NgranBands toHidl(::aidl::android::hardware::radio::network::NgranBands band);

V1_5::SignalThresholdInfo  //
toHidl(const ::aidl::android::hardware::radio::network::SignalThresholdInfo& info);

::aidl::android::hardware::radio::network::RadioAccessSpecifier  //
toAidl(const V1_5::RadioAccessSpecifier& spec);
V1_5::RadioAccessNetworks  //
toRadioAccessNetworks(::aidl::android::hardware::radio::AccessNetwork val);
V1_5::RadioAccessSpecifier  //
toHidl(const ::aidl::android::hardware::radio::network::RadioAccessSpecifier& spec);

V1_5::NetworkScanRequest  //
toHidl(const ::aidl::android::hardware::radio::network::NetworkScanRequest& req);

::aidl::android::hardware::radio::network::CellIdentity toAidl(const V1_5::CellIdentity& ci);

::aidl::android::hardware::radio::network::BarringInfo toAidl(const V1_5::BarringInfo& info);

::aidl::android::hardware::radio::network::ClosedSubscriberGroupInfo  //
toAidl(const V1_5::ClosedSubscriberGroupInfo& info);

::aidl::android::hardware::radio::network::CellInfo toAidl(const V1_5::CellInfo& info);
::aidl::android::hardware::radio::network::CellInfo toAidl(const V1_6::CellInfo& info);

::aidl::android::hardware::radio::network::LinkCapacityEstimate  //
toAidl(const V1_2::LinkCapacityEstimate& lce);
::aidl::android::hardware::radio::network::LinkCapacityEstimate  //
toAidl(const V1_6::LinkCapacityEstimate& lce);

::aidl::android::hardware::radio::network::PhysicalChannelConfig  //
toAidl(const V1_4::PhysicalChannelConfig& cfg);
::aidl::android::hardware::radio::network::PhysicalChannelConfig  //
toAidl(const V1_6::PhysicalChannelConfig& cfg);

::aidl::android::hardware::radio::network::SignalStrength toAidl(const V1_4::SignalStrength& sig);
::aidl::android::hardware::radio::network::SignalStrength toAidl(const V1_6::SignalStrength& sig);

::aidl::android::hardware::radio::network::NetworkScanResult  //
toAidl(const V1_5::NetworkScanResult& res);
::aidl::android::hardware::radio::network::NetworkScanResult  //
toAidl(const V1_6::NetworkScanResult& res);

::aidl::android::hardware::radio::network::SuppSvcNotification  //
toAidl(const V1_0::SuppSvcNotification& svc);

::aidl::android::hardware::radio::network::OperatorInfo toAidl(const V1_0::OperatorInfo& info);

::aidl::android::hardware::radio::network::RegStateResult toAidl(const V1_5::RegStateResult& res);
::aidl::android::hardware::radio::network::RegStateResult toAidl(const V1_6::RegStateResult& res);

::aidl::android::hardware::radio::network::LceDataInfo toAidl(const V1_0::LceDataInfo& info);

}  // namespace android::hardware::radio::compat
