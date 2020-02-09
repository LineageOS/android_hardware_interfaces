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

#ifndef android_hardware_automotive_vehicle_V2_0_impl_EmulatedVehicleConnector_H_
#define android_hardware_automotive_vehicle_V2_0_impl_EmulatedVehicleConnector_H_

#include <vhal_v2_0/VehicleConnector.h>
#include <vhal_v2_0/VehicleHal.h>

#include "GeneratorHub.h"

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace V2_0 {

namespace impl {

// Extension of the client/server interfaces for emulated vehicle

class EmulatedVehicleClient : public IVehicleClient {
  public:
    // Type of callback function for handling the new property values
    using PropertyCallBackType = std::function<void(const VehiclePropValue&, bool updateStatus)>;

    // Method from IVehicleClient
    void onPropertyValue(const VehiclePropValue& value, bool updateStatus) override;

    void registerPropertyValueCallback(PropertyCallBackType&& callback);

  private:
    PropertyCallBackType mPropCallback;
};

class EmulatedVehicleServer : public IVehicleServer {
  public:
    // Methods from IVehicleServer

    std::vector<VehiclePropConfig> onGetAllPropertyConfig() const override;

    StatusCode onSetProperty(const VehiclePropValue& value, bool updateStatus) override;

    bool onDump(const hidl_handle& fd, const hidl_vec<hidl_string>& options) override;

    // Set the Property Value Pool used in this server
    void setValuePool(VehiclePropValuePool* valuePool);

  private:
    GeneratorHub* getGenerator();

    VehiclePropValuePool* getValuePool() const;

    void onFakeValueGenerated(const VehiclePropValue& value);

    StatusCode handleGenerateFakeDataRequest(const VehiclePropValue& request);

    VehicleHal::VehiclePropValuePtr createApPowerStateReq(VehicleApPowerStateReq req, int32_t param);

    VehicleHal::VehiclePropValuePtr createHwInputKeyProp(VehicleHwKeyInputAction action,
                                                         int32_t keyCode, int32_t targetDisplay);

    // private data members

    GeneratorHub mGeneratorHub{
            std::bind(&EmulatedVehicleServer::onFakeValueGenerated, this, std::placeholders::_1)};

    VehiclePropValuePool* mValuePool{nullptr};

    // TODO(b/146207078): it might be clearer to move members below to an EmulatedUserHal class
    std::unique_ptr<VehiclePropValue> mInitialUserResponseFromCmd;
    StatusCode onSetInitialUserInfo(const VehiclePropValue& value, bool updateStatus);
    void dumpUserHal(int fd, std::string indent);
};

// Helper functions

using EmulatedPassthroughConnector =
        IPassThroughConnector<EmulatedVehicleClient, EmulatedVehicleServer>;
using EmulatedPassthroughConnectorPtr = std::unique_ptr<EmulatedPassthroughConnector>;

EmulatedPassthroughConnectorPtr makeEmulatedPassthroughConnector();

}  // namespace impl

}  // namespace V2_0
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android

#endif  // android_hardware_automotive_vehicle_V2_0_impl_EmulatedVehicleConnector_H_
