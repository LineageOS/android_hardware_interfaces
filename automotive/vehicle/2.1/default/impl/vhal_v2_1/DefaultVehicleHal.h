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

#ifndef android_hardware_automotive_vehicle_V2_1_impl_DefaultVehicleHal_H_
#define android_hardware_automotive_vehicle_V2_1_impl_DefaultVehicleHal_H_

#include <memory>

#include <utils/SystemClock.h>

#include <vhal_v2_0/VehicleHal.h>
#include <vhal_v2_0/DefaultVehicleHal.h>

#include "DefaultConfig.h"

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace V2_1 {

namespace impl {

using namespace std::placeholders;

class DefaultVehicleHal : public V2_0::VehicleHal {
public:
    DefaultVehicleHal(V2_0::VehicleHal* vhal20) : mVehicleHal20(vhal20) {}

    std::vector<V2_0::VehiclePropConfig> listProperties() override {
        std::vector<V2_0::VehiclePropConfig> propConfigs(mVehicleHal20->listProperties());

        // Join Vehicle Hal 2.0 and 2.1 configs.
        propConfigs.insert(propConfigs.end(),
                           std::begin(kVehicleProperties),
                           std::end(kVehicleProperties));

        return propConfigs;
    }

    VehiclePropValuePtr get(const V2_0::VehiclePropValue& requestedPropValue,
                            V2_0::StatusCode* outStatus) override {
        // TODO(pavelm): put logic related to VHAL 2.1 here (OBD, VMS, etc)
        return mVehicleHal20->get(requestedPropValue, outStatus);
    }

    V2_0::StatusCode set(const V2_0::VehiclePropValue& propValue) override {
        return mVehicleHal20->set(propValue);
    }

    V2_0::StatusCode subscribe(int32_t property,
                               int32_t areas,
                               float sampleRate) override {
        return mVehicleHal20->subscribe(property, areas, sampleRate);
    }

    V2_0::StatusCode unsubscribe(int32_t property) override {
        return mVehicleHal20->unsubscribe(property);
    }

    void onCreate() override {
        mVehicleHal20->init(getValuePool(),
                            std::bind(&DefaultVehicleHal::doHalEvent, this, _1),
                            std::bind(&DefaultVehicleHal::doHalPropertySetError, this, _1, _2, _3));
    }

private:
    V2_0::VehicleHal* mVehicleHal20;
};

}  // impl

}  // namespace V2_1
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android


#endif  // android_hardware_automotive_vehicle_V2_0_impl_DefaultVehicleHal_H_
