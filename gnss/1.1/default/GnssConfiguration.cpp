#include "GnssConfiguration.h"

namespace android {
namespace hardware {
namespace gnss {
namespace V1_1 {
namespace implementation {

// Methods from ::android::hardware::gnss::V1_0::IGnssConfiguration follow.
Return<bool> GnssConfiguration::setSuplEs(bool) {
    // TODO implement
    return bool{};
}

Return<bool> GnssConfiguration::setSuplVersion(uint32_t) {
    // TODO implement
    return bool{};
}

Return<bool> GnssConfiguration::setSuplMode(hidl_bitfield<SuplMode>) {
    // TODO implement
    return bool{};
}

Return<bool> GnssConfiguration::setGpsLock(hidl_bitfield<GpsLock>) {
    // TODO implement
    return bool{};
}

Return<bool> GnssConfiguration::setLppProfile(hidl_bitfield<LppProfile>) {
    // TODO implement
    return bool{};
}

Return<bool> GnssConfiguration::setGlonassPositioningProtocol(hidl_bitfield<GlonassPosProtocol>) {
    // TODO implement
    return bool{};
}

Return<bool> GnssConfiguration::setEmergencySuplPdn(bool) {
    // TODO implement
    return bool{};
}

// Methods from ::android::hardware::gnss::V1_1::IGnssConfiguration follow.
Return<bool> GnssConfiguration::setBlacklist(
    const hidl_vec<::android::hardware::gnss::V1_1::IGnssConfiguration::BlacklistedSource>&) {
    // TODO implement
    return bool{};
}

// Methods from ::android::hidl::base::V1_0::IBase follow.

}  // namespace implementation
}  // namespace V1_1
}  // namespace gnss
}  // namespace hardware
}  // namespace android
