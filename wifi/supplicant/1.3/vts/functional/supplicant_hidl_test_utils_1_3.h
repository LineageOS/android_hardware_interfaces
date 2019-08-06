/*
 * Copyright (C) 2019 The Android Open Source Project
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

#ifndef SUPPLICANT_HIDL_TEST_UTILS_1_3_H
#define SUPPLICANT_HIDL_TEST_UTILS_1_3_H

#include <android/hardware/wifi/supplicant/1.3/ISupplicantStaNetwork.h>

android::sp<android::hardware::wifi::supplicant::V1_3::ISupplicantStaNetwork>
createSupplicantStaNetwork_1_3();

#endif /* SUPPLICANT_HIDL_TEST_UTILS_1_3_H */
