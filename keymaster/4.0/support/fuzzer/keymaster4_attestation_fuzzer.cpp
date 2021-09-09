/*
 * Copyright (C) 2021 The Android Open Source Project
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
#include <android/hardware/keymaster/4.0/IKeymasterDevice.h>
#include <keymasterV4_0/attestation_record.h>
#include <keymasterV4_0/openssl_utils.h>
#include "keymaster4_common.h"

namespace android::hardware::keymaster::V4_0::fuzzer {

constexpr size_t kMinBytes = 1;
constexpr size_t kMaxBytes = 10;

class KeyMaster4AttestationFuzzer {
  public:
    void process(const uint8_t* data, size_t size);

  private:
    ErrorCode generateKey(const AuthorizationSet& keyDesc, hidl_vec<uint8_t>* keyBlob,
                          KeyCharacteristics* keyCharacteristics);
    ErrorCode attestKey(hidl_vec<uint8_t>& keyBlob, const AuthorizationSet& attestParams,
                        hidl_vec<hidl_vec<uint8_t>>* certificateChain);
    X509_Ptr parseCertificateBlob(const hidl_vec<uint8_t>& blob);
    ASN1_OCTET_STRING* getAttestationRecord(const X509* certificate);
    bool verifyAttestationRecord(const hidl_vec<uint8_t>& attestationCert);
    void invokeAttestationRecord();

    sp<IKeymasterDevice> mKeymaster = nullptr;
    std::unique_ptr<FuzzedDataProvider> mFdp = nullptr;
};

ErrorCode KeyMaster4AttestationFuzzer::generateKey(const AuthorizationSet& key_desc,
                                                   hidl_vec<uint8_t>* keyBlob,
                                                   KeyCharacteristics* keyCharacteristics) {
    ErrorCode error;
    mKeymaster->generateKey(key_desc.hidl_data(),
                            [&](ErrorCode hidlError, const hidl_vec<uint8_t>& hidlKeyBlob,
                                const KeyCharacteristics& hidlKeyCharacteristics) {
                                error = hidlError;
                                *keyBlob = hidlKeyBlob;
                                *keyCharacteristics = hidlKeyCharacteristics;
                            });
    return error;
}

ErrorCode KeyMaster4AttestationFuzzer::attestKey(hidl_vec<uint8_t>& keyBlob,
                                                 const AuthorizationSet& attestParams,
                                                 hidl_vec<hidl_vec<uint8_t>>* certificateChain) {
    ErrorCode error;
    auto rc = mKeymaster->attestKey(
            keyBlob, attestParams.hidl_data(),
            [&](ErrorCode hidlError, const hidl_vec<hidl_vec<uint8_t>>& hidlCertificateChain) {
                error = hidlError;
                *certificateChain = hidlCertificateChain;
            });

    if (!rc.isOk()) {
        return ErrorCode::UNKNOWN_ERROR;
    }
    return error;
}

X509_Ptr KeyMaster4AttestationFuzzer::parseCertificateBlob(const hidl_vec<uint8_t>& blob) {
    const uint8_t* p = blob.data();
    return X509_Ptr(d2i_X509(nullptr, &p, blob.size()));
}

/**
 * @brief getAttestationRecord() accepts a 'certificate' pointer and the return value points to the
 * data owned by 'certificate'. Hence, 'certificate' should not be freed and the return value cannot
 * outlive 'certificate'
 */
ASN1_OCTET_STRING* KeyMaster4AttestationFuzzer::getAttestationRecord(const X509* certificate) {
    ASN1_OBJECT_Ptr oid(OBJ_txt2obj(kAttestionRecordOid, 1 /* dotted string format */));
    if (!oid.get()) {
        return nullptr;
    }

    int location = X509_get_ext_by_OBJ(certificate, oid.get(), -1 /* search from beginning */);
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

bool KeyMaster4AttestationFuzzer::verifyAttestationRecord(
        const hidl_vec<uint8_t>& attestationCert) {
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
    uint32_t attestationVersion;
    uint32_t keymasterVersion;
    SecurityLevel securityLevel;
    SecurityLevel keymasterSecurityLevel;
    hidl_vec<uint8_t> attestationChallenge;
    hidl_vec<uint8_t> attestationUniqueId;

    auto error = parse_attestation_record(
            attestRecord->data, attestRecord->length, &attestationVersion, &securityLevel,
            &keymasterVersion, &keymasterSecurityLevel, &attestationChallenge,
            &attestationSwEnforced, &attestationHwEnforced, &attestationUniqueId);
    if (error != ErrorCode::OK) {
        return false;
    }

    hidl_vec<uint8_t> verifiedBootKey;
    keymaster_verified_boot_t verifiedBootState;
    bool device_locked;
    hidl_vec<uint8_t> verifiedBootHash;

    parse_root_of_trust(attestRecord->data, attestRecord->length, &verifiedBootKey,
                        &verifiedBootState, &device_locked, &verifiedBootHash);
    return true;
}

void KeyMaster4AttestationFuzzer::invokeAttestationRecord() {
    mKeymaster = IKeymasterDevice::getService();
    if (!mKeymaster) {
        return;
    }

    hidl_vec<uint8_t> keyBlob;
    KeyCharacteristics keyCharacteristics;
    generateKey(createAuthorizationSet(mFdp), &keyBlob, &keyCharacteristics);

    hidl_vec<hidl_vec<uint8_t>> certificateChain;

    std::vector<uint8_t> challenge, attestationId;
    challenge =
            mFdp->ConsumeBytes<uint8_t>(mFdp->ConsumeIntegralInRange<size_t>(kMinBytes, kMaxBytes));
    attestationId =
            mFdp->ConsumeBytes<uint8_t>(mFdp->ConsumeIntegralInRange<size_t>(kMinBytes, kMaxBytes));
    attestKey(keyBlob,
              AuthorizationSetBuilder()
                      .Authorization(TAG_ATTESTATION_CHALLENGE, challenge)
                      .Authorization(TAG_ATTESTATION_APPLICATION_ID, attestationId),
              &certificateChain);

    if (certificateChain.size() > 0) {
        verifyAttestationRecord(certificateChain[mFdp->ConsumeIntegralInRange<size_t>(
                0, certificateChain.size() - 1)]);
    }
}

void KeyMaster4AttestationFuzzer::process(const uint8_t* data, size_t size) {
    mFdp = std::make_unique<FuzzedDataProvider>(data, size);
    invokeAttestationRecord();
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    KeyMaster4AttestationFuzzer km4AttestationFuzzer;
    km4AttestationFuzzer.process(data, size);
    return 0;
}

}  // namespace android::hardware::keymaster::V4_0::fuzzer
