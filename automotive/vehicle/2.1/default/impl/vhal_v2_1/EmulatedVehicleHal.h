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

#ifndef android_hardware_automotive_vehicle_V2_1_impl_EmulatedVehicleHal_H_
#define android_hardware_automotive_vehicle_V2_1_impl_EmulatedVehicleHal_H_

#include <memory>

#include <utils/SystemClock.h>

#include <vhal_v2_0/EmulatedVehicleHal.h>
#include <vhal_v2_0/VehicleHal.h>
#include <vhal_v2_0/VehiclePropertyStore.h>
#include <vhal_v2_1/Obd2SensorStore.h>

#include "DefaultConfig.h"

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace V2_1 {

namespace impl {

using namespace std::placeholders;

class EmulatedVehicleHal : public V2_0::impl::EmulatedVehicleHal {
public:
    EmulatedVehicleHal(V2_0::VehiclePropertyStore* propStore)
        : V2_0::impl::EmulatedVehicleHal(propStore), mPropStore(propStore) {
        initStaticConfig();
    }

    VehiclePropValuePtr get(const V2_0::VehiclePropValue& requestedPropValue,
                            V2_0::StatusCode* outStatus) override;

    V2_0::StatusCode set(const V2_0::VehiclePropValue& propValue) override;

    void onCreate() override;

private:
    void initStaticConfig();
    void initObd2LiveFrame(const V2_0::VehiclePropConfig& propConfig);
    void initObd2FreezeFrame(const V2_0::VehiclePropConfig& propConfig);
    V2_0::StatusCode fillObd2FreezeFrame(const V2_0::VehiclePropValue& requestedPropValue,
                                        V2_0::VehiclePropValue* outValue);
    V2_0::StatusCode fillObd2DtcInfo(V2_0::VehiclePropValue *outValue);
    V2_0::StatusCode clearObd2FreezeFrames(const V2_0::VehiclePropValue& propValue);

private:
    V2_0::VehiclePropertyStore* mPropStore;
};

}  // impl

}  // namespace V2_1
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android


#endif  // android_hardware_automotive_vehicle_V2_0_impl_EmulatedVehicleHal_H_
