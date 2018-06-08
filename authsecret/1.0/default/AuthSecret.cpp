#include "AuthSecret.h"

namespace android {
namespace hardware {
namespace authsecret {
namespace V1_0 {
namespace implementation {

// Methods from ::android::hardware::authsecret::V1_0::IAuthSecret follow.
Return<void> AuthSecret::primaryUserCredential(const hidl_vec<uint8_t>& secret) {
    (void)secret;

    // To create a dependency on the credential, it is recommended to derive a
    // different value from the provided secret for each purpose e.g.
    //
    //     purpose1_secret = hash( "purpose1" || secret )
    //     purpose2_secret = hash( "purpose2" || secret )
    //
    // The derived values can then be used as cryptographic keys or stored
    // securely for comparison in a future call.
    //
    // For example, a security module might require that the credential has been
    // entered before it applies any updates. This can be achieved by storing a
    // derived value in the module and only applying updates when the same
    // derived value is presented again.
    //
    // This implementation does nothing.

    return Void();
}

// Note: on factory reset, clear all dependency on the secret.
//
// With the example of updating a security module, the stored value must be
// cleared so that the new primary user enrolled as the approver of updates.
//
// This implementation does nothing as there is no dependence on the secret.

}  // namespace implementation
}  // namespace V1_0
}  // namespace authsecret
}  // namespace hardware
}  // namespace android
