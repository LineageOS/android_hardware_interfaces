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

class DeviceUniqueAttestationTest : public KeyMintAidlTestBase {
  protected:
    void CheckUniqueAttestationResults(const vector<uint8_t>& key_blob,
                                       const vector<KeyCharacteristics>& key_characteristics,
                                       const AuthorizationSet& hw_enforced) {
        ASSERT_GT(cert_chain_.size(), 0);

        if (KeyMintAidlTestBase::dump_Attestations) {
            std::cout << bin2hex(cert_chain_[0].encodedCertificate) << std::endl;
        }

        ASSERT_GT(key_blob.size(), 0U);

        AuthorizationSet crypto_params = SecLevelAuthorizations(key_characteristics);

        // The device-unique attestation chain should contain exactly three certificates:
        // * The leaf with the attestation extension.
        // * An intermediate, signing the leaf using the device-unique key.
        // * A self-signed root, signed using some authority's key, certifying
        //   the device-unique key.
        const size_t chain_length = cert_chain_.size();
        ASSERT_TRUE(chain_length == 2 || chain_length == 3);
        // TODO(b/191361618): Once StrongBox implementations use a correctly-issued
        // certificate chain, do not skip issuers matching.
        EXPECT_TRUE(ChainSignaturesAreValid(cert_chain_, /* strict_issuer_check= */ false));

        AuthorizationSet sw_enforced = SwEnforcedAuthorizations(key_characteristics);
        EXPECT_TRUE(verify_attestation_record("challenge", "foo", sw_enforced, hw_enforced,
                                              SecLevel(), cert_chain_[0].encodedCertificate));
    }
};

/*
 * DeviceUniqueAttestationTest.RsaNonStrongBoxUnimplemented
 *
 * Verifies that non strongbox implementations do not implement Rsa device unique
 * attestation.
 */
TEST_P(DeviceUniqueAttestationTest, RsaNonStrongBoxUnimplemented) {
    if (SecLevel() == SecurityLevel::STRONGBOX) return;

    vector<uint8_t> key_blob;
    vector<KeyCharacteristics> key_characteristics;

    // Check RSA implementation
    auto result = GenerateKey(AuthorizationSetBuilder()
                                      .Authorization(TAG_NO_AUTH_REQUIRED)
                                      .RsaSigningKey(2048, 65537)
                                      .Digest(Digest::SHA_2_256)
                                      .Padding(PaddingMode::RSA_PKCS1_1_5_SIGN)
                                      .Authorization(TAG_INCLUDE_UNIQUE_ID)
                                      .Authorization(TAG_CREATION_DATETIME, 1619621648000)
                                      .AttestationChallenge("challenge")
                                      .AttestationApplicationId("foo")
                                      .Authorization(TAG_DEVICE_UNIQUE_ATTESTATION),
                              &key_blob, &key_characteristics);

    ASSERT_TRUE(result == ErrorCode::INVALID_ARGUMENT || result == ErrorCode::UNSUPPORTED_TAG);
}

/*
 * DeviceUniqueAttestationTest.EcdsaNonStrongBoxUnimplemented
 *
 * Verifies that non strongbox implementations do not implement Ecdsa device unique
 * attestation.
 */
TEST_P(DeviceUniqueAttestationTest, EcdsaNonStrongBoxUnimplemented) {
    if (SecLevel() == SecurityLevel::STRONGBOX) return;

    vector<uint8_t> key_blob;
    vector<KeyCharacteristics> key_characteristics;

    // Check Ecdsa implementation
    auto result = GenerateKey(AuthorizationSetBuilder()
                                      .Authorization(TAG_NO_AUTH_REQUIRED)
                                      .EcdsaSigningKey(EcCurve::P_256)
                                      .Digest(Digest::SHA_2_256)
                                      .Authorization(TAG_INCLUDE_UNIQUE_ID)
                                      .Authorization(TAG_CREATION_DATETIME, 1619621648000)
                                      .AttestationChallenge("challenge")
                                      .AttestationApplicationId("foo")
                                      .Authorization(TAG_DEVICE_UNIQUE_ATTESTATION),
                              &key_blob, &key_characteristics);

    ASSERT_TRUE(result == ErrorCode::INVALID_ARGUMENT || result == ErrorCode::UNSUPPORTED_TAG);
}

/*
 * DeviceUniqueAttestationTest.RsaDeviceUniqueAttestation
 *
 * Verifies that strongbox implementations of Rsa implements device unique
 * attestation correctly, if implemented.
 */
TEST_P(DeviceUniqueAttestationTest, RsaDeviceUniqueAttestation) {
    if (SecLevel() != SecurityLevel::STRONGBOX) return;

    vector<uint8_t> key_blob;
    vector<KeyCharacteristics> key_characteristics;
    int key_size = 2048;

    auto result = GenerateKey(AuthorizationSetBuilder()
                                      .Authorization(TAG_NO_AUTH_REQUIRED)
                                      .RsaSigningKey(key_size, 65537)
                                      .Digest(Digest::SHA_2_256)
                                      .Padding(PaddingMode::RSA_PKCS1_1_5_SIGN)
                                      .Authorization(TAG_INCLUDE_UNIQUE_ID)
                                      .Authorization(TAG_CREATION_DATETIME, 1619621648000)
                                      .AttestationChallenge("challenge")
                                      .AttestationApplicationId("foo")
                                      .Authorization(TAG_DEVICE_UNIQUE_ATTESTATION),
                              &key_blob, &key_characteristics);

    // It is optional for Strong box to support DeviceUniqueAttestation.
    if (result == ErrorCode::CANNOT_ATTEST_IDS) return;

    ASSERT_EQ(ErrorCode::OK, result);

    AuthorizationSetBuilder hw_enforced =
            AuthorizationSetBuilder()
                    .Authorization(TAG_DEVICE_UNIQUE_ATTESTATION)
                    .Authorization(TAG_NO_AUTH_REQUIRED)
                    .RsaSigningKey(2048, 65537)
                    .Digest(Digest::SHA_2_256)
                    .Padding(PaddingMode::RSA_PKCS1_1_5_SIGN)
                    .Authorization(TAG_ORIGIN, KeyOrigin::GENERATED)
                    .Authorization(TAG_OS_VERSION, os_version())
                    .Authorization(TAG_OS_PATCHLEVEL, os_patch_level());

    // Any patchlevels attached to the key should also be present in the attestation extension.
    AuthorizationSet auths;
    for (const auto& entry : key_characteristics) {
        auths.push_back(AuthorizationSet(entry.authorizations));
    }
    auto vendor_pl = auths.GetTagValue(TAG_VENDOR_PATCHLEVEL);
    if (vendor_pl) {
        hw_enforced.Authorization(TAG_VENDOR_PATCHLEVEL, *vendor_pl);
    }
    auto boot_pl = auths.GetTagValue(TAG_BOOT_PATCHLEVEL);
    if (boot_pl) {
        hw_enforced.Authorization(TAG_BOOT_PATCHLEVEL, *boot_pl);
    }

    CheckUniqueAttestationResults(key_blob, key_characteristics, hw_enforced);
}

/*
 * DeviceUniqueAttestationTest.EcdsaDeviceUniqueAttestation
 *
 * Verifies that strongbox implementations of Rsa implements device unique
 * attestation correctly, if implemented.
 */
TEST_P(DeviceUniqueAttestationTest, EcdsaDeviceUniqueAttestation) {
    if (SecLevel() != SecurityLevel::STRONGBOX) return;

    vector<uint8_t> key_blob;
    vector<KeyCharacteristics> key_characteristics;

    auto result = GenerateKey(AuthorizationSetBuilder()
                                      .Authorization(TAG_NO_AUTH_REQUIRED)
                                      .EcdsaSigningKey(EcCurve::P_256)
                                      .Digest(Digest::SHA_2_256)
                                      .Authorization(TAG_INCLUDE_UNIQUE_ID)
                                      .Authorization(TAG_CREATION_DATETIME, 1619621648000)
                                      .AttestationChallenge("challenge")
                                      .AttestationApplicationId("foo")
                                      .Authorization(TAG_DEVICE_UNIQUE_ATTESTATION),
                              &key_blob, &key_characteristics);

    // It is optional for Strong box to support DeviceUniqueAttestation.
    if (result == ErrorCode::CANNOT_ATTEST_IDS) return;
    ASSERT_EQ(ErrorCode::OK, result);

    AuthorizationSetBuilder hw_enforced =
            AuthorizationSetBuilder()
                    .Authorization(TAG_NO_AUTH_REQUIRED)
                    .EcdsaSigningKey(EcCurve::P_256)
                    .Digest(Digest::SHA_2_256)
                    .Authorization(TAG_DEVICE_UNIQUE_ATTESTATION)
                    .Authorization(TAG_ORIGIN, KeyOrigin::GENERATED)
                    .Authorization(TAG_OS_VERSION, os_version())
                    .Authorization(TAG_OS_PATCHLEVEL, os_patch_level());
    // Any patchlevels attached to the key should also be present in the attestation extension.
    AuthorizationSet auths;
    for (const auto& entry : key_characteristics) {
        auths.push_back(AuthorizationSet(entry.authorizations));
    }
    auto vendor_pl = auths.GetTagValue(TAG_VENDOR_PATCHLEVEL);
    if (vendor_pl) {
        hw_enforced.Authorization(TAG_VENDOR_PATCHLEVEL, *vendor_pl);
    }
    auto boot_pl = auths.GetTagValue(TAG_BOOT_PATCHLEVEL);
    if (boot_pl) {
        hw_enforced.Authorization(TAG_BOOT_PATCHLEVEL, *boot_pl);
    }

    CheckUniqueAttestationResults(key_blob, key_characteristics, hw_enforced);
}

/*
 * DeviceUniqueAttestationTest.EcdsaDeviceUniqueAttestationID
 *
 * Verifies that device unique attestation can include IDs that do match the
 * local device.
 */
TEST_P(DeviceUniqueAttestationTest, EcdsaDeviceUniqueAttestationID) {
    if (SecLevel() != SecurityLevel::STRONGBOX) return;

    // Collection of valid attestation ID tags.
    auto attestation_id_tags = AuthorizationSetBuilder();
    add_tag_from_prop(&attestation_id_tags, TAG_ATTESTATION_ID_BRAND, "ro.product.brand");
    add_tag_from_prop(&attestation_id_tags, TAG_ATTESTATION_ID_DEVICE, "ro.product.device");
    add_tag_from_prop(&attestation_id_tags, TAG_ATTESTATION_ID_PRODUCT, "ro.product.name");
    add_tag_from_prop(&attestation_id_tags, TAG_ATTESTATION_ID_SERIAL, "ro.serial");
    add_tag_from_prop(&attestation_id_tags, TAG_ATTESTATION_ID_MANUFACTURER,
                      "ro.product.manufacturer");
    add_tag_from_prop(&attestation_id_tags, TAG_ATTESTATION_ID_MODEL, "ro.product.model");
    vector<uint8_t> key_blob;
    vector<KeyCharacteristics> key_characteristics;

    for (const KeyParameter& tag : attestation_id_tags) {
        SCOPED_TRACE(testing::Message() << "+tag-" << tag);
        AuthorizationSetBuilder builder =
                AuthorizationSetBuilder()
                        .Authorization(TAG_NO_AUTH_REQUIRED)
                        .EcdsaSigningKey(EcCurve::P_256)
                        .Digest(Digest::SHA_2_256)
                        .Authorization(TAG_INCLUDE_UNIQUE_ID)
                        .Authorization(TAG_CREATION_DATETIME, 1619621648000)
                        .AttestationChallenge("challenge")
                        .AttestationApplicationId("foo")
                        .Authorization(TAG_DEVICE_UNIQUE_ATTESTATION);
        builder.push_back(tag);
        auto result = GenerateKey(builder, &key_blob, &key_characteristics);

        // It is optional for Strong box to support DeviceUniqueAttestation.
        if (result == ErrorCode::CANNOT_ATTEST_IDS) return;
        ASSERT_EQ(ErrorCode::OK, result);

        AuthorizationSetBuilder hw_enforced =
                AuthorizationSetBuilder()
                        .Authorization(TAG_NO_AUTH_REQUIRED)
                        .EcdsaSigningKey(EcCurve::P_256)
                        .Digest(Digest::SHA_2_256)
                        .Authorization(TAG_DEVICE_UNIQUE_ATTESTATION)
                        .Authorization(TAG_ORIGIN, KeyOrigin::GENERATED)
                        .Authorization(TAG_OS_VERSION, os_version())
                        .Authorization(TAG_OS_PATCHLEVEL, os_patch_level());
        // Expect the specified tag to be present in the attestation extension.
        hw_enforced.push_back(tag);
        // Any patchlevels attached to the key should also be present in the attestation extension.
        AuthorizationSet auths;
        for (const auto& entry : key_characteristics) {
            auths.push_back(AuthorizationSet(entry.authorizations));
        }
        auto vendor_pl = auths.GetTagValue(TAG_VENDOR_PATCHLEVEL);
        if (vendor_pl) {
            hw_enforced.Authorization(TAG_VENDOR_PATCHLEVEL, *vendor_pl);
        }
        auto boot_pl = auths.GetTagValue(TAG_BOOT_PATCHLEVEL);
        if (boot_pl) {
            hw_enforced.Authorization(TAG_BOOT_PATCHLEVEL, *boot_pl);
        }
        CheckUniqueAttestationResults(key_blob, key_characteristics, hw_enforced);
    }
}

/*
 * DeviceUniqueAttestationTest.EcdsaDeviceUniqueAttestationMismatchID
 *
 * Verifies that device unique attestation rejects attempts to attest to IDs that
 * don't match the local device.
 */
TEST_P(DeviceUniqueAttestationTest, EcdsaDeviceUniqueAttestationMismatchID) {
    if (SecLevel() != SecurityLevel::STRONGBOX) return;

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
        AuthorizationSetBuilder builder =
                AuthorizationSetBuilder()
                        .Authorization(TAG_NO_AUTH_REQUIRED)
                        .EcdsaSigningKey(EcCurve::P_256)
                        .Digest(Digest::SHA_2_256)
                        .Authorization(TAG_INCLUDE_UNIQUE_ID)
                        .Authorization(TAG_CREATION_DATETIME, 1619621648000)
                        .AttestationChallenge("challenge")
                        .AttestationApplicationId("foo")
                        .Authorization(TAG_DEVICE_UNIQUE_ATTESTATION);
        // Add the tag that doesn't match the local device's real ID.
        builder.push_back(invalid_tag);
        auto result = GenerateKey(builder, &key_blob, &key_characteristics);

        ASSERT_TRUE(result == ErrorCode::CANNOT_ATTEST_IDS || result == ErrorCode::INVALID_TAG);
    }
}

INSTANTIATE_KEYMINT_AIDL_TEST(DeviceUniqueAttestationTest);

}  // namespace aidl::android::hardware::security::keymint::test
