/*
 * Copyright 2020, The Android Open Source Project
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

#define LOG_TAG "FakeSecureHardwareProxy"

#include "FakeSecureHardwareProxy.h"

#include <android/hardware/identity/support/IdentityCredentialSupport.h>

#include <android-base/logging.h>
#include <android-base/stringprintf.h>
#include <string.h>
#include <map>

#include <openssl/sha.h>

#include <openssl/aes.h>
#include <openssl/bn.h>
#include <openssl/crypto.h>
#include <openssl/ec.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/hkdf.h>
#include <openssl/hmac.h>
#include <openssl/objects.h>
#include <openssl/pem.h>
#include <openssl/pkcs12.h>
#include <openssl/rand.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>

#include <libeic.h>

using ::std::optional;
using ::std::string;
using ::std::tuple;
using ::std::vector;

namespace android::hardware::identity {

// ----------------------------------------------------------------------

// The singleton EicProvisioning object used everywhere.
//
EicProvisioning FakeSecureHardwareProvisioningProxy::ctx_;

FakeSecureHardwareProvisioningProxy::~FakeSecureHardwareProvisioningProxy() {
    if (id_ != 0) {
        shutdown();
    }
}

bool FakeSecureHardwareProvisioningProxy::initialize(bool testCredential) {
    if (id_ != 0) {
        LOG(WARNING) << "Proxy is already initialized";
        return false;
    }
    bool initialized = eicProvisioningInit(&ctx_, testCredential);
    if (!initialized) {
        return false;
    }
    optional<uint32_t> id = getId();
    if (!id) {
        LOG(WARNING) << "Error getting id";
        return false;
    }
    id_ = id.value();
    return true;
}

bool FakeSecureHardwareProvisioningProxy::initializeForUpdate(
        bool testCredential, const string& docType,
        const vector<uint8_t>& encryptedCredentialKeys) {
    if (id_ != 0) {
        LOG(WARNING) << "Proxy is already initialized";
        return false;
    }
    bool initialized = eicProvisioningInitForUpdate(&ctx_, testCredential, docType.c_str(),
                                                    docType.size(), encryptedCredentialKeys.data(),
                                                    encryptedCredentialKeys.size());
    if (!initialized) {
        return false;
    }
    optional<uint32_t> id = getId();
    if (!id) {
        LOG(WARNING) << "Error getting id";
        return false;
    }
    id_ = id.value();
    return true;
}

optional<uint32_t> FakeSecureHardwareProvisioningProxy::getId() {
    uint32_t id;
    if (!eicProvisioningGetId(&ctx_, &id)) {
        return std::nullopt;
    }
    return id;
}

bool FakeSecureHardwareProvisioningProxy::validateId(const string& callerName) {
    if (id_ == 0) {
        LOG(WARNING) << "FakeSecureHardwareProvisioningProxy::" << callerName
                     << ": While validating expected id is 0";
        return false;
    }
    optional<uint32_t> id = getId();
    if (!id) {
        LOG(WARNING) << "FakeSecureHardwareProvisioningProxy::" << callerName
                     << ": Error getting id for validating";
        return false;
    }
    if (id.value() != id_) {
        LOG(WARNING) << "FakeSecureHardwareProvisioningProxy::" << callerName
                     << ": While validating expected id " << id_ << " but got " << id.value();
        return false;
    }
    return true;
}

bool FakeSecureHardwareProvisioningProxy::shutdown() {
    bool validated = validateId(__func__);
    id_ = 0;
    if (!validated) {
        return false;
    }
    if (!eicProvisioningShutdown(&ctx_)) {
        LOG(INFO) << "Error shutting down provisioning";
        return false;
    }
    return true;
}

// Returns public key certificate.
optional<vector<uint8_t>> FakeSecureHardwareProvisioningProxy::createCredentialKey(
        const vector<uint8_t>& challenge, const vector<uint8_t>& applicationId) {
    if (!validateId(__func__)) {
        return std::nullopt;
    }

    uint8_t publicKeyCert[4096];
    size_t publicKeyCertSize = sizeof publicKeyCert;
    if (!eicProvisioningCreateCredentialKey(&ctx_, challenge.data(), challenge.size(),
                                            applicationId.data(), applicationId.size(),
                                            /*attestationKeyBlob=*/nullptr,
                                            /*attestationKeyBlobSize=*/0,
                                            /*attestationKeyCert=*/nullptr,
                                            /*attestationKeyCertSize=*/0, publicKeyCert,
                                            &publicKeyCertSize)) {
        return std::nullopt;
    }
    vector<uint8_t> pubKeyCert(publicKeyCertSize);
    memcpy(pubKeyCert.data(), publicKeyCert, publicKeyCertSize);
    return pubKeyCert;
}

optional<vector<uint8_t>> FakeSecureHardwareProvisioningProxy::createCredentialKeyUsingRkp(
        const vector<uint8_t>& challenge, const vector<uint8_t>& applicationId,
        const vector<uint8_t>& attestationKeyBlob, const vector<uint8_t>& attstationKeyCert) {
    size_t publicKeyCertSize = 4096;
    vector<uint8_t> publicKeyCert(publicKeyCertSize);
    if (!eicProvisioningCreateCredentialKey(&ctx_, challenge.data(), challenge.size(),
                                            applicationId.data(), applicationId.size(),
                                            attestationKeyBlob.data(), attestationKeyBlob.size(),
                                            attstationKeyCert.data(), attstationKeyCert.size(),
                                            publicKeyCert.data(), &publicKeyCertSize)) {
        LOG(ERROR) << "error creating credential key";
        return std::nullopt;
    }
    publicKeyCert.resize(publicKeyCertSize);
    return publicKeyCert;
}

bool FakeSecureHardwareProvisioningProxy::startPersonalization(
        int accessControlProfileCount, const vector<int>& entryCounts, const string& docType,
        size_t expectedProofOfProvisioningSize) {
    if (!validateId(__func__)) {
        return false;
    }

    if (!eicProvisioningStartPersonalization(&ctx_, accessControlProfileCount,
                                             entryCounts.data(),
                                             entryCounts.size(),
                                             docType.c_str(), docType.size(),
                                             expectedProofOfProvisioningSize)) {
        return false;
    }
    return true;
}

// Returns MAC (28 bytes).
optional<vector<uint8_t>> FakeSecureHardwareProvisioningProxy::addAccessControlProfile(
        int id, const vector<uint8_t>& readerCertificate, bool userAuthenticationRequired,
        uint64_t timeoutMillis, uint64_t secureUserId) {
    if (!validateId(__func__)) {
        return std::nullopt;
    }

    vector<uint8_t> mac(28);
    uint8_t scratchSpace[512];
    if (!eicProvisioningAddAccessControlProfile(
                &ctx_, id, readerCertificate.data(), readerCertificate.size(),
                userAuthenticationRequired, timeoutMillis, secureUserId, mac.data(),
                scratchSpace, sizeof(scratchSpace))) {
        return std::nullopt;
    }
    return mac;
}

bool FakeSecureHardwareProvisioningProxy::beginAddEntry(const vector<int>& accessControlProfileIds,
                                                        const string& nameSpace, const string& name,
                                                        uint64_t entrySize) {
    if (!validateId(__func__)) {
        return false;
    }

    uint8_t scratchSpace[512];
    vector<uint8_t> uint8AccessControlProfileIds;
    for (size_t i = 0; i < accessControlProfileIds.size(); i++) {
        uint8AccessControlProfileIds.push_back(accessControlProfileIds[i] & 0xFF);
    }

    return eicProvisioningBeginAddEntry(&ctx_, uint8AccessControlProfileIds.data(),
                                        uint8AccessControlProfileIds.size(), nameSpace.c_str(),
                                        nameSpace.size(), name.c_str(), name.size(), entrySize,
                                        scratchSpace, sizeof(scratchSpace));
}

// Returns encryptedContent.
optional<vector<uint8_t>> FakeSecureHardwareProvisioningProxy::addEntryValue(
        const vector<int>& accessControlProfileIds, const string& nameSpace, const string& name,
        const vector<uint8_t>& content) {
    if (!validateId(__func__)) {
        return std::nullopt;
    }

    vector<uint8_t> eicEncryptedContent;
    uint8_t scratchSpace[512];
    vector<uint8_t> uint8AccessControlProfileIds;
    for (size_t i = 0; i < accessControlProfileIds.size(); i++) {
        uint8AccessControlProfileIds.push_back(accessControlProfileIds[i] & 0xFF);
    }

    eicEncryptedContent.resize(content.size() + 28);
    if (!eicProvisioningAddEntryValue(
                &ctx_, uint8AccessControlProfileIds.data(), uint8AccessControlProfileIds.size(),
                nameSpace.c_str(), nameSpace.size(), name.c_str(), name.size(), content.data(),
                content.size(), eicEncryptedContent.data(), scratchSpace, sizeof(scratchSpace))) {
        return std::nullopt;
    }
    return eicEncryptedContent;
}

// Returns signatureOfToBeSigned (EIC_ECDSA_P256_SIGNATURE_SIZE bytes).
optional<vector<uint8_t>> FakeSecureHardwareProvisioningProxy::finishAddingEntries() {
    if (!validateId(__func__)) {
        return std::nullopt;
    }

    vector<uint8_t> signatureOfToBeSigned(EIC_ECDSA_P256_SIGNATURE_SIZE);
    if (!eicProvisioningFinishAddingEntries(&ctx_, signatureOfToBeSigned.data())) {
        return std::nullopt;
    }
    return signatureOfToBeSigned;
}

// Returns encryptedCredentialKeys.
optional<vector<uint8_t>> FakeSecureHardwareProvisioningProxy::finishGetCredentialData(
        const string& docType) {
    if (!validateId(__func__)) {
        return std::nullopt;
    }

    vector<uint8_t> encryptedCredentialKeys(116);
    size_t size = encryptedCredentialKeys.size();
    if (!eicProvisioningFinishGetCredentialData(&ctx_, docType.c_str(), docType.size(),
                                                encryptedCredentialKeys.data(), &size)) {
        return std::nullopt;
    }
    encryptedCredentialKeys.resize(size);
    return encryptedCredentialKeys;
}

// ----------------------------------------------------------------------

// The singleton EicSession object used everywhere.
//
EicSession FakeSecureHardwareSessionProxy::ctx_;

FakeSecureHardwareSessionProxy::~FakeSecureHardwareSessionProxy() {
    if (id_ != 0) {
        shutdown();
    }
}

bool FakeSecureHardwareSessionProxy::initialize() {
    if (id_ != 0) {
        LOG(WARNING) << "Proxy is already initialized";
        return false;
    }
    bool initialized = eicSessionInit(&ctx_);
    if (!initialized) {
        return false;
    }
    optional<uint32_t> id = getId();
    if (!id) {
        LOG(WARNING) << "Error getting id";
        return false;
    }
    id_ = id.value();
    return true;
}

optional<uint32_t> FakeSecureHardwareSessionProxy::getId() {
    uint32_t id;
    if (!eicSessionGetId(&ctx_, &id)) {
        return std::nullopt;
    }
    return id;
}

bool FakeSecureHardwareSessionProxy::shutdown() {
    bool validated = validateId(__func__);
    id_ = 0;
    if (!validated) {
        return false;
    }
    if (!eicSessionShutdown(&ctx_)) {
        LOG(INFO) << "Error shutting down session";
        return false;
    }
    return true;
}

bool FakeSecureHardwareSessionProxy::validateId(const string& callerName) {
    if (id_ == 0) {
        LOG(WARNING) << "FakeSecureHardwareSessionProxy::" << callerName
                     << ": While validating expected id is 0";
        return false;
    }
    optional<uint32_t> id = getId();
    if (!id) {
        LOG(WARNING) << "FakeSecureHardwareSessionProxy::" << callerName
                     << ": Error getting id for validating";
        return false;
    }
    if (id.value() != id_) {
        LOG(WARNING) << "FakeSecureHardwareSessionProxy::" << callerName
                     << ": While validating expected id " << id_ << " but got " << id.value();
        return false;
    }
    return true;
}

optional<uint64_t> FakeSecureHardwareSessionProxy::getAuthChallenge() {
    if (!validateId(__func__)) {
        return std::nullopt;
    }

    uint64_t authChallenge;
    if (!eicSessionGetAuthChallenge(&ctx_, &authChallenge)) {
        return std::nullopt;
    }
    return authChallenge;
}

optional<vector<uint8_t>> FakeSecureHardwareSessionProxy::getEphemeralKeyPair() {
    if (!validateId(__func__)) {
        return std::nullopt;
    }

    vector<uint8_t> priv(EIC_P256_PRIV_KEY_SIZE);
    if (!eicSessionGetEphemeralKeyPair(&ctx_, priv.data())) {
        return std::nullopt;
    }
    return priv;
}

bool FakeSecureHardwareSessionProxy::setReaderEphemeralPublicKey(
        const vector<uint8_t>& readerEphemeralPublicKey) {
    if (!validateId(__func__)) {
        return false;
    }

    return eicSessionSetReaderEphemeralPublicKey(&ctx_, readerEphemeralPublicKey.data());
}

bool FakeSecureHardwareSessionProxy::setSessionTranscript(
        const vector<uint8_t>& sessionTranscript) {
    if (!validateId(__func__)) {
        return false;
    }

    return eicSessionSetSessionTranscript(&ctx_, sessionTranscript.data(),
                                          sessionTranscript.size());
}

// ----------------------------------------------------------------------

// The singleton EicPresentation object used everywhere.
//
EicPresentation FakeSecureHardwarePresentationProxy::ctx_;

FakeSecureHardwarePresentationProxy::~FakeSecureHardwarePresentationProxy() {
    if (id_ != 0) {
        shutdown();
    }
}

bool FakeSecureHardwarePresentationProxy::initialize(
        uint32_t sessionId, bool testCredential, const string& docType,
        const vector<uint8_t>& encryptedCredentialKeys) {
    if (id_ != 0) {
        LOG(WARNING) << "Proxy is already initialized";
        return false;
    }
    bool initialized =
            eicPresentationInit(&ctx_, sessionId, testCredential, docType.c_str(), docType.size(),
                                encryptedCredentialKeys.data(), encryptedCredentialKeys.size());
    if (!initialized) {
        return false;
    }
    optional<uint32_t> id = getId();
    if (!id) {
        LOG(WARNING) << "Error getting id";
        return false;
    }
    id_ = id.value();
    return true;
}

optional<uint32_t> FakeSecureHardwarePresentationProxy::getId() {
    uint32_t id;
    if (!eicPresentationGetId(&ctx_, &id)) {
        return std::nullopt;
    }
    return id;
}

bool FakeSecureHardwarePresentationProxy::validateId(const string& callerName) {
    if (id_ == 0) {
        LOG(WARNING) << "FakeSecureHardwarePresentationProxy::" << callerName
                     << ": While validating expected id is 0";
        return false;
    }
    optional<uint32_t> id = getId();
    if (!id) {
        LOG(WARNING) << "FakeSecureHardwarePresentationProxy::" << callerName
                     << ": Error getting id for validating";
        return false;
    }
    if (id.value() != id_) {
        LOG(WARNING) << "FakeSecureHardwarePresentationProxy::" << callerName
                     << ": While validating expected id " << id_ << " but got " << id.value();
        return false;
    }
    return true;
}

bool FakeSecureHardwarePresentationProxy::shutdown() {
    bool validated = validateId(__func__);
    id_ = 0;
    if (!validated) {
        return false;
    }
    if (!eicPresentationShutdown(&ctx_)) {
        LOG(INFO) << "Error shutting down presentation";
        return false;
    }
    return true;
}

// Returns publicKeyCert (1st component) and signingKeyBlob (2nd component)
optional<pair<vector<uint8_t>, vector<uint8_t>>>
FakeSecureHardwarePresentationProxy::generateSigningKeyPair(const string& docType, time_t now) {
    if (!validateId(__func__)) {
        return std::nullopt;
    }

    uint8_t publicKeyCert[512];
    size_t publicKeyCertSize = sizeof(publicKeyCert);
    vector<uint8_t> signingKeyBlob(60);

    if (!eicPresentationGenerateSigningKeyPair(&ctx_, docType.c_str(), docType.size(), now,
                                               publicKeyCert, &publicKeyCertSize,
                                               signingKeyBlob.data())) {
        return std::nullopt;
    }

    vector<uint8_t> cert;
    cert.resize(publicKeyCertSize);
    memcpy(cert.data(), publicKeyCert, publicKeyCertSize);

    return std::make_pair(cert, signingKeyBlob);
}

// Returns private key
optional<vector<uint8_t>> FakeSecureHardwarePresentationProxy::createEphemeralKeyPair() {
    if (!validateId(__func__)) {
        return std::nullopt;
    }

    vector<uint8_t> priv(EIC_P256_PRIV_KEY_SIZE);
    if (!eicPresentationCreateEphemeralKeyPair(&ctx_, priv.data())) {
        return std::nullopt;
    }
    return priv;
}

optional<uint64_t> FakeSecureHardwarePresentationProxy::createAuthChallenge() {
    if (!validateId(__func__)) {
        return std::nullopt;
    }

    uint64_t challenge;
    if (!eicPresentationCreateAuthChallenge(&ctx_, &challenge)) {
        return std::nullopt;
    }
    return challenge;
}

bool FakeSecureHardwarePresentationProxy::pushReaderCert(const vector<uint8_t>& certX509) {
    if (!validateId(__func__)) {
        return false;
    }

    return eicPresentationPushReaderCert(&ctx_, certX509.data(), certX509.size());
}

bool FakeSecureHardwarePresentationProxy::validateRequestMessage(
        const vector<uint8_t>& sessionTranscript, const vector<uint8_t>& requestMessage,
        int coseSignAlg, const vector<uint8_t>& readerSignatureOfToBeSigned) {
    if (!validateId(__func__)) {
        return false;
    }

    return eicPresentationValidateRequestMessage(
            &ctx_, sessionTranscript.data(), sessionTranscript.size(), requestMessage.data(),
            requestMessage.size(), coseSignAlg, readerSignatureOfToBeSigned.data(),
            readerSignatureOfToBeSigned.size());
}

bool FakeSecureHardwarePresentationProxy::setAuthToken(
        uint64_t challenge, uint64_t secureUserId, uint64_t authenticatorId,
        int hardwareAuthenticatorType, uint64_t timeStamp, const vector<uint8_t>& mac,
        uint64_t verificationTokenChallenge, uint64_t verificationTokenTimestamp,
        int verificationTokenSecurityLevel, const vector<uint8_t>& verificationTokenMac) {
    if (!validateId(__func__)) {
        return false;
    }

    return eicPresentationSetAuthToken(&ctx_, challenge, secureUserId, authenticatorId,
                                       hardwareAuthenticatorType, timeStamp, mac.data(), mac.size(),
                                       verificationTokenChallenge, verificationTokenTimestamp,
                                       verificationTokenSecurityLevel, verificationTokenMac.data(),
                                       verificationTokenMac.size());
}

optional<bool> FakeSecureHardwarePresentationProxy::validateAccessControlProfile(
        int id, const vector<uint8_t>& readerCertificate, bool userAuthenticationRequired,
        int timeoutMillis, uint64_t secureUserId, const vector<uint8_t>& mac) {
    if (!validateId(__func__)) {
        return std::nullopt;
    }

    bool accessGranted = false;
    uint8_t scratchSpace[512];
    if (!eicPresentationValidateAccessControlProfile(&ctx_, id, readerCertificate.data(),
                                                     readerCertificate.size(),
                                                     userAuthenticationRequired, timeoutMillis,
                                                     secureUserId, mac.data(), &accessGranted,
                                                     scratchSpace, sizeof(scratchSpace))) {
        return std::nullopt;
    }
    return accessGranted;
}

bool FakeSecureHardwarePresentationProxy::startRetrieveEntries() {
    if (!validateId(__func__)) {
        return false;
    }

    return eicPresentationStartRetrieveEntries(&ctx_);
}

bool FakeSecureHardwarePresentationProxy::prepareDeviceAuthentication(
        const vector<uint8_t>& sessionTranscript, const vector<uint8_t>& readerEphemeralPublicKey,
        const vector<uint8_t>& signingKeyBlob, const string& docType,
        unsigned int numNamespacesWithValues, size_t expectedDeviceNamespacesSize) {
    if (!validateId(__func__)) {
        return false;
    }

    if (signingKeyBlob.size() != 60) {
        eicDebug("Unexpected size %zd of signingKeyBlob, expected 60", signingKeyBlob.size());
        return false;
    }
    return eicPresentationPrepareDeviceAuthentication(
            &ctx_, sessionTranscript.data(), sessionTranscript.size(),
            readerEphemeralPublicKey.data(), readerEphemeralPublicKey.size(), signingKeyBlob.data(),
            docType.c_str(), docType.size(), numNamespacesWithValues, expectedDeviceNamespacesSize);
}

AccessCheckResult FakeSecureHardwarePresentationProxy::startRetrieveEntryValue(
        const string& nameSpace, const string& name, unsigned int newNamespaceNumEntries,
        int32_t entrySize, const vector<int32_t>& accessControlProfileIds) {
    if (!validateId(__func__)) {
        return AccessCheckResult::kFailed;
    }

    uint8_t scratchSpace[512];
    vector<uint8_t> uint8AccessControlProfileIds;
    for (size_t i = 0; i < accessControlProfileIds.size(); i++) {
        uint8AccessControlProfileIds.push_back(accessControlProfileIds[i] & 0xFF);
    }

    EicAccessCheckResult result = eicPresentationStartRetrieveEntryValue(
            &ctx_, nameSpace.c_str(), nameSpace.size(), name.c_str(), name.size(),
            newNamespaceNumEntries, entrySize, uint8AccessControlProfileIds.data(),
            uint8AccessControlProfileIds.size(), scratchSpace,
            sizeof(scratchSpace));
    switch (result) {
        case EIC_ACCESS_CHECK_RESULT_OK:
            return AccessCheckResult::kOk;
        case EIC_ACCESS_CHECK_RESULT_NO_ACCESS_CONTROL_PROFILES:
            return AccessCheckResult::kNoAccessControlProfiles;
        case EIC_ACCESS_CHECK_RESULT_FAILED:
            return AccessCheckResult::kFailed;
        case EIC_ACCESS_CHECK_RESULT_USER_AUTHENTICATION_FAILED:
            return AccessCheckResult::kUserAuthenticationFailed;
        case EIC_ACCESS_CHECK_RESULT_READER_AUTHENTICATION_FAILED:
            return AccessCheckResult::kReaderAuthenticationFailed;
    }
    eicDebug("Unknown result with code %d, returning kFailed", (int)result);
    return AccessCheckResult::kFailed;
}

optional<vector<uint8_t>> FakeSecureHardwarePresentationProxy::retrieveEntryValue(
        const vector<uint8_t>& encryptedContent, const string& nameSpace, const string& name,
        const vector<int32_t>& accessControlProfileIds) {
    if (!validateId(__func__)) {
        return std::nullopt;
    }

    uint8_t scratchSpace[512];
    vector<uint8_t> uint8AccessControlProfileIds;
    for (size_t i = 0; i < accessControlProfileIds.size(); i++) {
        uint8AccessControlProfileIds.push_back(accessControlProfileIds[i] & 0xFF);
    }

    vector<uint8_t> content;
    content.resize(encryptedContent.size() - 28);
    if (!eicPresentationRetrieveEntryValue(
                &ctx_, encryptedContent.data(), encryptedContent.size(), content.data(),
                nameSpace.c_str(), nameSpace.size(), name.c_str(), name.size(),
                uint8AccessControlProfileIds.data(), uint8AccessControlProfileIds.size(),
                scratchSpace, sizeof(scratchSpace))) {
        return std::nullopt;
    }
    return content;
}

optional<pair<vector<uint8_t>, vector<uint8_t>>>
FakeSecureHardwarePresentationProxy::finishRetrievalWithSignature() {
    if (!validateId(__func__)) {
        return std::nullopt;
    }

    vector<uint8_t> mac(32);
    size_t macSize = 32;
    vector<uint8_t> ecdsaSignature(EIC_ECDSA_P256_SIGNATURE_SIZE);
    size_t ecdsaSignatureSize = EIC_ECDSA_P256_SIGNATURE_SIZE;
    if (!eicPresentationFinishRetrievalWithSignature(&ctx_, mac.data(), &macSize,
                                                     ecdsaSignature.data(), &ecdsaSignatureSize)) {
        return std::nullopt;
    }
    mac.resize(macSize);
    ecdsaSignature.resize(ecdsaSignatureSize);
    return std::make_pair(mac, ecdsaSignature);
}

optional<vector<uint8_t>> FakeSecureHardwarePresentationProxy::finishRetrieval() {
    if (!validateId(__func__)) {
        return std::nullopt;
    }

    vector<uint8_t> mac(32);
    size_t macSize = 32;
    if (!eicPresentationFinishRetrieval(&ctx_, mac.data(), &macSize)) {
        return std::nullopt;
    }
    mac.resize(macSize);
    return mac;
}

optional<vector<uint8_t>> FakeSecureHardwarePresentationProxy::deleteCredential(
        const string& docType, const vector<uint8_t>& challenge, bool includeChallenge,
        size_t proofOfDeletionCborSize) {
    if (!validateId(__func__)) {
        return std::nullopt;
    }

    vector<uint8_t> signatureOfToBeSigned(EIC_ECDSA_P256_SIGNATURE_SIZE);
    if (!eicPresentationDeleteCredential(&ctx_, docType.c_str(), docType.size(), challenge.data(),
                                         challenge.size(), includeChallenge,
                                         proofOfDeletionCborSize, signatureOfToBeSigned.data())) {
        return std::nullopt;
    }
    return signatureOfToBeSigned;
}

optional<vector<uint8_t>> FakeSecureHardwarePresentationProxy::proveOwnership(
        const string& docType, bool testCredential, const vector<uint8_t>& challenge,
        size_t proofOfOwnershipCborSize) {
    if (!validateId(__func__)) {
        return std::nullopt;
    }

    vector<uint8_t> signatureOfToBeSigned(EIC_ECDSA_P256_SIGNATURE_SIZE);
    if (!eicPresentationProveOwnership(&ctx_, docType.c_str(), docType.size(), testCredential,
                                       challenge.data(), challenge.size(), proofOfOwnershipCborSize,
                                       signatureOfToBeSigned.data())) {
        return std::nullopt;
    }
    return signatureOfToBeSigned;
}

}  // namespace android::hardware::identity
