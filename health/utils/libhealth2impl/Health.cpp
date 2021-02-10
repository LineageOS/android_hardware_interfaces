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

#include <health2impl/Health.h>

#include <functional>
#include <string_view>

#include <android-base/file.h>
#include <android-base/logging.h>
#include <android/hardware/health/1.0/types.h>
#include <android/hardware/health/2.0/IHealthInfoCallback.h>
#include <android/hardware/health/2.0/types.h>
#include <android/hardware/health/2.1/IHealthInfoCallback.h>
#include <hal_conversion.h>
#include <healthd/healthd.h>
#include <hidl/HidlTransportSupport.h>
#include <hwbinder/IPCThreadState.h>

#include <health2impl/Callback.h>
#include <health2impl/HalHealthLoop.h>

using android::hardware::health::V1_0::BatteryStatus;
using android::hardware::health::V1_0::toString;
using android::hardware::health::V1_0::hal_conversion::convertFromHealthInfo;
using android::hardware::health::V1_0::hal_conversion::convertToHealthConfig;
using android::hardware::health::V1_0::hal_conversion::convertToHealthInfo;
using android::hardware::health::V2_0::Result;
using android::hardware::health::V2_1::IHealth;

using ScreenOn = decltype(healthd_config::screen_on);

namespace android {
namespace hardware {
namespace health {
namespace V2_1 {
namespace implementation {

/*
// If you need to call healthd_board_init, construct the Health instance with
// the healthd_config after calling healthd_board_init:
struct healthd_config* init_config(struct healthd_config* config) {
    healthd_board_init(config);
    return config;
}
class MyHealth : public Health {
    MyHealth(struct healthd_config* config) :
        Health(init_config(config)) {}
};
*/

Health::Health(std::unique_ptr<healthd_config>&& config) : healthd_config_(std::move(config)) {
    battery_monitor_.init(healthd_config_.get());
}

//
// Callbacks are not supported by the passthrough implementation.
//

Return<Result> Health::registerCallback(const sp<V2_0::IHealthInfoCallback>&) {
    return Result::NOT_SUPPORTED;
}

Return<Result> Health::unregisterCallback(const sp<V2_0::IHealthInfoCallback>&) {
    return Result::NOT_SUPPORTED;
}

Return<Result> Health::update() {
    Result result = Result::UNKNOWN;
    getHealthInfo_2_1([&](auto res, const auto& health_info) {
        result = res;
        if (res != Result::SUCCESS) {
            LOG(ERROR) << "Cannot call getHealthInfo_2_1: " << toString(res);
            return;
        }

        BatteryMonitor::logValues(health_info, *healthd_config_);
    });
    return result;
}

//
// Getters.
//

template <typename T>
static Return<void> GetProperty(BatteryMonitor* monitor, int id, T defaultValue,
                                const std::function<void(Result, T)>& callback) {
    struct BatteryProperty prop;
    T ret = defaultValue;
    Result result = Result::SUCCESS;
    status_t err = monitor->getProperty(static_cast<int>(id), &prop);
    if (err != OK) {
        LOG(DEBUG) << "getProperty(" << id << ")"
                   << " fails: (" << err << ") " << strerror(-err);
    } else {
        ret = static_cast<T>(prop.valueInt64);
    }
    switch (err) {
        case OK:
            result = Result::SUCCESS;
            break;
        case NAME_NOT_FOUND:
            result = Result::NOT_SUPPORTED;
            break;
        default:
            result = Result::UNKNOWN;
            break;
    }
    callback(result, static_cast<T>(ret));
    return Void();
}

Return<void> Health::getChargeCounter(getChargeCounter_cb _hidl_cb) {
    return GetProperty<int32_t>(&battery_monitor_, BATTERY_PROP_CHARGE_COUNTER, 0, _hidl_cb);
}

Return<void> Health::getCurrentNow(getCurrentNow_cb _hidl_cb) {
    return GetProperty<int32_t>(&battery_monitor_, BATTERY_PROP_CURRENT_NOW, 0, _hidl_cb);
}

Return<void> Health::getCurrentAverage(getCurrentAverage_cb _hidl_cb) {
    return GetProperty<int32_t>(&battery_monitor_, BATTERY_PROP_CURRENT_AVG, 0, _hidl_cb);
}

Return<void> Health::getCapacity(getCapacity_cb _hidl_cb) {
    return GetProperty<int32_t>(&battery_monitor_, BATTERY_PROP_CAPACITY, 0, _hidl_cb);
}

Return<void> Health::getEnergyCounter(getEnergyCounter_cb _hidl_cb) {
    return GetProperty<int64_t>(&battery_monitor_, BATTERY_PROP_ENERGY_COUNTER, 0, _hidl_cb);
}

Return<void> Health::getChargeStatus(getChargeStatus_cb _hidl_cb) {
    return GetProperty(&battery_monitor_, BATTERY_PROP_BATTERY_STATUS, BatteryStatus::UNKNOWN,
                       _hidl_cb);
}

Return<void> Health::getStorageInfo(getStorageInfo_cb _hidl_cb) {
    // This implementation does not support StorageInfo. An implementation may extend this
    // class and override this function to support storage info.
    _hidl_cb(Result::NOT_SUPPORTED, {});
    return Void();
}

Return<void> Health::getDiskStats(getDiskStats_cb _hidl_cb) {
    // This implementation does not support DiskStats. An implementation may extend this
    // class and override this function to support disk stats.
    _hidl_cb(Result::NOT_SUPPORTED, {});
    return Void();
}

template <typename T, typename Method>
static inline void GetHealthInfoField(Health* service, Method func, T* out) {
    *out = T{};
    std::invoke(func, service, [out](Result result, const T& value) {
        if (result == Result::SUCCESS) *out = value;
    });
}

Return<void> Health::getHealthInfo(getHealthInfo_cb _hidl_cb) {
    return getHealthInfo_2_1(
            [&](auto res, const auto& health_info) { _hidl_cb(res, health_info.legacy); });
}

Return<void> Health::getHealthInfo_2_1(getHealthInfo_2_1_cb _hidl_cb) {
    battery_monitor_.updateValues();

    HealthInfo health_info = battery_monitor_.getHealthInfo_2_1();

    // Fill in storage infos; these aren't retrieved by BatteryMonitor.
    GetHealthInfoField(this, &Health::getStorageInfo, &health_info.legacy.storageInfos);
    GetHealthInfoField(this, &Health::getDiskStats, &health_info.legacy.diskStats);

    // A subclass may want to update health info struct before returning it.
    UpdateHealthInfo(&health_info);

    _hidl_cb(Result::SUCCESS, health_info);
    return Void();
}

Return<void> Health::debug(const hidl_handle& handle, const hidl_vec<hidl_string>&) {
    if (handle == nullptr || handle->numFds == 0) {
        return Void();
    }

    int fd = handle->data[0];
    battery_monitor_.dumpState(fd);
    getHealthInfo_2_1([fd](auto res, const auto& info) {
        android::base::WriteStringToFd("\ngetHealthInfo -> ", fd);
        if (res == Result::SUCCESS) {
            android::base::WriteStringToFd(toString(info), fd);
        } else {
            android::base::WriteStringToFd(toString(res), fd);
        }
        android::base::WriteStringToFd("\n", fd);
    });

    fsync(fd);
    return Void();
}

Return<void> Health::getHealthConfig(getHealthConfig_cb _hidl_cb) {
    HealthConfig config = {};
    convertToHealthConfig(healthd_config_.get(), config.battery);
    config.bootMinCap = static_cast<int32_t>(healthd_config_->boot_min_cap);

    _hidl_cb(Result::SUCCESS, config);
    return Void();
}

Return<void> Health::shouldKeepScreenOn(shouldKeepScreenOn_cb _hidl_cb) {
    if (!healthd_config_->screen_on) {
        _hidl_cb(Result::NOT_SUPPORTED, true);
        return Void();
    }

    Result returned_result = Result::UNKNOWN;
    bool screen_on = true;
    getHealthInfo_2_1([&](auto res, const auto& health_info) {
        returned_result = res;
        if (returned_result != Result::SUCCESS) return;

        struct BatteryProperties props = {};
        V1_0::hal_conversion::convertFromHealthInfo(health_info.legacy.legacy, &props);
        screen_on = healthd_config_->screen_on(&props);
    });
    _hidl_cb(returned_result, screen_on);
    return Void();
}

//
// Subclass helpers / overrides
//

void Health::UpdateHealthInfo(HealthInfo* /* health_info */) {
    /*
        // Sample code for a subclass to implement this:
        // If you need to modify values (e.g. batteryChargeTimeToFullNowSeconds), do it here.
        health_info->batteryChargeTimeToFullNowSeconds = calculate_charge_time_seconds();

        // If you need to call healthd_board_battery_update:
        struct BatteryProperties props;
        convertFromHealthInfo(health_info.legacy.legacy, &props);
        healthd_board_battery_update(&props);
        convertToHealthInfo(&props, health_info.legacy.legacy);
    */
}

}  // namespace implementation
}  // namespace V2_1
}  // namespace health
}  // namespace hardware
}  // namespace android
