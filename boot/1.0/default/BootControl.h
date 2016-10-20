#ifndef HIDL_GENERATED_android_hardware_boot_V1_0_BootControl_H_
#define HIDL_GENERATED_android_hardware_boot_V1_0_BootControl_H_

#include <android/hardware/boot/1.0/IBootControl.h>
#include <hidl/Status.h>

#include <hidl/MQDescriptor.h>
namespace android {
namespace hardware {
namespace boot {
namespace V1_0 {
namespace implementation {

using ::android::hardware::boot::V1_0::BoolResult;
using ::android::hardware::boot::V1_0::CommandResult;
using ::android::hardware::boot::V1_0::IBootControl;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::hidl_vec;
using ::android::hardware::hidl_string;
using ::android::sp;

struct BootControl : public IBootControl {
    BootControl(boot_control_module_t* module);
    // Methods from ::android::hardware::boot::V1_0::IBootControl follow.
    Return<uint32_t> getNumberSlots()  override;
    Return<uint32_t> getCurrentSlot()  override;
    Return<void> markBootSuccessful(markBootSuccessful_cb _hidl_cb)  override;
    Return<void> setActiveBootSlot(uint32_t slot, setActiveBootSlot_cb _hidl_cb)  override;
    Return<void> setSlotAsUnbootable(uint32_t slot, setSlotAsUnbootable_cb _hidl_cb)  override;
    Return<BoolResult> isSlotBootable(uint32_t slot)  override;
    Return<BoolResult> isSlotMarkedSuccessful(uint32_t slot)  override;
    Return<void> getSuffix(uint32_t slot, getSuffix_cb _hidl_cb)  override;
private:
    boot_control_module_t* mModule;
};

extern "C" IBootControl* HIDL_FETCH_IBootControl(const char* name);

}  // namespace implementation
}  // namespace V1_0
}  // namespace boot
}  // namespace hardware
}  // namespace android

#endif  // HIDL_GENERATED_android_hardware_boot_V1_0_BootControl_H_
