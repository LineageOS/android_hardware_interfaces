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

#include <aidl/android/hardware/radio/messaging/CdmaBroadcastSmsConfigInfo.h>
#include <aidl/android/hardware/radio/messaging/CdmaSmsAck.h>
#include <aidl/android/hardware/radio/messaging/CdmaSmsMessage.h>
#include <aidl/android/hardware/radio/messaging/CdmaSmsWriteArgs.h>
#include <aidl/android/hardware/radio/messaging/GsmBroadcastSmsConfigInfo.h>
#include <aidl/android/hardware/radio/messaging/GsmSmsMessage.h>
#include <aidl/android/hardware/radio/messaging/ImsSmsMessage.h>
#include <aidl/android/hardware/radio/messaging/SendSmsResult.h>
#include <aidl/android/hardware/radio/messaging/SmsWriteArgs.h>
#include <android/hardware/radio/1.0/types.h>

namespace android::hardware::radio::compat {

V1_0::CdmaSmsAck toHidl(const ::aidl::android::hardware::radio::messaging::CdmaSmsAck& ack);

::aidl::android::hardware::radio::messaging::CdmaSmsMessage toAidl(const V1_0::CdmaSmsMessage& msg);
V1_0::CdmaSmsMessage toHidl(const ::aidl::android::hardware::radio::messaging::CdmaSmsMessage& msg);

V1_0::ImsSmsMessage toHidl(const ::aidl::android::hardware::radio::messaging::ImsSmsMessage& msg);

V1_0::GsmSmsMessage toHidl(const ::aidl::android::hardware::radio::messaging::GsmSmsMessage& msg);

::aidl::android::hardware::radio::messaging::CdmaBroadcastSmsConfigInfo  //
toAidl(const V1_0::CdmaBroadcastSmsConfigInfo& info);
V1_0::CdmaBroadcastSmsConfigInfo  //
toHidl(const ::aidl::android::hardware::radio::messaging::CdmaBroadcastSmsConfigInfo& info);

::aidl::android::hardware::radio::messaging::GsmBroadcastSmsConfigInfo  //
toAidl(const V1_0::GsmBroadcastSmsConfigInfo& info);
V1_0::GsmBroadcastSmsConfigInfo  //
toHidl(const ::aidl::android::hardware::radio::messaging::GsmBroadcastSmsConfigInfo& info);

V1_0::CdmaSmsWriteArgs  //
toHidl(const ::aidl::android::hardware::radio::messaging::CdmaSmsWriteArgs& args);

V1_0::SmsWriteArgs toHidl(const ::aidl::android::hardware::radio::messaging::SmsWriteArgs& args);

::aidl::android::hardware::radio::messaging::SendSmsResult toAidl(const V1_0::SendSmsResult& res);

}  // namespace android::hardware::radio::compat
