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

#include "Obd2SensorStore.h"

#include <VehicleUtils.h>

#include <utils/SystemClock.h>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace fake {
namespace obd2frame {

using ::aidl::android::hardware::automotive::vehicle::DiagnosticFloatSensorIndex;
using ::aidl::android::hardware::automotive::vehicle::DiagnosticIntegerSensorIndex;
using ::aidl::android::hardware::automotive::vehicle::StatusCode;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropertyType;
using ::android::base::Error;
using ::android::base::Result;

Obd2SensorStore::BitmaskInVector::BitmaskInVector(size_t numBits) {
    mNumBits = numBits;
    resize(numBits);
}

void Obd2SensorStore::BitmaskInVector::resize(size_t numBits) {
    mNumBits = numBits;
    mStorage = std::vector<uint8_t>((numBits + 7) / 8, 0);
}

Result<void> Obd2SensorStore::BitmaskInVector::set(size_t index, bool value) {
    const size_t byteIndex = index / 8;
    const size_t bitIndex = index % 8;
    if (index >= mNumBits) {
        return Error() << "out of bound";
    }
    const uint8_t byte = mStorage[byteIndex];
    uint8_t newValue = value ? (byte | (1 << bitIndex)) : (byte & ~(1 << bitIndex));
    mStorage[byteIndex] = newValue;
    return {};
}

Result<bool> Obd2SensorStore::BitmaskInVector::get(size_t index) const {
    const size_t byteIndex = index / 8;
    const size_t bitIndex = index % 8;
    if (index >= mNumBits) {
        return Error() << "out of bound";
    }
    const uint8_t byte = mStorage[byteIndex];
    return (byte & (1 << bitIndex)) != 0;
}

const std::vector<uint8_t>& Obd2SensorStore::BitmaskInVector::getBitmask() const {
    return mStorage;
}

Obd2SensorStore::Obd2SensorStore(std::shared_ptr<VehiclePropValuePool> valuePool,
                                 size_t numVendorIntegerSensors, size_t numVendorFloatSensors)
    : mValuePool(valuePool) {
    const size_t numSystemIntegerSensors = getLastIndex<DiagnosticIntegerSensorIndex>() + 1;
    const size_t numSystemFloatSensors = getLastIndex<DiagnosticFloatSensorIndex>() + 1;
    mIntegerSensors = std::vector<int32_t>(numSystemIntegerSensors + numVendorIntegerSensors, 0);
    mFloatSensors = std::vector<float>(numSystemFloatSensors + numVendorFloatSensors, 0);
    mSensorsBitmask.resize(mIntegerSensors.size() + mFloatSensors.size());
}

StatusCode Obd2SensorStore::setIntegerSensor(DiagnosticIntegerSensorIndex index, int32_t value) {
    return setIntegerSensor(toInt(index), value);
}
StatusCode Obd2SensorStore::setFloatSensor(DiagnosticFloatSensorIndex index, float value) {
    return setFloatSensor(toInt(index), value);
}

StatusCode Obd2SensorStore::setIntegerSensor(size_t index, int32_t value) {
    if (index >= mIntegerSensors.size()) {
        ALOGE("failed to set integer sensor: OOB");
        return StatusCode::INVALID_ARG;
    }
    mIntegerSensors[index] = value;
    if (auto result = mSensorsBitmask.set(index, true); !result.ok()) {
        ALOGE("failed to set integer sensor: %s", result.error().message().c_str());
        return StatusCode::INVALID_ARG;
    }
    return StatusCode::OK;
}

StatusCode Obd2SensorStore::setFloatSensor(size_t index, float value) {
    if (index >= mFloatSensors.size()) {
        ALOGE("failed to set integer sensor: OOB");
        return StatusCode::INVALID_ARG;
    }
    mFloatSensors[index] = value;
    if (auto result = mSensorsBitmask.set(index + mIntegerSensors.size(), true); !result.ok()) {
        ALOGE("failed to set float sensor: %s", result.error().message().c_str());
        return StatusCode::INVALID_ARG;
    }
    return StatusCode::OK;
}

const std::vector<int32_t>& Obd2SensorStore::getIntegerSensors() const {
    return mIntegerSensors;
}

const std::vector<float>& Obd2SensorStore::getFloatSensors() const {
    return mFloatSensors;
}

const std::vector<uint8_t>& Obd2SensorStore::getSensorsBitmask() const {
    return mSensorsBitmask.getBitmask();
}

VehiclePropValuePool::RecyclableType Obd2SensorStore::getSensorProperty(
        const std::string& dtc) const {
    auto propValue = mValuePool->obtain(VehiclePropertyType::MIXED);
    propValue->timestamp = elapsedRealtimeNano();
    propValue->value.int32Values = getIntegerSensors();
    propValue->value.floatValues = getFloatSensors();
    propValue->value.byteValues = getSensorsBitmask();
    propValue->value.stringValue = dtc;
    return propValue;
}

}  // namespace obd2frame
}  // namespace fake
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
