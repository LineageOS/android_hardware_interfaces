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

#define LOG_TAG "IdentityCredential"

#include "IdentityCredential.h"
#include "IdentityCredentialStore.h"
#include "Util.h"

#include <android/hardware/identity/support/IdentityCredentialSupport.h>

#include <string.h>

#include <android-base/logging.h>

#include <cppbor.h>
#include <cppbor_parse.h>

namespace aidl::android::hardware::identity {

using ::aidl::android::hardware::keymaster::Timestamp;
using ::std::optional;

using namespace ::android::hardware::identity;

int IdentityCredential::initialize() {
    auto [item, _, message] = cppbor::parse(credentialData_);
    if (item == nullptr) {
        LOG(ERROR) << "CredentialData is not valid CBOR: " << message;
        return IIdentityCredentialStore::STATUS_INVALID_DATA;
    }

    const cppbor::Array* arrayItem = item->asArray();
    if (arrayItem == nullptr || arrayItem->size() != 3) {
        LOG(ERROR) << "CredentialData is not an array with three elements";
        return IIdentityCredentialStore::STATUS_INVALID_DATA;
    }

    const cppbor::Tstr* docTypeItem = (*arrayItem)[0]->asTstr();
    const cppbor::Bool* testCredentialItem =
            ((*arrayItem)[1]->asSimple() != nullptr ? ((*arrayItem)[1]->asSimple()->asBool())
                                                    : nullptr);
    const cppbor::Bstr* encryptedCredentialKeysItem = (*arrayItem)[2]->asBstr();
    if (docTypeItem == nullptr || testCredentialItem == nullptr ||
        encryptedCredentialKeysItem == nullptr) {
        LOG(ERROR) << "CredentialData unexpected item types";
        return IIdentityCredentialStore::STATUS_INVALID_DATA;
    }

    docType_ = docTypeItem->value();
    testCredential_ = testCredentialItem->value();

    vector<uint8_t> hardwareBoundKey;
    if (testCredential_) {
        hardwareBoundKey = support::getTestHardwareBoundKey();
    } else {
        hardwareBoundKey = getHardwareBoundKey();
    }

    const vector<uint8_t>& encryptedCredentialKeys = encryptedCredentialKeysItem->value();
    const vector<uint8_t> docTypeVec(docType_.begin(), docType_.end());
    optional<vector<uint8_t>> decryptedCredentialKeys =
            support::decryptAes128Gcm(hardwareBoundKey, encryptedCredentialKeys, docTypeVec);
    if (!decryptedCredentialKeys) {
        LOG(ERROR) << "Error decrypting CredentialKeys";
        return IIdentityCredentialStore::STATUS_INVALID_DATA;
    }

    auto [dckItem, dckPos, dckMessage] = cppbor::parse(decryptedCredentialKeys.value());
    if (dckItem == nullptr) {
        LOG(ERROR) << "Decrypted CredentialKeys is not valid CBOR: " << dckMessage;
        return IIdentityCredentialStore::STATUS_INVALID_DATA;
    }
    const cppbor::Array* dckArrayItem = dckItem->asArray();
    if (dckArrayItem == nullptr || dckArrayItem->size() != 2) {
        LOG(ERROR) << "Decrypted CredentialKeys is not an array with two elements";
        return IIdentityCredentialStore::STATUS_INVALID_DATA;
    }
    const cppbor::Bstr* storageKeyItem = (*dckArrayItem)[0]->asBstr();
    const cppbor::Bstr* credentialPrivKeyItem = (*dckArrayItem)[1]->asBstr();
    if (storageKeyItem == nullptr || credentialPrivKeyItem == nullptr) {
        LOG(ERROR) << "CredentialKeys unexpected item types";
        return IIdentityCredentialStore::STATUS_INVALID_DATA;
    }
    storageKey_ = storageKeyItem->value();
    credentialPrivKey_ = credentialPrivKeyItem->value();

    return IIdentityCredentialStore::STATUS_OK;
}

ndk::ScopedAStatus IdentityCredential::deleteCredential(
        vector<int8_t>* outProofOfDeletionSignature) {
    cppbor::Array array = {"ProofOfDeletion", docType_, testCredential_};
    vector<uint8_t> proofOfDeletion = array.encode();

    optional<vector<uint8_t>> signature = support::coseSignEcDsa(credentialPrivKey_,
                                                                 proofOfDeletion,  // payload
                                                                 {},               // additionalData
                                                                 {});  // certificateChain
    if (!signature) {
        return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                IIdentityCredentialStore::STATUS_FAILED, "Error signing data"));
    }

    *outProofOfDeletionSignature = byteStringToSigned(signature.value());
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus IdentityCredential::createEphemeralKeyPair(vector<int8_t>* outKeyPair) {
    optional<vector<uint8_t>> kp = support::createEcKeyPair();
    if (!kp) {
        return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                IIdentityCredentialStore::STATUS_FAILED, "Error creating ephemeral key pair"));
    }

    // Stash public key of this key-pair for later check in startRetrieval().
    optional<vector<uint8_t>> publicKey = support::ecKeyPairGetPublicKey(kp.value());
    if (!publicKey) {
        return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                IIdentityCredentialStore::STATUS_FAILED,
                "Error getting public part of ephemeral key pair"));
    }
    ephemeralPublicKey_ = publicKey.value();

    *outKeyPair = byteStringToSigned(kp.value());
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus IdentityCredential::setReaderEphemeralPublicKey(
        const vector<int8_t>& publicKey) {
    readerPublicKey_ = byteStringToUnsigned(publicKey);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus IdentityCredential::createAuthChallenge(int64_t* outChallenge) {
    uint64_t challenge = 0;
    while (challenge == 0) {
        optional<vector<uint8_t>> bytes = support::getRandom(8);
        if (!bytes) {
            return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                    IIdentityCredentialStore::STATUS_FAILED,
                    "Error getting random data for challenge"));
        }

        challenge = 0;
        for (size_t n = 0; n < bytes.value().size(); n++) {
            challenge |= ((bytes.value())[n] << (n * 8));
        }
    }

    *outChallenge = challenge;
    return ndk::ScopedAStatus::ok();
}

// TODO: this could be a lot faster if we did all the splitting and pubkey extraction
// ahead of time.
bool checkReaderAuthentication(const SecureAccessControlProfile& profile,
                               const vector<uint8_t>& readerCertificateChain) {
    optional<vector<uint8_t>> acpPubKey = support::certificateChainGetTopMostKey(
            byteStringToUnsigned(profile.readerCertificate.encodedCertificate));
    if (!acpPubKey) {
        LOG(ERROR) << "Error extracting public key from readerCertificate in profile";
        return false;
    }

    optional<vector<vector<uint8_t>>> certificatesInChain =
            support::certificateChainSplit(readerCertificateChain);
    if (!certificatesInChain) {
        LOG(ERROR) << "Error splitting readerCertificateChain";
        return false;
    }
    for (const vector<uint8_t>& certInChain : certificatesInChain.value()) {
        optional<vector<uint8_t>> certPubKey = support::certificateChainGetTopMostKey(certInChain);
        if (!certPubKey) {
            LOG(ERROR)
                    << "Error extracting public key from certificate in chain presented by reader";
            return false;
        }
        if (acpPubKey.value() == certPubKey.value()) {
            return true;
        }
    }
    return false;
}

Timestamp clockGetTime() {
    struct timespec time;
    clock_gettime(CLOCK_MONOTONIC, &time);
    Timestamp ts;
    ts.milliSeconds = time.tv_sec * 1000 + time.tv_nsec / 1000000;
    return ts;
}

bool checkUserAuthentication(const SecureAccessControlProfile& profile,
                             const HardwareAuthToken& authToken, uint64_t authChallenge) {
    if (profile.secureUserId != authToken.userId) {
        LOG(ERROR) << "secureUserId in profile (" << profile.secureUserId
                   << ") differs from userId in authToken (" << authToken.userId << ")";
        return false;
    }

    if (profile.timeoutMillis == 0) {
        if (authToken.challenge == 0) {
            LOG(ERROR) << "No challenge in authToken";
            return false;
        }

        if (authToken.challenge != int64_t(authChallenge)) {
            LOG(ERROR) << "Challenge in authToken doesn't match the challenge we created";
            return false;
        }
        return true;
    }

    // Note that the Epoch for timestamps in HardwareAuthToken is at the
    // discretion of the vendor:
    //
    //   "[...] since some starting point (generally the most recent device
    //    boot) which all of the applications within one secure environment
    //    must agree upon."
    //
    // Therefore, if this software implementation is used on a device which isn't
    // the emulator then the assumption that the epoch is the same as used in
    // clockGetTime above will not hold. This is OK as this software
    // implementation should never be used on a real device.
    //
    Timestamp now = clockGetTime();
    if (authToken.timestamp.milliSeconds > now.milliSeconds) {
        LOG(ERROR) << "Timestamp in authToken (" << authToken.timestamp.milliSeconds
                   << ") is in the future (now: " << now.milliSeconds << ")";
        return false;
    }
    if (now.milliSeconds > authToken.timestamp.milliSeconds + profile.timeoutMillis) {
        LOG(ERROR) << "Deadline for authToken (" << authToken.timestamp.milliSeconds << " + "
                   << profile.timeoutMillis << " = "
                   << (authToken.timestamp.milliSeconds + profile.timeoutMillis)
                   << ") is in the past (now: " << now.milliSeconds << ")";
        return false;
    }
    return true;
}

ndk::ScopedAStatus IdentityCredential::startRetrieval(
        const vector<SecureAccessControlProfile>& accessControlProfiles,
        const HardwareAuthToken& authToken, const vector<int8_t>& itemsRequestS,
        const vector<int8_t>& sessionTranscriptS, const vector<int8_t>& readerSignatureS,
        const vector<int32_t>& requestCounts) {
    auto sessionTranscript = byteStringToUnsigned(sessionTranscriptS);
    auto itemsRequest = byteStringToUnsigned(itemsRequestS);
    auto readerSignature = byteStringToUnsigned(readerSignatureS);

    if (sessionTranscript.size() > 0) {
        auto [item, _, message] = cppbor::parse(sessionTranscript);
        if (item == nullptr) {
            return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                    IIdentityCredentialStore::STATUS_INVALID_DATA,
                    "SessionTranscript contains invalid CBOR"));
        }
        sessionTranscriptItem_ = std::move(item);
    }
    if (numStartRetrievalCalls_ > 0) {
        if (sessionTranscript_ != sessionTranscript) {
            return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                    IIdentityCredentialStore::STATUS_SESSION_TRANSCRIPT_MISMATCH,
                    "Passed-in SessionTranscript doesn't match previously used SessionTranscript"));
        }
    }
    sessionTranscript_ = sessionTranscript;

    // If there is a signature, validate that it was made with the top-most key in the
    // certificate chain embedded in the COSE_Sign1 structure.
    optional<vector<uint8_t>> readerCertificateChain;
    if (readerSignature.size() > 0) {
        readerCertificateChain = support::coseSignGetX5Chain(readerSignature);
        if (!readerCertificateChain) {
            return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                    IIdentityCredentialStore::STATUS_READER_SIGNATURE_CHECK_FAILED,
                    "Unable to get reader certificate chain from COSE_Sign1"));
        }

        if (!support::certificateChainValidate(readerCertificateChain.value())) {
            return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                    IIdentityCredentialStore::STATUS_READER_SIGNATURE_CHECK_FAILED,
                    "Error validating reader certificate chain"));
        }

        optional<vector<uint8_t>> readerPublicKey =
                support::certificateChainGetTopMostKey(readerCertificateChain.value());
        if (!readerPublicKey) {
            return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                    IIdentityCredentialStore::STATUS_READER_SIGNATURE_CHECK_FAILED,
                    "Unable to get public key from reader certificate chain"));
        }

        const vector<uint8_t>& itemsRequestBytes = itemsRequest;
        vector<uint8_t> dataThatWasSigned = cppbor::Array()
                                                    .add("ReaderAuthentication")
                                                    .add(sessionTranscriptItem_->clone())
                                                    .add(cppbor::Semantic(24, itemsRequestBytes))
                                                    .encode();
        if (!support::coseCheckEcDsaSignature(readerSignature,
                                              dataThatWasSigned,  // detached content
                                              readerPublicKey.value())) {
            return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                    IIdentityCredentialStore::STATUS_READER_SIGNATURE_CHECK_FAILED,
                    "readerSignature check failed"));
        }
    }

    // Here's where we would validate the passed-in |authToken| to assure ourselves
    // that it comes from the e.g. biometric hardware and wasn't made up by an attacker.
    //
    // However this involves calculating the MAC. However this requires access
    // to the key needed to a pre-shared key which we don't have...
    //

    // To prevent replay-attacks, we check that the public part of the ephemeral
    // key we previously created, is present in the DeviceEngagement part of
    // SessionTranscript as a COSE_Key, in uncompressed form.
    //
    // We do this by just searching for the X and Y coordinates.
    if (sessionTranscript.size() > 0) {
        const cppbor::Array* array = sessionTranscriptItem_->asArray();
        if (array == nullptr || array->size() != 2) {
            return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                    IIdentityCredentialStore::STATUS_EPHEMERAL_PUBLIC_KEY_NOT_FOUND,
                    "SessionTranscript is not an array with two items"));
        }
        const cppbor::Semantic* taggedEncodedDE = (*array)[0]->asSemantic();
        if (taggedEncodedDE == nullptr || taggedEncodedDE->value() != 24) {
            return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                    IIdentityCredentialStore::STATUS_EPHEMERAL_PUBLIC_KEY_NOT_FOUND,
                    "First item in SessionTranscript array is not a "
                    "semantic with value 24"));
        }
        const cppbor::Bstr* encodedDE = (taggedEncodedDE->child())->asBstr();
        if (encodedDE == nullptr) {
            return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                    IIdentityCredentialStore::STATUS_EPHEMERAL_PUBLIC_KEY_NOT_FOUND,
                    "Child of semantic in first item in SessionTranscript "
                    "array is not a bstr"));
        }
        const vector<uint8_t>& bytesDE = encodedDE->value();

        auto [getXYSuccess, ePubX, ePubY] = support::ecPublicKeyGetXandY(ephemeralPublicKey_);
        if (!getXYSuccess) {
            return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                    IIdentityCredentialStore::STATUS_EPHEMERAL_PUBLIC_KEY_NOT_FOUND,
                    "Error extracting X and Y from ePub"));
        }
        if (sessionTranscript.size() > 0 &&
            !(memmem(bytesDE.data(), bytesDE.size(), ePubX.data(), ePubX.size()) != nullptr &&
              memmem(bytesDE.data(), bytesDE.size(), ePubY.data(), ePubY.size()) != nullptr)) {
            return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                    IIdentityCredentialStore::STATUS_EPHEMERAL_PUBLIC_KEY_NOT_FOUND,
                    "Did not find ephemeral public key's X and Y coordinates in "
                    "SessionTranscript (make sure leading zeroes are not used)"));
        }
    }

    // itemsRequest: If non-empty, contains request data that may be signed by the
    // reader.  The content can be defined in the way appropriate for the
    // credential, but there are three requirements that must be met to work with
    // this HAL:
    if (itemsRequest.size() > 0) {
        // 1. The content must be a CBOR-encoded structure.
        auto [item, _, message] = cppbor::parse(itemsRequest);
        if (item == nullptr) {
            return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                    IIdentityCredentialStore::STATUS_INVALID_ITEMS_REQUEST_MESSAGE,
                    "Error decoding CBOR in itemsRequest"));
        }

        // 2. The CBOR structure must be a map.
        const cppbor::Map* map = item->asMap();
        if (map == nullptr) {
            return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                    IIdentityCredentialStore::STATUS_INVALID_ITEMS_REQUEST_MESSAGE,
                    "itemsRequest is not a CBOR map"));
        }

        // 3. The map must contain a key "nameSpaces" whose value contains a map, as described in
        //    the example below.
        //
        //   NameSpaces = {
        //     + NameSpace => DataElements ; Requested data elements for each NameSpace
        //   }
        //
        //   NameSpace = tstr
        //
        //   DataElements = {
        //     + DataElement => IntentToRetain
        //   }
        //
        //   DataElement = tstr
        //   IntentToRetain = bool
        //
        // Here's an example of an |itemsRequest| CBOR value satisfying above requirements 1.
        // through 3.:
        //
        //    {
        //        'docType' : 'org.iso.18013-5.2019',
        //        'nameSpaces' : {
        //            'org.iso.18013-5.2019' : {
        //                'Last name' : false,
        //                'Birth date' : false,
        //                'First name' : false,
        //                'Home address' : true
        //            },
        //            'org.aamva.iso.18013-5.2019' : {
        //                'Real Id' : false
        //            }
        //        }
        //    }
        //
        const cppbor::Map* nsMap = nullptr;
        for (size_t n = 0; n < map->size(); n++) {
            const auto& [keyItem, valueItem] = (*map)[n];
            if (keyItem->type() == cppbor::TSTR && keyItem->asTstr()->value() == "nameSpaces" &&
                valueItem->type() == cppbor::MAP) {
                nsMap = valueItem->asMap();
                break;
            }
        }
        if (nsMap == nullptr) {
            return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                    IIdentityCredentialStore::STATUS_INVALID_ITEMS_REQUEST_MESSAGE,
                    "No nameSpaces map in top-most map"));
        }

        for (size_t n = 0; n < nsMap->size(); n++) {
            auto [nsKeyItem, nsValueItem] = (*nsMap)[n];
            const cppbor::Tstr* nsKey = nsKeyItem->asTstr();
            const cppbor::Map* nsInnerMap = nsValueItem->asMap();
            if (nsKey == nullptr || nsInnerMap == nullptr) {
                return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                        IIdentityCredentialStore::STATUS_INVALID_ITEMS_REQUEST_MESSAGE,
                        "Type mismatch in nameSpaces map"));
            }
            string requestedNamespace = nsKey->value();
            vector<string> requestedKeys;
            for (size_t m = 0; m < nsInnerMap->size(); m++) {
                const auto& [innerMapKeyItem, innerMapValueItem] = (*nsInnerMap)[m];
                const cppbor::Tstr* nameItem = innerMapKeyItem->asTstr();
                const cppbor::Simple* simple = innerMapValueItem->asSimple();
                const cppbor::Bool* intentToRetainItem =
                        (simple != nullptr) ? simple->asBool() : nullptr;
                if (nameItem == nullptr || intentToRetainItem == nullptr) {
                    return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                            IIdentityCredentialStore::STATUS_INVALID_ITEMS_REQUEST_MESSAGE,
                            "Type mismatch in value in nameSpaces map"));
                }
                requestedKeys.push_back(nameItem->value());
            }
            requestedNameSpacesAndNames_[requestedNamespace] = requestedKeys;
        }
    }

    // Finally, validate all the access control profiles in the requestData.
    bool haveAuthToken = (authToken.mac.size() > 0);
    for (const auto& profile : accessControlProfiles) {
        if (!secureAccessControlProfileCheckMac(profile, storageKey_)) {
            return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                    IIdentityCredentialStore::STATUS_INVALID_DATA,
                    "Error checking MAC for profile"));
        }
        int accessControlCheck = IIdentityCredentialStore::STATUS_OK;
        if (profile.userAuthenticationRequired) {
            if (!haveAuthToken || !checkUserAuthentication(profile, authToken, authChallenge_)) {
                accessControlCheck = IIdentityCredentialStore::STATUS_USER_AUTHENTICATION_FAILED;
            }
        } else if (profile.readerCertificate.encodedCertificate.size() > 0) {
            if (!readerCertificateChain ||
                !checkReaderAuthentication(profile, readerCertificateChain.value())) {
                accessControlCheck = IIdentityCredentialStore::STATUS_READER_AUTHENTICATION_FAILED;
            }
        }
        profileIdToAccessCheckResult_[profile.id] = accessControlCheck;
    }

    deviceNameSpacesMap_ = cppbor::Map();
    currentNameSpaceDeviceNameSpacesMap_ = cppbor::Map();

    requestCountsRemaining_ = requestCounts;
    currentNameSpace_ = "";

    itemsRequest_ = itemsRequest;

    numStartRetrievalCalls_ += 1;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus IdentityCredential::startRetrieveEntryValue(
        const string& nameSpace, const string& name, int32_t entrySize,
        const vector<int32_t>& accessControlProfileIds) {
    if (name.empty()) {
        return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                IIdentityCredentialStore::STATUS_INVALID_DATA, "Name cannot be empty"));
    }
    if (nameSpace.empty()) {
        return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                IIdentityCredentialStore::STATUS_INVALID_DATA, "Name space cannot be empty"));
    }

    if (requestCountsRemaining_.size() == 0) {
        return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                IIdentityCredentialStore::STATUS_INVALID_DATA,
                "No more name spaces left to go through"));
    }

    if (currentNameSpace_ == "") {
        // First call.
        currentNameSpace_ = nameSpace;
    }

    if (nameSpace == currentNameSpace_) {
        // Same namespace.
        if (requestCountsRemaining_[0] == 0) {
            return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                    IIdentityCredentialStore::STATUS_INVALID_DATA,
                    "No more entries to be retrieved in current name space"));
        }
        requestCountsRemaining_[0] -= 1;
    } else {
        // New namespace.
        if (requestCountsRemaining_[0] != 0) {
            return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                    IIdentityCredentialStore::STATUS_INVALID_DATA,
                    "Moved to new name space but one or more entries need to be retrieved "
                    "in current name space"));
        }
        if (currentNameSpaceDeviceNameSpacesMap_.size() > 0) {
            deviceNameSpacesMap_.add(currentNameSpace_,
                                     std::move(currentNameSpaceDeviceNameSpacesMap_));
        }
        currentNameSpaceDeviceNameSpacesMap_ = cppbor::Map();

        requestCountsRemaining_.erase(requestCountsRemaining_.begin());
        currentNameSpace_ = nameSpace;
    }

    // It's permissible to have an empty itemsRequest... but if non-empty you can
    // only request what was specified in said itemsRequest. Enforce that.
    if (itemsRequest_.size() > 0) {
        const auto& it = requestedNameSpacesAndNames_.find(nameSpace);
        if (it == requestedNameSpacesAndNames_.end()) {
            return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                    IIdentityCredentialStore::STATUS_NOT_IN_REQUEST_MESSAGE,
                    "Name space was not requested in startRetrieval"));
        }
        const auto& dataItemNames = it->second;
        if (std::find(dataItemNames.begin(), dataItemNames.end(), name) == dataItemNames.end()) {
            return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                    IIdentityCredentialStore::STATUS_NOT_IN_REQUEST_MESSAGE,
                    "Data item name in name space was not requested in startRetrieval"));
        }
    }

    // Enforce access control.
    //
    // Access is granted if at least one of the profiles grants access.
    //
    // If an item is configured without any profiles, access is denied.
    //
    int accessControl = IIdentityCredentialStore::STATUS_NO_ACCESS_CONTROL_PROFILES;
    for (auto id : accessControlProfileIds) {
        auto search = profileIdToAccessCheckResult_.find(id);
        if (search == profileIdToAccessCheckResult_.end()) {
            return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                    IIdentityCredentialStore::STATUS_INVALID_DATA,
                    "Requested entry with unvalidated profile id"));
        }
        int accessControlForProfile = search->second;
        if (accessControlForProfile == IIdentityCredentialStore::STATUS_OK) {
            accessControl = IIdentityCredentialStore::STATUS_OK;
            break;
        }
        accessControl = accessControlForProfile;
    }
    if (accessControl != IIdentityCredentialStore::STATUS_OK) {
        return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                int(accessControl), "Access control check failed"));
    }

    entryAdditionalData_ = entryCreateAdditionalData(nameSpace, name, accessControlProfileIds);

    currentName_ = name;
    entryRemainingBytes_ = entrySize;
    entryValue_.resize(0);

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus IdentityCredential::retrieveEntryValue(const vector<int8_t>& encryptedContentS,
                                                          vector<int8_t>* outContent) {
    auto encryptedContent = byteStringToUnsigned(encryptedContentS);

    optional<vector<uint8_t>> content =
            support::decryptAes128Gcm(storageKey_, encryptedContent, entryAdditionalData_);
    if (!content) {
        return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                IIdentityCredentialStore::STATUS_INVALID_DATA, "Error decrypting data"));
    }

    size_t chunkSize = content.value().size();

    if (chunkSize > entryRemainingBytes_) {
        LOG(ERROR) << "Retrieved chunk of size " << chunkSize
                   << " is bigger than remaining space of size " << entryRemainingBytes_;
        return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                IIdentityCredentialStore::STATUS_INVALID_DATA,
                "Retrieved chunk is bigger than remaining space"));
    }

    entryRemainingBytes_ -= chunkSize;
    if (entryRemainingBytes_ > 0) {
        if (chunkSize != IdentityCredentialStore::kGcmChunkSize) {
            return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                    IIdentityCredentialStore::STATUS_INVALID_DATA,
                    "Retrieved non-final chunk of size which isn't kGcmChunkSize"));
        }
    }

    entryValue_.insert(entryValue_.end(), content.value().begin(), content.value().end());

    if (entryRemainingBytes_ == 0) {
        auto [entryValueItem, _, message] = cppbor::parse(entryValue_);
        if (entryValueItem == nullptr) {
            return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                    IIdentityCredentialStore::STATUS_INVALID_DATA,
                    "Retrieved data which is invalid CBOR"));
        }
        currentNameSpaceDeviceNameSpacesMap_.add(currentName_, std::move(entryValueItem));
    }

    *outContent = byteStringToSigned(content.value());
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus IdentityCredential::finishRetrieval(const vector<int8_t>& signingKeyBlobS,
                                                       vector<int8_t>* outMac,
                                                       vector<int8_t>* outDeviceNameSpaces) {
    auto signingKeyBlob = byteStringToUnsigned(signingKeyBlobS);

    if (currentNameSpaceDeviceNameSpacesMap_.size() > 0) {
        deviceNameSpacesMap_.add(currentNameSpace_,
                                 std::move(currentNameSpaceDeviceNameSpacesMap_));
    }
    vector<uint8_t> encodedDeviceNameSpaces = deviceNameSpacesMap_.encode();

    // If there's no signing key or no sessionTranscript or no reader ephemeral
    // public key, we return the empty MAC.
    optional<vector<uint8_t>> mac;
    if (signingKeyBlob.size() > 0 && sessionTranscript_.size() > 0 && readerPublicKey_.size() > 0) {
        cppbor::Array array;
        array.add("DeviceAuthentication");
        array.add(sessionTranscriptItem_->clone());
        array.add(docType_);
        array.add(cppbor::Semantic(24, encodedDeviceNameSpaces));
        vector<uint8_t> encodedDeviceAuthentication = array.encode();

        vector<uint8_t> docTypeAsBlob(docType_.begin(), docType_.end());
        optional<vector<uint8_t>> signingKey =
                support::decryptAes128Gcm(storageKey_, signingKeyBlob, docTypeAsBlob);
        if (!signingKey) {
            return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                    IIdentityCredentialStore::STATUS_INVALID_DATA,
                    "Error decrypting signingKeyBlob"));
        }

        optional<vector<uint8_t>> sharedSecret =
                support::ecdh(readerPublicKey_, signingKey.value());
        if (!sharedSecret) {
            return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                    IIdentityCredentialStore::STATUS_FAILED, "Error doing ECDH"));
        }

        vector<uint8_t> salt = {0x00};
        vector<uint8_t> info = {};
        optional<vector<uint8_t>> derivedKey = support::hkdf(sharedSecret.value(), salt, info, 32);
        if (!derivedKey) {
            return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                    IIdentityCredentialStore::STATUS_FAILED,
                    "Error deriving key from shared secret"));
        }

        mac = support::coseMac0(derivedKey.value(), {},        // payload
                                encodedDeviceAuthentication);  // additionalData
        if (!mac) {
            return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                    IIdentityCredentialStore::STATUS_FAILED, "Error MACing data"));
        }
    }

    *outMac = byteStringToSigned(mac.value_or(vector<uint8_t>({})));
    *outDeviceNameSpaces = byteStringToSigned(encodedDeviceNameSpaces);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus IdentityCredential::generateSigningKeyPair(
        vector<int8_t>* outSigningKeyBlob, Certificate* outSigningKeyCertificate) {
    string serialDecimal = "0";  // TODO: set serial to something unique
    string issuer = "Android Open Source Project";
    string subject = "Android IdentityCredential Reference Implementation";
    time_t validityNotBefore = time(nullptr);
    time_t validityNotAfter = validityNotBefore + 365 * 24 * 3600;

    optional<vector<uint8_t>> signingKeyPKCS8 = support::createEcKeyPair();
    if (!signingKeyPKCS8) {
        return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                IIdentityCredentialStore::STATUS_FAILED, "Error creating signingKey"));
    }

    optional<vector<uint8_t>> signingPublicKey =
            support::ecKeyPairGetPublicKey(signingKeyPKCS8.value());
    if (!signingPublicKey) {
        return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                IIdentityCredentialStore::STATUS_FAILED,
                "Error getting public part of signingKey"));
    }

    optional<vector<uint8_t>> signingKey = support::ecKeyPairGetPrivateKey(signingKeyPKCS8.value());
    if (!signingKey) {
        return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                IIdentityCredentialStore::STATUS_FAILED,
                "Error getting private part of signingKey"));
    }

    optional<vector<uint8_t>> certificate = support::ecPublicKeyGenerateCertificate(
            signingPublicKey.value(), credentialPrivKey_, serialDecimal, issuer, subject,
            validityNotBefore, validityNotAfter);
    if (!certificate) {
        return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                IIdentityCredentialStore::STATUS_FAILED, "Error creating signingKey"));
    }

    optional<vector<uint8_t>> nonce = support::getRandom(12);
    if (!nonce) {
        return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                IIdentityCredentialStore::STATUS_FAILED, "Error getting random"));
    }
    vector<uint8_t> docTypeAsBlob(docType_.begin(), docType_.end());
    optional<vector<uint8_t>> encryptedSigningKey = support::encryptAes128Gcm(
            storageKey_, nonce.value(), signingKey.value(), docTypeAsBlob);
    if (!encryptedSigningKey) {
        return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                IIdentityCredentialStore::STATUS_FAILED, "Error encrypting signingKey"));
    }
    *outSigningKeyBlob = byteStringToSigned(encryptedSigningKey.value());
    *outSigningKeyCertificate = Certificate();
    outSigningKeyCertificate->encodedCertificate = byteStringToSigned(certificate.value());
    return ndk::ScopedAStatus::ok();
}

}  // namespace aidl::android::hardware::identity
