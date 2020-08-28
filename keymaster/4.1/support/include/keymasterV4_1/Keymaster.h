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

#pragma once

#include <memory>
#include <vector>

#include <android/hardware/keymaster/4.1/IKeymasterDevice.h>
#include <keymasterV4_1/keymaster_tags.h>

namespace android::hardware::keymaster::V4_1::support {

/**
 * Keymaster abstracts the underlying V4_1::IKeymasterDevice.  There are two implementations,
 * Keymaster3 which wraps a V3_0::IKeymasterDevice and Keymaster4, which wraps either a
 * V4_0::IKeymasterDevice or a V4_1::IKeymasterDevice.  There is a V3_0::IKeymasterDevice
 * implementation that is used to wrap pre-HIDL keymaster implementations, and Keymaster3 will wrap
 * that.
 *
 * The reason for adding this additional layer, rather than simply using the latest HAL directly and
 * subclassing it to wrap any older HAL, is because this provides a place to put additional methods
 * which clients can use when they need to distinguish between different underlying HAL versions,
 * while still having to use only the latest interface.  Plus it's a handy place to keep some
 * convenience methods.
 */
class Keymaster : public IKeymasterDevice {
  public:
    using KeymasterSet = std::vector<android::sp<Keymaster>>;

    Keymaster(const hidl_string& descriptor, const hidl_string& instanceName)
        : descriptor_(descriptor), instanceName_(instanceName) {}
    virtual ~Keymaster() {}

    struct VersionResult {
        hidl_string keymasterName;
        hidl_string authorName;
        uint8_t majorVersion;
        uint8_t minorVersion;
        SecurityLevel securityLevel;
        bool supportsEc;

        bool operator>(const VersionResult& other) const {
            auto lhs = std::tie(securityLevel, majorVersion, minorVersion, supportsEc);
            auto rhs = std::tie(other.securityLevel, other.majorVersion, other.minorVersion,
                                other.supportsEc);
            return lhs > rhs;
        }
    };

    virtual const VersionResult& halVersion() const = 0;
    const hidl_string& descriptor() const { return descriptor_; }
    const hidl_string& instanceName() const { return instanceName_; }

    /**
     * If ec is in the vendor error code range (<-10000), logs the fact to logcat.
     * There are no side effects otherwise.
     */
    void logIfKeymasterVendorError(ErrorCode ec) const;
    void logIfKeymasterVendorError(V4_0::ErrorCode ec) const {
        logIfKeymasterVendorError(static_cast<ErrorCode>(ec));
    }

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

}  // namespace android::hardware::keymaster::V4_1::support
