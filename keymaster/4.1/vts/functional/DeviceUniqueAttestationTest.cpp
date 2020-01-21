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

#include "Keymaster4_1HidlTest.h"

#include <cutils/properties.h>

#include <openssl/x509.h>

#include <keymasterV4_1/attestation_record.h>
#include <keymasterV4_1/authorization_set.h>

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

    EXPECT_EQ(expected_sw_enforced, attestation.software_enforced)
            << DIFFERENCE(expected_sw_enforced, attestation.software_enforced);
    EXPECT_EQ(expected_hw_enforced, attestation.hardware_enforced)
            << DIFFERENCE(expected_hw_enforced, attestation.hardware_enforced);
}

}  // namespace

using std::string;
using DeviceUniqueAttestationTest = Keymaster4_1HidlTest;

TEST_P(DeviceUniqueAttestationTest, StrongBoxOnly) {
    if (SecLevel() != SecurityLevel::STRONGBOX) return;

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
                              .Authorization(TAG_ATTESTATION_CHALLENGE, HidlBuf("challenge"))
                              .Authorization(TAG_ATTESTATION_APPLICATION_ID, HidlBuf("foo")),
                      &cert_chain)));
}

TEST_P(DeviceUniqueAttestationTest, Rsa) {
    if (SecLevel() != SecurityLevel::STRONGBOX) return;
    ASSERT_EQ(ErrorCode::OK,
              convert(GenerateKey(AuthorizationSetBuilder()
                                          .Authorization(TAG_NO_AUTH_REQUIRED)
                                          .RsaSigningKey(2048, 65537)
                                          .Digest(Digest::SHA_2_256)
                                          .Padding(PaddingMode::RSA_PKCS1_1_5_SIGN)
                                          .Authorization(TAG_CREATION_DATETIME, 1))));

    hidl_vec<hidl_vec<uint8_t>> cert_chain;
    HidlBuf challenge("challenge");
    HidlBuf app_id("foo");
    EXPECT_EQ(ErrorCode::OK,
              convert(AttestKey(AuthorizationSetBuilder()
                                        .Authorization(TAG_DEVICE_UNIQUE_ATTESTATION)
                                        .Authorization(TAG_ATTESTATION_CHALLENGE, challenge)
                                        .Authorization(TAG_ATTESTATION_APPLICATION_ID, app_id),
                                &cert_chain)));

    EXPECT_EQ(1U, cert_chain.size());
    auto [err, attestation] = parse_attestation_record(cert_chain[0]);
    EXPECT_EQ(ErrorCode::OK, err);

    check_attestation_record(attestation, challenge,
                             /* sw_enforced */
                             AuthorizationSetBuilder()
                                     .Authorization(TAG_CREATION_DATETIME, 1)
                                     .Authorization(TAG_ATTESTATION_APPLICATION_ID, app_id),
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
    ASSERT_EQ(ErrorCode::OK,
              convert(GenerateKey(AuthorizationSetBuilder()
                                          .Authorization(TAG_NO_AUTH_REQUIRED)
                                          .EcdsaSigningKey(256)
                                          .Digest(Digest::SHA_2_256)
                                          .Authorization(TAG_CREATION_DATETIME, 1))));

    hidl_vec<hidl_vec<uint8_t>> cert_chain;
    HidlBuf challenge("challenge");
    HidlBuf app_id("foo");
    EXPECT_EQ(ErrorCode::OK,
              convert(AttestKey(AuthorizationSetBuilder()
                                        .Authorization(TAG_DEVICE_UNIQUE_ATTESTATION)
                                        .Authorization(TAG_ATTESTATION_CHALLENGE, challenge)
                                        .Authorization(TAG_ATTESTATION_APPLICATION_ID, app_id),
                                &cert_chain)));

    EXPECT_EQ(1U, cert_chain.size());
    auto [err, attestation] = parse_attestation_record(cert_chain[0]);
    EXPECT_EQ(ErrorCode::OK, err);

    check_attestation_record(attestation, challenge,
                             /* sw_enforced */
                             AuthorizationSetBuilder()
                                     .Authorization(TAG_CREATION_DATETIME, 1)
                                     .Authorization(TAG_ATTESTATION_APPLICATION_ID, app_id),
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

INSTANTIATE_KEYMASTER_4_1_HIDL_TEST(DeviceUniqueAttestationTest);

}  // namespace test
}  // namespace android::hardware::keymaster::V4_1
