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

#include <aidl/android/hardware/identity/BnWritableIdentityCredential.h>
#include <android/hardware/identity/support/IdentityCredentialSupport.h>

#include <cppbor.h>

namespace aidl::android::hardware::identity {

using ::std::string;
using ::std::vector;

class WritableIdentityCredential : public BnWritableIdentityCredential {
  public:
    WritableIdentityCredential(const string& docType, bool testCredential)
        : docType_(docType), testCredential_(testCredential) {}

    // Creates the Credential Key. Returns false on failure. Must be called
    // right after construction.
    bool initialize();

    // Methods from IWritableIdentityCredential follow.
    ndk::ScopedAStatus getAttestationCertificate(const vector<int8_t>& attestationApplicationId,
                                                 const vector<int8_t>& attestationChallenge,
                                                 vector<Certificate>* outCertificateChain) override;

    ndk::ScopedAStatus startPersonalization(int32_t accessControlProfileCount,
                                            const vector<int32_t>& entryCounts) override;

    ndk::ScopedAStatus addAccessControlProfile(
            int32_t id, const Certificate& readerCertificate, bool userAuthenticationRequired,
            int64_t timeoutMillis, int64_t secureUserId,
            SecureAccessControlProfile* outSecureAccessControlProfile) override;

    ndk::ScopedAStatus beginAddEntry(const vector<int32_t>& accessControlProfileIds,
                                     const string& nameSpace, const string& name,
                                     int32_t entrySize) override;

    ndk::ScopedAStatus addEntryValue(const vector<int8_t>& content,
                                     vector<int8_t>* outEncryptedContent) override;

    ndk::ScopedAStatus finishAddingEntries(
            vector<int8_t>* outCredentialData,
            vector<int8_t>* outProofOfProvisioningSignature) override;

    // private:
    string docType_;
    bool testCredential_;

    // These are set in initialize().
    vector<uint8_t> storageKey_;
    vector<uint8_t> credentialPrivKey_;
    vector<uint8_t> credentialPubKey_;

    // These fields are initialized during startPersonalization()
    size_t numAccessControlProfileRemaining_;
    vector<int32_t> remainingEntryCounts_;
    cppbor::Array signedDataAccessControlProfiles_;
    cppbor::Map signedDataNamespaces_;
    cppbor::Array signedDataCurrentNamespace_;

    // These fields are initialized during beginAddEntry()
    size_t entryRemainingBytes_;
    vector<uint8_t> entryAdditionalData_;
    string entryNameSpace_;
    string entryName_;
    vector<int32_t> entryAccessControlProfileIds_;
    vector<uint8_t> entryBytes_;
};

}  // namespace aidl::android::hardware::identity

#endif  // ANDROID_HARDWARE_IDENTITY_WRITABLEIDENTITYCREDENTIAL_H
