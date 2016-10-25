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

#ifndef android_hardware_vehicle_V2_0_VehicleDebugUtils_H_
#define android_hardware_vehicle_V2_0_VehicleDebugUtils_H_

#include <android/hardware/vehicle/2.0/types.h>
#include <vehicle_hal_manager/VehicleUtils.h>
#include <ios>
#include <sstream>

namespace android {
namespace hardware {
namespace vehicle {
namespace V2_0 {

const VehiclePropConfig kVehicleProperties[] = {
    {
        .prop = VehicleProperty::INFO_MAKE,
        .access = VehiclePropertyAccess::READ,
        .changeMode = VehiclePropertyChangeMode::STATIC,
        .permissionModel = VehiclePermissionModel::OEM_ONLY,
        .configString = "Some=config,options=if,you=have_any",
    },

    {
        .prop = VehicleProperty::HVAC_FAN_SPEED,
        .access = VehiclePropertyAccess::READ_WRITE,
        .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
        .permissionModel = VehiclePermissionModel::NO_RESTRICTION,
        .supportedAreas = static_cast<int32_t>(
            VehicleAreaZone::ROW_1_LEFT | VehicleAreaZone::ROW_1_RIGHT),
        .areaConfigs = init_hidl_vec({
                                         VehicleAreaConfig {
                                             .areaId = val(
                                                 VehicleAreaZone::ROW_2_LEFT),
                                             .minInt32Value = 1,
                                             .maxInt32Value = 7},
                                         VehicleAreaConfig {
                                             .areaId = val(
                                                 VehicleAreaZone::ROW_1_RIGHT),
                                             .minInt32Value = 1,
                                             .maxInt32Value = 5,
                                         }
                                     }),
    },

    {
        .prop = VehicleProperty::INFO_FUEL_CAPACITY,
        .access = VehiclePropertyAccess::READ,
        .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
        .permissionModel = VehiclePermissionModel::OEM_ONLY,
        .areaConfigs = init_hidl_vec({
                                         VehicleAreaConfig {
                                             .minFloatValue = 0,
                                             .maxFloatValue = 1.0
                                         }
                                     })
    },

    {
        .prop = VehicleProperty::DISPLAY_BRIGHTNESS,
        .access = VehiclePropertyAccess::READ_WRITE,
        .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
        .permissionModel = VehiclePermissionModel::OEM_ONLY,
        .areaConfigs = init_hidl_vec({
                                         VehicleAreaConfig {
                                             .minInt32Value = 0,
                                             .maxInt32Value = 10
                                         }
                                     })
    }
};

constexpr auto kTimeout = std::chrono::milliseconds(500);

class MockedVehicleCallback : public IVehicleCallback {
public:
    // Methods from ::android::hardware::vehicle::V2_0::IVehicleCallback follow.
    Return<void> onPropertyEvent(
            const hidl_vec<VehiclePropValue>& values) override {
        {
            MuxGuard  g(mLock);
            mReceivedEvents.push_back(values);
        }
        mEventCond.notify_one();
        return Return<void>();
    }
    Return<void> onPropertySet(const VehiclePropValue& value) override {
        return Return<void>();
    }
    Return<void> onError(StatusCode errorCode,
                         VehicleProperty propId,
                         VehiclePropertyOperation operation) override {
        return Return<void>();
    }

    bool waitForExpectedEvents(size_t expectedEvents) {
        std::unique_lock<std::mutex> g(mLock);

        if (expectedEvents == 0 && mReceivedEvents.size() == 0) {
            // No events expected, let's sleep a little bit to make sure
            // nothing will show up.
            return mEventCond.wait_for(g, kTimeout) == std::cv_status::timeout;
        }

        while (expectedEvents != mReceivedEvents.size()) {
            if (mEventCond.wait_for(g, kTimeout) == std::cv_status::timeout) {
                return false;
            }
        }
        return true;
    }

    void reset() {
        mReceivedEvents.clear();
    }

    const std::vector<hidl_vec<VehiclePropValue>>& getReceivedEvents() {
        return mReceivedEvents;
    }

private:
    using MuxGuard = std::lock_guard<std::mutex>;

    std::mutex mLock;
    std::condition_variable mEventCond;
    std::vector<hidl_vec<VehiclePropValue>> mReceivedEvents;
};

template<typename T>
inline std::string hexString(T value) {
    std::stringstream ss;
    ss << std::showbase << std::hex << value;
    return ss.str();
}

template <typename T, typename Collection>
inline void assertAllExistsAnyOrder(
        std::initializer_list<T> expected,
        const Collection& actual,
        const char* msg) {
    std::set<T> expectedSet = expected;

    for (auto a: actual) {
        ASSERT_EQ(1u, expectedSet.erase(a))
                << msg << "\nContains not unexpected value.\n";
    }

    ASSERT_EQ(0u, expectedSet.size())
            << msg
            << "\nDoesn't contain expected value.";
}

#define ASSERT_ALL_EXISTS(...) \
    assertAllExistsAnyOrder(__VA_ARGS__, (std::string("Called from: ") + \
            std::string(__FILE__) + std::string(":") + \
            std::to_string(__LINE__)).c_str()); \

template<typename T>
inline std::string enumToHexString(T value) {
    return hexString(val(value));
}

template <typename T>
inline std::string toString(const hidl_vec<T>& vec) {
    std::stringstream ss("[");
    for (size_t i = 0; i < vec.size(); i++) {
        if (i != 0) ss << ",";
        ss << vec[i];
    }
    ss << "]";
    return ss.str();
}

inline std::string toString(const VehiclePropValue &v) {
    std::stringstream ss;
    ss << "VehiclePropValue {n"
       << "  prop: " << enumToHexString(v.prop) << ",\n"
       << "  areaId: " << hexString(v.areaId) << ",\n"
       << "  timestamp: " << v.timestamp << ",\n"
       << "  value {\n"
       << "    int32Values: " << toString(v.value.int32Values) << ",\n"
       << "    floatValues: " << toString(v.value.floatValues) << ",\n"
       << "    int64Values: " << toString(v.value.int64Values) << ",\n"
       << "    bytes: " << toString(v.value.bytes) << ",\n"
       << "    string: " << v.value.stringValue.c_str() << ",\n"
       << "  }\n"
       << "}\n";

    return ss.str();
}

inline std::string toString(const VehiclePropConfig &config) {
    std::stringstream ss;
    ss << "VehiclePropConfig {\n"
       << "  prop: " << enumToHexString(config.prop) << ",\n"
       << "  supportedAreas: " << hexString(config.supportedAreas) << ",\n"
       << "  access: " << enumToHexString(config.access) << ",\n"
       << "  permissionModel: " << enumToHexString(config.permissionModel) << ",\n"
       << "  changeMode: " << enumToHexString(config.changeMode) << ",\n"
       << "  configFlags: " << hexString(config.configFlags) << ",\n"
       << "  minSampleRate: " << config.minSampleRate << ",\n"
       << "  maxSampleRate: " << config.maxSampleRate << ",\n"
       << "  configString: " << config.configString.c_str() << ",\n";

    ss << "  areaConfigs {\n";
    for (size_t i = 0; i < config.areaConfigs.size(); i++) {
        const auto &area = config.areaConfigs[i];
        ss << "    areaId: " << hexString(area.areaId) << ",\n"
           << "    minFloatValue: " << area.minFloatValue << ",\n"
           << "    minFloatValue: " << area.maxFloatValue << ",\n"
           << "    minInt32Value: " << area.minInt32Value << ",\n"
           << "    minInt32Value: " << area.maxInt32Value << ",\n"
           << "    minInt64Value: " << area.minInt64Value << ",\n"
           << "    minInt64Value: " << area.maxInt64Value << ",\n";
    }
    ss << "  }\n"
       << "}\n";

    return ss.str();
}


}  // namespace V2_0
}  // namespace vehicle
}  // namespace hardware
}  // namespace android


#endif //VEHICLEHALDEBUGUTILS_H
