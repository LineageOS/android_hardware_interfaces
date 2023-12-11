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

#define LOG_TAG "VehiclePropertyStore"
#include <utils/Log.h>
#include <utils/SystemClock.h>

#include "VehiclePropertyStore.h"

#include <VehicleHalTypes.h>
#include <VehicleUtils.h>
#include <android-base/stringprintf.h>
#include <math/HashCombine.h>

#include <inttypes.h>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {

using ::aidl::android::hardware::automotive::vehicle::StatusCode;
using ::aidl::android::hardware::automotive::vehicle::VehicleAreaConfig;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropConfig;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropertyStatus;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropValue;
using ::android::base::Result;
using ::android::base::StringPrintf;

bool VehiclePropertyStore::RecordId::operator==(const VehiclePropertyStore::RecordId& other) const {
    return area == other.area && token == other.token;
}

std::string VehiclePropertyStore::RecordId::toString() const {
    return StringPrintf("RecordID{{.areaId=% " PRId32 ", .token=%" PRId64 "}", area, token);
}

size_t VehiclePropertyStore::RecordIdHash::operator()(RecordId const& recordId) const {
    size_t res = 0;
    hashCombine(res, recordId.area);
    hashCombine(res, recordId.token);
    return res;
}

VehiclePropertyStore::~VehiclePropertyStore() {
    std::scoped_lock<std::mutex> lockGuard(mLock);

    // Recycling record requires mValuePool, so need to recycle them before destroying mValuePool.
    mRecordsByPropId.clear();
    mValuePool.reset();
}

const VehiclePropertyStore::Record* VehiclePropertyStore::getRecordLocked(int32_t propId) const
        REQUIRES(mLock) {
    auto RecordIt = mRecordsByPropId.find(propId);
    return RecordIt == mRecordsByPropId.end() ? nullptr : &RecordIt->second;
}

VehiclePropertyStore::Record* VehiclePropertyStore::getRecordLocked(int32_t propId)
        REQUIRES(mLock) {
    auto RecordIt = mRecordsByPropId.find(propId);
    return RecordIt == mRecordsByPropId.end() ? nullptr : &RecordIt->second;
}

VehiclePropertyStore::RecordId VehiclePropertyStore::getRecordIdLocked(
        const VehiclePropValue& propValue, const VehiclePropertyStore::Record& record) const
        REQUIRES(mLock) {
    VehiclePropertyStore::RecordId recId{
            .area = isGlobalProp(propValue.prop) ? 0 : propValue.areaId, .token = 0};

    if (record.tokenFunction != nullptr) {
        recId.token = record.tokenFunction(propValue);
    }
    return recId;
}

VhalResult<VehiclePropValuePool::RecyclableType> VehiclePropertyStore::readValueLocked(
        const RecordId& recId, const Record& record) const REQUIRES(mLock) {
    if (auto it = record.values.find(recId); it != record.values.end()) {
        return mValuePool->obtain(*(it->second));
    }
    return StatusError(StatusCode::NOT_AVAILABLE)
           << "Record ID: " << recId.toString() << " is not found";
}

void VehiclePropertyStore::registerProperty(const VehiclePropConfig& config,
                                            VehiclePropertyStore::TokenFunction tokenFunc) {
    std::scoped_lock<std::mutex> g(mLock);

    mRecordsByPropId[config.prop] = Record{
            .propConfig = config,
            .tokenFunction = tokenFunc,
    };
}

VhalResult<void> VehiclePropertyStore::writeValue(VehiclePropValuePool::RecyclableType propValue,
                                                  bool updateStatus,
                                                  VehiclePropertyStore::EventMode eventMode,
                                                  bool useCurrentTimestamp) {
    bool valueUpdated = true;
    VehiclePropValue updatedValue;
    OnValueChangeCallback onValueChangeCallback = nullptr;
    OnValuesChangeCallback onValuesChangeCallback = nullptr;
    int32_t propId;
    int32_t areaId;
    {
        std::scoped_lock<std::mutex> g(mLock);

        // Must set timestamp inside the lock to make sure no other writeValue will update the
        // the timestamp to a newer one while we are writing this value.
        if (useCurrentTimestamp) {
            propValue->timestamp = elapsedRealtimeNano();
        }

        propId = propValue->prop;
        areaId = propValue->areaId;

        VehiclePropertyStore::Record* record = getRecordLocked(propId);
        if (record == nullptr) {
            return StatusError(StatusCode::INVALID_ARG)
                   << "property: " << propId << " not registered";
        }

        if (!isGlobalProp(propId) && getAreaConfig(*propValue, record->propConfig) == nullptr) {
            return StatusError(StatusCode::INVALID_ARG)
                   << "no config for property: " << propId << " area ID: " << propValue->areaId;
        }

        VehiclePropertyStore::RecordId recId = getRecordIdLocked(*propValue, *record);
        if (auto it = record->values.find(recId); it != record->values.end()) {
            const VehiclePropValue* valueToUpdate = it->second.get();
            int64_t oldTimestampNanos = valueToUpdate->timestamp;
            VehiclePropertyStatus oldStatus = valueToUpdate->status;
            // propValue is outdated and drops it.
            if (oldTimestampNanos > propValue->timestamp) {
                return StatusError(StatusCode::INVALID_ARG)
                       << "outdated timestampNanos: " << propValue->timestamp;
            }
            if (!updateStatus) {
                propValue->status = oldStatus;
            }

            valueUpdated = (valueToUpdate->value != propValue->value ||
                            valueToUpdate->status != propValue->status ||
                            valueToUpdate->prop != propValue->prop ||
                            valueToUpdate->areaId != propValue->areaId);
        } else if (!updateStatus) {
            propValue->status = VehiclePropertyStatus::AVAILABLE;
        }

        record->values[recId] = std::move(propValue);

        if (eventMode == EventMode::NEVER) {
            return {};
        }
        updatedValue = *(record->values[recId]);

        onValuesChangeCallback = mOnValuesChangeCallback;
        onValueChangeCallback = mOnValueChangeCallback;
    }

    if (onValuesChangeCallback == nullptr && onValueChangeCallback == nullptr) {
        ALOGW("No callback registered, ignoring property update for propId: %" PRId32
              ", area ID: %" PRId32,
              propId, areaId);
        return {};
    }

    // Invoke the callback outside the lock to prevent dead-lock.
    if (eventMode == EventMode::ALWAYS || valueUpdated) {
        if (onValuesChangeCallback != nullptr) {
            onValuesChangeCallback({updatedValue});
        } else {
            onValueChangeCallback(updatedValue);
        }
    }
    return {};
}

void VehiclePropertyStore::refreshTimestamp(int32_t propId, int32_t areaId, EventMode eventMode) {
    std::unordered_map<PropIdAreaId, EventMode, PropIdAreaIdHash> eventModeByPropIdAreaId;
    PropIdAreaId propIdAreaId = {
            .propId = propId,
            .areaId = areaId,
    };
    eventModeByPropIdAreaId[propIdAreaId] = eventMode;
    refreshTimestamps(eventModeByPropIdAreaId);
}

void VehiclePropertyStore::refreshTimestamps(
        std::unordered_map<PropIdAreaId, EventMode, PropIdAreaIdHash> eventModeByPropIdAreaId) {
    std::vector<VehiclePropValue> updatedValues;
    OnValuesChangeCallback onValuesChangeCallback = nullptr;
    OnValueChangeCallback onValueChangeCallback = nullptr;
    {
        std::scoped_lock<std::mutex> g(mLock);

        onValuesChangeCallback = mOnValuesChangeCallback;
        onValueChangeCallback = mOnValueChangeCallback;

        for (const auto& [propIdAreaId, eventMode] : eventModeByPropIdAreaId) {
            int32_t propId = propIdAreaId.propId;
            int32_t areaId = propIdAreaId.areaId;
            VehiclePropertyStore::Record* record = getRecordLocked(propId);
            if (record == nullptr) {
                continue;
            }

            VehiclePropValue propValue = {
                    .areaId = areaId,
                    .prop = propId,
                    .value = {},
            };

            VehiclePropertyStore::RecordId recId = getRecordIdLocked(propValue, *record);
            if (auto it = record->values.find(recId); it != record->values.end()) {
                it->second->timestamp = elapsedRealtimeNano();
                if (eventMode == EventMode::ALWAYS) {
                    updatedValues.push_back(*(it->second));
                }
            } else {
                continue;
            }
        }
    }

    // Invoke the callback outside the lock to prevent dead-lock.
    if (updatedValues.empty()) {
        return;
    }
    if (!onValuesChangeCallback && !onValueChangeCallback) {
        // If no callback is set, then we don't have to do anything.
        for (const auto& updateValue : updatedValues) {
            ALOGW("No callback registered, ignoring property update for propId: %" PRId32
                  ", area ID: %" PRId32,
                  updateValue.prop, updateValue.areaId);
        }
        return;
    }
    if (onValuesChangeCallback != nullptr) {
        onValuesChangeCallback(updatedValues);
    } else {
        // Fallback to use multiple onValueChangeCallback
        for (const auto& updateValue : updatedValues) {
            onValueChangeCallback(updateValue);
        }
    }
}

void VehiclePropertyStore::removeValue(const VehiclePropValue& propValue) {
    std::scoped_lock<std::mutex> g(mLock);

    VehiclePropertyStore::Record* record = getRecordLocked(propValue.prop);
    if (record == nullptr) {
        return;
    }

    VehiclePropertyStore::RecordId recId = getRecordIdLocked(propValue, *record);
    if (auto it = record->values.find(recId); it != record->values.end()) {
        record->values.erase(it);
    }
}

void VehiclePropertyStore::removeValuesForProperty(int32_t propId) {
    std::scoped_lock<std::mutex> g(mLock);

    VehiclePropertyStore::Record* record = getRecordLocked(propId);
    if (record == nullptr) {
        return;
    }

    record->values.clear();
}

std::vector<VehiclePropValuePool::RecyclableType> VehiclePropertyStore::readAllValues() const {
    std::scoped_lock<std::mutex> g(mLock);

    std::vector<VehiclePropValuePool::RecyclableType> allValues;

    for (auto const& [_, record] : mRecordsByPropId) {
        for (auto const& [_, value] : record.values) {
            allValues.push_back(std::move(mValuePool->obtain(*value)));
        }
    }

    return allValues;
}

VehiclePropertyStore::ValuesResultType VehiclePropertyStore::readValuesForProperty(
        int32_t propId) const {
    std::scoped_lock<std::mutex> g(mLock);

    std::vector<VehiclePropValuePool::RecyclableType> values;

    const VehiclePropertyStore::Record* record = getRecordLocked(propId);
    if (record == nullptr) {
        return StatusError(StatusCode::INVALID_ARG) << "property: " << propId << " not registered";
    }

    for (auto const& [_, value] : record->values) {
        values.push_back(std::move(mValuePool->obtain(*value)));
    }
    return values;
}

VehiclePropertyStore::ValueResultType VehiclePropertyStore::readValue(
        const VehiclePropValue& propValue) const {
    std::scoped_lock<std::mutex> g(mLock);

    int32_t propId = propValue.prop;
    const VehiclePropertyStore::Record* record = getRecordLocked(propId);
    if (record == nullptr) {
        return StatusError(StatusCode::INVALID_ARG) << "property: " << propId << " not registered";
    }

    VehiclePropertyStore::RecordId recId = getRecordIdLocked(propValue, *record);
    return readValueLocked(recId, *record);
}

VehiclePropertyStore::ValueResultType VehiclePropertyStore::readValue(int32_t propId,
                                                                      int32_t areaId,
                                                                      int64_t token) const {
    std::scoped_lock<std::mutex> g(mLock);

    const VehiclePropertyStore::Record* record = getRecordLocked(propId);
    if (record == nullptr) {
        return StatusError(StatusCode::INVALID_ARG) << "property: " << propId << " not registered";
    }

    VehiclePropertyStore::RecordId recId{.area = isGlobalProp(propId) ? 0 : areaId, .token = token};
    return readValueLocked(recId, *record);
}

std::vector<VehiclePropConfig> VehiclePropertyStore::getAllConfigs() const {
    std::scoped_lock<std::mutex> g(mLock);

    std::vector<VehiclePropConfig> configs;
    configs.reserve(mRecordsByPropId.size());
    for (auto& [_, config] : mRecordsByPropId) {
        configs.push_back(config.propConfig);
    }
    return configs;
}

VhalResult<const VehiclePropConfig*> VehiclePropertyStore::getConfig(int32_t propId) const {
    std::scoped_lock<std::mutex> g(mLock);

    const VehiclePropertyStore::Record* record = getRecordLocked(propId);
    if (record == nullptr) {
        return StatusError(StatusCode::INVALID_ARG) << "property: " << propId << " not registered";
    }

    return &record->propConfig;
}

VhalResult<VehiclePropConfig> VehiclePropertyStore::getPropConfig(int32_t propId) const {
    std::scoped_lock<std::mutex> g(mLock);

    const VehiclePropertyStore::Record* record = getRecordLocked(propId);
    if (record == nullptr) {
        return StatusError(StatusCode::INVALID_ARG) << "property: " << propId << " not registered";
    }

    return record->propConfig;
}

void VehiclePropertyStore::setOnValueChangeCallback(
        const VehiclePropertyStore::OnValueChangeCallback& callback) {
    std::scoped_lock<std::mutex> g(mLock);

    mOnValueChangeCallback = callback;
}

void VehiclePropertyStore::setOnValuesChangeCallback(
        const VehiclePropertyStore::OnValuesChangeCallback& callback) {
    std::scoped_lock<std::mutex> g(mLock);

    mOnValuesChangeCallback = callback;
}

}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
