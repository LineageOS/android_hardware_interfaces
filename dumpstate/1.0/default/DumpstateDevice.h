#ifndef ANDROID_HARDWARE_DUMPSTATE_V1_0_DUMPSTATEDEVICE_H
#define ANDROID_HARDWARE_DUMPSTATE_V1_0_DUMPSTATEDEVICE_H

#include <android/hardware/dumpstate/1.0/IDumpstateDevice.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>

namespace android {
namespace hardware {
namespace dumpstate {
namespace V1_0 {
namespace implementation {

using ::android::hardware::dumpstate::V1_0::IDumpstateDevice;
using ::android::hardware::hidl_array;
using ::android::hardware::hidl_handle;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;

struct DumpstateDevice : public IDumpstateDevice {
    // Methods from ::android::hardware::dumpstate::V1_0::IDumpstateDevice follow.
    Return<void> dumpstateBoard(const hidl_handle& h) override;

};

extern "C" IDumpstateDevice* HIDL_FETCH_IDumpstateDevice(const char* name);

}  // namespace implementation
}  // namespace V1_0
}  // namespace dumpstate
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_DUMPSTATE_V1_0_DUMPSTATEDEVICE_H
