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

#ifndef android_hardware_vehicle_V2_0_VehiclePropConfigIndex_H_
#define android_hardware_vehicle_V2_0_VehiclePropConfigIndex_H_

#include <utils/KeyedVector.h>

#include <android/hardware/vehicle/2.0/IVehicle.h>

namespace android {
namespace hardware {
namespace vehicle {
namespace V2_0 {

/*
 * This is thread-safe immutable class to hold vehicle property configuration
 * data.
 */
class VehiclePropConfigIndex {
public:
    VehiclePropConfigIndex(
        const std::vector<VehiclePropConfig>& properties)
        : mConfigs(properties), mPropToConfig(mConfigs)
    {}

    bool hasConfig(VehicleProperty property) const {
        return mPropToConfig.indexOfKey(property) >= 0;
    }

    const VehiclePropConfig& getConfig(VehicleProperty property) const {
        return *mPropToConfig.valueFor(property);
    }

    const std::vector<VehiclePropConfig>& getAllConfigs() const {
        return mConfigs;
    }

private:
    typedef KeyedVector<VehicleProperty, const VehiclePropConfig*> PropConfigMap;
    class ImmutablePropConfigMap : private PropConfigMap {
    public:
        ImmutablePropConfigMap(const std::vector<VehiclePropConfig>& configs) {
            setCapacity(configs.size());
            for (auto& config : configs) {
                add(config.prop, &config);
            }
        }
    public:
        using PropConfigMap::valueFor;
        using PropConfigMap::indexOfKey;
    };

private:
    const std::vector<VehiclePropConfig> mConfigs;
    const ImmutablePropConfigMap mPropToConfig;  // mConfigs must be declared
                                                 // first.
};

}  // namespace V2_0
}  // namespace vehicle
}  // namespace hardware
}  // namespace android

#endif // android_hardware_vehicle_V2_0_VehiclePropConfigIndex_H_
