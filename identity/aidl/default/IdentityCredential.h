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

#include <aidl/android/hardware/identity/BnIdentityCredential.h>
#include <aidl/android/hardware/keymaster/HardwareAuthToken.h>
#include <android/hardware/identity/support/IdentityCredentialSupport.h>

#include <map>
#include <set>
#include <string>
#include <vector>

#include <cppbor/cppbor.h>

namespace aidl::android::hardware::identity {

using ::aidl::android::hardware::keymaster::HardwareAuthToken;
using ::std::map;
using ::std::string;
using ::std::vector;

using MapStringToVectorOfStrings = map<string, vector<string>>;

class IdentityCredential : public BnIdentityCredential {
  public:
    IdentityCredential(const vector<uint8_t>& credentialData)
        : credentialData_(credentialData), numStartRetrievalCalls_(0), authChallenge_(0) {}

    // Parses and decrypts credentialData_, return a status code from
    // IIdentityCredentialStore. Must be called right after construction.
    int initialize();

    // Methods from IIdentityCredential follow.
    ndk::ScopedAStatus deleteCredential(vector<int8_t>* outProofOfDeletionSignature) override;
    ndk::ScopedAStatus createEphemeralKeyPair(vector<int8_t>* outKeyPair) override;
    ndk::ScopedAStatus setReaderEphemeralPublicKey(const vector<int8_t>& publicKey) override;
    ndk::ScopedAStatus createAuthChallenge(int64_t* outChallenge) override;
    ndk::ScopedAStatus startRetrieval(
            const vector<SecureAccessControlProfile>& accessControlProfiles,
            const HardwareAuthToken& authToken, const vector<int8_t>& itemsRequest,
            const vector<int8_t>& sessionTranscript, const vector<int8_t>& readerSignature,
            const vector<int32_t>& requestCounts) override;
    ndk::ScopedAStatus startRetrieveEntryValue(
            const string& nameSpace, const string& name, int32_t entrySize,
            const vector<int32_t>& accessControlProfileIds) override;
    ndk::ScopedAStatus retrieveEntryValue(const vector<int8_t>& encryptedContent,
                                          vector<int8_t>* outContent) override;
    ndk::ScopedAStatus finishRetrieval(const vector<int8_t>& signingKeyBlob, vector<int8_t>* outMac,
                                       vector<int8_t>* outDeviceNameSpaces) override;
    ndk::ScopedAStatus generateSigningKeyPair(vector<int8_t>* outSigningKeyBlob,
                                              Certificate* outSigningKeyCertificate) override;

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
    map<int32_t, int> profileIdToAccessCheckResult_;
    vector<uint8_t> sessionTranscript_;
    std::unique_ptr<cppbor::Item> sessionTranscriptItem_;
    vector<uint8_t> itemsRequest_;
    vector<int32_t> requestCountsRemaining_;
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

}  // namespace aidl::android::hardware::identity

#endif  // ANDROID_HARDWARE_IDENTITY_IDENTITYCREDENTIAL_H
