/*
 * Copyright (C) 2020 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "VtsRemotelyProvisionableComponentTests"

#include <AndroidRemotelyProvisionedComponentDevice.h>
#include <aidl/android/hardware/security/keymint/IRemotelyProvisionedComponent.h>
#include <aidl/android/hardware/security/keymint/SecurityLevel.h>
#include <android/binder_manager.h>
#include <cppbor_parse.h>
#include <gmock/gmock.h>
#include <keymaster/cppcose/cppcose.h>
#include <keymaster/keymaster_configuration.h>
#include <keymint_support/authorization_set.h>
#include <openssl/ec.h>
#include <openssl/ec_key.h>
#include <openssl/x509.h>
#include <remote_prov/remote_prov_utils.h>

#include "KeyMintAidlTestBase.h"

namespace aidl::android::hardware::security::keymint::test {

using ::std::string;
using ::std::vector;

namespace {

#define INSTANTIATE_REM_PROV_AIDL_TEST(name)                                         \
    INSTANTIATE_TEST_SUITE_P(                                                        \
            PerInstance, name,                                                       \
            testing::ValuesIn(VtsRemotelyProvisionedComponentTests::build_params()), \
            ::android::PrintInstanceNameToString)

using bytevec = std::vector<uint8_t>;
using testing::MatchesRegex;
using namespace remote_prov;
using namespace keymaster;

bytevec string_to_bytevec(const char* s) {
    const uint8_t* p = reinterpret_cast<const uint8_t*>(s);
    return bytevec(p, p + strlen(s));
}

ErrMsgOr<MacedPublicKey> corrupt_maced_key(const MacedPublicKey& macedPubKey) {
    auto [coseMac0, _, mac0ParseErr] = cppbor::parse(macedPubKey.macedKey);
    if (!coseMac0 || coseMac0->asArray()->size() != kCoseMac0EntryCount) {
        return "COSE Mac0 parse failed";
    }
    auto protParams = coseMac0->asArray()->get(kCoseMac0ProtectedParams)->asBstr();
    auto unprotParams = coseMac0->asArray()->get(kCoseMac0UnprotectedParams)->asMap();
    auto payload = coseMac0->asArray()->get(kCoseMac0Payload)->asBstr();
    auto tag = coseMac0->asArray()->get(kCoseMac0Tag)->asBstr();
    if (!protParams || !unprotParams || !payload || !tag) {
        return "Invalid COSE_Sign1: missing content";
    }
    auto corruptMac0 = cppbor::Array();
    corruptMac0.add(protParams->clone());
    corruptMac0.add(unprotParams->clone());
    corruptMac0.add(payload->clone());
    vector<uint8_t> tagData = tag->value();
    tagData[0] ^= 0x08;
    tagData[tagData.size() - 1] ^= 0x80;
    corruptMac0.add(cppbor::Bstr(tagData));

    return MacedPublicKey{corruptMac0.encode()};
}

ErrMsgOr<cppbor::Array> corrupt_sig(const cppbor::Array* coseSign1) {
    if (coseSign1->size() != kCoseSign1EntryCount) {
        return "Invalid COSE_Sign1, wrong entry count";
    }
    const cppbor::Bstr* protectedParams = coseSign1->get(kCoseSign1ProtectedParams)->asBstr();
    const cppbor::Map* unprotectedParams = coseSign1->get(kCoseSign1UnprotectedParams)->asMap();
    const cppbor::Bstr* payload = coseSign1->get(kCoseSign1Payload)->asBstr();
    const cppbor::Bstr* signature = coseSign1->get(kCoseSign1Signature)->asBstr();
    if (!protectedParams || !unprotectedParams || !payload || !signature) {
        return "Invalid COSE_Sign1: missing content";
    }

    auto corruptSig = cppbor::Array();
    corruptSig.add(protectedParams->clone());
    corruptSig.add(unprotectedParams->clone());
    corruptSig.add(payload->clone());
    vector<uint8_t> sigData = signature->value();
    sigData[0] ^= 0x08;
    corruptSig.add(cppbor::Bstr(sigData));

    return std::move(corruptSig);
}

ErrMsgOr<EekChain> corrupt_sig_chain(const EekChain& eek, int which) {
    auto [chain, _, parseErr] = cppbor::parse(eek.chain);
    if (!chain || !chain->asArray()) {
        return "EekChain parse failed";
    }

    cppbor::Array* eekChain = chain->asArray();
    if (which >= eekChain->size()) {
        return "selected sig out of range";
    }
    auto corruptChain = cppbor::Array();

    for (int ii = 0; ii < eekChain->size(); ++ii) {
        if (ii == which) {
            auto sig = corrupt_sig(eekChain->get(which)->asArray());
            if (!sig) {
                return "Failed to build corrupted signature" + sig.moveMessage();
            }
            corruptChain.add(sig.moveValue());
        } else {
            corruptChain.add(eekChain->get(ii)->clone());
        }
    }
    return EekChain{corruptChain.encode(), eek.last_pubkey, eek.last_privkey};
}

string device_suffix(const string& name) {
    size_t pos = name.find('/');
    if (pos == string::npos) {
        return name;
    }
    return name.substr(pos + 1);
}

bool matching_keymint_device(const string& rp_name, std::shared_ptr<IKeyMintDevice>* keyMint) {
    string rp_suffix = device_suffix(rp_name);

    vector<string> km_names = ::android::getAidlHalInstanceNames(IKeyMintDevice::descriptor);
    for (const string& km_name : km_names) {
        // If the suffix of the KeyMint instance equals the suffix of the
        // RemotelyProvisionedComponent instance, assume they match.
        if (device_suffix(km_name) == rp_suffix && AServiceManager_isDeclared(km_name.c_str())) {
            ::ndk::SpAIBinder binder(AServiceManager_waitForService(km_name.c_str()));
            *keyMint = IKeyMintDevice::fromBinder(binder);
            return true;
        }
    }
    return false;
}

}  // namespace

class VtsRemotelyProvisionedComponentTests : public testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        if (AServiceManager_isDeclared(GetParam().c_str())) {
            ::ndk::SpAIBinder binder(AServiceManager_waitForService(GetParam().c_str()));
            provisionable_ = IRemotelyProvisionedComponent::fromBinder(binder);
        }
        ASSERT_NE(provisionable_, nullptr);
    }

    static vector<string> build_params() {
        auto params = ::android::getAidlHalInstanceNames(IRemotelyProvisionedComponent::descriptor);
        return params;
    }

  protected:
    std::shared_ptr<IRemotelyProvisionedComponent> provisionable_;
};

using GenerateKeyTests = VtsRemotelyProvisionedComponentTests;

INSTANTIATE_REM_PROV_AIDL_TEST(GenerateKeyTests);

/**
 * Generate and validate a production-mode key.  MAC tag can't be verified, but
 * the private key blob should be usable in KeyMint operations.
 */
TEST_P(GenerateKeyTests, generateEcdsaP256Key_prodMode) {
    MacedPublicKey macedPubKey;
    bytevec privateKeyBlob;
    bool testMode = false;
    auto status = provisionable_->generateEcdsaP256KeyPair(testMode, &macedPubKey, &privateKeyBlob);
    ASSERT_TRUE(status.isOk());
    vector<uint8_t> coseKeyData;
    check_maced_pubkey(macedPubKey, testMode, &coseKeyData);
}

/**
 * Generate and validate a production-mode key, then use it as a KeyMint attestation key.
 */
TEST_P(GenerateKeyTests, generateAndUseEcdsaP256Key_prodMode) {
    // See if there is a matching IKeyMintDevice for this IRemotelyProvisionedComponent.
    std::shared_ptr<IKeyMintDevice> keyMint;
    if (!matching_keymint_device(GetParam(), &keyMint)) {
        // No matching IKeyMintDevice.
        GTEST_SKIP() << "Skipping key use test as no matching KeyMint device found";
        return;
    }
    KeyMintHardwareInfo info;
    ASSERT_TRUE(keyMint->getHardwareInfo(&info).isOk());

    MacedPublicKey macedPubKey;
    bytevec privateKeyBlob;
    bool testMode = false;
    auto status = provisionable_->generateEcdsaP256KeyPair(testMode, &macedPubKey, &privateKeyBlob);
    ASSERT_TRUE(status.isOk());
    vector<uint8_t> coseKeyData;
    check_maced_pubkey(macedPubKey, testMode, &coseKeyData);

    AttestationKey attestKey;
    attestKey.keyBlob = std::move(privateKeyBlob);
    attestKey.issuerSubjectName = make_name_from_str("Android Keystore Key");

    // Generate an ECDSA key that is attested by the generated P256 keypair.
    AuthorizationSet keyDesc = AuthorizationSetBuilder()
                                       .Authorization(TAG_NO_AUTH_REQUIRED)
                                       .EcdsaSigningKey(256)
                                       .AttestationChallenge("foo")
                                       .AttestationApplicationId("bar")
                                       .Digest(Digest::NONE)
                                       .SetDefaultValidity();
    KeyCreationResult creationResult;
    auto result = keyMint->generateKey(keyDesc.vector_data(), attestKey, &creationResult);
    ASSERT_TRUE(result.isOk());
    vector<uint8_t> attested_key_blob = std::move(creationResult.keyBlob);
    vector<KeyCharacteristics> attested_key_characteristics =
            std::move(creationResult.keyCharacteristics);
    vector<Certificate> attested_key_cert_chain = std::move(creationResult.certificateChain);
    EXPECT_EQ(attested_key_cert_chain.size(), 1);

    AuthorizationSet hw_enforced = HwEnforcedAuthorizations(attested_key_characteristics);
    AuthorizationSet sw_enforced = SwEnforcedAuthorizations(attested_key_characteristics);
    EXPECT_TRUE(verify_attestation_record("foo", "bar", sw_enforced, hw_enforced,
                                          info.securityLevel,
                                          attested_key_cert_chain[0].encodedCertificate));

    // Attestation by itself is not valid (last entry is not self-signed).
    EXPECT_FALSE(ChainSignaturesAreValid(attested_key_cert_chain));

    // The signature over the attested key should correspond to the P256 public key.
    X509_Ptr key_cert(parse_cert_blob(attested_key_cert_chain[0].encodedCertificate));
    ASSERT_TRUE(key_cert.get());
    EVP_PKEY_Ptr signing_pubkey;
    p256_pub_key(coseKeyData, &signing_pubkey);
    ASSERT_TRUE(signing_pubkey.get());

    ASSERT_TRUE(X509_verify(key_cert.get(), signing_pubkey.get()))
            << "Verification of attested certificate failed "
            << "OpenSSL error string: " << ERR_error_string(ERR_get_error(), NULL);
}

/**
 * Generate and validate a test-mode key.
 */
TEST_P(GenerateKeyTests, generateEcdsaP256Key_testMode) {
    MacedPublicKey macedPubKey;
    bytevec privateKeyBlob;
    bool testMode = true;
    auto status = provisionable_->generateEcdsaP256KeyPair(testMode, &macedPubKey, &privateKeyBlob);
    ASSERT_TRUE(status.isOk());

    check_maced_pubkey(macedPubKey, testMode, nullptr);
}

class CertificateRequestTest : public VtsRemotelyProvisionedComponentTests {
  protected:
    CertificateRequestTest() : eekId_(string_to_bytevec("eekid")), challenge_(randomBytes(32)) {
        generateEek(3);
    }

    void generateEek(size_t eekLength) {
        auto chain = generateEekChain(eekLength, eekId_);
        EXPECT_TRUE(chain) << chain.message();
        if (chain) eekChain_ = chain.moveValue();
        eekLength_ = eekLength;
    }

    void generateKeys(bool testMode, size_t numKeys) {
        keysToSign_ = std::vector<MacedPublicKey>(numKeys);
        cborKeysToSign_ = cppbor::Array();

        for (auto& key : keysToSign_) {
            bytevec privateKeyBlob;
            auto status = provisionable_->generateEcdsaP256KeyPair(testMode, &key, &privateKeyBlob);
            ASSERT_TRUE(status.isOk()) << status.getMessage();

            vector<uint8_t> payload_value;
            check_maced_pubkey(key, testMode, &payload_value);
            cborKeysToSign_.add(cppbor::EncodedItem(payload_value));
        }
    }

    void checkProtectedData(const DeviceInfo& deviceInfo, const cppbor::Array& keysToSign,
                            const bytevec& keysToSignMac, const ProtectedData& protectedData) {
        auto [parsedProtectedData, _, protDataErrMsg] = cppbor::parse(protectedData.protectedData);
        ASSERT_TRUE(parsedProtectedData) << protDataErrMsg;
        ASSERT_TRUE(parsedProtectedData->asArray());
        ASSERT_EQ(parsedProtectedData->asArray()->size(), kCoseEncryptEntryCount);

        auto senderPubkey = getSenderPubKeyFromCoseEncrypt(parsedProtectedData);
        ASSERT_TRUE(senderPubkey) << senderPubkey.message();
        EXPECT_EQ(senderPubkey->second, eekId_);

        auto sessionKey = x25519_HKDF_DeriveKey(eekChain_.last_pubkey, eekChain_.last_privkey,
                                                senderPubkey->first, false /* senderIsA */);
        ASSERT_TRUE(sessionKey) << sessionKey.message();

        auto protectedDataPayload =
                decryptCoseEncrypt(*sessionKey, parsedProtectedData.get(), bytevec{} /* aad */);
        ASSERT_TRUE(protectedDataPayload) << protectedDataPayload.message();

        auto [parsedPayload, __, payloadErrMsg] = cppbor::parse(*protectedDataPayload);
        ASSERT_TRUE(parsedPayload) << "Failed to parse payload: " << payloadErrMsg;
        ASSERT_TRUE(parsedPayload->asArray());
        EXPECT_EQ(parsedPayload->asArray()->size(), 2U);

        auto& signedMac = parsedPayload->asArray()->get(0);
        auto& bcc = parsedPayload->asArray()->get(1);
        ASSERT_TRUE(signedMac && signedMac->asArray());
        ASSERT_TRUE(bcc && bcc->asArray());

        // BCC is [ pubkey, + BccEntry]
        auto bccContents = validateBcc(bcc->asArray());
        ASSERT_TRUE(bccContents) << "\n" << bccContents.message() << "\n" << prettyPrint(bcc.get());
        ASSERT_GT(bccContents->size(), 0U);

        auto [deviceInfoMap, __2, deviceInfoErrMsg] = cppbor::parse(deviceInfo.deviceInfo);
        ASSERT_TRUE(deviceInfoMap) << "Failed to parse deviceInfo: " << deviceInfoErrMsg;
        ASSERT_TRUE(deviceInfoMap->asMap());

        auto& signingKey = bccContents->back().pubKey;
        auto macKey = verifyAndParseCoseSign1(/* ignore_signature = */ false, signedMac->asArray(),
                                              signingKey,
                                              cppbor::Array()  // SignedMacAad
                                                      .add(challenge_)
                                                      .add(std::move(deviceInfoMap))
                                                      .add(keysToSignMac)
                                                      .encode());
        ASSERT_TRUE(macKey) << macKey.message();

        auto coseMac0 = cppbor::Array()
                                .add(cppbor::Map()  // protected
                                             .add(ALGORITHM, HMAC_256)
                                             .canonicalize()
                                             .encode())
                                .add(cppbor::Map())        // unprotected
                                .add(keysToSign.encode())  // payload (keysToSign)
                                .add(keysToSignMac);       // tag

        auto macPayload = verifyAndParseCoseMac0(&coseMac0, *macKey);
        ASSERT_TRUE(macPayload) << macPayload.message();
    }

    bytevec eekId_;
    size_t eekLength_;
    EekChain eekChain_;
    bytevec challenge_;
    std::vector<MacedPublicKey> keysToSign_;
    cppbor::Array cborKeysToSign_;
};

/**
 * Generate an empty certificate request in test mode, and decrypt and verify the structure and
 * content.
 */
TEST_P(CertificateRequestTest, EmptyRequest_testMode) {
    bool testMode = true;
    for (size_t eekLength : {2, 3, 7}) {
        SCOPED_TRACE(testing::Message() << "EEK of length " << eekLength);
        generateEek(eekLength);

        bytevec keysToSignMac;
        DeviceInfo deviceInfo;
        ProtectedData protectedData;
        auto status = provisionable_->generateCertificateRequest(
                testMode, {} /* keysToSign */, eekChain_.chain, challenge_, &deviceInfo,
                &protectedData, &keysToSignMac);
        ASSERT_TRUE(status.isOk()) << status.getMessage();

        checkProtectedData(deviceInfo, cppbor::Array(), keysToSignMac, protectedData);
    }
}

/**
 * Generate an empty certificate request in prod mode.  Generation will fail because we don't have a
 * valid GEEK.
 *
 * TODO(swillden): Get a valid GEEK and use it so the generation can succeed, though we won't be
 * able to decrypt.
 */
TEST_P(CertificateRequestTest, EmptyRequest_prodMode) {
    bool testMode = false;
    for (size_t eekLength : {2, 3, 7}) {
        SCOPED_TRACE(testing::Message() << "EEK of length " << eekLength);
        generateEek(eekLength);

        bytevec keysToSignMac;
        DeviceInfo deviceInfo;
        ProtectedData protectedData;
        auto status = provisionable_->generateCertificateRequest(
                testMode, {} /* keysToSign */, eekChain_.chain, challenge_, &deviceInfo,
                &protectedData, &keysToSignMac);
        EXPECT_FALSE(status.isOk());
        EXPECT_EQ(status.getServiceSpecificError(),
                  BnRemotelyProvisionedComponent::STATUS_INVALID_EEK);
    }
}

/**
 * Generate a non-empty certificate request in test mode.  Decrypt, parse and validate the contents.
 */
TEST_P(CertificateRequestTest, NonEmptyRequest_testMode) {
    bool testMode = true;
    generateKeys(testMode, 4 /* numKeys */);

    for (size_t eekLength : {2, 3, 7}) {
        SCOPED_TRACE(testing::Message() << "EEK of length " << eekLength);
        generateEek(eekLength);

        bytevec keysToSignMac;
        DeviceInfo deviceInfo;
        ProtectedData protectedData;
        auto status = provisionable_->generateCertificateRequest(
                testMode, keysToSign_, eekChain_.chain, challenge_, &deviceInfo, &protectedData,
                &keysToSignMac);
        ASSERT_TRUE(status.isOk()) << status.getMessage();

        checkProtectedData(deviceInfo, cborKeysToSign_, keysToSignMac, protectedData);
    }
}

/**
 * Generate a non-empty certificate request in prod mode.  Must fail because we don't have a valid
 * GEEK.
 *
 * TODO(swillden): Get a valid GEEK and use it so the generation can succeed, though we won't be
 * able to decrypt.
 */
TEST_P(CertificateRequestTest, NonEmptyRequest_prodMode) {
    bool testMode = false;
    generateKeys(testMode, 4 /* numKeys */);

    for (size_t eekLength : {2, 3, 7}) {
        SCOPED_TRACE(testing::Message() << "EEK of length " << eekLength);
        generateEek(eekLength);

        bytevec keysToSignMac;
        DeviceInfo deviceInfo;
        ProtectedData protectedData;
        auto status = provisionable_->generateCertificateRequest(
                testMode, keysToSign_, eekChain_.chain, challenge_, &deviceInfo, &protectedData,
                &keysToSignMac);
        EXPECT_FALSE(status.isOk());
        EXPECT_EQ(status.getServiceSpecificError(),
                  BnRemotelyProvisionedComponent::STATUS_INVALID_EEK);
    }
}

/**
 * Generate a non-empty certificate request in test mode, but with the MAC corrupted on the keypair.
 */
TEST_P(CertificateRequestTest, NonEmptyRequestCorruptMac_testMode) {
    bool testMode = true;
    generateKeys(testMode, 1 /* numKeys */);
    MacedPublicKey keyWithCorruptMac = corrupt_maced_key(keysToSign_[0]).moveValue();

    bytevec keysToSignMac;
    DeviceInfo deviceInfo;
    ProtectedData protectedData;
    auto status = provisionable_->generateCertificateRequest(
            testMode, {keyWithCorruptMac}, eekChain_.chain, challenge_, &deviceInfo, &protectedData,
            &keysToSignMac);
    ASSERT_FALSE(status.isOk()) << status.getMessage();
    EXPECT_EQ(status.getServiceSpecificError(), BnRemotelyProvisionedComponent::STATUS_INVALID_MAC);
}

/**
 * Generate a non-empty certificate request in prod mode, but with the MAC corrupted on the keypair.
 */
TEST_P(CertificateRequestTest, NonEmptyRequestCorruptMac_prodMode) {
    bool testMode = true;
    generateKeys(testMode, 1 /* numKeys */);
    MacedPublicKey keyWithCorruptMac = corrupt_maced_key(keysToSign_[0]).moveValue();

    bytevec keysToSignMac;
    DeviceInfo deviceInfo;
    ProtectedData protectedData;
    auto status = provisionable_->generateCertificateRequest(
            testMode, {keyWithCorruptMac}, eekChain_.chain, challenge_, &deviceInfo, &protectedData,
            &keysToSignMac);
    ASSERT_FALSE(status.isOk()) << status.getMessage();
    auto rc = status.getServiceSpecificError();

    // TODO(drysdale): drop the INVALID_EEK potential error code when a real GEEK is available.
    EXPECT_TRUE(rc == BnRemotelyProvisionedComponent::STATUS_INVALID_EEK ||
                rc == BnRemotelyProvisionedComponent::STATUS_INVALID_MAC);
}

/**
 * Generate a non-empty certificate request in prod mode that has a corrupt EEK chain.
 * Confirm that the request is rejected.
 *
 * TODO(drysdale): Update to use a valid GEEK, so that the test actually confirms that the
 * implementation is checking signatures.
 */
TEST_P(CertificateRequestTest, NonEmptyCorruptEekRequest_prodMode) {
    bool testMode = false;
    generateKeys(testMode, 4 /* numKeys */);

    for (size_t ii = 0; ii < eekLength_; ii++) {
        auto chain = corrupt_sig_chain(eekChain_, ii);
        ASSERT_TRUE(chain) << chain.message();
        EekChain corruptEek = chain.moveValue();

        bytevec keysToSignMac;
        DeviceInfo deviceInfo;
        ProtectedData protectedData;
        auto status = provisionable_->generateCertificateRequest(
                testMode, keysToSign_, corruptEek.chain, challenge_, &deviceInfo, &protectedData,
                &keysToSignMac);
        ASSERT_FALSE(status.isOk());
        ASSERT_EQ(status.getServiceSpecificError(),
                  BnRemotelyProvisionedComponent::STATUS_INVALID_EEK);
    }
}

/**
 * Generate a non-empty certificate request in prod mode that has an incomplete EEK chain.
 * Confirm that the request is rejected.
 *
 * TODO(drysdale): Update to use a valid GEEK, so that the test actually confirms that the
 * implementation is checking signatures.
 */
TEST_P(CertificateRequestTest, NonEmptyIncompleteEekRequest_prodMode) {
    bool testMode = false;
    generateKeys(testMode, 4 /* numKeys */);

    // Build an EEK chain that omits the first self-signed cert.
    auto truncatedChain = cppbor::Array();
    auto [chain, _, parseErr] = cppbor::parse(eekChain_.chain);
    ASSERT_TRUE(chain);
    auto eekChain = chain->asArray();
    ASSERT_NE(eekChain, nullptr);
    for (size_t ii = 1; ii < eekChain->size(); ii++) {
        truncatedChain.add(eekChain->get(ii)->clone());
    }

    bytevec keysToSignMac;
    DeviceInfo deviceInfo;
    ProtectedData protectedData;
    auto status = provisionable_->generateCertificateRequest(
            testMode, keysToSign_, truncatedChain.encode(), challenge_, &deviceInfo, &protectedData,
            &keysToSignMac);
    ASSERT_FALSE(status.isOk());
    ASSERT_EQ(status.getServiceSpecificError(), BnRemotelyProvisionedComponent::STATUS_INVALID_EEK);
}

/**
 * Generate a non-empty certificate request in test mode, with prod keys.  Must fail with
 * STATUS_PRODUCTION_KEY_IN_TEST_REQUEST.
 */
TEST_P(CertificateRequestTest, NonEmptyRequest_prodKeyInTestCert) {
    generateKeys(false /* testMode */, 2 /* numKeys */);

    bytevec keysToSignMac;
    DeviceInfo deviceInfo;
    ProtectedData protectedData;
    auto status = provisionable_->generateCertificateRequest(
            true /* testMode */, keysToSign_, eekChain_.chain, challenge_, &deviceInfo,
            &protectedData, &keysToSignMac);
    ASSERT_FALSE(status.isOk());
    ASSERT_EQ(status.getServiceSpecificError(),
              BnRemotelyProvisionedComponent::STATUS_PRODUCTION_KEY_IN_TEST_REQUEST);
}

/**
 * Generate a non-empty certificate request in prod mode, with test keys.  Must fail with
 * STATUS_TEST_KEY_IN_PRODUCTION_REQUEST.
 */
TEST_P(CertificateRequestTest, NonEmptyRequest_testKeyInProdCert) {
    generateKeys(true /* testMode */, 2 /* numKeys */);

    bytevec keysToSignMac;
    DeviceInfo deviceInfo;
    ProtectedData protectedData;
    auto status = provisionable_->generateCertificateRequest(
            false /* testMode */, keysToSign_, eekChain_.chain, challenge_, &deviceInfo,
            &protectedData, &keysToSignMac);
    ASSERT_FALSE(status.isOk());
    ASSERT_EQ(status.getServiceSpecificError(),
              BnRemotelyProvisionedComponent::STATUS_TEST_KEY_IN_PRODUCTION_REQUEST);
}

INSTANTIATE_REM_PROV_AIDL_TEST(CertificateRequestTest);

}  // namespace aidl::android::hardware::security::keymint::test
