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

#ifndef HEALTHD_HEALTH_HAL_UTILS_H
#define HEALTHD_HEALTH_HAL_UTILS_H

#include <android/hardware/health/2.0/IHealth.h>

namespace android {
namespace hardware {
namespace health {
namespace V2_0 {

// Simplified version of BatteryService.HealthServiceWrapper.init().
// Returns the "default" health instance when it is available, and "backup" instance
// otherwise. Before health 1.0 HAL is removed, this function should be used instead
// of IHealth::getService().
sp<IHealth> get_health_service();

}  // namespace V2_0
}  // namespace health
}  // namespace hardware
}  // namespace android

#endif  // HEALTHD_HEALTH_HAL_UTILS_H
