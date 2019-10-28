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

#define LOG_TAG "IdentityCredentialStore"

#include "IdentityCredentialStore.h"
#include "IdentityCredential.h"
#include "WritableIdentityCredential.h"

#include <android-base/logging.h>

namespace android {
namespace hardware {
namespace identity {
namespace implementation {

// Methods from ::android::hardware::identity::IIdentityCredentialStore follow.

Return<void> IdentityCredentialStore::getHardwareInformation(getHardwareInformation_cb _hidl_cb) {
    _hidl_cb(support::resultOK(), "IdentityCredential Reference Implementation", "Google",
             kGcmChunkSize, false /* isDirectAccess */, {} /* supportedDocTypes */);
    return Void();
}

Return<void> IdentityCredentialStore::createCredential(const hidl_string& docType,
                                                       bool testCredential,
                                                       createCredential_cb _hidl_cb) {
    auto writable_credential = new WritableIdentityCredential(docType, testCredential);
    if (!writable_credential->initialize()) {
        _hidl_cb(support::result(ResultCode::FAILED,
                                 "Error initializing WritableIdentityCredential"),
                 writable_credential);
        return Void();
    }
    _hidl_cb(support::resultOK(), writable_credential);
    return Void();
}

Return<void> IdentityCredentialStore::getCredential(const hidl_vec<uint8_t>& credentialData,
                                                    getCredential_cb _hidl_cb) {
    auto credential = new IdentityCredential(credentialData);
    // We only support CIPHERSUITE_ECDHE_HKDF_ECDSA_WITH_AES_256_GCM_SHA256 right now.
    auto ret = credential->initialize();
    if (ret != ResultCode::OK) {
        _hidl_cb(support::result(ret, "Error initializing IdentityCredential"), credential);
        return Void();
    }
    _hidl_cb(support::resultOK(), credential);
    return Void();
}

}  // namespace implementation
}  // namespace identity
}  // namespace hardware
}  // namespace android
