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

#pragma once

#include <functional>
#include <string_view>

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <android-base/properties.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>
#include <gtest/gtest.h>
#include <openssl/x509.h>

#include <aidl/android/hardware/security/keymint/ErrorCode.h>
#include <aidl/android/hardware/security/keymint/IKeyMintDevice.h>
#include <aidl/android/hardware/security/keymint/MacedPublicKey.h>

#include <keymint_support/attestation_record.h>
#include <keymint_support/authorization_set.h>
#include <keymint_support/openssl_utils.h>

namespace aidl::android::hardware::security::keymint {

::std::ostream& operator<<(::std::ostream& os, const AuthorizationSet& set);

inline bool operator==(const keymint::AuthorizationSet& a, const keymint::AuthorizationSet& b) {
    return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin());
}

namespace test {

using ::android::sp;
using Status = ::ndk::ScopedAStatus;
using ::std::optional;
using ::std::shared_ptr;
using ::std::string;
using ::std::vector;

constexpr uint64_t kOpHandleSentinel = 0xFFFFFFFFFFFFFFFF;

class KeyMintAidlTestBase : public ::testing::TestWithParam<string> {
  public:
    struct KeyData {
        vector<uint8_t> blob;
        vector<KeyCharacteristics> characteristics;
    };

    static bool arm_deleteAllKeys;
    static bool dump_Attestations;

    void SetUp() override;
    void TearDown() override {
        if (key_blob_.size()) {
            CheckedDeleteKey();
        }
        AbortIfNeeded();
    }

    void InitializeKeyMint(std::shared_ptr<IKeyMintDevice> keyMint);
    IKeyMintDevice& keyMint() { return *keymint_; }
    int32_t AidlVersion();
    uint32_t os_version() { return os_version_; }
    uint32_t os_patch_level() { return os_patch_level_; }
    uint32_t vendor_patch_level() { return vendor_patch_level_; }
    uint32_t boot_patch_level(const vector<KeyCharacteristics>& key_characteristics);
    uint32_t boot_patch_level();
    bool isDeviceIdAttestationRequired();

    bool Curve25519Supported();

    ErrorCode GetReturnErrorCode(const Status& result);

    ErrorCode GenerateKey(const AuthorizationSet& key_desc, vector<uint8_t>* key_blob,
                          vector<KeyCharacteristics>* key_characteristics) {
        return GenerateKey(key_desc, std::nullopt /* attest_key */, key_blob, key_characteristics,
                           &cert_chain_);
    }
    ErrorCode GenerateKey(const AuthorizationSet& key_desc,
                          const optional<AttestationKey>& attest_key, vector<uint8_t>* key_blob,
                          vector<KeyCharacteristics>* key_characteristics,
                          vector<Certificate>* cert_chain);
    ErrorCode GenerateKey(const AuthorizationSet& key_desc,
                          const optional<AttestationKey>& attest_key = std::nullopt);

    // Generate key for implementations which do not support factory attestation.
    ErrorCode GenerateKeyWithSelfSignedAttestKey(const AuthorizationSet& attest_key_desc,
                                                 const AuthorizationSet& key_desc,
                                                 vector<uint8_t>* key_blob,
                                                 vector<KeyCharacteristics>* key_characteristics,
                                                 vector<Certificate>* cert_chain);

    ErrorCode GenerateKeyWithSelfSignedAttestKey(const AuthorizationSet& attest_key_desc,
                                                 const AuthorizationSet& key_desc,
                                                 vector<uint8_t>* key_blob,
                                                 vector<KeyCharacteristics>* key_characteristics) {
        return GenerateKeyWithSelfSignedAttestKey(attest_key_desc, key_desc, key_blob,
                                                  key_characteristics, &cert_chain_);
    }

    ErrorCode ImportKey(const AuthorizationSet& key_desc, KeyFormat format,
                        const string& key_material, vector<uint8_t>* key_blob,
                        vector<KeyCharacteristics>* key_characteristics);
    ErrorCode ImportKey(const AuthorizationSet& key_desc, KeyFormat format,
                        const string& key_material);

    ErrorCode ImportWrappedKey(string wrapped_key, string wrapping_key,
                               const AuthorizationSet& wrapping_key_desc, string masking_key,
                               const AuthorizationSet& unwrapping_params, int64_t password_sid,
                               int64_t biometric_sid);
    ErrorCode ImportWrappedKey(string wrapped_key, string wrapping_key,
                               const AuthorizationSet& wrapping_key_desc, string masking_key,
                               const AuthorizationSet& unwrapping_params) {
        return ImportWrappedKey(wrapped_key, wrapping_key, wrapping_key_desc, masking_key,
                                unwrapping_params, 0 /* password_sid */, 0 /* biometric_sid */);
    }

    ErrorCode GetCharacteristics(const vector<uint8_t>& key_blob, const vector<uint8_t>& app_id,
                                 const vector<uint8_t>& app_data,
                                 vector<KeyCharacteristics>* key_characteristics);
    ErrorCode GetCharacteristics(const vector<uint8_t>& key_blob,
                                 vector<KeyCharacteristics>* key_characteristics);

    void CheckCharacteristics(const vector<uint8_t>& key_blob,
                              const vector<KeyCharacteristics>& generate_characteristics);
    void CheckAppIdCharacteristics(const vector<uint8_t>& key_blob, std::string_view app_id_string,
                                   std::string_view app_data_string,
                                   const vector<KeyCharacteristics>& generate_characteristics);

    ErrorCode DeleteKey(vector<uint8_t>* key_blob, bool keep_key_blob = false);
    ErrorCode DeleteKey(bool keep_key_blob = false);

    ErrorCode DeleteAllKeys();

    ErrorCode DestroyAttestationIds();

    void CheckedDeleteKey(vector<uint8_t>* key_blob, bool keep_key_blob = false);
    void CheckedDeleteKey();

    ErrorCode Begin(KeyPurpose purpose, const vector<uint8_t>& key_blob,
                    const AuthorizationSet& in_params, AuthorizationSet* out_params,
                    std::shared_ptr<IKeyMintOperation>& op);
    ErrorCode Begin(KeyPurpose purpose, const vector<uint8_t>& key_blob,
                    const AuthorizationSet& in_params, AuthorizationSet* out_params);
    ErrorCode Begin(KeyPurpose purpose, const AuthorizationSet& in_params,
                    AuthorizationSet* out_params);
    ErrorCode Begin(KeyPurpose purpose, const AuthorizationSet& in_params);

    ErrorCode UpdateAad(const string& input);
    ErrorCode Update(const string& input, string* output);

    ErrorCode Finish(const string& message, const string& signature, string* output);
    ErrorCode Finish(const string& message, string* output) {
        return Finish(message, {} /* signature */, output);
    }
    ErrorCode Finish(string* output) { return Finish({} /* message */, output); }

    ErrorCode Abort();
    ErrorCode Abort(const shared_ptr<IKeyMintOperation>& op);
    void AbortIfNeeded();

    string ProcessMessage(const vector<uint8_t>& key_blob, KeyPurpose operation,
                          const string& message, const AuthorizationSet& in_params,
                          AuthorizationSet* out_params);
    std::tuple<ErrorCode, std::string /* processedMessage */> ProcessMessage(
            const vector<uint8_t>& key_blob, KeyPurpose operation, const std::string& message,
            const AuthorizationSet& in_params);
    string SignMessage(const vector<uint8_t>& key_blob, const string& message,
                       const AuthorizationSet& params);
    string SignMessage(const string& message, const AuthorizationSet& params);

    string MacMessage(const string& message, Digest digest, size_t mac_length);

    void CheckAesIncrementalEncryptOperation(BlockMode block_mode, int message_size);

    void CheckHmacTestVector(const string& key, const string& message, Digest digest,
                             const string& expected_mac);

    void CheckAesCtrTestVector(const string& key, const string& nonce, const string& message,
                               const string& expected_ciphertext);

    void CheckTripleDesTestVector(KeyPurpose purpose, BlockMode block_mode,
                                  PaddingMode padding_mode, const string& key, const string& iv,
                                  const string& input, const string& expected_output);

    void VerifyMessage(const vector<uint8_t>& key_blob, const string& message,
                       const string& signature, const AuthorizationSet& params);
    void VerifyMessage(const string& message, const string& signature,
                       const AuthorizationSet& params);
    void LocalVerifyMessage(const string& message, const string& signature,
                            const AuthorizationSet& params);

    string LocalRsaEncryptMessage(const string& message, const AuthorizationSet& params);
    string EncryptMessage(const vector<uint8_t>& key_blob, const string& message,
                          const AuthorizationSet& in_params, AuthorizationSet* out_params);
    string EncryptMessage(const string& message, const AuthorizationSet& params,
                          AuthorizationSet* out_params);
    string EncryptMessage(const string& message, const AuthorizationSet& params);
    string EncryptMessage(const string& message, BlockMode block_mode, PaddingMode padding);
    string EncryptMessage(const string& message, BlockMode block_mode, PaddingMode padding,
                          vector<uint8_t>* iv_out);
    string EncryptMessage(const string& message, BlockMode block_mode, PaddingMode padding,
                          const vector<uint8_t>& iv_in);
    string EncryptMessage(const string& message, BlockMode block_mode, PaddingMode padding,
                          uint8_t mac_length_bits, const vector<uint8_t>& iv_in);
    string EncryptMessage(const string& message, BlockMode block_mode, PaddingMode padding,
                          uint8_t mac_length_bits);

    string DecryptMessage(const vector<uint8_t>& key_blob, const string& ciphertext,
                          const AuthorizationSet& params);
    string DecryptMessage(const string& ciphertext, const AuthorizationSet& params);
    string DecryptMessage(const string& ciphertext, BlockMode block_mode, PaddingMode padding_mode,
                          const vector<uint8_t>& iv);

    std::pair<ErrorCode, vector<uint8_t>> UpgradeKey(const vector<uint8_t>& key_blob);

    template <typename TagType>
    std::tuple<KeyData /* aesKey */, KeyData /* hmacKey */, KeyData /* rsaKey */,
               KeyData /* ecdsaKey */>
    CreateTestKeys(
            TagType tagToTest, ErrorCode expectedReturn,
            std::function<void(AuthorizationSetBuilder*)> tagModifier =
                    [](AuthorizationSetBuilder*) {}) {
        /* AES */
        KeyData aesKeyData;
        AuthorizationSetBuilder aesBuilder = AuthorizationSetBuilder()
                                                     .AesEncryptionKey(128)
                                                     .Authorization(tagToTest)
                                                     .BlockMode(BlockMode::ECB)
                                                     .Padding(PaddingMode::NONE)
                                                     .Authorization(TAG_NO_AUTH_REQUIRED);
        tagModifier(&aesBuilder);
        ErrorCode errorCode =
                GenerateKey(aesBuilder, &aesKeyData.blob, &aesKeyData.characteristics);
        EXPECT_EQ(expectedReturn, errorCode);

        /* HMAC */
        KeyData hmacKeyData;
        AuthorizationSetBuilder hmacBuilder = AuthorizationSetBuilder()
                                                      .HmacKey(128)
                                                      .Authorization(tagToTest)
                                                      .Digest(Digest::SHA_2_256)
                                                      .Authorization(TAG_MIN_MAC_LENGTH, 128)
                                                      .Authorization(TAG_NO_AUTH_REQUIRED);
        tagModifier(&hmacBuilder);
        errorCode = GenerateKey(hmacBuilder, &hmacKeyData.blob, &hmacKeyData.characteristics);
        EXPECT_EQ(expectedReturn, errorCode);

        /* RSA */
        KeyData rsaKeyData;
        AuthorizationSetBuilder rsaBuilder = AuthorizationSetBuilder()
                                                     .RsaSigningKey(2048, 65537)
                                                     .Authorization(tagToTest)
                                                     .Digest(Digest::NONE)
                                                     .Padding(PaddingMode::NONE)
                                                     .Authorization(TAG_NO_AUTH_REQUIRED)
                                                     .SetDefaultValidity();
        tagModifier(&rsaBuilder);
        errorCode = GenerateKey(rsaBuilder, &rsaKeyData.blob, &rsaKeyData.characteristics);
        if (!(SecLevel() == SecurityLevel::STRONGBOX &&
              ErrorCode::ATTESTATION_KEYS_NOT_PROVISIONED == errorCode)) {
            EXPECT_EQ(expectedReturn, errorCode);
        }

        /* ECDSA */
        KeyData ecdsaKeyData;
        AuthorizationSetBuilder ecdsaBuilder = AuthorizationSetBuilder()
                                                       .EcdsaSigningKey(EcCurve::P_256)
                                                       .Authorization(tagToTest)
                                                       .Digest(Digest::SHA_2_256)
                                                       .Authorization(TAG_NO_AUTH_REQUIRED)
                                                       .SetDefaultValidity();
        tagModifier(&ecdsaBuilder);
        errorCode = GenerateKey(ecdsaBuilder, &ecdsaKeyData.blob, &ecdsaKeyData.characteristics);
        if (!(SecLevel() == SecurityLevel::STRONGBOX &&
              ErrorCode::ATTESTATION_KEYS_NOT_PROVISIONED == errorCode)) {
            EXPECT_EQ(expectedReturn, errorCode);
        }
        return {aesKeyData, hmacKeyData, rsaKeyData, ecdsaKeyData};
    }
    bool IsSecure() const { return securityLevel_ != SecurityLevel::SOFTWARE; }
    SecurityLevel SecLevel() const { return securityLevel_; }

    vector<uint32_t> ValidKeySizes(Algorithm algorithm);
    vector<uint32_t> InvalidKeySizes(Algorithm algorithm);

    vector<BlockMode> ValidBlockModes(Algorithm algorithm);
    vector<PaddingMode> ValidPaddingModes(Algorithm algorithm, BlockMode blockMode);
    vector<PaddingMode> InvalidPaddingModes(Algorithm algorithm, BlockMode blockMode);

    vector<EcCurve> ValidCurves();
    vector<EcCurve> InvalidCurves();

    vector<Digest> ValidDigests(bool withNone, bool withMD5);
    vector<uint64_t> ValidExponents();

    static vector<string> build_params() {
        auto params = ::android::getAidlHalInstanceNames(IKeyMintDevice::descriptor);
        return params;
    }

    std::shared_ptr<IKeyMintOperation> op_;
    vector<Certificate> cert_chain_;
    vector<uint8_t> key_blob_;
    vector<KeyCharacteristics> key_characteristics_;

    const vector<KeyParameter>& SecLevelAuthorizations(
            const vector<KeyCharacteristics>& key_characteristics);
    inline const vector<KeyParameter>& SecLevelAuthorizations() {
        return SecLevelAuthorizations(key_characteristics_);
    }
    const vector<KeyParameter>& SecLevelAuthorizations(
            const vector<KeyCharacteristics>& key_characteristics, SecurityLevel securityLevel);

    ErrorCode UseAesKey(const vector<uint8_t>& aesKeyBlob);
    ErrorCode UseHmacKey(const vector<uint8_t>& hmacKeyBlob);
    ErrorCode UseRsaKey(const vector<uint8_t>& rsaKeyBlob);
    ErrorCode UseEcdsaKey(const vector<uint8_t>& ecdsaKeyBlob);

  protected:
    std::shared_ptr<IKeyMintDevice> keymint_;
    uint32_t os_version_;
    uint32_t os_patch_level_;
    uint32_t vendor_patch_level_;
    bool timestamp_token_required_;

    SecurityLevel securityLevel_;
    string name_;
    string author_;
    long challenge_;
};

// If the given property is available, add it to the tag set under the given tag ID.
template <Tag tag>
void add_tag_from_prop(AuthorizationSetBuilder* tags, TypedTag<TagType::BYTES, tag> ttag,
                       const char* prop) {
    std::string prop_value = ::android::base::GetProperty(prop, /* default= */ "");
    if (!prop_value.empty()) {
        tags->Authorization(ttag, prop_value.data(), prop_value.size());
    }
}

// Return the VSR API level for this device.
int get_vsr_api_level();

// Indicate whether the test is running on a GSI image.
bool is_gsi_image();

vector<uint8_t> build_serial_blob(const uint64_t serial_int);
void verify_subject(const X509* cert, const string& subject, bool self_signed);
void verify_serial(X509* cert, const uint64_t expected_serial);
void verify_subject_and_serial(const Certificate& certificate,  //
                               const uint64_t expected_serial,  //
                               const string& subject, bool self_signed);
void verify_root_of_trust(const vector<uint8_t>& verified_boot_key,  //
                          bool device_locked,                        //
                          VerifiedBoot verified_boot_state,          //
                          const vector<uint8_t>& verified_boot_hash);
bool verify_attestation_record(int aidl_version,                       //
                               const string& challenge,                //
                               const string& app_id,                   //
                               AuthorizationSet expected_sw_enforced,  //
                               AuthorizationSet expected_hw_enforced,  //
                               SecurityLevel security_level,
                               const vector<uint8_t>& attestation_cert,
                               vector<uint8_t>* unique_id = nullptr);

string bin2hex(const vector<uint8_t>& data);
X509_Ptr parse_cert_blob(const vector<uint8_t>& blob);
vector<uint8_t> make_name_from_str(const string& name);
void check_maced_pubkey(const MacedPublicKey& macedPubKey, bool testMode,
                        vector<uint8_t>* payload_value);
void p256_pub_key(const vector<uint8_t>& coseKeyData, EVP_PKEY_Ptr* signingKey);
bool check_feature(const std::string& name);

AuthorizationSet HwEnforcedAuthorizations(const vector<KeyCharacteristics>& key_characteristics);
AuthorizationSet SwEnforcedAuthorizations(const vector<KeyCharacteristics>& key_characteristics);
::testing::AssertionResult ChainSignaturesAreValid(const vector<Certificate>& chain,
                                                   bool strict_issuer_check = true);

#define INSTANTIATE_KEYMINT_AIDL_TEST(name)                                          \
    INSTANTIATE_TEST_SUITE_P(PerInstance, name,                                      \
                             testing::ValuesIn(KeyMintAidlTestBase::build_params()), \
                             ::android::PrintInstanceNameToString);                  \
    GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(name);

}  // namespace test

}  // namespace aidl::android::hardware::security::keymint
