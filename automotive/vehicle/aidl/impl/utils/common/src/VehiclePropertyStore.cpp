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

#include "VehiclePropertyStore.h"

#include <VehicleUtils.h>
#include <android-base/format.h>
#include <math/HashCombine.h>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {

using ::aidl::android::hardware::automotive::vehicle::VehicleAreaConfig;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropConfig;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropertyStatus;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropValue;
using ::android::base::Result;

bool VehiclePropertyStore::RecordId::operator==(const VehiclePropertyStore::RecordId& other) const {
    return area == other.area && token == other.token;
}

std::string VehiclePropertyStore::RecordId::toString() const {
    return ::fmt::format("RecordID{{.areaId={:d}, .token={:d}}}", area, token);
}

size_t VehiclePropertyStore::RecordIdHash::operator()(RecordId const& recordId) const {
    size_t res = 0;
    hashCombine(res, recordId.area);
    hashCombine(res, recordId.token);
    return res;
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

Result<VehiclePropValuePool::RecyclableType> VehiclePropertyStore::readValueLocked(
        const RecordId& recId, const Record& record) const REQUIRES(mLock) {
    auto it = record.values.find(recId);
    if (it == record.values.end()) {
        return Errorf("Record ID: {} is not found", recId.toString());
    }
    return mValuePool->obtain(*(it->second));
}

void VehiclePropertyStore::registerProperty(const VehiclePropConfig& config,
                                            VehiclePropertyStore::TokenFunction tokenFunc) {
    std::lock_guard<std::mutex> g(mLock);

    mRecordsByPropId[config.prop] = Record{
            .propConfig = config,
            .tokenFunction = tokenFunc,
    };
}

Result<void> VehiclePropertyStore::writeValue(VehiclePropValuePool::RecyclableType propValue,
                                              bool updateStatus) {
    std::lock_guard<std::mutex> g(mLock);

    VehiclePropertyStore::Record* record = getRecordLocked(propValue->prop);
    if (record == nullptr) {
        return Errorf("property: {:d} not registered", propValue->prop);
    }

    if (!isGlobalProp(propValue->prop) &&
        getAreaConfig(*propValue, record->propConfig) == nullptr) {
        return Errorf("no config for property: {:d} area: {:d}", propValue->prop,
                      propValue->areaId);
    }

    VehiclePropertyStore::RecordId recId = getRecordIdLocked(*propValue, *record);
    auto it = record->values.find(recId);
    if (it == record->values.end()) {
        record->values[recId] = std::move(propValue);
        if (!updateStatus) {
            record->values[recId]->status = VehiclePropertyStatus::AVAILABLE;
        }
        return {};
    }
    const VehiclePropValue* valueToUpdate = it->second.get();
    long oldTimestamp = valueToUpdate->timestamp;
    VehiclePropertyStatus oldStatus = valueToUpdate->status;
    // propValue is outdated and drops it.
    if (oldTimestamp > propValue->timestamp) {
        return Errorf("outdated timestamp: {:d}", propValue->timestamp);
    }
    record->values[recId] = std::move(propValue);
    if (!updateStatus) {
        record->values[recId]->status = oldStatus;
    }

    return {};
}

void VehiclePropertyStore::removeValue(const VehiclePropValue& propValue) {
    std::lock_guard<std::mutex> g(mLock);

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
    std::lock_guard<std::mutex> g(mLock);

    VehiclePropertyStore::Record* record = getRecordLocked(propId);
    if (record == nullptr) {
        return;
    }

    record->values.clear();
}

std::vector<VehiclePropValuePool::RecyclableType> VehiclePropertyStore::readAllValues() const {
    std::lock_guard<std::mutex> g(mLock);

    std::vector<VehiclePropValuePool::RecyclableType> allValues;

    for (auto const& [_, record] : mRecordsByPropId) {
        for (auto const& [_, value] : record.values) {
            allValues.push_back(std::move(mValuePool->obtain(*value)));
        }
    }

    return allValues;
}

Result<std::vector<VehiclePropValuePool::RecyclableType>>
VehiclePropertyStore::readValuesForProperty(int32_t propId) const {
    std::lock_guard<std::mutex> g(mLock);

    std::vector<VehiclePropValuePool::RecyclableType> values;

    const VehiclePropertyStore::Record* record = getRecordLocked(propId);
    if (record == nullptr) {
        return Errorf("property: {:d} not registered", propId);
    }

    for (auto const& [_, value] : record->values) {
        values.push_back(std::move(mValuePool->obtain(*value)));
    }
    return values;
}

Result<VehiclePropValuePool::RecyclableType> VehiclePropertyStore::readValue(
        const VehiclePropValue& propValue) const {
    std::lock_guard<std::mutex> g(mLock);

    const VehiclePropertyStore::Record* record = getRecordLocked(propValue.prop);
    if (record == nullptr) {
        return Errorf("property: {:d} not registered", propValue.prop);
    }

    VehiclePropertyStore::RecordId recId = getRecordIdLocked(propValue, *record);
    return readValueLocked(recId, *record);
}

Result<VehiclePropValuePool::RecyclableType> VehiclePropertyStore::readValue(int32_t propId,
                                                                             int32_t areaId,
                                                                             int64_t token) const {
    std::lock_guard<std::mutex> g(mLock);

    const VehiclePropertyStore::Record* record = getRecordLocked(propId);
    if (record == nullptr) {
        return Errorf("property: {:d} not registered", propId);
    }

    VehiclePropertyStore::RecordId recId{.area = isGlobalProp(propId) ? 0 : areaId, .token = token};
    return readValueLocked(recId, *record);
}

std::vector<VehiclePropConfig> VehiclePropertyStore::getAllConfigs() const {
    std::lock_guard<std::mutex> g(mLock);

    std::vector<VehiclePropConfig> configs;
    configs.reserve(mRecordsByPropId.size());
    for (auto& [_, config] : mRecordsByPropId) {
        configs.push_back(config.propConfig);
    }
    return configs;
}

Result<const VehiclePropConfig*> VehiclePropertyStore::getConfig(int32_t propId) const {
    std::lock_guard<std::mutex> g(mLock);

    const VehiclePropertyStore::Record* record = getRecordLocked(propId);
    if (record == nullptr) {
        return Errorf("property: {:d} not registered", propId);
    }

    return &record->propConfig;
}

}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
