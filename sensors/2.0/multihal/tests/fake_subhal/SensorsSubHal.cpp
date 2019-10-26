/*
 * Copyright (C) 2019 The Android Open Source Project
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

#include "SensorsSubHal.h"

#include <android/hardware/sensors/2.0/types.h>
#include <log/log.h>

ISensorsSubHal* sensorsHalGetSubHal(uint32_t* version) {
#if defined SUPPORT_CONTINUOUS_SENSORS && defined SUPPORT_ON_CHANGE_SENSORS
    static ::android::hardware::sensors::V2_0::subhal::implementation::AllSensorsSubHal subHal;
#elif defined SUPPORT_CONTINUOUS_SENSORS
    static ::android::hardware::sensors::V2_0::subhal::implementation::ContinuousSensorsSubHal
            subHal;
#elif defined SUPPORT_ON_CHANGE_SENSORS
    static ::android::hardware::sensors::V2_0::subhal::implementation::OnChangeSensorsSubHal subHal;
#else
    static ::android::hardware::sensors::V2_0::subhal::implementation::SensorsSubHal subHal;
#endif  // defined SUPPORT_CONTINUOUS_SENSORS && defined SUPPORT_ON_CHANGE_SENSORS
    *version = SUB_HAL_2_0_VERSION;
    return &subHal;
}

namespace android {
namespace hardware {
namespace sensors {
namespace V2_0 {
namespace subhal {
namespace implementation {

using ::android::hardware::Void;
using ::android::hardware::sensors::V1_0::Event;
using ::android::hardware::sensors::V1_0::OperationMode;
using ::android::hardware::sensors::V1_0::RateLevel;
using ::android::hardware::sensors::V1_0::Result;
using ::android::hardware::sensors::V1_0::SharedMemInfo;
using ::android::hardware::sensors::V2_0::SensorTimeout;
using ::android::hardware::sensors::V2_0::WakeLockQueueFlagBits;
using ::android::hardware::sensors::V2_0::implementation::ScopedWakelock;

SensorsSubHal::SensorsSubHal() : mCallback(nullptr), mNextHandle(1) {}

// Methods from ::android::hardware::sensors::V2_0::ISensors follow.
Return<void> SensorsSubHal::getSensorsList(getSensorsList_cb _hidl_cb) {
    std::vector<SensorInfo> sensors;
    for (const auto& sensor : mSensors) {
        sensors.push_back(sensor.second->getSensorInfo());
    }

    _hidl_cb(sensors);
    return Void();
}

Return<Result> SensorsSubHal::setOperationMode(OperationMode mode) {
    for (auto sensor : mSensors) {
        sensor.second->setOperationMode(mode);
    }
    mCurrentOperationMode = mode;
    return Result::OK;
}

Return<Result> SensorsSubHal::activate(int32_t sensorHandle, bool enabled) {
    auto sensor = mSensors.find(sensorHandle);
    if (sensor != mSensors.end()) {
        sensor->second->activate(enabled);
        return Result::OK;
    }
    return Result::BAD_VALUE;
}

Return<Result> SensorsSubHal::batch(int32_t sensorHandle, int64_t samplingPeriodNs,
                                    int64_t /* maxReportLatencyNs */) {
    auto sensor = mSensors.find(sensorHandle);
    if (sensor != mSensors.end()) {
        sensor->second->batch(samplingPeriodNs);
        return Result::OK;
    }
    return Result::BAD_VALUE;
}

Return<Result> SensorsSubHal::flush(int32_t sensorHandle) {
    auto sensor = mSensors.find(sensorHandle);
    if (sensor != mSensors.end()) {
        return sensor->second->flush();
    }
    return Result::BAD_VALUE;
}

Return<Result> SensorsSubHal::injectSensorData(const Event& event) {
    auto sensor = mSensors.find(event.sensorHandle);
    if (sensor != mSensors.end()) {
        return sensor->second->injectEvent(event);
    }

    return Result::BAD_VALUE;
}

Return<void> SensorsSubHal::registerDirectChannel(const SharedMemInfo& /* mem */,
                                                  registerDirectChannel_cb _hidl_cb) {
    _hidl_cb(Result::INVALID_OPERATION, -1 /* channelHandle */);
    return Return<void>();
}

Return<Result> SensorsSubHal::unregisterDirectChannel(int32_t /* channelHandle */) {
    return Result::INVALID_OPERATION;
}

Return<void> SensorsSubHal::configDirectReport(int32_t /* sensorHandle */,
                                               int32_t /* channelHandle */, RateLevel /* rate */,
                                               configDirectReport_cb _hidl_cb) {
    _hidl_cb(Result::INVALID_OPERATION, 0 /* reportToken */);
    return Return<void>();
}

Return<void> SensorsSubHal::debug(const hidl_handle& fd, const hidl_vec<hidl_string>& args) {
    if (fd.getNativeHandle() == nullptr || fd->numFds < 1) {
        ALOGE("%s: missing fd for writing", __FUNCTION__);
        return Void();
    }

    FILE* out = fdopen(dup(fd->data[0]), "w");

    if (args.size() != 0) {
        fprintf(out,
                "Note: sub-HAL %s currently does not support args. Input arguments are "
                "ignored.\n",
                getName().c_str());
    }

    std::ostringstream stream;
    stream << "Available sensors:" << std::endl;
    for (auto sensor : mSensors) {
        SensorInfo info = sensor.second->getSensorInfo();
        stream << "Name: " << info.name << std::endl;
        stream << "Min delay: " << info.minDelay << std::endl;
        stream << "Flags: " << info.flags << std::endl;
    }
    stream << std::endl;

    fprintf(out, "%s", stream.str().c_str());

    fclose(out);
    return Return<void>();
}

Return<Result> SensorsSubHal::initialize(const sp<IHalProxyCallback>& halProxyCallback) {
    mCallback = halProxyCallback;
    setOperationMode(OperationMode::NORMAL);
    return Result::OK;
}

void SensorsSubHal::postEvents(const std::vector<Event>& events, bool wakeup) {
    ScopedWakelock wakelock = mCallback->createScopedWakelock(wakeup);
    mCallback->postEvents(events, std::move(wakelock));
}

ContinuousSensorsSubHal::ContinuousSensorsSubHal() {
    AddSensor<AccelSensor>();
    AddSensor<GyroSensor>();
    AddSensor<MagnetometerSensor>();
    AddSensor<PressureSensor>();
    AddSensor<DeviceTempSensor>();
}

OnChangeSensorsSubHal::OnChangeSensorsSubHal() {
    AddSensor<AmbientTempSensor>();
    AddSensor<LightSensor>();
    AddSensor<ProximitySensor>();
    AddSensor<RelativeHumiditySensor>();
}

AllSensorsSubHal::AllSensorsSubHal() {
    AddSensor<AccelSensor>();
    AddSensor<GyroSensor>();
    AddSensor<MagnetometerSensor>();
    AddSensor<PressureSensor>();
    AddSensor<DeviceTempSensor>();
    AddSensor<AmbientTempSensor>();
    AddSensor<LightSensor>();
    AddSensor<ProximitySensor>();
    AddSensor<RelativeHumiditySensor>();
}

Return<Result> SetOperationModeFailingSensorsSubHal::setOperationMode(OperationMode /*mode*/) {
    return Result::BAD_VALUE;
}

Return<void> AllSupportDirectChannelSensorsSubHal::getSensorsList(getSensorsList_cb _hidl_cb) {
    std::vector<SensorInfo> sensors;
    for (const auto& sensor : mSensors) {
        SensorInfo sensorInfo = sensor.second->getSensorInfo();
        sensorInfo.flags |= V1_0::SensorFlagBits::MASK_DIRECT_CHANNEL;
        sensorInfo.flags |= V1_0::SensorFlagBits::MASK_DIRECT_REPORT;
        sensors.push_back(sensorInfo);
    }
    _hidl_cb(sensors);
    return Void();
}

Return<void> DoesNotSupportDirectChannelSensorsSubHal::getSensorsList(getSensorsList_cb _hidl_cb) {
    std::vector<SensorInfo> sensors;
    for (const auto& sensor : mSensors) {
        SensorInfo sensorInfo = sensor.second->getSensorInfo();
        sensorInfo.flags &= ~static_cast<uint32_t>(V1_0::SensorFlagBits::MASK_DIRECT_CHANNEL);
        sensorInfo.flags &= ~static_cast<uint32_t>(V1_0::SensorFlagBits::MASK_DIRECT_REPORT);
        sensors.push_back(sensorInfo);
    }
    _hidl_cb(sensors);
    return Void();
}

void AddAndRemoveDynamicSensorsSubHal::addDynamicSensors(
        const std::vector<SensorInfo>& sensorsAdded) {
    mCallback->onDynamicSensorsConnected(sensorsAdded);
}

void AddAndRemoveDynamicSensorsSubHal::removeDynamicSensors(
        const std::vector<int32_t>& sensorHandlesRemoved) {
    mCallback->onDynamicSensorsDisconnected(sensorHandlesRemoved);
}

}  // namespace implementation
}  // namespace subhal
}  // namespace V2_0
}  // namespace sensors
}  // namespace hardware
}  // namespace android
