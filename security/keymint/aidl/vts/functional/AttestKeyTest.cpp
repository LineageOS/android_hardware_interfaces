/*
 * Copyright (C) 2021 The Android Open Source Project
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

#define LOG_TAG "keymint_1_attest_key_test"
#include <cutils/log.h>
#include <cutils/properties.h>

#include <keymint_support/key_param_output.h>
#include <keymint_support/openssl_utils.h>

#include "KeyMintAidlTestBase.h"

namespace aidl::android::hardware::security::keymint::test {

namespace {

bool IsSelfSigned(const vector<Certificate>& chain) {
    if (chain.size() != 1) return false;
    return ChainSignaturesAreValid(chain);
}

}  // namespace

class AttestKeyTest : public KeyMintAidlTestBase {
  public:
    void SetUp() override {
        check_skip_test();
        KeyMintAidlTestBase::SetUp();
    }

  protected:
    const string FEATURE_KEYSTORE_APP_ATTEST_KEY = "android.hardware.keystore.app_attest_key";

    const string FEATURE_STRONGBOX_KEYSTORE = "android.hardware.strongbox_keystore";

    ErrorCode GenerateAttestKey(const AuthorizationSet& key_desc,
                                const optional<AttestationKey>& attest_key,
                                vector<uint8_t>* key_blob,
                                vector<KeyCharacteristics>* key_characteristics,
                                vector<Certificate>* cert_chain) {
        // The original specification for KeyMint v1 required ATTEST_KEY not be combined
        // with any other key purpose, but the original VTS tests incorrectly did exactly that.
        // This means that a device that launched prior to Android T (API level 33) may
        // accept or even require KeyPurpose::SIGN too.
        if (property_get_int32("ro.board.first_api_level", 0) < 33) {
            AuthorizationSet key_desc_plus_sign = key_desc;
            key_desc_plus_sign.push_back(TAG_PURPOSE, KeyPurpose::SIGN);

            auto result = GenerateKey(key_desc_plus_sign, attest_key, key_blob, key_characteristics,
                                      cert_chain);
            if (result == ErrorCode::OK) {
                return result;
            }
            // If the key generation failed, it may be because the device is (correctly)
            // rejecting the combination of ATTEST_KEY+SIGN.  Fall through to try again with
            // just ATTEST_KEY.
        }
        return GenerateKey(key_desc, attest_key, key_blob, key_characteristics, cert_chain);
    }

    // Check if ATTEST_KEY feature is disabled
    bool is_attest_key_feature_disabled(void) const {
        if (!check_feature(FEATURE_KEYSTORE_APP_ATTEST_KEY)) {
            GTEST_LOG_(INFO) << "Feature " + FEATURE_KEYSTORE_APP_ATTEST_KEY + " is disabled";
            return true;
        }

        return false;
    }

    // Check if StrongBox KeyStore is enabled
    bool is_strongbox_enabled(void) const {
        if (check_feature(FEATURE_STRONGBOX_KEYSTORE)) {
            GTEST_LOG_(INFO) << "Feature " + FEATURE_STRONGBOX_KEYSTORE + " is enabled";
            return true;
        }

        return false;
    }

    // Check if chipset has received a waiver allowing it to be launched with
    // Android S (or later) with Keymaster 4.0 in StrongBox
    bool is_chipset_allowed_km4_strongbox(void) const {
        std::array<char, PROPERTY_VALUE_MAX> buffer;

        auto res = property_get("ro.vendor.qti.soc_model", buffer.data(), nullptr);
        if (res <= 0) return false;

        const string allowed_soc_models[] = {"SM8450", "SM8475", "SM8550", "SXR2230P"};

        for (const string model : allowed_soc_models) {
            if (model.compare(buffer.data()) == 0) {
                GTEST_LOG_(INFO) << "QTI SOC Model " + model + " is allowed SB KM 4.0";
                return true;
            }
        }

        return false;
    }

    // Skip the test if all the following conditions hold:
    // 1. ATTEST_KEY feature is disabled
    // 2. STRONGBOX is enabled
    // 3. The device is running one of the chipsets that have received a waiver
    //     allowing it to be launched with Android S (or later) with Keymaster 4.0
    //     in StrongBox
    void check_skip_test(void) const {
        if (is_attest_key_feature_disabled() && is_strongbox_enabled() &&
            is_chipset_allowed_km4_strongbox()) {
            GTEST_SKIP() << "Test is not applicable";
        }
    }
};

/*
 * AttestKeyTest.AllRsaSizes
 *
 * This test creates self signed RSA attestation keys of various sizes, and verify they can be
 * used to sign other RSA and EC keys.
 */
TEST_P(AttestKeyTest, AllRsaSizes) {
    for (auto size : ValidKeySizes(Algorithm::RSA)) {
        /*
         * Create attestation key.
         */
        AttestationKey attest_key;
        vector<KeyCharacteristics> attest_key_characteristics;
        vector<Certificate> attest_key_cert_chain;
        ASSERT_EQ(ErrorCode::OK,
                  GenerateAttestKey(AuthorizationSetBuilder()
                                            .RsaKey(size, 65537)
                                            .AttestKey()
                                            .SetDefaultValidity(),
                                    {} /* attestation signing key */, &attest_key.keyBlob,
                                    &attest_key_characteristics, &attest_key_cert_chain));

        ASSERT_GT(attest_key_cert_chain.size(), 0);
        EXPECT_EQ(attest_key_cert_chain.size(), 1);
        EXPECT_TRUE(IsSelfSigned(attest_key_cert_chain)) << "Failed on size " << size;

        /*
         * Use attestation key to sign RSA signing key
         */
        attest_key.issuerSubjectName = make_name_from_str("Android Keystore Key");
        vector<uint8_t> attested_key_blob;
        vector<KeyCharacteristics> attested_key_characteristics;
        vector<Certificate> attested_key_cert_chain;
        EXPECT_EQ(ErrorCode::OK,
                  GenerateKey(AuthorizationSetBuilder()
                                      .RsaSigningKey(2048, 65537)
                                      .Authorization(TAG_NO_AUTH_REQUIRED)
                                      .AttestationChallenge("foo")
                                      .AttestationApplicationId("bar")
                                      .SetDefaultValidity(),
                              attest_key, &attested_key_blob, &attested_key_characteristics,
                              &attested_key_cert_chain));

        CheckedDeleteKey(&attested_key_blob);

        AuthorizationSet hw_enforced = HwEnforcedAuthorizations(attested_key_characteristics);
        AuthorizationSet sw_enforced = SwEnforcedAuthorizations(attested_key_characteristics);
        EXPECT_TRUE(verify_attestation_record(AidlVersion(), "foo", "bar", sw_enforced, hw_enforced,
                                              SecLevel(),
                                              attested_key_cert_chain[0].encodedCertificate));

        // Attestation by itself is not valid (last entry is not self-signed).
        EXPECT_FALSE(ChainSignaturesAreValid(attested_key_cert_chain));

        // Appending the attest_key chain to the attested_key_chain should yield a valid chain.
        attested_key_cert_chain.push_back(attest_key_cert_chain[0]);
        EXPECT_TRUE(ChainSignaturesAreValid(attested_key_cert_chain));
        EXPECT_EQ(attested_key_cert_chain.size(), 2);

        /*
         * Use attestation key to sign RSA decryption key
         */
        attested_key_characteristics.resize(0);
        attested_key_cert_chain.resize(0);
        EXPECT_EQ(ErrorCode::OK,
                  GenerateKey(AuthorizationSetBuilder()
                                      .RsaEncryptionKey(2048, 65537)
                                      .Digest(Digest::NONE)
                                      .Padding(PaddingMode::NONE)
                                      .Authorization(TAG_NO_AUTH_REQUIRED)
                                      .AttestationChallenge("foo2")
                                      .AttestationApplicationId("bar2")
                                      .SetDefaultValidity(),
                              attest_key, &attested_key_blob, &attested_key_characteristics,
                              &attested_key_cert_chain));

        CheckedDeleteKey(&attested_key_blob);

        hw_enforced = HwEnforcedAuthorizations(attested_key_characteristics);
        sw_enforced = SwEnforcedAuthorizations(attested_key_characteristics);
        EXPECT_TRUE(verify_attestation_record(AidlVersion(), "foo2", "bar2", sw_enforced,
                                              hw_enforced, SecLevel(),
                                              attested_key_cert_chain[0].encodedCertificate));

        // Attestation by itself is not valid (last entry is not self-signed).
        EXPECT_FALSE(ChainSignaturesAreValid(attested_key_cert_chain));

        // Appending the attest_key chain to the attested_key_chain should yield a valid chain.
        attested_key_cert_chain.push_back(attest_key_cert_chain[0]);
        EXPECT_TRUE(ChainSignaturesAreValid(attested_key_cert_chain));
        EXPECT_EQ(attested_key_cert_chain.size(), 2);

        /*
         * Use attestation key to sign EC key. Specify a CREATION_DATETIME for this one.
         */
        attested_key_characteristics.resize(0);
        attested_key_cert_chain.resize(0);
        uint64_t timestamp = 1619621648000;
        EXPECT_EQ(ErrorCode::OK,
                  GenerateKey(AuthorizationSetBuilder()
                                      .EcdsaSigningKey(EcCurve::P_256)
                                      .Authorization(TAG_NO_AUTH_REQUIRED)
                                      .AttestationChallenge("foo")
                                      .AttestationApplicationId("bar")
                                      .Authorization(TAG_CREATION_DATETIME, timestamp)
                                      .SetDefaultValidity(),
                              attest_key, &attested_key_blob, &attested_key_characteristics,
                              &attested_key_cert_chain));

        // The returned key characteristics will include CREATION_DATETIME (checked below)
        // in SecurityLevel::KEYSTORE; this will be stripped out in the CheckCharacteristics()
        // call below, to match what getKeyCharacteristics() returns (which doesn't include
        // any SecurityLevel::KEYSTORE characteristics).
        CheckCharacteristics(attested_key_blob, attested_key_characteristics);

        CheckedDeleteKey(&attested_key_blob);
        CheckedDeleteKey(&attest_key.keyBlob);

        hw_enforced = HwEnforcedAuthorizations(attested_key_characteristics);
        sw_enforced = SwEnforcedAuthorizations(attested_key_characteristics);

        // The client-specified CREATION_DATETIME should be in sw_enforced.
        // Its presence will also trigger verify_attestation_record() to check that
        // it is in the attestation extension with a matching value.
        EXPECT_TRUE(sw_enforced.Contains(TAG_CREATION_DATETIME, timestamp))
                << "expected CREATION_TIMESTAMP in sw_enforced:" << sw_enforced
                << " not in hw_enforced:" << hw_enforced;
        EXPECT_TRUE(verify_attestation_record(AidlVersion(), "foo", "bar", sw_enforced, hw_enforced,
                                              SecLevel(),
                                              attested_key_cert_chain[0].encodedCertificate));

        // Attestation by itself is not valid (last entry is not self-signed).
        EXPECT_FALSE(ChainSignaturesAreValid(attested_key_cert_chain));

        // Appending the attest_key chain to the attested_key_chain should yield a valid chain.
        attested_key_cert_chain.push_back(attest_key_cert_chain[0]);
        EXPECT_TRUE(ChainSignaturesAreValid(attested_key_cert_chain));

        // Bail early if anything failed.
        if (HasFailure()) return;
    }
}

/*
 * AttestKeyTest.RsaAttestKeyMultiPurposeFail
 *
 * This test attempts to create an RSA attestation key that also allows signing.
 */
TEST_P(AttestKeyTest, RsaAttestKeyMultiPurposeFail) {
    if (AidlVersion() < 2) {
        // The KeyMint v1 spec required that KeyPurpose::ATTEST_KEY not be combined
        // with other key purposes.  However, this was not checked at the time
        // so we can only be strict about checking this for implementations of KeyMint
        // version 2 and above.
        GTEST_SKIP() << "Single-purpose for KeyPurpose::ATTEST_KEY only strict since KeyMint v2";
    }

    vector<uint8_t> attest_key_blob;
    vector<KeyCharacteristics> attest_key_characteristics;
    vector<Certificate> attest_key_cert_chain;
    ASSERT_EQ(ErrorCode::INCOMPATIBLE_PURPOSE,
              GenerateKey(AuthorizationSetBuilder()
                                  .RsaSigningKey(2048, 65537)
                                  .AttestKey()
                                  .SetDefaultValidity(),
                          {} /* attestation signing key */, &attest_key_blob,
                          &attest_key_characteristics, &attest_key_cert_chain));
}

/*
 * AttestKeyTest.RsaAttestedAttestKeys
 *
 * This test creates an RSA attestation key signed by factory keys, and varifies it can be
 * used to sign other RSA and EC keys.
 */
TEST_P(AttestKeyTest, RsaAttestedAttestKeys) {
    auto challenge = "hello";
    auto app_id = "foo";

    auto subject = "cert subj 2";
    vector<uint8_t> subject_der(make_name_from_str(subject));

    // An X.509 certificate serial number SHOULD be >0, but this is not policed. Check
    // that a zero value doesn't cause problems.
    uint64_t serial_int = 0;
    vector<uint8_t> serial_blob(build_serial_blob(serial_int));

    /*
     * Create attestation key.
     */
    AttestationKey attest_key;
    vector<KeyCharacteristics> attest_key_characteristics;
    vector<Certificate> attest_key_cert_chain;
    auto result = GenerateAttestKey(AuthorizationSetBuilder()
                                            .RsaKey(2048, 65537)
                                            .AttestKey()
                                            .AttestationChallenge(challenge)
                                            .AttestationApplicationId(app_id)
                                            .Authorization(TAG_CERTIFICATE_SERIAL, serial_blob)
                                            .Authorization(TAG_CERTIFICATE_SUBJECT, subject_der)
                                            .Authorization(TAG_NO_AUTH_REQUIRED)
                                            .SetDefaultValidity(),
                                    {} /* attestation signing key */, &attest_key.keyBlob,
                                    &attest_key_characteristics, &attest_key_cert_chain);
    // Strongbox may not support factory provisioned attestation key.
    if (SecLevel() == SecurityLevel::STRONGBOX) {
        if (result == ErrorCode::ATTESTATION_KEYS_NOT_PROVISIONED) return;
    }
    ASSERT_EQ(ErrorCode::OK, result);

    EXPECT_GT(attest_key_cert_chain.size(), 1);
    verify_subject_and_serial(attest_key_cert_chain[0], serial_int, subject, false);
    EXPECT_TRUE(ChainSignaturesAreValid(attest_key_cert_chain));

    AuthorizationSet hw_enforced = HwEnforcedAuthorizations(attest_key_characteristics);
    AuthorizationSet sw_enforced = SwEnforcedAuthorizations(attest_key_characteristics);
    EXPECT_TRUE(verify_attestation_record(AidlVersion(), challenge, app_id,  //
                                          sw_enforced, hw_enforced, SecLevel(),
                                          attest_key_cert_chain[0].encodedCertificate));

    /*
     * Use attestation key to sign RSA key
     */
    attest_key.issuerSubjectName = subject_der;
    vector<uint8_t> attested_key_blob;
    vector<KeyCharacteristics> attested_key_characteristics;
    vector<Certificate> attested_key_cert_chain;

    auto subject2 = "cert subject";
    vector<uint8_t> subject_der2(make_name_from_str(subject2));

    uint64_t serial_int2 = 255;
    vector<uint8_t> serial_blob2(build_serial_blob(serial_int2));

    EXPECT_EQ(ErrorCode::OK,
              GenerateKey(AuthorizationSetBuilder()
                                  .RsaSigningKey(2048, 65537)
                                  .Authorization(TAG_NO_AUTH_REQUIRED)
                                  .AttestationChallenge("foo")
                                  .AttestationApplicationId("bar")
                                  .Authorization(TAG_CERTIFICATE_SERIAL, serial_blob2)
                                  .Authorization(TAG_CERTIFICATE_SUBJECT, subject_der2)
                                  .SetDefaultValidity(),
                          attest_key, &attested_key_blob, &attested_key_characteristics,
                          &attested_key_cert_chain));

    CheckedDeleteKey(&attested_key_blob);
    CheckedDeleteKey(&attest_key.keyBlob);

    AuthorizationSet hw_enforced2 = HwEnforcedAuthorizations(attested_key_characteristics);
    AuthorizationSet sw_enforced2 = SwEnforcedAuthorizations(attested_key_characteristics);
    EXPECT_TRUE(verify_attestation_record(AidlVersion(), "foo", "bar", sw_enforced2, hw_enforced2,
                                          SecLevel(),
                                          attested_key_cert_chain[0].encodedCertificate));

    // Attestation by itself is not valid (last entry is not self-signed).
    EXPECT_FALSE(ChainSignaturesAreValid(attested_key_cert_chain));

    // Appending the attest_key chain to the attested_key_chain should yield a valid chain.
    attested_key_cert_chain.insert(attested_key_cert_chain.end(), attest_key_cert_chain.begin(),
                                   attest_key_cert_chain.end());

    EXPECT_TRUE(ChainSignaturesAreValid(attested_key_cert_chain));
    EXPECT_GT(attested_key_cert_chain.size(), 2);
    verify_subject_and_serial(attested_key_cert_chain[0], serial_int2, subject2, false);
}

/*
 * AttestKeyTest.RsaAttestKeyChaining
 *
 * This test creates a chain of multiple RSA attest keys, each used to sign the next attest key,
 * with the last attest key signed by the factory chain.
 */
TEST_P(AttestKeyTest, RsaAttestKeyChaining) {
    const int chain_size = 6;
    vector<vector<uint8_t>> key_blob_list(chain_size);
    vector<vector<Certificate>> cert_chain_list(chain_size);

    for (int i = 0; i < chain_size; i++) {
        string sub = "attest key chaining ";
        char index = '1' + i;
        string subject = sub + index;
        vector<uint8_t> subject_der(make_name_from_str(subject));

        uint64_t serial_int = 7000 + i;
        vector<uint8_t> serial_blob(build_serial_blob(serial_int));

        vector<KeyCharacteristics> attested_key_characteristics;
        AttestationKey attest_key;
        optional<AttestationKey> attest_key_opt;

        if (i > 0) {
            attest_key.issuerSubjectName = make_name_from_str(sub + (char)(index - 1));
            attest_key.keyBlob = key_blob_list[i - 1];
            attest_key_opt = attest_key;
        }

        auto result = GenerateAttestKey(AuthorizationSetBuilder()
                                                .RsaKey(2048, 65537)
                                                .AttestKey()
                                                .AttestationChallenge("foo")
                                                .AttestationApplicationId("bar")
                                                .Authorization(TAG_NO_AUTH_REQUIRED)
                                                .Authorization(TAG_CERTIFICATE_SERIAL, serial_blob)
                                                .Authorization(TAG_CERTIFICATE_SUBJECT, subject_der)
                                                .SetDefaultValidity(),
                                        attest_key_opt, &key_blob_list[i],
                                        &attested_key_characteristics, &cert_chain_list[i]);
        // Strongbox may not support factory provisioned attestation key.
        if (SecLevel() == SecurityLevel::STRONGBOX) {
            if (result == ErrorCode::ATTESTATION_KEYS_NOT_PROVISIONED) return;
        }
        ASSERT_EQ(ErrorCode::OK, result);

        AuthorizationSet hw_enforced = HwEnforcedAuthorizations(attested_key_characteristics);
        AuthorizationSet sw_enforced = SwEnforcedAuthorizations(attested_key_characteristics);
        ASSERT_GT(cert_chain_list[i].size(), 0);
        EXPECT_TRUE(verify_attestation_record(AidlVersion(), "foo", "bar", sw_enforced, hw_enforced,
                                              SecLevel(),
                                              cert_chain_list[i][0].encodedCertificate));

        if (i > 0) {
            /*
             * The first key is attestated with factory chain, but all the rest of the keys are
             * not supposed to be returned in attestation certificate chains.
             */
            EXPECT_FALSE(ChainSignaturesAreValid(cert_chain_list[i]));

            // Appending the attest_key chain to the attested_key_chain should yield a valid chain.
            cert_chain_list[i].insert(cert_chain_list[i].end(),        //
                                      cert_chain_list[i - 1].begin(),  //
                                      cert_chain_list[i - 1].end());
        }

        EXPECT_TRUE(ChainSignaturesAreValid(cert_chain_list[i]));
        EXPECT_GT(cert_chain_list[i].size(), i + 1);
        verify_subject_and_serial(cert_chain_list[i][0], serial_int, subject, false);
    }

    for (int i = 0; i < chain_size; i++) {
        CheckedDeleteKey(&key_blob_list[i]);
    }
}

/*
 * AttestKeyTest.EcAttestKeyChaining
 *
 * This test creates a chain of multiple Ec attest keys, each used to sign the next attest key,
 * with the last attest key signed by the factory chain.
 */
TEST_P(AttestKeyTest, EcAttestKeyChaining) {
    const int chain_size = 6;
    vector<vector<uint8_t>> key_blob_list(chain_size);
    vector<vector<Certificate>> cert_chain_list(chain_size);

    for (int i = 0; i < chain_size; i++) {
        string sub = "Ec attest key chaining ";
        char index = '1' + i;
        string subject = sub + index;
        vector<uint8_t> subject_der(make_name_from_str(subject));

        uint64_t serial_int = 800000 + i;
        vector<uint8_t> serial_blob(build_serial_blob(serial_int));

        vector<KeyCharacteristics> attested_key_characteristics;
        AttestationKey attest_key;
        optional<AttestationKey> attest_key_opt;

        if (i > 0) {
            attest_key.issuerSubjectName = make_name_from_str(sub + (char)(index - 1));
            attest_key.keyBlob = key_blob_list[i - 1];
            attest_key_opt = attest_key;
        }

        auto result = GenerateAttestKey(AuthorizationSetBuilder()
                                                .EcdsaKey(EcCurve::P_256)
                                                .AttestKey()
                                                .AttestationChallenge("foo")
                                                .AttestationApplicationId("bar")
                                                .Authorization(TAG_CERTIFICATE_SERIAL, serial_blob)
                                                .Authorization(TAG_CERTIFICATE_SUBJECT, subject_der)
                                                .Authorization(TAG_NO_AUTH_REQUIRED)
                                                .SetDefaultValidity(),
                                        attest_key_opt, &key_blob_list[i],
                                        &attested_key_characteristics, &cert_chain_list[i]);
        // Strongbox may not support factory provisioned attestation key.
        if (SecLevel() == SecurityLevel::STRONGBOX) {
            if (result == ErrorCode::ATTESTATION_KEYS_NOT_PROVISIONED) return;
        }
        ASSERT_EQ(ErrorCode::OK, result);

        AuthorizationSet hw_enforced = HwEnforcedAuthorizations(attested_key_characteristics);
        AuthorizationSet sw_enforced = SwEnforcedAuthorizations(attested_key_characteristics);
        ASSERT_GT(cert_chain_list[i].size(), 0);
        EXPECT_TRUE(verify_attestation_record(AidlVersion(), "foo", "bar", sw_enforced, hw_enforced,
                                              SecLevel(),
                                              cert_chain_list[i][0].encodedCertificate));

        if (i > 0) {
            /*
             * The first key is attestated with factory chain, but all the rest of the keys are
             * not supposed to be returned in attestation certificate chains.
             */
            EXPECT_FALSE(ChainSignaturesAreValid(cert_chain_list[i]));

            // Appending the attest_key chain to the attested_key_chain should yield a valid chain.
            cert_chain_list[i].insert(cert_chain_list[i].end(),        //
                                      cert_chain_list[i - 1].begin(),  //
                                      cert_chain_list[i - 1].end());
        }

        EXPECT_TRUE(ChainSignaturesAreValid(cert_chain_list[i]));
        EXPECT_GT(cert_chain_list[i].size(), i + 1);
        verify_subject_and_serial(cert_chain_list[i][0], serial_int, subject, false);
    }

    for (int i = 0; i < chain_size; i++) {
        CheckedDeleteKey(&key_blob_list[i]);
    }
}

/*
 * AttestKeyTest.EcAttestKeyMultiPurposeFail
 *
 * This test attempts to create an EC attestation key that also allows signing.
 */
TEST_P(AttestKeyTest, EcAttestKeyMultiPurposeFail) {
    if (AidlVersion() < 2) {
        // The KeyMint v1 spec required that KeyPurpose::ATTEST_KEY not be combined
        // with other key purposes.  However, this was not checked at the time
        // so we can only be strict about checking this for implementations of KeyMint
        // version 2 and above.
        GTEST_SKIP() << "Single-purpose for KeyPurpose::ATTEST_KEY only strict since KeyMint v2";
    }
    vector<uint8_t> attest_key_blob;
    vector<KeyCharacteristics> attest_key_characteristics;
    vector<Certificate> attest_key_cert_chain;
    ASSERT_EQ(ErrorCode::INCOMPATIBLE_PURPOSE,
              GenerateKey(AuthorizationSetBuilder()
                                  .EcdsaSigningKey(EcCurve::P_256)
                                  .AttestKey()
                                  .SetDefaultValidity(),
                          {} /* attestation signing key */, &attest_key_blob,
                          &attest_key_characteristics, &attest_key_cert_chain));
}

/*
 * AttestKeyTest.AlternateAttestKeyChaining
 *
 * This test creates a chain of multiple attest keys, in the order Ec - RSA - Ec - RSA ....
 * Each attest key is used to sign the next attest key, with the last attest key signed by
 * the factory chain. This is to verify different algorithms of attest keys can
 * cross sign each other and be chained together.
 */
TEST_P(AttestKeyTest, AlternateAttestKeyChaining) {
    const int chain_size = 6;
    vector<vector<uint8_t>> key_blob_list(chain_size);
    vector<vector<Certificate>> cert_chain_list(chain_size);

    for (int i = 0; i < chain_size; i++) {
        string sub = "Alt attest key chaining ";
        char index = '1' + i;
        string subject = sub + index;
        vector<uint8_t> subject_der(make_name_from_str(subject));

        uint64_t serial_int = 90000000 + i;
        vector<uint8_t> serial_blob(build_serial_blob(serial_int));

        vector<KeyCharacteristics> attested_key_characteristics;
        AttestationKey attest_key;
        optional<AttestationKey> attest_key_opt;

        if (i > 0) {
            attest_key.issuerSubjectName = make_name_from_str(sub + (char)(index - 1));
            attest_key.keyBlob = key_blob_list[i - 1];
            attest_key_opt = attest_key;
        }
        ErrorCode result;
        if ((i & 0x1) == 1) {
            result = GenerateAttestKey(AuthorizationSetBuilder()
                                               .EcdsaKey(EcCurve::P_256)
                                               .AttestKey()
                                               .AttestationChallenge("foo")
                                               .AttestationApplicationId("bar")
                                               .Authorization(TAG_CERTIFICATE_SERIAL, serial_blob)
                                               .Authorization(TAG_CERTIFICATE_SUBJECT, subject_der)
                                               .Authorization(TAG_NO_AUTH_REQUIRED)
                                               .SetDefaultValidity(),
                                       attest_key_opt, &key_blob_list[i],
                                       &attested_key_characteristics, &cert_chain_list[i]);
        } else {
            result = GenerateAttestKey(AuthorizationSetBuilder()
                                               .RsaKey(2048, 65537)
                                               .AttestKey()
                                               .AttestationChallenge("foo")
                                               .AttestationApplicationId("bar")
                                               .Authorization(TAG_CERTIFICATE_SERIAL, serial_blob)
                                               .Authorization(TAG_CERTIFICATE_SUBJECT, subject_der)
                                               .Authorization(TAG_NO_AUTH_REQUIRED)
                                               .SetDefaultValidity(),
                                       attest_key_opt, &key_blob_list[i],
                                       &attested_key_characteristics, &cert_chain_list[i]);
        }
        // Strongbox may not support factory provisioned attestation key.
        if (SecLevel() == SecurityLevel::STRONGBOX) {
            if (result == ErrorCode::ATTESTATION_KEYS_NOT_PROVISIONED) return;
        }
        ASSERT_EQ(ErrorCode::OK, result);

        AuthorizationSet hw_enforced = HwEnforcedAuthorizations(attested_key_characteristics);
        AuthorizationSet sw_enforced = SwEnforcedAuthorizations(attested_key_characteristics);
        ASSERT_GT(cert_chain_list[i].size(), 0);
        EXPECT_TRUE(verify_attestation_record(AidlVersion(), "foo", "bar", sw_enforced, hw_enforced,
                                              SecLevel(),
                                              cert_chain_list[i][0].encodedCertificate));

        if (i > 0) {
            /*
             * The first key is attestated with factory chain, but all the rest of the keys are
             * not supposed to be returned in attestation certificate chains.
             */
            EXPECT_FALSE(ChainSignaturesAreValid(cert_chain_list[i]));

            // Appending the attest_key chain to the attested_key_chain should yield a valid chain.
            cert_chain_list[i].insert(cert_chain_list[i].end(),        //
                                      cert_chain_list[i - 1].begin(),  //
                                      cert_chain_list[i - 1].end());
        }

        EXPECT_TRUE(ChainSignaturesAreValid(cert_chain_list[i]));
        EXPECT_GT(cert_chain_list[i].size(), i + 1);
        verify_subject_and_serial(cert_chain_list[i][0], serial_int, subject, false);
    }

    for (int i = 0; i < chain_size; i++) {
        CheckedDeleteKey(&key_blob_list[i]);
    }
}

TEST_P(AttestKeyTest, MissingChallenge) {
    for (auto size : ValidKeySizes(Algorithm::RSA)) {
        /*
         * Create attestation key.
         */
        AttestationKey attest_key;
        vector<KeyCharacteristics> attest_key_characteristics;
        vector<Certificate> attest_key_cert_chain;
        ASSERT_EQ(ErrorCode::OK,
                  GenerateAttestKey(AuthorizationSetBuilder()
                                            .RsaKey(size, 65537)
                                            .AttestKey()
                                            .SetDefaultValidity(),
                                    {} /* attestation signing key */, &attest_key.keyBlob,
                                    &attest_key_characteristics, &attest_key_cert_chain));

        EXPECT_EQ(attest_key_cert_chain.size(), 1);
        EXPECT_TRUE(IsSelfSigned(attest_key_cert_chain)) << "Failed on size " << size;

        /*
         * Use attestation key to sign RSA / ECDSA key but forget to provide a challenge
         */
        attest_key.issuerSubjectName = make_name_from_str("Android Keystore Key");
        vector<uint8_t> attested_key_blob;
        vector<KeyCharacteristics> attested_key_characteristics;
        vector<Certificate> attested_key_cert_chain;
        EXPECT_EQ(ErrorCode::ATTESTATION_CHALLENGE_MISSING,
                  GenerateKey(AuthorizationSetBuilder()
                                      .RsaSigningKey(2048, 65537)
                                      .Authorization(TAG_NO_AUTH_REQUIRED)
                                      .AttestationApplicationId("bar")
                                      .SetDefaultValidity(),
                              attest_key, &attested_key_blob, &attested_key_characteristics,
                              &attested_key_cert_chain));

        EXPECT_EQ(ErrorCode::ATTESTATION_CHALLENGE_MISSING,
                  GenerateKey(AuthorizationSetBuilder()
                                      .EcdsaSigningKey(EcCurve::P_256)
                                      .Authorization(TAG_NO_AUTH_REQUIRED)
                                      .AttestationApplicationId("bar")
                                      .SetDefaultValidity(),
                              attest_key, &attested_key_blob, &attested_key_characteristics,
                              &attested_key_cert_chain));

        CheckedDeleteKey(&attest_key.keyBlob);
    }
}

TEST_P(AttestKeyTest, AllEcCurves) {
    for (auto curve : ValidCurves()) {
        /*
         * Create attestation key.
         */
        AttestationKey attest_key;
        vector<KeyCharacteristics> attest_key_characteristics;
        vector<Certificate> attest_key_cert_chain;
        ASSERT_EQ(
                ErrorCode::OK,
                GenerateAttestKey(
                        AuthorizationSetBuilder().EcdsaKey(curve).AttestKey().SetDefaultValidity(),
                        {} /* attestation signing key */, &attest_key.keyBlob,
                        &attest_key_characteristics, &attest_key_cert_chain));

        ASSERT_GT(attest_key_cert_chain.size(), 0);
        EXPECT_EQ(attest_key_cert_chain.size(), 1);
        EXPECT_TRUE(IsSelfSigned(attest_key_cert_chain)) << "Failed on curve " << curve;

        /*
         * Use attestation key to sign RSA key
         */
        attest_key.issuerSubjectName = make_name_from_str("Android Keystore Key");
        vector<uint8_t> attested_key_blob;
        vector<KeyCharacteristics> attested_key_characteristics;
        vector<Certificate> attested_key_cert_chain;
        EXPECT_EQ(ErrorCode::OK,
                  GenerateKey(AuthorizationSetBuilder()
                                      .RsaSigningKey(2048, 65537)
                                      .Authorization(TAG_NO_AUTH_REQUIRED)
                                      .AttestationChallenge("foo")
                                      .AttestationApplicationId("bar")
                                      .SetDefaultValidity(),
                              attest_key, &attested_key_blob, &attested_key_characteristics,
                              &attested_key_cert_chain));

        ASSERT_GT(attested_key_cert_chain.size(), 0);
        CheckedDeleteKey(&attested_key_blob);

        AuthorizationSet hw_enforced = HwEnforcedAuthorizations(attested_key_characteristics);
        AuthorizationSet sw_enforced = SwEnforcedAuthorizations(attested_key_characteristics);
        EXPECT_TRUE(verify_attestation_record(AidlVersion(), "foo", "bar", sw_enforced, hw_enforced,
                                              SecLevel(),
                                              attested_key_cert_chain[0].encodedCertificate));

        // Attestation by itself is not valid (last entry is not self-signed).
        EXPECT_FALSE(ChainSignaturesAreValid(attested_key_cert_chain));

        // Appending the attest_key chain to the attested_key_chain should yield a valid chain.
        if (attest_key_cert_chain.size() > 0) {
            attested_key_cert_chain.push_back(attest_key_cert_chain[0]);
        }
        EXPECT_TRUE(ChainSignaturesAreValid(attested_key_cert_chain));

        /*
         * Use attestation key to sign EC key
         */
        EXPECT_EQ(ErrorCode::OK,
                  GenerateKey(AuthorizationSetBuilder()
                                      .EcdsaSigningKey(EcCurve::P_256)
                                      .Authorization(TAG_NO_AUTH_REQUIRED)
                                      .AttestationChallenge("foo")
                                      .AttestationApplicationId("bar")
                                      .SetDefaultValidity(),
                              attest_key, &attested_key_blob, &attested_key_characteristics,
                              &attested_key_cert_chain));

        ASSERT_GT(attested_key_cert_chain.size(), 0);
        CheckedDeleteKey(&attested_key_blob);
        CheckedDeleteKey(&attest_key.keyBlob);

        hw_enforced = HwEnforcedAuthorizations(attested_key_characteristics);
        sw_enforced = SwEnforcedAuthorizations(attested_key_characteristics);
        EXPECT_TRUE(verify_attestation_record(AidlVersion(), "foo", "bar", sw_enforced, hw_enforced,
                                              SecLevel(),
                                              attested_key_cert_chain[0].encodedCertificate));

        // Attestation by itself is not valid (last entry is not self-signed).
        EXPECT_FALSE(ChainSignaturesAreValid(attested_key_cert_chain));

        // Appending the attest_key chain to the attested_key_chain should yield a valid chain.
        if (attest_key_cert_chain.size() > 0) {
            attested_key_cert_chain.push_back(attest_key_cert_chain[0]);
        }
        EXPECT_TRUE(ChainSignaturesAreValid(attested_key_cert_chain));

        // Bail early if anything failed.
        if (HasFailure()) return;
    }
}

TEST_P(AttestKeyTest, AttestWithNonAttestKey) {
    // Create non-attestation key.
    AttestationKey non_attest_key;
    vector<KeyCharacteristics> non_attest_key_characteristics;
    vector<Certificate> non_attest_key_cert_chain;
    ASSERT_EQ(
            ErrorCode::OK,
            GenerateKey(
                    AuthorizationSetBuilder().EcdsaSigningKey(EcCurve::P_256).SetDefaultValidity(),
                    {} /* attestation signing key */, &non_attest_key.keyBlob,
                    &non_attest_key_characteristics, &non_attest_key_cert_chain));

    ASSERT_GT(non_attest_key_cert_chain.size(), 0);
    EXPECT_EQ(non_attest_key_cert_chain.size(), 1);
    EXPECT_TRUE(IsSelfSigned(non_attest_key_cert_chain));

    // Attempt to sign attestation with non-attest key.
    vector<uint8_t> attested_key_blob;
    vector<KeyCharacteristics> attested_key_characteristics;
    vector<Certificate> attested_key_cert_chain;
    EXPECT_EQ(ErrorCode::INCOMPATIBLE_PURPOSE,
              GenerateKey(AuthorizationSetBuilder()
                                  .EcdsaSigningKey(EcCurve::P_256)
                                  .Authorization(TAG_NO_AUTH_REQUIRED)
                                  .AttestationChallenge("foo")
                                  .AttestationApplicationId("bar")
                                  .SetDefaultValidity(),
                          non_attest_key, &attested_key_blob, &attested_key_characteristics,
                          &attested_key_cert_chain));
}

TEST_P(AttestKeyTest, EcdsaAttestationID) {
    if (is_gsi_image()) {
        // GSI sets up a standard set of device identifiers that may not match
        // the device identifiers held by the device.
        GTEST_SKIP() << "Test not applicable under GSI";
    }
    // Create attestation key.
    AttestationKey attest_key;
    vector<KeyCharacteristics> attest_key_characteristics;
    vector<Certificate> attest_key_cert_chain;
    ASSERT_EQ(ErrorCode::OK,
              GenerateAttestKey(AuthorizationSetBuilder()
                                        .EcdsaKey(EcCurve::P_256)
                                        .AttestKey()
                                        .SetDefaultValidity(),
                                {} /* attestation signing key */, &attest_key.keyBlob,
                                &attest_key_characteristics, &attest_key_cert_chain));
    attest_key.issuerSubjectName = make_name_from_str("Android Keystore Key");
    ASSERT_GT(attest_key_cert_chain.size(), 0);
    EXPECT_EQ(attest_key_cert_chain.size(), 1);
    EXPECT_TRUE(IsSelfSigned(attest_key_cert_chain));

    // Collection of valid attestation ID tags.
    auto attestation_id_tags = AuthorizationSetBuilder();
    add_tag_from_prop(&attestation_id_tags, TAG_ATTESTATION_ID_BRAND, "ro.product.brand");
    add_tag_from_prop(&attestation_id_tags, TAG_ATTESTATION_ID_DEVICE, "ro.product.device");
    add_tag_from_prop(&attestation_id_tags, TAG_ATTESTATION_ID_PRODUCT, "ro.product.name");
    add_tag_from_prop(&attestation_id_tags, TAG_ATTESTATION_ID_SERIAL, "ro.serial");
    add_tag_from_prop(&attestation_id_tags, TAG_ATTESTATION_ID_MANUFACTURER,
                      "ro.product.manufacturer");
    add_tag_from_prop(&attestation_id_tags, TAG_ATTESTATION_ID_MODEL, "ro.product.model");

    for (const KeyParameter& tag : attestation_id_tags) {
        SCOPED_TRACE(testing::Message() << "+tag-" << tag);
        // Use attestation key to sign an ECDSA key, but include an attestation ID field.
        AuthorizationSetBuilder builder = AuthorizationSetBuilder()
                                                  .EcdsaSigningKey(EcCurve::P_256)
                                                  .Authorization(TAG_NO_AUTH_REQUIRED)
                                                  .AttestationChallenge("challenge")
                                                  .AttestationApplicationId("foo")
                                                  .SetDefaultValidity();
        builder.push_back(tag);
        vector<uint8_t> attested_key_blob;
        vector<KeyCharacteristics> attested_key_characteristics;
        vector<Certificate> attested_key_cert_chain;
        auto result = GenerateKey(builder, attest_key, &attested_key_blob,
                                  &attested_key_characteristics, &attested_key_cert_chain);
        if (result == ErrorCode::CANNOT_ATTEST_IDS && !isDeviceIdAttestationRequired()) {
            continue;
        }

        ASSERT_EQ(result, ErrorCode::OK);

        CheckedDeleteKey(&attested_key_blob);

        AuthorizationSet hw_enforced = HwEnforcedAuthorizations(attested_key_characteristics);
        AuthorizationSet sw_enforced = SwEnforcedAuthorizations(attested_key_characteristics);

        // The attested key characteristics will not contain APPLICATION_ID_* fields (their
        // spec definitions all have "Must never appear in KeyCharacteristics"), but the
        // attestation extension should contain them, so make sure the extra tag is added.
        hw_enforced.push_back(tag);

        EXPECT_TRUE(verify_attestation_record(AidlVersion(), "challenge", "foo", sw_enforced,
                                              hw_enforced, SecLevel(),
                                              attested_key_cert_chain[0].encodedCertificate));
    }
    CheckedDeleteKey(&attest_key.keyBlob);
}

TEST_P(AttestKeyTest, EcdsaAttestationMismatchID) {
    // Create attestation key.
    AttestationKey attest_key;
    vector<KeyCharacteristics> attest_key_characteristics;
    vector<Certificate> attest_key_cert_chain;
    ASSERT_EQ(ErrorCode::OK,
              GenerateAttestKey(AuthorizationSetBuilder()
                                        .EcdsaKey(EcCurve::P_256)
                                        .AttestKey()
                                        .SetDefaultValidity(),
                                {} /* attestation signing key */, &attest_key.keyBlob,
                                &attest_key_characteristics, &attest_key_cert_chain));
    attest_key.issuerSubjectName = make_name_from_str("Android Keystore Key");
    ASSERT_GT(attest_key_cert_chain.size(), 0);
    EXPECT_EQ(attest_key_cert_chain.size(), 1);
    EXPECT_TRUE(IsSelfSigned(attest_key_cert_chain));

    // Collection of invalid attestation ID tags.
    auto attestation_id_tags =
            AuthorizationSetBuilder()
                    .Authorization(TAG_ATTESTATION_ID_BRAND, "bogus-brand")
                    .Authorization(TAG_ATTESTATION_ID_DEVICE, "devious-device")
                    .Authorization(TAG_ATTESTATION_ID_PRODUCT, "punctured-product")
                    .Authorization(TAG_ATTESTATION_ID_SERIAL, "suspicious-serial")
                    .Authorization(TAG_ATTESTATION_ID_IMEI, "invalid-imei")
                    .Authorization(TAG_ATTESTATION_ID_MEID, "mismatching-meid")
                    .Authorization(TAG_ATTESTATION_ID_MANUFACTURER, "malformed-manufacturer")
                    .Authorization(TAG_ATTESTATION_ID_MODEL, "malicious-model");
    vector<uint8_t> key_blob;
    vector<KeyCharacteristics> key_characteristics;

    for (const KeyParameter& invalid_tag : attestation_id_tags) {
        SCOPED_TRACE(testing::Message() << "+tag-" << invalid_tag);

        // Use attestation key to sign an ECDSA key, but include an invalid
        // attestation ID field.
        AuthorizationSetBuilder builder = AuthorizationSetBuilder()
                                                  .EcdsaSigningKey(EcCurve::P_256)
                                                  .Authorization(TAG_NO_AUTH_REQUIRED)
                                                  .AttestationChallenge("challenge")
                                                  .AttestationApplicationId("foo")
                                                  .SetDefaultValidity();
        builder.push_back(invalid_tag);
        vector<uint8_t> attested_key_blob;
        vector<KeyCharacteristics> attested_key_characteristics;
        vector<Certificate> attested_key_cert_chain;
        auto result = GenerateKey(builder, attest_key, &attested_key_blob,
                                  &attested_key_characteristics, &attested_key_cert_chain);

        ASSERT_TRUE(result == ErrorCode::CANNOT_ATTEST_IDS || result == ErrorCode::INVALID_TAG)
                << "result = " << result;
    }
    CheckedDeleteKey(&attest_key.keyBlob);
}

INSTANTIATE_KEYMINT_AIDL_TEST(AttestKeyTest);

}  // namespace aidl::android::hardware::security::keymint::test
