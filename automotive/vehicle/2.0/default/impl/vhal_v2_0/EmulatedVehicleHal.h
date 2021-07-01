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

// This file is just used for soft migration from EmulatedVehicleHal to DefaultVehicleHal.
// The virtualized VHAL that uses EmulatedVehicleHal is at a different repo and cannot be updated
// together with this repo, so we need a soft migration. Once the rename is finished at the
// virtualized VHAL side, this file would be removed.

#pragma once

#include "DefaultVehicleHal.h"

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace V2_0 {

namespace impl {

class EmulatedVehicleHal : public DefaultVehicleHal {
  public:
    EmulatedVehicleHal(VehiclePropertyStore* propStore, VehicleHalClient* client)
        : DefaultVehicleHal(propStore, client){};
};

}  // namespace impl

}  // namespace V2_0
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
