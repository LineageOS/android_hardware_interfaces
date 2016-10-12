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

#include "VehicleCallback.h"

namespace android {
namespace hardware {
namespace vehicle {
namespace V2_0 {
namespace implementation {

// Methods from ::android::hardware::vehicle::V2_0::IVehicleCallback follow.
Return<void> VehicleCallback::onPropertyEvent(const hidl_vec<VehiclePropValue>& value)  {
    // TODO(pavelm): add default implementation
    return Void();
}

// Methods from ::android::hardware::vehicle::V2_0::IVehicleCallback follow.
Return<void> VehicleCallback::onPropertySet(const VehiclePropValue& value)  {
    // TODO(pavelm): add default implementation
    return Void();
}

Return<void> VehicleCallback::onError(StatusCode errorCode,
                                      VehicleProperty propId,
                                      VehiclePropertyOperation operation)  {
    // TODO(pavelm): add default implementation
    return Void();
}


IVehicleCallback* HIDL_FETCH_IVehicleCallback(const char* /* name */) {
    return new VehicleCallback();
}

} // namespace implementation
}  // namespace V2_0
}  // namespace vehicle
}  // namespace hardware
}  // namespace android
