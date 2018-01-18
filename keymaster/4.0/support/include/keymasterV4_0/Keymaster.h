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
    virtual ~Keymaster() {}

    struct VersionResult {
        ErrorCode error;
        uint8_t majorVersion;
        SecurityLevel securityLevel;
        bool supportsEc;
    };

    virtual VersionResult halVersion() = 0;
};

}  // namespace support
}  // namespace V4_0
}  // namespace keymaster
}  // namespace hardware
}  // namespace android

#endif  // HARDWARE_INTERFACES_KEYMASTER_40_SUPPORT_KEYMASTER_H_
