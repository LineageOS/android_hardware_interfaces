/*
 * Copyright (C) 2020 The Android Open Source Project
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

#ifndef ANDROID_HARDWARE_SENSORS_V2_1_ISENSORSCALLBACKWRAPPER_H
#define ANDROID_HARDWARE_SENSORS_V2_1_ISENSORSCALLBACKWRAPPER_H

#include "convertV2_1.h"

#include "android/hardware/sensors/1.0/ISensors.h"
#include "android/hardware/sensors/1.0/types.h"
#include "android/hardware/sensors/2.0/ISensors.h"
#include "android/hardware/sensors/2.0/ISensorsCallback.h"
#include "android/hardware/sensors/2.1/ISensors.h"
#include "android/hardware/sensors/2.1/ISensorsCallback.h"
#include "android/hardware/sensors/2.1/types.h"

#include <utils/LightRefBase.h>

#include <cassert>

namespace android {
namespace hardware {
namespace sensors {
namespace V2_1 {
namespace implementation {

/**
 * The ISensorsCallbackWrapper classes below abstract away the common logic between both the V2.0
 * and V2.1 versions of the Sensors HAL interface. This allows users of these classes to only care
 * about the HAL version at init time and then interact with either version of the callback without
 * worrying about the class type by utilizing the base class.
 */
class ISensorsCallbackWrapperBase : public VirtualLightRefBase {
  public:
    virtual Return<void> onDynamicSensorsConnected(
            const hidl_vec<V2_1::SensorInfo>& sensorInfos) = 0;

    virtual Return<void> onDynamicSensorsDisconnected(const hidl_vec<int32_t>& sensorHandles) = 0;
};

template <typename T>
class SensorsCallbackWrapperBase : public ISensorsCallbackWrapperBase {
  public:
    SensorsCallbackWrapperBase(sp<T> sensorsCallback) : mSensorsCallback(sensorsCallback){};

    virtual Return<void> onDynamicSensorsConnected(
            const hidl_vec<V2_1::SensorInfo>& sensorInfos) override {
        return mSensorsCallback->onDynamicSensorsConnected(convertToOldSensorInfos(sensorInfos));
    }

    Return<void> onDynamicSensorsDisconnected(const hidl_vec<int32_t>& sensorHandles) {
        return mSensorsCallback->onDynamicSensorsDisconnected(sensorHandles);
    }

  protected:
    sp<T> mSensorsCallback;
};

class ISensorsCallbackWrapperV2_0
    : public SensorsCallbackWrapperBase<hardware::sensors::V2_0::ISensorsCallback> {
  public:
    ISensorsCallbackWrapperV2_0(sp<hardware::sensors::V2_0::ISensorsCallback> sensorsCallback)
        : SensorsCallbackWrapperBase(sensorsCallback){};
};

class ISensorsCallbackWrapperV2_1
    : public SensorsCallbackWrapperBase<hardware::sensors::V2_1::ISensorsCallback> {
  public:
    ISensorsCallbackWrapperV2_1(sp<hardware::sensors::V2_1::ISensorsCallback> sensorsCallback)
        : SensorsCallbackWrapperBase(sensorsCallback) {}

    Return<void> onDynamicSensorsConnected(const hidl_vec<V2_1::SensorInfo>& sensorInfos) override {
        return mSensorsCallback->onDynamicSensorsConnected_2_1(sensorInfos);
    }
};

}  // namespace implementation
}  // namespace V2_1
}  // namespace sensors
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_SENSORS_V2_1_ISENSORSCALLBACKWRAPPER_H