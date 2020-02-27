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

#define LOG_TAG "WritableIdentityCredential"

#include "WritableIdentityCredential.h"
#include "IdentityCredentialStore.h"

#include <android/hardware/identity/support/IdentityCredentialSupport.h>

#include <android-base/logging.h>

#include <cppbor/cppbor.h>
#include <cppbor/cppbor_parse.h>

#include <utility>

#include "IdentityCredentialStore.h"
#include "Util.h"
#include "WritableIdentityCredential.h"

namespace aidl::android::hardware::identity {

using ::std::optional;
using namespace ::android::hardware::identity;

bool WritableIdentityCredential::initialize() {
    optional<vector<uint8_t>> random = support::getRandom(16);
    if (!random) {
        LOG(ERROR) << "Error creating storageKey";
        return false;
    }
    storageKey_ = random.value();

    return true;
}

// This function generates the attestation certificate using the passed in
// |attestationApplicationId| and |attestationChallenge|.  It will generate an
// attestation certificate with current time and expires one year from now.  The
// certificate shall contain all values as specified in hal.
ndk::ScopedAStatus WritableIdentityCredential::getAttestationCertificate(
        const vector<int8_t>& attestationApplicationId,  //
        const vector<int8_t>& attestationChallenge,      //
        vector<Certificate>* outCertificateChain) {
    if (!credentialPrivKey_.empty() || !credentialPubKey_.empty() || !certificateChain_.empty()) {
        return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                IIdentityCredentialStore::STATUS_FAILED,
                "Error attestation certificate previously generated"));
    }

    vector<uint8_t> challenge(attestationChallenge.begin(), attestationChallenge.end());
    vector<uint8_t> appId(attestationApplicationId.begin(), attestationApplicationId.end());

    optional<std::pair<vector<uint8_t>, vector<vector<uint8_t>>>> keyAttestationPair =
            support::createEcKeyPairAndAttestation(challenge, appId);
    if (!keyAttestationPair) {
        LOG(ERROR) << "Error creating credentialKey and attestation";
        return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                IIdentityCredentialStore::STATUS_FAILED,
                "Error creating credentialKey and attestation"));
    }

    vector<uint8_t> keyPair = keyAttestationPair.value().first;
    certificateChain_ = keyAttestationPair.value().second;

    optional<vector<uint8_t>> pubKey = support::ecKeyPairGetPublicKey(keyPair);
    if (!pubKey) {
        return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                IIdentityCredentialStore::STATUS_FAILED,
                "Error getting public part of credentialKey"));
    }
    credentialPubKey_ = pubKey.value();

    optional<vector<uint8_t>> privKey = support::ecKeyPairGetPrivateKey(keyPair);
    if (!privKey) {
        return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                IIdentityCredentialStore::STATUS_FAILED,
                "Error getting private part of credentialKey"));
    }
    credentialPrivKey_ = privKey.value();

    // convert from vector<vector<uint8_t>>> to vector<Certificate>*
    *outCertificateChain = vector<Certificate>();
    for (const vector<uint8_t>& cert : certificateChain_) {
        Certificate c = Certificate();
        c.encodedCertificate = byteStringToSigned(cert);
        outCertificateChain->push_back(std::move(c));
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus WritableIdentityCredential::startPersonalization(
        int32_t accessControlProfileCount, const vector<int32_t>& entryCounts) {
    numAccessControlProfileRemaining_ = accessControlProfileCount;
    remainingEntryCounts_ = entryCounts;
    entryNameSpace_ = "";

    signedDataAccessControlProfiles_ = cppbor::Array();
    signedDataNamespaces_ = cppbor::Map();
    signedDataCurrentNamespace_ = cppbor::Array();

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus WritableIdentityCredential::addAccessControlProfile(
        int32_t id, const Certificate& readerCertificate, bool userAuthenticationRequired,
        int64_t timeoutMillis, int64_t secureUserId,
        SecureAccessControlProfile* outSecureAccessControlProfile) {
    SecureAccessControlProfile profile;

    if (numAccessControlProfileRemaining_ == 0) {
        return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                IIdentityCredentialStore::STATUS_INVALID_DATA,
                "numAccessControlProfileRemaining_ is 0 and expected non-zero"));
    }

    // Spec requires if |userAuthenticationRequired| is false, then |timeoutMillis| must also
    // be zero.
    if (!userAuthenticationRequired && timeoutMillis != 0) {
        return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                IIdentityCredentialStore::STATUS_INVALID_DATA,
                "userAuthenticationRequired is false but timeout is non-zero"));
    }

    profile.id = id;
    profile.readerCertificate = readerCertificate;
    profile.userAuthenticationRequired = userAuthenticationRequired;
    profile.timeoutMillis = timeoutMillis;
    profile.secureUserId = secureUserId;
    optional<vector<uint8_t>> mac = secureAccessControlProfileCalcMac(profile, storageKey_);
    if (!mac) {
        return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                IIdentityCredentialStore::STATUS_FAILED, "Error calculating MAC for profile"));
    }
    profile.mac = byteStringToSigned(mac.value());

    cppbor::Map profileMap;
    profileMap.add("id", profile.id);
    if (profile.readerCertificate.encodedCertificate.size() > 0) {
        profileMap.add(
                "readerCertificate",
                cppbor::Bstr(byteStringToUnsigned(profile.readerCertificate.encodedCertificate)));
    }
    if (profile.userAuthenticationRequired) {
        profileMap.add("userAuthenticationRequired", profile.userAuthenticationRequired);
        profileMap.add("timeoutMillis", profile.timeoutMillis);
    }
    signedDataAccessControlProfiles_.add(std::move(profileMap));

    numAccessControlProfileRemaining_--;

    *outSecureAccessControlProfile = profile;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus WritableIdentityCredential::beginAddEntry(
        const vector<int32_t>& accessControlProfileIds, const string& nameSpace, const string& name,
        int32_t entrySize) {
    if (numAccessControlProfileRemaining_ != 0) {
        LOG(ERROR) << "numAccessControlProfileRemaining_ is " << numAccessControlProfileRemaining_
                   << " and expected zero";
        return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                IIdentityCredentialStore::STATUS_INVALID_DATA,
                "numAccessControlProfileRemaining_ is not zero"));
    }

    if (remainingEntryCounts_.size() == 0) {
        return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                IIdentityCredentialStore::STATUS_INVALID_DATA, "No more namespaces to add to"));
    }

    // Handle initial beginEntry() call.
    if (entryNameSpace_ == "") {
        entryNameSpace_ = nameSpace;
    }

    // If the namespace changed...
    if (nameSpace != entryNameSpace_) {
        // Then check that all entries in the previous namespace have been added..
        if (remainingEntryCounts_[0] != 0) {
            return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                    IIdentityCredentialStore::STATUS_INVALID_DATA,
                    "New namespace but a non-zero number of entries remain to be added"));
        }
        remainingEntryCounts_.erase(remainingEntryCounts_.begin());

        if (signedDataCurrentNamespace_.size() > 0) {
            signedDataNamespaces_.add(entryNameSpace_, std::move(signedDataCurrentNamespace_));
            signedDataCurrentNamespace_ = cppbor::Array();
        }
    } else {
        // Same namespace...
        if (remainingEntryCounts_[0] == 0) {
            return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                    IIdentityCredentialStore::STATUS_INVALID_DATA,
                    "Same namespace but no entries remain to be added"));
        }
        remainingEntryCounts_[0] -= 1;
    }

    entryAdditionalData_ = entryCreateAdditionalData(nameSpace, name, accessControlProfileIds);

    entryRemainingBytes_ = entrySize;
    entryNameSpace_ = nameSpace;
    entryName_ = name;
    entryAccessControlProfileIds_ = accessControlProfileIds;
    entryBytes_.resize(0);
    // LOG(INFO) << "name=" << name << " entrySize=" << entrySize;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus WritableIdentityCredential::addEntryValue(const vector<int8_t>& contentS,
                                                             vector<int8_t>* outEncryptedContent) {
    auto content = byteStringToUnsigned(contentS);
    size_t contentSize = content.size();

    if (contentSize > IdentityCredentialStore::kGcmChunkSize) {
        return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                IIdentityCredentialStore::STATUS_INVALID_DATA,
                "Passed in chunk of is bigger than kGcmChunkSize"));
    }
    if (contentSize > entryRemainingBytes_) {
        return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                IIdentityCredentialStore::STATUS_INVALID_DATA,
                "Passed in chunk is bigger than remaining space"));
    }

    entryBytes_.insert(entryBytes_.end(), content.begin(), content.end());
    entryRemainingBytes_ -= contentSize;
    if (entryRemainingBytes_ > 0) {
        if (contentSize != IdentityCredentialStore::kGcmChunkSize) {
            return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                    IIdentityCredentialStore::STATUS_INVALID_DATA,
                    "Retrieved non-final chunk which isn't kGcmChunkSize"));
        }
    }

    optional<vector<uint8_t>> nonce = support::getRandom(12);
    if (!nonce) {
        return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                IIdentityCredentialStore::STATUS_FAILED, "Error getting nonce"));
    }
    optional<vector<uint8_t>> encryptedContent =
            support::encryptAes128Gcm(storageKey_, nonce.value(), content, entryAdditionalData_);
    if (!encryptedContent) {
        return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                IIdentityCredentialStore::STATUS_FAILED, "Error encrypting content"));
    }

    if (entryRemainingBytes_ == 0) {
        // TODO: ideally do do this without parsing the data (but still validate data is valid
        // CBOR).
        auto [item, _, message] = cppbor::parse(entryBytes_);
        if (item == nullptr) {
            return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                    IIdentityCredentialStore::STATUS_INVALID_DATA, "Data is not valid CBOR"));
        }
        cppbor::Map entryMap;
        entryMap.add("name", entryName_);
        entryMap.add("value", std::move(item));
        cppbor::Array profileIdArray;
        for (auto id : entryAccessControlProfileIds_) {
            profileIdArray.add(id);
        }
        entryMap.add("accessControlProfiles", std::move(profileIdArray));
        signedDataCurrentNamespace_.add(std::move(entryMap));
    }

    *outEncryptedContent = byteStringToSigned(encryptedContent.value());
    return ndk::ScopedAStatus::ok();
}

// Writes CBOR-encoded structure to |credentialKeys| containing |storageKey| and
// |credentialPrivKey|.
static bool generateCredentialKeys(const vector<uint8_t>& storageKey,
                                   const vector<uint8_t>& credentialPrivKey,
                                   vector<uint8_t>& credentialKeys) {
    if (storageKey.size() != 16) {
        LOG(ERROR) << "Size of storageKey is not 16";
        return false;
    }

    cppbor::Array array;
    array.add(cppbor::Bstr(storageKey));
    array.add(cppbor::Bstr(credentialPrivKey));
    credentialKeys = array.encode();
    return true;
}

// Writes CBOR-encoded structure to |credentialData| containing |docType|,
// |testCredential| and |credentialKeys|. The latter element will be stored in
// encrypted form, using |hardwareBoundKey| as the encryption key.
bool generateCredentialData(const vector<uint8_t>& hardwareBoundKey, const string& docType,
                            bool testCredential, const vector<uint8_t>& credentialKeys,
                            vector<uint8_t>& credentialData) {
    optional<vector<uint8_t>> nonce = support::getRandom(12);
    if (!nonce) {
        LOG(ERROR) << "Error getting random";
        return false;
    }
    vector<uint8_t> docTypeAsVec(docType.begin(), docType.end());
    optional<vector<uint8_t>> credentialBlob = support::encryptAes128Gcm(
            hardwareBoundKey, nonce.value(), credentialKeys, docTypeAsVec);
    if (!credentialBlob) {
        LOG(ERROR) << "Error encrypting CredentialKeys blob";
        return false;
    }

    cppbor::Array array;
    array.add(docType);
    array.add(testCredential);
    array.add(cppbor::Bstr(credentialBlob.value()));
    credentialData = array.encode();
    return true;
}

ndk::ScopedAStatus WritableIdentityCredential::finishAddingEntries(
        vector<int8_t>* outCredentialData, vector<int8_t>* outProofOfProvisioningSignature) {
    if (signedDataCurrentNamespace_.size() > 0) {
        signedDataNamespaces_.add(entryNameSpace_, std::move(signedDataCurrentNamespace_));
    }
    cppbor::Array popArray;
    popArray.add("ProofOfProvisioning")
            .add(docType_)
            .add(std::move(signedDataAccessControlProfiles_))
            .add(std::move(signedDataNamespaces_))
            .add(testCredential_);
    vector<uint8_t> encodedCbor = popArray.encode();

    optional<vector<uint8_t>> signature = support::coseSignEcDsa(credentialPrivKey_,
                                                                 encodedCbor,  // payload
                                                                 {},           // additionalData
                                                                 {});          // certificateChain
    if (!signature) {
        return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                IIdentityCredentialStore::STATUS_FAILED, "Error signing data"));
    }

    vector<uint8_t> credentialKeys;
    if (!generateCredentialKeys(storageKey_, credentialPrivKey_, credentialKeys)) {
        return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                IIdentityCredentialStore::STATUS_FAILED, "Error generating CredentialKeys"));
    }

    vector<uint8_t> credentialData;
    if (!generateCredentialData(
                testCredential_ ? support::getTestHardwareBoundKey() : getHardwareBoundKey(),
                docType_, testCredential_, credentialKeys, credentialData)) {
        return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                IIdentityCredentialStore::STATUS_FAILED, "Error generating CredentialData"));
    }

    *outCredentialData = byteStringToSigned(credentialData);
    *outProofOfProvisioningSignature = byteStringToSigned(signature.value());
    return ndk::ScopedAStatus::ok();
}

}  // namespace aidl::android::hardware::identity
