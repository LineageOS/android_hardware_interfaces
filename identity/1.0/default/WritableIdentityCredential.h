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

#ifndef ANDROID_HARDWARE_IDENTITY_WRITABLEIDENTITYCREDENTIAL_H
#define ANDROID_HARDWARE_IDENTITY_WRITABLEIDENTITYCREDENTIAL_H

#include <android/hardware/identity/1.0/IWritableIdentityCredential.h>

#include <android/hardware/identity/support/IdentityCredentialSupport.h>

#include <cppbor.h>

namespace android {
namespace hardware {
namespace identity {
namespace implementation {

using ::std::string;
using ::std::vector;

using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::identity::V1_0::IWritableIdentityCredential;
using ::android::hardware::identity::V1_0::Result;
using ::android::hardware::identity::V1_0::ResultCode;
using ::android::hardware::identity::V1_0::SecureAccessControlProfile;

class WritableIdentityCredential : public IWritableIdentityCredential {
  public:
    WritableIdentityCredential(const hidl_string& docType, bool testCredential)
        : docType_(docType), testCredential_(testCredential) {}

    // Creates the Credential Key. Returns false on failure. Must be called
    // right after construction.
    bool initialize();

    // Methods from ::android::hardware::identity::IWritableIdentityCredential
    // follow.
    Return<void> getAttestationCertificate(const hidl_vec<uint8_t>& attestationChallenge,
                                           getAttestationCertificate_cb _hidl_cb) override;

    Return<void> startPersonalization(uint16_t accessControlProfileCount,
                                      const hidl_vec<uint16_t>& entryCounts,
                                      startPersonalization_cb _hidl_cb) override;

    Return<void> addAccessControlProfile(uint16_t id, const hidl_vec<uint8_t>& readerCertificate,
                                         bool userAuthenticationRequired, uint64_t timeoutMillis,
                                         uint64_t secureUserId,
                                         addAccessControlProfile_cb _hidl_cb) override;

    Return<void> beginAddEntry(const hidl_vec<uint16_t>& accessControlProfileIds,
                               const hidl_string& nameSpace, const hidl_string& name,
                               uint32_t entrySize, beginAddEntry_cb _hidl_cb) override;

    Return<void> addEntryValue(const hidl_vec<uint8_t>& content,
                               addEntryValue_cb _hidl_cb) override;

    Return<void> finishAddingEntries(finishAddingEntries_cb _hidl_cb) override;

  private:
    string docType_;
    bool testCredential_;

    // These are set in initialize().
    vector<uint8_t> storageKey_;
    vector<uint8_t> credentialPrivKey_;
    vector<uint8_t> credentialPubKey_;

    // These fields are initialized during startPersonalization()
    size_t numAccessControlProfileRemaining_;
    vector<uint16_t> remainingEntryCounts_;
    cppbor::Array signedDataAccessControlProfiles_;
    cppbor::Map signedDataNamespaces_;
    cppbor::Array signedDataCurrentNamespace_;

    // These fields are initialized during beginAddEntry()
    size_t entryRemainingBytes_;
    vector<uint8_t> entryAdditionalData_;
    string entryNameSpace_;
    string entryName_;
    vector<uint16_t> entryAccessControlProfileIds_;
    vector<uint8_t> entryBytes_;
};

}  // namespace implementation
}  // namespace identity
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_IDENTITY_WRITABLEIDENTITYCREDENTIAL_H
