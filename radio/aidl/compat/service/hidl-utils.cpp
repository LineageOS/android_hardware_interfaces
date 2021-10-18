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

#include "hidl-utils.h"

#include <android-base/logging.h>

namespace android::hardware::hidl_utils {

class HalDeathRecipient : public hidl_death_recipient {
    void serviceDied(uint64_t /* cookie */, const wp<hidl::base::V1_0::IBase>& /* who */) override {
        LOG(FATAL) << "One of the linked HALs died. Restarting...";
    }
};

static const auto gHalDeathRecipient = sp<HalDeathRecipient>::make();

void linkDeathToDeath(sp<::android::hidl::base::V1_0::IBase> hal) {
    const auto linkStatus = hal->linkToDeath(gHalDeathRecipient, 0);
    CHECK(linkStatus.withDefault(false)) << "Failed to link to HAL death";
}

}  // namespace android::hardware::hidl_utils
