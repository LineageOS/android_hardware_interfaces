/*
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

#include <keymasterV4_1/Keymaster4.h>

#include <android-base/logging.h>

namespace android::hardware::keymaster::V4_1::support {

void Keymaster4::getVersionIfNeeded() {
    if (haveVersion_) return;

    auto rc = km4_0_dev_->getHardwareInfo([&](SecurityLevel securityLevel,
                                              const hidl_string& keymasterName,
                                              const hidl_string& authorName) {
        version_ = {keymasterName,
                    authorName,
                    4 /* major version */,
                    static_cast<uint8_t>((km4_1_dev_) ? 1 : 0) /* minor version */,
                    securityLevel,
                    true /* supportsEc */};
        haveVersion_ = true;
    });

    CHECK(rc.isOk()) << "Got error " << rc.description() << " trying to get hardware info";
}

}  // namespace android::hardware::keymaster::V4_1::support
