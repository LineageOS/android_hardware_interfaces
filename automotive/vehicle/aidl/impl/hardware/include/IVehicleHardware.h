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

#ifndef android_hardware_automotive_vehicle_aidl_impl_hardware_include_IVehicleHardware_H_
#define android_hardware_automotive_vehicle_aidl_impl_hardware_include_IVehicleHardware_H_

#include <VehicleHalTypes.h>

#include <memory>
#include <vector>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {

// A structure used to return dumped information.
struct DumpResult {
    // If callerShouldDumpState is true, caller would print the information in buffer and
    // continue to dump its state, otherwise would just dump the buffer and skip its own
    // dumping logic.
    bool callerShouldDumpState;
    // The dumped information for the caller to print.
    std::string buffer;
    // To pass if DefaultVehicleHal should refresh the property configs
    bool refreshPropertyConfigs = false;
};

// A structure to represent a set value error event reported from vehicle.
struct SetValueErrorEvent {
    aidl::android::hardware::automotive::vehicle::StatusCode errorCode;
    int32_t propId;
    int32_t areaId;
};

// An abstract interface to access vehicle hardware.
// For virtualized VHAL, GrpcVehicleHardware would communicate with a VehicleHardware
// implementation in another VM through GRPC. For non-virtualzied VHAL, VHAL directly communicates
// with a VehicleHardware through this interface.
class IVehicleHardware {
  public:
    using SetValuesCallback = std::function<void(
            std::vector<aidl::android::hardware::automotive::vehicle::SetValueResult>)>;
    using GetValuesCallback = std::function<void(
            std::vector<aidl::android::hardware::automotive::vehicle::GetValueResult>)>;
    using PropertyChangeCallback = std::function<void(
            std::vector<aidl::android::hardware::automotive::vehicle::VehiclePropValue>)>;
    using PropertySetErrorCallback = std::function<void(std::vector<SetValueErrorEvent>)>;

    virtual ~IVehicleHardware() = default;

    // Get all the property configs.
    virtual std::vector<aidl::android::hardware::automotive::vehicle::VehiclePropConfig>
    getAllPropertyConfigs() const = 0;

    // Set property values asynchronously. Server could return before the property set requests
    // are sent to vehicle bus or before property set confirmation is received. The callback is
    // safe to be called after the function returns and is safe to be called in a different thread.
    virtual aidl::android::hardware::automotive::vehicle::StatusCode setValues(
            std::shared_ptr<const SetValuesCallback> callback,
            const std::vector<aidl::android::hardware::automotive::vehicle::SetValueRequest>&
                    requests) = 0;

    // Get property values asynchronously. Server could return before the property values are ready.
    // The callback is safe to be called after the function returns and is safe to be called in a
    // different thread.
    virtual aidl::android::hardware::automotive::vehicle::StatusCode getValues(
            std::shared_ptr<const GetValuesCallback> callback,
            const std::vector<aidl::android::hardware::automotive::vehicle::GetValueRequest>&
                    requests) const = 0;

    // Update the sampling rate for the specified property and the specified areaId (0 for global
    // property) if server supports it. The property must be a continuous property.
    // {@code sampleRate} means that for this specific property, the server must generate at least
    // this many OnPropertyChange events per seconds.
    // A sampleRate of 0 means the property is no longer subscribed and server does not need to
    // generate any onPropertyEvent for this property.
    // This would be called if sample rate is updated for a subscriber, a new subscriber is added
    // or an existing subscriber is removed. For example:
    // 1. We have no subscriber for speed.
    // 2. A new subscriber is subscribing speed for 10 times/s, updsateSampleRate would be called
    //    with sampleRate as 10. The impl is now polling vehicle speed from bus 10 times/s.
    // 3. A new subscriber is subscribing speed for 5 times/s, because it is less than 10
    //    times/sec, updateSampleRate would not be called.
    // 4. The initial subscriber is removed, updateSampleRate would be called with sampleRate as
    //    5, because now it only needs to report event 5times/sec. The impl can now poll vehicle
    //    speed 5 times/s. If the impl is still polling at 10 times/s, that is okay as long as
    //    the polling rate is larger than 5times/s. DefaultVehicleHal would ignore the additional
    //    events.
    // 5. The second subscriber is removed, updateSampleRate would be called with sampleRate as 0.
    //    The impl can optionally disable the polling for vehicle speed.
    //
    // If the impl is always polling at {@code maxSampleRate} as specified in config, then this
    // function can be a no-op.
    virtual aidl::android::hardware::automotive::vehicle::StatusCode updateSampleRate(
            [[maybe_unused]] int32_t propId, [[maybe_unused]] int32_t areaId,
            [[maybe_unused]] float sampleRate) {
        return aidl::android::hardware::automotive::vehicle::StatusCode::OK;
    }

    // Dump debug information in the server.
    virtual DumpResult dump(const std::vector<std::string>& options) = 0;

    // Check whether the system is healthy, return {@code StatusCode::OK} for healthy.
    virtual aidl::android::hardware::automotive::vehicle::StatusCode checkHealth() = 0;

    // Register a callback that would be called when there is a property change event from vehicle.
    // Must only be called once during initialization.
    virtual void registerOnPropertyChangeEvent(
            std::unique_ptr<const PropertyChangeCallback> callback) = 0;

    // Register a callback that would be called when there is a property set error event from
    // vehicle. Must only be called once during initialization.
    virtual void registerOnPropertySetErrorEvent(
            std::unique_ptr<const PropertySetErrorCallback> callback) = 0;
};

}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android

#endif  // android_hardware_automotive_vehicle_aidl_impl_hardware_include_IVehicleHardware_H_
