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

#ifndef android_hardware_automotive_vehicle_V2_0_VehicleHal_H
#define android_hardware_automotive_vehicle_V2_0_VehicleHal_H

#include <android/hardware/automotive/vehicle/2.0/IVehicle.h>
#include "VehicleObjectPool.h"

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace V2_0 {

/**
 * This is a low-level vehicle hal interface that should be implemented by
 * Vendor.
 */
class VehicleHal {
public:
    using VehiclePropValuePtr = recyclable_ptr<VehiclePropValue>;

    using HalEventFunction = std::function<void(VehiclePropValuePtr)>;
    using HalErrorFunction = std::function<void(
            StatusCode errorCode, int32_t property, int32_t areaId)>;

    virtual ~VehicleHal() {}

    virtual std::vector<VehiclePropConfig> listProperties() = 0;
    virtual VehiclePropValuePtr get(const VehiclePropValue& requestedPropValue,
                                    StatusCode* outStatus) = 0;

    virtual StatusCode set(const VehiclePropValue& propValue) = 0;

    /**
     * Subscribe to HAL property events. This method might be called multiple
     * times for the same vehicle property to update sample rate.
     *
     * @param property to subscribe
     * @param sampleRate sample rate in Hz for properties that support sample
     *                   rate, e.g. for properties with
     *                   VehiclePropertyChangeMode::CONTINUOUS
     */
    virtual StatusCode subscribe(int32_t property,
                                 float sampleRate) = 0;

    /**
     * Unsubscribe from HAL events for given property
     *
     * @param property vehicle property to unsubscribe
     */
    virtual StatusCode unsubscribe(int32_t property) = 0;

    /**
     * Override this method if you need to do one-time initialization.
     */
    virtual void onCreate() {}

    /**
     * Dump method forwarded from HIDL's debug().
     *
     * By default it doesn't dump anything and let caller dump its properties, but it could be
     * override to change the behavior. For example:
     *
     * - To augment caller's dump, it should dump its state and return true.
     * - To not dump anything at all, it should just return false.
     * - To provide custom dump (like dumping just specific state or executing a custom command),
     *   it should check if options is not empty, handle the options accordingly, then return false.
     *
     * @param handle handle used to dump the contents.
     * @param options options passed to dump.
     *
     * @return whether the caller should dump its state.
     */
    virtual bool dump(const hidl_handle& /* handle */, const hidl_vec<hidl_string>& /* options */) {
        return true;
    }

    void init(
        VehiclePropValuePool* valueObjectPool,
        const HalEventFunction& onHalEvent,
        const HalErrorFunction& onHalError) {
        mValuePool = valueObjectPool;
        mOnHalEvent = onHalEvent;
        mOnHalPropertySetError = onHalError;

        onCreate();
    }

    VehiclePropValuePool* getValuePool() {
        return mValuePool;
    }
protected:
    /* Propagates property change events to vehicle HAL clients. */
    void doHalEvent(VehiclePropValuePtr v) {
        mOnHalEvent(std::move(v));
    }

    /* Propagates error during set operation to the vehicle HAL clients. */
    void doHalPropertySetError(StatusCode errorCode,
                               int32_t propId,
                               int32_t areaId) {
        mOnHalPropertySetError(errorCode, propId, areaId);
    }

private:
    HalEventFunction mOnHalEvent;
    HalErrorFunction mOnHalPropertySetError;
    VehiclePropValuePool* mValuePool;
};

}  // namespace V2_0
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android

#endif //android_hardware_automotive_vehicle_V2_0_VehicleHal_H_
