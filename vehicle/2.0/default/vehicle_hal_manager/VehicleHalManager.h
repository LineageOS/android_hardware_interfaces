/*
 * Copyright (C) 2015 The Android Open Source Project
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

#ifndef android_hardware_vehicle_V2_0_VehicleHalManager_H_
#define android_hardware_vehicle_V2_0_VehicleHalManager_H_

#include <stdint.h>
#include <inttypes.h>
#include <sys/types.h>
#include <memory>
#include <map>
#include <set>
#include <list>

#include <hwbinder/IPCThreadState.h>

#include <android/hardware/vehicle/2.0/IVehicle.h>

#include "VehicleHal.h"
#include "VehiclePropConfigIndex.h"
#include "ConcurrentQueue.h"
#include "SubscriptionManager.h"
#include "VehicleObjectPool.h"

namespace android {
namespace hardware {
namespace vehicle {
namespace V2_0 {

/**
 * This class is a thick proxy between IVehicle HIDL interface and vendor's implementation.
 *
 * It has some boilerplate code like batching and caching property values, checking permissions,
 * etc. Vendors must implement VehicleHal class.
 */
class VehicleHalManager : public IVehicle {
public:
    VehicleHalManager(VehicleHal* vehicleHal)
        : mHal(vehicleHal) {
        init();
    }

    virtual ~VehicleHalManager();

    void init();

    // ---------------------------------------------------------------------------------------------
    // Methods derived from IVehicle
    Return<void> getAllPropConfigs(getAllPropConfigs_cb _hidl_cb)  override;
    Return<void> getPropConfigs(const hidl_vec<VehicleProperty>& properties,
                                getPropConfigs_cb _hidl_cb)  override;
    Return<void> get(VehicleProperty propId, int32_t areaId, get_cb _hidl_cb)  override;
    Return<StatusCode> set(const VehiclePropValue& value)  override;
    Return<StatusCode> subscribe(const sp<IVehicleCallback>& callback,
                                const hidl_vec<SubscribeOptions>& options)  override;
    Return<StatusCode> unsubscribe(const sp<IVehicleCallback>& callback,
                                   VehicleProperty propId)  override;
    Return<void> debugDump(debugDump_cb _hidl_cb = nullptr) override;

private:
    using VehiclePropValuePtr = VehicleHal::VehiclePropValuePtr;

    // ---------------------------------------------------------------------------------------------
    // Events received from VehicleHal
    void onHalEvent(VehiclePropValuePtr  v);
    void onHalError(VehicleProperty property,
                    status_t errorCode,
                    VehiclePropertyOperation operation);

    // ---------------------------------------------------------------------------------------------
    // This method will be called from BatchingConsumer thread
    void onBatchHalEvent(const std::vector<VehiclePropValuePtr >& values);

    static bool isSubscribable(const VehiclePropConfig& config);
    static bool isSampleRateFixed(VehiclePropertyChangeMode mode);
    static float checkSampleRate(const VehiclePropConfig& config,
                                 float sampleRate);

    static Return<StatusCode> ok() {
        return Return<StatusCode>(StatusCode::OK);
    }
    static Return<StatusCode> invalidArg() {
        return Return<StatusCode>(StatusCode::INVALID_ARG);
    }

private:
    VehicleHal* mHal;
    std::unique_ptr<VehiclePropConfigIndex> mConfigIndex;
    SubscriptionManager mSubscriptionManager;

    hidl_vec<VehiclePropValue> mHidlVecOfVehiclePropValuePool;

    ConcurrentQueue<VehiclePropValuePtr> mEventQueue;
    BatchingConsumer<VehiclePropValuePtr> mBatchingConsumer;
    VehiclePropValuePool mValueObjectPool;
};

}  // namespace V2_0
}  // namespace vehicle
}  // namespace hardware
}  // namespace android


#endif // android_hardware_vehicle_V2_0_VehicleHalManager_H_
