#ifndef HIDL_GENERATED_android_hardware_tests_inheritance_V1_0_Fetcher_H_
#define HIDL_GENERATED_android_hardware_tests_inheritance_V1_0_Fetcher_H_

#include "Child.h"
#include <android/hardware/tests/inheritance/1.0/IFetcher.h>
#include <hidl/Status.h>

#include <hidl/MQDescriptor.h>
namespace android {
namespace hardware {
namespace tests {
namespace inheritance {
namespace V1_0 {
namespace implementation {

using ::android::hardware::tests::inheritance::V1_0::IFetcher;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::hidl_vec;
using ::android::hardware::hidl_string;
using ::android::sp;

struct Fetcher : public IFetcher {

    Fetcher();

    // Methods from ::android::hardware::tests::inheritance::V1_0::IFetcher follow.
    Return<void> getGrandparent(bool sendRemote, getGrandparent_cb _hidl_cb)  override;
    Return<void> getParent(bool sendRemote, getParent_cb _hidl_cb)  override;
    Return<void> getChild(bool sendRemote, getChild_cb _hidl_cb)  override;

private:
    sp<IChild> mPrecious;
};

extern "C" IFetcher* HIDL_FETCH_IFetcher(const char* name);

}  // namespace implementation
}  // namespace V1_0
}  // namespace inheritance
}  // namespace tests
}  // namespace hardware
}  // namespace android

#endif  // HIDL_GENERATED_android_hardware_tests_inheritance_V1_0_Fetcher_H_
