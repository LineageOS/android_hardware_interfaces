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

#include <VtsHalHidlTargetTestBase.h>
#include <android-base/logging.h>

#include "hostapd_hidl_test_utils.h"
#include "hostapd_hidl_test_utils_1_1.h"

using ::android::sp;
using ::android::hardware::wifi::hostapd::V1_1::IHostapd;

sp<IHostapd> getHostapd_1_1() { return IHostapd::castFrom(getHostapd()); }
