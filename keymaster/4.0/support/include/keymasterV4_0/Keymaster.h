/*
 **
 ** Copyright 2017, The Android Open Source Project
 **
 ** Licensed under the Apache License, Version 2.0 (the "License");
 ** you may not use this file except in compliance with the License.
 ** You may obtain a copy of the License at
 **
 **     http://www.apache.org/licenses/LICENSE-2.0
 **
 ** Unless required by applicable law or agreed to in writing, software
 ** distributed under the License is distributed on an "AS IS" BASIS,
 ** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 ** See the License for the specific language governing permissions and
 ** limitations under the License.
 */

#ifndef HARDWARE_INTERFACES_KEYMASTER_40_SUPPORT_KEYMASTER_H_
#define HARDWARE_INTERFACES_KEYMASTER_40_SUPPORT_KEYMASTER_H_

#include <android/hardware/keymaster/4.0/IKeymasterDevice.h>

namespace android {
namespace hardware {
namespace keymaster {
namespace V4_0 {
namespace support {

/**
 * Keymaster abstracts the underlying V4_0::IKeymasterDevice.  There is one implementation
 * (Keymaster4) which is a trivial passthrough and one that wraps a V3_0::IKeymasterDevice.
 *
 * The reason for adding this additional layer, rather than simply using the latest HAL directly and
 * subclassing it to wrap any older HAL, is because this provides a place to put additional methods
 * which clients can use when they need to distinguish between different underlying HAL versions,
 * while still having to use only the latest interface.
 */
class Keymaster : public IKeymasterDevice {
   public:
    using KeymasterSet = std::vector<std::unique_ptr<Keymaster>>;

    Keymaster(const hidl_string& descriptor, const hidl_string& instanceName)
        : descriptor_(descriptor), instanceName_(instanceName) {}
    virtual ~Keymaster() {}

    struct VersionResult {
        hidl_string keymasterName;
        hidl_string authorName;
        uint8_t majorVersion;
        SecurityLevel securityLevel;
        bool supportsEc;

        bool operator>(const VersionResult& other) const {
            auto lhs = std::tie(securityLevel, majorVersion, supportsEc);
            auto rhs = std::tie(other.securityLevel, other.majorVersion, other.supportsEc);
            return lhs > rhs;
        }
    };

    virtual const VersionResult& halVersion() const = 0;
    const hidl_string& descriptor() const { return descriptor_; }
    const hidl_string& instanceName() const { return instanceName_; }

    /**
     * Returns all available Keymaster3 and Keymaster4 instances, in order of most secure to least
     * secure (as defined by VersionResult::operator<).
     */
    static KeymasterSet enumerateAvailableDevices();

    /**
     * Ask provided Keymaster instances to compute a shared HMAC key using
     * getHmacSharingParameters() and computeSharedHmac().  This computation is idempotent as long
     * as the same set of Keymaster instances is used each time (and if all of the instances work
     * correctly).  It must be performed once per boot, but should do no harm to be repeated.
     *
     * If key agreement fails, this method will crash the process (with CHECK).
     */
    static void performHmacKeyAgreement(const KeymasterSet& keymasters);

   private:
    hidl_string descriptor_;
    hidl_string instanceName_;
};

std::ostream& operator<<(std::ostream& os, const Keymaster& keymaster);

}  // namespace support
}  // namespace V4_0
}  // namespace keymaster
}  // namespace hardware
}  // namespace android

#endif  // HARDWARE_INTERFACES_KEYMASTER_40_SUPPORT_KEYMASTER_H_
