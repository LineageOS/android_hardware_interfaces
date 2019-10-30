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
#pragma once

#include <memory>
#include <mutex>
#include <vector>

#include <android-base/unique_fd.h>
#include <android/hardware/health/2.1/IHealth.h>
#include <healthd/BatteryMonitor.h>
#include <hidl/Status.h>

#include <health2impl/Callback.h>

using ::android::sp;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hidl::base::V1_0::IBase;

namespace android {
namespace hardware {
namespace health {
namespace V2_1 {
namespace implementation {

class Health : public IHealth {
  public:
    Health(std::unique_ptr<healthd_config>&& config);

    // Methods from ::android::hardware::health::V2_0::IHealth follow.
    Return<::android::hardware::health::V2_0::Result> registerCallback(
            const sp<::android::hardware::health::V2_0::IHealthInfoCallback>& callback) override;
    Return<::android::hardware::health::V2_0::Result> unregisterCallback(
            const sp<::android::hardware::health::V2_0::IHealthInfoCallback>& callback) override;
    Return<::android::hardware::health::V2_0::Result> update() override;
    Return<void> getChargeCounter(getChargeCounter_cb _hidl_cb) override;
    Return<void> getCurrentNow(getCurrentNow_cb _hidl_cb) override;
    Return<void> getCurrentAverage(getCurrentAverage_cb _hidl_cb) override;
    Return<void> getCapacity(getCapacity_cb _hidl_cb) override;
    Return<void> getEnergyCounter(getEnergyCounter_cb _hidl_cb) override;
    Return<void> getChargeStatus(getChargeStatus_cb _hidl_cb) override;
    Return<void> getStorageInfo(getStorageInfo_cb _hidl_cb) override;
    Return<void> getDiskStats(getDiskStats_cb _hidl_cb) override;
    Return<void> getHealthInfo(getHealthInfo_cb _hidl_cb) override;

    // Methods from ::android::hardware::health::V2_1::IHealth follow.
    Return<void> getHealthConfig(getHealthConfig_cb _hidl_cb) override;
    Return<void> getHealthInfo_2_1(getHealthInfo_2_1_cb _hidl_cb) override;
    Return<void> shouldKeepScreenOn(shouldKeepScreenOn_cb _hidl_cb) override;

    // Methods from ::android::hidl::base::V1_0::IBase follow.
    Return<void> debug(const hidl_handle& fd, const hidl_vec<hidl_string>& args) override;

  protected:
    // A subclass can override this to modify any health info object before
    // returning to clients. This is similar to healthd_board_battery_update().
    // By default, it does nothing.
    virtual void UpdateHealthInfo(HealthInfo* health_info);

  private:
    bool unregisterCallbackInternal(const sp<IBase>& callback);

    BatteryMonitor battery_monitor_;
    std::unique_ptr<healthd_config> healthd_config_;

    std::mutex callbacks_lock_;
    std::vector<std::unique_ptr<Callback>> callbacks_;
};

}  // namespace implementation
}  // namespace V2_1
}  // namespace health
}  // namespace hardware
}  // namespace android
