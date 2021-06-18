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

#define LOG_TAG "keymaster_hidl_hal_test"
#include <cutils/log.h>
#include <vector>

#include "Keymaster4_1HidlTest.h"

#include <cutils/properties.h>

#include <openssl/x509.h>

#include <keymasterV4_1/attestation_record.h>
#include <keymasterV4_1/authorization_set.h>

using android::hardware::keymaster::V4_0::test::add_tag_from_prop;

// Not to dump the attestation by default. Can enable by specify the parameter
// "--dump_attestations" on lunching VTS
static bool dumpAttestations = false;

namespace android::hardware::keymaster::V4_0 {

bool operator==(const AuthorizationSet& a, const AuthorizationSet& b) {
    return std::equal(a.begin(), a.end(), b.begin(), b.end());
}

}  // namespace android::hardware::keymaster::V4_0

namespace android::hardware::keymaster::V4_1 {

inline ::std::ostream& operator<<(::std::ostream& os, Tag tag) {
    return os << toString(tag);
}

namespace test {

using std::string;
using std::tuple;

namespace {

char nibble2hex[16] = {'0', '1', '2', '3', '4', '5', '6', '7',
                       '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

string bin2hex(const hidl_vec<uint8_t>& data) {
    string retval;
    retval.reserve(data.size() * 2 + 1);
    for (uint8_t byte : data) {
        retval.push_back(nibble2hex[0x0F & (byte >> 4)]);
        retval.push_back(nibble2hex[0x0F & byte]);
    }
    return retval;
}

inline void dumpContent(string content) {
    std::cout << content << std::endl;
}

struct AuthorizationSetDifferences {
    string aName;
    string bName;
    AuthorizationSet aWhackB;
    AuthorizationSet bWhackA;
};

std::ostream& operator<<(std::ostream& o, const AuthorizationSetDifferences& diffs) {
    if (!diffs.aWhackB.empty()) {
        o << "Set " << diffs.aName << " contains the following that " << diffs.bName << " does not"
          << diffs.aWhackB;
        if (!diffs.bWhackA.empty()) o << std::endl;
    }

    if (!diffs.bWhackA.empty()) {
        o << "Set " << diffs.bName << " contains the following that " << diffs.aName << " does not"
          << diffs.bWhackA;
    }
    return o;
}

// Computes and returns a \ b and b \ a ('\' is the set-difference operator, a \ b means all the
// elements that are in a but not b, i.e. take a and whack all the elements in b) to the provided
// stream.  The sets must be sorted.
//
// This provides a simple and clear view of how the two sets differ, generally much
// easier than scrutinizing printouts of the two sets.
AuthorizationSetDifferences difference(string aName, const AuthorizationSet& a, string bName,
                                       const AuthorizationSet& b) {
    AuthorizationSetDifferences diffs = {std::move(aName), std::move(bName), {}, {}};
    std::set_difference(a.begin(), a.end(), b.begin(), b.end(), std::back_inserter(diffs.aWhackB));
    std::set_difference(b.begin(), b.end(), a.begin(), a.end(), std::back_inserter(diffs.bWhackA));
    return diffs;
}

#define DIFFERENCE(a, b) difference(#a, a, #b, b)

void check_root_of_trust(const RootOfTrust& root_of_trust) {
    char vb_meta_device_state[PROPERTY_VALUE_MAX];
    if (property_get("ro.boot.vbmeta.device_state", vb_meta_device_state, "") == 0) return;

    char vb_meta_digest[PROPERTY_VALUE_MAX];
    EXPECT_GT(property_get("ro.boot.vbmeta.digest", vb_meta_digest, ""), 0);
    EXPECT_EQ(vb_meta_digest, bin2hex(root_of_trust.verified_boot_hash));

    // Verified boot key should be all 0's if the boot state is not verified or self signed
    HidlBuf empty_boot_key(string(32, '\0'));

    char vb_meta_bootstate[PROPERTY_VALUE_MAX];
    auto& verified_boot_key = root_of_trust.verified_boot_key;
    auto& verified_boot_state = root_of_trust.verified_boot_state;
    EXPECT_GT(property_get("ro.boot.verifiedbootstate", vb_meta_bootstate, ""), 0);
    if (!strcmp(vb_meta_bootstate, "green")) {
        EXPECT_EQ(verified_boot_state, V4_0::KM_VERIFIED_BOOT_VERIFIED);
        EXPECT_NE(verified_boot_key, empty_boot_key);
    } else if (!strcmp(vb_meta_bootstate, "yellow")) {
        EXPECT_EQ(verified_boot_state, V4_0::KM_VERIFIED_BOOT_SELF_SIGNED);
        EXPECT_NE(verified_boot_key, empty_boot_key);
    } else if (!strcmp(vb_meta_bootstate, "orange")) {
        EXPECT_EQ(verified_boot_state, V4_0::KM_VERIFIED_BOOT_UNVERIFIED);
        EXPECT_EQ(verified_boot_key, empty_boot_key);
    } else if (!strcmp(vb_meta_bootstate, "red")) {
        EXPECT_EQ(verified_boot_state, V4_0::KM_VERIFIED_BOOT_FAILED);
    } else {
        EXPECT_EQ(verified_boot_state, V4_0::KM_VERIFIED_BOOT_UNVERIFIED);
        EXPECT_EQ(verified_boot_key, empty_boot_key);
    }
}

bool tag_in_list(const KeyParameter& entry) {
    // Attestations don't contain everything in key authorization lists, so we need to filter
    // the key lists to produce the lists that we expect to match the attestations.
    auto tag_list = {
            Tag::INCLUDE_UNIQUE_ID, Tag::BLOB_USAGE_REQUIREMENTS, Tag::EC_CURVE,
            Tag::HARDWARE_TYPE,     Tag::VENDOR_PATCHLEVEL,       Tag::BOOT_PATCHLEVEL,
            Tag::CREATION_DATETIME,
    };
    return std::find(tag_list.begin(), tag_list.end(), (V4_1::Tag)entry.tag) != tag_list.end();
}

AuthorizationSet filter_tags(const AuthorizationSet& set) {
    AuthorizationSet filtered;
    std::remove_copy_if(set.begin(), set.end(), std::back_inserter(filtered), tag_in_list);
    return filtered;
}

void check_attestation_record(AttestationRecord attestation, const HidlBuf& challenge,
                              AuthorizationSet expected_sw_enforced,
                              AuthorizationSet expected_hw_enforced,
                              SecurityLevel expected_security_level) {
    EXPECT_EQ(41U, attestation.keymaster_version);
    EXPECT_EQ(4U, attestation.attestation_version);
    EXPECT_EQ(expected_security_level, attestation.attestation_security_level);
    EXPECT_EQ(expected_security_level, attestation.keymaster_security_level);
    EXPECT_EQ(challenge, attestation.attestation_challenge);

    check_root_of_trust(attestation.root_of_trust);

    // Sort all of the authorization lists, so that equality matching works.
    expected_sw_enforced.Sort();
    expected_hw_enforced.Sort();
    attestation.software_enforced.Sort();
    attestation.hardware_enforced.Sort();

    expected_sw_enforced = filter_tags(expected_sw_enforced);
    expected_hw_enforced = filter_tags(expected_hw_enforced);
    AuthorizationSet attestation_sw_enforced = filter_tags(attestation.software_enforced);
    AuthorizationSet attestation_hw_enforced = filter_tags(attestation.hardware_enforced);

    EXPECT_EQ(expected_sw_enforced, attestation_sw_enforced)
            << DIFFERENCE(expected_sw_enforced, attestation_sw_enforced);
    EXPECT_EQ(expected_hw_enforced, attestation_hw_enforced)
            << DIFFERENCE(expected_hw_enforced, attestation_hw_enforced);
}

X509_Ptr parse_cert_blob(const std::vector<uint8_t>& blob) {
    const uint8_t* p = blob.data();
    return X509_Ptr(d2i_X509(nullptr /* allocate new */, &p, blob.size()));
}

bool check_certificate_chain_signatures(const hidl_vec<hidl_vec<uint8_t>>& cert_chain) {
    // TODO: Check that root is self-signed once b/187803288 is resolved.
    for (size_t i = 0; i < cert_chain.size() - 1; ++i) {
        X509_Ptr key_cert(parse_cert_blob(cert_chain[i]));
        X509_Ptr signing_cert(parse_cert_blob(cert_chain[i + 1]));

        if (!key_cert.get() || !signing_cert.get()) {
            return false;
        }

        EVP_PKEY_Ptr signing_pubkey(X509_get_pubkey(signing_cert.get()));
        if (!signing_pubkey.get()) {
            return false;
        }

        if (!X509_verify(key_cert.get(), signing_pubkey.get())) {
            return false;
        }
    }
    return true;
}

}  // namespace

using std::string;
using DeviceUniqueAttestationTest = Keymaster4_1HidlTest;

TEST_P(DeviceUniqueAttestationTest, NonStrongBoxOnly) {
    if (SecLevel() == SecurityLevel::STRONGBOX) return;

    ASSERT_EQ(ErrorCode::OK, convert(GenerateKey(AuthorizationSetBuilder()
                                                         .Authorization(TAG_NO_AUTH_REQUIRED)
                                                         .RsaSigningKey(2048, 65537)
                                                         .Digest(Digest::SHA_2_256)
                                                         .Padding(PaddingMode::RSA_PKCS1_1_5_SIGN)
                                                         .Authorization(TAG_INCLUDE_UNIQUE_ID))));

    hidl_vec<hidl_vec<uint8_t>> cert_chain;
    EXPECT_EQ(ErrorCode::UNIMPLEMENTED,
              convert(AttestKey(
                      AuthorizationSetBuilder()
                              .Authorization(TAG_DEVICE_UNIQUE_ATTESTATION)
                              .Authorization(TAG_ATTESTATION_CHALLENGE, HidlBuf("challenge"))
                              .Authorization(TAG_ATTESTATION_APPLICATION_ID, HidlBuf("foo")),
                      &cert_chain)));
    CheckedDeleteKey();

    ASSERT_EQ(ErrorCode::OK, convert(GenerateKey(AuthorizationSetBuilder()
                                                         .Authorization(TAG_NO_AUTH_REQUIRED)
                                                         .EcdsaSigningKey(EcCurve::P_256)
                                                         .Digest(Digest::SHA_2_256)
                                                         .Authorization(TAG_INCLUDE_UNIQUE_ID))));

    EXPECT_EQ(ErrorCode::UNIMPLEMENTED,
              convert(AttestKey(
                      AuthorizationSetBuilder()
                              .Authorization(TAG_DEVICE_UNIQUE_ATTESTATION)
                              .Authorization(TAG_ATTESTATION_CHALLENGE, HidlBuf("challenge"))
                              .Authorization(TAG_ATTESTATION_APPLICATION_ID, HidlBuf("foo")),
                      &cert_chain)));
    CheckedDeleteKey();
}

TEST_P(DeviceUniqueAttestationTest, Rsa) {
    if (SecLevel() != SecurityLevel::STRONGBOX) return;
    ASSERT_EQ(ErrorCode::OK, convert(GenerateKey(AuthorizationSetBuilder()
                                                         .Authorization(TAG_NO_AUTH_REQUIRED)
                                                         .RsaSigningKey(2048, 65537)
                                                         .Digest(Digest::SHA_2_256)
                                                         .Padding(PaddingMode::RSA_PKCS1_1_5_SIGN)
                                                         .Authorization(TAG_INCLUDE_UNIQUE_ID))));

    hidl_vec<hidl_vec<uint8_t>> cert_chain;
    HidlBuf challenge("challenge");
    HidlBuf app_id("foo");
    ErrorCode result =
            convert(AttestKey(AuthorizationSetBuilder()
                                      .Authorization(TAG_DEVICE_UNIQUE_ATTESTATION)
                                      .Authorization(TAG_ATTESTATION_CHALLENGE, challenge)
                                      .Authorization(TAG_ATTESTATION_APPLICATION_ID, app_id),
                              &cert_chain));

    // It is optional for Strong box to support DeviceUniqueAttestation.
    if (result == ErrorCode::CANNOT_ATTEST_IDS) return;

    EXPECT_EQ(ErrorCode::OK, result);
    EXPECT_EQ(2U, cert_chain.size());
    EXPECT_TRUE(check_certificate_chain_signatures(cert_chain));
    if (dumpAttestations) {
      for (auto cert_ : cert_chain) dumpContent(bin2hex(cert_));
    }
    auto [err, attestation] = parse_attestation_record(cert_chain[0]);
    ASSERT_EQ(ErrorCode::OK, err);

    check_attestation_record(
            attestation, challenge,
            /* sw_enforced */
            AuthorizationSetBuilder().Authorization(TAG_ATTESTATION_APPLICATION_ID, app_id),
            /* hw_enforced */
            AuthorizationSetBuilder()
                    .Authorization(TAG_DEVICE_UNIQUE_ATTESTATION)
                    .Authorization(TAG_NO_AUTH_REQUIRED)
                    .RsaSigningKey(2048, 65537)
                    .Digest(Digest::SHA_2_256)
                    .Padding(PaddingMode::RSA_PKCS1_1_5_SIGN)
                    .Authorization(TAG_ORIGIN, KeyOrigin::GENERATED)
                    .Authorization(TAG_OS_VERSION, os_version())
                    .Authorization(TAG_OS_PATCHLEVEL, os_patch_level()),
            SecLevel());
}

TEST_P(DeviceUniqueAttestationTest, Ecdsa) {
    if (SecLevel() != SecurityLevel::STRONGBOX) return;
    ASSERT_EQ(ErrorCode::OK, convert(GenerateKey(AuthorizationSetBuilder()
                                                         .Authorization(TAG_NO_AUTH_REQUIRED)
                                                         .EcdsaSigningKey(256)
                                                         .Digest(Digest::SHA_2_256)
                                                         .Authorization(TAG_INCLUDE_UNIQUE_ID))));

    hidl_vec<hidl_vec<uint8_t>> cert_chain;
    HidlBuf challenge("challenge");
    HidlBuf app_id("foo");
    ErrorCode result =
            convert(AttestKey(AuthorizationSetBuilder()
                                      .Authorization(TAG_DEVICE_UNIQUE_ATTESTATION)
                                      .Authorization(TAG_ATTESTATION_CHALLENGE, challenge)
                                      .Authorization(TAG_ATTESTATION_APPLICATION_ID, app_id),
                              &cert_chain));

    // It is optional for Strong box to support DeviceUniqueAttestation.
    if (result == ErrorCode::CANNOT_ATTEST_IDS) return;

    EXPECT_EQ(ErrorCode::OK, result);
    EXPECT_EQ(2U, cert_chain.size());
    EXPECT_TRUE(check_certificate_chain_signatures(cert_chain));
    if (dumpAttestations) {
      for (auto cert_ : cert_chain) dumpContent(bin2hex(cert_));
    }
    auto [err, attestation] = parse_attestation_record(cert_chain[0]);
    ASSERT_EQ(ErrorCode::OK, err);

    check_attestation_record(
            attestation, challenge,
            /* sw_enforced */
            AuthorizationSetBuilder().Authorization(TAG_ATTESTATION_APPLICATION_ID, app_id),
            /* hw_enforced */
            AuthorizationSetBuilder()
                    .Authorization(TAG_DEVICE_UNIQUE_ATTESTATION)
                    .Authorization(TAG_NO_AUTH_REQUIRED)
                    .EcdsaSigningKey(256)
                    .Digest(Digest::SHA_2_256)
                    .Authorization(TAG_EC_CURVE, EcCurve::P_256)
                    .Authorization(TAG_ORIGIN, KeyOrigin::GENERATED)
                    .Authorization(TAG_OS_VERSION, os_version())
                    .Authorization(TAG_OS_PATCHLEVEL, os_patch_level()),
            SecLevel());
}

TEST_P(DeviceUniqueAttestationTest, EcdsaDeviceUniqueAttestationID) {
    if (SecLevel() != SecurityLevel::STRONGBOX) return;

    ASSERT_EQ(ErrorCode::OK, convert(GenerateKey(AuthorizationSetBuilder()
                                                         .Authorization(TAG_NO_AUTH_REQUIRED)
                                                         .EcdsaSigningKey(256)
                                                         .Digest(Digest::SHA_2_256)
                                                         .Authorization(TAG_INCLUDE_UNIQUE_ID))));

    // Collection of valid attestation ID tags.
    auto attestation_id_tags = AuthorizationSetBuilder();
    add_tag_from_prop(&attestation_id_tags, V4_0::TAG_ATTESTATION_ID_BRAND, "ro.product.brand");
    add_tag_from_prop(&attestation_id_tags, V4_0::TAG_ATTESTATION_ID_DEVICE, "ro.product.device");
    add_tag_from_prop(&attestation_id_tags, V4_0::TAG_ATTESTATION_ID_PRODUCT, "ro.product.name");
    add_tag_from_prop(&attestation_id_tags, V4_0::TAG_ATTESTATION_ID_SERIAL, "ro.serial");
    add_tag_from_prop(&attestation_id_tags, V4_0::TAG_ATTESTATION_ID_MANUFACTURER,
                      "ro.product.manufacturer");
    add_tag_from_prop(&attestation_id_tags, V4_0::TAG_ATTESTATION_ID_MODEL, "ro.product.model");

    for (const KeyParameter& tag : attestation_id_tags) {
        hidl_vec<hidl_vec<uint8_t>> cert_chain;
        HidlBuf challenge("challenge");
        HidlBuf app_id("foo");
        AuthorizationSetBuilder builder =
                AuthorizationSetBuilder()
                        .Authorization(TAG_DEVICE_UNIQUE_ATTESTATION)
                        .Authorization(TAG_ATTESTATION_CHALLENGE, challenge)
                        .Authorization(TAG_ATTESTATION_APPLICATION_ID, app_id);
        builder.push_back(tag);
        ErrorCode result = convert(AttestKey(builder, &cert_chain));

        // It is optional for Strong box to support DeviceUniqueAttestation.
        if (result == ErrorCode::CANNOT_ATTEST_IDS) return;

        ASSERT_EQ(ErrorCode::OK, result);
        EXPECT_EQ(2U, cert_chain.size());
        if (dumpAttestations) {
            for (auto cert_ : cert_chain) dumpContent(bin2hex(cert_));
        }
        auto [err, attestation] = parse_attestation_record(cert_chain[0]);
        ASSERT_EQ(ErrorCode::OK, err);

        AuthorizationSetBuilder hw_enforced =
                AuthorizationSetBuilder()
                        .Authorization(TAG_DEVICE_UNIQUE_ATTESTATION)
                        .Authorization(TAG_NO_AUTH_REQUIRED)
                        .EcdsaSigningKey(256)
                        .Digest(Digest::SHA_2_256)
                        .Authorization(TAG_ORIGIN, KeyOrigin::GENERATED)
                        .Authorization(TAG_OS_VERSION, os_version())
                        .Authorization(TAG_OS_PATCHLEVEL, os_patch_level());
        hw_enforced.push_back(tag);
        check_attestation_record(
                attestation, challenge,
                /* sw_enforced */
                AuthorizationSetBuilder().Authorization(TAG_ATTESTATION_APPLICATION_ID, app_id),
                hw_enforced, SecLevel());
    }
}

TEST_P(DeviceUniqueAttestationTest, EcdsaDeviceUniqueAttestationMismatchID) {
    if (SecLevel() != SecurityLevel::STRONGBOX) return;

    ASSERT_EQ(ErrorCode::OK, convert(GenerateKey(AuthorizationSetBuilder()
                                                         .Authorization(TAG_NO_AUTH_REQUIRED)
                                                         .EcdsaSigningKey(256)
                                                         .Digest(Digest::SHA_2_256)
                                                         .Authorization(TAG_INCLUDE_UNIQUE_ID))));

    // Collection of invalid attestation ID tags.
    std::string invalid = "completely-invalid";
    auto attestation_id_tags =
            AuthorizationSetBuilder()
                    .Authorization(V4_0::TAG_ATTESTATION_ID_BRAND, invalid.data(), invalid.size())
                    .Authorization(V4_0::TAG_ATTESTATION_ID_DEVICE, invalid.data(), invalid.size())
                    .Authorization(V4_0::TAG_ATTESTATION_ID_PRODUCT, invalid.data(), invalid.size())
                    .Authorization(V4_0::TAG_ATTESTATION_ID_SERIAL, invalid.data(), invalid.size())
                    .Authorization(V4_0::TAG_ATTESTATION_ID_IMEI, invalid.data(), invalid.size())
                    .Authorization(V4_0::TAG_ATTESTATION_ID_MEID, invalid.data(), invalid.size())
                    .Authorization(V4_0::TAG_ATTESTATION_ID_MANUFACTURER, invalid.data(),
                                   invalid.size())
                    .Authorization(V4_0::TAG_ATTESTATION_ID_MODEL, invalid.data(), invalid.size());

    for (const KeyParameter& invalid_tag : attestation_id_tags) {
        hidl_vec<hidl_vec<uint8_t>> cert_chain;
        HidlBuf challenge("challenge");
        HidlBuf app_id("foo");
        AuthorizationSetBuilder builder =
                AuthorizationSetBuilder()
                        .Authorization(TAG_DEVICE_UNIQUE_ATTESTATION)
                        .Authorization(TAG_ATTESTATION_CHALLENGE, challenge)
                        .Authorization(TAG_ATTESTATION_APPLICATION_ID, app_id);
        builder.push_back(invalid_tag);
        ErrorCode result = convert(AttestKey(builder, &cert_chain));

        EXPECT_TRUE(result == ErrorCode::CANNOT_ATTEST_IDS || result == ErrorCode::INVALID_TAG)
                << "result: " << static_cast<int32_t>(result);
    }
}

INSTANTIATE_KEYMASTER_4_1_HIDL_TEST(DeviceUniqueAttestationTest);

}  // namespace test
}  // namespace android::hardware::keymaster::V4_1

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    for (int i = 1; i < argc; ++i) {
        if (argv[i][0] == '-') {
            if (std::string(argv[i]) == "--dump_attestations") {
                dumpAttestations = true;
            }
        }
    }
    int status = RUN_ALL_TESTS();
    ALOGI("Test result = %d", status);
    return status;
}
