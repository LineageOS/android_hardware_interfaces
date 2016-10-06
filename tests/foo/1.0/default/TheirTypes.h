#ifndef HIDL_GENERATED_android_hardware_tests_foo_V1_0_TheirTypes_H_
#define HIDL_GENERATED_android_hardware_tests_foo_V1_0_TheirTypes_H_

#include <android/hardware/tests/foo/1.0/ITheirTypes.h>
#include <hidl/Status.h>

#include <hidl/MQDescriptor.h>
namespace android {
namespace hardware {
namespace tests {
namespace foo {
namespace V1_0 {
namespace implementation {

using ::android::hardware::tests::foo::V1_0::ITheirTypes;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::hidl_vec;
using ::android::hardware::hidl_string;
using ::android::sp;

struct TheirTypes : public ITheirTypes {
    // Methods from ::android::hardware::tests::foo::V1_0::ITheirTypes follow.

};

extern "C" ITheirTypes* HIDL_FETCH_ITheirTypes(const char* name);

}  // namespace implementation
}  // namespace V1_0
}  // namespace foo
}  // namespace tests
}  // namespace hardware
}  // namespace android

#endif  // HIDL_GENERATED_android_hardware_tests_foo_V1_0_TheirTypes_H_
