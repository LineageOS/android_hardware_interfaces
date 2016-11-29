#ifndef ANDROID_HARDWARE_THERMAL_V1_0_THERMAL_H
#define ANDROID_HARDWARE_THERMAL_V1_0_THERMAL_H

#include <android/hardware/thermal/1.0/IThermal.h>
#include <hidl/Status.h>
#include <hardware/thermal.h>

#include <hidl/MQDescriptor.h>

namespace android {
namespace hardware {
namespace thermal {
namespace V1_0 {
namespace implementation {

using ::android::hardware::thermal::V1_0::CoolingDevice;
using ::android::hardware::thermal::V1_0::CpuUsage;
using ::android::hardware::thermal::V1_0::IThermal;
using ::android::hardware::thermal::V1_0::Temperature;
using ::android::hardware::thermal::V1_0::ThermalStatus;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::hidl_vec;
using ::android::hardware::hidl_string;
using ::android::sp;

struct Thermal : public IThermal {
    Thermal(thermal_module_t* module);
    // Methods from ::android::hardware::thermal::V1_0::IThermal follow.
    Return<void> getTemperatures(getTemperatures_cb _hidl_cb)  override;
    Return<void> getCpuUsages(getCpuUsages_cb _hidl_cb)  override;
    Return<void> getCoolingDevices(getCoolingDevices_cb _hidl_cb)  override;
    private:
        thermal_module_t* mModule;
};

extern "C" IThermal* HIDL_FETCH_IThermal(const char* name);

}  // namespace implementation
}  // namespace V1_0
}  // namespace thermal
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_THERMAL_V1_0_THERMAL_H
