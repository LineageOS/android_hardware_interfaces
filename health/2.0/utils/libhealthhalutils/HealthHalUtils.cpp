/*
 * Copyright (C) 2018 The Android Open Source Project
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

#define LOG_TAG "HealthHalUtils"

#include <android-base/logging.h>
#include <healthhalutils/HealthHalUtils.h>

namespace android {
namespace hardware {
namespace health {
namespace V2_0 {

sp<IHealth> get_health_service() {
    // For the core and vendor variant, the "backup" instance points to healthd,
    // which is removed.
    // For the recovery variant, the "backup" instance has a different
    // meaning. It points to android.hardware.health@2.0-impl-default.recovery
    // which was assumed by OEMs to be always installed when a
    // vendor-specific libhealthd is not necessary. Hence, its behavior
    // is kept. See health/2.0/README.md.
    // android.hardware.health@2.0-impl-default.recovery, and subsequently the
    // special handling of recovery mode below, can be removed once health@2.1
    // is the minimum required version (i.e. compatibility matrix level 5 is the
    // minimum supported level). Health 2.1 requires OEMs to install the
    // implementation to the recovery partition when it is necessary (i.e. on
    // non-A/B devices, where IsBatteryOk() is needed in recovery).
    for (auto&& instanceName :
#ifdef __ANDROID_RECOVERY__
         { "default", "backup" }
#else
         {"default"}
#endif
    ) {
        auto ret = IHealth::getService(instanceName);
        if (ret != nullptr) {
            return ret;
        }
        LOG(INFO) << "health: cannot get " << instanceName << " service";
    }
    return nullptr;
}

}  // namespace V2_0
}  // namespace health
}  // namespace hardware
}  // namespace android
