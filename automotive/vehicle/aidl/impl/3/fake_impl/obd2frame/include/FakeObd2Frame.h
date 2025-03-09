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
#ifndef android_hardware_automotive_vehicle_aidl_impl_fake_impl_obd2frame_include_FakeObd2Frame_H_
#define android_hardware_automotive_vehicle_aidl_impl_fake_impl_obd2frame_include_FakeObd2Frame_H_

#include <Obd2SensorStore.h>
#include <VehicleHalTypes.h>
#include <VehiclePropertyStore.h>

#include <android-base/result.h>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace fake {
namespace obd2frame {

class FakeObd2Frame final {
  public:
    explicit FakeObd2Frame(std::shared_ptr<VehiclePropertyStore> propStore)
        : mPropStore(propStore) {}

    void initObd2LiveFrame(
            const aidl::android::hardware::automotive::vehicle::VehiclePropConfig& propConfig);
    void initObd2FreezeFrame(
            const aidl::android::hardware::automotive::vehicle::VehiclePropConfig& propConfig);
    VhalResult<VehiclePropValuePool::RecyclableType> getObd2FreezeFrame(
            const aidl::android::hardware::automotive::vehicle::VehiclePropValue&
                    requestedPropValue) const;
    VhalResult<VehiclePropValuePool::RecyclableType> getObd2DtcInfo() const;
    VhalResult<void> clearObd2FreezeFrames(
            const aidl::android::hardware::automotive::vehicle::VehiclePropValue& propValue);
    static bool isDiagnosticProperty(
            const aidl::android::hardware::automotive::vehicle::VehiclePropConfig& propConfig);

  private:
    std::shared_ptr<VehiclePropertyStore> mPropStore;

    std::unique_ptr<Obd2SensorStore> fillDefaultObd2Frame(size_t numVendorIntegerSensors,
                                                          size_t numVendorFloatSensors);
};

}  // namespace obd2frame
}  // namespace fake
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android

#endif  // android_hardware_automotive_vehicle_aidl_impl_fake_impl_obd2frame_include_FakeObd2Frame_H_
