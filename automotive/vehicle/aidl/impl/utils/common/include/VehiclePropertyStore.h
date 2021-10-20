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

#ifndef android_hardware_automotive_vehicle_aidl_impl_utils_common_include_VehiclePropertyStore_H_
#define android_hardware_automotive_vehicle_aidl_impl_utils_common_include_VehiclePropertyStore_H_

#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <unordered_map>

#include <VehicleHalTypes.h>
#include <VehicleObjectPool.h>
#include <android-base/result.h>
#include <android-base/thread_annotations.h>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {

// Encapsulates work related to storing and accessing configuration, storing and modifying
// vehicle property values.
//
// VehiclePropertyValues stored in a sorted map thus it makes easier to get range of values, e.g.
// to get value for all areas for particular property.
//
// This class is thread-safe, however it uses blocking synchronization across all methods.
class VehiclePropertyStore final {
  public:
    explicit VehiclePropertyStore(std::shared_ptr<VehiclePropValuePool> valuePool)
        : mValuePool(valuePool) {}

    ~VehiclePropertyStore();

    // Callback when a property value has been updated or a new value added.
    using OnValueChangeCallback = std::function<void(
            const ::aidl::android::hardware::automotive::vehicle::VehiclePropValue&)>;

    // Function that used to calculate unique token for given VehiclePropValue.
    using TokenFunction = ::std::function<int64_t(
            const ::aidl::android::hardware::automotive::vehicle::VehiclePropValue& value)>;

    // Register the given property according to the config. A property has to be registered first
    // before write/read. If tokenFunc is not nullptr, it would be used to generate a unique
    // property token to act as the key the property store. Otherwise, {propertyID, areaID} would be
    // used as the key.
    void registerProperty(
            const ::aidl::android::hardware::automotive::vehicle::VehiclePropConfig& config,
            TokenFunction tokenFunc = nullptr);

    // Stores provided value. Returns error if config wasn't registered. If 'updateStatus' is
    // true, the 'status' in 'propValue' would be stored. Otherwise, if this is a new value,
    // 'status' would be initialized to {@code VehiclePropertyStatus::AVAILABLE}, if this is to
    // override an existing value, the status for the existing value would be used for the
    // overridden value.
    ::android::base::Result<void> writeValue(VehiclePropValuePool::RecyclableType propValue,
                                             bool updateStatus = false);

    // Remove a given property value from the property store. The 'propValue' would be used to
    // generate the key for the value to remove.
    void removeValue(
            const ::aidl::android::hardware::automotive::vehicle::VehiclePropValue& propValue);

    // Remove all the values for the property.
    void removeValuesForProperty(int32_t propId);

    // Read all the stored values.
    std::vector<VehiclePropValuePool::RecyclableType> readAllValues() const;

    // Read all the values for the property.
    ::android::base::Result<std::vector<VehiclePropValuePool::RecyclableType>>
    readValuesForProperty(int32_t propId) const;

    // Read the value for the requested property.
    ::android::base::Result<VehiclePropValuePool::RecyclableType> readValue(
            const ::aidl::android::hardware::automotive::vehicle::VehiclePropValue& request) const;

    // Read the value for the requested property.
    ::android::base::Result<VehiclePropValuePool::RecyclableType> readValue(
            int32_t prop, int32_t area = 0, int64_t token = 0) const;

    // Get all property configs.
    std::vector<::aidl::android::hardware::automotive::vehicle::VehiclePropConfig> getAllConfigs()
            const;

    // Get the property config for the requested property.
    ::android::base::Result<
            const ::aidl::android::hardware::automotive::vehicle::VehiclePropConfig*>
    getConfig(int32_t propId) const;

    // Set a callback that would be called when a property value has been updated.
    void setOnValueChangeCallback(const OnValueChangeCallback& callback);

  private:
    struct RecordId {
        int32_t area;
        int64_t token;

        std::string toString() const;

        bool operator==(const RecordId& other) const;
    };

    struct RecordIdHash {
        size_t operator()(RecordId const& recordId) const;
    };

    struct Record {
        ::aidl::android::hardware::automotive::vehicle::VehiclePropConfig propConfig;
        TokenFunction tokenFunction;
        std::unordered_map<RecordId, VehiclePropValuePool::RecyclableType, RecordIdHash> values;
    };

    // {@code VehiclePropValuePool} is thread-safe.
    std::shared_ptr<VehiclePropValuePool> mValuePool;
    mutable std::mutex mLock;
    std::unordered_map<int32_t, Record> mRecordsByPropId GUARDED_BY(mLock);
    OnValueChangeCallback mOnValueChangeCallback GUARDED_BY(mLock);

    const Record* getRecordLocked(int32_t propId) const;

    Record* getRecordLocked(int32_t propId);

    RecordId getRecordIdLocked(
            const ::aidl::android::hardware::automotive::vehicle::VehiclePropValue& propValue,
            const Record& record) const;

    ::android::base::Result<VehiclePropValuePool::RecyclableType> readValueLocked(
            const RecordId& recId, const Record& record) const;
};

}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android

#endif  // android_hardware_automotive_vehicle_aidl_impl_utils_common_include_VehiclePropertyStore_H_
