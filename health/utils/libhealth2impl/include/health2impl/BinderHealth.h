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

#include <android-base/unique_fd.h>
#include <android/hardware/health/2.1/IHealth.h>
#include <healthd/healthd.h>

#include <health2impl/Callback.h>
#include <health2impl/HalHealthLoop.h>

namespace android {
namespace hardware {
namespace health {
namespace V2_1 {
namespace implementation {

// binderized health HAL implementation.
class BinderHealth : public HalHealthLoop, public IHealth, public hidl_death_recipient {
  public:
    // |impl| should be the passthrough implementation.
    BinderHealth(const std::string& name, const sp<IHealth>& impl);

    // Methods from ::android::hardware::health::V2_0::IHealth follow.
    Return<::android::hardware::health::V2_0::Result> registerCallback(
            const sp<::android::hardware::health::V2_0::IHealthInfoCallback>& callback) override;
    Return<::android::hardware::health::V2_0::Result> unregisterCallback(
            const sp<::android::hardware::health::V2_0::IHealthInfoCallback>& callback) override;
    Return<::android::hardware::health::V2_0::Result> update() override;
    Return<void> getChargeCounter(getChargeCounter_cb _hidl_cb) override {
        return service()->getChargeCounter(_hidl_cb);
    }
    Return<void> getCurrentNow(getCurrentNow_cb _hidl_cb) override {
        return service()->getCurrentNow(_hidl_cb);
    }
    Return<void> getCurrentAverage(getCurrentAverage_cb _hidl_cb) override {
        return service()->getCurrentAverage(_hidl_cb);
    }
    Return<void> getCapacity(getCapacity_cb _hidl_cb) override {
        return service()->getCapacity(_hidl_cb);
    }
    Return<void> getEnergyCounter(getEnergyCounter_cb _hidl_cb) override {
        return service()->getEnergyCounter(_hidl_cb);
    }
    Return<void> getChargeStatus(getChargeStatus_cb _hidl_cb) override {
        return service()->getChargeStatus(_hidl_cb);
    }
    Return<void> getStorageInfo(getStorageInfo_cb _hidl_cb) override {
        return service()->getStorageInfo(_hidl_cb);
    }
    Return<void> getDiskStats(getDiskStats_cb _hidl_cb) override {
        return service()->getDiskStats(_hidl_cb);
    }
    Return<void> getHealthInfo(getHealthInfo_cb _hidl_cb) override {
        return service()->getHealthInfo(_hidl_cb);
    }

    // Methods from ::android::hardware::health::V2_1::IHealth follow.
    Return<void> getHealthConfig(getHealthConfig_cb _hidl_cb) override {
        return service()->getHealthConfig(_hidl_cb);
    }
    Return<void> getHealthInfo_2_1(getHealthInfo_2_1_cb _hidl_cb) override {
        return service()->getHealthInfo_2_1(_hidl_cb);
    }
    Return<void> shouldKeepScreenOn(shouldKeepScreenOn_cb _hidl_cb) override {
        return service()->shouldKeepScreenOn(_hidl_cb);
    }

    // Methods from ::android::hidl::base::V1_0::IBase follow.
    Return<void> debug(const hidl_handle& fd, const hidl_vec<hidl_string>& args) override {
        return service()->debug(fd, args);
    }

    // hidl_death_recipient implementation.
    void serviceDied(uint64_t cookie, const wp<IBase>& who) override;

    // Called by BinderHealthCallback.
    void OnHealthInfoChanged(const HealthInfo& health_info) override;

  protected:
    void Init(struct healthd_config* config) override;
    int PrepareToWait() override;
    // A subclass may override this if it wants to handle binder events differently.
    virtual void BinderEvent(uint32_t epevents);

  private:
    bool unregisterCallbackInternal(const sp<IBase>& callback);
    int binder_fd_ = -1;
    std::mutex callbacks_lock_;
    std::vector<std::unique_ptr<Callback>> callbacks_;
};

}  // namespace implementation
}  // namespace V2_1
}  // namespace health
}  // namespace hardware
}  // namespace android
