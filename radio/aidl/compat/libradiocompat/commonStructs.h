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

#include <aidl/android/hardware/radio/RadioIndicationType.h>
#include <aidl/android/hardware/radio/RadioResponseInfo.h>
#include <android/hardware/radio/1.6/types.h>

namespace android::hardware::radio::compat {

aidl::android::hardware::radio::RadioResponseInfo notSupported(int32_t serial);

std::string toAidl(const hidl_string& str);
hidl_string toHidl(const std::string& str);
uint8_t toAidl(int8_t v);
int8_t toAidl(uint8_t v);
int32_t toAidl(uint32_t v);
uint8_t toHidl(int8_t v);

aidl::android::hardware::radio::RadioIndicationType toAidl(V1_0::RadioIndicationType type);
aidl::android::hardware::radio::RadioResponseType toAidl(V1_0::RadioResponseType type);
aidl::android::hardware::radio::RadioError toAidl(V1_0::RadioError type);
aidl::android::hardware::radio::RadioError toAidl(V1_6::RadioError type);

aidl::android::hardware::radio::RadioResponseInfo toAidl(const V1_0::RadioResponseInfo& info);
aidl::android::hardware::radio::RadioResponseInfo toAidl(const V1_6::RadioResponseInfo& info);

}  // namespace android::hardware::radio::compat
