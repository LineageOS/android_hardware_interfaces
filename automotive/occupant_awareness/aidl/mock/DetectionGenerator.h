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

#pragma once

#include <aidl/android/hardware/automotive/occupant_awareness/BnOccupantAwareness.h>

namespace android {
namespace hardware {
namespace automotive {
namespace occupant_awareness {
namespace V1_0 {
namespace implementation {

using ::aidl::android::hardware::automotive::occupant_awareness::BnOccupantAwareness;
using ::aidl::android::hardware::automotive::occupant_awareness::OccupantDetections;
using ::aidl::android::hardware::automotive::occupant_awareness::Role;

class DetectionGenerator {
  public:
    static int getSupportedRoles() {
        return static_cast<int>(Role::DRIVER) | static_cast<int>(Role::FRONT_PASSENGER);
    }
    static int getSupportedCapabilities() {
        return static_cast<int>(BnOccupantAwareness::CAP_PRESENCE_DETECTION) |
               static_cast<int>(BnOccupantAwareness::CAP_DRIVER_MONITORING_DETECTION);
    }

    OccupantDetections GetNextDetections();
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace occupant_awareness
}  // namespace automotive
}  // namespace hardware
}  // namespace android
