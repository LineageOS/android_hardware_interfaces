/*
 * Copyright (C) 2024 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <aidl/android/hardware/security/keymint/AttestationKey.h>
#include <aidl/android/hardware/security/keymint/KeyCreationResult.h>
#include <android/binder_manager.h>
#include <keymint_common.h>
#include <keymint_support/attestation_record.h>
#include <keymint_support/openssl_utils.h>
#include <utils/Log.h>

namespace android::hardware::security::keymint_support::fuzzer {
using namespace android;
using AStatus = ::ndk::ScopedAStatus;
std::shared_ptr<IKeyMintDevice> gKeyMint = nullptr;

constexpr size_t kMaxBytes = 256;
const std::string kServiceName = "android.hardware.security.keymint.IKeyMintDevice/default";

class KeyMintAttestationFuzzer {
  public:
    KeyMintAttestationFuzzer(const uint8_t* data, size_t size) : mFdp(data, size){};
    void process();

  private:
    KeyCreationResult generateKey(const AuthorizationSet& keyDesc,
                                  const std::optional<AttestationKey>& attestKey,
                                  vector<uint8_t>* keyBlob,
                                  vector<KeyCharacteristics>* keyCharacteristics,
                                  vector<Certificate>* certChain);
    X509_Ptr parseCertificateBlob(const vector<uint8_t>& blob);
    ASN1_OCTET_STRING* getAttestationRecord(const X509* certificate);
    bool verifyAttestationRecord(const vector<uint8_t>& attestationCert);
    FuzzedDataProvider mFdp;
};

KeyCreationResult KeyMintAttestationFuzzer::generateKey(
        const AuthorizationSet& keyDesc, const std::optional<AttestationKey>& attestKey,
        vector<uint8_t>* keyBlob, vector<KeyCharacteristics>* keyCharacteristics,
        vector<Certificate>* certChain) {
    KeyCreationResult creationResult;
    AStatus result = gKeyMint->generateKey(keyDesc.vector_data(), attestKey, &creationResult);
    if (result.isOk() && creationResult.keyBlob.size() > 0) {
        *keyBlob = std::move(creationResult.keyBlob);
        *keyCharacteristics = std::move(creationResult.keyCharacteristics);
        *certChain = std::move(creationResult.certificateChain);
    }
    return creationResult;
}

X509_Ptr KeyMintAttestationFuzzer::parseCertificateBlob(const vector<uint8_t>& blob) {
    const uint8_t* data = blob.data();
    return X509_Ptr(d2i_X509(nullptr, &data, blob.size()));
}

ASN1_OCTET_STRING* KeyMintAttestationFuzzer::getAttestationRecord(const X509* certificate) {
    ASN1_OBJECT_Ptr oid(OBJ_txt2obj(kAttestionRecordOid, 1 /* dotted string format */));
    if (!oid.get()) {
        return nullptr;
    }

    int32_t location = X509_get_ext_by_OBJ(certificate, oid.get(), -1 /* search from beginning */);
    if (location == -1) {
        return nullptr;
    }

    X509_EXTENSION* attestRecordExt = X509_get_ext(certificate, location);
    if (!attestRecordExt) {
        return nullptr;
    }

    ASN1_OCTET_STRING* attestRecord = X509_EXTENSION_get_data(attestRecordExt);
    return attestRecord;
}

bool KeyMintAttestationFuzzer::verifyAttestationRecord(const vector<uint8_t>& attestationCert) {
    X509_Ptr cert(parseCertificateBlob(attestationCert));
    if (!cert.get()) {
        return false;
    }

    ASN1_OCTET_STRING* attestRecord = getAttestationRecord(cert.get());
    if (!attestRecord) {
        return false;
    }

    AuthorizationSet attestationSwEnforced;
    AuthorizationSet attestationHwEnforced;
    SecurityLevel attestationSecurityLevel;
    SecurityLevel keymintSecurityLevel;
    vector<uint8_t> attestationChallenge;
    vector<uint8_t> attestationUniqueId;
    uint32_t attestationVersion;
    uint32_t keymintVersion;

    auto error = parse_attestation_record(attestRecord->data, attestRecord->length,
                                          &attestationVersion, &attestationSecurityLevel,
                                          &keymintVersion, &keymintSecurityLevel,
                                          &attestationChallenge, &attestationSwEnforced,
                                          &attestationHwEnforced, &attestationUniqueId);
    if (error != ErrorCode::OK) {
        return false;
    }

    VerifiedBoot verifiedBootState;
    vector<uint8_t> verifiedBootKey;
    vector<uint8_t> verifiedBootHash;
    bool device_locked;

    error = parse_root_of_trust(attestRecord->data, attestRecord->length, &verifiedBootKey,
                                &verifiedBootState, &device_locked, &verifiedBootHash);
    if (error != ErrorCode::OK) {
        return false;
    }
    return true;
}

void KeyMintAttestationFuzzer::process() {
    AttestationKey attestKey;
    vector<Certificate> attestKeyCertChain;
    vector<KeyCharacteristics> attestKeyCharacteristics;
    generateKey(createAuthSetForAttestKey(&mFdp), {}, &attestKey.keyBlob, &attestKeyCharacteristics,
                &attestKeyCertChain);

    vector<Certificate> attestedKeyCertChain;
    vector<KeyCharacteristics> attestedKeyCharacteristics;
    vector<uint8_t> attestedKeyBlob;
    attestKey.issuerSubjectName = mFdp.ConsumeBytes<uint8_t>(kMaxBytes);
    generateKey(createAuthorizationSet(&mFdp), attestKey, &attestedKeyBlob,
                &attestedKeyCharacteristics, &attestedKeyCertChain);

    if (attestedKeyCertChain.size() > 0) {
        size_t leafCert = attestedKeyCertChain.size() - 1;
        verifyAttestationRecord(attestedKeyCertChain[leafCert].encodedCertificate);
    }
}

extern "C" int LLVMFuzzerInitialize(int /* *argc */, char /* ***argv */) {
    ::ndk::SpAIBinder binder(AServiceManager_waitForService(kServiceName.c_str()));
    gKeyMint = std::move(IKeyMintDevice::fromBinder(binder));
    LOG_ALWAYS_FATAL_IF(!gKeyMint, "Failed to get IKeyMintDevice instance.");
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    KeyMintAttestationFuzzer kmAttestationFuzzer(data, size);
    kmAttestationFuzzer.process();
    return 0;
}

}  // namespace android::hardware::security::keymint_support::fuzzer
