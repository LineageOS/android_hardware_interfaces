#include "SurfaceFlingerConfigs.h"

#include <android-base/logging.h>

namespace android {
namespace hardware {
namespace configstore {
namespace V1_0 {
namespace implementation {

// Methods from ::android::hardware::configstore::V1_0::ISurfaceFlingerConfigs follow.
Return<void> SurfaceFlingerConfigs::vsyncEventPhaseOffsetNs(vsyncEventPhaseOffsetNs_cb _hidl_cb) {
#ifdef VSYNC_EVENT_PHASE_OFFSET_NS
    _hidl_cb({true, VSYNC_EVENT_PHASE_OFFSET_NS});
    LOG(INFO) << "vsync event phase offset ns =  " << VSYNC_EVENT_PHASE_OFFSET_NS;
#else
    _hidl_cb({false, 0});
#endif
    return Void();
}


// Methods from ::android::hidl::base::V1_0::IBase follow.

ISurfaceFlingerConfigs* HIDL_FETCH_ISurfaceFlingerConfigs(const char* /* name */) {
    return new SurfaceFlingerConfigs();
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace configstore
}  // namespace hardware
}  // namespace android
