#include "MyTypes.h"

namespace android {
namespace hardware {
namespace tests {
namespace foo {
namespace V1_0 {
namespace implementation {

// Methods from ::android::hardware::tests::foo::V1_0::IMyTypes follow.

IMyTypes* HIDL_FETCH_IMyTypes(const char* /* name */) {
    return new MyTypes();
}

} // namespace implementation
}  // namespace V1_0
}  // namespace foo
}  // namespace tests
}  // namespace hardware
}  // namespace android
