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

#ifndef ANDROID_HARDWARE_IDENTITY_IDENTITYCREDENTIAL_H
#define ANDROID_HARDWARE_IDENTITY_IDENTITYCREDENTIAL_H

#include <android/hardware/identity/1.0/IIdentityCredential.h>

#include <android/hardware/identity/support/IdentityCredentialSupport.h>

#include <map>
#include <set>
#include <string>
#include <vector>

#include <cppbor/cppbor.h>

namespace android {
namespace hardware {
namespace identity {
namespace implementation {

using ::std::map;
using ::std::string;
using ::std::vector;

using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::identity::V1_0::IIdentityCredential;
using ::android::hardware::identity::V1_0::Result;
using ::android::hardware::identity::V1_0::ResultCode;
using ::android::hardware::identity::V1_0::SecureAccessControlProfile;
using ::android::hardware::keymaster::V4_0::HardwareAuthToken;

using MapStringToVectorOfStrings = map<string, vector<string>>;

class IdentityCredential : public IIdentityCredential {
  public:
    IdentityCredential(const hidl_vec<uint8_t>& credentialData)
        : credentialData_(credentialData), numStartRetrievalCalls_(0), authChallenge_(0) {}

    // Parses and decrypts credentialData_, return false on failure. Must be
    // called right after construction.
    ResultCode initialize();

    // Methods from ::android::hardware::identity::IIdentityCredential follow.

    Return<void> deleteCredential(deleteCredential_cb _hidl_cb) override;
    Return<void> createEphemeralKeyPair(createEphemeralKeyPair_cb _hidl_cb) override;

    Return<void> setReaderEphemeralPublicKey(const hidl_vec<uint8_t>& publicKey,
                                             setReaderEphemeralPublicKey_cb _hidl_cb) override;

    Return<void> createAuthChallenge(createAuthChallenge_cb _hidl_cb) override;

    Return<void> startRetrieval(const hidl_vec<SecureAccessControlProfile>& accessControlProfiles,
                                const HardwareAuthToken& authToken,
                                const hidl_vec<uint8_t>& itemsRequest,
                                const hidl_vec<uint8_t>& sessionTranscript,
                                const hidl_vec<uint8_t>& readerSignature,
                                const hidl_vec<uint16_t>& requestCounts,
                                startRetrieval_cb _hidl_cb) override;
    Return<void> startRetrieveEntryValue(const hidl_string& nameSpace, const hidl_string& name,
                                         uint32_t entrySize,
                                         const hidl_vec<uint16_t>& accessControlProfileIds,
                                         startRetrieveEntryValue_cb _hidl_cb) override;
    Return<void> retrieveEntryValue(const hidl_vec<uint8_t>& encryptedContent,
                                    retrieveEntryValue_cb _hidl_cb) override;
    Return<void> finishRetrieval(const hidl_vec<uint8_t>& signingKeyBlob,
                                 finishRetrieval_cb _hidl_cb) override;

    Return<void> generateSigningKeyPair(generateSigningKeyPair_cb _hidl_cb) override;

  private:
    // Set by constructor
    vector<uint8_t> credentialData_;
    int numStartRetrievalCalls_;

    // Set by initialize()
    string docType_;
    bool testCredential_;
    vector<uint8_t> storageKey_;
    vector<uint8_t> credentialPrivKey_;

    // Set by createEphemeralKeyPair()
    vector<uint8_t> ephemeralPublicKey_;

    // Set by setReaderEphemeralPublicKey()
    vector<uint8_t> readerPublicKey_;

    // Set by createAuthChallenge()
    uint64_t authChallenge_;

    // Set at startRetrieval() time.
    map<uint16_t, ResultCode> profileIdToAccessCheckResult_;
    vector<uint8_t> sessionTranscript_;
    std::unique_ptr<cppbor::Item> sessionTranscriptItem_;
    vector<uint8_t> itemsRequest_;
    vector<uint16_t> requestCountsRemaining_;
    MapStringToVectorOfStrings requestedNameSpacesAndNames_;
    cppbor::Map deviceNameSpacesMap_;
    cppbor::Map currentNameSpaceDeviceNameSpacesMap_;

    // Set at startRetrieveEntryValue() time.
    string currentNameSpace_;
    string currentName_;
    size_t entryRemainingBytes_;
    vector<uint8_t> entryValue_;
    vector<uint8_t> entryAdditionalData_;
};

}  // namespace implementation
}  // namespace identity
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_IDENTITY_IDENTITYCREDENTIAL_H
