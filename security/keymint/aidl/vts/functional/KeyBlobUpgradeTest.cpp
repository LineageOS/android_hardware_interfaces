/*
 * Copyright (C) 2022 The Android Open Source Project
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

// The tests in this file are intended to be run manually, to allow testing of whether
// keyblob upgrade works correctly.  The manual procedure is roughly:
//
// 1) Run the "*Before*" subset of these tests with the `--keyblob_dir <dir>` command-line argument
//    so that keyblobs are saved to a directory on the device:
//
//      VtsAidlKeyMintTargetTest --gtest_filter="*KeyBlobUpgradeTest*Before*" \
//                               --keyblob_dir /data/local/tmp/keymint-blobs
//
//    All tests should pass, and the `UpgradeKeyBlobs` test should indicate that no keyblob
//    upgrades were needed.
//
// 2) Copy the generated keyblobs off the device into a safe place.
//
//      adb pull /data/local/tmp/keymint-blobs
//
// 3) Upgrade the device to a new version.
//
// 4) Push the saved keyblobs back onto the upgraded device.
//
//      adb push keymint-blobs /data/local/tmp/keymint-blobs
//
// 5) Run the "*After*" subset of these tests, with the following command-line arguments
//    `--keyblob_dir <dir>`: pointing to the directory with the keyblobs.
//    `--expect_upgrade {yes|no}` (Optional): To specify if users expect an upgrade on the keyBlobs,
//                                            will be "yes" by default.
//
//      VtsAidlKeyMintTargetTest --gtest_filter="*KeyBlobUpgradeTest*After*" \
//                               --keyblob_dir /data/local/tmp/keymint-blobs \
//                               --expect_upgrade {yes|no}
//
//    (Note that this skips the `CreateKeyBlobs` test, which would otherwise replace the saved
//    keyblobs with freshly generated ones.).
//
//    All tests should pass, and the `UpgradeKeyBlobs` test should have output that matches whether
//    upgrade was expected or not.

#define LOG_TAG "keymint_1_test"
#include <cutils/log.h>

#include <algorithm>
#include <fstream>
#include <iostream>

#include <unistd.h>

#include <openssl/curve25519.h>
#include <openssl/ec.h>
#include <openssl/evp.h>
#include <openssl/mem.h>
#include <openssl/x509v3.h>

#include "KeyMintAidlTestBase.h"

using aidl::android::hardware::security::keymint::KeyCharacteristics;

namespace aidl::android::hardware::security::keymint::test {

namespace {

// Names for individual key types to create and use.  Note that some the names
// induce specific behaviour, as indicated by the functions below.

std::vector<std::string> keyblob_names_tee = {
        "aes-key",        "aes-key-rr",      "des-key",           "hmac-key",
        "rsa-key",        "p256-key",        "ed25519-key",       "x25519-key",
        "rsa-attest-key", "p256-attest-key", "ed25519-attest-key"};

std::vector<std::string> keyblob_names_tee_no_25519 = {
        "aes-key", "aes-key-rr", "des-key",        "hmac-key",
        "rsa-key", "p256-key",   "rsa-attest-key", "p256-attest-key"};

std::vector<std::string> keyblob_names_sb = {"aes-key",        "aes-key-rr",     "des-key",
                                             "hmac-key",       "rsa-key",        "p256-key",
                                             "rsa-attest-key", "p256-attest-key"};

// Helper functions to detect particular key types based on the name.
bool requires_attest_key(const std::string& name) {
    return name.find("-attest-key") != std::string::npos;
}

bool requires_rr(const std::string& name) {
    return name.find("-rr") != std::string::npos;
}

bool is_asymmetric(const std::string& name) {
    return (name.find("rsa") != std::string::npos || name.find("25519") != std::string::npos ||
            name.find("p256") != std::string::npos);
}

std::string keyblob_subdir(const std::string& keyblob_dir, const std::string& full_name,
                           bool create) {
    if (keyblob_dir.empty()) {
        return "";
    }

    // Use a subdirectory for the specific instance, so two different KeyMint instances won't
    // clash with each other.
    size_t found = full_name.find_last_of('/');
    std::string subdir = keyblob_dir + "/" + full_name.substr(found + 1);

    if (create) {
        mkdir(keyblob_dir.c_str(), 0777);
        mkdir(subdir.c_str(), 0777);
    }
    return subdir;
}

void save_keyblob(const std::string& subdir, const std::string& name,
                  const vector<uint8_t>& keyblob,
                  const std::vector<KeyCharacteristics>& key_characteristics) {
    // Write the keyblob out to a file.
    std::string blobname(subdir + "/" + name + ".keyblob");
    std::ofstream blobfile(blobname, std::ios::out | std::ios::trunc | std::ios::binary);
    blobfile.write(reinterpret_cast<const char*>(keyblob.data()), keyblob.size());
    blobfile.close();

    // Dump the characteristics too.
    std::string charsname(subdir + "/" + name + ".chars");
    std::ofstream charsfile(charsname, std::ios::out | std::ios::trunc);
    charsfile << "{\n";
    for (const auto& characteristic : key_characteristics) {
        charsfile << "  " << characteristic.toString() << "\n";
    }
    charsfile << "}\n";
    charsfile.close();

    // Also write out a hexdump of the keyblob for convenience.
    std::string hexname(subdir + "/" + name + ".hex");
    std::ofstream hexfile(hexname, std::ios::out | std::ios::trunc);
    hexfile << bin2hex(keyblob) << "\n";
    hexfile.close();
}

void save_keyblob_and_cert(const std::string& subdir, const std::string& name,
                           const vector<uint8_t>& keyblob,
                           const std::vector<KeyCharacteristics>& key_characteristics,
                           const std::vector<Certificate>& cert_chain) {
    save_keyblob(subdir, name, keyblob, key_characteristics);

    if (is_asymmetric(name)) {
        // Dump the leaf certificate as DER.
        if (cert_chain.empty()) {
            FAIL() << "No cert available for " << name;
        } else {
            const vector<uint8_t>& certdata = cert_chain[0].encodedCertificate;
            std::string certname(subdir + "/" + name + ".cert");
            std::ofstream certfile(certname, std::ios::out | std::ios::trunc | std::ios::binary);
            certfile.write(reinterpret_cast<const char*>(certdata.data()), certdata.size());
            certfile.close();
        }
    }
}

void delete_keyblob(const std::string& subdir, const std::string& name) {
    std::string blobname(subdir + "/" + name + ".keyblob");
    unlink(blobname.c_str());
    std::string charsname(subdir + "/" + name + ".chars");
    unlink(charsname.c_str());
    std::string hexname(subdir + "/" + name + ".hex");
    unlink(hexname.c_str());
    std::string certname(subdir + "/" + name + ".cert");
    unlink(certname.c_str());
}

std::vector<uint8_t> load_file(const std::string& subdir, const std::string& name,
                               const std::string& suffix) {
    std::string blobname(subdir + "/" + name + suffix);
    std::ifstream blobfile(blobname, std::ios::in | std::ios::binary);

    std::vector<uint8_t> data((std::istreambuf_iterator<char>(blobfile)),
                              std::istreambuf_iterator<char>());
    return data;
}

std::vector<uint8_t> load_keyblob(const std::string& subdir, const std::string& name) {
    return load_file(subdir, name, ".keyblob");
}

std::vector<uint8_t> load_cert(const std::string& subdir, const std::string& name) {
    return load_file(subdir, name, ".cert");
}

}  // namespace

class KeyBlobUpgradeTest : public KeyMintAidlTestBase {
  protected:
    const std::vector<std::string>& keyblob_names() {
        if (SecLevel() == SecurityLevel::STRONGBOX) {
            return keyblob_names_sb;
        } else if (!Curve25519Supported()) {
            return keyblob_names_tee_no_25519;
        } else {
            return keyblob_names_tee;
        }
    }

    void UpgradeKeyBlobs(bool expectUpgrade) {
        std::string subdir = keyblob_subdir(keyblob_dir, GetParam(), /* create? */ false);
        if (subdir.empty()) {
            GTEST_SKIP() << "No keyblob directory provided";
        }

        for (std::string name : keyblob_names()) {
            if (requires_attest_key(name) && shouldSkipAttestKeyTest()) {
                std::cerr << "Skipping variant '" << name
                          << "' which requires ATTEST_KEY support that has been waivered\n";
                continue;
            }
            for (bool with_hidden : {false, true}) {
                std::string app_id;
                std::string app_data;
                auto builder = AuthorizationSetBuilder();
                if (with_hidden) {
                    // Build a variant keyblob that requires app_id/app_data
                    app_id = "appid";
                    app_data = "appdata";
                    builder.Authorization(TAG_APPLICATION_ID, "appid")
                            .Authorization(TAG_APPLICATION_DATA, "appdata");
                    name += "-hidden";
                }
                SCOPED_TRACE(testing::Message() << name);

                // Load the old format keyblob.
                std::vector<uint8_t> keyblob = load_keyblob(subdir, name);
                if (keyblob.empty()) {
                    if (requires_rr(name)) {
                        std::cerr << "Skipping missing keyblob file '" << name
                                  << "', assuming rollback resistance unavailable\n";
                    } else {
                        FAIL() << "Missing keyblob file '" << name << "'";
                    }
                    continue;
                }

                // An upgrade will either produce a new keyblob or no data (if upgrade isn't
                // needed).
                std::vector<uint8_t> upgraded_keyblob;
                Status result =
                        keymint_->upgradeKey(keyblob, builder.vector_data(), &upgraded_keyblob);
                ASSERT_EQ(ErrorCode::OK, GetReturnErrorCode(result));

                if (upgraded_keyblob.empty()) {
                    std::cerr << "Keyblob '" << name << "' did not require upgrade\n";
                    EXPECT_TRUE(!expectUpgrade) << "Keyblob '" << name << "' unexpectedly upgraded";
                } else {
                    // Ensure the old format keyblob is deleted (so any secure deletion data is
                    // cleaned up).
                    EXPECT_EQ(ErrorCode::OK, DeleteKey(&keyblob));

                    std::vector<uint8_t> app_id_v(app_id.begin(), app_id.end());
                    std::vector<uint8_t> app_data_v(app_data.begin(), app_data.end());
                    std::vector<KeyCharacteristics> key_characteristics;
                    result = keymint_->getKeyCharacteristics(upgraded_keyblob, app_id_v, app_data_v,
                                                             &key_characteristics);
                    ASSERT_EQ(ErrorCode::OK, GetReturnErrorCode(result))
                            << "Failed getKeyCharacteristics() after upgrade";

                    save_keyblob(subdir, name, upgraded_keyblob, key_characteristics);
                    // Cert file is left unchanged.
                    std::cerr << "Keyblob '" << name << "' upgraded\n";
                    EXPECT_TRUE(expectUpgrade)
                            << "Keyblob '" << name << "' unexpectedly left as-is";
                }
            }
        }
    }
};

// To save off keyblobs before upgrade, use:
//
//    VtsAidlKeyMintTargetTest --gtest_filter="*KeyBlobUpgradeTest.CreateKeyBlobs*" \
//                             --keyblob_dir /data/local/tmp/keymint-blobs
//
// Then copy the contents of the /data/local/tmp/keymint-blobs/ directory somewhere safe:
//
//    adb pull /data/local/tmp/keymint-blobs/
TEST_P(KeyBlobUpgradeTest, CreateKeyBlobsBefore) {
    std::string subdir = keyblob_subdir(keyblob_dir, GetParam(), /* create? */ true);

    std::map<const std::string, AuthorizationSetBuilder> keys_info = {
            {"aes-key", AuthorizationSetBuilder()
                                .AesEncryptionKey(256)
                                .BlockMode(BlockMode::ECB)
                                .Padding(PaddingMode::PKCS7)
                                .Authorization(TAG_NO_AUTH_REQUIRED)},
            {"aes-key-rr", AuthorizationSetBuilder()
                                   .AesEncryptionKey(256)
                                   .BlockMode(BlockMode::ECB)
                                   .Padding(PaddingMode::PKCS7)
                                   .Authorization(TAG_ROLLBACK_RESISTANCE)
                                   .Authorization(TAG_NO_AUTH_REQUIRED)},
            {"des-key", AuthorizationSetBuilder()
                                .TripleDesEncryptionKey(168)
                                .BlockMode(BlockMode::ECB)
                                .Padding(PaddingMode::PKCS7)
                                .Authorization(TAG_NO_AUTH_REQUIRED)},
            {"hmac-key", AuthorizationSetBuilder()
                                 .HmacKey(128)
                                 .Digest(Digest::SHA_2_256)
                                 .Authorization(TAG_MIN_MAC_LENGTH, 128)
                                 .Authorization(TAG_NO_AUTH_REQUIRED)},
            {"rsa-key", AuthorizationSetBuilder()
                                .RsaEncryptionKey(2048, 65537)
                                .Authorization(TAG_PURPOSE, KeyPurpose::SIGN)
                                .Digest(Digest::NONE)
                                .Digest(Digest::SHA_2_256)
                                .Padding(PaddingMode::NONE)
                                .Authorization(TAG_NO_AUTH_REQUIRED)
                                .SetDefaultValidity()},
            {
                    "p256-key",
                    AuthorizationSetBuilder()
                            .EcdsaSigningKey(EcCurve::P_256)
                            .Authorization(TAG_PURPOSE, KeyPurpose::AGREE_KEY)
                            .Digest(Digest::NONE)
                            .Digest(Digest::SHA_2_256)
                            .Authorization(TAG_NO_AUTH_REQUIRED)
                            .SetDefaultValidity(),
            },
            {
                    "ed25519-key",
                    AuthorizationSetBuilder()
                            .EcdsaSigningKey(EcCurve::CURVE_25519)
                            .Digest(Digest::NONE)
                            .Authorization(TAG_NO_AUTH_REQUIRED)
                            .SetDefaultValidity(),
            },
            {"x25519-key", AuthorizationSetBuilder()
                                   .Authorization(TAG_EC_CURVE, EcCurve::CURVE_25519)
                                   .Authorization(TAG_PURPOSE, KeyPurpose::AGREE_KEY)
                                   .Authorization(TAG_ALGORITHM, Algorithm::EC)
                                   .Authorization(TAG_NO_AUTH_REQUIRED)
                                   .SetDefaultValidity()},
            {"rsa-attest-key", AuthorizationSetBuilder()
                                       .RsaKey(2048, 65537)
                                       .AttestKey()
                                       .Authorization(TAG_NO_AUTH_REQUIRED)
                                       .SetDefaultValidity()},
            {
                    "p256-attest-key",
                    AuthorizationSetBuilder()
                            .EcdsaKey(EcCurve::P_256)
                            .AttestKey()
                            .Authorization(TAG_NO_AUTH_REQUIRED)
                            .SetDefaultValidity(),
            },
            {
                    "ed25519-attest-key",
                    AuthorizationSetBuilder()
                            .EcdsaKey(EcCurve::CURVE_25519)
                            .AttestKey()
                            .Authorization(TAG_NO_AUTH_REQUIRED)
                            .SetDefaultValidity(),
            }};

    for (std::string name : keyblob_names()) {
        if (requires_attest_key(name) && shouldSkipAttestKeyTest()) {
            std::cerr << "Skipping variant '" << name
                      << "' which requires ATTEST_KEY support that has been waivered\n";
            continue;
        }
        auto entry = keys_info.find(name);
        ASSERT_NE(entry, keys_info.end()) << "no builder for " << name;
        auto builder = entry->second;
        for (bool with_hidden : {false, true}) {
            if (with_hidden) {
                // Build a variant keyblob that requires app_id/app_data
                builder.Authorization(TAG_APPLICATION_ID, "appid")
                        .Authorization(TAG_APPLICATION_DATA, "appdata");
                name += "-hidden";
            }
            SCOPED_TRACE(testing::Message() << name);

            vector<uint8_t> keyblob;
            vector<KeyCharacteristics> key_characteristics;
            vector<Certificate> cert_chain;
            auto result =
                    GenerateKey(builder, std::nullopt, &keyblob, &key_characteristics, &cert_chain);

            if (requires_rr(name) && result == ErrorCode::ROLLBACK_RESISTANCE_UNAVAILABLE) {
                // Rollback resistance support is optional.
                std::cerr << "Skipping '" << name << "' key as rollback resistance unavailable\n";
                continue;
            }
            ASSERT_EQ(ErrorCode::OK, result) << " failed for " << name;

            if (!subdir.empty()) {
                save_keyblob_and_cert(subdir, name, keyblob, key_characteristics, cert_chain);
            }
        }
    }

    if (!subdir.empty()) {
        std::cerr << "Save generated keyblobs with:\n\n    adb pull " << keyblob_dir << "\n\n";
    }
}

TEST_P(KeyBlobUpgradeTest, UpgradeKeyBlobsBefore) {
    // Check that attempting to upgrade valid keyblobs does nothing.
    UpgradeKeyBlobs(/* expectUpgrade= */ false);
}

// To run this test:
//
// - save off some keyblobs before upgrade as per the CreateKeyBlobs test above.
// - upgrade the device to a version that should trigger keyblob upgrade (e.g. different patchlevel)
// - put the saved keyblobs back onto the upgraded device:
//
//     adb push keymint-blobs /data/local/tmp/keymint-blobs
//
// - run the test with:
//
//     VtsAidlKeyMintTargetTest --gtest_filter="*KeyBlobUpgradeTest.UpgradeKeyBlobsAfter*" \
//                              --keyblob_dir /data/local/tmp/keymint-blobs
//                              --expect_upgrade {yes|no}
//
// - this replaces the keyblob contents in that directory; if needed, save the upgraded keyblobs
//   with:
//      adb pull /data/local/tmp/keymint-blobs/
TEST_P(KeyBlobUpgradeTest, UpgradeKeyBlobsAfter) {
    bool expectUpgrade = true;  // this test expects upgrade to happen by default
    if (expect_upgrade.has_value() && expect_upgrade == false) {
        std::cout << "Not expecting key upgrade due to --expect_upgrade no\n";
        expectUpgrade = false;
    }
    UpgradeKeyBlobs(expectUpgrade);
}

// To run this test:
//
// - save off some keyblobs before upgrade as per the CreateKeyBlobs test above
// - if needed, upgrade the saved keyblobs as per the UpgradeKeyBlobs test above
// - run the test with:
//
//     VtsAidlKeyMintTargetTest --gtest_filter="*KeyBlobUpgradeTest.UseKeyBlobs*" \
//                              --keyblob_dir /data/local/tmp/keymint-blobs
TEST_P(KeyBlobUpgradeTest, UseKeyBlobsBeforeOrAfter) {
    std::string subdir = keyblob_subdir(keyblob_dir, GetParam(), /* create? */ false);
    if (subdir.empty()) {
        GTEST_SKIP() << "No keyblob directory provided with (e.g.) --keyblob_dir "
                        "/data/local/tmp/keymint-blobs";
    }

    for (std::string name : keyblob_names()) {
        if (requires_attest_key(name) && shouldSkipAttestKeyTest()) {
            std::cerr << "Skipping variant '" << name
                      << "' which requires ATTEST_KEY support that has been waivered\n";
            continue;
        }
        for (bool with_hidden : {false, true}) {
            auto builder = AuthorizationSetBuilder();
            if (with_hidden) {
                // Build a variant keyblob that requires app_id/app_data
                builder.Authorization(TAG_APPLICATION_ID, "appid")
                        .Authorization(TAG_APPLICATION_DATA, "appdata");
                name += "-hidden";
            }
            SCOPED_TRACE(testing::Message() << name);
            std::vector<uint8_t> keyblob = load_keyblob(subdir, name);
            if (keyblob.empty()) {
                if (requires_rr(name)) {
                    std::cerr << "Skipping missing keyblob file '" << name
                              << "', assuming rollback resistance unavailable\n";
                } else {
                    FAIL() << "Missing keyblob file '" << name << "'";
                }
                continue;
            }

            std::vector<uint8_t> cert;
            if (is_asymmetric(name)) {
                cert = load_cert(subdir, name);
            }

            // Perform an algorithm-specific operation with the keyblob.
            string message = "Hello World!";
            AuthorizationSet out_params;
            if (name.find("aes-key") != std::string::npos) {
                builder.BlockMode(BlockMode::ECB).Padding(PaddingMode::PKCS7);
                string ciphertext = EncryptMessage(keyblob, message, builder, &out_params);
                string plaintext = DecryptMessage(keyblob, ciphertext, builder);
                EXPECT_EQ(message, plaintext);
            } else if (name.find("des-key") != std::string::npos) {
                builder.BlockMode(BlockMode::ECB).Padding(PaddingMode::PKCS7);
                string ciphertext = EncryptMessage(keyblob, message, builder, &out_params);
                string plaintext = DecryptMessage(keyblob, ciphertext, builder);
                EXPECT_EQ(message, plaintext);
            } else if (name.find("hmac-key") != std::string::npos) {
                builder.Digest(Digest::SHA_2_256);
                auto sign_builder = builder;
                sign_builder.Authorization(TAG_MAC_LENGTH, 128);
                string tag = SignMessage(keyblob, message, sign_builder);
                VerifyMessage(keyblob, message, tag, builder);
            } else if (name.find("rsa-key") != std::string::npos) {
                builder.Digest(Digest::NONE).Padding(PaddingMode::NONE);
                string signature = SignMessage(keyblob, message, builder);
                LocalVerifyMessage(cert, message, signature, builder);
            } else if (name.find("p256-key") != std::string::npos) {
                builder.Digest(Digest::SHA_2_256);
                string signature = SignMessage(keyblob, message, builder);
                LocalVerifyMessage(cert, message, signature, builder);
            } else if (name.find("ed25519-key") != std::string::npos) {
                builder.Digest(Digest::NONE);
                string signature = SignMessage(keyblob, message, builder);
                LocalVerifyMessage(cert, message, signature, builder);
            } else if (name.find("x25519-key") != std::string::npos) {
                // Generate EC key on same curve locally (with access to private key material).
                uint8_t localPrivKeyData[32];
                uint8_t localPubKeyData[32];
                X25519_keypair(localPubKeyData, localPrivKeyData);
                EVP_PKEY_Ptr localPrivKey(EVP_PKEY_new_raw_private_key(
                        EVP_PKEY_X25519, nullptr, localPrivKeyData, sizeof(localPrivKeyData)));
                // Get encoded form of the public part of the locally generated key.
                unsigned char* p = nullptr;
                int localPublicKeySize = i2d_PUBKEY(localPrivKey.get(), &p);
                ASSERT_GT(localPublicKeySize, 0);
                vector<uint8_t> localPublicKey(
                        reinterpret_cast<const uint8_t*>(p),
                        reinterpret_cast<const uint8_t*>(p + localPublicKeySize));
                OPENSSL_free(p);

                // Agree on a key between local and KeyMint.
                string data;
                ASSERT_EQ(ErrorCode::OK,
                          Begin(KeyPurpose::AGREE_KEY, keyblob, builder, &out_params));
                ASSERT_EQ(ErrorCode::OK,
                          Finish(string(localPublicKey.begin(), localPublicKey.end()), &data));
                vector<uint8_t> keymint_data(data.begin(), data.end());

                // Extract the public key for the KeyMint key from the cert.
                X509_Ptr kmKeyCert(parse_cert_blob(cert));
                ASSERT_NE(kmKeyCert, nullptr);
                EVP_PKEY_Ptr kmPubKey = EVP_PKEY_Ptr(X509_get_pubkey(kmKeyCert.get()));
                ASSERT_NE(kmPubKey.get(), nullptr);

                size_t kmPubKeySize = 32;
                uint8_t kmPubKeyData[32];
                ASSERT_EQ(1,
                          EVP_PKEY_get_raw_public_key(kmPubKey.get(), kmPubKeyData, &kmPubKeySize));
                ASSERT_EQ(kmPubKeySize, 32);

                // Agree on a key between KeyMint and local.
                uint8_t sharedKey[32];
                ASSERT_EQ(1, X25519(sharedKey, localPrivKeyData, kmPubKeyData));
                vector<uint8_t> local_data(sharedKey, sharedKey + 32);

                // Both ways round should agree.
                EXPECT_EQ(keymint_data, local_data);
            } else if (requires_attest_key(name)) {
                // Covers rsa-attest-key, p256-attest-key, ed25519-attest-key.

                // Use attestation key to sign RSA signing key
                AttestationKey attest_key;
                attest_key.keyBlob = keyblob;
                attest_key.attestKeyParams = builder.vector_data();
                attest_key.issuerSubjectName = make_name_from_str("Android Keystore Key");
                vector<uint8_t> attested_key_blob;
                vector<KeyCharacteristics> attested_key_characteristics;
                vector<Certificate> attested_key_cert_chain;
                EXPECT_EQ(ErrorCode::OK,
                          GenerateKey(AuthorizationSetBuilder()
                                              .RsaSigningKey(2048, 65537)
                                              .Authorization(TAG_NO_AUTH_REQUIRED)
                                              .AttestationChallenge("challenge")
                                              .AttestationApplicationId("app-id")
                                              .SetDefaultValidity(),
                                      attest_key, &attested_key_blob, &attested_key_characteristics,
                                      &attested_key_cert_chain));
                KeyBlobDeleter(keymint_, attested_key_blob);
            } else {
                FAIL() << "Unexpected name: " << name;
            }
        }
    }
}

// This test target deletes any keys from the keyblob subdirectory that have rollback resistance
// enabled.
TEST_P(KeyBlobUpgradeTest, DeleteRRKeyBlobsAfter) {
    std::string subdir = keyblob_subdir(keyblob_dir, GetParam(), /* create? */ false);
    if (subdir.empty()) {
        GTEST_SKIP() << "No keyblob directory provided with (e.g.) --keyblob_dir "
                        "/data/local/tmp/keymint-blobs";
    }

    for (std::string name : keyblob_names()) {
        for (bool with_hidden : {false, true}) {
            auto builder = AuthorizationSetBuilder();
            if (with_hidden) {
                // Build a variant keyblob that requires app_id/app_data
                builder.Authorization(TAG_APPLICATION_ID, "appid")
                        .Authorization(TAG_APPLICATION_DATA, "appdata");
                name += "-hidden";
            }
            if (!requires_rr(name)) {
                std::cerr << "Skipping keyblob file '" << name
                          << "' which does not use rollback resistance\n";
                continue;
            }
            SCOPED_TRACE(testing::Message() << name);
            std::vector<uint8_t> keyblob = load_keyblob(subdir, name);
            if (keyblob.empty()) {
                std::cerr << "Skipping missing keyblob file '" << name
                          << "', assuming rollback resistance unavailable\n";
                continue;
            }

            // Delete the key
            ASSERT_EQ(ErrorCode::OK, DeleteKey(&keyblob));

            // Remove all files relating to the deleted key.
            std::cerr << "Deleting files for deleted key '" << name << "';\n";
            delete_keyblob(subdir, name);

            // Attempting to use the keyblob after deletion should fail.
            AuthorizationSet out_params;
            if (name.find("aes-key") != std::string::npos) {
                builder.BlockMode(BlockMode::ECB).Padding(PaddingMode::PKCS7);
                EXPECT_EQ(ErrorCode::INVALID_KEY_BLOB,
                          Begin(KeyPurpose::ENCRYPT, keyblob, builder, &out_params));
            } else {
                FAIL() << "Unexpected name: " << name;
            }
        }
    }
}

INSTANTIATE_KEYMINT_AIDL_TEST(KeyBlobUpgradeTest);

}  // namespace aidl::android::hardware::security::keymint::test
