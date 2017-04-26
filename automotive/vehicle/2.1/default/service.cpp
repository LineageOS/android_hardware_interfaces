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

#define LOG_TAG "automotive.vehicle@2.1-service"
#include <android/log.h>
#include <hidl/HidlTransportSupport.h>

#include <iostream>

#include <android/hardware/automotive/vehicle/2.1/IVehicle.h>

#include <vhal_v2_0/VehicleHalManager.h>
#include <vhal_v2_0/VehiclePropertyStore.h>
#include <vhal_v2_0/EmulatedVehicleHal.h>

#include <vhal_v2_1/EmulatedVehicleHal.h>

using namespace android;
using namespace android::hardware;

namespace V2_1 = ::android::hardware::automotive::vehicle::V2_1;
namespace V2_0 = ::android::hardware::automotive::vehicle::V2_0;

using StatusCode = V2_0::StatusCode;
using VehiclePropValue = V2_0::VehiclePropValue;

/* Just wrapper that passes all calls to the provided V2_0::IVehicle object */
struct Vehicle_V2_1 : public V2_1::IVehicle {

    Vehicle_V2_1(V2_0::IVehicle* vehicle20) : mVehicle20(vehicle20) {}

    // Methods derived from IVehicle
    Return<void> getAllPropConfigs(getAllPropConfigs_cb _hidl_cb)  override {
        return mVehicle20->getAllPropConfigs(_hidl_cb);
    }

    Return<void> getPropConfigs(const hidl_vec<int32_t>& properties,
                                getPropConfigs_cb _hidl_cb)  override {
        return mVehicle20->getPropConfigs(properties, _hidl_cb);
    }

    Return<void> get(const V2_0::VehiclePropValue& requestedPropValue,
                     get_cb _hidl_cb)  override {
        return mVehicle20->get(requestedPropValue, _hidl_cb);
    }

    Return<StatusCode> set(const VehiclePropValue& value) override {
        return mVehicle20->set(value);
    }

    Return<StatusCode> subscribe(const sp<V2_0::IVehicleCallback>& callback,
                                 const hidl_vec<V2_0::SubscribeOptions>&
                                 options)  override {
        return mVehicle20->subscribe(callback, options);
    }

    Return<StatusCode> unsubscribe(const sp<V2_0::IVehicleCallback>& callback,
                                   int32_t propId)  override {
        return mVehicle20->unsubscribe(callback, propId);
    }

    Return<void> debugDump(debugDump_cb _hidl_cb = nullptr) override {
        return mVehicle20->debugDump(_hidl_cb);
    }

private:
    V2_0::IVehicle* mVehicle20;
};

int main(int /* argc */, char* /* argv */ []) {
    auto store = std::make_unique<V2_0::VehiclePropertyStore>();
    auto hal = std::make_unique<V2_1::impl::EmulatedVehicleHal>(store.get());
    auto emulator = std::make_unique<V2_0::impl::VehicleEmulator>(hal.get());
    auto vehicleManager = std::make_unique<V2_0::VehicleHalManager>(hal.get());

    Vehicle_V2_1 vehicle21(vehicleManager.get());

    configureRpcThreadpool(1, true /* callerWillJoin */);

    ALOGI("Registering as service...");
    status_t status = vehicle21.registerAsService();

    if (status != OK) {
        ALOGE("Unable to register vehicle service (%d).", status);
        return 1;
    }

    ALOGI("Ready");
    joinRpcThreadpool();
    return 1;
}
