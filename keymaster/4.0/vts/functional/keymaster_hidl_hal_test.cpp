/*
 * Copyright (C) 2017 The Android Open Source Project
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

#include <iostream>
#include <signal.h>

#include <openssl/evp.h>
#include <openssl/mem.h>
#include <openssl/x509.h>

#include <cutils/properties.h>

#include <keymasterV4_0/attestation_record.h>
#include <keymasterV4_0/key_param_output.h>
#include <keymasterV4_0/openssl_utils.h>

#include "KeymasterHidlTest.h"

static bool arm_deleteAllKeys = false;
static bool dump_Attestations = false;

namespace android {
namespace hardware {

template <typename T>
bool operator==(const hidl_vec<T>& a, const hidl_vec<T>& b) {
    if (a.size() != b.size()) {
        return false;
    }
    for (size_t i = 0; i < a.size(); ++i) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

namespace keymaster {
namespace V4_0 {

bool operator==(const AuthorizationSet& a, const AuthorizationSet& b) {
    return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin());
}

bool operator==(const KeyCharacteristics& a, const KeyCharacteristics& b) {
    // This isn't very efficient. Oh, well.
    AuthorizationSet a_sw(a.softwareEnforced);
    AuthorizationSet b_sw(b.softwareEnforced);
    AuthorizationSet a_tee(b.hardwareEnforced);
    AuthorizationSet b_tee(b.hardwareEnforced);

    a_sw.Sort();
    b_sw.Sort();
    a_tee.Sort();
    b_tee.Sort();

    return a_sw == b_sw && a_tee == b_tee;
}

namespace test {
namespace {

template <TagType tag_type, Tag tag, typename ValueT>
bool contains(hidl_vec<KeyParameter>& set, TypedTag<tag_type, tag> ttag, ValueT expected_value) {
    size_t count = std::count_if(set.begin(), set.end(), [&](const KeyParameter& param) {
        return param.tag == tag && accessTagValue(ttag, param) == expected_value;
    });
    return count == 1;
}

template <TagType tag_type, Tag tag>
bool contains(hidl_vec<KeyParameter>& set, TypedTag<tag_type, tag>) {
    size_t count = std::count_if(set.begin(), set.end(),
                                 [&](const KeyParameter& param) { return param.tag == tag; });
    return count > 0;
}

constexpr char hex_value[256] = {0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0,  //
                                 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0,  //
                                 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0,  //
                                 0, 1,  2,  3,  4,  5,  6,  7, 8, 9, 0, 0, 0, 0, 0, 0,  // '0'..'9'
                                 0, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 'A'..'F'
                                 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0,  //
                                 0, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 'a'..'f'
                                 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0,  //
                                 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0,  //
                                 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0,  //
                                 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0,  //
                                 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0,  //
                                 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0,  //
                                 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0,  //
                                 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0,  //
                                 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0};

string hex2str(string a) {
    string b;
    size_t num = a.size() / 2;
    b.resize(num);
    for (size_t i = 0; i < num; i++) {
        b[i] = (hex_value[a[i * 2] & 0xFF] << 4) + (hex_value[a[i * 2 + 1] & 0xFF]);
    }
    return b;
}

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

string rsa_key = hex2str(
    "30820275020100300d06092a864886f70d01010105000482025f3082025b"
    "02010002818100c6095409047d8634812d5a218176e45c41d60a75b13901"
    "f234226cffe776521c5a77b9e389417b71c0b6a44d13afe4e4a2805d46c9"
    "da2935adb1ff0c1f24ea06e62b20d776430a4d435157233c6f916783c30e"
    "310fcbd89b85c2d56771169785ac12bca244abda72bfb19fc44d27c81e1d"
    "92de284f4061edfd99280745ea6d2502030100010281801be0f04d9cae37"
    "18691f035338308e91564b55899ffb5084d2460e6630257e05b3ceab0297"
    "2dfabcd6ce5f6ee2589eb67911ed0fac16e43a444b8c861e544a05933657"
    "72f8baf6b22fc9e3c5f1024b063ac080a7b2234cf8aee8f6c47bbf4fd3ac"
    "e7240290bef16c0b3f7f3cdd64ce3ab5912cf6e32f39ab188358afcccd80"
    "81024100e4b49ef50f765d3b24dde01aceaaf130f2c76670a91a61ae08af"
    "497b4a82be6dee8fcdd5e3f7ba1cfb1f0c926b88f88c92bfab137fba2285"
    "227b83c342ff7c55024100ddabb5839c4c7f6bf3d4183231f005b31aa58a"
    "ffdda5c79e4cce217f6bc930dbe563d480706c24e9ebfcab28a6cdefd324"
    "b77e1bf7251b709092c24ff501fd91024023d4340eda3445d8cd26c14411"
    "da6fdca63c1ccd4b80a98ad52b78cc8ad8beb2842c1d280405bc2f6c1bea"
    "214a1d742ab996b35b63a82a5e470fa88dbf823cdd02401b7b57449ad30d"
    "1518249a5f56bb98294d4b6ac12ffc86940497a5a5837a6cf946262b4945"
    "26d328c11e1126380fde04c24f916dec250892db09a6d77cdba351024077"
    "62cd8f4d050da56bd591adb515d24d7ccd32cca0d05f866d583514bd7324"
    "d5f33645e8ed8b4a1cb3cc4a1d67987399f2a09f5b3fb68c88d5e5d90ac3"
    "3492d6");

string ec_256_key = hex2str(
    "308187020100301306072a8648ce3d020106082a8648ce3d030107046d30"
    "6b0201010420737c2ecd7b8d1940bf2930aa9b4ed3ff941eed09366bc032"
    "99986481f3a4d859a14403420004bf85d7720d07c25461683bc648b4778a"
    "9a14dd8a024e3bdd8c7ddd9ab2b528bbc7aa1b51f14ebbbb0bd0ce21bcc4"
    "1c6eb00083cf3376d11fd44949e0b2183bfe");

string ec_521_key = hex2str(
    "3081EE020100301006072A8648CE3D020106052B810400230481D63081D3"
    "02010104420011458C586DB5DAA92AFAB03F4FE46AA9D9C3CE9A9B7A006A"
    "8384BEC4C78E8E9D18D7D08B5BCFA0E53C75B064AD51C449BAE0258D54B9"
    "4B1E885DED08ED4FB25CE9A1818903818600040149EC11C6DF0FA122C6A9"
    "AFD9754A4FA9513A627CA329E349535A5629875A8ADFBE27DCB932C05198"
    "6377108D054C28C6F39B6F2C9AF81802F9F326B842FF2E5F3C00AB7635CF"
    "B36157FC0882D574A10D839C1A0C049DC5E0D775E2EE50671A208431BB45"
    "E78E70BEFE930DB34818EE4D5C26259F5C6B8E28A652950F9F88D7B4B2C9"
    "D9");

string ec_256_key_rfc5915 =
        hex2str("308193020100301306072a8648ce3d020106082a8648ce3d030107047930"
                "770201010420782370a8c8ce5537baadd04dcff079c8158cfa9c67b818b3"
                "8e8d21c9fa750c1da00a06082a8648ce3d030107a14403420004e2cc561e"
                "e701da0ad0ef0d176bb0c919d42e79c393fdc1bd6c4010d85cf2cf8e68c9"
                "05464666f98dad4f01573ba81078b3428570a439ba3229fbc026c550682f");

string ec_256_key_sec1 =
        hex2str("308187020100301306072a8648ce3d020106082a8648ce3d030107046d30"
                "6b0201010420782370a8c8ce5537baadd04dcff079c8158cfa9c67b818b3"
                "8e8d21c9fa750c1da14403420004e2cc561ee701da0ad0ef0d176bb0c919"
                "d42e79c393fdc1bd6c4010d85cf2cf8e68c905464666f98dad4f01573ba8"
                "1078b3428570a439ba3229fbc026c550682f");

struct RSA_Delete {
    void operator()(RSA* p) { RSA_free(p); }
};

X509* parse_cert_blob(const hidl_vec<uint8_t>& blob) {
    const uint8_t* p = blob.data();
    return d2i_X509(nullptr, &p, blob.size());
}

bool verify_chain(const hidl_vec<hidl_vec<uint8_t>>& chain, const std::string& msg,
                  const std::string& signature) {
    {
        EVP_MD_CTX md_ctx_verify;
        X509_Ptr signing_cert(parse_cert_blob(chain[0]));
        EVP_PKEY_Ptr signing_pubkey(X509_get_pubkey(signing_cert.get()));
        EXPECT_TRUE(signing_pubkey);
        ERR_print_errors_cb(
            [](const char* str, size_t len, void* ctx) -> int {
                (void)ctx;
                std::cerr << std::string(str, len) << std::endl;
                return 1;
            },
            nullptr);

        EVP_MD_CTX_init(&md_ctx_verify);

        bool result = false;
        EXPECT_TRUE((result = EVP_DigestVerifyInit(&md_ctx_verify, NULL, EVP_sha256(), NULL,
                                                   signing_pubkey.get())));
        EXPECT_TRUE(
            (result = result && EVP_DigestVerifyUpdate(&md_ctx_verify, msg.c_str(), msg.size())));
        EXPECT_TRUE((result = result && EVP_DigestVerifyFinal(
                                            &md_ctx_verify,
                                            reinterpret_cast<const uint8_t*>(signature.c_str()),
                                            signature.size())));
        EVP_MD_CTX_cleanup(&md_ctx_verify);
        if (!result) return false;
    }
    for (size_t i = 0; i < chain.size(); ++i) {
        X509_Ptr key_cert(parse_cert_blob(chain[i]));
        X509_Ptr signing_cert;
        if (i < chain.size() - 1) {
            signing_cert.reset(parse_cert_blob(chain[i + 1]));
        } else {
            signing_cert.reset(parse_cert_blob(chain[i]));
        }
        EXPECT_TRUE(!!key_cert.get() && !!signing_cert.get());
        if (!key_cert.get() || !signing_cert.get()) return false;

        EVP_PKEY_Ptr signing_pubkey(X509_get_pubkey(signing_cert.get()));
        EXPECT_TRUE(!!signing_pubkey.get());
        if (!signing_pubkey.get()) return false;

        EXPECT_EQ(1, X509_verify(key_cert.get(), signing_pubkey.get()))
            << "Verification of certificate " << i << " failed "
            << "OpenSSL error string: " << ERR_error_string(ERR_get_error(), NULL);

        char* cert_issuer =  //
            X509_NAME_oneline(X509_get_issuer_name(key_cert.get()), nullptr, 0);
        char* signer_subj =
            X509_NAME_oneline(X509_get_subject_name(signing_cert.get()), nullptr, 0);
        EXPECT_STREQ(cert_issuer, signer_subj) << "Cert " << i << " has wrong issuer.";
        if (i == 0) {
            char* cert_sub = X509_NAME_oneline(X509_get_subject_name(key_cert.get()), nullptr, 0);
            EXPECT_STREQ("/CN=Android Keystore Key", cert_sub)
                << "Cert " << i << " has wrong subject.";
            OPENSSL_free(cert_sub);
        }

        OPENSSL_free(cert_issuer);
        OPENSSL_free(signer_subj);

        if (dump_Attestations) std::cout << bin2hex(chain[i]) << std::endl;
    }

    return true;
}

// Extract attestation record from cert. Returned object is still part of cert; don't free it
// separately.
ASN1_OCTET_STRING* get_attestation_record(X509* certificate) {
    ASN1_OBJECT_Ptr oid(OBJ_txt2obj(kAttestionRecordOid, 1 /* dotted string format */));
    EXPECT_TRUE(!!oid.get());
    if (!oid.get()) return nullptr;

    int location = X509_get_ext_by_OBJ(certificate, oid.get(), -1 /* search from beginning */);
    EXPECT_NE(-1, location) << "Attestation extension not found in certificate";
    if (location == -1) return nullptr;

    X509_EXTENSION* attest_rec_ext = X509_get_ext(certificate, location);
    EXPECT_TRUE(!!attest_rec_ext)
        << "Found attestation extension but couldn't retrieve it?  Probably a BoringSSL bug.";
    if (!attest_rec_ext) return nullptr;

    ASN1_OCTET_STRING* attest_rec = X509_EXTENSION_get_data(attest_rec_ext);
    EXPECT_TRUE(!!attest_rec) << "Attestation extension contained no data";
    return attest_rec;
}

bool tag_in_list(const KeyParameter& entry) {
    // Attestations don't contain everything in key authorization lists, so we need to filter
    // the key lists to produce the lists that we expect to match the attestations.
    auto tag_list = {
        Tag::INCLUDE_UNIQUE_ID, Tag::BLOB_USAGE_REQUIREMENTS, Tag::EC_CURVE, Tag::HARDWARE_TYPE,
    };
    return std::find(tag_list.begin(), tag_list.end(), entry.tag) != tag_list.end();
}

AuthorizationSet filter_tags(const AuthorizationSet& set) {
    AuthorizationSet filtered;
    std::remove_copy_if(set.begin(), set.end(), std::back_inserter(filtered), tag_in_list);
    return filtered;
}

std::string make_string(const uint8_t* data, size_t length) {
    return std::string(reinterpret_cast<const char*>(data), length);
}

template <size_t N>
std::string make_string(const uint8_t (&a)[N]) {
    return make_string(a, N);
}

bool avb_verification_enabled() {
    char value[PROPERTY_VALUE_MAX];
    return property_get("ro.boot.vbmeta.device_state", value, "") != 0;
}

}  // namespace

bool verify_attestation_record(const string& challenge, const string& app_id,
                               AuthorizationSet expected_sw_enforced,
                               AuthorizationSet expected_hw_enforced, SecurityLevel security_level,
                               const hidl_vec<uint8_t>& attestation_cert) {
    X509_Ptr cert(parse_cert_blob(attestation_cert));
    EXPECT_TRUE(!!cert.get());
    if (!cert.get()) return false;

    ASN1_OCTET_STRING* attest_rec = get_attestation_record(cert.get());
    EXPECT_TRUE(!!attest_rec);
    if (!attest_rec) return false;

    AuthorizationSet att_sw_enforced;
    AuthorizationSet att_hw_enforced;
    uint32_t att_attestation_version;
    uint32_t att_keymaster_version;
    SecurityLevel att_attestation_security_level;
    SecurityLevel att_keymaster_security_level;
    HidlBuf att_challenge;
    HidlBuf att_unique_id;
    HidlBuf att_app_id;

    auto error = parse_attestation_record(attest_rec->data,                 //
                                          attest_rec->length,               //
                                          &att_attestation_version,         //
                                          &att_attestation_security_level,  //
                                          &att_keymaster_version,           //
                                          &att_keymaster_security_level,    //
                                          &att_challenge,                   //
                                          &att_sw_enforced,                 //
                                          &att_hw_enforced,                 //
                                          &att_unique_id);
    EXPECT_EQ(ErrorCode::OK, error);
    if (error != ErrorCode::OK) return false;

    EXPECT_TRUE(att_attestation_version == 3);

    expected_sw_enforced.push_back(TAG_ATTESTATION_APPLICATION_ID, HidlBuf(app_id));

    EXPECT_EQ(att_keymaster_version, 4U);
    EXPECT_EQ(security_level, att_keymaster_security_level);
    EXPECT_EQ(security_level, att_attestation_security_level);

    EXPECT_EQ(challenge.length(), att_challenge.size());
    EXPECT_EQ(0, memcmp(challenge.data(), att_challenge.data(), challenge.length()));

    char property_value[PROPERTY_VALUE_MAX] = {};
    // TODO(b/136282179): When running under VTS-on-GSI the TEE-backed
    // keymaster implementation will report YYYYMM dates instead of YYYYMMDD
    // for the BOOT_PATCH_LEVEL.
    if (avb_verification_enabled()) {
        for (int i = 0; i < att_hw_enforced.size(); i++) {
            if (att_hw_enforced[i].tag == TAG_BOOT_PATCHLEVEL ||
                att_hw_enforced[i].tag == TAG_VENDOR_PATCHLEVEL) {
                std::string date = std::to_string(att_hw_enforced[i].f.integer);
                // strptime seems to require delimiters, but the tag value will
                // be YYYYMMDD
                date.insert(6, "-");
                date.insert(4, "-");
                EXPECT_EQ(date.size(), 10);
                struct tm time;
                strptime(date.c_str(), "%Y-%m-%d", &time);

                // Day of the month (0-31)
                EXPECT_GE(time.tm_mday, 0);
                EXPECT_LT(time.tm_mday, 32);
                // Months since Jan (0-11)
                EXPECT_GE(time.tm_mon, 0);
                EXPECT_LT(time.tm_mon, 12);
                // Years since 1900
                EXPECT_GT(time.tm_year, 110);
                EXPECT_LT(time.tm_year, 200);
            }
        }
    }

    // Check to make sure boolean values are properly encoded. Presence of a boolean tag indicates
    // true. A provided boolean tag that can be pulled back out of the certificate indicates correct
    // encoding. No need to check if it's in both lists, since the AuthorizationSet compare below
    // will handle mismatches of tags.
    EXPECT_TRUE(expected_hw_enforced.Contains(TAG_NO_AUTH_REQUIRED));

    // Alternatively this checks the opposite - a false boolean tag (one that isn't provided in
    // the authorization list during key generation) isn't being attested to in the certificate.
    EXPECT_FALSE(expected_hw_enforced.Contains(TAG_TRUSTED_USER_PRESENCE_REQUIRED));
    EXPECT_FALSE(att_hw_enforced.Contains(TAG_TRUSTED_USER_PRESENCE_REQUIRED));

    if (att_hw_enforced.Contains(TAG_ALGORITHM, Algorithm::EC)) {
        // For ECDSA keys, either an EC_CURVE or a KEY_SIZE can be specified, but one must be.
        EXPECT_TRUE(att_hw_enforced.Contains(TAG_EC_CURVE) ||
                    att_hw_enforced.Contains(TAG_KEY_SIZE));
    }

    // Test root of trust elements
    HidlBuf verified_boot_key;
    keymaster_verified_boot_t verified_boot_state;
    bool device_locked;
    HidlBuf verified_boot_hash;
    error = parse_root_of_trust(attest_rec->data, attest_rec->length, &verified_boot_key,
                                &verified_boot_state, &device_locked, &verified_boot_hash);
    EXPECT_EQ(ErrorCode::OK, error);

    if (avb_verification_enabled()) {
        EXPECT_NE(property_get("ro.boot.vbmeta.digest", property_value, ""), 0);
        string prop_string(property_value);
        EXPECT_EQ(prop_string.size(), 64);
        EXPECT_EQ(prop_string, bin2hex(verified_boot_hash));

        EXPECT_NE(property_get("ro.boot.vbmeta.device_state", property_value, ""), 0);
        if (!strcmp(property_value, "unlocked")) {
            EXPECT_FALSE(device_locked);
        } else {
            EXPECT_TRUE(device_locked);
        }

        // Check that the expected result from VBMeta matches the build type. Only a user build
        // should have AVB reporting the device is locked.
        EXPECT_NE(property_get("ro.build.type", property_value, ""), 0);
        if (!strcmp(property_value, "user")) {
            EXPECT_TRUE(device_locked);
        } else {
            EXPECT_FALSE(device_locked);
        }
    }

    // Verified boot key should be all 0's if the boot state is not verified or self signed
    std::string empty_boot_key(32, '\0');
    std::string verified_boot_key_str((const char*)verified_boot_key.data(),
                                      verified_boot_key.size());
    EXPECT_NE(property_get("ro.boot.verifiedbootstate", property_value, ""), 0);
    if (!strcmp(property_value, "green")) {
        EXPECT_EQ(verified_boot_state, KM_VERIFIED_BOOT_VERIFIED);
        EXPECT_NE(0, memcmp(verified_boot_key.data(), empty_boot_key.data(),
                            verified_boot_key.size()));
    } else if (!strcmp(property_value, "yellow")) {
        EXPECT_EQ(verified_boot_state, KM_VERIFIED_BOOT_SELF_SIGNED);
        EXPECT_NE(0, memcmp(verified_boot_key.data(), empty_boot_key.data(),
                            verified_boot_key.size()));
    } else if (!strcmp(property_value, "orange")) {
        EXPECT_EQ(verified_boot_state, KM_VERIFIED_BOOT_UNVERIFIED);
        EXPECT_EQ(0, memcmp(verified_boot_key.data(), empty_boot_key.data(),
                            verified_boot_key.size()));
    } else if (!strcmp(property_value, "red")) {
        EXPECT_EQ(verified_boot_state, KM_VERIFIED_BOOT_FAILED);
        EXPECT_EQ(0, memcmp(verified_boot_key.data(), empty_boot_key.data(),
                            verified_boot_key.size()));
    } else {
        EXPECT_TRUE(false);
    }

    att_sw_enforced.Sort();
    expected_sw_enforced.Sort();
    EXPECT_EQ(filter_tags(expected_sw_enforced), filter_tags(att_sw_enforced));

    att_hw_enforced.Sort();
    expected_hw_enforced.Sort();
    EXPECT_EQ(filter_tags(expected_hw_enforced), filter_tags(att_hw_enforced));

    return true;
}

class NewKeyGenerationTest : public KeymasterHidlTest {
   protected:
    void CheckBaseParams(const KeyCharacteristics& keyCharacteristics) {
        // TODO(swillden): Distinguish which params should be in which auth list.

        AuthorizationSet auths(keyCharacteristics.hardwareEnforced);
        auths.push_back(AuthorizationSet(keyCharacteristics.softwareEnforced));

        EXPECT_TRUE(auths.Contains(TAG_ORIGIN, KeyOrigin::GENERATED));
        EXPECT_TRUE(auths.Contains(TAG_PURPOSE, KeyPurpose::SIGN));
        EXPECT_TRUE(auths.Contains(TAG_PURPOSE, KeyPurpose::VERIFY));

        // Verify that App ID, App data and ROT are NOT included.
        EXPECT_FALSE(auths.Contains(TAG_ROOT_OF_TRUST));
        EXPECT_FALSE(auths.Contains(TAG_APPLICATION_ID));
        EXPECT_FALSE(auths.Contains(TAG_APPLICATION_DATA));

        // Check that some unexpected tags/values are NOT present.
        EXPECT_FALSE(auths.Contains(TAG_PURPOSE, KeyPurpose::ENCRYPT));
        EXPECT_FALSE(auths.Contains(TAG_PURPOSE, KeyPurpose::DECRYPT));
        EXPECT_FALSE(auths.Contains(TAG_AUTH_TIMEOUT, 301U));

        // Now check that unspecified, defaulted tags are correct.
        EXPECT_TRUE(auths.Contains(TAG_CREATION_DATETIME));

        EXPECT_TRUE(auths.Contains(TAG_OS_VERSION, os_version()))
            << "OS version is " << os_version() << " key reported "
            << auths.GetTagValue(TAG_OS_VERSION);
        EXPECT_TRUE(auths.Contains(TAG_OS_PATCHLEVEL, os_patch_level()))
            << "OS patch level is " << os_patch_level() << " key reported "
            << auths.GetTagValue(TAG_OS_PATCHLEVEL);
    }

    void CheckCharacteristics(const HidlBuf& key_blob,
                              const KeyCharacteristics& key_characteristics) {
        KeyCharacteristics retrieved_chars;
        ASSERT_EQ(ErrorCode::OK, GetCharacteristics(key_blob, &retrieved_chars));
        EXPECT_EQ(key_characteristics, retrieved_chars);
    }
};

/*
 * NewKeyGenerationTest.Rsa
 *
 * Verifies that keymaster can generate all required RSA key sizes, and that the resulting keys have
 * correct characteristics.
 */
TEST_P(NewKeyGenerationTest, Rsa) {
    for (auto key_size : ValidKeySizes(Algorithm::RSA)) {
        HidlBuf key_blob;
        KeyCharacteristics key_characteristics;
        ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                     .RsaSigningKey(key_size, 65537)
                                                     .Digest(Digest::NONE)
                                                     .Padding(PaddingMode::NONE),
                                             &key_blob, &key_characteristics));

        ASSERT_GT(key_blob.size(), 0U);
        CheckBaseParams(key_characteristics);
        CheckCharacteristics(key_blob, key_characteristics);

        AuthorizationSet crypto_params;
        if (IsSecure()) {
            crypto_params = key_characteristics.hardwareEnforced;
        } else {
            crypto_params = key_characteristics.softwareEnforced;
        }

        EXPECT_TRUE(crypto_params.Contains(TAG_ALGORITHM, Algorithm::RSA));
        EXPECT_TRUE(crypto_params.Contains(TAG_KEY_SIZE, key_size))
            << "Key size " << key_size << "missing";
        EXPECT_TRUE(crypto_params.Contains(TAG_RSA_PUBLIC_EXPONENT, 65537U));

        CheckedDeleteKey(&key_blob);
    }
}

/*
 * NewKeyGenerationTest.NoInvalidRsaSizes
 *
 * Verifies that keymaster cannot generate any RSA key sizes that are designated as invalid.
 */
TEST_P(NewKeyGenerationTest, NoInvalidRsaSizes) {
    for (auto key_size : InvalidKeySizes(Algorithm::RSA)) {
        HidlBuf key_blob;
        KeyCharacteristics key_characteristics;
        ASSERT_EQ(ErrorCode::UNSUPPORTED_KEY_SIZE,
                  GenerateKey(AuthorizationSetBuilder()
                                      .RsaSigningKey(key_size, 65537)
                                      .Digest(Digest::NONE)
                                      .Padding(PaddingMode::NONE),
                              &key_blob, &key_characteristics));
    }
}

/*
 * NewKeyGenerationTest.RsaNoDefaultSize
 *
 * Verifies that failing to specify a key size for RSA key generation returns UNSUPPORTED_KEY_SIZE.
 */
TEST_P(NewKeyGenerationTest, RsaNoDefaultSize) {
    ASSERT_EQ(ErrorCode::UNSUPPORTED_KEY_SIZE,
              GenerateKey(AuthorizationSetBuilder()
                              .Authorization(TAG_ALGORITHM, Algorithm::RSA)
                              .Authorization(TAG_RSA_PUBLIC_EXPONENT, 3U)
                              .SigningKey()));
}

/*
 * NewKeyGenerationTest.Ecdsa
 *
 * Verifies that keymaster can generate all required EC key sizes, and that the resulting keys have
 * correct characteristics.
 */
TEST_P(NewKeyGenerationTest, Ecdsa) {
    for (auto key_size : ValidKeySizes(Algorithm::EC)) {
        HidlBuf key_blob;
        KeyCharacteristics key_characteristics;
        ASSERT_EQ(
            ErrorCode::OK,
            GenerateKey(AuthorizationSetBuilder().EcdsaSigningKey(key_size).Digest(Digest::NONE),
                        &key_blob, &key_characteristics));
        ASSERT_GT(key_blob.size(), 0U);
        CheckBaseParams(key_characteristics);
        CheckCharacteristics(key_blob, key_characteristics);

        AuthorizationSet crypto_params;
        if (IsSecure()) {
            crypto_params = key_characteristics.hardwareEnforced;
        } else {
            crypto_params = key_characteristics.softwareEnforced;
        }

        EXPECT_TRUE(crypto_params.Contains(TAG_ALGORITHM, Algorithm::EC));
        EXPECT_TRUE(crypto_params.Contains(TAG_KEY_SIZE, key_size))
            << "Key size " << key_size << "missing";

        CheckedDeleteKey(&key_blob);
    }
}

/*
 * NewKeyGenerationTest.EcdsaDefaultSize
 *
 * Verifies that failing to specify a key size for EC key generation returns UNSUPPORTED_KEY_SIZE.
 */
TEST_P(NewKeyGenerationTest, EcdsaDefaultSize) {
    ASSERT_EQ(ErrorCode::UNSUPPORTED_KEY_SIZE,
              GenerateKey(AuthorizationSetBuilder()
                              .Authorization(TAG_ALGORITHM, Algorithm::EC)
                              .SigningKey()
                              .Digest(Digest::NONE)));
}

/*
 * NewKeyGenerationTest.EcdsaInvalidSize
 *
 * Verifies that specifying an invalid key size for EC key generation returns UNSUPPORTED_KEY_SIZE.
 */
TEST_P(NewKeyGenerationTest, EcdsaInvalidSize) {
    for (auto key_size : InvalidKeySizes(Algorithm::EC)) {
        HidlBuf key_blob;
        KeyCharacteristics key_characteristics;
        ASSERT_EQ(
            ErrorCode::UNSUPPORTED_KEY_SIZE,
            GenerateKey(AuthorizationSetBuilder().EcdsaSigningKey(key_size).Digest(Digest::NONE),
                        &key_blob, &key_characteristics));
    }

    ASSERT_EQ(ErrorCode::UNSUPPORTED_KEY_SIZE,
              GenerateKey(AuthorizationSetBuilder().EcdsaSigningKey(190).Digest(Digest::NONE)));
}

/*
 * NewKeyGenerationTest.EcdsaMismatchKeySize
 *
 * Verifies that specifying mismatched key size and curve for EC key generation returns
 * INVALID_ARGUMENT.
 */
TEST_P(NewKeyGenerationTest, EcdsaMismatchKeySize) {
    if (SecLevel() == SecurityLevel::STRONGBOX) return;

    ASSERT_EQ(ErrorCode::INVALID_ARGUMENT,
              GenerateKey(AuthorizationSetBuilder()
                              .EcdsaSigningKey(224)
                              .Authorization(TAG_EC_CURVE, EcCurve::P_256)
                              .Digest(Digest::NONE)));
}

/*
 * NewKeyGenerationTest.EcdsaAllValidSizes
 *
 * Verifies that keymaster supports all required EC key sizes.
 */
TEST_P(NewKeyGenerationTest, EcdsaAllValidSizes) {
    auto valid_sizes = ValidKeySizes(Algorithm::EC);
    for (size_t size : valid_sizes) {
        EXPECT_EQ(ErrorCode::OK,
                  GenerateKey(AuthorizationSetBuilder().EcdsaSigningKey(size).Digest(Digest::NONE)))
            << "Failed to generate size: " << size;
        CheckCharacteristics(key_blob_, key_characteristics_);
        CheckedDeleteKey();
    }
}

/*
 * NewKeyGenerationTest.EcdsaInvalidCurves
 *
 * Verifies that keymaster does not support any curve designated as unsupported.
 */
TEST_P(NewKeyGenerationTest, EcdsaAllValidCurves) {
    Digest digest;
    if (SecLevel() == SecurityLevel::STRONGBOX) {
        digest = Digest::SHA_2_256;
    } else {
        digest = Digest::SHA_2_512;
    }
    for (auto curve : ValidCurves()) {
        EXPECT_EQ(
            ErrorCode::OK,
            GenerateKey(AuthorizationSetBuilder().EcdsaSigningKey(curve).Digest(digest)))
            << "Failed to generate key on curve: " << curve;
        CheckCharacteristics(key_blob_, key_characteristics_);
        CheckedDeleteKey();
    }
}

/*
 * NewKeyGenerationTest.Hmac
 *
 * Verifies that keymaster supports all required digests, and that the resulting keys have correct
 * characteristics.
 */
TEST_P(NewKeyGenerationTest, Hmac) {
    for (auto digest : ValidDigests(false /* withNone */, true /* withMD5 */)) {
        HidlBuf key_blob;
        KeyCharacteristics key_characteristics;
        constexpr size_t key_size = 128;
        ASSERT_EQ(
            ErrorCode::OK,
            GenerateKey(AuthorizationSetBuilder().HmacKey(key_size).Digest(digest).Authorization(
                            TAG_MIN_MAC_LENGTH, 128),
                        &key_blob, &key_characteristics));

        ASSERT_GT(key_blob.size(), 0U);
        CheckBaseParams(key_characteristics);
        CheckCharacteristics(key_blob, key_characteristics);

        AuthorizationSet hardwareEnforced = key_characteristics.hardwareEnforced;
        AuthorizationSet softwareEnforced = key_characteristics.softwareEnforced;
        if (IsSecure()) {
            EXPECT_TRUE(hardwareEnforced.Contains(TAG_ALGORITHM, Algorithm::HMAC));
            EXPECT_TRUE(hardwareEnforced.Contains(TAG_KEY_SIZE, key_size))
                << "Key size " << key_size << "missing";
        } else {
            EXPECT_TRUE(softwareEnforced.Contains(TAG_ALGORITHM, Algorithm::HMAC));
            EXPECT_TRUE(softwareEnforced.Contains(TAG_KEY_SIZE, key_size))
                << "Key size " << key_size << "missing";
        }

        CheckedDeleteKey(&key_blob);
    }
}

/*
 * NewKeyGenerationTest.HmacCheckKeySizes
 *
 * Verifies that keymaster supports all key sizes, and rejects all invalid key sizes.
 */
TEST_P(NewKeyGenerationTest, HmacCheckKeySizes) {
    for (size_t key_size = 0; key_size <= 512; ++key_size) {
        if (key_size < 64 || key_size % 8 != 0) {
            // To keep this test from being very slow, we only test a random fraction of non-byte
            // key sizes.  We test only ~10% of such cases. Since there are 392 of them, we expect
            // to run ~40 of them in each run.
            if (key_size % 8 == 0 || random() % 10 == 0) {
                EXPECT_EQ(ErrorCode::UNSUPPORTED_KEY_SIZE,
                          GenerateKey(AuthorizationSetBuilder()
                                          .HmacKey(key_size)
                                          .Digest(Digest::SHA_2_256)
                                          .Authorization(TAG_MIN_MAC_LENGTH, 256)))
                    << "HMAC key size " << key_size << " invalid";
            }
        } else {
            EXPECT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                     .HmacKey(key_size)
                                                     .Digest(Digest::SHA_2_256)
                                                     .Authorization(TAG_MIN_MAC_LENGTH, 256)))
                << "Failed to generate HMAC key of size " << key_size;
            CheckCharacteristics(key_blob_, key_characteristics_);
            CheckedDeleteKey();
        }
    }
}

/*
 * NewKeyGenerationTest.HmacCheckMinMacLengths
 *
 * Verifies that keymaster supports all required MAC lengths and rejects all invalid lengths.  This
 * test is probabilistic in order to keep the runtime down, but any failure prints out the specific
 * MAC length that failed, so reproducing a failed run will be easy.
 */
TEST_P(NewKeyGenerationTest, HmacCheckMinMacLengths) {
    for (size_t min_mac_length = 0; min_mac_length <= 256; ++min_mac_length) {
        if (min_mac_length < 64 || min_mac_length % 8 != 0) {
            // To keep this test from being very long, we only test a random fraction of non-byte
            // lengths.  We test only ~10% of such cases. Since there are 172 of them, we expect to
            // run ~17 of them in each run.
            if (min_mac_length % 8 == 0 || random() % 10 == 0) {
                EXPECT_EQ(ErrorCode::UNSUPPORTED_MIN_MAC_LENGTH,
                          GenerateKey(AuthorizationSetBuilder()
                                          .HmacKey(128)
                                          .Digest(Digest::SHA_2_256)
                                          .Authorization(TAG_MIN_MAC_LENGTH, min_mac_length)))
                    << "HMAC min mac length " << min_mac_length << " invalid.";
            }
        } else {
            EXPECT_EQ(ErrorCode::OK,
                      GenerateKey(AuthorizationSetBuilder()
                                      .HmacKey(128)
                                      .Digest(Digest::SHA_2_256)
                                      .Authorization(TAG_MIN_MAC_LENGTH, min_mac_length)))
                << "Failed to generate HMAC key with min MAC length " << min_mac_length;
            CheckCharacteristics(key_blob_, key_characteristics_);
            CheckedDeleteKey();
        }
    }
}

/*
 * NewKeyGenerationTest.HmacMultipleDigests
 *
 * Verifies that keymaster rejects HMAC key generation with multiple specified digest algorithms.
 */
TEST_P(NewKeyGenerationTest, HmacMultipleDigests) {
    if (SecLevel() == SecurityLevel::STRONGBOX) return;

    ASSERT_EQ(ErrorCode::UNSUPPORTED_DIGEST,
              GenerateKey(AuthorizationSetBuilder()
                              .HmacKey(128)
                              .Digest(Digest::SHA1)
                              .Digest(Digest::SHA_2_256)
                              .Authorization(TAG_MIN_MAC_LENGTH, 128)));
}

/*
 * NewKeyGenerationTest.HmacDigestNone
 *
 * Verifies that keymaster rejects HMAC key generation with no digest or Digest::NONE
 */
TEST_P(NewKeyGenerationTest, HmacDigestNone) {
    ASSERT_EQ(
        ErrorCode::UNSUPPORTED_DIGEST,
        GenerateKey(AuthorizationSetBuilder().HmacKey(128).Authorization(TAG_MIN_MAC_LENGTH, 128)));

    ASSERT_EQ(ErrorCode::UNSUPPORTED_DIGEST,
              GenerateKey(AuthorizationSetBuilder()
                              .HmacKey(128)
                              .Digest(Digest::NONE)
                              .Authorization(TAG_MIN_MAC_LENGTH, 128)));
}

typedef KeymasterHidlTest SigningOperationsTest;

/*
 * SigningOperationsTest.RsaSuccess
 *
 * Verifies that raw RSA signature operations succeed.
 */
TEST_P(SigningOperationsTest, RsaSuccess) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .RsaSigningKey(2048, 65537)
                                             .Digest(Digest::NONE)
                                             .Padding(PaddingMode::NONE)
                                             .Authorization(TAG_NO_AUTH_REQUIRED)));
    string message = "12345678901234567890123456789012";
    string signature = SignMessage(
        message, AuthorizationSetBuilder().Digest(Digest::NONE).Padding(PaddingMode::NONE));
}

/*
 * SigningOperationsTest.RsaGetKeyCharacteristicsRequiresCorrectAppIdAppData
 *
 * Verifies that getting RSA key characteristics requires the correct app ID/data.
 */
TEST_P(SigningOperationsTest, RsaGetKeyCharacteristicsRequiresCorrectAppIdAppData) {
    HidlBuf key_blob;
    KeyCharacteristics key_characteristics;
    ASSERT_EQ(ErrorCode::OK,
              GenerateKey(AuthorizationSetBuilder()
                                  .Authorization(TAG_NO_AUTH_REQUIRED)
                                  .RsaSigningKey(2048, 65537)
                                  .Digest(Digest::NONE)
                                  .Padding(PaddingMode::NONE)
                                  .Authorization(TAG_APPLICATION_ID, HidlBuf("clientid"))
                                  .Authorization(TAG_APPLICATION_DATA, HidlBuf("appdata")),
                          &key_blob, &key_characteristics));
    CheckGetCharacteristics(key_blob, HidlBuf("clientid"), HidlBuf("appdata"),
                            &key_characteristics);
}

/*
 * SigningOperationsTest.RsaUseRequiresCorrectAppIdAppData
 *
 * Verifies that using an RSA key requires the correct app ID/data.
 */
TEST_P(SigningOperationsTest, RsaUseRequiresCorrectAppIdAppData) {
    ASSERT_EQ(ErrorCode::OK,
              GenerateKey(AuthorizationSetBuilder()
                                  .Authorization(TAG_NO_AUTH_REQUIRED)
                                  .RsaSigningKey(2048, 65537)
                                  .Digest(Digest::NONE)
                                  .Padding(PaddingMode::NONE)
                                  .Authorization(TAG_APPLICATION_ID, HidlBuf("clientid"))
                                  .Authorization(TAG_APPLICATION_DATA, HidlBuf("appdata"))));
    EXPECT_EQ(ErrorCode::INVALID_KEY_BLOB,
              Begin(KeyPurpose::SIGN,
                    AuthorizationSetBuilder().Digest(Digest::NONE).Padding(PaddingMode::NONE)));
    AbortIfNeeded();
    EXPECT_EQ(ErrorCode::INVALID_KEY_BLOB,
              Begin(KeyPurpose::SIGN,
                    AuthorizationSetBuilder()
                            .Digest(Digest::NONE)
                            .Padding(PaddingMode::NONE)
                            .Authorization(TAG_APPLICATION_ID, HidlBuf("clientid"))));
    AbortIfNeeded();
    EXPECT_EQ(ErrorCode::INVALID_KEY_BLOB,
              Begin(KeyPurpose::SIGN,
                    AuthorizationSetBuilder()
                            .Digest(Digest::NONE)
                            .Padding(PaddingMode::NONE)
                            .Authorization(TAG_APPLICATION_DATA, HidlBuf("appdata"))));
    AbortIfNeeded();
    EXPECT_EQ(ErrorCode::OK,
              Begin(KeyPurpose::SIGN,
                    AuthorizationSetBuilder()
                            .Digest(Digest::NONE)
                            .Padding(PaddingMode::NONE)
                            .Authorization(TAG_APPLICATION_DATA, HidlBuf("appdata"))
                            .Authorization(TAG_APPLICATION_ID, HidlBuf("clientid"))));
    AbortIfNeeded();
}

/*
 * SigningOperationsTest.RsaPssSha256Success
 *
 * Verifies that RSA-PSS signature operations succeed.
 */
TEST_P(SigningOperationsTest, RsaPssSha256Success) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .RsaSigningKey(2048, 65537)
                                             .Digest(Digest::SHA_2_256)
                                             .Padding(PaddingMode::RSA_PSS)
                                             .Authorization(TAG_NO_AUTH_REQUIRED)));
    // Use large message, which won't work without digesting.
    string message(1024, 'a');
    string signature = SignMessage(
        message, AuthorizationSetBuilder().Digest(Digest::SHA_2_256).Padding(PaddingMode::RSA_PSS));
}

/*
 * SigningOperationsTest.RsaPaddingNoneDoesNotAllowOther
 *
 * Verifies that keymaster rejects signature operations that specify a padding mode when the key
 * supports only unpadded operations.
 */
TEST_P(SigningOperationsTest, RsaPaddingNoneDoesNotAllowOther) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .RsaSigningKey(2048, 65537)
                                             .Digest(Digest::NONE)
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .Padding(PaddingMode::NONE)));
    string message = "12345678901234567890123456789012";
    string signature;

    EXPECT_EQ(ErrorCode::INCOMPATIBLE_PADDING_MODE,
              Begin(KeyPurpose::SIGN, AuthorizationSetBuilder()
                                          .Digest(Digest::NONE)
                                          .Padding(PaddingMode::RSA_PKCS1_1_5_SIGN)));
}

/*
 * SigningOperationsTest.NoUserConfirmation
 *
 * Verifies that keymaster rejects signing operations for keys with
 * TRUSTED_CONFIRMATION_REQUIRED and no valid confirmation token
 * presented.
 */
TEST_P(SigningOperationsTest, NoUserConfirmation) {
    if (SecLevel() == SecurityLevel::STRONGBOX) return;
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .RsaSigningKey(1024, 65537)
                                             .Digest(Digest::NONE)
                                             .Padding(PaddingMode::NONE)
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .Authorization(TAG_TRUSTED_CONFIRMATION_REQUIRED)));

    const string message = "12345678901234567890123456789012";
    EXPECT_EQ(ErrorCode::OK,
              Begin(KeyPurpose::SIGN,
                    AuthorizationSetBuilder().Digest(Digest::NONE).Padding(PaddingMode::NONE)));
    string signature;
    EXPECT_EQ(ErrorCode::NO_USER_CONFIRMATION, Finish(message, &signature));
}

/*
 * SigningOperationsTest.RsaPkcs1Sha256Success
 *
 * Verifies that digested RSA-PKCS1 signature operations succeed.
 */
TEST_P(SigningOperationsTest, RsaPkcs1Sha256Success) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .RsaSigningKey(2048, 65537)
                                             .Digest(Digest::SHA_2_256)
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .Padding(PaddingMode::RSA_PKCS1_1_5_SIGN)));
    string message(1024, 'a');
    string signature = SignMessage(message, AuthorizationSetBuilder()
                                                .Digest(Digest::SHA_2_256)
                                                .Padding(PaddingMode::RSA_PKCS1_1_5_SIGN));
}

/*
 * SigningOperationsTest.RsaPkcs1NoDigestSuccess
 *
 * Verifies that undigested RSA-PKCS1 signature operations succeed.
 */
TEST_P(SigningOperationsTest, RsaPkcs1NoDigestSuccess) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .RsaSigningKey(2048, 65537)
                                             .Digest(Digest::NONE)
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .Padding(PaddingMode::RSA_PKCS1_1_5_SIGN)));
    string message(53, 'a');
    string signature = SignMessage(
        message,
        AuthorizationSetBuilder().Digest(Digest::NONE).Padding(PaddingMode::RSA_PKCS1_1_5_SIGN));
}

/*
 * SigningOperationsTest.RsaPkcs1NoDigestTooLarge
 *
 * Verifies that undigested RSA-PKCS1 signature operations fail with the correct error code when
 * given a too-long message.
 */
TEST_P(SigningOperationsTest, RsaPkcs1NoDigestTooLong) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .RsaSigningKey(2048, 65537)
                                             .Digest(Digest::NONE)
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .Padding(PaddingMode::RSA_PKCS1_1_5_SIGN)));
    string message(257, 'a');

    EXPECT_EQ(ErrorCode::OK,
              Begin(KeyPurpose::SIGN, AuthorizationSetBuilder()
                                          .Digest(Digest::NONE)
                                          .Padding(PaddingMode::RSA_PKCS1_1_5_SIGN)));
    string signature;
    EXPECT_EQ(ErrorCode::INVALID_INPUT_LENGTH, Finish(message, &signature));
}

/*
 * SigningOperationsTest.RsaPssSha512TooSmallKey
 *
 * Verifies that undigested RSA-PSS signature operations fail with the correct error code when
 * used with a key that is too small for the message.
 *
 * A PSS-padded message is of length salt_size + digest_size + 16 (sizes in bits), and the keymaster
 * specification requires that salt_size == digest_size, so the message will be digest_size * 2 +
 * 16. Such a message can only be signed by a given key if the key is at least that size. This test
 * uses SHA512, which has a digest_size == 512, so the message size is 1040 bits, too large for a
 * 1024-bit key.
 */
TEST_P(SigningOperationsTest, RsaPssSha512TooSmallKey) {
    if (SecLevel() == SecurityLevel::STRONGBOX) return;
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .RsaSigningKey(1024, 65537)
                                             .Digest(Digest::SHA_2_512)
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .Padding(PaddingMode::RSA_PSS)));
    EXPECT_EQ(
        ErrorCode::INCOMPATIBLE_DIGEST,
        Begin(KeyPurpose::SIGN,
              AuthorizationSetBuilder().Digest(Digest::SHA_2_512).Padding(PaddingMode::RSA_PSS)));
}

/*
 * SigningOperationsTest.RsaNoPaddingTooLong
 *
 * Verifies that raw RSA signature operations fail with the correct error code when
 * given a too-long message.
 */
TEST_P(SigningOperationsTest, RsaNoPaddingTooLong) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .RsaSigningKey(2048, 65537)
                                             .Digest(Digest::NONE)
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .Padding(PaddingMode::RSA_PKCS1_1_5_SIGN)));
    // One byte too long
    string message(2048 / 8 + 1, 'a');
    ASSERT_EQ(ErrorCode::OK,
              Begin(KeyPurpose::SIGN, AuthorizationSetBuilder()
                                          .Digest(Digest::NONE)
                                          .Padding(PaddingMode::RSA_PKCS1_1_5_SIGN)));
    string result;
    ErrorCode finish_error_code = Finish(message, &result);
    EXPECT_TRUE(finish_error_code == ErrorCode::INVALID_INPUT_LENGTH ||
                finish_error_code == ErrorCode::INVALID_ARGUMENT);

    // Very large message that should exceed the transfer buffer size of any reasonable TEE.
    message = string(128 * 1024, 'a');
    ASSERT_EQ(ErrorCode::OK,
              Begin(KeyPurpose::SIGN, AuthorizationSetBuilder()
                                          .Digest(Digest::NONE)
                                          .Padding(PaddingMode::RSA_PKCS1_1_5_SIGN)));
    finish_error_code = Finish(message, &result);
    EXPECT_TRUE(finish_error_code == ErrorCode::INVALID_INPUT_LENGTH ||
                finish_error_code == ErrorCode::INVALID_ARGUMENT);
}

/*
 * SigningOperationsTest.RsaAbort
 *
 * Verifies that operations can be aborted correctly.  Uses an RSA signing operation for the test,
 * but the behavior should be algorithm and purpose-independent.
 */
TEST_P(SigningOperationsTest, RsaAbort) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .RsaSigningKey(2048, 65537)
                                             .Digest(Digest::NONE)
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .Padding(PaddingMode::NONE)));

    ASSERT_EQ(ErrorCode::OK,
              Begin(KeyPurpose::SIGN,
                    AuthorizationSetBuilder().Digest(Digest::NONE).Padding(PaddingMode::NONE)));
    EXPECT_EQ(ErrorCode::OK, Abort(op_handle_));

    // Another abort should fail
    EXPECT_EQ(ErrorCode::INVALID_OPERATION_HANDLE, Abort(op_handle_));

    // Set to sentinel, so TearDown() doesn't try to abort again.
    op_handle_ = kOpHandleSentinel;
}

/*
 * SigningOperationsTest.RsaUnsupportedPadding
 *
 * Verifies that RSA operations fail with the correct error (but key gen succeeds) when used with a
 * padding mode inappropriate for RSA.
 */
TEST_P(SigningOperationsTest, RsaUnsupportedPadding) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .RsaSigningKey(2048, 65537)
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .Digest(Digest::SHA_2_256 /* supported digest */)
                                             .Padding(PaddingMode::PKCS7)));
    ASSERT_EQ(
        ErrorCode::UNSUPPORTED_PADDING_MODE,
        Begin(KeyPurpose::SIGN,
              AuthorizationSetBuilder().Digest(Digest::SHA_2_256).Padding(PaddingMode::PKCS7)));
}

/*
 * SigningOperationsTest.RsaPssNoDigest
 *
 * Verifies that RSA PSS operations fail when no digest is used.  PSS requires a digest.
 */
TEST_P(SigningOperationsTest, RsaNoDigest) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .RsaSigningKey(2048, 65537)
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .Digest(Digest::NONE)
                                             .Padding(PaddingMode::RSA_PSS)));
    ASSERT_EQ(ErrorCode::INCOMPATIBLE_DIGEST,
              Begin(KeyPurpose::SIGN,
                    AuthorizationSetBuilder().Digest(Digest::NONE).Padding(PaddingMode::RSA_PSS)));

    ASSERT_EQ(ErrorCode::UNSUPPORTED_DIGEST,
              Begin(KeyPurpose::SIGN, AuthorizationSetBuilder().Padding(PaddingMode::RSA_PSS)));
}

/*
 * SigningOperationsTest.RsaPssNoDigest
 *
 * Verifies that RSA operations fail when no padding mode is specified.  PaddingMode::NONE is
 * supported in some cases (as validated in other tests), but a mode must be specified.
 */
TEST_P(SigningOperationsTest, RsaNoPadding) {
    // Padding must be specified
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .RsaKey(2048, 65537)
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .SigningKey()
                                             .Digest(Digest::NONE)));
    ASSERT_EQ(ErrorCode::UNSUPPORTED_PADDING_MODE,
              Begin(KeyPurpose::SIGN, AuthorizationSetBuilder().Digest(Digest::NONE)));
}

/*
 * SigningOperationsTest.RsaShortMessage
 *
 * Verifies that raw RSA signatures succeed with a message shorter than the key size.
 */
TEST_P(SigningOperationsTest, RsaTooShortMessage) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .RsaSigningKey(2048, 65537)
                                             .Digest(Digest::NONE)
                                             .Padding(PaddingMode::NONE)));

    // Barely shorter
    string message(2048 / 8 - 1, 'a');
    SignMessage(message, AuthorizationSetBuilder().Digest(Digest::NONE).Padding(PaddingMode::NONE));

    // Much shorter
    message = "a";
    SignMessage(message, AuthorizationSetBuilder().Digest(Digest::NONE).Padding(PaddingMode::NONE));
}

/*
 * SigningOperationsTest.RsaSignWithEncryptionKey
 *
 * Verifies that RSA encryption keys cannot be used to sign.
 */
TEST_P(SigningOperationsTest, RsaSignWithEncryptionKey) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .RsaEncryptionKey(2048, 65537)
                                             .Digest(Digest::NONE)
                                             .Padding(PaddingMode::NONE)));
    ASSERT_EQ(ErrorCode::INCOMPATIBLE_PURPOSE,
              Begin(KeyPurpose::SIGN,
                    AuthorizationSetBuilder().Digest(Digest::NONE).Padding(PaddingMode::NONE)));
}

/*
 * SigningOperationsTest.RsaSignTooLargeMessage
 *
 * Verifies that attempting a raw signature of a message which is the same length as the key, but
 * numerically larger than the public modulus, fails with the correct error.
 */
TEST_P(SigningOperationsTest, RsaSignTooLargeMessage) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .RsaSigningKey(2048, 65537)
                                             .Digest(Digest::NONE)
                                             .Padding(PaddingMode::NONE)));

    // Largest possible message will always be larger than the public modulus.
    string message(2048 / 8, static_cast<char>(0xff));
    ASSERT_EQ(ErrorCode::OK, Begin(KeyPurpose::SIGN, AuthorizationSetBuilder()
                                                         .Authorization(TAG_NO_AUTH_REQUIRED)
                                                         .Digest(Digest::NONE)
                                                         .Padding(PaddingMode::NONE)));
    string signature;
    ASSERT_EQ(ErrorCode::INVALID_ARGUMENT, Finish(message, &signature));
}

/*
 * SigningOperationsTest.EcdsaAllSizesAndHashes
 *
 * Verifies that ECDSA operations succeed with all possible key sizes and hashes.
 */
TEST_P(SigningOperationsTest, EcdsaAllSizesAndHashes) {
    for (auto key_size : ValidKeySizes(Algorithm::EC)) {
        for (auto digest : ValidDigests(false /* withNone */, false /* withMD5 */)) {
            ErrorCode error = GenerateKey(AuthorizationSetBuilder()
                                              .Authorization(TAG_NO_AUTH_REQUIRED)
                                              .EcdsaSigningKey(key_size)
                                              .Digest(digest));
            EXPECT_EQ(ErrorCode::OK, error) << "Failed to generate ECDSA key with size " << key_size
                                            << " and digest " << digest;
            if (error != ErrorCode::OK) continue;

            string message(1024, 'a');
            if (digest == Digest::NONE) message.resize(key_size / 8);
            SignMessage(message, AuthorizationSetBuilder().Digest(digest));
            CheckedDeleteKey();
        }
    }
}

/*
 * SigningOperationsTest.EcdsaAllCurves
 *
 * Verifies that ECDSA operations succeed with all possible curves.
 */
TEST_P(SigningOperationsTest, EcdsaAllCurves) {
    for (auto curve : ValidCurves()) {
        ErrorCode error = GenerateKey(AuthorizationSetBuilder()
                                          .Authorization(TAG_NO_AUTH_REQUIRED)
                                          .EcdsaSigningKey(curve)
                                          .Digest(Digest::SHA_2_256));
        EXPECT_EQ(ErrorCode::OK, error) << "Failed to generate ECDSA key with curve " << curve;
        if (error != ErrorCode::OK) continue;

        string message(1024, 'a');
        SignMessage(message, AuthorizationSetBuilder().Digest(Digest::SHA_2_256));
        CheckedDeleteKey();
    }
}

/*
 * SigningOperationsTest.EcdsaNoDigestHugeData
 *
 * Verifies that ECDSA operations support very large messages, even without digesting.  This should
 * work because ECDSA actually only signs the leftmost L_n bits of the message, however large it may
 * be.  Not using digesting is a bad idea, but in some cases digesting is done by the framework.
 */
TEST_P(SigningOperationsTest, EcdsaNoDigestHugeData) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .EcdsaSigningKey(256)
                                             .Digest(Digest::NONE)));
    string message(1 * 1024, 'a');
    SignMessage(message, AuthorizationSetBuilder().Digest(Digest::NONE));
}

/*
 * SigningOperationsTest.EcGetKeyCharacteristicsRequiresCorrectAppIdAppData
 *
 * Verifies that getting EC key characteristics requires the correct app ID/data.
 */
TEST_P(SigningOperationsTest, EcGetKeyCharacteristicsRequiresCorrectAppIdAppData) {
    HidlBuf key_blob;
    KeyCharacteristics key_characteristics;
    ASSERT_EQ(ErrorCode::OK,
              GenerateKey(AuthorizationSetBuilder()
                                  .Authorization(TAG_NO_AUTH_REQUIRED)
                                  .EcdsaSigningKey(256)
                                  .Digest(Digest::NONE)
                                  .Authorization(TAG_APPLICATION_ID, HidlBuf("clientid"))
                                  .Authorization(TAG_APPLICATION_DATA, HidlBuf("appdata")),
                          &key_blob, &key_characteristics));
    CheckGetCharacteristics(key_blob, HidlBuf("clientid"), HidlBuf("appdata"),
                            &key_characteristics);
}

/*
 * SigningOperationsTest.EcUseRequiresCorrectAppIdAppData
 *
 * Verifies that using an EC key requires the correct app ID/data.
 */
TEST_P(SigningOperationsTest, EcUseRequiresCorrectAppIdAppData) {
    ASSERT_EQ(ErrorCode::OK,
              GenerateKey(AuthorizationSetBuilder()
                                  .Authorization(TAG_NO_AUTH_REQUIRED)
                                  .EcdsaSigningKey(256)
                                  .Digest(Digest::NONE)
                                  .Authorization(TAG_APPLICATION_ID, HidlBuf("clientid"))
                                  .Authorization(TAG_APPLICATION_DATA, HidlBuf("appdata"))));
    EXPECT_EQ(ErrorCode::INVALID_KEY_BLOB,
              Begin(KeyPurpose::SIGN, AuthorizationSetBuilder().Digest(Digest::NONE)));
    AbortIfNeeded();
    EXPECT_EQ(ErrorCode::INVALID_KEY_BLOB,
              Begin(KeyPurpose::SIGN,
                    AuthorizationSetBuilder()
                            .Digest(Digest::NONE)
                            .Authorization(TAG_APPLICATION_ID, HidlBuf("clientid"))));
    AbortIfNeeded();
    EXPECT_EQ(ErrorCode::INVALID_KEY_BLOB,
              Begin(KeyPurpose::SIGN,
                    AuthorizationSetBuilder()
                            .Digest(Digest::NONE)
                            .Authorization(TAG_APPLICATION_DATA, HidlBuf("appdata"))));
    AbortIfNeeded();
    EXPECT_EQ(ErrorCode::OK,
              Begin(KeyPurpose::SIGN,
                    AuthorizationSetBuilder()
                            .Digest(Digest::NONE)
                            .Authorization(TAG_APPLICATION_DATA, HidlBuf("appdata"))
                            .Authorization(TAG_APPLICATION_ID, HidlBuf("clientid"))));
    AbortIfNeeded();
}

/*
 * SigningOperationsTest.AesEcbSign
 *
 * Verifies that attempts to use AES keys to sign fail in the correct way.
 */
TEST_P(SigningOperationsTest, AesEcbSign) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .SigningKey()
                                             .AesEncryptionKey(128)
                                             .Authorization(TAG_BLOCK_MODE, BlockMode::ECB)));

    AuthorizationSet out_params;
    EXPECT_EQ(ErrorCode::UNSUPPORTED_PURPOSE,
              Begin(KeyPurpose::SIGN, AuthorizationSet() /* in_params */, &out_params));
    EXPECT_EQ(ErrorCode::UNSUPPORTED_PURPOSE,
              Begin(KeyPurpose::VERIFY, AuthorizationSet() /* in_params */, &out_params));
}

/*
 * SigningOperationsTest.HmacAllDigests
 *
 * Verifies that HMAC works with all digests.
 */
TEST_P(SigningOperationsTest, HmacAllDigests) {
    for (auto digest : ValidDigests(false /* withNone */, false /* withMD5 */)) {
        ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .HmacKey(128)
                                                 .Digest(digest)
                                                 .Authorization(TAG_MIN_MAC_LENGTH, 160)))
            << "Failed to create HMAC key with digest " << digest;
        string message = "12345678901234567890123456789012";
        string signature = MacMessage(message, digest, 160);
        EXPECT_EQ(160U / 8U, signature.size())
            << "Failed to sign with HMAC key with digest " << digest;
        CheckedDeleteKey();
    }
}

/*
 * SigningOperationsTest.HmacSha256TooLargeMacLength
 *
 * Verifies that HMAC fails in the correct way when asked to generate a MAC larger than the digest
 * size.
 */
TEST_P(SigningOperationsTest, HmacSha256TooLargeMacLength) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .HmacKey(128)
                                             .Digest(Digest::SHA_2_256)
                                             .Authorization(TAG_MIN_MAC_LENGTH, 256)));
    AuthorizationSet output_params;
    EXPECT_EQ(
        ErrorCode::UNSUPPORTED_MAC_LENGTH,
        Begin(
            KeyPurpose::SIGN, key_blob_,
            AuthorizationSetBuilder().Digest(Digest::SHA_2_256).Authorization(TAG_MAC_LENGTH, 264),
            &output_params, &op_handle_));
}

/*
 * SigningOperationsTest.HmacSha256TooSmallMacLength
 *
 * Verifies that HMAC fails in the correct way when asked to generate a MAC smaller than the
 * specified minimum MAC length.
 */
TEST_P(SigningOperationsTest, HmacSha256TooSmallMacLength) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .HmacKey(128)
                                             .Digest(Digest::SHA_2_256)
                                             .Authorization(TAG_MIN_MAC_LENGTH, 128)));
    AuthorizationSet output_params;
    EXPECT_EQ(
        ErrorCode::INVALID_MAC_LENGTH,
        Begin(
            KeyPurpose::SIGN, key_blob_,
            AuthorizationSetBuilder().Digest(Digest::SHA_2_256).Authorization(TAG_MAC_LENGTH, 120),
            &output_params, &op_handle_));
}

/*
 * SigningOperationsTest.HmacRfc4231TestCase3
 *
 * Validates against the test vectors from RFC 4231 test case 3.
 */
TEST_P(SigningOperationsTest, HmacRfc4231TestCase3) {
    string key(20, 0xaa);
    string message(50, 0xdd);
    uint8_t sha_224_expected[] = {
        0x7f, 0xb3, 0xcb, 0x35, 0x88, 0xc6, 0xc1, 0xf6, 0xff, 0xa9, 0x69, 0x4d, 0x7d, 0x6a,
        0xd2, 0x64, 0x93, 0x65, 0xb0, 0xc1, 0xf6, 0x5d, 0x69, 0xd1, 0xec, 0x83, 0x33, 0xea,
    };
    uint8_t sha_256_expected[] = {
        0x77, 0x3e, 0xa9, 0x1e, 0x36, 0x80, 0x0e, 0x46, 0x85, 0x4d, 0xb8,
        0xeb, 0xd0, 0x91, 0x81, 0xa7, 0x29, 0x59, 0x09, 0x8b, 0x3e, 0xf8,
        0xc1, 0x22, 0xd9, 0x63, 0x55, 0x14, 0xce, 0xd5, 0x65, 0xfe,
    };
    uint8_t sha_384_expected[] = {
        0x88, 0x06, 0x26, 0x08, 0xd3, 0xe6, 0xad, 0x8a, 0x0a, 0xa2, 0xac, 0xe0,
        0x14, 0xc8, 0xa8, 0x6f, 0x0a, 0xa6, 0x35, 0xd9, 0x47, 0xac, 0x9f, 0xeb,
        0xe8, 0x3e, 0xf4, 0xe5, 0x59, 0x66, 0x14, 0x4b, 0x2a, 0x5a, 0xb3, 0x9d,
        0xc1, 0x38, 0x14, 0xb9, 0x4e, 0x3a, 0xb6, 0xe1, 0x01, 0xa3, 0x4f, 0x27,
    };
    uint8_t sha_512_expected[] = {
        0xfa, 0x73, 0xb0, 0x08, 0x9d, 0x56, 0xa2, 0x84, 0xef, 0xb0, 0xf0, 0x75, 0x6c,
        0x89, 0x0b, 0xe9, 0xb1, 0xb5, 0xdb, 0xdd, 0x8e, 0xe8, 0x1a, 0x36, 0x55, 0xf8,
        0x3e, 0x33, 0xb2, 0x27, 0x9d, 0x39, 0xbf, 0x3e, 0x84, 0x82, 0x79, 0xa7, 0x22,
        0xc8, 0x06, 0xb4, 0x85, 0xa4, 0x7e, 0x67, 0xc8, 0x07, 0xb9, 0x46, 0xa3, 0x37,
        0xbe, 0xe8, 0x94, 0x26, 0x74, 0x27, 0x88, 0x59, 0xe1, 0x32, 0x92, 0xfb,
    };

    CheckHmacTestVector(key, message, Digest::SHA_2_256, make_string(sha_256_expected));
    if (SecLevel() != SecurityLevel::STRONGBOX) {
        CheckHmacTestVector(key, message, Digest::SHA_2_224, make_string(sha_224_expected));
        CheckHmacTestVector(key, message, Digest::SHA_2_384, make_string(sha_384_expected));
        CheckHmacTestVector(key, message, Digest::SHA_2_512, make_string(sha_512_expected));
    }
}

/*
 * SigningOperationsTest.HmacRfc4231TestCase5
 *
 * Validates against the test vectors from RFC 4231 test case 5.
 */
TEST_P(SigningOperationsTest, HmacRfc4231TestCase5) {
    string key(20, 0x0c);
    string message = "Test With Truncation";

    uint8_t sha_224_expected[] = {
        0x0e, 0x2a, 0xea, 0x68, 0xa9, 0x0c, 0x8d, 0x37,
        0xc9, 0x88, 0xbc, 0xdb, 0x9f, 0xca, 0x6f, 0xa8,
    };
    uint8_t sha_256_expected[] = {
        0xa3, 0xb6, 0x16, 0x74, 0x73, 0x10, 0x0e, 0xe0,
        0x6e, 0x0c, 0x79, 0x6c, 0x29, 0x55, 0x55, 0x2b,
    };
    uint8_t sha_384_expected[] = {
        0x3a, 0xbf, 0x34, 0xc3, 0x50, 0x3b, 0x2a, 0x23,
        0xa4, 0x6e, 0xfc, 0x61, 0x9b, 0xae, 0xf8, 0x97,
    };
    uint8_t sha_512_expected[] = {
        0x41, 0x5f, 0xad, 0x62, 0x71, 0x58, 0x0a, 0x53,
        0x1d, 0x41, 0x79, 0xbc, 0x89, 0x1d, 0x87, 0xa6,
    };

    CheckHmacTestVector(key, message, Digest::SHA_2_256, make_string(sha_256_expected));
    if (SecLevel() != SecurityLevel::STRONGBOX) {
        CheckHmacTestVector(key, message, Digest::SHA_2_224, make_string(sha_224_expected));
        CheckHmacTestVector(key, message, Digest::SHA_2_384, make_string(sha_384_expected));
        CheckHmacTestVector(key, message, Digest::SHA_2_512, make_string(sha_512_expected));
    }
}

typedef KeymasterHidlTest VerificationOperationsTest;

/*
 * VerificationOperationsTest.RsaSuccess
 *
 * Verifies that a simple RSA signature/verification sequence succeeds.
 */
TEST_P(VerificationOperationsTest, RsaSuccess) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .RsaSigningKey(2048, 65537)
                                             .Digest(Digest::NONE)
                                             .Padding(PaddingMode::NONE)));
    string message = "12345678901234567890123456789012";
    string signature = SignMessage(
        message, AuthorizationSetBuilder().Digest(Digest::NONE).Padding(PaddingMode::NONE));
    VerifyMessage(message, signature,
                  AuthorizationSetBuilder().Digest(Digest::NONE).Padding(PaddingMode::NONE));
}

/*
 * VerificationOperationsTest.RsaSuccess
 *
 * Verifies RSA signature/verification for all padding modes and digests.
 */
TEST_P(VerificationOperationsTest, RsaAllPaddingsAndDigests) {
    auto authorizations = AuthorizationSetBuilder()
                              .Authorization(TAG_NO_AUTH_REQUIRED)
                              .RsaSigningKey(2048, 65537)
                              .Digest(ValidDigests(true /* withNone */, true /* withMD5 */))
                              .Padding(PaddingMode::NONE)
                              .Padding(PaddingMode::RSA_PSS)
                              .Padding(PaddingMode::RSA_PKCS1_1_5_SIGN);

    ASSERT_EQ(ErrorCode::OK, GenerateKey(authorizations));

    string message(128, 'a');
    string corrupt_message(message);
    ++corrupt_message[corrupt_message.size() / 2];

    for (auto padding :
         {PaddingMode::NONE, PaddingMode::RSA_PSS, PaddingMode::RSA_PKCS1_1_5_SIGN}) {
        for (auto digest : ValidDigests(true /* withNone */, true /* withMD5 */)) {
            if (padding == PaddingMode::NONE && digest != Digest::NONE) {
                // Digesting only makes sense with padding.
                continue;
            }

            if (padding == PaddingMode::RSA_PSS && digest == Digest::NONE) {
                // PSS requires digesting.
                continue;
            }

            string signature =
                SignMessage(message, AuthorizationSetBuilder().Digest(digest).Padding(padding));
            VerifyMessage(message, signature,
                          AuthorizationSetBuilder().Digest(digest).Padding(padding));

            if (digest != Digest::NONE) {
                // Verify with OpenSSL.
                HidlBuf pubkey;
                ASSERT_EQ(ErrorCode::OK, ExportKey(KeyFormat::X509, &pubkey));

                const uint8_t* p = pubkey.data();
                EVP_PKEY_Ptr pkey(d2i_PUBKEY(nullptr /* alloc new */, &p, pubkey.size()));
                ASSERT_TRUE(pkey.get());

                EVP_MD_CTX digest_ctx;
                EVP_MD_CTX_init(&digest_ctx);
                EVP_PKEY_CTX* pkey_ctx;
                const EVP_MD* md = openssl_digest(digest);
                ASSERT_NE(md, nullptr);
                EXPECT_EQ(1, EVP_DigestVerifyInit(&digest_ctx, &pkey_ctx, md, nullptr /* engine */,
                                                  pkey.get()));

                switch (padding) {
                    case PaddingMode::RSA_PSS:
                        EXPECT_GT(EVP_PKEY_CTX_set_rsa_padding(pkey_ctx, RSA_PKCS1_PSS_PADDING), 0);
                        EXPECT_GT(EVP_PKEY_CTX_set_rsa_pss_saltlen(pkey_ctx, EVP_MD_size(md)), 0);
                        break;
                    case PaddingMode::RSA_PKCS1_1_5_SIGN:
                        // PKCS1 is the default; don't need to set anything.
                        break;
                    default:
                        FAIL();
                        break;
                }

                EXPECT_EQ(1, EVP_DigestVerifyUpdate(&digest_ctx, message.data(), message.size()));
                EXPECT_EQ(1, EVP_DigestVerifyFinal(
                                 &digest_ctx, reinterpret_cast<const uint8_t*>(signature.data()),
                                 signature.size()));
                EVP_MD_CTX_cleanup(&digest_ctx);
            }

            // Corrupt signature shouldn't verify.
            string corrupt_signature(signature);
            ++corrupt_signature[corrupt_signature.size() / 2];

            EXPECT_EQ(ErrorCode::OK,
                      Begin(KeyPurpose::VERIFY,
                            AuthorizationSetBuilder().Digest(digest).Padding(padding)));
            string result;
            EXPECT_EQ(ErrorCode::VERIFICATION_FAILED, Finish(message, corrupt_signature, &result));

            // Corrupt message shouldn't verify
            EXPECT_EQ(ErrorCode::OK,
                      Begin(KeyPurpose::VERIFY,
                            AuthorizationSetBuilder().Digest(digest).Padding(padding)));
            EXPECT_EQ(ErrorCode::VERIFICATION_FAILED, Finish(corrupt_message, signature, &result));
        }
    }
}

/*
 * VerificationOperationsTest.RsaSuccess
 *
 * Verifies ECDSA signature/verification for all digests and curves.
 */
TEST_P(VerificationOperationsTest, EcdsaAllDigestsAndCurves) {
    auto digests = ValidDigests(true /* withNone */, false /* withMD5 */);

    string message = "1234567890";
    string corrupt_message = "2234567890";
    for (auto curve : ValidCurves()) {
        ErrorCode error = GenerateKey(AuthorizationSetBuilder()
                                          .Authorization(TAG_NO_AUTH_REQUIRED)
                                          .EcdsaSigningKey(curve)
                                          .Digest(digests));
        EXPECT_EQ(ErrorCode::OK, error) << "Failed to generate key for EC curve " << curve;
        if (error != ErrorCode::OK) {
            continue;
        }

        for (auto digest : digests) {
            string signature = SignMessage(message, AuthorizationSetBuilder().Digest(digest));
            VerifyMessage(message, signature, AuthorizationSetBuilder().Digest(digest));

            // Verify with OpenSSL
            if (digest != Digest::NONE) {
                HidlBuf pubkey;
                ASSERT_EQ(ErrorCode::OK, ExportKey(KeyFormat::X509, &pubkey))
                    << curve << ' ' << digest;

                const uint8_t* p = pubkey.data();
                EVP_PKEY_Ptr pkey(d2i_PUBKEY(nullptr /* alloc new */, &p, pubkey.size()));
                ASSERT_TRUE(pkey.get());

                EVP_MD_CTX digest_ctx;
                EVP_MD_CTX_init(&digest_ctx);
                EVP_PKEY_CTX* pkey_ctx;
                const EVP_MD* md = openssl_digest(digest);

                EXPECT_EQ(1, EVP_DigestVerifyInit(&digest_ctx, &pkey_ctx, md, nullptr /* engine */,
                                                  pkey.get()))
                    << curve << ' ' << digest;

                EXPECT_EQ(1, EVP_DigestVerifyUpdate(&digest_ctx, message.data(), message.size()))
                    << curve << ' ' << digest;

                EXPECT_EQ(1, EVP_DigestVerifyFinal(
                                 &digest_ctx, reinterpret_cast<const uint8_t*>(signature.data()),
                                 signature.size()))
                    << curve << ' ' << digest;

                EVP_MD_CTX_cleanup(&digest_ctx);
            }

            // Corrupt signature shouldn't verify.
            string corrupt_signature(signature);
            ++corrupt_signature[corrupt_signature.size() / 2];

            EXPECT_EQ(ErrorCode::OK,
                      Begin(KeyPurpose::VERIFY, AuthorizationSetBuilder().Digest(digest)))
                << curve << ' ' << digest;

            string result;
            EXPECT_EQ(ErrorCode::VERIFICATION_FAILED, Finish(message, corrupt_signature, &result))
                << curve << ' ' << digest;

            // Corrupt message shouldn't verify
            EXPECT_EQ(ErrorCode::OK,
                      Begin(KeyPurpose::VERIFY, AuthorizationSetBuilder().Digest(digest)))
                << curve << ' ' << digest;

            EXPECT_EQ(ErrorCode::VERIFICATION_FAILED, Finish(corrupt_message, signature, &result))
                << curve << ' ' << digest;
        }

        auto rc = DeleteKey();
        ASSERT_TRUE(rc == ErrorCode::OK || rc == ErrorCode::UNIMPLEMENTED);
    }
}

/*
 * VerificationOperationsTest.HmacSigningKeyCannotVerify
 *
 * Verifies HMAC signing and verification, but that a signing key cannot be used to verify.
 */
TEST_P(VerificationOperationsTest, HmacSigningKeyCannotVerify) {
    string key_material = "HelloThisIsAKey";

    HidlBuf signing_key, verification_key;
    KeyCharacteristics signing_key_chars, verification_key_chars;
    EXPECT_EQ(ErrorCode::OK,
              ImportKey(AuthorizationSetBuilder()
                            .Authorization(TAG_NO_AUTH_REQUIRED)
                            .Authorization(TAG_ALGORITHM, Algorithm::HMAC)
                            .Authorization(TAG_PURPOSE, KeyPurpose::SIGN)
                            .Digest(Digest::SHA_2_256)
                            .Authorization(TAG_MIN_MAC_LENGTH, 160),
                        KeyFormat::RAW, key_material, &signing_key, &signing_key_chars));
    EXPECT_EQ(ErrorCode::OK,
              ImportKey(AuthorizationSetBuilder()
                            .Authorization(TAG_NO_AUTH_REQUIRED)
                            .Authorization(TAG_ALGORITHM, Algorithm::HMAC)
                            .Authorization(TAG_PURPOSE, KeyPurpose::VERIFY)
                            .Digest(Digest::SHA_2_256)
                            .Authorization(TAG_MIN_MAC_LENGTH, 160),
                        KeyFormat::RAW, key_material, &verification_key, &verification_key_chars));

    string message = "This is a message.";
    string signature = SignMessage(
        signing_key, message,
        AuthorizationSetBuilder().Digest(Digest::SHA_2_256).Authorization(TAG_MAC_LENGTH, 160));

    // Signing key should not work.
    AuthorizationSet out_params;
    EXPECT_EQ(ErrorCode::INCOMPATIBLE_PURPOSE,
              Begin(KeyPurpose::VERIFY, signing_key, AuthorizationSetBuilder().Digest(Digest::SHA_2_256),
                    &out_params, &op_handle_));

    // Verification key should work.
    VerifyMessage(verification_key, message, signature,
                  AuthorizationSetBuilder().Digest(Digest::SHA_2_256));

    CheckedDeleteKey(&signing_key);
    CheckedDeleteKey(&verification_key);
}

typedef KeymasterHidlTest ExportKeyTest;

/*
 * ExportKeyTest.RsaUnsupportedKeyFormat
 *
 * Verifies that attempting to export RSA keys in PKCS#8 format fails with the correct error.
 */
TEST_P(ExportKeyTest, RsaUnsupportedKeyFormat) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .RsaSigningKey(2048, 65537)
                                             .Digest(Digest::NONE)
                                             .Padding(PaddingMode::NONE)));
    HidlBuf export_data;
    ASSERT_EQ(ErrorCode::UNSUPPORTED_KEY_FORMAT, ExportKey(KeyFormat::PKCS8, &export_data));
}

/*
 * ExportKeyTest.RsaCorruptedKeyBlob
 *
 * Verifies that attempting to export RSA keys from corrupted key blobs fails.  This is essentially
 * a poor-man's key blob fuzzer.
 */
TEST_P(ExportKeyTest, RsaCorruptedKeyBlob) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .RsaSigningKey(2048, 65537)
                                             .Digest(Digest::NONE)
                                             .Padding(PaddingMode::NONE)));
    for (size_t i = 0; i < key_blob_.size(); ++i) {
        HidlBuf corrupted(key_blob_);
        ++corrupted[i];

        HidlBuf export_data;
        EXPECT_EQ(ErrorCode::INVALID_KEY_BLOB,
                  ExportKey(KeyFormat::X509, corrupted, HidlBuf(), HidlBuf(), &export_data))
            << "Blob corrupted at offset " << i << " erroneously accepted as valid";
    }
}

/*
 * ExportKeyTest.RsaCorruptedKeyBlob
 *
 * Verifies that attempting to export ECDSA keys from corrupted key blobs fails.  This is
 * essentially a poor-man's key blob fuzzer.
 */
TEST_P(ExportKeyTest, EcCorruptedKeyBlob) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .EcdsaSigningKey(EcCurve::P_256)
                                             .Digest(Digest::NONE)));
    for (size_t i = 0; i < key_blob_.size(); ++i) {
        HidlBuf corrupted(key_blob_);
        ++corrupted[i];

        HidlBuf export_data;
        EXPECT_EQ(ErrorCode::INVALID_KEY_BLOB,
                  ExportKey(KeyFormat::X509, corrupted, HidlBuf(), HidlBuf(), &export_data))
            << "Blob corrupted at offset " << i << " erroneously accepted as valid";
    }
}

/*
 * ExportKeyTest.AesKeyUnexportable
 *
 * Verifies that attempting to export AES keys fails in the expected way.
 */
TEST_P(ExportKeyTest, AesKeyUnexportable) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .AesEncryptionKey(128)
                                             .EcbMode()
                                             .Padding(PaddingMode::NONE)));

    HidlBuf export_data;
    EXPECT_EQ(ErrorCode::UNSUPPORTED_KEY_FORMAT, ExportKey(KeyFormat::X509, &export_data));
    EXPECT_EQ(ErrorCode::UNSUPPORTED_KEY_FORMAT, ExportKey(KeyFormat::PKCS8, &export_data));
    EXPECT_EQ(ErrorCode::UNSUPPORTED_KEY_FORMAT, ExportKey(KeyFormat::RAW, &export_data));
}

class ImportKeyTest : public KeymasterHidlTest {
   public:
    template <TagType tag_type, Tag tag, typename ValueT>
    void CheckCryptoParam(TypedTag<tag_type, tag> ttag, ValueT expected) {
        SCOPED_TRACE("CheckCryptoParam");
        if (IsSecure()) {
            EXPECT_TRUE(contains(key_characteristics_.hardwareEnforced, ttag, expected))
                << "Tag " << tag << " with value " << expected << " not found";
            EXPECT_FALSE(contains(key_characteristics_.softwareEnforced, ttag))
                << "Tag " << tag << " found";
        } else {
            EXPECT_TRUE(contains(key_characteristics_.softwareEnforced, ttag, expected))
                << "Tag " << tag << " with value " << expected << " not found";
            EXPECT_FALSE(contains(key_characteristics_.hardwareEnforced, ttag))
                << "Tag " << tag << " found";
        }
    }

    void CheckOrigin() {
        SCOPED_TRACE("CheckOrigin");
        if (IsSecure()) {
            EXPECT_TRUE(
                contains(key_characteristics_.hardwareEnforced, TAG_ORIGIN, KeyOrigin::IMPORTED));
        } else {
            EXPECT_TRUE(
                contains(key_characteristics_.softwareEnforced, TAG_ORIGIN, KeyOrigin::IMPORTED));
        }
    }
};

/*
 * ImportKeyTest.RsaSuccess
 *
 * Verifies that importing and using an RSA key pair works correctly.
 */
TEST_P(ImportKeyTest, RsaSuccess) {
    ASSERT_EQ(ErrorCode::OK, ImportKey(AuthorizationSetBuilder()
                                           .Authorization(TAG_NO_AUTH_REQUIRED)
                                           .RsaSigningKey(1024, 65537)
                                           .Digest(Digest::SHA_2_256)
                                           .Padding(PaddingMode::RSA_PSS),
                                       KeyFormat::PKCS8, rsa_key));

    CheckCryptoParam(TAG_ALGORITHM, Algorithm::RSA);
    CheckCryptoParam(TAG_KEY_SIZE, 1024U);
    CheckCryptoParam(TAG_RSA_PUBLIC_EXPONENT, 65537U);
    CheckCryptoParam(TAG_DIGEST, Digest::SHA_2_256);
    CheckCryptoParam(TAG_PADDING, PaddingMode::RSA_PSS);
    CheckOrigin();

    string message(1024 / 8, 'a');
    auto params = AuthorizationSetBuilder().Digest(Digest::SHA_2_256).Padding(PaddingMode::RSA_PSS);
    string signature = SignMessage(message, params);
    VerifyMessage(message, signature, params);
}

/*
 * ImportKeyTest.RsaKeySizeMismatch
 *
 * Verifies that importing an RSA key pair with a size that doesn't match the key fails in the
 * correct way.
 */
TEST_P(ImportKeyTest, RsaKeySizeMismatch) {
    ASSERT_EQ(ErrorCode::IMPORT_PARAMETER_MISMATCH,
              ImportKey(AuthorizationSetBuilder()
                            .RsaSigningKey(2048 /* Doesn't match key */, 65537)
                            .Digest(Digest::NONE)
                            .Padding(PaddingMode::NONE),
                        KeyFormat::PKCS8, rsa_key));
}

/*
 * ImportKeyTest.RsaPublicExponentMismatch
 *
 * Verifies that importing an RSA key pair with a public exponent that doesn't match the key fails
 * in the correct way.
 */
TEST_P(ImportKeyTest, RsaPublicExponentMismatch) {
    ASSERT_EQ(ErrorCode::IMPORT_PARAMETER_MISMATCH,
              ImportKey(AuthorizationSetBuilder()
                            .RsaSigningKey(1024, 3 /* Doesn't match key */)
                            .Digest(Digest::NONE)
                            .Padding(PaddingMode::NONE),
                        KeyFormat::PKCS8, rsa_key));
}

/*
 * ImportKeyTest.EcdsaSuccess
 *
 * Verifies that importing and using an ECDSA P-256 key pair works correctly.
 */
TEST_P(ImportKeyTest, EcdsaSuccess) {
    ASSERT_EQ(ErrorCode::OK, ImportKey(AuthorizationSetBuilder()
                                           .Authorization(TAG_NO_AUTH_REQUIRED)
                                           .EcdsaSigningKey(256)
                                           .Digest(Digest::SHA_2_256),
                                       KeyFormat::PKCS8, ec_256_key));

    CheckCryptoParam(TAG_ALGORITHM, Algorithm::EC);
    CheckCryptoParam(TAG_KEY_SIZE, 256U);
    CheckCryptoParam(TAG_DIGEST, Digest::SHA_2_256);
    CheckCryptoParam(TAG_EC_CURVE, EcCurve::P_256);

    CheckOrigin();

    string message(32, 'a');
    auto params = AuthorizationSetBuilder().Digest(Digest::SHA_2_256);
    string signature = SignMessage(message, params);
    VerifyMessage(message, signature, params);
}

/*
 * ImportKeyTest.EcdsaP256RFC5915Success
 *
 * Verifies that importing and using an ECDSA P-256 key pair encoded using RFC5915 works correctly.
 */
TEST_P(ImportKeyTest, EcdsaP256RFC5915Success) {
    ASSERT_EQ(ErrorCode::OK, ImportKey(AuthorizationSetBuilder()
                                               .Authorization(TAG_NO_AUTH_REQUIRED)
                                               .EcdsaSigningKey(256)
                                               .Digest(Digest::SHA_2_256),
                                       KeyFormat::PKCS8, ec_256_key_rfc5915));

    CheckCryptoParam(TAG_ALGORITHM, Algorithm::EC);
    CheckCryptoParam(TAG_KEY_SIZE, 256U);
    CheckCryptoParam(TAG_DIGEST, Digest::SHA_2_256);
    CheckCryptoParam(TAG_EC_CURVE, EcCurve::P_256);

    CheckOrigin();

    string message(32, 'a');
    auto params = AuthorizationSetBuilder().Digest(Digest::SHA_2_256);
    string signature = SignMessage(message, params);
    VerifyMessage(message, signature, params);
}

/*
 * ImportKeyTest.EcdsaP256SEC1Success
 *
 * Verifies that importing and using an ECDSA P-256 key pair encoded using SEC1 works correctly.
 */
TEST_P(ImportKeyTest, EcdsaP256SEC1Success) {
    ASSERT_EQ(ErrorCode::OK, ImportKey(AuthorizationSetBuilder()
                                               .Authorization(TAG_NO_AUTH_REQUIRED)
                                               .EcdsaSigningKey(256)
                                               .Digest(Digest::SHA_2_256),
                                       KeyFormat::PKCS8, ec_256_key_sec1));

    CheckCryptoParam(TAG_ALGORITHM, Algorithm::EC);
    CheckCryptoParam(TAG_KEY_SIZE, 256U);
    CheckCryptoParam(TAG_DIGEST, Digest::SHA_2_256);
    CheckCryptoParam(TAG_EC_CURVE, EcCurve::P_256);

    CheckOrigin();

    string message(32, 'a');
    auto params = AuthorizationSetBuilder().Digest(Digest::SHA_2_256);
    string signature = SignMessage(message, params);
    VerifyMessage(message, signature, params);
}

/*
 * ImportKeyTest.Ecdsa521Success
 *
 * Verifies that importing and using an ECDSA P-521 key pair works correctly.
 */
TEST_P(ImportKeyTest, Ecdsa521Success) {
    if (SecLevel() == SecurityLevel::STRONGBOX) return;
    ASSERT_EQ(ErrorCode::OK, ImportKey(AuthorizationSetBuilder()
                                           .Authorization(TAG_NO_AUTH_REQUIRED)
                                           .EcdsaSigningKey(521)
                                           .Digest(Digest::SHA_2_256),
                                       KeyFormat::PKCS8, ec_521_key));

    CheckCryptoParam(TAG_ALGORITHM, Algorithm::EC);
    CheckCryptoParam(TAG_KEY_SIZE, 521U);
    CheckCryptoParam(TAG_DIGEST, Digest::SHA_2_256);
    CheckCryptoParam(TAG_EC_CURVE, EcCurve::P_521);
    CheckOrigin();

    string message(32, 'a');
    auto params = AuthorizationSetBuilder().Digest(Digest::SHA_2_256);
    string signature = SignMessage(message, params);
    VerifyMessage(message, signature, params);
}

/*
 * ImportKeyTest.EcdsaSizeMismatch
 *
 * Verifies that importing an ECDSA key pair with a size that doesn't match the key fails in the
 * correct way.
 */
TEST_P(ImportKeyTest, EcdsaSizeMismatch) {
    ASSERT_EQ(ErrorCode::IMPORT_PARAMETER_MISMATCH,
              ImportKey(AuthorizationSetBuilder()
                            .EcdsaSigningKey(224 /* Doesn't match key */)
                            .Digest(Digest::NONE),
                        KeyFormat::PKCS8, ec_256_key));
}

/*
 * ImportKeyTest.EcdsaCurveMismatch
 *
 * Verifies that importing an ECDSA key pair with a curve that doesn't match the key fails in the
 * correct way.
 */
TEST_P(ImportKeyTest, EcdsaCurveMismatch) {
    ASSERT_EQ(ErrorCode::IMPORT_PARAMETER_MISMATCH,
              ImportKey(AuthorizationSetBuilder()
                            .EcdsaSigningKey(EcCurve::P_224 /* Doesn't match key */)
                            .Digest(Digest::NONE),
                        KeyFormat::PKCS8, ec_256_key));
}

/*
 * ImportKeyTest.AesSuccess
 *
 * Verifies that importing and using an AES key works.
 */
TEST_P(ImportKeyTest, AesSuccess) {
    string key = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    ASSERT_EQ(ErrorCode::OK, ImportKey(AuthorizationSetBuilder()
                                           .Authorization(TAG_NO_AUTH_REQUIRED)
                                           .AesEncryptionKey(key.size() * 8)
                                           .EcbMode()
                                           .Padding(PaddingMode::PKCS7),
                                       KeyFormat::RAW, key));

    CheckCryptoParam(TAG_ALGORITHM, Algorithm::AES);
    CheckCryptoParam(TAG_KEY_SIZE, 128U);
    CheckCryptoParam(TAG_PADDING, PaddingMode::PKCS7);
    CheckCryptoParam(TAG_BLOCK_MODE, BlockMode::ECB);
    CheckOrigin();

    string message = "Hello World!";
    auto params = AuthorizationSetBuilder().BlockMode(BlockMode::ECB).Padding(PaddingMode::PKCS7);
    string ciphertext = EncryptMessage(message, params);
    string plaintext = DecryptMessage(ciphertext, params);
    EXPECT_EQ(message, plaintext);
}

/*
 * ImportKeyTest.AesSuccess
 *
 * Verifies that importing and using an HMAC key works.
 */
TEST_P(ImportKeyTest, HmacKeySuccess) {
    string key = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    ASSERT_EQ(ErrorCode::OK, ImportKey(AuthorizationSetBuilder()
                                           .Authorization(TAG_NO_AUTH_REQUIRED)
                                           .HmacKey(key.size() * 8)
                                           .Digest(Digest::SHA_2_256)
                                           .Authorization(TAG_MIN_MAC_LENGTH, 256),
                                       KeyFormat::RAW, key));

    CheckCryptoParam(TAG_ALGORITHM, Algorithm::HMAC);
    CheckCryptoParam(TAG_KEY_SIZE, 128U);
    CheckCryptoParam(TAG_DIGEST, Digest::SHA_2_256);
    CheckOrigin();

    string message = "Hello World!";
    string signature = MacMessage(message, Digest::SHA_2_256, 256);
    VerifyMessage(message, signature, AuthorizationSetBuilder().Digest(Digest::SHA_2_256));
}

auto wrapped_key = hex2str(
    "3082017902010004820100934bf94e2aa28a3f83c9f79297250262fbe3276b5a1c91159bbfa3ef8957aac84b59b30b"
    "455a79c2973480823d8b3863c3deef4a8e243590268d80e18751a0e130f67ce6a1ace9f79b95e097474febc981195b"
    "1d13a69086c0863f66a7b7fdb48792227b1ac5e2489febdf087ab5486483033a6f001ca5d1ec1e27f5c30f4cec2642"
    "074a39ae68aee552e196627a8e3d867e67a8c01b11e75f13cca0a97ab668b50cda07a8ecb7cd8e3dd7009c9636534f"
    "6f239cffe1fc8daa466f78b676c7119efb96bce4e69ca2a25d0b34ed9c3ff999b801597d5220e307eaa5bee507fb94"
    "d1fa69f9e519b2de315bac92c36f2ea1fa1df4478c0ddedeae8c70e0233cd098040cd796b02c370f1fa4cc0124f130"
    "2e0201033029a1083106020100020101a203020120a30402020100a4053103020101a6053103020140bf8377020500"
    "0420ccd540855f833a5e1480bfd2d36faf3aeee15df5beabe2691bc82dde2a7aa910041064c9f689c60ff6223ab6e6"
    "999e0eb6e5");

auto wrapped_key_masked = hex2str(
    "3082017902010004820100aad93ed5924f283b4bb5526fbe7a1412f9d9749ec30db9062b29e574a8546f33c8873245"
    "2f5b8e6a391ee76c39ed1712c61d8df6213dec1cffbc17a8c6d04c7b30893d8daa9b2015213e21946821553207f8f9"
    "931c4caba23ed3bee28b36947e47f10e0a5c3dc51c988a628daad3e5e1f4005e79c2d5a96c284b4b8d7e4948f331e5"
    "b85dd5a236f85579f3ea1d1b848487470bdb0ab4f81a12bee42c99fe0df4bee3759453e69ad1d68a809ce06b949f76"
    "94a990429b2fe81e066ff43e56a21602db70757922a4bcc23ab89f1e35da77586775f423e519c2ea394caf48a28d0c"
    "8020f1dcf6b3a68ec246f615ae96dae9a079b1f6eb959033c1af5c125fd94168040c6d9721d08589581ab49204a330"
    "2e0201033029a1083106020100020101a203020120a30402020100a4053103020101a6053103020140bf8377020500"
    "0420a61c6e247e25b3e6e69aa78eb03c2d4ac20d1f99a9a024a76f35c8e2cab9b68d04102560c70109ae67c030f00b"
    "98b512a670");

auto wrapping_key = hex2str(
    "308204be020100300d06092a864886f70d0101010500048204a8308204a40201000282010100aec367931d8900ce56"
    "b0067f7d70e1fc653f3f34d194c1fed50018fb43db937b06e673a837313d56b1c725150a3fef86acbddc41bb759c28"
    "54eae32d35841efb5c18d82bc90a1cb5c1d55adf245b02911f0b7cda88c421ff0ebafe7c0d23be312d7bd5921ffaea"
    "1347c157406fef718f682643e4e5d33c6703d61c0cf7ac0bf4645c11f5c1374c3886427411c449796792e0bef75dec"
    "858a2123c36753e02a95a96d7c454b504de385a642e0dfc3e60ac3a7ee4991d0d48b0172a95f9536f02ba13cecccb9"
    "2b727db5c27e5b2f5cec09600b286af5cf14c42024c61ddfe71c2a8d7458f185234cb00e01d282f10f8fc6721d2aed"
    "3f4833cca2bd8fa62821dd55020301000102820100431447b6251908112b1ee76f99f3711a52b6630960046c2de70d"
    "e188d833f8b8b91e4d785caeeeaf4f0f74414e2cda40641f7fe24f14c67a88959bdb27766df9e710b630a03adc683b"
    "5d2c43080e52bee71e9eaeb6de297a5fea1072070d181c822bccff087d63c940ba8a45f670feb29fb4484d1c95e6d2"
    "579ba02aae0a00900c3ebf490e3d2cd7ee8d0e20c536e4dc5a5097272888cddd7e91f228b1c4d7474c55b8fcd618c4"
    "a957bbddd5ad7407cc312d8d98a5caf7e08f4a0d6b45bb41c652659d5a5ba05b663737a8696281865ba20fbdd7f851"
    "e6c56e8cbe0ddbbf24dc03b2d2cb4c3d540fb0af52e034a2d06698b128e5f101e3b51a34f8d8b4f8618102818100de"
    "392e18d682c829266cc3454e1d6166242f32d9a1d10577753e904ea7d08bff841be5bac82a164c5970007047b8c517"
    "db8f8f84e37bd5988561bdf503d4dc2bdb38f885434ae42c355f725c9a60f91f0788e1f1a97223b524b5357fdf72e2"
    "f696bab7d78e32bf92ba8e1864eab1229e91346130748a6e3c124f9149d71c743502818100c95387c0f9d35f137b57"
    "d0d65c397c5e21cc251e47008ed62a542409c8b6b6ac7f8967b3863ca645fcce49582a9aa17349db6c4a95affdae0d"
    "ae612e1afac99ed39a2d934c880440aed8832f9843163a47f27f392199dc1202f9a0f9bd08308007cb1e4e7f583093"
    "66a7de25f7c3c9b880677c068e1be936e81288815252a8a102818057ff8ca1895080b2cae486ef0adfd791fb0235c0"
    "b8b36cd6c136e52e4085f4ea5a063212a4f105a3764743e53281988aba073f6e0027298e1c4378556e0efca0e14ece"
    "1af76ad0b030f27af6f0ab35fb73a060d8b1a0e142fa2647e93b32e36d8282ae0a4de50ab7afe85500a16f43a64719"
    "d6e2b9439823719cd08bcd03178102818100ba73b0bb28e3f81e9bd1c568713b101241acc607976c4ddccc90e65b65"
    "56ca31516058f92b6e09f3b160ff0e374ec40d78ae4d4979fde6ac06a1a400c61dd31254186af30b22c10582a8a43e"
    "34fe949c5f3b9755bae7baa7b7b7a6bd03b38cef55c86885fc6c1978b9cee7ef33da507c9df6b9277cff1e6aaa5d57"
    "aca528466102818100c931617c77829dfb1270502be9195c8f2830885f57dba869536811e6864236d0c4736a0008a1"
    "45af36b8357a7c3d139966d04c4e00934ea1aede3bb6b8ec841dc95e3f579751e2bfdfe27ae778983f959356210723"
    "287b0affcc9f727044d48c373f1babde0724fa17a4fd4da0902c7c9b9bf27ba61be6ad02dfddda8f4e6822");

string zero_masking_key =
    hex2str("0000000000000000000000000000000000000000000000000000000000000000");
string masking_key = hex2str("D796B02C370F1FA4CC0124F14EC8CBEBE987E825246265050F399A51FD477DFC");

class ImportWrappedKeyTest : public KeymasterHidlTest {};

TEST_P(ImportWrappedKeyTest, Success) {
    auto wrapping_key_desc = AuthorizationSetBuilder()
                                 .RsaEncryptionKey(2048, 65537)
                                 .Digest(Digest::SHA_2_256)
                                 .Padding(PaddingMode::RSA_OAEP)
                                 .Authorization(TAG_PURPOSE, KeyPurpose::WRAP_KEY);

    ASSERT_EQ(ErrorCode::OK,
              ImportWrappedKey(
                  wrapped_key, wrapping_key, wrapping_key_desc, zero_masking_key,
                  AuthorizationSetBuilder()
                      .Digest(Digest::SHA_2_256)
                      .Padding(PaddingMode::RSA_OAEP)));

    string message = "Hello World!";
    auto params = AuthorizationSetBuilder().BlockMode(BlockMode::ECB).Padding(PaddingMode::PKCS7);
    string ciphertext = EncryptMessage(message, params);
    string plaintext = DecryptMessage(ciphertext, params);
    EXPECT_EQ(message, plaintext);
}

TEST_P(ImportWrappedKeyTest, SuccessMasked) {
    auto wrapping_key_desc = AuthorizationSetBuilder()
                                 .RsaEncryptionKey(2048, 65537)
                                 .Digest(Digest::SHA_2_256)
                                 .Padding(PaddingMode::RSA_OAEP)
                                 .Authorization(TAG_PURPOSE, KeyPurpose::WRAP_KEY);

    ASSERT_EQ(ErrorCode::OK,
              ImportWrappedKey(
                  wrapped_key_masked, wrapping_key, wrapping_key_desc, masking_key,
                  AuthorizationSetBuilder()
                      .Digest(Digest::SHA_2_256)
                      .Padding(PaddingMode::RSA_OAEP)));
}

TEST_P(ImportWrappedKeyTest, WrongMask) {
    auto wrapping_key_desc = AuthorizationSetBuilder()
                                 .RsaEncryptionKey(2048, 65537)
                                 .Digest(Digest::SHA_2_256)
                                 .Padding(PaddingMode::RSA_OAEP)
                                 .Authorization(TAG_PURPOSE, KeyPurpose::WRAP_KEY);

    ASSERT_EQ(ErrorCode::VERIFICATION_FAILED,
              ImportWrappedKey(
                  wrapped_key_masked, wrapping_key, wrapping_key_desc, zero_masking_key,
                  AuthorizationSetBuilder()
                      .Digest(Digest::SHA_2_256)
                      .Padding(PaddingMode::RSA_OAEP)));
}

TEST_P(ImportWrappedKeyTest, WrongPurpose) {
    auto wrapping_key_desc = AuthorizationSetBuilder()
                                 .RsaEncryptionKey(2048, 65537)
                                 .Digest(Digest::SHA_2_256)
                                 .Padding(PaddingMode::RSA_OAEP);

    ASSERT_EQ(ErrorCode::INCOMPATIBLE_PURPOSE,
              ImportWrappedKey(
                  wrapped_key_masked, wrapping_key, wrapping_key_desc, zero_masking_key,
                  AuthorizationSetBuilder()
                      .Digest(Digest::SHA_2_256)
                      .Padding(PaddingMode::RSA_OAEP)));
}

typedef KeymasterHidlTest EncryptionOperationsTest;

/*
 * EncryptionOperationsTest.RsaNoPaddingSuccess
 *
 * Verifies that raw RSA encryption works.
 */
TEST_P(EncryptionOperationsTest, RsaNoPaddingSuccess) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .RsaEncryptionKey(2048, 65537)
                                             .Padding(PaddingMode::NONE)));

    string message = string(2048 / 8, 'a');
    auto params = AuthorizationSetBuilder().Padding(PaddingMode::NONE);
    string ciphertext1 = EncryptMessage(message, params);
    EXPECT_EQ(2048U / 8, ciphertext1.size());

    string ciphertext2 = EncryptMessage(message, params);
    EXPECT_EQ(2048U / 8, ciphertext2.size());

    // Unpadded RSA is deterministic
    EXPECT_EQ(ciphertext1, ciphertext2);
}

/*
 * EncryptionOperationsTest.RsaNoPaddingShortMessage
 *
 * Verifies that raw RSA encryption of short messages works.
 */
TEST_P(EncryptionOperationsTest, RsaNoPaddingShortMessage) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .RsaEncryptionKey(2048, 65537)
                                             .Padding(PaddingMode::NONE)));

    string message = "1";
    auto params = AuthorizationSetBuilder().Padding(PaddingMode::NONE);

    string ciphertext = EncryptMessage(message, params);
    EXPECT_EQ(2048U / 8, ciphertext.size());

    string expected_plaintext = string(2048U / 8 - 1, 0) + message;
    string plaintext = DecryptMessage(ciphertext, params);

    EXPECT_EQ(expected_plaintext, plaintext);

    // Degenerate case, encrypting a numeric 1 yields 0x00..01 as the ciphertext.
    message = static_cast<char>(1);
    ciphertext = EncryptMessage(message, params);
    EXPECT_EQ(2048U / 8, ciphertext.size());
    EXPECT_EQ(ciphertext, string(2048U / 8 - 1, 0) + message);
}

/*
 * EncryptionOperationsTest.RsaNoPaddingTooLong
 *
 * Verifies that raw RSA encryption of too-long messages fails in the expected way.
 */
TEST_P(EncryptionOperationsTest, RsaNoPaddingTooLong) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .RsaEncryptionKey(2048, 65537)
                                             .Padding(PaddingMode::NONE)));

    string message(2048 / 8 + 1, 'a');

    auto params = AuthorizationSetBuilder().Padding(PaddingMode::NONE);
    EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::ENCRYPT, params));

    string result;
    EXPECT_EQ(ErrorCode::INVALID_INPUT_LENGTH, Finish(message, &result));
}

/*
 * EncryptionOperationsTest.RsaNoPaddingTooLarge
 *
 * Verifies that raw RSA encryption of too-large (numerically) messages fails in the expected way.
 */
TEST_P(EncryptionOperationsTest, RsaNoPaddingTooLarge) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .RsaEncryptionKey(2048, 65537)
                                             .Padding(PaddingMode::NONE)));

    HidlBuf exported;
    ASSERT_EQ(ErrorCode::OK, ExportKey(KeyFormat::X509, &exported));

    const uint8_t* p = exported.data();
    EVP_PKEY_Ptr pkey(d2i_PUBKEY(nullptr /* alloc new */, &p, exported.size()));
    RSA_Ptr rsa(EVP_PKEY_get1_RSA(pkey.get()));

    size_t modulus_len = BN_num_bytes(rsa->n);
    ASSERT_EQ(2048U / 8, modulus_len);
    std::unique_ptr<uint8_t[]> modulus_buf(new uint8_t[modulus_len]);
    BN_bn2bin(rsa->n, modulus_buf.get());

    // The modulus is too big to encrypt.
    string message(reinterpret_cast<const char*>(modulus_buf.get()), modulus_len);

    auto params = AuthorizationSetBuilder().Padding(PaddingMode::NONE);
    EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::ENCRYPT, params));

    string result;
    EXPECT_EQ(ErrorCode::INVALID_ARGUMENT, Finish(message, &result));

    // One smaller than the modulus is okay.
    BN_sub(rsa->n, rsa->n, BN_value_one());
    modulus_len = BN_num_bytes(rsa->n);
    ASSERT_EQ(2048U / 8, modulus_len);
    BN_bn2bin(rsa->n, modulus_buf.get());
    message = string(reinterpret_cast<const char*>(modulus_buf.get()), modulus_len);
    EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::ENCRYPT, params));
    EXPECT_EQ(ErrorCode::OK, Finish(message, &result));
}

/*
 * EncryptionOperationsTest.RsaOaepSuccess
 *
 * Verifies that RSA-OAEP encryption operations work, with all digests.
 */
TEST_P(EncryptionOperationsTest, RsaOaepSuccess) {
    auto digests = ValidDigests(false /* withNone */, true /* withMD5 */);

    size_t key_size = 2048;  // Need largish key for SHA-512 test.
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .RsaEncryptionKey(key_size, 65537)
                                             .Padding(PaddingMode::RSA_OAEP)
                                             .Digest(digests)));

    string message = "Hello";

    for (auto digest : digests) {
        auto params = AuthorizationSetBuilder().Digest(digest).Padding(PaddingMode::RSA_OAEP);
        string ciphertext1 = EncryptMessage(message, params);
        if (HasNonfatalFailure()) std::cout << "-->" << digest << std::endl;
        EXPECT_EQ(key_size / 8, ciphertext1.size());

        string ciphertext2 = EncryptMessage(message, params);
        EXPECT_EQ(key_size / 8, ciphertext2.size());

        // OAEP randomizes padding so every result should be different (with astronomically high
        // probability).
        EXPECT_NE(ciphertext1, ciphertext2);

        string plaintext1 = DecryptMessage(ciphertext1, params);
        EXPECT_EQ(message, plaintext1) << "RSA-OAEP failed with digest " << digest;
        string plaintext2 = DecryptMessage(ciphertext2, params);
        EXPECT_EQ(message, plaintext2) << "RSA-OAEP failed with digest " << digest;

        // Decrypting corrupted ciphertext should fail.
        size_t offset_to_corrupt = random() % ciphertext1.size();
        char corrupt_byte;
        do {
            corrupt_byte = static_cast<char>(random() % 256);
        } while (corrupt_byte == ciphertext1[offset_to_corrupt]);
        ciphertext1[offset_to_corrupt] = corrupt_byte;

        EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::DECRYPT, params));
        string result;
        EXPECT_EQ(ErrorCode::UNKNOWN_ERROR, Finish(ciphertext1, &result));
        EXPECT_EQ(0U, result.size());
    }
}

/*
 * EncryptionOperationsTest.RsaOaepInvalidDigest
 *
 * Verifies that RSA-OAEP encryption operations fail in the correct way when asked to operate
 * without a digest.
 */
TEST_P(EncryptionOperationsTest, RsaOaepInvalidDigest) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .RsaEncryptionKey(2048, 65537)
                                             .Padding(PaddingMode::RSA_OAEP)
                                             .Digest(Digest::NONE)));
    string message = "Hello World!";

    auto params = AuthorizationSetBuilder().Padding(PaddingMode::RSA_OAEP).Digest(Digest::NONE);
    EXPECT_EQ(ErrorCode::INCOMPATIBLE_DIGEST, Begin(KeyPurpose::ENCRYPT, params));
}

/*
 * EncryptionOperationsTest.RsaOaepInvalidDigest
 *
 * Verifies that RSA-OAEP encryption operations fail in the correct way when asked to decrypt with a
 * different digest than was used to encrypt.
 */
TEST_P(EncryptionOperationsTest, RsaOaepDecryptWithWrongDigest) {
    if (SecLevel() == SecurityLevel::STRONGBOX) return;

    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .RsaEncryptionKey(1024, 65537)
                                             .Padding(PaddingMode::RSA_OAEP)
                                             .Digest(Digest::SHA_2_224, Digest::SHA_2_256)));
    string message = "Hello World!";
    string ciphertext = EncryptMessage(
        message,
        AuthorizationSetBuilder().Digest(Digest::SHA_2_224).Padding(PaddingMode::RSA_OAEP));

    EXPECT_EQ(
        ErrorCode::OK,
        Begin(KeyPurpose::DECRYPT,
              AuthorizationSetBuilder().Digest(Digest::SHA_2_256).Padding(PaddingMode::RSA_OAEP)));
    string result;
    EXPECT_EQ(ErrorCode::UNKNOWN_ERROR, Finish(ciphertext, &result));
    EXPECT_EQ(0U, result.size());
}

/*
 * EncryptionOperationsTest.RsaOaepTooLarge
 *
 * Verifies that RSA-OAEP encryption operations fail in the correct way when asked to encrypt a
 * too-large message.
 */
TEST_P(EncryptionOperationsTest, RsaOaepTooLarge) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .RsaEncryptionKey(2048, 65537)
                                             .Padding(PaddingMode::RSA_OAEP)
                                             .Digest(Digest::SHA_2_256)));
    constexpr size_t digest_size = 256 /* SHA_2_256 */ / 8;
    constexpr size_t oaep_overhead = 2 * digest_size + 2;
    string message(2048 / 8 - oaep_overhead + 1, 'a');
    EXPECT_EQ(ErrorCode::OK,
              Begin(KeyPurpose::ENCRYPT,
                    AuthorizationSetBuilder().Padding(PaddingMode::RSA_OAEP).Digest(Digest::SHA_2_256)));
    string result;
    auto error = Finish(message, &result);
    EXPECT_TRUE(error == ErrorCode::INVALID_INPUT_LENGTH || error == ErrorCode::INVALID_ARGUMENT);
    EXPECT_EQ(0U, result.size());
}

/*
 * EncryptionOperationsTest.RsaPkcs1Success
 *
 * Verifies that RSA PKCS encryption/decrypts works.
 */
TEST_P(EncryptionOperationsTest, RsaPkcs1Success) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .RsaEncryptionKey(2048, 65537)
                                             .Padding(PaddingMode::RSA_PKCS1_1_5_ENCRYPT)));

    string message = "Hello World!";
    auto params = AuthorizationSetBuilder().Padding(PaddingMode::RSA_PKCS1_1_5_ENCRYPT);
    string ciphertext1 = EncryptMessage(message, params);
    EXPECT_EQ(2048U / 8, ciphertext1.size());

    string ciphertext2 = EncryptMessage(message, params);
    EXPECT_EQ(2048U / 8, ciphertext2.size());

    // PKCS1 v1.5 randomizes padding so every result should be different.
    EXPECT_NE(ciphertext1, ciphertext2);

    string plaintext = DecryptMessage(ciphertext1, params);
    EXPECT_EQ(message, plaintext);

    // Decrypting corrupted ciphertext should fail.
    size_t offset_to_corrupt = random() % ciphertext1.size();
    char corrupt_byte;
    do {
        corrupt_byte = static_cast<char>(random() % 256);
    } while (corrupt_byte == ciphertext1[offset_to_corrupt]);
    ciphertext1[offset_to_corrupt] = corrupt_byte;

    EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::DECRYPT, params));
    string result;
    EXPECT_EQ(ErrorCode::UNKNOWN_ERROR, Finish(ciphertext1, &result));
    EXPECT_EQ(0U, result.size());
}

/*
 * EncryptionOperationsTest.RsaPkcs1TooLarge
 *
 * Verifies that RSA PKCS encryption fails in the correct way when the mssage is too large.
 */
TEST_P(EncryptionOperationsTest, RsaPkcs1TooLarge) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .RsaEncryptionKey(2048, 65537)
                                             .Padding(PaddingMode::RSA_PKCS1_1_5_ENCRYPT)));
    string message(2048 / 8 - 10, 'a');

    auto params = AuthorizationSetBuilder().Padding(PaddingMode::RSA_PKCS1_1_5_ENCRYPT);
    EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::ENCRYPT, params));
    string result;
    auto error = Finish(message, &result);
    EXPECT_TRUE(error == ErrorCode::INVALID_INPUT_LENGTH || error == ErrorCode::INVALID_ARGUMENT);
    EXPECT_EQ(0U, result.size());
}

/*
 * EncryptionOperationsTest.EcdsaEncrypt
 *
 * Verifies that attempting to use ECDSA keys to encrypt fails in the correct way.
 */
TEST_P(EncryptionOperationsTest, EcdsaEncrypt) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .EcdsaSigningKey(256)
                                             .Digest(Digest::NONE)));
    auto params = AuthorizationSetBuilder().Digest(Digest::NONE);
    ASSERT_EQ(ErrorCode::UNSUPPORTED_PURPOSE, Begin(KeyPurpose::ENCRYPT, params));
    ASSERT_EQ(ErrorCode::UNSUPPORTED_PURPOSE, Begin(KeyPurpose::DECRYPT, params));
}

/*
 * EncryptionOperationsTest.HmacEncrypt
 *
 * Verifies that attempting to use HMAC keys to encrypt fails in the correct way.
 */
TEST_P(EncryptionOperationsTest, HmacEncrypt) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .HmacKey(128)
                                             .Digest(Digest::SHA_2_256)
                                             .Padding(PaddingMode::NONE)
                                             .Authorization(TAG_MIN_MAC_LENGTH, 128)));
    auto params = AuthorizationSetBuilder()
                      .Digest(Digest::SHA_2_256)
                      .Padding(PaddingMode::NONE)
                      .Authorization(TAG_MAC_LENGTH, 128);
    ASSERT_EQ(ErrorCode::UNSUPPORTED_PURPOSE, Begin(KeyPurpose::ENCRYPT, params));
    ASSERT_EQ(ErrorCode::UNSUPPORTED_PURPOSE, Begin(KeyPurpose::DECRYPT, params));
}

/*
 * EncryptionOperationsTest.AesEcbRoundTripSuccess
 *
 * Verifies that AES ECB mode works.
 */
TEST_P(EncryptionOperationsTest, AesEcbRoundTripSuccess) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .AesEncryptionKey(128)
                                             .Authorization(TAG_BLOCK_MODE, BlockMode::ECB)
                                             .Padding(PaddingMode::NONE)));

    auto params = AuthorizationSetBuilder().BlockMode(BlockMode::ECB).Padding(PaddingMode::NONE);

    // Two-block message.
    string message = "12345678901234567890123456789012";
    string ciphertext1 = EncryptMessage(message, params);
    EXPECT_EQ(message.size(), ciphertext1.size());

    string ciphertext2 = EncryptMessage(string(message), params);
    EXPECT_EQ(message.size(), ciphertext2.size());

    // ECB is deterministic.
    EXPECT_EQ(ciphertext1, ciphertext2);

    string plaintext = DecryptMessage(ciphertext1, params);
    EXPECT_EQ(message, plaintext);
}

/*
 * EncryptionOperationsTest.AesEcbRoundTripSuccess
 *
 * Verifies that AES encryption fails in the correct way when an unauthorized mode is specified.
 */
TEST_P(EncryptionOperationsTest, AesWrongMode) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .AesEncryptionKey(128)
                                             .Authorization(TAG_BLOCK_MODE, BlockMode::CBC)
                                             .Padding(PaddingMode::NONE)));
    // Two-block message.
    string message = "12345678901234567890123456789012";
    EXPECT_EQ(
        ErrorCode::INCOMPATIBLE_BLOCK_MODE,
        Begin(KeyPurpose::ENCRYPT,
              AuthorizationSetBuilder().BlockMode(BlockMode::ECB).Padding(PaddingMode::NONE)));
}

/*
 * EncryptionOperationsTest.AesWrongPurpose
 *
 * Verifies that AES encryption fails in the correct way when an unauthorized purpose is specified.
 */
TEST_P(EncryptionOperationsTest, AesWrongPurpose) {
    auto err = GenerateKey(AuthorizationSetBuilder()
                                   .Authorization(TAG_NO_AUTH_REQUIRED)
                                   .AesKey(128)
                                   .Authorization(TAG_PURPOSE, KeyPurpose::ENCRYPT)
                                   .Authorization(TAG_BLOCK_MODE, BlockMode::GCM)
                                   .Authorization(TAG_MIN_MAC_LENGTH, 128)
                                   .Padding(PaddingMode::NONE));
    ASSERT_EQ(ErrorCode::OK, err) << "Got " << err;

    err = Begin(KeyPurpose::DECRYPT,
                AuthorizationSetBuilder().BlockMode(BlockMode::GCM).Padding(PaddingMode::NONE));
    EXPECT_EQ(ErrorCode::INCOMPATIBLE_PURPOSE, err) << "Got " << err;

    CheckedDeleteKey();

    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .AesKey(128)
                                                 .Authorization(TAG_PURPOSE, KeyPurpose::DECRYPT)
                                                 .Authorization(TAG_BLOCK_MODE, BlockMode::GCM)
                                                 .Authorization(TAG_MIN_MAC_LENGTH, 128)
                                                 .Padding(PaddingMode::NONE)));

    err = Begin(KeyPurpose::ENCRYPT,
                AuthorizationSetBuilder().BlockMode(BlockMode::GCM).Padding(PaddingMode::NONE));
    EXPECT_EQ(ErrorCode::INCOMPATIBLE_PURPOSE, err) << "Got " << err;
}

/*
 * EncryptionOperationsTest.AesEcbNoPaddingWrongInputSize
 *
 * Verifies that AES encryption fails in the correct way when provided an input that is not a
 * multiple of the block size and no padding is specified.
 */
TEST_P(EncryptionOperationsTest, AesEcbNoPaddingWrongInputSize) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .AesEncryptionKey(128)
                                             .Authorization(TAG_BLOCK_MODE, BlockMode::ECB)
                                             .Padding(PaddingMode::NONE)));
    // Message is slightly shorter than two blocks.
    string message(16 * 2 - 1, 'a');

    auto params = AuthorizationSetBuilder().BlockMode(BlockMode::ECB).Padding(PaddingMode::NONE);
    EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::ENCRYPT, params));
    string ciphertext;
    EXPECT_EQ(ErrorCode::INVALID_INPUT_LENGTH, Finish(message, &ciphertext));
    EXPECT_EQ(0U, ciphertext.size());
}

/*
 * EncryptionOperationsTest.AesEcbPkcs7Padding
 *
 * Verifies that AES PKCS7 padding works for any message length.
 */
TEST_P(EncryptionOperationsTest, AesEcbPkcs7Padding) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .AesEncryptionKey(128)
                                             .Authorization(TAG_BLOCK_MODE, BlockMode::ECB)
                                             .Padding(PaddingMode::PKCS7)));

    auto params = AuthorizationSetBuilder().BlockMode(BlockMode::ECB).Padding(PaddingMode::PKCS7);

    // Try various message lengths; all should work.
    for (size_t i = 0; i < 32; ++i) {
        string message(i, 'a');
        string ciphertext = EncryptMessage(message, params);
        EXPECT_EQ(i + 16 - (i % 16), ciphertext.size());
        string plaintext = DecryptMessage(ciphertext, params);
        EXPECT_EQ(message, plaintext);
    }
}

/*
 * EncryptionOperationsTest.AesEcbWrongPadding
 *
 * Verifies that AES enryption fails in the correct way when an unauthorized padding mode is
 * specified.
 */
TEST_P(EncryptionOperationsTest, AesEcbWrongPadding) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .AesEncryptionKey(128)
                                             .Authorization(TAG_BLOCK_MODE, BlockMode::ECB)
                                             .Padding(PaddingMode::NONE)));

    auto params = AuthorizationSetBuilder().BlockMode(BlockMode::ECB).Padding(PaddingMode::PKCS7);

    // Try various message lengths; all should fail
    for (size_t i = 0; i < 32; ++i) {
        string message(i, 'a');
        EXPECT_EQ(ErrorCode::INCOMPATIBLE_PADDING_MODE, Begin(KeyPurpose::ENCRYPT, params));
    }
}

/*
 * EncryptionOperationsTest.AesEcbPkcs7PaddingCorrupted
 *
 * Verifies that AES decryption fails in the correct way when the padding is corrupted.
 */
TEST_P(EncryptionOperationsTest, AesEcbPkcs7PaddingCorrupted) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .AesEncryptionKey(128)
                                             .Authorization(TAG_BLOCK_MODE, BlockMode::ECB)
                                             .Padding(PaddingMode::PKCS7)));

    auto params = AuthorizationSetBuilder().BlockMode(BlockMode::ECB).Padding(PaddingMode::PKCS7);

    string message = "a";
    string ciphertext = EncryptMessage(message, params);
    EXPECT_EQ(16U, ciphertext.size());
    EXPECT_NE(ciphertext, message);
    ++ciphertext[ciphertext.size() / 2];

    EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::DECRYPT, params));
    string plaintext;
    EXPECT_EQ(ErrorCode::INVALID_INPUT_LENGTH, Finish(message, &plaintext));
}

HidlBuf CopyIv(const AuthorizationSet& set) {
    auto iv = set.GetTagValue(TAG_NONCE);
    EXPECT_TRUE(iv.isOk());
    return iv.value();
}

/*
 * EncryptionOperationsTest.AesCtrRoundTripSuccess
 *
 * Verifies that AES CTR mode works.
 */
TEST_P(EncryptionOperationsTest, AesCtrRoundTripSuccess) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .AesEncryptionKey(128)
                                             .Authorization(TAG_BLOCK_MODE, BlockMode::CTR)
                                             .Padding(PaddingMode::NONE)));

    auto params = AuthorizationSetBuilder().BlockMode(BlockMode::CTR).Padding(PaddingMode::NONE);

    string message = "123";
    AuthorizationSet out_params;
    string ciphertext1 = EncryptMessage(message, params, &out_params);
    HidlBuf iv1 = CopyIv(out_params);
    EXPECT_EQ(16U, iv1.size());

    EXPECT_EQ(message.size(), ciphertext1.size());

    out_params.Clear();
    string ciphertext2 = EncryptMessage(message, params, &out_params);
    HidlBuf iv2 = CopyIv(out_params);
    EXPECT_EQ(16U, iv2.size());

    // IVs should be random, so ciphertexts should differ.
    EXPECT_NE(ciphertext1, ciphertext2);

    auto params_iv1 =
        AuthorizationSetBuilder().Authorizations(params).Authorization(TAG_NONCE, iv1);
    auto params_iv2 =
        AuthorizationSetBuilder().Authorizations(params).Authorization(TAG_NONCE, iv2);

    string plaintext = DecryptMessage(ciphertext1, params_iv1);
    EXPECT_EQ(message, plaintext);
    plaintext = DecryptMessage(ciphertext2, params_iv2);
    EXPECT_EQ(message, plaintext);

    // Using the wrong IV will result in a "valid" decryption, but the data will be garbage.
    plaintext = DecryptMessage(ciphertext1, params_iv2);
    EXPECT_NE(message, plaintext);
    plaintext = DecryptMessage(ciphertext2, params_iv1);
    EXPECT_NE(message, plaintext);
}

/*
 * EncryptionOperationsTest.AesIncremental
 *
 * Verifies that AES works, all modes, when provided data in various size increments.
 */
TEST_P(EncryptionOperationsTest, AesIncremental) {
    auto block_modes = {
        BlockMode::ECB, BlockMode::CBC, BlockMode::CTR, BlockMode::GCM,
    };

    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .AesEncryptionKey(128)
                                             .BlockMode(block_modes)
                                             .Padding(PaddingMode::NONE)
                                             .Authorization(TAG_MIN_MAC_LENGTH, 128)));

    for (int increment = 1; increment <= 240; ++increment) {
        for (auto block_mode : block_modes) {
            string message(240, 'a');
            auto params = AuthorizationSetBuilder()
                              .BlockMode(block_mode)
                              .Padding(PaddingMode::NONE)
                              .Authorization(TAG_MAC_LENGTH, 128) /* for GCM */;

            AuthorizationSet output_params;
            EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::ENCRYPT, params, &output_params));

            string ciphertext;
            size_t input_consumed;
            string to_send;
            for (size_t i = 0; i < message.size(); i += increment) {
                to_send.append(message.substr(i, increment));
                EXPECT_EQ(ErrorCode::OK, Update(to_send, &ciphertext, &input_consumed));
                EXPECT_EQ(to_send.length(), input_consumed);
                to_send = to_send.substr(input_consumed);
                EXPECT_EQ(0U, to_send.length());

                switch (block_mode) {
                    case BlockMode::ECB:
                    case BlockMode::CBC:
                        // Implementations must take as many blocks as possible, leaving less than
                        // a block.
                        EXPECT_LE(to_send.length(), 16U);
                        break;
                    case BlockMode::GCM:
                    case BlockMode::CTR:
                        // Implementations must always take all the data.
                        EXPECT_EQ(0U, to_send.length());
                        break;
                }
            }
            EXPECT_EQ(ErrorCode::OK, Finish(to_send, &ciphertext)) << "Error sending " << to_send;

            switch (block_mode) {
                case BlockMode::GCM:
                    EXPECT_EQ(message.size() + 16, ciphertext.size());
                    break;
                case BlockMode::CTR:
                    EXPECT_EQ(message.size(), ciphertext.size());
                    break;
                case BlockMode::CBC:
                case BlockMode::ECB:
                    EXPECT_EQ(message.size() + message.size() % 16, ciphertext.size());
                    break;
            }

            auto iv = output_params.GetTagValue(TAG_NONCE);
            switch (block_mode) {
                case BlockMode::CBC:
                case BlockMode::GCM:
                case BlockMode::CTR:
                    ASSERT_TRUE(iv.isOk()) << "No IV for block mode " << block_mode;
                    EXPECT_EQ(block_mode == BlockMode::GCM ? 12U : 16U, iv.value().size());
                    params.push_back(TAG_NONCE, iv.value());
                    break;

                case BlockMode::ECB:
                    EXPECT_FALSE(iv.isOk()) << "ECB mode should not generate IV";
                    break;
            }

            EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::DECRYPT, params))
                << "Decrypt begin() failed for block mode " << block_mode;

            string plaintext;
            for (size_t i = 0; i < ciphertext.size(); i += increment) {
                to_send.append(ciphertext.substr(i, increment));
                EXPECT_EQ(ErrorCode::OK, Update(to_send, &plaintext, &input_consumed));
                to_send = to_send.substr(input_consumed);
            }
            ErrorCode error = Finish(to_send, &plaintext);
            ASSERT_EQ(ErrorCode::OK, error) << "Decryption failed for block mode " << block_mode
                                            << " and increment " << increment;
            if (error == ErrorCode::OK) {
                ASSERT_EQ(message, plaintext) << "Decryption didn't match for block mode "
                                              << block_mode << " and increment " << increment;
            }
        }
    }
}

struct AesCtrSp80038aTestVector {
    const char* key;
    const char* nonce;
    const char* plaintext;
    const char* ciphertext;
};

// These test vectors are taken from
// http://csrc.nist.gov/publications/nistpubs/800-38a/sp800-38a.pdf, section F.5.
static const AesCtrSp80038aTestVector kAesCtrSp80038aTestVectors[] = {
    // AES-128
    {
        "2b7e151628aed2a6abf7158809cf4f3c", "f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff",
        "6bc1bee22e409f96e93d7e117393172aae2d8a571e03ac9c9eb76fac45af8e51"
        "30c81c46a35ce411e5fbc1191a0a52eff69f2445df4f9b17ad2b417be66c3710",
        "874d6191b620e3261bef6864990db6ce9806f66b7970fdff8617187bb9fffdff"
        "5ae4df3edbd5d35e5b4f09020db03eab1e031dda2fbe03d1792170a0f3009cee",
    },
    // AES-192
    {
        "8e73b0f7da0e6452c810f32b809079e562f8ead2522c6b7b", "f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff",
        "6bc1bee22e409f96e93d7e117393172aae2d8a571e03ac9c9eb76fac45af8e51"
        "30c81c46a35ce411e5fbc1191a0a52eff69f2445df4f9b17ad2b417be66c3710",
        "1abc932417521ca24f2b0459fe7e6e0b090339ec0aa6faefd5ccc2c6f4ce8e94"
        "1e36b26bd1ebc670d1bd1d665620abf74f78a7f6d29809585a97daec58c6b050",
    },
    // AES-256
    {
        "603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4",
        "f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff",
        "6bc1bee22e409f96e93d7e117393172aae2d8a571e03ac9c9eb76fac45af8e51"
        "30c81c46a35ce411e5fbc1191a0a52eff69f2445df4f9b17ad2b417be66c3710",
        "601ec313775789a5b7a7f504bbf3d228f443e3ca4d62b59aca84e990cacaf5c5"
        "2b0930daa23de94ce87017ba2d84988ddfc9c58db67aada613c2dd08457941a6",
    },
};

/*
 * EncryptionOperationsTest.AesCtrSp80038aTestVector
 *
 * Verifies AES CTR implementation against SP800-38A test vectors.
 */
TEST_P(EncryptionOperationsTest, AesCtrSp80038aTestVector) {
    std::vector<uint32_t> InvalidSizes = InvalidKeySizes(Algorithm::AES);
    for (size_t i = 0; i < 3; i++) {
        const AesCtrSp80038aTestVector& test(kAesCtrSp80038aTestVectors[i]);
        const string key = hex2str(test.key);
        if (std::find(InvalidSizes.begin(), InvalidSizes.end(), (key.size() * 8)) !=
            InvalidSizes.end())
            continue;
        const string nonce = hex2str(test.nonce);
        const string plaintext = hex2str(test.plaintext);
        const string ciphertext = hex2str(test.ciphertext);
        CheckAesCtrTestVector(key, nonce, plaintext, ciphertext);
    }
}

/*
 * EncryptionOperationsTest.AesCtrIncompatiblePaddingMode
 *
 * Verifies that keymaster rejects use of CTR mode with PKCS7 padding in the correct way.
 */
TEST_P(EncryptionOperationsTest, AesCtrIncompatiblePaddingMode) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .AesEncryptionKey(128)
                                             .Authorization(TAG_BLOCK_MODE, BlockMode::CTR)
                                             .Padding(PaddingMode::PKCS7)));
    auto params = AuthorizationSetBuilder().BlockMode(BlockMode::CTR).Padding(PaddingMode::NONE);
    EXPECT_EQ(ErrorCode::INCOMPATIBLE_PADDING_MODE, Begin(KeyPurpose::ENCRYPT, params));
}

/*
 * EncryptionOperationsTest.AesCtrInvalidCallerNonce
 *
 * Verifies that keymaster fails correctly when the user supplies an incorrect-size nonce.
 */
TEST_P(EncryptionOperationsTest, AesCtrInvalidCallerNonce) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .AesEncryptionKey(128)
                                             .Authorization(TAG_BLOCK_MODE, BlockMode::CTR)
                                             .Authorization(TAG_CALLER_NONCE)
                                             .Padding(PaddingMode::NONE)));

    auto params = AuthorizationSetBuilder()
                      .BlockMode(BlockMode::CTR)
                      .Padding(PaddingMode::NONE)
                      .Authorization(TAG_NONCE, HidlBuf(string(1, 'a')));
    EXPECT_EQ(ErrorCode::INVALID_NONCE, Begin(KeyPurpose::ENCRYPT, params));

    params = AuthorizationSetBuilder()
                 .BlockMode(BlockMode::CTR)
                 .Padding(PaddingMode::NONE)
                 .Authorization(TAG_NONCE, HidlBuf(string(15, 'a')));
    EXPECT_EQ(ErrorCode::INVALID_NONCE, Begin(KeyPurpose::ENCRYPT, params));

    params = AuthorizationSetBuilder()
                 .BlockMode(BlockMode::CTR)
                 .Padding(PaddingMode::NONE)
                 .Authorization(TAG_NONCE, HidlBuf(string(17, 'a')));
    EXPECT_EQ(ErrorCode::INVALID_NONCE, Begin(KeyPurpose::ENCRYPT, params));
}

/*
 * EncryptionOperationsTest.AesCtrInvalidCallerNonce
 *
 * Verifies that keymaster fails correctly when the user supplies an incorrect-size nonce.
 */
TEST_P(EncryptionOperationsTest, AesCbcRoundTripSuccess) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .AesEncryptionKey(128)
                                             .Authorization(TAG_BLOCK_MODE, BlockMode::CBC)
                                             .Padding(PaddingMode::NONE)));
    // Two-block message.
    string message = "12345678901234567890123456789012";
    auto params = AuthorizationSetBuilder().BlockMode(BlockMode::CBC).Padding(PaddingMode::NONE);
    AuthorizationSet out_params;
    string ciphertext1 = EncryptMessage(message, params, &out_params);
    HidlBuf iv1 = CopyIv(out_params);
    EXPECT_EQ(message.size(), ciphertext1.size());

    out_params.Clear();

    string ciphertext2 = EncryptMessage(message, params, &out_params);
    HidlBuf iv2 = CopyIv(out_params);
    EXPECT_EQ(message.size(), ciphertext2.size());

    // IVs should be random, so ciphertexts should differ.
    EXPECT_NE(ciphertext1, ciphertext2);

    params.push_back(TAG_NONCE, iv1);
    string plaintext = DecryptMessage(ciphertext1, params);
    EXPECT_EQ(message, plaintext);
}

/*
 * EncryptionOperationsTest.AesCallerNonce
 *
 * Verifies that AES caller-provided nonces work correctly.
 */
TEST_P(EncryptionOperationsTest, AesCallerNonce) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .AesEncryptionKey(128)
                                             .Authorization(TAG_BLOCK_MODE, BlockMode::CBC)
                                             .Authorization(TAG_CALLER_NONCE)
                                             .Padding(PaddingMode::NONE)));

    string message = "12345678901234567890123456789012";

    // Don't specify nonce, should get a random one.
    AuthorizationSetBuilder params =
        AuthorizationSetBuilder().BlockMode(BlockMode::CBC).Padding(PaddingMode::NONE);
    AuthorizationSet out_params;
    string ciphertext = EncryptMessage(message, params, &out_params);
    EXPECT_EQ(message.size(), ciphertext.size());
    EXPECT_EQ(16U, out_params.GetTagValue(TAG_NONCE).value().size());

    params.push_back(TAG_NONCE, out_params.GetTagValue(TAG_NONCE).value());
    string plaintext = DecryptMessage(ciphertext, params);
    EXPECT_EQ(message, plaintext);

    // Now specify a nonce, should also work.
    params = AuthorizationSetBuilder()
                 .BlockMode(BlockMode::CBC)
                 .Padding(PaddingMode::NONE)
                 .Authorization(TAG_NONCE, HidlBuf("abcdefghijklmnop"));
    out_params.Clear();
    ciphertext = EncryptMessage(message, params, &out_params);

    // Decrypt with correct nonce.
    plaintext = DecryptMessage(ciphertext, params);
    EXPECT_EQ(message, plaintext);

    // Try with wrong nonce.
    params = AuthorizationSetBuilder()
                 .BlockMode(BlockMode::CBC)
                 .Padding(PaddingMode::NONE)
                 .Authorization(TAG_NONCE, HidlBuf("aaaaaaaaaaaaaaaa"));
    plaintext = DecryptMessage(ciphertext, params);
    EXPECT_NE(message, plaintext);
}

/*
 * EncryptionOperationsTest.AesCallerNonceProhibited
 *
 * Verifies that caller-provided nonces are not permitted when not specified in the key
 * authorizations.
 */
TEST_P(EncryptionOperationsTest, AesCallerNonceProhibited) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .AesEncryptionKey(128)
                                             .Authorization(TAG_BLOCK_MODE, BlockMode::CBC)
                                             .Padding(PaddingMode::NONE)));

    string message = "12345678901234567890123456789012";

    // Don't specify nonce, should get a random one.
    AuthorizationSetBuilder params =
        AuthorizationSetBuilder().BlockMode(BlockMode::CBC).Padding(PaddingMode::NONE);
    AuthorizationSet out_params;
    string ciphertext = EncryptMessage(message, params, &out_params);
    EXPECT_EQ(message.size(), ciphertext.size());
    EXPECT_EQ(16U, out_params.GetTagValue(TAG_NONCE).value().size());

    params.push_back(TAG_NONCE, out_params.GetTagValue(TAG_NONCE).value());
    string plaintext = DecryptMessage(ciphertext, params);
    EXPECT_EQ(message, plaintext);

    // Now specify a nonce, should fail
    params = AuthorizationSetBuilder()
                 .BlockMode(BlockMode::CBC)
                 .Padding(PaddingMode::NONE)
                 .Authorization(TAG_NONCE, HidlBuf("abcdefghijklmnop"));
    out_params.Clear();
    EXPECT_EQ(ErrorCode::CALLER_NONCE_PROHIBITED, Begin(KeyPurpose::ENCRYPT, params, &out_params));
}

/*
 * EncryptionOperationsTest.AesGcmRoundTripSuccess
 *
 * Verifies that AES GCM mode works.
 */
TEST_P(EncryptionOperationsTest, AesGcmRoundTripSuccess) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .AesEncryptionKey(128)
                                             .Authorization(TAG_BLOCK_MODE, BlockMode::GCM)
                                             .Padding(PaddingMode::NONE)
                                             .Authorization(TAG_MIN_MAC_LENGTH, 128)));

    string aad = "foobar";
    string message = "123456789012345678901234567890123456";

    auto begin_params = AuthorizationSetBuilder()
                            .BlockMode(BlockMode::GCM)
                            .Padding(PaddingMode::NONE)
                            .Authorization(TAG_MAC_LENGTH, 128);

    auto update_params =
        AuthorizationSetBuilder().Authorization(TAG_ASSOCIATED_DATA, aad.data(), aad.size());

    // Encrypt
    AuthorizationSet begin_out_params;
    ASSERT_EQ(ErrorCode::OK, Begin(KeyPurpose::ENCRYPT, begin_params, &begin_out_params))
        << "Begin encrypt";
    string ciphertext;
    AuthorizationSet update_out_params;
    ASSERT_EQ(ErrorCode::OK,
              Finish(op_handle_, update_params, message, "", &update_out_params, &ciphertext));

    ASSERT_EQ(ciphertext.length(), message.length() + 16);

    // Grab nonce
    begin_params.push_back(begin_out_params);

    // Decrypt.
    ASSERT_EQ(ErrorCode::OK, Begin(KeyPurpose::DECRYPT, begin_params)) << "Begin decrypt";
    string plaintext;
    size_t input_consumed;
    ASSERT_EQ(ErrorCode::OK, Update(op_handle_, update_params, ciphertext, &update_out_params,
                                    &plaintext, &input_consumed));
    EXPECT_EQ(ciphertext.size(), input_consumed);
    EXPECT_EQ(ErrorCode::OK, Finish("", &plaintext));
    EXPECT_EQ(message.length(), plaintext.length());
    EXPECT_EQ(message, plaintext);
}

/*
 * EncryptionOperationsTest.AesGcmRoundTripWithDelaySuccess
 *
 * Verifies that AES GCM mode works, even when there's a long delay
 * between operations.
 */
TEST_P(EncryptionOperationsTest, AesGcmRoundTripWithDelaySuccess) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .AesEncryptionKey(128)
                                             .Authorization(TAG_BLOCK_MODE, BlockMode::GCM)
                                             .Padding(PaddingMode::NONE)
                                             .Authorization(TAG_MIN_MAC_LENGTH, 128)));

    string aad = "foobar";
    string message = "123456789012345678901234567890123456";

    auto begin_params = AuthorizationSetBuilder()
                            .BlockMode(BlockMode::GCM)
                            .Padding(PaddingMode::NONE)
                            .Authorization(TAG_MAC_LENGTH, 128);

    auto update_params =
        AuthorizationSetBuilder().Authorization(TAG_ASSOCIATED_DATA, aad.data(), aad.size());

    // Encrypt
    AuthorizationSet begin_out_params;
    ASSERT_EQ(ErrorCode::OK, Begin(KeyPurpose::ENCRYPT, begin_params, &begin_out_params))
        << "Begin encrypt";
    string ciphertext;
    AuthorizationSet update_out_params;
    sleep(5);
    ASSERT_EQ(ErrorCode::OK,
              Finish(op_handle_, update_params, message, "", &update_out_params, &ciphertext));

    ASSERT_EQ(ciphertext.length(), message.length() + 16);

    // Grab nonce
    begin_params.push_back(begin_out_params);

    // Decrypt.
    ASSERT_EQ(ErrorCode::OK, Begin(KeyPurpose::DECRYPT, begin_params)) << "Begin decrypt";
    string plaintext;
    size_t input_consumed;
    sleep(5);
    ASSERT_EQ(ErrorCode::OK, Update(op_handle_, update_params, ciphertext, &update_out_params,
                                    &plaintext, &input_consumed));
    EXPECT_EQ(ciphertext.size(), input_consumed);
    sleep(5);
    EXPECT_EQ(ErrorCode::OK, Finish("", &plaintext));
    EXPECT_EQ(message.length(), plaintext.length());
    EXPECT_EQ(message, plaintext);
}

/*
 * EncryptionOperationsTest.AesGcmDifferentNonces
 *
 * Verifies that encrypting the same data with different nonces produces different outputs.
 */
TEST_P(EncryptionOperationsTest, AesGcmDifferentNonces) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .AesEncryptionKey(128)
                                                 .Authorization(TAG_BLOCK_MODE, BlockMode::GCM)
                                                 .Padding(PaddingMode::NONE)
                                                 .Authorization(TAG_MIN_MAC_LENGTH, 128)
                                                 .Authorization(TAG_CALLER_NONCE)));

    string aad = "foobar";
    string message = "123456789012345678901234567890123456";
    string nonce1 = "000000000000";
    string nonce2 = "111111111111";
    string nonce3 = "222222222222";

    string ciphertext1 =
            EncryptMessage(message, BlockMode::GCM, PaddingMode::NONE, 128, HidlBuf(nonce1));
    string ciphertext2 =
            EncryptMessage(message, BlockMode::GCM, PaddingMode::NONE, 128, HidlBuf(nonce2));
    string ciphertext3 =
            EncryptMessage(message, BlockMode::GCM, PaddingMode::NONE, 128, HidlBuf(nonce3));

    ASSERT_NE(ciphertext1, ciphertext2);
    ASSERT_NE(ciphertext1, ciphertext3);
    ASSERT_NE(ciphertext2, ciphertext3);
}

/*
 * EncryptionOperationsTest.AesGcmTooShortTag
 *
 * Verifies that AES GCM mode fails correctly when a too-short tag length is specified.
 */
TEST_P(EncryptionOperationsTest, AesGcmTooShortTag) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .AesEncryptionKey(128)
                                             .BlockMode(BlockMode::GCM)
                                             .Padding(PaddingMode::NONE)
                                             .Authorization(TAG_MIN_MAC_LENGTH, 128)));
    string message = "123456789012345678901234567890123456";
    auto params = AuthorizationSetBuilder()
                      .BlockMode(BlockMode::GCM)
                      .Padding(PaddingMode::NONE)
                      .Authorization(TAG_MAC_LENGTH, 96);

    EXPECT_EQ(ErrorCode::INVALID_MAC_LENGTH, Begin(KeyPurpose::ENCRYPT, params));
}

/*
 * EncryptionOperationsTest.AesGcmTooShortTagOnDecrypt
 *
 * Verifies that AES GCM mode fails correctly when a too-short tag is provided to decryption.
 */
TEST_P(EncryptionOperationsTest, AesGcmTooShortTagOnDecrypt) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .AesEncryptionKey(128)
                                             .BlockMode(BlockMode::GCM)
                                             .Padding(PaddingMode::NONE)
                                             .Authorization(TAG_MIN_MAC_LENGTH, 128)));
    string aad = "foobar";
    string message = "123456789012345678901234567890123456";
    auto params = AuthorizationSetBuilder()
                      .BlockMode(BlockMode::GCM)
                      .Padding(PaddingMode::NONE)
                      .Authorization(TAG_MAC_LENGTH, 128);

    auto finish_params =
        AuthorizationSetBuilder().Authorization(TAG_ASSOCIATED_DATA, aad.data(), aad.size());

    // Encrypt
    AuthorizationSet begin_out_params;
    EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::ENCRYPT, params, &begin_out_params));
    EXPECT_EQ(1U, begin_out_params.size());
    ASSERT_TRUE(begin_out_params.GetTagValue(TAG_NONCE).isOk());

    AuthorizationSet finish_out_params;
    string ciphertext;
    EXPECT_EQ(ErrorCode::OK, Finish(op_handle_, finish_params, message, "" /* signature */,
                                    &finish_out_params, &ciphertext));

    params = AuthorizationSetBuilder()
                 .Authorizations(begin_out_params)
                 .BlockMode(BlockMode::GCM)
                 .Padding(PaddingMode::NONE)
                 .Authorization(TAG_MAC_LENGTH, 96);

    // Decrypt.
    EXPECT_EQ(ErrorCode::INVALID_MAC_LENGTH, Begin(KeyPurpose::DECRYPT, params));
}

/*
 * EncryptionOperationsTest.AesGcmCorruptKey
 *
 * Verifies that AES GCM mode fails correctly when the decryption key is incorrect.
 */
TEST_P(EncryptionOperationsTest, AesGcmCorruptKey) {
    const uint8_t nonce_bytes[] = {
        0xb7, 0x94, 0x37, 0xae, 0x08, 0xff, 0x35, 0x5d, 0x7d, 0x8a, 0x4d, 0x0f,
    };
    string nonce = make_string(nonce_bytes);
    const uint8_t ciphertext_bytes[] = {
        0xb3, 0xf6, 0x79, 0x9e, 0x8f, 0x93, 0x26, 0xf2, 0xdf, 0x1e, 0x80, 0xfc, 0xd2, 0xcb, 0x16,
        0xd7, 0x8c, 0x9d, 0xc7, 0xcc, 0x14, 0xbb, 0x67, 0x78, 0x62, 0xdc, 0x6c, 0x63, 0x9b, 0x3a,
        0x63, 0x38, 0xd2, 0x4b, 0x31, 0x2d, 0x39, 0x89, 0xe5, 0x92, 0x0b, 0x5d, 0xbf, 0xc9, 0x76,
        0x76, 0x5e, 0xfb, 0xfe, 0x57, 0xbb, 0x38, 0x59, 0x40, 0xa7, 0xa4, 0x3b, 0xdf, 0x05, 0xbd,
        0xda, 0xe3, 0xc9, 0xd6, 0xa2, 0xfb, 0xbd, 0xfc, 0xc0, 0xcb, 0xa0,
    };
    string ciphertext = make_string(ciphertext_bytes);

    auto params = AuthorizationSetBuilder()
                      .BlockMode(BlockMode::GCM)
                      .Padding(PaddingMode::NONE)
                      .Authorization(TAG_MAC_LENGTH, 128)
                      .Authorization(TAG_NONCE, nonce.data(), nonce.size());

    auto import_params = AuthorizationSetBuilder()
                             .Authorization(TAG_NO_AUTH_REQUIRED)
                             .AesEncryptionKey(128)
                             .BlockMode(BlockMode::GCM)
                             .Padding(PaddingMode::NONE)
                             .Authorization(TAG_CALLER_NONCE)
                             .Authorization(TAG_MIN_MAC_LENGTH, 128);

    // Import correct key and decrypt
    const uint8_t key_bytes[] = {
        0xba, 0x76, 0x35, 0x4f, 0x0a, 0xed, 0x6e, 0x8d,
        0x91, 0xf4, 0x5c, 0x4f, 0xf5, 0xa0, 0x62, 0xdb,
    };
    string key = make_string(key_bytes);
    ASSERT_EQ(ErrorCode::OK, ImportKey(import_params, KeyFormat::RAW, key));
    string plaintext = DecryptMessage(ciphertext, params);
    CheckedDeleteKey();

    // Corrupt key and attempt to decrypt
    key[0] = 0;
    ASSERT_EQ(ErrorCode::OK, ImportKey(import_params, KeyFormat::RAW, key));
    EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::DECRYPT, params));
    EXPECT_EQ(ErrorCode::VERIFICATION_FAILED, Finish(ciphertext, &plaintext));
    CheckedDeleteKey();
}

/*
 * EncryptionOperationsTest.AesGcmAadNoData
 *
 * Verifies that AES GCM mode works when provided additional authenticated data, but no data to
 * encrypt.
 */
TEST_P(EncryptionOperationsTest, AesGcmAadNoData) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .AesEncryptionKey(128)
                                             .BlockMode(BlockMode::GCM)
                                             .Padding(PaddingMode::NONE)
                                             .Authorization(TAG_MIN_MAC_LENGTH, 128)));

    string aad = "1234567890123456";
    auto params = AuthorizationSetBuilder()
                      .BlockMode(BlockMode::GCM)
                      .Padding(PaddingMode::NONE)
                      .Authorization(TAG_MAC_LENGTH, 128);

    auto finish_params =
        AuthorizationSetBuilder().Authorization(TAG_ASSOCIATED_DATA, aad.data(), aad.size());

    // Encrypt
    AuthorizationSet begin_out_params;
    EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::ENCRYPT, params, &begin_out_params));
    string ciphertext;
    AuthorizationSet finish_out_params;
    EXPECT_EQ(ErrorCode::OK, Finish(op_handle_, finish_params, "" /* input */, "" /* signature */,
                                    &finish_out_params, &ciphertext));
    EXPECT_TRUE(finish_out_params.empty());

    // Grab nonce
    params.push_back(begin_out_params);

    // Decrypt.
    EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::DECRYPT, params));
    string plaintext;
    EXPECT_EQ(ErrorCode::OK, Finish(op_handle_, finish_params, ciphertext, "" /* signature */,
                                    &finish_out_params, &plaintext));

    EXPECT_TRUE(finish_out_params.empty());

    EXPECT_EQ("", plaintext);
}

/*
 * EncryptionOperationsTest.AesGcmMultiPartAad
 *
 * Verifies that AES GCM mode works when provided additional authenticated data in multiple chunks.
 */
TEST_P(EncryptionOperationsTest, AesGcmMultiPartAad) {
    const size_t tag_bits = 128;
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .AesEncryptionKey(128)
                                             .BlockMode(BlockMode::GCM)
                                             .Padding(PaddingMode::NONE)
                                             .Authorization(TAG_MIN_MAC_LENGTH, 128)));

    string message = "123456789012345678901234567890123456";
    auto begin_params = AuthorizationSetBuilder()
                            .BlockMode(BlockMode::GCM)
                            .Padding(PaddingMode::NONE)
                            .Authorization(TAG_MAC_LENGTH, tag_bits);
    AuthorizationSet begin_out_params;

    auto update_params =
        AuthorizationSetBuilder().Authorization(TAG_ASSOCIATED_DATA, "foo", (size_t)3);

    EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::ENCRYPT, begin_params, &begin_out_params));

    // No data, AAD only.
    string ciphertext;
    size_t input_consumed;
    AuthorizationSet update_out_params;
    EXPECT_EQ(ErrorCode::OK, Update(op_handle_, update_params, "" /* input */, &update_out_params,
                                    &ciphertext, &input_consumed));
    EXPECT_EQ(0U, input_consumed);
    EXPECT_EQ(0U, ciphertext.size());
    EXPECT_TRUE(update_out_params.empty());

    // AAD and data.
    EXPECT_EQ(ErrorCode::OK, Update(op_handle_, update_params, message, &update_out_params,
                                    &ciphertext, &input_consumed));
    EXPECT_EQ(message.size(), input_consumed);
    EXPECT_TRUE(update_out_params.empty());

    EXPECT_EQ(ErrorCode::OK, Finish("" /* input */, &ciphertext));
    // Expect 128-bit (16-byte) tag appended to ciphertext.
    EXPECT_EQ(message.size() + (tag_bits >> 3), ciphertext.size());

    // Grab nonce.
    begin_params.push_back(begin_out_params);

    // Decrypt
    update_params =
        AuthorizationSetBuilder().Authorization(TAG_ASSOCIATED_DATA, "foofoo", (size_t)6);

    EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::DECRYPT, begin_params));
    string plaintext;
    EXPECT_EQ(ErrorCode::OK, Finish(op_handle_, update_params, ciphertext, "" /* signature */,
                                    &update_out_params, &plaintext));
    EXPECT_TRUE(update_out_params.empty());
    EXPECT_EQ(message, plaintext);
}

/*
 * EncryptionOperationsTest.AesGcmAadOutOfOrder
 *
 * Verifies that AES GCM mode fails correctly when given AAD after data to encipher.
 */
TEST_P(EncryptionOperationsTest, AesGcmAadOutOfOrder) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .AesEncryptionKey(128)
                                             .BlockMode(BlockMode::GCM)
                                             .Padding(PaddingMode::NONE)
                                             .Authorization(TAG_MIN_MAC_LENGTH, 128)));

    string message = "123456789012345678901234567890123456";
    auto begin_params = AuthorizationSetBuilder()
                            .BlockMode(BlockMode::GCM)
                            .Padding(PaddingMode::NONE)
                            .Authorization(TAG_MAC_LENGTH, 128);
    AuthorizationSet begin_out_params;

    auto update_params =
        AuthorizationSetBuilder().Authorization(TAG_ASSOCIATED_DATA, "foo", (size_t)3);

    EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::ENCRYPT, begin_params, &begin_out_params));

    // No data, AAD only.
    string ciphertext;
    size_t input_consumed;
    AuthorizationSet update_out_params;
    EXPECT_EQ(ErrorCode::OK, Update(op_handle_, update_params, "" /* input */, &update_out_params,
                                    &ciphertext, &input_consumed));
    EXPECT_EQ(0U, input_consumed);
    EXPECT_EQ(0U, ciphertext.size());
    EXPECT_TRUE(update_out_params.empty());

    // AAD and data.
    EXPECT_EQ(ErrorCode::OK, Update(op_handle_, update_params, message, &update_out_params,
                                    &ciphertext, &input_consumed));
    EXPECT_EQ(message.size(), input_consumed);
    EXPECT_TRUE(update_out_params.empty());

    // More AAD
    EXPECT_EQ(ErrorCode::INVALID_TAG, Update(op_handle_, update_params, "", &update_out_params,
                                             &ciphertext, &input_consumed));

    op_handle_ = kOpHandleSentinel;
}

/*
 * EncryptionOperationsTest.AesGcmBadAad
 *
 * Verifies that AES GCM decryption fails correctly when additional authenticated date is wrong.
 */
TEST_P(EncryptionOperationsTest, AesGcmBadAad) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .AesEncryptionKey(128)
                                             .BlockMode(BlockMode::GCM)
                                             .Padding(PaddingMode::NONE)
                                             .Authorization(TAG_MIN_MAC_LENGTH, 128)));

    string message = "12345678901234567890123456789012";
    auto begin_params = AuthorizationSetBuilder()
                            .BlockMode(BlockMode::GCM)
                            .Padding(PaddingMode::NONE)
                            .Authorization(TAG_MAC_LENGTH, 128);

    auto finish_params =
        AuthorizationSetBuilder().Authorization(TAG_ASSOCIATED_DATA, "foobar", (size_t)6);

    // Encrypt
    AuthorizationSet begin_out_params;
    EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::ENCRYPT, begin_params, &begin_out_params));
    string ciphertext;
    AuthorizationSet finish_out_params;
    EXPECT_EQ(ErrorCode::OK, Finish(op_handle_, finish_params, message, "" /* signature */,
                                    &finish_out_params, &ciphertext));

    // Grab nonce
    begin_params.push_back(begin_out_params);

    finish_params = AuthorizationSetBuilder().Authorization(TAG_ASSOCIATED_DATA,
                                                            "barfoo" /* Wrong AAD */, (size_t)6);

    // Decrypt.
    EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::DECRYPT, begin_params, &begin_out_params));
    string plaintext;
    EXPECT_EQ(ErrorCode::VERIFICATION_FAILED,
              Finish(op_handle_, finish_params, ciphertext, "" /* signature */, &finish_out_params,
                     &plaintext));
}

/*
 * EncryptionOperationsTest.AesGcmWrongNonce
 *
 * Verifies that AES GCM decryption fails correctly when the nonce is incorrect.
 */
TEST_P(EncryptionOperationsTest, AesGcmWrongNonce) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .AesEncryptionKey(128)
                                             .BlockMode(BlockMode::GCM)
                                             .Padding(PaddingMode::NONE)
                                             .Authorization(TAG_MIN_MAC_LENGTH, 128)));

    string message = "12345678901234567890123456789012";
    auto begin_params = AuthorizationSetBuilder()
                            .BlockMode(BlockMode::GCM)
                            .Padding(PaddingMode::NONE)
                            .Authorization(TAG_MAC_LENGTH, 128);

    auto finish_params =
        AuthorizationSetBuilder().Authorization(TAG_ASSOCIATED_DATA, "foobar", (size_t)6);

    // Encrypt
    AuthorizationSet begin_out_params;
    EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::ENCRYPT, begin_params, &begin_out_params));
    string ciphertext;
    AuthorizationSet finish_out_params;
    EXPECT_EQ(ErrorCode::OK, Finish(op_handle_, finish_params, message, "" /* signature */,
                                    &finish_out_params, &ciphertext));

    // Wrong nonce
    begin_params.push_back(TAG_NONCE, HidlBuf("123456789012"));

    // Decrypt.
    EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::DECRYPT, begin_params, &begin_out_params));
    string plaintext;
    EXPECT_EQ(ErrorCode::VERIFICATION_FAILED,
              Finish(op_handle_, finish_params, ciphertext, "" /* signature */, &finish_out_params,
                     &plaintext));

    // With wrong nonce, should have gotten garbage plaintext (or none).
    EXPECT_NE(message, plaintext);
}

/*
 * EncryptionOperationsTest.AesGcmCorruptTag
 *
 * Verifies that AES GCM decryption fails correctly when the tag is wrong.
 */
TEST_P(EncryptionOperationsTest, AesGcmCorruptTag) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .AesEncryptionKey(128)
                                             .BlockMode(BlockMode::GCM)
                                             .Padding(PaddingMode::NONE)
                                             .Authorization(TAG_MIN_MAC_LENGTH, 128)));

    string aad = "1234567890123456";
    string message = "123456789012345678901234567890123456";

    auto params = AuthorizationSetBuilder()
                      .BlockMode(BlockMode::GCM)
                      .Padding(PaddingMode::NONE)
                      .Authorization(TAG_MAC_LENGTH, 128);

    auto finish_params =
        AuthorizationSetBuilder().Authorization(TAG_ASSOCIATED_DATA, aad.data(), aad.size());

    // Encrypt
    AuthorizationSet begin_out_params;
    EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::ENCRYPT, params, &begin_out_params));
    string ciphertext;
    AuthorizationSet finish_out_params;
    EXPECT_EQ(ErrorCode::OK, Finish(op_handle_, finish_params, message, "" /* signature */,
                                    &finish_out_params, &ciphertext));
    EXPECT_TRUE(finish_out_params.empty());

    // Corrupt tag
    ++(*ciphertext.rbegin());

    // Grab nonce
    params.push_back(begin_out_params);

    // Decrypt.
    EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::DECRYPT, params));
    string plaintext;
    EXPECT_EQ(ErrorCode::VERIFICATION_FAILED,
              Finish(op_handle_, finish_params, ciphertext, "" /* signature */, &finish_out_params,
                     &plaintext));
    EXPECT_TRUE(finish_out_params.empty());
}

/*
 * EncryptionOperationsTest.TripleDesEcbRoundTripSuccess
 *
 * Verifies that 3DES is basically functional.
 */
TEST_P(EncryptionOperationsTest, TripleDesEcbRoundTripSuccess) {
    auto auths = AuthorizationSetBuilder()
                     .TripleDesEncryptionKey(168)
                     .BlockMode(BlockMode::ECB)
                     .Authorization(TAG_NO_AUTH_REQUIRED)
                     .Padding(PaddingMode::NONE);

    ASSERT_EQ(ErrorCode::OK, GenerateKey(auths));
    // Two-block message.
    string message = "1234567890123456";
    auto inParams = AuthorizationSetBuilder().BlockMode(BlockMode::ECB).Padding(PaddingMode::NONE);
    string ciphertext1 = EncryptMessage(message, inParams);
    EXPECT_EQ(message.size(), ciphertext1.size());

    string ciphertext2 = EncryptMessage(string(message), inParams);
    EXPECT_EQ(message.size(), ciphertext2.size());

    // ECB is deterministic.
    EXPECT_EQ(ciphertext1, ciphertext2);

    string plaintext = DecryptMessage(ciphertext1, inParams);
    EXPECT_EQ(message, plaintext);
}

/*
 * EncryptionOperationsTest.TripleDesEcbNotAuthorized
 *
 * Verifies that CBC keys reject ECB usage.
 */
TEST_P(EncryptionOperationsTest, TripleDesEcbNotAuthorized) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .TripleDesEncryptionKey(168)
                                             .BlockMode(BlockMode::CBC)
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .Padding(PaddingMode::NONE)));

    auto inParams = AuthorizationSetBuilder().BlockMode(BlockMode::ECB).Padding(PaddingMode::NONE);
    EXPECT_EQ(ErrorCode::INCOMPATIBLE_BLOCK_MODE, Begin(KeyPurpose::ENCRYPT, inParams));
}

/*
 * EncryptionOperationsTest.TripleDesEcbPkcs7Padding
 *
 * Tests ECB mode with PKCS#7 padding, various message sizes.
 */
TEST_P(EncryptionOperationsTest, TripleDesEcbPkcs7Padding) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .TripleDesEncryptionKey(168)
                                             .BlockMode(BlockMode::ECB)
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .Padding(PaddingMode::PKCS7)));

    for (size_t i = 0; i < 32; ++i) {
        string message(i, 'a');
        auto inParams =
            AuthorizationSetBuilder().BlockMode(BlockMode::ECB).Padding(PaddingMode::PKCS7);
        string ciphertext = EncryptMessage(message, inParams);
        EXPECT_EQ(i + 8 - (i % 8), ciphertext.size());
        string plaintext = DecryptMessage(ciphertext, inParams);
        EXPECT_EQ(message, plaintext);
    }
}

/*
 * EncryptionOperationsTest.TripleDesEcbNoPaddingKeyWithPkcs7Padding
 *
 * Verifies that keys configured for no padding reject PKCS7 padding
 */
TEST_P(EncryptionOperationsTest, TripleDesEcbNoPaddingKeyWithPkcs7Padding) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .TripleDesEncryptionKey(168)
                                             .BlockMode(BlockMode::ECB)
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .Padding(PaddingMode::NONE)));
    for (size_t i = 0; i < 32; ++i) {
        auto inParams =
            AuthorizationSetBuilder().BlockMode(BlockMode::ECB).Padding(PaddingMode::PKCS7);
        EXPECT_EQ(ErrorCode::INCOMPATIBLE_PADDING_MODE, Begin(KeyPurpose::ENCRYPT, inParams));
    }
}

/*
 * EncryptionOperationsTest.TripleDesEcbPkcs7PaddingCorrupted
 *
 * Verifies that corrupted padding is detected.
 */
TEST_P(EncryptionOperationsTest, TripleDesEcbPkcs7PaddingCorrupted) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .TripleDesEncryptionKey(168)
                                             .BlockMode(BlockMode::ECB)
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .Padding(PaddingMode::PKCS7)));

    string message = "a";
    string ciphertext = EncryptMessage(message, BlockMode::ECB, PaddingMode::PKCS7);
    EXPECT_EQ(8U, ciphertext.size());
    EXPECT_NE(ciphertext, message);
    ++ciphertext[ciphertext.size() / 2];

    AuthorizationSetBuilder begin_params;
    begin_params.push_back(TAG_BLOCK_MODE, BlockMode::ECB);
    begin_params.push_back(TAG_PADDING, PaddingMode::PKCS7);
    EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::DECRYPT, begin_params));
    string plaintext;
    size_t input_consumed;
    EXPECT_EQ(ErrorCode::OK, Update(ciphertext, &plaintext, &input_consumed));
    EXPECT_EQ(ciphertext.size(), input_consumed);
    EXPECT_EQ(ErrorCode::INVALID_ARGUMENT, Finish(&plaintext));
}

struct TripleDesTestVector {
    const char* name;
    const KeyPurpose purpose;
    const BlockMode block_mode;
    const PaddingMode padding_mode;
    const char* key;
    const char* iv;
    const char* input;
    const char* output;
};

// These test vectors are from NIST CAVP, plus a few custom variants to test padding, since all of
// the NIST vectors are multiples of the block size.
static const TripleDesTestVector kTripleDesTestVectors[] = {
    {
        "TECBMMT3 Encrypt 0", KeyPurpose::ENCRYPT, BlockMode::ECB, PaddingMode::NONE,
        "a2b5bc67da13dc92cd9d344aa238544a0e1fa79ef76810cd",  // key
        "",                                                  // IV
        "329d86bdf1bc5af4",                                  // input
        "d946c2756d78633f",                                  // output
    },
    {
        "TECBMMT3 Encrypt 1", KeyPurpose::ENCRYPT, BlockMode::ECB, PaddingMode::NONE,
        "49e692290d2a5e46bace79b9648a4c5d491004c262dc9d49",  // key
        "",                                                  // IV
        "6b1540781b01ce1997adae102dbf3c5b",                  // input
        "4d0dc182d6e481ac4a3dc6ab6976ccae",                  // output
    },
    {
        "TECBMMT3 Decrypt 0", KeyPurpose::DECRYPT, BlockMode::ECB, PaddingMode::NONE,
        "52daec2ac7dc1958377392682f37860b2cc1ea2304bab0e9",  // key
        "",                                                  // IV
        "6daad94ce08acfe7",                                  // input
        "660e7d32dcc90e79",                                  // output
    },
    {
        "TECBMMT3 Decrypt 1", KeyPurpose::DECRYPT, BlockMode::ECB, PaddingMode::NONE,
        "7f8fe3d3f4a48394fb682c2919926d6ddfce8932529229ce",  // key
        "",                                                  // IV
        "e9653a0a1f05d31b9acd12d73aa9879d",                  // input
        "9b2ae9d998efe62f1b592e7e1df8ff38",                  // output
    },
    {
        "TCBCMMT3 Encrypt 0", KeyPurpose::ENCRYPT, BlockMode::CBC, PaddingMode::NONE,
        "b5cb1504802326c73df186e3e352a20de643b0d63ee30e37",  // key
        "43f791134c5647ba",                                  // IV
        "dcc153cef81d6f24",                                  // input
        "92538bd8af18d3ba",                                  // output
    },
    {
        "TCBCMMT3 Encrypt 1", KeyPurpose::ENCRYPT, BlockMode::CBC, PaddingMode::NONE,
        "a49d7564199e97cb529d2c9d97bf2f98d35edf57ba1f7358",  // key
        "c2e999cb6249023c",                                  // IV
        "c689aee38a301bb316da75db36f110b5",                  // input
        "e9afaba5ec75ea1bbe65506655bb4ecb",                  // output
    },
    {
        "TCBCMMT3 Encrypt 1 PKCS7 variant", KeyPurpose::ENCRYPT, BlockMode::CBC, PaddingMode::PKCS7,
        "a49d7564199e97cb529d2c9d97bf2f98d35edf57ba1f7358",  // key
        "c2e999cb6249023c",                                  // IV
        "c689aee38a301bb316da75db36f110b500",                // input
        "e9afaba5ec75ea1bbe65506655bb4ecb825aa27ec0656156",  // output
    },
    {
        "TCBCMMT3 Encrypt 1 PKCS7 decrypted", KeyPurpose::DECRYPT, BlockMode::CBC,
        PaddingMode::PKCS7,
        "a49d7564199e97cb529d2c9d97bf2f98d35edf57ba1f7358",  // key
        "c2e999cb6249023c",                                  // IV
        "e9afaba5ec75ea1bbe65506655bb4ecb825aa27ec0656156",  // input
        "c689aee38a301bb316da75db36f110b500",                // output
    },
    {
        "TCBCMMT3 Decrypt 0", KeyPurpose::DECRYPT, BlockMode::CBC, PaddingMode::NONE,
        "5eb6040d46082c7aa7d06dfd08dfeac8c18364c1548c3ba1",  // key
        "41746c7e442d3681",                                  // IV
        "c53a7b0ec40600fe",                                  // input
        "d4f00eb455de1034",                                  // output
    },
    {
        "TCBCMMT3 Decrypt 1", KeyPurpose::DECRYPT, BlockMode::CBC, PaddingMode::NONE,
        "5b1cce7c0dc1ec49130dfb4af45785ab9179e567f2c7d549",  // key
        "3982bc02c3727d45",                                  // IV
        "6006f10adef52991fcc777a1238bbb65",                  // input
        "edae09288e9e3bc05746d872b48e3b29",                  // output
    },
};

/*
 * EncryptionOperationsTest.TripleDesTestVector
 *
 * Verifies that NIST (plus a few extra) test vectors produce the correct results.
 */
TEST_P(EncryptionOperationsTest, TripleDesTestVector) {
    constexpr size_t num_tests = sizeof(kTripleDesTestVectors) / sizeof(TripleDesTestVector);
    for (auto* test = kTripleDesTestVectors; test < kTripleDesTestVectors + num_tests; ++test) {
        SCOPED_TRACE(test->name);
        CheckTripleDesTestVector(test->purpose, test->block_mode, test->padding_mode,
                                 hex2str(test->key), hex2str(test->iv), hex2str(test->input),
                                 hex2str(test->output));
    }
}

/*
 * EncryptionOperationsTest.TripleDesCbcRoundTripSuccess
 *
 * Validates CBC mode functionality.
 */
TEST_P(EncryptionOperationsTest, TripleDesCbcRoundTripSuccess) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .TripleDesEncryptionKey(168)
                                             .BlockMode(BlockMode::CBC)
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .Padding(PaddingMode::NONE)));
    // Two-block message.
    string message = "1234567890123456";
    HidlBuf iv1;
    string ciphertext1 = EncryptMessage(message, BlockMode::CBC, PaddingMode::NONE, &iv1);
    EXPECT_EQ(message.size(), ciphertext1.size());

    HidlBuf iv2;
    string ciphertext2 = EncryptMessage(message, BlockMode::CBC, PaddingMode::NONE, &iv2);
    EXPECT_EQ(message.size(), ciphertext2.size());

    // IVs should be random, so ciphertexts should differ.
    EXPECT_NE(iv1, iv2);
    EXPECT_NE(ciphertext1, ciphertext2);

    string plaintext = DecryptMessage(ciphertext1, BlockMode::CBC, PaddingMode::NONE, iv1);
    EXPECT_EQ(message, plaintext);
}

/*
 * EncryptionOperationsTest.TripleDesCallerIv
 *
 * Validates that 3DES keys can allow caller-specified IVs, and use them correctly.
 */
TEST_P(EncryptionOperationsTest, TripleDesCallerIv) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .TripleDesEncryptionKey(168)
                                             .BlockMode(BlockMode::CBC)
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .Authorization(TAG_CALLER_NONCE)
                                             .Padding(PaddingMode::NONE)));
    string message = "1234567890123456";
    HidlBuf iv;
    // Don't specify IV, should get a random one.
    string ciphertext1 = EncryptMessage(message, BlockMode::CBC, PaddingMode::NONE, &iv);
    EXPECT_EQ(message.size(), ciphertext1.size());
    EXPECT_EQ(8U, iv.size());

    string plaintext = DecryptMessage(ciphertext1, BlockMode::CBC, PaddingMode::NONE, iv);
    EXPECT_EQ(message, plaintext);

    // Now specify an IV, should also work.
    iv = HidlBuf("abcdefgh");
    string ciphertext2 = EncryptMessage(message, BlockMode::CBC, PaddingMode::NONE, iv);

    // Decrypt with correct IV.
    plaintext = DecryptMessage(ciphertext2, BlockMode::CBC, PaddingMode::NONE, iv);
    EXPECT_EQ(message, plaintext);

    // Now try with wrong IV.
    plaintext = DecryptMessage(ciphertext2, BlockMode::CBC, PaddingMode::NONE, HidlBuf("aaaaaaaa"));
    EXPECT_NE(message, plaintext);
}

/*
 * EncryptionOperationsTest, TripleDesCallerNonceProhibited.
 *
 * Verifies that 3DES keys without TAG_CALLER_NONCE do not allow caller-specified IVS.
 */
TEST_P(EncryptionOperationsTest, TripleDesCallerNonceProhibited) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .TripleDesEncryptionKey(168)
                                             .BlockMode(BlockMode::CBC)
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .Padding(PaddingMode::NONE)));

    string message = "12345678901234567890123456789012";
    HidlBuf iv;
    // Don't specify nonce, should get a random one.
    string ciphertext1 = EncryptMessage(message, BlockMode::CBC, PaddingMode::NONE, &iv);
    EXPECT_EQ(message.size(), ciphertext1.size());
    EXPECT_EQ(8U, iv.size());

    string plaintext = DecryptMessage(ciphertext1, BlockMode::CBC, PaddingMode::NONE, iv);
    EXPECT_EQ(message, plaintext);

    // Now specify a nonce, should fail.
    auto input_params = AuthorizationSetBuilder()
                            .Authorization(TAG_NONCE, HidlBuf("abcdefgh"))
                            .BlockMode(BlockMode::CBC)
                            .Padding(PaddingMode::NONE);
    AuthorizationSet output_params;
    EXPECT_EQ(ErrorCode::CALLER_NONCE_PROHIBITED,
              Begin(KeyPurpose::ENCRYPT, input_params, &output_params));
}

/*
 * EncryptionOperationsTest.TripleDesCbcNotAuthorized
 *
 * Verifies that 3DES ECB-only keys do not allow CBC usage.
 */
TEST_P(EncryptionOperationsTest, TripleDesCbcNotAuthorized) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .TripleDesEncryptionKey(168)
                                             .BlockMode(BlockMode::ECB)
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .Padding(PaddingMode::NONE)));
    // Two-block message.
    string message = "1234567890123456";
    auto begin_params =
        AuthorizationSetBuilder().BlockMode(BlockMode::CBC).Padding(PaddingMode::NONE);
    EXPECT_EQ(ErrorCode::INCOMPATIBLE_BLOCK_MODE, Begin(KeyPurpose::ENCRYPT, begin_params));
}

/*
 * EncryptionOperationsTest.TripleDesCbcNoPaddingWrongInputSize
 *
 * Verifies that unpadded CBC operations reject inputs that are not a multiple of block size.
 */
TEST_P(EncryptionOperationsTest, TripleDesCbcNoPaddingWrongInputSize) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .TripleDesEncryptionKey(168)
                                             .BlockMode(BlockMode::CBC)
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .Padding(PaddingMode::NONE)));
    // Message is slightly shorter than two blocks.
    string message = "123456789012345";

    auto begin_params =
        AuthorizationSetBuilder().BlockMode(BlockMode::CBC).Padding(PaddingMode::NONE);
    AuthorizationSet output_params;
    EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::ENCRYPT, begin_params, &output_params));
    string ciphertext;
    EXPECT_EQ(ErrorCode::INVALID_INPUT_LENGTH, Finish(message, "", &ciphertext));
}

/*
 * EncryptionOperationsTest, TripleDesCbcPkcs7Padding.
 *
 * Verifies that PKCS7 padding works correctly in CBC mode.
 */
TEST_P(EncryptionOperationsTest, TripleDesCbcPkcs7Padding) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .TripleDesEncryptionKey(168)
                                             .BlockMode(BlockMode::CBC)
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .Padding(PaddingMode::PKCS7)));

    // Try various message lengths; all should work.
    for (size_t i = 0; i < 32; ++i) {
        string message(i, 'a');
        HidlBuf iv;
        string ciphertext = EncryptMessage(message, BlockMode::CBC, PaddingMode::PKCS7, &iv);
        EXPECT_EQ(i + 8 - (i % 8), ciphertext.size());
        string plaintext = DecryptMessage(ciphertext, BlockMode::CBC, PaddingMode::PKCS7, iv);
        EXPECT_EQ(message, plaintext);
    }
}

/*
 * EncryptionOperationsTest.TripleDesCbcNoPaddingKeyWithPkcs7Padding
 *
 * Verifies that a key that requires PKCS7 padding cannot be used in unpadded mode.
 */
TEST_P(EncryptionOperationsTest, TripleDesCbcNoPaddingKeyWithPkcs7Padding) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .TripleDesEncryptionKey(168)
                                             .BlockMode(BlockMode::CBC)
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .Padding(PaddingMode::NONE)));

    // Try various message lengths; all should fail.
    for (size_t i = 0; i < 32; ++i) {
        auto begin_params =
            AuthorizationSetBuilder().BlockMode(BlockMode::CBC).Padding(PaddingMode::PKCS7);
        EXPECT_EQ(ErrorCode::INCOMPATIBLE_PADDING_MODE, Begin(KeyPurpose::ENCRYPT, begin_params));
    }
}

/*
 * EncryptionOperationsTest.TripleDesCbcPkcs7PaddingCorrupted
 *
 * Verifies that corrupted PKCS7 padding is rejected during decryption.
 */
TEST_P(EncryptionOperationsTest, TripleDesCbcPkcs7PaddingCorrupted) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .TripleDesEncryptionKey(168)
                                             .BlockMode(BlockMode::CBC)
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .Padding(PaddingMode::PKCS7)));

    string message = "a";
    HidlBuf iv;
    string ciphertext = EncryptMessage(message, BlockMode::CBC, PaddingMode::PKCS7, &iv);
    EXPECT_EQ(8U, ciphertext.size());
    EXPECT_NE(ciphertext, message);
    ++ciphertext[ciphertext.size() / 2];

    auto begin_params = AuthorizationSetBuilder()
                            .BlockMode(BlockMode::CBC)
                            .Padding(PaddingMode::PKCS7)
                            .Authorization(TAG_NONCE, iv);
    EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::DECRYPT, begin_params));
    string plaintext;
    size_t input_consumed;
    EXPECT_EQ(ErrorCode::OK, Update(ciphertext, &plaintext, &input_consumed));
    EXPECT_EQ(ciphertext.size(), input_consumed);
    EXPECT_EQ(ErrorCode::INVALID_ARGUMENT, Finish(&plaintext));
}

/*
 * EncryptionOperationsTest, TripleDesCbcIncrementalNoPadding.
 *
 * Verifies that 3DES CBC works with many different input sizes.
 */
TEST_P(EncryptionOperationsTest, TripleDesCbcIncrementalNoPadding) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .TripleDesEncryptionKey(168)
                                             .BlockMode(BlockMode::CBC)
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .Padding(PaddingMode::NONE)));

    int increment = 7;
    string message(240, 'a');
    AuthorizationSet input_params =
        AuthorizationSetBuilder().BlockMode(BlockMode::CBC).Padding(PaddingMode::NONE);
    AuthorizationSet output_params;
    EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::ENCRYPT, input_params, &output_params));

    string ciphertext;
    size_t input_consumed;
    for (size_t i = 0; i < message.size(); i += increment)
        EXPECT_EQ(ErrorCode::OK,
                  Update(message.substr(i, increment), &ciphertext, &input_consumed));
    EXPECT_EQ(ErrorCode::OK, Finish(&ciphertext));
    EXPECT_EQ(message.size(), ciphertext.size());

    // Move TAG_NONCE into input_params
    input_params = output_params;
    input_params.push_back(TAG_BLOCK_MODE, BlockMode::CBC);
    input_params.push_back(TAG_PADDING, PaddingMode::NONE);
    output_params.Clear();

    EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::DECRYPT, input_params, &output_params));
    string plaintext;
    for (size_t i = 0; i < ciphertext.size(); i += increment)
        EXPECT_EQ(ErrorCode::OK,
                  Update(ciphertext.substr(i, increment), &plaintext, &input_consumed));
    EXPECT_EQ(ErrorCode::OK, Finish(&plaintext));
    EXPECT_EQ(ciphertext.size(), plaintext.size());
    EXPECT_EQ(message, plaintext);
}

typedef KeymasterHidlTest MaxOperationsTest;

/*
 * MaxOperationsTest.TestLimitAes
 *
 * Verifies that the max uses per boot tag works correctly with AES keys.
 */
TEST_P(MaxOperationsTest, TestLimitAes) {
    if (SecLevel() == SecurityLevel::STRONGBOX) return;

    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .AesEncryptionKey(128)
                                             .EcbMode()
                                             .Padding(PaddingMode::NONE)
                                             .Authorization(TAG_MAX_USES_PER_BOOT, 3)));

    string message = "1234567890123456";

    auto params = AuthorizationSetBuilder().EcbMode().Padding(PaddingMode::NONE);

    EncryptMessage(message, params);
    EncryptMessage(message, params);
    EncryptMessage(message, params);

    // Fourth time should fail.
    EXPECT_EQ(ErrorCode::KEY_MAX_OPS_EXCEEDED, Begin(KeyPurpose::ENCRYPT, params));
}

/*
 * MaxOperationsTest.TestLimitAes
 *
 * Verifies that the max uses per boot tag works correctly with RSA keys.
 */
TEST_P(MaxOperationsTest, TestLimitRsa) {
    if (SecLevel() == SecurityLevel::STRONGBOX) return;

    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .RsaSigningKey(1024, 65537)
                                             .NoDigestOrPadding()
                                             .Authorization(TAG_MAX_USES_PER_BOOT, 3)));

    string message = "1234567890123456";

    auto params = AuthorizationSetBuilder().NoDigestOrPadding();

    SignMessage(message, params);
    SignMessage(message, params);
    SignMessage(message, params);

    // Fourth time should fail.
    EXPECT_EQ(ErrorCode::KEY_MAX_OPS_EXCEEDED, Begin(KeyPurpose::SIGN, params));
}

typedef KeymasterHidlTest AddEntropyTest;

/*
 * AddEntropyTest.AddEntropy
 *
 * Verifies that the addRngEntropy method doesn't blow up.  There's no way to test that entropy is
 * actually added.
 */
TEST_P(AddEntropyTest, AddEntropy) {
    EXPECT_EQ(ErrorCode::OK, keymaster().addRngEntropy(HidlBuf("foo")));
}

/*
 * AddEntropyTest.AddEmptyEntropy
 *
 * Verifies that the addRngEntropy method doesn't blow up when given an empty buffer.
 */
TEST_P(AddEntropyTest, AddEmptyEntropy) {
    EXPECT_EQ(ErrorCode::OK, keymaster().addRngEntropy(HidlBuf()));
}

/*
 * AddEntropyTest.AddLargeEntropy
 *
 * Verifies that the addRngEntropy method doesn't blow up when given a largish amount of data.
 */
TEST_P(AddEntropyTest, AddLargeEntropy) {
    EXPECT_EQ(ErrorCode::OK, keymaster().addRngEntropy(HidlBuf(string(2 * 1024, 'a'))));
}

typedef KeymasterHidlTest AttestationTest;

/*
 * AttestationTest.RsaAttestation
 *
 * Verifies that attesting to RSA keys works and generates the expected output.
 */
TEST_P(AttestationTest, RsaAttestation) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .RsaSigningKey(2048, 65537)
                                             .Digest(Digest::SHA_2_256)
                                             .Padding(PaddingMode::RSA_PKCS1_1_5_SIGN)
                                             .Authorization(TAG_INCLUDE_UNIQUE_ID)));

    hidl_vec<hidl_vec<uint8_t>> cert_chain;
    ASSERT_EQ(ErrorCode::OK,
              AttestKey(AuthorizationSetBuilder()
                            .Authorization(TAG_ATTESTATION_CHALLENGE, HidlBuf("challenge"))
                            .Authorization(TAG_ATTESTATION_APPLICATION_ID, HidlBuf("foo")),
                        &cert_chain));
    EXPECT_GE(cert_chain.size(), 2U);

    string message = "12345678901234567890123456789012";
    string signature = SignMessage(message, AuthorizationSetBuilder()
                                                .Digest(Digest::SHA_2_256)
                                                .Padding(PaddingMode::RSA_PKCS1_1_5_SIGN));

    EXPECT_TRUE(verify_chain(cert_chain, message, signature));
    EXPECT_TRUE(verify_attestation_record("challenge", "foo",                     //
                                          key_characteristics_.softwareEnforced,  //
                                          key_characteristics_.hardwareEnforced,  //
                                          SecLevel(), cert_chain[0]));
}

/*
 * AttestationTest.RsaAttestationRequiresAppId
 *
 * Verifies that attesting to RSA requires app ID.
 */
TEST_P(AttestationTest, RsaAttestationRequiresAppId) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .RsaSigningKey(2048, 65537)
                                             .Digest(Digest::NONE)
                                             .Padding(PaddingMode::NONE)
                                             .Authorization(TAG_INCLUDE_UNIQUE_ID)));

    hidl_vec<hidl_vec<uint8_t>> cert_chain;
    EXPECT_EQ(ErrorCode::ATTESTATION_APPLICATION_ID_MISSING,
              AttestKey(AuthorizationSetBuilder().Authorization(TAG_ATTESTATION_CHALLENGE,
                                                                HidlBuf("challenge")),
                        &cert_chain));
}

/*
 * AttestationTest.EcAttestation
 *
 * Verifies that attesting to EC keys works and generates the expected output.
 */
TEST_P(AttestationTest, EcAttestation) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .EcdsaSigningKey(EcCurve::P_256)
                                             .Digest(Digest::SHA_2_256)
                                             .Authorization(TAG_INCLUDE_UNIQUE_ID)));

    hidl_vec<hidl_vec<uint8_t>> cert_chain;
    ASSERT_EQ(ErrorCode::OK,
              AttestKey(AuthorizationSetBuilder()
                            .Authorization(TAG_ATTESTATION_CHALLENGE, HidlBuf("challenge"))
                            .Authorization(TAG_ATTESTATION_APPLICATION_ID, HidlBuf("foo")),
                        &cert_chain));
    EXPECT_GE(cert_chain.size(), 2U);

    string message(1024, 'a');
    string signature = SignMessage(message, AuthorizationSetBuilder().Digest(Digest::SHA_2_256));

    EXPECT_TRUE(verify_chain(cert_chain, message, signature));
    EXPECT_TRUE(verify_attestation_record("challenge", "foo",                     //
                                          key_characteristics_.softwareEnforced,  //
                                          key_characteristics_.hardwareEnforced,  //
                                          SecLevel(), cert_chain[0]));
}

/*
 * AttestationTest.EcAttestationRequiresAttestationAppId
 *
 * Verifies that attesting to EC keys requires app ID
 */
TEST_P(AttestationTest, EcAttestationRequiresAttestationAppId) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .EcdsaSigningKey(EcCurve::P_256)
                                             .Digest(Digest::SHA_2_256)
                                             .Authorization(TAG_INCLUDE_UNIQUE_ID)));

    hidl_vec<hidl_vec<uint8_t>> cert_chain;
    EXPECT_EQ(ErrorCode::ATTESTATION_APPLICATION_ID_MISSING,
              AttestKey(AuthorizationSetBuilder().Authorization(TAG_ATTESTATION_CHALLENGE,
                                                                HidlBuf("challenge")),
                        &cert_chain));
}

/*
 * AttestationTest.AttestationApplicationIDLengthProperlyEncoded
 *
 * Verifies that the Attestation Application ID software enforced tag has a proper length encoding.
 * Some implementations break strict encoding rules by encoding a length between 127 and 256 in one
 * byte. Proper DER encoding specifies that for lengths greather than 127, one byte should be used
 * to specify how many following bytes will be used to encode the length.
 */
TEST_P(AttestationTest, AttestationApplicationIDLengthProperlyEncoded) {
    std::vector<uint32_t> app_id_lengths{143, 258};
    for (uint32_t length : app_id_lengths) {
        ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                     .Authorization(TAG_NO_AUTH_REQUIRED)
                                                     .EcdsaSigningKey(EcCurve::P_256)
                                                     .Digest(Digest::SHA_2_256)));

        hidl_vec<hidl_vec<uint8_t>> cert_chain;
        const string app_id(length, 'a');
        ASSERT_EQ(ErrorCode::OK,
                  AttestKey(AuthorizationSetBuilder()
                                    .Authorization(TAG_ATTESTATION_CHALLENGE, HidlBuf("challenge"))
                                    .Authorization(TAG_ATTESTATION_APPLICATION_ID, HidlBuf(app_id)),
                            &cert_chain));
        EXPECT_GE(cert_chain.size(), 2U);

        EXPECT_TRUE(verify_attestation_record("challenge", app_id,                    //
                                              key_characteristics_.softwareEnforced,  //
                                              key_characteristics_.hardwareEnforced,  //
                                              SecLevel(), cert_chain[0]));
        CheckedDeleteKey();
    }
}
/*
 * AttestationTest.AesAttestation
 *
 * Verifies that attesting to AES keys fails in the expected way.
 */
TEST_P(AttestationTest, AesAttestation) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .AesEncryptionKey(128)
                                             .EcbMode()
                                             .Padding(PaddingMode::PKCS7)));

    hidl_vec<hidl_vec<uint8_t>> cert_chain;
    EXPECT_EQ(ErrorCode::INCOMPATIBLE_ALGORITHM,
              AttestKey(AuthorizationSetBuilder()
                            .Authorization(TAG_ATTESTATION_CHALLENGE, HidlBuf("challenge"))
                            .Authorization(TAG_ATTESTATION_APPLICATION_ID, HidlBuf("foo")),
                        &cert_chain));
}

/*
 * AttestationTest.HmacAttestation
 *
 * Verifies that attesting to HMAC keys fails in the expected way.
 */
TEST_P(AttestationTest, HmacAttestation) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .HmacKey(128)
                                             .EcbMode()
                                             .Digest(Digest::SHA_2_256)
                                             .Authorization(TAG_MIN_MAC_LENGTH, 128)));

    hidl_vec<hidl_vec<uint8_t>> cert_chain;
    EXPECT_EQ(ErrorCode::INCOMPATIBLE_ALGORITHM,
              AttestKey(AuthorizationSetBuilder()
                            .Authorization(TAG_ATTESTATION_CHALLENGE, HidlBuf("challenge"))
                            .Authorization(TAG_ATTESTATION_APPLICATION_ID, HidlBuf("foo")),
                        &cert_chain));
}

typedef KeymasterHidlTest KeyDeletionTest;

/**
 * KeyDeletionTest.DeleteKey
 *
 * This test checks that if rollback protection is implemented, DeleteKey invalidates a formerly
 * valid key blob.
 */
TEST_P(KeyDeletionTest, DeleteKey) {
    auto error = GenerateKey(AuthorizationSetBuilder()
                                     .RsaSigningKey(2048, 65537)
                                     .Digest(Digest::NONE)
                                     .Padding(PaddingMode::NONE)
                                     .Authorization(TAG_NO_AUTH_REQUIRED)
                                     .Authorization(TAG_ROLLBACK_RESISTANCE));
    ASSERT_TRUE(error == ErrorCode::ROLLBACK_RESISTANCE_UNAVAILABLE || error == ErrorCode::OK);

    // Delete must work if rollback protection is implemented
    if (error == ErrorCode::OK) {
        AuthorizationSet hardwareEnforced(key_characteristics_.hardwareEnforced);
        ASSERT_TRUE(hardwareEnforced.Contains(TAG_ROLLBACK_RESISTANCE));

        ASSERT_EQ(ErrorCode::OK, DeleteKey(true /* keep key blob */));

        string message = "12345678901234567890123456789012";
        AuthorizationSet begin_out_params;
        EXPECT_EQ(ErrorCode::INVALID_KEY_BLOB,
                  Begin(KeyPurpose::SIGN, key_blob_,
                        AuthorizationSetBuilder().Digest(Digest::NONE).Padding(PaddingMode::NONE),
                        &begin_out_params, &op_handle_));
        AbortIfNeeded();
        key_blob_ = HidlBuf();
    }
}

/**
 * KeyDeletionTest.DeleteInvalidKey
 *
 * This test checks that the HAL excepts invalid key blobs..
 */
TEST_P(KeyDeletionTest, DeleteInvalidKey) {
    // Generate key just to check if rollback protection is implemented
    auto error = GenerateKey(AuthorizationSetBuilder()
                                     .RsaSigningKey(2048, 65537)
                                     .Digest(Digest::NONE)
                                     .Padding(PaddingMode::NONE)
                                     .Authorization(TAG_NO_AUTH_REQUIRED)
                                     .Authorization(TAG_ROLLBACK_RESISTANCE));
    ASSERT_TRUE(error == ErrorCode::ROLLBACK_RESISTANCE_UNAVAILABLE || error == ErrorCode::OK);

    // Delete must work if rollback protection is implemented
    if (error == ErrorCode::OK) {
        AuthorizationSet hardwareEnforced(key_characteristics_.hardwareEnforced);
        ASSERT_TRUE(hardwareEnforced.Contains(TAG_ROLLBACK_RESISTANCE));

        // Delete the key we don't care about the result at this point.
        DeleteKey();

        // Now create an invalid key blob and delete it.
        key_blob_ = HidlBuf("just some garbage data which is not a valid key blob");

        ASSERT_EQ(ErrorCode::OK, DeleteKey());
    }
}

/**
 * KeyDeletionTest.DeleteAllKeys
 *
 * This test is disarmed by default. To arm it use --arm_deleteAllKeys.
 *
 * BEWARE: This test has serious side effects. All user keys will be lost! This includes
 * FBE/FDE encryption keys, which means that the device will not even boot until after the
 * device has been wiped manually (e.g., fastboot flashall -w), and new FBE/FDE keys have
 * been provisioned. Use this test only on dedicated testing devices that have no valuable
 * credentials stored in Keystore/Keymaster.
 */
TEST_P(KeyDeletionTest, DeleteAllKeys) {
    if (!arm_deleteAllKeys) return;
    auto error = GenerateKey(AuthorizationSetBuilder()
                                     .RsaSigningKey(2048, 65537)
                                     .Digest(Digest::NONE)
                                     .Padding(PaddingMode::NONE)
                                     .Authorization(TAG_NO_AUTH_REQUIRED)
                                     .Authorization(TAG_ROLLBACK_RESISTANCE));
    ASSERT_TRUE(error == ErrorCode::ROLLBACK_RESISTANCE_UNAVAILABLE || error == ErrorCode::OK);

    // Delete must work if rollback protection is implemented
    if (error == ErrorCode::OK) {
        AuthorizationSet hardwareEnforced(key_characteristics_.hardwareEnforced);
        ASSERT_TRUE(hardwareEnforced.Contains(TAG_ROLLBACK_RESISTANCE));

        ASSERT_EQ(ErrorCode::OK, DeleteAllKeys());

        string message = "12345678901234567890123456789012";
        AuthorizationSet begin_out_params;

        EXPECT_EQ(ErrorCode::INVALID_KEY_BLOB,
                  Begin(KeyPurpose::SIGN, key_blob_,
                        AuthorizationSetBuilder().Digest(Digest::NONE).Padding(PaddingMode::NONE),
                        &begin_out_params, &op_handle_));
        AbortIfNeeded();
        key_blob_ = HidlBuf();
    }
}

using UpgradeKeyTest = KeymasterHidlTest;

/*
 * UpgradeKeyTest.UpgradeKey
 *
 * Verifies that calling upgrade key on an up-to-date key works (i.e. does nothing).
 */
TEST_P(UpgradeKeyTest, UpgradeKey) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .AesEncryptionKey(128)
                                             .Padding(PaddingMode::NONE)
                                             .Authorization(TAG_NO_AUTH_REQUIRED)));

    auto result = UpgradeKey(key_blob_);

    // Key doesn't need upgrading.  Should get okay, but no new key blob.
    EXPECT_EQ(result, std::make_pair(ErrorCode::OK, HidlBuf()));
}

using ClearOperationsTest = KeymasterHidlTest;

/*
 * ClearSlotsTest.TooManyOperations
 *
 * Verifies that TOO_MANY_OPERATIONS is returned after the max number of
 * operations are started without being finished or aborted. Also verifies
 * that aborting the operations clears the operations.
 *
 */
TEST_P(ClearOperationsTest, TooManyOperations) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .RsaEncryptionKey(2048, 65537)
                                             .Padding(PaddingMode::NONE)));

    auto params = AuthorizationSetBuilder().Padding(PaddingMode::NONE);
    int max_operations = SecLevel() == SecurityLevel::STRONGBOX ? 4 : 16;
    OperationHandle op_handles[max_operations];
    AuthorizationSet out_params;
    for(int i=0; i<max_operations; i++) {
        EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::ENCRYPT, key_blob_, params, &out_params, &(op_handles[i])));
    }
    EXPECT_EQ(ErrorCode::TOO_MANY_OPERATIONS,
         Begin(KeyPurpose::ENCRYPT, key_blob_, params, &out_params, &op_handle_));
    // Try again just in case there's a weird overflow bug
    EXPECT_EQ(ErrorCode::TOO_MANY_OPERATIONS,
         Begin(KeyPurpose::ENCRYPT, key_blob_, params, &out_params, &op_handle_));
    for(int i=0; i<max_operations; i++) {
        EXPECT_EQ(ErrorCode::OK, Abort(op_handles[i]));
    }
    EXPECT_EQ(ErrorCode::OK,
         Begin(KeyPurpose::ENCRYPT, key_blob_, params, &out_params, &op_handle_));
    AbortIfNeeded();
}

/*
 * ClearSlotsTest.ServiceDeath
 *
 * Verifies that the service is restarted after death and the ongoing
 * operations are cleared.
 */
TEST_P(ClearOperationsTest, ServiceDeath) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .RsaEncryptionKey(2048, 65537)
                                             .Padding(PaddingMode::NONE)));

    auto params = AuthorizationSetBuilder().Padding(PaddingMode::NONE);
    int max_operations = SecLevel() == SecurityLevel::STRONGBOX ? 4 : 16;
    OperationHandle op_handles[max_operations];
    AuthorizationSet out_params;
    for(int i=0; i<max_operations; i++) {
        EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::ENCRYPT, key_blob_, params, &out_params, &(op_handles[i])));
    }
    EXPECT_EQ(ErrorCode::TOO_MANY_OPERATIONS,
         Begin(KeyPurpose::ENCRYPT, key_blob_, params, &out_params, &op_handle_));

    DebugInfo debug_info;
    GetDebugInfo(&debug_info);
    kill(debug_info.pid, SIGKILL);
    // wait 1 second for keymaster to restart
    sleep(1);
    InitializeKeymaster();

    for(int i=0; i<max_operations; i++) {
        EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::ENCRYPT, key_blob_, params, &out_params, &(op_handles[i])));
    }
    EXPECT_EQ(ErrorCode::TOO_MANY_OPERATIONS,
         Begin(KeyPurpose::ENCRYPT, key_blob_, params, &out_params, &op_handle_));
    for(int i=0; i<max_operations; i++) {
        EXPECT_EQ(ErrorCode::OK, Abort(op_handles[i]));
    }
}

typedef KeymasterHidlTest TransportLimitTest;

/*
 * TransportLimitTest.LargeFinishInput
 *
 * Verifies that passing large input data to finish either succeeds or fails as expected.
 */
TEST_P(TransportLimitTest, LargeFinishInput) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                 .Authorization(TAG_NO_AUTH_REQUIRED)
                                                 .AesEncryptionKey(128)
                                                 .BlockMode(BlockMode::ECB)
                                                 .Padding(PaddingMode::NONE)));

    for (int msg_size = 10 /*1KB*/; msg_size <= 17 /*128KB*/; msg_size++) {
        auto cipher_params =
                AuthorizationSetBuilder().BlockMode(BlockMode::ECB).Padding(PaddingMode::NONE);

        AuthorizationSet out_params;
        EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::ENCRYPT, cipher_params, &out_params));

        string plain_message = std::string(1 << msg_size, 'x');
        string encrypted_message;
        auto rc = Finish(plain_message, &encrypted_message);

        if (rc == ErrorCode::OK) {
            EXPECT_EQ(plain_message.size(), encrypted_message.size())
                    << "Encrypt finish returned OK, but did not consume all of the given input";
        } else {
            EXPECT_EQ(ErrorCode::INVALID_INPUT_LENGTH, rc)
                    << "Encrypt finish failed in an unexpected way when given a large input";
            continue;
        }
        cipher_params.push_back(out_params);

        EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::DECRYPT, cipher_params));

        string decrypted_message;
        rc = Finish(encrypted_message, &decrypted_message);

        if (rc == ErrorCode::OK) {
            EXPECT_EQ(plain_message.size(), decrypted_message.size())
                    << "Decrypt finish returned OK, did not consume all of the given input";
        } else {
            EXPECT_EQ(ErrorCode::INVALID_INPUT_LENGTH, rc)
                    << "Encrypt finish failed in an unexpected way when given a large input";
        }
    }

    CheckedDeleteKey();
}

static const auto kKeymasterDeviceChoices =
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(IKeymasterDevice::descriptor));

INSTANTIATE_TEST_SUITE_P(PerInstance, NewKeyGenerationTest, kKeymasterDeviceChoices,
                         android::hardware::PrintInstanceNameToString);

INSTANTIATE_TEST_SUITE_P(PerInstance, ImportKeyTest, kKeymasterDeviceChoices,
                         android::hardware::PrintInstanceNameToString);

INSTANTIATE_TEST_SUITE_P(PerInstance, ImportWrappedKeyTest, kKeymasterDeviceChoices,
                         android::hardware::PrintInstanceNameToString);

INSTANTIATE_TEST_SUITE_P(PerInstance, SigningOperationsTest, kKeymasterDeviceChoices,
                         android::hardware::PrintInstanceNameToString);

INSTANTIATE_TEST_SUITE_P(PerInstance, VerificationOperationsTest, kKeymasterDeviceChoices,
                         android::hardware::PrintInstanceNameToString);

INSTANTIATE_TEST_SUITE_P(PerInstance, ExportKeyTest, kKeymasterDeviceChoices,
                         android::hardware::PrintInstanceNameToString);

INSTANTIATE_TEST_SUITE_P(PerInstance, EncryptionOperationsTest, kKeymasterDeviceChoices,
                         android::hardware::PrintInstanceNameToString);

INSTANTIATE_TEST_SUITE_P(PerInstance, MaxOperationsTest, kKeymasterDeviceChoices,
                         android::hardware::PrintInstanceNameToString);

INSTANTIATE_TEST_SUITE_P(PerInstance, AddEntropyTest, kKeymasterDeviceChoices,
                         android::hardware::PrintInstanceNameToString);

INSTANTIATE_TEST_SUITE_P(PerInstance, AttestationTest, kKeymasterDeviceChoices,
                         android::hardware::PrintInstanceNameToString);

INSTANTIATE_TEST_SUITE_P(PerInstance, KeyDeletionTest, kKeymasterDeviceChoices,
                         android::hardware::PrintInstanceNameToString);

INSTANTIATE_TEST_SUITE_P(PerInstance, TransportLimitTest, kKeymasterDeviceChoices,
                         android::hardware::PrintInstanceNameToString);

}  // namespace test
}  // namespace V4_0
}  // namespace keymaster
}  // namespace hardware
}  // namespace android

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    for (int i = 1; i < argc; ++i) {
        if (argv[i][0] == '-') {
            if (std::string(argv[i]) == "--arm_deleteAllKeys") {
                arm_deleteAllKeys = true;
            }
            if (std::string(argv[i]) == "--dump_attestations") {
                dump_Attestations = true;
            }
        }
    }
    int status = RUN_ALL_TESTS();
    ALOGI("Test result = %d", status);
    return status;
}
