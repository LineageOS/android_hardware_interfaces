#define LOG_TAG "dumpstate"

#include "DumpstateDevice.h"

#include <log/log.h>

#include "DumpstateUtil.h"

namespace android {
namespace hardware {
namespace dumpstate {
namespace V1_0 {
namespace implementation {

// Methods from ::android::hardware::dumpstate::V1_0::IDumpstateDevice follow.
Return<void> DumpstateDevice::dumpstateBoard(const native_handle_t* handle) {
    if (handle->numFds < 1) {
        ALOGE("no FDs\n");
        return Void();
    }

    int fd = handle->data[0];
    if (fd < 0) {
        ALOGE("invalid FD: %d\n", handle->data[0]);
        return Void();
    }
    ALOGD("DumpstateDevice::dumpstateBoard() FD: %d\n", fd);
    ALOGI("Dumpstate HIDL not provided by device\n");
    dprintf(fd, "Dumpstate HIDL not provided by device; providing bogus data.\n");

    // Shows some examples on how to use the libdumpstateutils API.
    dprintf(fd, "Time now is: ");
    RunCommandToFd(fd, {"/system/bin/date"});
    dprintf(fd, "Contents of a small file (/system/etc/hosts):\n");
    DumpFileToFd(fd, "/system/etc/hosts");

    return Void();
}


IDumpstateDevice* HIDL_FETCH_IDumpstateDevice(const char* /* name */) {
    // TODO: temporary returning nullptr until it's implemented on master devices
    return nullptr;
//    return new DumpstateDevice();
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace dumpstate
}  // namespace hardware
}  // namespace android
