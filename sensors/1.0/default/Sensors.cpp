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

#include "Sensors.h"
#include "convert.h"
#include "multihal.h"

#include <android-base/logging.h>

#include <sys/stat.h>

namespace android {
namespace hardware {
namespace sensors {
namespace V1_0 {
namespace implementation {

/*
 * If a multi-hal configuration file exists in the proper location,
 * return true indicating we need to use multi-hal functionality.
 */
static bool UseMultiHal() {
    const std::string& name = MULTI_HAL_CONFIG_FILE_PATH;
    struct stat buffer;
    return (stat (name.c_str(), &buffer) == 0);
}

static Result ResultFromStatus(status_t err) {
    switch (err) {
        case OK:
            return Result::OK;
        case BAD_VALUE:
            return Result::BAD_VALUE;
        case PERMISSION_DENIED:
            return Result::PERMISSION_DENIED;
        default:
            return Result::INVALID_OPERATION;
    }
}

Sensors::Sensors()
    : mInitCheck(NO_INIT),
      mSensorModule(nullptr),
      mSensorDevice(nullptr) {
    status_t err = OK;
    if (UseMultiHal()) {
        mSensorModule = ::get_multi_hal_module_info();
    } else {
        err = hw_get_module(
            SENSORS_HARDWARE_MODULE_ID,
            (hw_module_t const **)&mSensorModule);
    }
    if (mSensorModule == NULL) {
        err = UNKNOWN_ERROR;
    }

    if (err != OK) {
        LOG(ERROR) << "Couldn't load "
                   << SENSORS_HARDWARE_MODULE_ID
                   << " module ("
                   << strerror(-err)
                   << ")";

        mInitCheck = err;
        return;
    }

    err = sensors_open_1(&mSensorModule->common, &mSensorDevice);

    if (err != OK) {
        LOG(ERROR) << "Couldn't open device for module "
                   << SENSORS_HARDWARE_MODULE_ID
                   << " ("
                   << strerror(-err)
                   << ")";

        mInitCheck = err;
        return;
    }

    // Require all the old HAL APIs to be present except for injection, which
    // is considered optional.
    CHECK_GE(getHalDeviceVersion(), SENSORS_DEVICE_API_VERSION_1_3);

    mInitCheck = OK;
}

status_t Sensors::initCheck() const {
    return mInitCheck;
}

Return<void> Sensors::getSensorsList(getSensorsList_cb _hidl_cb) {
    sensor_t const *list;
    size_t count = mSensorModule->get_sensors_list(mSensorModule, &list);

    hidl_vec<SensorInfo> out;
    out.resize(count);

    for (size_t i = 0; i < count; ++i) {
        const sensor_t *src = &list[i];
        SensorInfo *dst = &out[i];

        convertFromSensor(*src, dst);
    }

    _hidl_cb(out);

    return Void();
}

int Sensors::getHalDeviceVersion() const {
    if (!mSensorDevice) {
        return -1;
    }

    return mSensorDevice->common.version;
}

Return<Result> Sensors::setOperationMode(OperationMode mode) {
    return ResultFromStatus(mSensorModule->set_operation_mode((uint32_t)mode));
}

Return<Result> Sensors::activate(
        int32_t sensor_handle, bool enabled) {
    return ResultFromStatus(
            mSensorDevice->activate(
                reinterpret_cast<sensors_poll_device_t *>(mSensorDevice),
                sensor_handle,
                enabled));
}

Return<void> Sensors::poll(int32_t maxCount, poll_cb _hidl_cb) {
    hidl_vec<Event> out;
    hidl_vec<SensorInfo> dynamicSensorsAdded;

    if (maxCount <= 0) {
        _hidl_cb(Result::BAD_VALUE, out, dynamicSensorsAdded);
        return Void();
    }

    int bufferSize = maxCount <= kPollMaxBufferSize ? maxCount : kPollMaxBufferSize;

    std::unique_ptr<sensors_event_t[]> data(new sensors_event_t[bufferSize]);

    int err = mSensorDevice->poll(
            reinterpret_cast<sensors_poll_device_t *>(mSensorDevice),
            data.get(), bufferSize);

    if (err < 0) {
        _hidl_cb(ResultFromStatus(err), out, dynamicSensorsAdded);
        return Void();
    }

    const size_t count = (size_t)err;

    for (size_t i = 0; i < count; ++i) {
        if (data[i].type != SENSOR_TYPE_DYNAMIC_SENSOR_META) {
            continue;
        }

        const dynamic_sensor_meta_event_t *dyn = &data[i].dynamic_sensor_meta;

        if (!dyn->connected) {
            continue;
        }

        CHECK(dyn->sensor != nullptr);
        CHECK_EQ(dyn->sensor->handle, dyn->handle);

        SensorInfo info;
        convertFromSensor(*dyn->sensor, &info);

        size_t numDynamicSensors = dynamicSensorsAdded.size();
        dynamicSensorsAdded.resize(numDynamicSensors + 1);
        dynamicSensorsAdded[numDynamicSensors] = info;
    }

    out.resize(count);
    convertFromSensorEvents(err, data.get(), &out);

    _hidl_cb(Result::OK, out, dynamicSensorsAdded);

    return Void();
}

Return<Result> Sensors::batch(
        int32_t sensor_handle,
        int64_t sampling_period_ns,
        int64_t max_report_latency_ns) {
    return ResultFromStatus(
            mSensorDevice->batch(
                mSensorDevice,
                sensor_handle,
                0, /*flags*/
                sampling_period_ns,
                max_report_latency_ns));
}

Return<Result> Sensors::flush(int32_t sensor_handle) {
    return ResultFromStatus(mSensorDevice->flush(mSensorDevice, sensor_handle));
}

Return<Result> Sensors::injectSensorData(const Event& event) {
    if (getHalDeviceVersion() < SENSORS_DEVICE_API_VERSION_1_4) {
        return Result::INVALID_OPERATION;
    }

    sensors_event_t out;
    convertToSensorEvent(event, &out);

    return ResultFromStatus(
            mSensorDevice->inject_sensor_data(mSensorDevice, &out));
}

Return<void> Sensors::registerDirectChannel(
        const SharedMemInfo& mem, registerDirectChannel_cb _hidl_cb) {
    //TODO(b/30985702): finish implementation
    (void) mem;
    _hidl_cb(Result::INVALID_OPERATION, -1);
    return Void();
}

Return<Result> Sensors::unregisterDirectChannel(int32_t channelHandle) {
    //TODO(b/30985702): finish implementation
    (void) channelHandle;
    return Result::INVALID_OPERATION;
}

Return<void> Sensors::configDirectReport(
        int32_t sensorHandle, int32_t channelHandle, RateLevel rate,
        configDirectReport_cb _hidl_cb) {
    //TODO(b/30985702): finish implementation
    (void) sensorHandle;
    (void) channelHandle;
    (void) rate;

    _hidl_cb(Result::INVALID_OPERATION, -1);
    return Void();
}

// static
void Sensors::convertFromSensorEvents(
        size_t count,
        const sensors_event_t *srcArray,
        hidl_vec<Event> *dstVec) {
    for (size_t i = 0; i < count; ++i) {
        const sensors_event_t &src = srcArray[i];
        Event *dst = &(*dstVec)[i];

        convertFromSensorEvent(src, dst);
    }
}

ISensors *HIDL_FETCH_ISensors(const char * /* hal */) {
    Sensors *sensors = new Sensors;
    if (sensors->initCheck() != OK) {
        delete sensors;
        sensors = nullptr;

        return nullptr;
    }

    return sensors;
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace sensors
}  // namespace hardware
}  // namespace android
