#include "GnssBatching.h"

namespace android {
namespace hardware {
namespace gnss {
namespace V1_0 {
namespace implementation {

GnssBatching::GnssBatching(const FlpLocationInterface* flpLocationIface) :
    mFlpLocationIface(flpLocationIface) {}


// Methods from ::android::hardware::gnss::V1_0::IGnssBatching follow.
Return<bool> GnssBatching::init(const sp<IGnssBatchingCallback>& callback) {
    // TODO(b/34133439) implement
    return false;
}

Return<uint16_t> GnssBatching::getBatchSize() {
    // TODO(b/34133439) implement
    return 0;
}

Return<bool> GnssBatching::start(const IGnssBatching::Options& options) {
    // TODO(b/34133439) implement
    return false;
}

Return<void> GnssBatching::flush() {
    // TODO(b/34133439) implement
    return Void();
}

Return<bool> GnssBatching::stop() {
    // TODO(b/34133439) implement
    return false;
}

Return<void> GnssBatching::cleanup() {
    // TODO(b/34133439) implement
    return Void();
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace gnss
}  // namespace hardware
}  // namespace android
