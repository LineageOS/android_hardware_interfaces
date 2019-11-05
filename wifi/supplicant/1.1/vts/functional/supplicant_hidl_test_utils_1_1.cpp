/*
 * Copyright (C) 2016 The Android Open Source Project
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

#include <android-base/logging.h>

#include "supplicant_hidl_test_utils.h"
#include "supplicant_hidl_test_utils_1_1.h"

using ::android::hardware::wifi::supplicant::V1_1::ISupplicant;
using ::android::hardware::wifi::supplicant::V1_1::ISupplicantStaIface;
using ::android::hardware::wifi::supplicant::V1_1::ISupplicantStaNetwork;
using ::android::sp;

sp<ISupplicant> getSupplicant_1_1(const std::string& supplicant_instance_name,
                                  bool isP2pOn) {
    return ISupplicant::castFrom(
        getSupplicant(supplicant_instance_name, isP2pOn));
}

sp<ISupplicantStaIface> getSupplicantStaIface_1_1(
    const sp<ISupplicant>& supplicant) {
    return ISupplicantStaIface::castFrom(getSupplicantStaIface(supplicant));
}

sp<ISupplicantStaNetwork> createSupplicantStaNetwork_1_1(
    const sp<ISupplicant>& supplicant) {
    return ISupplicantStaNetwork::castFrom(
        createSupplicantStaNetwork(supplicant));
}
