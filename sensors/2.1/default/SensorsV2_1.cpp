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

#include "SensorsV2_1.h"

#include "Sensor.h"

namespace android {
namespace hardware {
namespace sensors {
namespace V2_1 {
namespace implementation {

using V2_X::implementation::ISensorsEventCallback;
using V2_X::implementation::OnChangeSensor;

class HingeAngleSensor : public OnChangeSensor {
  public:
    HingeAngleSensor(int32_t sensorHandle, ISensorsEventCallback* callback)
        : OnChangeSensor(callback) {
        mSensorInfo.sensorHandle = sensorHandle;
        mSensorInfo.name = "Hinge Angle Sensor";
        mSensorInfo.vendor = "Vendor String";
        mSensorInfo.version = 1;
        mSensorInfo.type = SensorType::HINGE_ANGLE;
        mSensorInfo.typeAsString = "";
        mSensorInfo.maxRange = 360.0f;
        mSensorInfo.resolution = 1.0f;
        mSensorInfo.power = 0.001f;
        mSensorInfo.minDelay = 40 * 1000;  // microseconds
        mSensorInfo.maxDelay = V2_X::implementation::kDefaultMaxDelayUs;
        mSensorInfo.fifoReservedEventCount = 0;
        mSensorInfo.fifoMaxEventCount = 0;
        mSensorInfo.requiredPermission = "";
        mSensorInfo.flags = static_cast<uint32_t>(V1_0::SensorFlagBits::ON_CHANGE_MODE);
    }
};

SensorsV2_1::SensorsV2_1() {
    AddSensor<HingeAngleSensor>();
}

// Methods from ::android::hardware::sensors::V2_1::ISensors follow.
Return<void> SensorsV2_1::getSensorsList_2_1(ISensors::getSensorsList_2_1_cb _hidl_cb) {
    std::vector<SensorInfo> sensors;
    for (const auto& sensor : mSensors) {
        sensors.push_back(sensor.second->getSensorInfo());
    }

    // Call the HIDL callback with the SensorInfo
    _hidl_cb(sensors);

    return Void();
}

Return<Result> SensorsV2_1::initialize_2_1(
        const ::android::hardware::MQDescriptorSync<V2_1::Event>& eventQueueDescriptor,
        const ::android::hardware::MQDescriptorSync<uint32_t>& wakeLockDescriptor,
        const sp<V2_1::ISensorsCallback>& sensorsCallback) {
    auto eventQueue = std::make_unique<MessageQueue<V2_1::Event, kSynchronizedReadWrite>>(
            eventQueueDescriptor, true /* resetPointers */);
    std::unique_ptr<EventMessageQueueWrapperBase> wrapper =
            std::make_unique<EventMessageQueueWrapperV2_1>(eventQueue);
    mCallbackWrapper = new ISensorsCallbackWrapper(sensorsCallback);
    return initializeBase(wrapper, wakeLockDescriptor, mCallbackWrapper);
}

Return<Result> SensorsV2_1::injectSensorData_2_1(const V2_1::Event& event) {
    return injectSensorData(convertToOldEvent(event));
}

}  // namespace implementation
}  // namespace V2_1
}  // namespace sensors
}  // namespace hardware
}  // namespace android