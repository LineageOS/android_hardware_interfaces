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
class VehiclePropertyStore {
  public:
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

    // Stores provided value. Returns true if value was written returns false if config wasn't
    // registered.
    ::android::base::Result<void> writeValue(
            const ::aidl::android::hardware::automotive::vehicle::VehiclePropValue& propValue);

    // Remove a given property value from the property store. The 'propValue' would be used to
    // generate the key for the value to remove.
    void removeValue(
            const ::aidl::android::hardware::automotive::vehicle::VehiclePropValue& propValue);

    // Remove all the values for the property.
    void removeValuesForProperty(int32_t propId);

    // Read all the stored values.
    std::vector<::aidl::android::hardware::automotive::vehicle::VehiclePropValue> readAllValues()
            const;

    // Read all the values for the property.
    ::android::base::Result<
            std::vector<::aidl::android::hardware::automotive::vehicle::VehiclePropValue>>
    readValuesForProperty(int32_t propId) const;

    // Read the value for the requested property.
    ::android::base::Result<
            std::unique_ptr<::aidl::android::hardware::automotive::vehicle::VehiclePropValue>>
    readValue(
            const ::aidl::android::hardware::automotive::vehicle::VehiclePropValue& request) const;

    // Read the value for the requested property.
    ::android::base::Result<
            std::unique_ptr<::aidl::android::hardware::automotive::vehicle::VehiclePropValue>>
    readValue(int32_t prop, int32_t area = 0, int64_t token = 0) const;

    // Get all property configs.
    std::vector<::aidl::android::hardware::automotive::vehicle::VehiclePropConfig> getAllConfigs()
            const;

    // Get the property config for the requested property.
    ::android::base::Result<
            const ::aidl::android::hardware::automotive::vehicle::VehiclePropConfig*>
    getConfig(int32_t propId) const;

  private:
    struct RecordId {
        int32_t area;
        int64_t token;

        bool operator==(const RecordId& other) const;
        bool operator<(const RecordId& other) const;

        std::string toString() const;
    };

    struct Record {
        ::aidl::android::hardware::automotive::vehicle::VehiclePropConfig propConfig;
        TokenFunction tokenFunction;
        std::map<RecordId, ::aidl::android::hardware::automotive::vehicle::VehiclePropValue> values;
    };

    mutable std::mutex mLock;
    std::unordered_map<int32_t, Record> mRecordsByPropId GUARDED_BY(mLock);

    const Record* getRecordLocked(int32_t propId) const;

    Record* getRecordLocked(int32_t propId);

    RecordId getRecordIdLocked(
            const ::aidl::android::hardware::automotive::vehicle::VehiclePropValue& propValue,
            const Record& record) const;

    ::android::base::Result<
            std::unique_ptr<::aidl::android::hardware::automotive::vehicle::VehiclePropValue>>
    readValueLocked(const RecordId& recId, const Record& record) const;
};

}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android

#endif  // android_hardware_automotive_vehicle_aidl_impl_utils_common_include_VehiclePropertyStore_H_
