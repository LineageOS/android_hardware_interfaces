#ifndef ANDROID_HARDWARE_HEALTH_V2_0_HEALTH_H
#define ANDROID_HARDWARE_HEALTH_V2_0_HEALTH_H

#include <memory>
#include <vector>

#include <android/hardware/health/1.0/types.h>
#include <android/hardware/health/2.0/IHealth.h>
#include <healthd/BatteryMonitor.h>
#include <hidl/Status.h>

using android::hardware::health::V2_0::StorageInfo;
using android::hardware::health::V2_0::DiskStats;

void get_storage_info(std::vector<struct StorageInfo>& info);
void get_disk_stats(std::vector<struct DiskStats>& stats);

namespace android {
namespace hardware {
namespace health {
namespace V2_0 {
namespace implementation {

using V1_0::BatteryStatus;

using ::android::hidl::base::V1_0::IBase;

struct Health : public IHealth, hidl_death_recipient {
   public:
    static sp<IHealth> initInstance(struct healthd_config* c);
    // Should only be called by implementation itself (-impl, -service).
    // Clients should not call this function. Instead, initInstance() initializes and returns the
    // global instance that has fewer functions.
    static sp<Health> getImplementation();

    Health(struct healthd_config* c);

    void notifyListeners(HealthInfo* info);

    // Methods from IHealth follow.
    Return<Result> registerCallback(const sp<IHealthInfoCallback>& callback) override;
    Return<Result> unregisterCallback(const sp<IHealthInfoCallback>& callback) override;
    Return<Result> update() override;
    Return<void> getChargeCounter(getChargeCounter_cb _hidl_cb) override;
    Return<void> getCurrentNow(getCurrentNow_cb _hidl_cb) override;
    Return<void> getCurrentAverage(getCurrentAverage_cb _hidl_cb) override;
    Return<void> getCapacity(getCapacity_cb _hidl_cb) override;
    Return<void> getEnergyCounter(getEnergyCounter_cb _hidl_cb) override;
    Return<void> getChargeStatus(getChargeStatus_cb _hidl_cb) override;
    Return<void> getStorageInfo(getStorageInfo_cb _hidl_cb) override;
    Return<void> getDiskStats(getDiskStats_cb _hidl_cb) override;
    Return<void> getHealthInfo(getHealthInfo_cb _hidl_cb) override;

    // Methods from ::android::hidl::base::V1_0::IBase follow.
    Return<void> debug(const hidl_handle& fd, const hidl_vec<hidl_string>& args) override;

    void serviceDied(uint64_t cookie, const wp<IBase>& /* who */) override;

   private:
    static sp<Health> instance_;

    std::recursive_mutex callbacks_lock_;
    std::vector<sp<IHealthInfoCallback>> callbacks_;
    std::unique_ptr<BatteryMonitor> battery_monitor_;

    bool unregisterCallbackInternal(const sp<IBase>& cb);

    // update() and only notify the given callback, but none of the other callbacks.
    // If cb is null, do not notify any callback at all.
    Return<Result> updateAndNotify(const sp<IHealthInfoCallback>& cb);
};

}  // namespace implementation
}  // namespace V2_0
}  // namespace health
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_HEALTH_V2_0_HEALTH_H
