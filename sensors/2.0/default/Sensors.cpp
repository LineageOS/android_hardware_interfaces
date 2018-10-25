/*
 * Copyright (C) 2018 The Android Open Source Project
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

#include "Sensors.h"

namespace android {
namespace hardware {
namespace sensors {
namespace V2_0 {
namespace implementation {

using ::android::hardware::sensors::V1_0::Event;
using ::android::hardware::sensors::V1_0::OperationMode;
using ::android::hardware::sensors::V1_0::RateLevel;
using ::android::hardware::sensors::V1_0::Result;
using ::android::hardware::sensors::V1_0::SharedMemInfo;

// Methods from ::android::hardware::sensors::V2_0::ISensors follow.
Return<void> Sensors::getSensorsList(getSensorsList_cb /* _hidl_cb */) {
    // TODO implement
    return Void();
}

Return<Result> Sensors::setOperationMode(OperationMode /* mode */) {
    // TODO implement
    return Result{};
}

Return<Result> Sensors::activate(int32_t /* sensorHandle */, bool /* enabled */) {
    // TODO implement
    return Result{};
}

Return<Result> Sensors::initialize(
    const ::android::hardware::MQDescriptorSync<Event>& /* eventQueueDescriptor */,
    const ::android::hardware::MQDescriptorSync<uint32_t>& /* wakeLockDescriptor */,
    const sp<ISensorsCallback>& /* sensorsCallback */) {
    // TODO implement
    return Result{};
}

Return<Result> Sensors::batch(int32_t /* sensorHandle */, int64_t /* samplingPeriodNs */,
                              int64_t /* maxReportLatencyNs */) {
    // TODO implement
    return Result{};
}

Return<Result> Sensors::flush(int32_t /* sensorHandle */) {
    // TODO implement
    return Result{};
}

Return<Result> Sensors::injectSensorData(const Event& /* event */) {
    // TODO implement
    return Result{};
}

Return<void> Sensors::registerDirectChannel(const SharedMemInfo& /* mem */,
                                            registerDirectChannel_cb /* _hidl_cb */) {
    // TODO implement
    return Void();
}

Return<Result> Sensors::unregisterDirectChannel(int32_t /* channelHandle */) {
    // TODO implement
    return Result{};
}

Return<void> Sensors::configDirectReport(int32_t /* sensorHandle */, int32_t /* channelHandle */,
                                         RateLevel /* rate */,
                                         configDirectReport_cb /* _hidl_cb */) {
    // TODO implement
    return Void();
}

}  // namespace implementation
}  // namespace V2_0
}  // namespace sensors
}  // namespace hardware
}  // namespace android
