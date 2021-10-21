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

#ifndef android_hardware_automotive_vehicle_aidl_impl_fake_impl_hardware_include_FakeVehicleHardware_H_
#define android_hardware_automotive_vehicle_aidl_impl_fake_impl_hardware_include_FakeVehicleHardware_H_

#include <DefaultConfig.h>
#include <IVehicleHardware.h>
#include <VehicleHalTypes.h>
#include <VehiclePropertyStore.h>
#include <android-base/thread_annotations.h>

#include <map>
#include <memory>
#include <mutex>
#include <vector>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {

class FakeVehicleHardware final : public IVehicleHardware {
  public:
    using SetValuesCallback = std::function<void(
            const std::vector<::aidl::android::hardware::automotive::vehicle::SetValueResult>&)>;
    using GetValuesCallback = std::function<void(
            const std::vector<::aidl::android::hardware::automotive::vehicle::GetValueResult>&)>;
    using OnPropertyChangeCallback = std::function<void(
            const std::vector<::aidl::android::hardware::automotive::vehicle::VehiclePropValue>&)>;
    using OnPropertySetErrorCallback = std::function<void(const std::vector<SetValueErrorEvent>&)>;

    FakeVehicleHardware();

    explicit FakeVehicleHardware(std::unique_ptr<VehiclePropValuePool> valuePool);

    // Get all the property configs.
    std::vector<::aidl::android::hardware::automotive::vehicle::VehiclePropConfig>
    getAllPropertyConfigs() const override;

    // Set property values asynchronously. Server could return before the property set requests
    // are sent to vehicle bus or before property set confirmation is received. The callback is
    // safe to be called after the function returns and is safe to be called in a different thread.
    ::aidl::android::hardware::automotive::vehicle::StatusCode setValues(
            SetValuesCallback&& callback,
            const std::vector<::aidl::android::hardware::automotive::vehicle::SetValueRequest>&
                    requests) override;

    // Get property values asynchronously. Server could return before the property values are ready.
    // The callback is safe to be called after the function returns and is safe to be called in a
    // different thread.
    ::aidl::android::hardware::automotive::vehicle::StatusCode getValues(
            GetValuesCallback&& callback,
            const std::vector<::aidl::android::hardware::automotive::vehicle::GetValueRequest>&
                    requests) const override;

    // Dump debug information in the server.
    DumpResult dump(const std::vector<std::string>& options) override;

    // Check whether the system is healthy, return {@code StatusCode::OK} for healthy.
    ::aidl::android::hardware::automotive::vehicle::StatusCode checkHealth() override;

    // Register a callback that would be called when there is a property change event from vehicle.
    void registerOnPropertyChangeEvent(OnPropertyChangeCallback&& callback) override;

    // Register a callback that would be called when there is a property set error event from
    // vehicle.
    void registerOnPropertySetErrorEvent(OnPropertySetErrorCallback&& callback) override;

  private:
    void storePropInitialValue(const defaultconfig::ConfigDeclaration& config);
    void init(std::shared_ptr<VehiclePropValuePool> valuePool);
    void onValueChangeCallback(
            const ::aidl::android::hardware::automotive::vehicle::VehiclePropValue& value);

    std::unique_ptr<VehiclePropertyStore> mServerSidePropStore;
    // mValuePool is also used in mServerSidePropStore.
    std::shared_ptr<VehiclePropValuePool> mValuePool;
    std::mutex mCallbackLock;
    OnPropertyChangeCallback mOnPropertyChangeCallback GUARDED_BY(mCallbackLock);
    OnPropertySetErrorCallback mOnPropertySetErrorCallback GUARDED_BY(mCallbackLock);
};

}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android

#endif  // android_hardware_automotive_vehicle_aidl_impl_fake_impl_hardware_include_FakeVehicleHardware_H_
