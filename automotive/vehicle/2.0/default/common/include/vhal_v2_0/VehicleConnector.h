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

#ifndef android_hardware_automotive_vehicle_V2_0_VehicleConnector_H_
#define android_hardware_automotive_vehicle_V2_0_VehicleConnector_H_

#include <vector>

#include <android/hardware/automotive/vehicle/2.0/types.h>

#include "VehicleClient.h"
#include "VehicleServer.h"

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace V2_0 {

/**
 *  This file defines the interface of client/server pair for HAL-vehicle
 *  communication. Vehicle HAL may use this interface to talk to the vehicle
 *  regardless of the underlying communication channels.
 */

/**
 *  If Android has direct access to the vehicle, then the client and
 *  the server may act in passthrough mode to avoid extra IPC
 *
 *  Template is used here for spliting the logic of operating Android objects (VehicleClientType),
 *  talking to cars (VehicleServerType) and the commucation between client and server (passthrough
 *  mode in this case), so that we can easily combine different parts together without duplicating
 *  codes (for example, in Google VHAL, the server talks to the fake car in the same way no matter
 *  if it is on top of passthrough connector or VSOCK or any other communication channels between
 *  client and server)
 *
 *  The alternative may be factoring the common logic of every operations for both client and
 *  server. Which is not always the case. Making sure different non-template connectors calling
 *  the same method is hard, especially when the engineer maintaining the code may not be aware
 *  of it when making changes. Template is a clean and easy way to solve this problem in this
 *  case.
 */
template <typename VehicleClientType, typename VehicleServerType>
class IPassThroughConnector : public VehicleClientType, public VehicleServerType {
    static_assert(std::is_base_of_v<IVehicleClient, VehicleClientType>);
    static_assert(std::is_base_of_v<IVehicleServer, VehicleServerType>);

  public:
    std::vector<VehiclePropConfig> getAllPropertyConfig() const override {
        return this->onGetAllPropertyConfig();
    }

    StatusCode setProperty(const VehiclePropValue& value, bool updateStatus) override {
        return this->onSetProperty(value, updateStatus);
    }

    void onPropertyValueFromCar(const VehiclePropValue& value, bool updateStatus) override {
        return this->onPropertyValue(value, updateStatus);
    }

    bool dump(const hidl_handle& handle, const hidl_vec<hidl_string>& options) override {
        return this->onDump(handle, options);
    }

    // To be implemented:
    // virtual std::vector<VehiclePropConfig> onGetAllPropertyConfig() = 0;
    // virtual void onPropertyValue(const VehiclePropValue& value) = 0;
    // virtual StatusCode onSetProperty(const VehiclePropValue& value) = 0;
};

}  // namespace V2_0
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android

#endif  // android_hardware_automotive_vehicle_V2_0_VehicleConnector_H_
