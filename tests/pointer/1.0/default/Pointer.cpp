#include "Pointer.h"

namespace android {
namespace hardware {
namespace tests {
namespace pointer {
namespace V1_0 {
namespace implementation {

IPointer* HIDL_FETCH_IPointer(const char* /* name */) {
    return new Pointer();
}

} // namespace implementation
}  // namespace V1_0
}  // namespace pointer
}  // namespace tests
}  // namespace hardware
}  // namespace android
