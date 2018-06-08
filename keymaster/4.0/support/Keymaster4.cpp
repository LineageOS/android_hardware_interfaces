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

#include <keymasterV4_0/Keymaster4.h>

#include <android-base/logging.h>

namespace android {
namespace hardware {
namespace keymaster {
namespace V4_0 {
namespace support {

void Keymaster4::getVersionIfNeeded() {
    if (haveVersion_) return;

    auto rc =
        dev_->getHardwareInfo([&](SecurityLevel securityLevel, const hidl_string& keymasterName,
                                  const hidl_string& authorName) {
            version_ = {keymasterName, authorName, 4 /* major version */, securityLevel,
                        true /* supportsEc */};
            haveVersion_ = true;
        });

    CHECK(rc.isOk()) << "Got error " << rc.description() << " trying to get hardware info";
}

}  // namespace support
}  // namespace V4_0
}  // namespace keymaster
}  // namespace hardware
}  // namespace android
