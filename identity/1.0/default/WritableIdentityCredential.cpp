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

namespace android {
namespace hardware {
namespace identity {
namespace implementation {

using ::std::optional;

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

bool WritableIdentityCredential::initialize() {
    optional<vector<uint8_t>> keyPair = support::createEcKeyPair();
    if (!keyPair) {
        LOG(ERROR) << "Error creating credentialKey";
        return false;
    }

    optional<vector<uint8_t>> pubKey = support::ecKeyPairGetPublicKey(keyPair.value());
    if (!pubKey) {
        LOG(ERROR) << "Error getting public part of credentialKey";
        return false;
    }
    credentialPubKey_ = pubKey.value();

    optional<vector<uint8_t>> privKey = support::ecKeyPairGetPrivateKey(keyPair.value());
    if (!privKey) {
        LOG(ERROR) << "Error getting private part of credentialKey";
        return false;
    }
    credentialPrivKey_ = privKey.value();

    optional<vector<uint8_t>> random = support::getRandom(16);
    if (!random) {
        LOG(ERROR) << "Error creating storageKey";
        return false;
    }
    storageKey_ = random.value();

    return true;
}

// TODO: use |attestationApplicationId| and |attestationChallenge| and also
//       ensure the returned certificate chain satisfy the requirements listed in
//       the docs for IWritableIdentityCredential::getAttestationCertificate()
//
Return<void> WritableIdentityCredential::getAttestationCertificate(
        const hidl_vec<uint8_t>& /* attestationApplicationId */,
        const hidl_vec<uint8_t>& /* attestationChallenge */,
        getAttestationCertificate_cb _hidl_cb) {
    // For now, we dynamically generate an attestion key on each and every
    // request and use that to sign CredentialKey. In a real implementation this
    // would look very differently.
    optional<vector<uint8_t>> attestationKeyPair = support::createEcKeyPair();
    if (!attestationKeyPair) {
        _hidl_cb(support::result(ResultCode::FAILED, "Error creating attestationKey"), {});
        return Void();
    }

    optional<vector<uint8_t>> attestationPubKey =
            support::ecKeyPairGetPublicKey(attestationKeyPair.value());
    if (!attestationPubKey) {
        _hidl_cb(support::result(ResultCode::FAILED, "Error getting public part of attestationKey"),
                 {});
        return Void();
    }

    optional<vector<uint8_t>> attestationPrivKey =
            support::ecKeyPairGetPrivateKey(attestationKeyPair.value());
    if (!attestationPrivKey) {
        _hidl_cb(
                support::result(ResultCode::FAILED, "Error getting private part of attestationKey"),
                {});
        return Void();
    }

    string serialDecimal;
    string issuer;
    string subject;
    time_t validityNotBefore = time(nullptr);
    time_t validityNotAfter = validityNotBefore + 365 * 24 * 3600;

    // First create a certificate for |credentialPubKey| which is signed by
    // |attestationPrivKey|.
    //
    serialDecimal = "0";  // TODO: set serial to |attestationChallenge|
    issuer = "Android Open Source Project";
    subject = "Android IdentityCredential CredentialKey";
    optional<vector<uint8_t>> credentialPubKeyCertificate = support::ecPublicKeyGenerateCertificate(
            credentialPubKey_, attestationPrivKey.value(), serialDecimal, issuer, subject,
            validityNotBefore, validityNotAfter);
    if (!credentialPubKeyCertificate) {
        _hidl_cb(support::result(ResultCode::FAILED,
                                 "Error creating certificate for credentialPubKey"),
                 {});
        return Void();
    }

    // This is followed by a certificate for |attestationPubKey| self-signed by
    // |attestationPrivKey|.
    serialDecimal = "0";  // TODO: set serial
    issuer = "Android Open Source Project";
    subject = "Android IdentityCredential AttestationKey";
    optional<vector<uint8_t>> attestationKeyCertificate = support::ecPublicKeyGenerateCertificate(
            attestationPubKey.value(), attestationPrivKey.value(), serialDecimal, issuer, subject,
            validityNotBefore, validityNotAfter);
    if (!attestationKeyCertificate) {
        _hidl_cb(support::result(ResultCode::FAILED,
                                 "Error creating certificate for attestationPubKey"),
                 {});
        return Void();
    }

    // Concatenate the certificates to form the chain.
    vector<uint8_t> certificateChain;
    certificateChain.insert(certificateChain.end(), credentialPubKeyCertificate.value().begin(),
                            credentialPubKeyCertificate.value().end());
    certificateChain.insert(certificateChain.end(), attestationKeyCertificate.value().begin(),
                            attestationKeyCertificate.value().end());

    optional<vector<vector<uint8_t>>> splitCertChain =
            support::certificateChainSplit(certificateChain);
    if (!splitCertChain) {
        _hidl_cb(support::result(ResultCode::FAILED, "Error splitting certificate chain"), {});
        return Void();
    }
    hidl_vec<hidl_vec<uint8_t>> ret;
    ret.resize(splitCertChain.value().size());
    std::copy(splitCertChain.value().begin(), splitCertChain.value().end(), ret.begin());
    _hidl_cb(support::resultOK(), ret);
    return Void();
}

Return<void> WritableIdentityCredential::startPersonalization(uint16_t accessControlProfileCount,
                                                              const hidl_vec<uint16_t>& entryCounts,
                                                              startPersonalization_cb _hidl_cb) {
    numAccessControlProfileRemaining_ = accessControlProfileCount;
    remainingEntryCounts_ = entryCounts;
    entryNameSpace_ = "";

    signedDataAccessControlProfiles_ = cppbor::Array();
    signedDataNamespaces_ = cppbor::Map();
    signedDataCurrentNamespace_ = cppbor::Array();

    _hidl_cb(support::resultOK());
    return Void();
}

Return<void> WritableIdentityCredential::addAccessControlProfile(
        uint16_t id, const hidl_vec<uint8_t>& readerCertificate, bool userAuthenticationRequired,
        uint64_t timeoutMillis, uint64_t secureUserId, addAccessControlProfile_cb _hidl_cb) {
    SecureAccessControlProfile profile;

    if (numAccessControlProfileRemaining_ == 0) {
        _hidl_cb(support::result(ResultCode::INVALID_DATA,
                                 "numAccessControlProfileRemaining_ is 0 and expected non-zero"),
                 profile);
        return Void();
    }

    // Spec requires if |userAuthenticationRequired| is false, then |timeoutMillis| must also
    // be zero.
    if (!userAuthenticationRequired && timeoutMillis != 0) {
        _hidl_cb(support::result(ResultCode::INVALID_DATA,
                                 "userAuthenticationRequired is false but timeout is non-zero"),
                 profile);
        return Void();
    }

    profile.id = id;
    profile.readerCertificate = readerCertificate;
    profile.userAuthenticationRequired = userAuthenticationRequired;
    profile.timeoutMillis = timeoutMillis;
    profile.secureUserId = secureUserId;
    optional<vector<uint8_t>> mac =
            support::secureAccessControlProfileCalcMac(profile, storageKey_);
    if (!mac) {
        _hidl_cb(support::result(ResultCode::FAILED, "Error calculating MAC for profile"), profile);
        return Void();
    }
    profile.mac = mac.value();

    cppbor::Map profileMap;
    profileMap.add("id", profile.id);
    if (profile.readerCertificate.size() > 0) {
        profileMap.add("readerCertificate", cppbor::Bstr(profile.readerCertificate));
    }
    if (profile.userAuthenticationRequired) {
        profileMap.add("userAuthenticationRequired", profile.userAuthenticationRequired);
        profileMap.add("timeoutMillis", profile.timeoutMillis);
    }
    signedDataAccessControlProfiles_.add(std::move(profileMap));

    numAccessControlProfileRemaining_--;

    _hidl_cb(support::resultOK(), profile);
    return Void();
}

Return<void> WritableIdentityCredential::beginAddEntry(
        const hidl_vec<uint16_t>& accessControlProfileIds, const hidl_string& nameSpace,
        const hidl_string& name, uint32_t entrySize, beginAddEntry_cb _hidl_cb) {
    if (numAccessControlProfileRemaining_ != 0) {
        LOG(ERROR) << "numAccessControlProfileRemaining_ is " << numAccessControlProfileRemaining_
                   << " and expected zero";
        _hidl_cb(support::result(ResultCode::INVALID_DATA,
                                 "numAccessControlProfileRemaining_ is %zd and expected zero",
                                 numAccessControlProfileRemaining_));
        return Void();
    }

    if (remainingEntryCounts_.size() == 0) {
        _hidl_cb(support::result(ResultCode::INVALID_DATA, "No more namespaces to add to"));
        return Void();
    }

    // Handle initial beginEntry() call.
    if (entryNameSpace_ == "") {
        entryNameSpace_ = nameSpace;
    }

    // If the namespace changed...
    if (nameSpace != entryNameSpace_) {
        // Then check that all entries in the previous namespace have been added..
        if (remainingEntryCounts_[0] != 0) {
            _hidl_cb(support::result(ResultCode::INVALID_DATA,
                                     "New namespace but %d entries remain to be added",
                                     int(remainingEntryCounts_[0])));
            return Void();
        }
        remainingEntryCounts_.erase(remainingEntryCounts_.begin());

        if (signedDataCurrentNamespace_.size() > 0) {
            signedDataNamespaces_.add(entryNameSpace_, std::move(signedDataCurrentNamespace_));
            signedDataCurrentNamespace_ = cppbor::Array();
        }
    } else {
        // Same namespace...
        if (remainingEntryCounts_[0] == 0) {
            _hidl_cb(support::result(ResultCode::INVALID_DATA,
                                     "Same namespace but no entries remain to be added"));
            return Void();
        }
        remainingEntryCounts_[0] -= 1;
    }

    entryAdditionalData_ =
            support::entryCreateAdditionalData(nameSpace, name, accessControlProfileIds);

    entryRemainingBytes_ = entrySize;
    entryNameSpace_ = nameSpace;
    entryName_ = name;
    entryAccessControlProfileIds_ = accessControlProfileIds;
    entryBytes_.resize(0);
    // LOG(INFO) << "name=" << name << " entrySize=" << entrySize;

    _hidl_cb(support::resultOK());
    return Void();
}

Return<void> WritableIdentityCredential::addEntryValue(const hidl_vec<uint8_t>& content,
                                                       addEntryValue_cb _hidl_cb) {
    size_t contentSize = content.size();

    if (contentSize > IdentityCredentialStore::kGcmChunkSize) {
        _hidl_cb(support::result(
                         ResultCode::INVALID_DATA,
                         "Passed in chunk of size %zd is bigger than kGcmChunkSize which is %zd",
                         contentSize, IdentityCredentialStore::kGcmChunkSize),
                 {});
        return Void();
    }
    if (contentSize > entryRemainingBytes_) {
        _hidl_cb(support::result(ResultCode::INVALID_DATA,
                                 "Passed in chunk of size %zd is bigger than remaining space "
                                 "of size %zd",
                                 contentSize, entryRemainingBytes_),
                 {});
        return Void();
    }

    entryBytes_.insert(entryBytes_.end(), content.begin(), content.end());
    entryRemainingBytes_ -= contentSize;
    if (entryRemainingBytes_ > 0) {
        if (contentSize != IdentityCredentialStore::kGcmChunkSize) {
            _hidl_cb(support::result(ResultCode::INVALID_DATA,
                                     "Retrieved non-final chunk of size %zd but expected "
                                     "kGcmChunkSize which is %zd",
                                     contentSize, IdentityCredentialStore::kGcmChunkSize),
                     {});
            return Void();
        }
    }

    optional<vector<uint8_t>> nonce = support::getRandom(12);
    if (!nonce) {
        _hidl_cb(support::result(ResultCode::FAILED, "Error getting nonce"), {});
        return Void();
    }
    optional<vector<uint8_t>> encryptedContent =
            support::encryptAes128Gcm(storageKey_, nonce.value(), content, entryAdditionalData_);
    if (!encryptedContent) {
        _hidl_cb(support::result(ResultCode::FAILED, "Error encrypting content"), {});
        return Void();
    }

    if (entryRemainingBytes_ == 0) {
        // TODO: ideally do do this without parsing the data (but still validate data is valid
        // CBOR).
        auto [item, _, message] = cppbor::parse(entryBytes_);
        if (item == nullptr) {
            _hidl_cb(support::result(ResultCode::INVALID_DATA, "Data is not valid CBOR"), {});
            return Void();
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

    _hidl_cb(support::resultOK(), encryptedContent.value());
    return Void();
}

Return<void> WritableIdentityCredential::finishAddingEntries(finishAddingEntries_cb _hidl_cb) {
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
        _hidl_cb(support::result(ResultCode::FAILED, "Error signing data"), {}, {});
        return Void();
    }

    vector<uint8_t> credentialKeys;
    if (!generateCredentialKeys(storageKey_, credentialPrivKey_, credentialKeys)) {
        _hidl_cb(support::result(ResultCode::FAILED, "Error generating CredentialKeys"), {}, {});
        return Void();
    }

    vector<uint8_t> credentialData;
    if (!generateCredentialData(testCredential_ ? support::getTestHardwareBoundKey()
                                                : support::getHardwareBoundKey(),
                                docType_, testCredential_, credentialKeys, credentialData)) {
        _hidl_cb(support::result(ResultCode::FAILED, "Error generating CredentialData"), {}, {});
        return Void();
    }

    _hidl_cb(support::resultOK(), credentialData, signature.value());
    return Void();
}

}  // namespace implementation
}  // namespace identity
}  // namespace hardware
}  // namespace android
