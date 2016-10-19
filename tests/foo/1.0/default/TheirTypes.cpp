#include "TheirTypes.h"

namespace android {
namespace hardware {
namespace tests {
namespace foo {
namespace V1_0 {
namespace implementation {

// Methods from ::android::hardware::tests::foo::V1_0::ITheirTypes follow.

ITheirTypes* HIDL_FETCH_ITheirTypes(const char* /* name */) {
    return new TheirTypes();
}

} // namespace implementation
}  // namespace V1_0
}  // namespace foo
}  // namespace tests
}  // namespace hardware
}  // namespace android
