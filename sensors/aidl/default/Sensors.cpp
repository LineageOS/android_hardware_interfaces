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

#include "sensors-impl/Sensors.h"

using ::aidl::android::hardware::common::fmq::MQDescriptor;
using ::aidl::android::hardware::common::fmq::SynchronizedReadWrite;
using ::aidl::android::hardware::sensors::Event;
using ::aidl::android::hardware::sensors::ISensors;
using ::aidl::android::hardware::sensors::ISensorsCallback;
using ::aidl::android::hardware::sensors::SensorInfo;

namespace aidl {
namespace android {
namespace hardware {
namespace sensors {

// TODO(b/195593357): Implement AIDL HAL
::ndk::ScopedAStatus Sensors::activate(int32_t /* in_sensorHandle */, bool /* in_enabled */) {
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Sensors::batch(int32_t /* in_sensorHandle */,
                                    int64_t /* in_samplingPeriodNs */,
                                    int64_t /* in_maxReportLatencyNs */) {
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Sensors::configDirectReport(int32_t /* in_sensorHandle */,
                                                 int32_t /* in_channelHandle */,
                                                 ISensors::RateLevel /* in_rate */,
                                                 int32_t* /* _aidl_return */) {
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Sensors::flush(int32_t /* in_sensorHandle */) {
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Sensors::getSensorsList(std::vector<SensorInfo>* /* _aidl_return */) {
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Sensors::initialize(
        const MQDescriptor<Event, SynchronizedReadWrite>& /* in_eventQueueDescriptor */,
        const MQDescriptor<int32_t, SynchronizedReadWrite>& /* in_wakeLockDescriptor */,
        const std::shared_ptr<ISensorsCallback>& /* in_sensorsCallback */) {
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Sensors::injectSensorData(const Event& /* in_event */) {
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Sensors::registerDirectChannel(const ISensors::SharedMemInfo& /* in_mem */,
                                                    int32_t* /* _aidl_return */) {
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Sensors::setOperationMode(OperationMode /* in_mode */) {
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Sensors::unregisterDirectChannel(int32_t /* in_channelHandle */) {
    return ndk::ScopedAStatus::ok();
}

}  // namespace sensors
}  // namespace hardware
}  // namespace android
}  // namespace aidl
