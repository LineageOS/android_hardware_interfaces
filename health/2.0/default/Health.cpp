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
#define LOG_TAG "android.hardware.health@2.0-impl"
#include <android-base/logging.h>

#include <android-base/file.h>
#include <android/hardware/health/2.0/types.h>
#include <health2/Health.h>

#include <hal_conversion.h>
#include <hidl/HidlTransportSupport.h>

using HealthInfo_1_0 = android::hardware::health::V1_0::HealthInfo;
using android::hardware::health::V1_0::hal_conversion::convertFromHealthInfo;

extern void healthd_battery_update_internal(bool);

namespace android {
namespace hardware {
namespace health {
namespace V2_0 {
namespace implementation {

sp<Health> Health::instance_;

Health::Health(struct healthd_config* c) {
    // TODO(b/69268160): remove when libhealthd is removed.
    healthd_board_init(c);
    battery_monitor_ = std::make_unique<BatteryMonitor>();
    battery_monitor_->init(c);
}

// Methods from IHealth follow.
Return<Result> Health::registerCallback(const sp<IHealthInfoCallback>& callback) {
    if (callback == nullptr) {
        return Result::SUCCESS;
    }

    {
        std::lock_guard<decltype(callbacks_lock_)> lock(callbacks_lock_);
        callbacks_.push_back(callback);
        // unlock
    }

    auto linkRet = callback->linkToDeath(this, 0u /* cookie */);
    if (!linkRet.withDefault(false)) {
        LOG(WARNING) << __func__ << "Cannot link to death: "
                     << (linkRet.isOk() ? "linkToDeath returns false" : linkRet.description());
        // ignore the error
    }

    return updateAndNotify(callback);
}

bool Health::unregisterCallbackInternal(const sp<IBase>& callback) {
    if (callback == nullptr) return false;

    bool removed = false;
    std::lock_guard<decltype(callbacks_lock_)> lock(callbacks_lock_);
    for (auto it = callbacks_.begin(); it != callbacks_.end();) {
        if (interfacesEqual(*it, callback)) {
            it = callbacks_.erase(it);
            removed = true;
        } else {
            ++it;
        }
    }
    (void)callback->unlinkToDeath(this).isOk();  // ignore errors
    return removed;
}

Return<Result> Health::unregisterCallback(const sp<IHealthInfoCallback>& callback) {
    return unregisterCallbackInternal(callback) ? Result::SUCCESS : Result::NOT_FOUND;
}

template <typename T>
void getProperty(const std::unique_ptr<BatteryMonitor>& monitor, int id, T defaultValue,
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
}

Return<void> Health::getChargeCounter(getChargeCounter_cb _hidl_cb) {
    getProperty<int32_t>(battery_monitor_, BATTERY_PROP_CHARGE_COUNTER, 0, _hidl_cb);
    return Void();
}

Return<void> Health::getCurrentNow(getCurrentNow_cb _hidl_cb) {
    getProperty<int32_t>(battery_monitor_, BATTERY_PROP_CURRENT_NOW, 0, _hidl_cb);
    return Void();
}

Return<void> Health::getCurrentAverage(getCurrentAverage_cb _hidl_cb) {
    getProperty<int32_t>(battery_monitor_, BATTERY_PROP_CURRENT_AVG, 0, _hidl_cb);
    return Void();
}

Return<void> Health::getCapacity(getCapacity_cb _hidl_cb) {
    getProperty<int32_t>(battery_monitor_, BATTERY_PROP_CAPACITY, 0, _hidl_cb);
    return Void();
}

Return<void> Health::getEnergyCounter(getEnergyCounter_cb _hidl_cb) {
    getProperty<int64_t>(battery_monitor_, BATTERY_PROP_ENERGY_COUNTER, 0, _hidl_cb);
    return Void();
}

Return<void> Health::getChargeStatus(getChargeStatus_cb _hidl_cb) {
    getProperty(battery_monitor_, BATTERY_PROP_BATTERY_STATUS, BatteryStatus::UNKNOWN, _hidl_cb);
    return Void();
}

Return<Result> Health::update() {
    if (!healthd_mode_ops || !healthd_mode_ops->battery_update) {
        LOG(WARNING) << "health@2.0: update: not initialized. "
                     << "update() should not be called in charger";
        return Result::UNKNOWN;
    }

    // Retrieve all information and call healthd_mode_ops->battery_update, which calls
    // notifyListeners.
    battery_monitor_->updateValues();
    const HealthInfo_1_0& health_info = battery_monitor_->getHealthInfo_1_0();
    struct BatteryProperties props;
    convertFromHealthInfo(health_info, &props);
    bool log = (healthd_board_battery_update(&props) == 0);
    if (log) {
        battery_monitor_->logValues();
    }
    healthd_mode_ops->battery_update(&props);
    bool chargerOnline = battery_monitor_->isChargerOnline();

    // adjust uevent / wakealarm periods
    healthd_battery_update_internal(chargerOnline);

    return Result::SUCCESS;
}

Return<Result> Health::updateAndNotify(const sp<IHealthInfoCallback>& callback) {
    std::lock_guard<decltype(callbacks_lock_)> lock(callbacks_lock_);
    std::vector<sp<IHealthInfoCallback>> storedCallbacks{std::move(callbacks_)};
    callbacks_.clear();
    if (callback != nullptr) {
        callbacks_.push_back(callback);
    }
    Return<Result> result = update();
    callbacks_ = std::move(storedCallbacks);
    return result;
}

void Health::notifyListeners(HealthInfo* healthInfo) {
    std::vector<StorageInfo> info;
    get_storage_info(info);

    std::vector<DiskStats> stats;
    get_disk_stats(stats);

    int32_t currentAvg = 0;

    struct BatteryProperty prop;
    status_t ret = battery_monitor_->getProperty(BATTERY_PROP_CURRENT_AVG, &prop);
    if (ret == OK) {
        currentAvg = static_cast<int32_t>(prop.valueInt64);
    }

    healthInfo->batteryCurrentAverage = currentAvg;
    healthInfo->diskStats = stats;
    healthInfo->storageInfos = info;

    std::lock_guard<decltype(callbacks_lock_)> lock(callbacks_lock_);
    for (auto it = callbacks_.begin(); it != callbacks_.end();) {
        auto ret = (*it)->healthInfoChanged(*healthInfo);
        if (!ret.isOk() && ret.isDeadObject()) {
            it = callbacks_.erase(it);
        } else {
            ++it;
        }
    }
}

Return<void> Health::debug(const hidl_handle& handle, const hidl_vec<hidl_string>&) {
    if (handle != nullptr && handle->numFds >= 1) {
        int fd = handle->data[0];
        battery_monitor_->dumpState(fd);

        getHealthInfo([fd](auto res, const auto& info) {
            android::base::WriteStringToFd("\ngetHealthInfo -> ", fd);
            if (res == Result::SUCCESS) {
                android::base::WriteStringToFd(toString(info), fd);
            } else {
                android::base::WriteStringToFd(toString(res), fd);
            }
            android::base::WriteStringToFd("\n", fd);
        });

        fsync(fd);
    }
    return Void();
}

Return<void> Health::getStorageInfo(getStorageInfo_cb _hidl_cb) {
    std::vector<struct StorageInfo> info;
    get_storage_info(info);
    hidl_vec<struct StorageInfo> info_vec(info);
    if (!info.size()) {
        _hidl_cb(Result::NOT_SUPPORTED, info_vec);
    } else {
        _hidl_cb(Result::SUCCESS, info_vec);
    }
    return Void();
}

Return<void> Health::getDiskStats(getDiskStats_cb _hidl_cb) {
    std::vector<struct DiskStats> stats;
    get_disk_stats(stats);
    hidl_vec<struct DiskStats> stats_vec(stats);
    if (!stats.size()) {
        _hidl_cb(Result::NOT_SUPPORTED, stats_vec);
    } else {
        _hidl_cb(Result::SUCCESS, stats_vec);
    }
    return Void();
}

Return<void> Health::getHealthInfo(getHealthInfo_cb _hidl_cb) {
    using android::hardware::health::V1_0::hal_conversion::convertToHealthInfo;

    updateAndNotify(nullptr);
    HealthInfo healthInfo = battery_monitor_->getHealthInfo_2_0();

    std::vector<StorageInfo> info;
    get_storage_info(info);

    std::vector<DiskStats> stats;
    get_disk_stats(stats);

    int32_t currentAvg = 0;

    struct BatteryProperty prop;
    status_t ret = battery_monitor_->getProperty(BATTERY_PROP_CURRENT_AVG, &prop);
    if (ret == OK) {
        currentAvg = static_cast<int32_t>(prop.valueInt64);
    }

    healthInfo.batteryCurrentAverage = currentAvg;
    healthInfo.diskStats = stats;
    healthInfo.storageInfos = info;

    _hidl_cb(Result::SUCCESS, healthInfo);
    return Void();
}

void Health::serviceDied(uint64_t /* cookie */, const wp<IBase>& who) {
    (void)unregisterCallbackInternal(who.promote());
}

sp<IHealth> Health::initInstance(struct healthd_config* c) {
    if (instance_ == nullptr) {
        instance_ = new Health(c);
    }
    return instance_;
}

sp<Health> Health::getImplementation() {
    CHECK(instance_ != nullptr);
    return instance_;
}

}  // namespace implementation
}  // namespace V2_0
}  // namespace health
}  // namespace hardware
}  // namespace android
