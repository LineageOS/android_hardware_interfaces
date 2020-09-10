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

#ifndef ANDROID_HARDWARE_SENSORS_V2_1_H
#define ANDROID_HARDWARE_SENSORS_V2_1_H

#include "Sensors.h"

#include "EventMessageQueueWrapper.h"

#include <android/hardware/sensors/2.1/ISensors.h>

namespace android {
namespace hardware {
namespace sensors {
namespace V2_1 {
namespace implementation {

using Result = ::android::hardware::sensors::V1_0::Result;
using Sensors = ::android::hardware::sensors::V2_X::implementation::Sensors<ISensors>;

class ISensorsCallbackWrapper : public V2_0::ISensorsCallback {
  public:
    ISensorsCallbackWrapper(const sp<V2_1::ISensorsCallback>& callback) : mCallback(callback) {}

    Return<void> onDynamicSensorsConnected(const hidl_vec<V1_0::SensorInfo>& sensorInfos) override {
        return mCallback->onDynamicSensorsConnected_2_1(convertToNewSensorInfos(sensorInfos));
    }

    Return<void> onDynamicSensorsDisconnected(const hidl_vec<int32_t>& sensorHandles) override {
        return mCallback->onDynamicSensorsDisconnected(sensorHandles);
    }

  private:
    sp<V2_1::ISensorsCallback> mCallback;
};

struct SensorsV2_1 : public Sensors {
    SensorsV2_1();

    // Methods from ::android::hardware::sensors::V2_1::ISensors follow.
    Return<void> getSensorsList_2_1(ISensors::getSensorsList_2_1_cb _hidl_cb) override;

    Return<Result> initialize_2_1(
            const ::android::hardware::MQDescriptorSync<V2_1::Event>& eventQueueDescriptor,
            const ::android::hardware::MQDescriptorSync<uint32_t>& wakeLockDescriptor,
            const sp<V2_1::ISensorsCallback>& sensorsCallback) override;

    Return<Result> injectSensorData_2_1(const V2_1::Event& event) override;

  private:
    sp<ISensorsCallbackWrapper> mCallbackWrapper;
};

}  // namespace implementation
}  // namespace V2_1
}  // namespace sensors
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_SENSORS_V2_1_H