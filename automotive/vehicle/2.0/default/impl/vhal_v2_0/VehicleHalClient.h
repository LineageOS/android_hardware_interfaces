/*
 * Copyright (C) 2020 The Android Open Source Project
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

#include <vhal_v2_0/VehicleClient.h>

namespace android::hardware::automotive::vehicle::V2_0::impl {

// The common client operations that may be used by both native and
// virtualized VHAL clients.
class VehicleHalClient : public IVehicleClient {
  public:
    // Type of callback function for handling the new property values
    using PropertyCallBackType = std::function<void(const VehiclePropValue&, bool updateStatus)>;

    // Method from IVehicleClient
    void onPropertyValue(const VehiclePropValue& value, bool updateStatus) override;

    void registerPropertyValueCallback(PropertyCallBackType&& callback);

  private:
    PropertyCallBackType mPropCallback;
};

}  // namespace android::hardware::automotive::vehicle::V2_0::impl
