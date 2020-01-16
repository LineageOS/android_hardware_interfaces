/*
 * Copyright 2019, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ANDROID_HARDWARE_IDENTITY_IDENTITYCREDENTIALSTORE_H
#define ANDROID_HARDWARE_IDENTITY_IDENTITYCREDENTIALSTORE_H

#include <android/hardware/identity/1.0/IIdentityCredentialStore.h>

namespace android {
namespace hardware {
namespace identity {
namespace implementation {

using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::identity::V1_0::IIdentityCredentialStore;
using ::android::hardware::identity::V1_0::Result;
using ::android::hardware::identity::V1_0::ResultCode;

class IdentityCredentialStore : public IIdentityCredentialStore {
  public:
    IdentityCredentialStore() {}

    // The GCM chunk size used by this implementation is 64 KiB.
    static constexpr size_t kGcmChunkSize = 64 * 1024;

    // Methods from ::android::hardware::identity::IIdentityCredentialStore follow.
    Return<void> getHardwareInformation(getHardwareInformation_cb _hidl_cb) override;
    Return<void> createCredential(const hidl_string& docType, bool testCredential,
                                  createCredential_cb _hidl_cb) override;
    Return<void> getCredential(const hidl_vec<uint8_t>& credentialData,
                               getCredential_cb _hidl_cb) override;
};

}  // namespace implementation
}  // namespace identity
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_IDENTITY_IDENTITYCREDENTIALSTORE_H
