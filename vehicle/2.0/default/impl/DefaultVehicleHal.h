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

#ifndef android_hardware_vehicle_V2_0_impl_DefaultVehicleHal_H_
#define android_hardware_vehicle_V2_0_impl_DefaultVehicleHal_H_

#include <VehicleHal.h>
#include <impl/DefaultConfig.h>
#include <utils/SystemClock.h>

namespace android {
namespace hardware {
namespace vehicle {
namespace V2_0 {

namespace impl {

class DefaultVehicleHal : public VehicleHal {
public:
    std::vector<VehiclePropConfig> listProperties() override {
        return std::vector<VehiclePropConfig>(std::begin(kVehicleProperties),
                                              std::end(kVehicleProperties));
    }

    VehiclePropValuePtr get(const VehiclePropValue& requestedPropValue,
                            StatusCode* outStatus) override;

    StatusCode set(const VehiclePropValue& propValue) override;

    StatusCode subscribe(int32_t /*property*/,
                         int32_t /*areas*/,
                         float /*sampleRate*/) override {
        // TODO(pavelm): implement
        return StatusCode::OK;
    }

    StatusCode unsubscribe(int32_t /*property*/) override {
        // TODO(pavelm): implement
        return StatusCode::OK;
    }

private:
    StatusCode getHvacTemperature(int32_t areaId, float* outValue);
    StatusCode setHvacTemperature(int32_t areaId, float value);
    StatusCode getHvacDefroster(int32_t areaId, bool* outValue);
    StatusCode setHvacDefroster(int32_t areaId, bool value);
    StatusCode fillObd2LiveFrame (VehiclePropValuePtr* v);
    StatusCode fillObd2FreezeFrame (VehiclePropValuePtr* v);
private:
    int32_t mFanSpeed = 3;
    int32_t mBrightness = 7;
    float mRow1LeftHvacTemperatureSet = 16;
    float mRow1RightHvacTemperatureSet = 22;
    bool mFrontDefroster = false;
    bool mRearDefroster = false;
    bool mHvacPowerOn = true;
    bool mHvacRecircOn = true;
    bool mHvacAcOn = true;
    bool mHvacAutoOn = true;
    VehicleHvacFanDirection mFanDirection = VehicleHvacFanDirection::FACE;
};

}  // impl

}  // namespace V2_0
}  // namespace vehicle
}  // namespace hardware
}  // namespace android


#endif  // android_hardware_vehicle_V2_0_impl_DefaultVehicleHal_H_
