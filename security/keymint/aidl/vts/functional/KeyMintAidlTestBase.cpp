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

#include "KeyMintAidlTestBase.h"

#include <chrono>
#include <unordered_set>
#include <vector>

#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <cppbor_parse.h>
#include <cutils/properties.h>
#include <gmock/gmock.h>
#include <openssl/mem.h>
#include <remote_prov/remote_prov_utils.h>

#include <keymaster/cppcose/cppcose.h>
#include <keymint_support/attestation_record.h>
#include <keymint_support/key_param_output.h>
#include <keymint_support/keymint_utils.h>
#include <keymint_support/openssl_utils.h>

namespace aidl::android::hardware::security::keymint {

using namespace cppcose;
using namespace std::literals::chrono_literals;
using std::endl;
using std::optional;
using std::unique_ptr;
using ::testing::AssertionFailure;
using ::testing::AssertionResult;
using ::testing::AssertionSuccess;
using ::testing::MatchesRegex;

::std::ostream& operator<<(::std::ostream& os, const AuthorizationSet& set) {
    if (set.size() == 0)
        os << "(Empty)" << ::std::endl;
    else {
        os << "\n";
        for (auto& entry : set) os << entry << ::std::endl;
    }
    return os;
}

namespace test {

namespace {
typedef KeyMintAidlTestBase::KeyData KeyData;
// Predicate for testing basic characteristics validity in generation or import.
bool KeyCharacteristicsBasicallyValid(SecurityLevel secLevel,
                                      const vector<KeyCharacteristics>& key_characteristics) {
    if (key_characteristics.empty()) return false;

    std::unordered_set<SecurityLevel> levels_seen;
    for (auto& entry : key_characteristics) {
        if (entry.authorizations.empty()) return false;

        // Just ignore the SecurityLevel::KEYSTORE as the KM won't do any enforcement on this.
        if (entry.securityLevel == SecurityLevel::KEYSTORE) continue;

        if (levels_seen.find(entry.securityLevel) != levels_seen.end()) return false;
        levels_seen.insert(entry.securityLevel);

        // Generally, we should only have one entry, at the same security level as the KM
        // instance.  There is an exception: StrongBox KM can have some authorizations that are
        // enforced by the TEE.
        bool isExpectedSecurityLevel = secLevel == entry.securityLevel ||
                                       (secLevel == SecurityLevel::STRONGBOX &&
                                        entry.securityLevel == SecurityLevel::TRUSTED_ENVIRONMENT);

        if (!isExpectedSecurityLevel) return false;
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

bool avb_verification_enabled() {
    char value[PROPERTY_VALUE_MAX];
    return property_get("ro.boot.vbmeta.device_state", value, "") != 0;
}

char nibble2hex[16] = {'0', '1', '2', '3', '4', '5', '6', '7',
                       '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

// Attestations don't contain everything in key authorization lists, so we need to filter the key
// lists to produce the lists that we expect to match the attestations.
auto kTagsToFilter = {
        Tag::BLOB_USAGE_REQUIREMENTS,  //
        Tag::CREATION_DATETIME,        //
        Tag::EC_CURVE,
        Tag::HARDWARE_TYPE,
        Tag::INCLUDE_UNIQUE_ID,
};

AuthorizationSet filtered_tags(const AuthorizationSet& set) {
    AuthorizationSet filtered;
    std::remove_copy_if(
            set.begin(), set.end(), std::back_inserter(filtered), [](const auto& entry) -> bool {
                return std::find(kTagsToFilter.begin(), kTagsToFilter.end(), entry.tag) !=
                       kTagsToFilter.end();
            });
    return filtered;
}

string x509NameToStr(X509_NAME* name) {
    char* s = X509_NAME_oneline(name, nullptr, 0);
    string retval(s);
    OPENSSL_free(s);
    return retval;
}

}  // namespace

bool KeyMintAidlTestBase::arm_deleteAllKeys = false;
bool KeyMintAidlTestBase::dump_Attestations = false;

ErrorCode KeyMintAidlTestBase::GetReturnErrorCode(const Status& result) {
    if (result.isOk()) return ErrorCode::OK;

    if (result.getExceptionCode() == EX_SERVICE_SPECIFIC) {
        return static_cast<ErrorCode>(result.getServiceSpecificError());
    }

    return ErrorCode::UNKNOWN_ERROR;
}

void KeyMintAidlTestBase::InitializeKeyMint(std::shared_ptr<IKeyMintDevice> keyMint) {
    ASSERT_NE(keyMint, nullptr);
    keymint_ = std::move(keyMint);

    KeyMintHardwareInfo info;
    ASSERT_TRUE(keymint_->getHardwareInfo(&info).isOk());

    securityLevel_ = info.securityLevel;
    name_.assign(info.keyMintName.begin(), info.keyMintName.end());
    author_.assign(info.keyMintAuthorName.begin(), info.keyMintAuthorName.end());

    os_version_ = getOsVersion();
    os_patch_level_ = getOsPatchlevel();
}

void KeyMintAidlTestBase::SetUp() {
    if (AServiceManager_isDeclared(GetParam().c_str())) {
        ::ndk::SpAIBinder binder(AServiceManager_waitForService(GetParam().c_str()));
        InitializeKeyMint(IKeyMintDevice::fromBinder(binder));
    } else {
        InitializeKeyMint(nullptr);
    }
}

ErrorCode KeyMintAidlTestBase::GenerateKey(const AuthorizationSet& key_desc,
                                           const optional<AttestationKey>& attest_key,
                                           vector<uint8_t>* key_blob,
                                           vector<KeyCharacteristics>* key_characteristics,
                                           vector<Certificate>* cert_chain) {
    EXPECT_NE(key_blob, nullptr) << "Key blob pointer must not be null.  Test bug";
    EXPECT_NE(key_characteristics, nullptr)
            << "Previous characteristics not deleted before generating key.  Test bug.";

    KeyCreationResult creationResult;
    Status result = keymint_->generateKey(key_desc.vector_data(), attest_key, &creationResult);
    if (result.isOk()) {
        EXPECT_PRED2(KeyCharacteristicsBasicallyValid, SecLevel(),
                     creationResult.keyCharacteristics);
        EXPECT_GT(creationResult.keyBlob.size(), 0);
        *key_blob = std::move(creationResult.keyBlob);
        *key_characteristics = std::move(creationResult.keyCharacteristics);
        *cert_chain = std::move(creationResult.certificateChain);

        auto algorithm = key_desc.GetTagValue(TAG_ALGORITHM);
        EXPECT_TRUE(algorithm);
        if (algorithm &&
            (algorithm.value() == Algorithm::RSA || algorithm.value() == Algorithm::EC)) {
            EXPECT_GE(cert_chain->size(), 1);
            if (key_desc.Contains(TAG_ATTESTATION_CHALLENGE)) {
                if (attest_key) {
                    EXPECT_EQ(cert_chain->size(), 1);
                } else {
                    EXPECT_GT(cert_chain->size(), 1);
                }
            }
        } else {
            // For symmetric keys there should be no certificates.
            EXPECT_EQ(cert_chain->size(), 0);
        }
    }

    return GetReturnErrorCode(result);
}

ErrorCode KeyMintAidlTestBase::GenerateKey(const AuthorizationSet& key_desc,
                                           const optional<AttestationKey>& attest_key) {
    return GenerateKey(key_desc, attest_key, &key_blob_, &key_characteristics_, &cert_chain_);
}

ErrorCode KeyMintAidlTestBase::ImportKey(const AuthorizationSet& key_desc, KeyFormat format,
                                         const string& key_material, vector<uint8_t>* key_blob,
                                         vector<KeyCharacteristics>* key_characteristics) {
    Status result;

    cert_chain_.clear();
    key_characteristics->clear();
    key_blob->clear();

    KeyCreationResult creationResult;
    result = keymint_->importKey(key_desc.vector_data(), format,
                                 vector<uint8_t>(key_material.begin(), key_material.end()),
                                 {} /* attestationSigningKeyBlob */, &creationResult);

    if (result.isOk()) {
        EXPECT_PRED2(KeyCharacteristicsBasicallyValid, SecLevel(),
                     creationResult.keyCharacteristics);
        EXPECT_GT(creationResult.keyBlob.size(), 0);

        *key_blob = std::move(creationResult.keyBlob);
        *key_characteristics = std::move(creationResult.keyCharacteristics);
        cert_chain_ = std::move(creationResult.certificateChain);

        auto algorithm = key_desc.GetTagValue(TAG_ALGORITHM);
        EXPECT_TRUE(algorithm);
        if (algorithm &&
            (algorithm.value() == Algorithm::RSA || algorithm.value() == Algorithm::EC)) {
            EXPECT_GE(cert_chain_.size(), 1);
            if (key_desc.Contains(TAG_ATTESTATION_CHALLENGE)) EXPECT_GT(cert_chain_.size(), 1);
        } else {
            // For symmetric keys there should be no certificates.
            EXPECT_EQ(cert_chain_.size(), 0);
        }
    }

    return GetReturnErrorCode(result);
}

ErrorCode KeyMintAidlTestBase::ImportKey(const AuthorizationSet& key_desc, KeyFormat format,
                                         const string& key_material) {
    return ImportKey(key_desc, format, key_material, &key_blob_, &key_characteristics_);
}

ErrorCode KeyMintAidlTestBase::ImportWrappedKey(string wrapped_key, string wrapping_key,
                                                const AuthorizationSet& wrapping_key_desc,
                                                string masking_key,
                                                const AuthorizationSet& unwrapping_params) {
    EXPECT_EQ(ErrorCode::OK, ImportKey(wrapping_key_desc, KeyFormat::PKCS8, wrapping_key));

    key_characteristics_.clear();

    KeyCreationResult creationResult;
    Status result = keymint_->importWrappedKey(
            vector<uint8_t>(wrapped_key.begin(), wrapped_key.end()), key_blob_,
            vector<uint8_t>(masking_key.begin(), masking_key.end()),
            unwrapping_params.vector_data(), 0 /* passwordSid */, 0 /* biometricSid */,
            &creationResult);

    if (result.isOk()) {
        EXPECT_PRED2(KeyCharacteristicsBasicallyValid, SecLevel(),
                     creationResult.keyCharacteristics);
        EXPECT_GT(creationResult.keyBlob.size(), 0);

        key_blob_ = std::move(creationResult.keyBlob);
        key_characteristics_ = std::move(creationResult.keyCharacteristics);
        cert_chain_ = std::move(creationResult.certificateChain);

        AuthorizationSet allAuths;
        for (auto& entry : key_characteristics_) {
            allAuths.push_back(AuthorizationSet(entry.authorizations));
        }
        auto algorithm = allAuths.GetTagValue(TAG_ALGORITHM);
        EXPECT_TRUE(algorithm);
        if (algorithm &&
            (algorithm.value() == Algorithm::RSA || algorithm.value() == Algorithm::EC)) {
            EXPECT_GE(cert_chain_.size(), 1);
        } else {
            // For symmetric keys there should be no certificates.
            EXPECT_EQ(cert_chain_.size(), 0);
        }
    }

    return GetReturnErrorCode(result);
}

ErrorCode KeyMintAidlTestBase::DeleteKey(vector<uint8_t>* key_blob, bool keep_key_blob) {
    Status result = keymint_->deleteKey(*key_blob);
    if (!keep_key_blob) {
        *key_blob = vector<uint8_t>();
    }

    EXPECT_TRUE(result.isOk()) << result.getServiceSpecificError() << endl;
    return GetReturnErrorCode(result);
}

ErrorCode KeyMintAidlTestBase::DeleteKey(bool keep_key_blob) {
    return DeleteKey(&key_blob_, keep_key_blob);
}

ErrorCode KeyMintAidlTestBase::DeleteAllKeys() {
    Status result = keymint_->deleteAllKeys();
    EXPECT_TRUE(result.isOk()) << result.getServiceSpecificError() << endl;
    return GetReturnErrorCode(result);
}

void KeyMintAidlTestBase::CheckedDeleteKey(vector<uint8_t>* key_blob, bool keep_key_blob) {
    ErrorCode result = DeleteKey(key_blob, keep_key_blob);
    EXPECT_TRUE(result == ErrorCode::OK || result == ErrorCode::UNIMPLEMENTED) << result << endl;
}

void KeyMintAidlTestBase::CheckedDeleteKey() {
    CheckedDeleteKey(&key_blob_);
}

ErrorCode KeyMintAidlTestBase::Begin(KeyPurpose purpose, const vector<uint8_t>& key_blob,
                                     const AuthorizationSet& in_params,
                                     AuthorizationSet* out_params,
                                     std::shared_ptr<IKeyMintOperation>& op) {
    SCOPED_TRACE("Begin");
    Status result;
    BeginResult out;
    result = keymint_->begin(purpose, key_blob, in_params.vector_data(), HardwareAuthToken(), &out);

    if (result.isOk()) {
        *out_params = out.params;
        challenge_ = out.challenge;
        op = out.operation;
    }

    return GetReturnErrorCode(result);
}

ErrorCode KeyMintAidlTestBase::Begin(KeyPurpose purpose, const vector<uint8_t>& key_blob,
                                     const AuthorizationSet& in_params,
                                     AuthorizationSet* out_params) {
    SCOPED_TRACE("Begin");
    Status result;
    BeginResult out;

    result = keymint_->begin(purpose, key_blob, in_params.vector_data(), HardwareAuthToken(), &out);

    if (result.isOk()) {
        *out_params = out.params;
        challenge_ = out.challenge;
        op_ = out.operation;
    }

    return GetReturnErrorCode(result);
}

ErrorCode KeyMintAidlTestBase::Begin(KeyPurpose purpose, const AuthorizationSet& in_params,
                                     AuthorizationSet* out_params) {
    SCOPED_TRACE("Begin");
    EXPECT_EQ(nullptr, op_);
    return Begin(purpose, key_blob_, in_params, out_params);
}

ErrorCode KeyMintAidlTestBase::Begin(KeyPurpose purpose, const AuthorizationSet& in_params) {
    SCOPED_TRACE("Begin");
    AuthorizationSet out_params;
    ErrorCode result = Begin(purpose, in_params, &out_params);
    EXPECT_TRUE(out_params.empty());
    return result;
}

ErrorCode KeyMintAidlTestBase::UpdateAad(const string& input) {
    return GetReturnErrorCode(op_->updateAad(vector<uint8_t>(input.begin(), input.end()),
                                             {} /* hardwareAuthToken */,
                                             {} /* verificationToken */));
}

ErrorCode KeyMintAidlTestBase::Update(const string& input, string* output) {
    SCOPED_TRACE("Update");

    Status result;
    if (!output) return ErrorCode::UNEXPECTED_NULL_POINTER;

    std::vector<uint8_t> o_put;
    result = op_->update(vector<uint8_t>(input.begin(), input.end()), {}, {}, &o_put);

    if (result.isOk()) output->append(o_put.begin(), o_put.end());

    return GetReturnErrorCode(result);
}

ErrorCode KeyMintAidlTestBase::Finish(const string& input, const string& signature,
                                      string* output) {
    SCOPED_TRACE("Finish");
    Status result;

    EXPECT_NE(op_, nullptr);
    if (!op_) return ErrorCode::UNEXPECTED_NULL_POINTER;

    vector<uint8_t> oPut;
    result = op_->finish(vector<uint8_t>(input.begin(), input.end()),
                         vector<uint8_t>(signature.begin(), signature.end()), {} /* authToken */,
                         {} /* timestampToken */, {} /* confirmationToken */, &oPut);

    if (result.isOk()) output->append(oPut.begin(), oPut.end());

    op_ = {};
    return GetReturnErrorCode(result);
}

ErrorCode KeyMintAidlTestBase::Abort(const std::shared_ptr<IKeyMintOperation>& op) {
    SCOPED_TRACE("Abort");

    EXPECT_NE(op, nullptr);
    if (!op) return ErrorCode::UNEXPECTED_NULL_POINTER;

    Status retval = op->abort();
    EXPECT_TRUE(retval.isOk());
    return static_cast<ErrorCode>(retval.getServiceSpecificError());
}

ErrorCode KeyMintAidlTestBase::Abort() {
    SCOPED_TRACE("Abort");

    EXPECT_NE(op_, nullptr);
    if (!op_) return ErrorCode::UNEXPECTED_NULL_POINTER;

    Status retval = op_->abort();
    return static_cast<ErrorCode>(retval.getServiceSpecificError());
}

void KeyMintAidlTestBase::AbortIfNeeded() {
    SCOPED_TRACE("AbortIfNeeded");
    if (op_) {
        EXPECT_EQ(ErrorCode::OK, Abort());
        op_.reset();
    }
}

auto KeyMintAidlTestBase::ProcessMessage(const vector<uint8_t>& key_blob, KeyPurpose operation,
                                         const string& message, const AuthorizationSet& in_params)
        -> std::tuple<ErrorCode, string> {
    AuthorizationSet begin_out_params;
    ErrorCode result = Begin(operation, key_blob, in_params, &begin_out_params);
    if (result != ErrorCode::OK) return {result, {}};

    string output;
    return {Finish(message, &output), output};
}

string KeyMintAidlTestBase::ProcessMessage(const vector<uint8_t>& key_blob, KeyPurpose operation,
                                           const string& message, const AuthorizationSet& in_params,
                                           AuthorizationSet* out_params) {
    SCOPED_TRACE("ProcessMessage");
    AuthorizationSet begin_out_params;
    ErrorCode result = Begin(operation, key_blob, in_params, out_params);
    EXPECT_EQ(ErrorCode::OK, result);
    if (result != ErrorCode::OK) {
        return "";
    }

    string output;
    EXPECT_EQ(ErrorCode::OK, Finish(message, &output));
    return output;
}

string KeyMintAidlTestBase::SignMessage(const vector<uint8_t>& key_blob, const string& message,
                                        const AuthorizationSet& params) {
    SCOPED_TRACE("SignMessage");
    AuthorizationSet out_params;
    string signature = ProcessMessage(key_blob, KeyPurpose::SIGN, message, params, &out_params);
    EXPECT_TRUE(out_params.empty());
    return signature;
}

string KeyMintAidlTestBase::SignMessage(const string& message, const AuthorizationSet& params) {
    SCOPED_TRACE("SignMessage");
    return SignMessage(key_blob_, message, params);
}

string KeyMintAidlTestBase::MacMessage(const string& message, Digest digest, size_t mac_length) {
    SCOPED_TRACE("MacMessage");
    return SignMessage(
            key_blob_, message,
            AuthorizationSetBuilder().Digest(digest).Authorization(TAG_MAC_LENGTH, mac_length));
}

void KeyMintAidlTestBase::CheckHmacTestVector(const string& key, const string& message,
                                              Digest digest, const string& expected_mac) {
    SCOPED_TRACE("CheckHmacTestVector");
    ASSERT_EQ(ErrorCode::OK,
              ImportKey(AuthorizationSetBuilder()
                                .Authorization(TAG_NO_AUTH_REQUIRED)
                                .HmacKey(key.size() * 8)
                                .Authorization(TAG_MIN_MAC_LENGTH, expected_mac.size() * 8)
                                .Digest(digest),
                        KeyFormat::RAW, key));
    string signature = MacMessage(message, digest, expected_mac.size() * 8);
    EXPECT_EQ(expected_mac, signature)
            << "Test vector didn't match for key of size " << key.size() << " message of size "
            << message.size() << " and digest " << digest;
    CheckedDeleteKey();
}

void KeyMintAidlTestBase::CheckAesCtrTestVector(const string& key, const string& nonce,
                                                const string& message,
                                                const string& expected_ciphertext) {
    SCOPED_TRACE("CheckAesCtrTestVector");
    ASSERT_EQ(ErrorCode::OK, ImportKey(AuthorizationSetBuilder()
                                               .Authorization(TAG_NO_AUTH_REQUIRED)
                                               .AesEncryptionKey(key.size() * 8)
                                               .BlockMode(BlockMode::CTR)
                                               .Authorization(TAG_CALLER_NONCE)
                                               .Padding(PaddingMode::NONE),
                                       KeyFormat::RAW, key));

    auto params = AuthorizationSetBuilder()
                          .Authorization(TAG_NONCE, nonce.data(), nonce.size())
                          .BlockMode(BlockMode::CTR)
                          .Padding(PaddingMode::NONE);
    AuthorizationSet out_params;
    string ciphertext = EncryptMessage(key_blob_, message, params, &out_params);
    EXPECT_EQ(expected_ciphertext, ciphertext);
}

void KeyMintAidlTestBase::CheckTripleDesTestVector(KeyPurpose purpose, BlockMode block_mode,
                                                   PaddingMode padding_mode, const string& key,
                                                   const string& iv, const string& input,
                                                   const string& expected_output) {
    auto authset = AuthorizationSetBuilder()
                           .TripleDesEncryptionKey(key.size() * 7)
                           .BlockMode(block_mode)
                           .Authorization(TAG_NO_AUTH_REQUIRED)
                           .Padding(padding_mode);
    if (iv.size()) authset.Authorization(TAG_CALLER_NONCE);
    ASSERT_EQ(ErrorCode::OK, ImportKey(authset, KeyFormat::RAW, key));
    ASSERT_GT(key_blob_.size(), 0U);

    auto begin_params = AuthorizationSetBuilder().BlockMode(block_mode).Padding(padding_mode);
    if (iv.size()) begin_params.Authorization(TAG_NONCE, iv.data(), iv.size());
    AuthorizationSet output_params;
    string output = ProcessMessage(key_blob_, purpose, input, begin_params, &output_params);
    EXPECT_EQ(expected_output, output);
}

void KeyMintAidlTestBase::VerifyMessage(const vector<uint8_t>& key_blob, const string& message,
                                        const string& signature, const AuthorizationSet& params) {
    SCOPED_TRACE("VerifyMessage");
    AuthorizationSet begin_out_params;
    ASSERT_EQ(ErrorCode::OK, Begin(KeyPurpose::VERIFY, key_blob, params, &begin_out_params));

    string output;
    EXPECT_EQ(ErrorCode::OK, Finish(message, signature, &output));
    EXPECT_TRUE(output.empty());
    op_ = {};
}

void KeyMintAidlTestBase::VerifyMessage(const string& message, const string& signature,
                                        const AuthorizationSet& params) {
    SCOPED_TRACE("VerifyMessage");
    VerifyMessage(key_blob_, message, signature, params);
}

string KeyMintAidlTestBase::EncryptMessage(const vector<uint8_t>& key_blob, const string& message,
                                           const AuthorizationSet& in_params,
                                           AuthorizationSet* out_params) {
    SCOPED_TRACE("EncryptMessage");
    return ProcessMessage(key_blob, KeyPurpose::ENCRYPT, message, in_params, out_params);
}

string KeyMintAidlTestBase::EncryptMessage(const string& message, const AuthorizationSet& params,
                                           AuthorizationSet* out_params) {
    SCOPED_TRACE("EncryptMessage");
    return EncryptMessage(key_blob_, message, params, out_params);
}

string KeyMintAidlTestBase::EncryptMessage(const string& message, const AuthorizationSet& params) {
    SCOPED_TRACE("EncryptMessage");
    AuthorizationSet out_params;
    string ciphertext = EncryptMessage(message, params, &out_params);
    EXPECT_TRUE(out_params.empty()) << "Output params should be empty. Contained: " << out_params;
    return ciphertext;
}

string KeyMintAidlTestBase::EncryptMessage(const string& message, BlockMode block_mode,
                                           PaddingMode padding) {
    SCOPED_TRACE("EncryptMessage");
    auto params = AuthorizationSetBuilder().BlockMode(block_mode).Padding(padding);
    AuthorizationSet out_params;
    string ciphertext = EncryptMessage(message, params, &out_params);
    EXPECT_TRUE(out_params.empty()) << "Output params should be empty. Contained: " << out_params;
    return ciphertext;
}

string KeyMintAidlTestBase::EncryptMessage(const string& message, BlockMode block_mode,
                                           PaddingMode padding, vector<uint8_t>* iv_out) {
    SCOPED_TRACE("EncryptMessage");
    auto params = AuthorizationSetBuilder().BlockMode(block_mode).Padding(padding);
    AuthorizationSet out_params;
    string ciphertext = EncryptMessage(message, params, &out_params);
    EXPECT_EQ(1U, out_params.size());
    auto ivVal = out_params.GetTagValue(TAG_NONCE);
    EXPECT_TRUE(ivVal);
    if (ivVal) *iv_out = *ivVal;
    return ciphertext;
}

string KeyMintAidlTestBase::EncryptMessage(const string& message, BlockMode block_mode,
                                           PaddingMode padding, const vector<uint8_t>& iv_in) {
    SCOPED_TRACE("EncryptMessage");
    auto params = AuthorizationSetBuilder()
                          .BlockMode(block_mode)
                          .Padding(padding)
                          .Authorization(TAG_NONCE, iv_in);
    AuthorizationSet out_params;
    string ciphertext = EncryptMessage(message, params, &out_params);
    return ciphertext;
}

string KeyMintAidlTestBase::EncryptMessage(const string& message, BlockMode block_mode,
                                           PaddingMode padding, uint8_t mac_length_bits,
                                           const vector<uint8_t>& iv_in) {
    SCOPED_TRACE("EncryptMessage");
    auto params = AuthorizationSetBuilder()
                          .BlockMode(block_mode)
                          .Padding(padding)
                          .Authorization(TAG_MAC_LENGTH, mac_length_bits)
                          .Authorization(TAG_NONCE, iv_in);
    AuthorizationSet out_params;
    string ciphertext = EncryptMessage(message, params, &out_params);
    return ciphertext;
}

string KeyMintAidlTestBase::DecryptMessage(const vector<uint8_t>& key_blob,
                                           const string& ciphertext,
                                           const AuthorizationSet& params) {
    SCOPED_TRACE("DecryptMessage");
    AuthorizationSet out_params;
    string plaintext =
            ProcessMessage(key_blob, KeyPurpose::DECRYPT, ciphertext, params, &out_params);
    EXPECT_TRUE(out_params.empty());
    return plaintext;
}

string KeyMintAidlTestBase::DecryptMessage(const string& ciphertext,
                                           const AuthorizationSet& params) {
    SCOPED_TRACE("DecryptMessage");
    return DecryptMessage(key_blob_, ciphertext, params);
}

string KeyMintAidlTestBase::DecryptMessage(const string& ciphertext, BlockMode block_mode,
                                           PaddingMode padding_mode, const vector<uint8_t>& iv) {
    SCOPED_TRACE("DecryptMessage");
    auto params = AuthorizationSetBuilder()
                          .BlockMode(block_mode)
                          .Padding(padding_mode)
                          .Authorization(TAG_NONCE, iv);
    return DecryptMessage(key_blob_, ciphertext, params);
}

std::pair<ErrorCode, vector<uint8_t>> KeyMintAidlTestBase::UpgradeKey(
        const vector<uint8_t>& key_blob) {
    std::pair<ErrorCode, vector<uint8_t>> retval;
    vector<uint8_t> outKeyBlob;
    Status result = keymint_->upgradeKey(key_blob, vector<KeyParameter>(), &outKeyBlob);
    ErrorCode errorcode = GetReturnErrorCode(result);
    retval = std::tie(errorcode, outKeyBlob);

    return retval;
}
vector<uint32_t> KeyMintAidlTestBase::ValidKeySizes(Algorithm algorithm) {
    switch (algorithm) {
        case Algorithm::RSA:
            switch (SecLevel()) {
                case SecurityLevel::SOFTWARE:
                case SecurityLevel::TRUSTED_ENVIRONMENT:
                    return {2048, 3072, 4096};
                case SecurityLevel::STRONGBOX:
                    return {2048};
                default:
                    ADD_FAILURE() << "Invalid security level " << uint32_t(SecLevel());
                    break;
            }
            break;
        case Algorithm::EC:
            switch (SecLevel()) {
                case SecurityLevel::SOFTWARE:
                case SecurityLevel::TRUSTED_ENVIRONMENT:
                    return {224, 256, 384, 521};
                case SecurityLevel::STRONGBOX:
                    return {256};
                default:
                    ADD_FAILURE() << "Invalid security level " << uint32_t(SecLevel());
                    break;
            }
            break;
        case Algorithm::AES:
            return {128, 256};
        case Algorithm::TRIPLE_DES:
            return {168};
        case Algorithm::HMAC: {
            vector<uint32_t> retval((512 - 64) / 8 + 1);
            uint32_t size = 64 - 8;
            std::generate(retval.begin(), retval.end(), [&]() { return (size += 8); });
            return retval;
        }
        default:
            ADD_FAILURE() << "Invalid Algorithm: " << algorithm;
            return {};
    }
    ADD_FAILURE() << "Should be impossible to get here";
    return {};
}

vector<uint32_t> KeyMintAidlTestBase::InvalidKeySizes(Algorithm algorithm) {
    if (SecLevel() == SecurityLevel::STRONGBOX) {
        switch (algorithm) {
            case Algorithm::RSA:
                return {3072, 4096};
            case Algorithm::EC:
                return {224, 384, 521};
            case Algorithm::AES:
                return {192};
            default:
                return {};
        }
    }
    return {};
}

vector<EcCurve> KeyMintAidlTestBase::ValidCurves() {
    if (securityLevel_ == SecurityLevel::STRONGBOX) {
        return {EcCurve::P_256};
    } else {
        return {EcCurve::P_224, EcCurve::P_256, EcCurve::P_384, EcCurve::P_521};
    }
}

vector<EcCurve> KeyMintAidlTestBase::InvalidCurves() {
    if (SecLevel() == SecurityLevel::TRUSTED_ENVIRONMENT) return {};
    CHECK(SecLevel() == SecurityLevel::STRONGBOX);
    return {EcCurve::P_224, EcCurve::P_384, EcCurve::P_521};
}

vector<Digest> KeyMintAidlTestBase::ValidDigests(bool withNone, bool withMD5) {
    switch (SecLevel()) {
        case SecurityLevel::SOFTWARE:
        case SecurityLevel::TRUSTED_ENVIRONMENT:
            if (withNone) {
                if (withMD5)
                    return {Digest::NONE,      Digest::MD5,       Digest::SHA1,
                            Digest::SHA_2_224, Digest::SHA_2_256, Digest::SHA_2_384,
                            Digest::SHA_2_512};
                else
                    return {Digest::NONE,      Digest::SHA1,      Digest::SHA_2_224,
                            Digest::SHA_2_256, Digest::SHA_2_384, Digest::SHA_2_512};
            } else {
                if (withMD5)
                    return {Digest::MD5,       Digest::SHA1,      Digest::SHA_2_224,
                            Digest::SHA_2_256, Digest::SHA_2_384, Digest::SHA_2_512};
                else
                    return {Digest::SHA1, Digest::SHA_2_224, Digest::SHA_2_256, Digest::SHA_2_384,
                            Digest::SHA_2_512};
            }
            break;
        case SecurityLevel::STRONGBOX:
            if (withNone)
                return {Digest::NONE, Digest::SHA_2_256};
            else
                return {Digest::SHA_2_256};
            break;
        default:
            ADD_FAILURE() << "Invalid security level " << uint32_t(SecLevel());
            break;
    }
    ADD_FAILURE() << "Should be impossible to get here";
    return {};
}

static const vector<KeyParameter> kEmptyAuthList{};

const vector<KeyParameter>& KeyMintAidlTestBase::SecLevelAuthorizations(
        const vector<KeyCharacteristics>& key_characteristics) {
    auto found = std::find_if(key_characteristics.begin(), key_characteristics.end(),
                              [this](auto& entry) { return entry.securityLevel == SecLevel(); });
    return (found == key_characteristics.end()) ? kEmptyAuthList : found->authorizations;
}

const vector<KeyParameter>& KeyMintAidlTestBase::SecLevelAuthorizations(
        const vector<KeyCharacteristics>& key_characteristics, SecurityLevel securityLevel) {
    auto found = std::find_if(
            key_characteristics.begin(), key_characteristics.end(),
            [securityLevel](auto& entry) { return entry.securityLevel == securityLevel; });
    return (found == key_characteristics.end()) ? kEmptyAuthList : found->authorizations;
}

ErrorCode KeyMintAidlTestBase::UseAesKey(const vector<uint8_t>& aesKeyBlob) {
    auto [result, ciphertext] = ProcessMessage(
            aesKeyBlob, KeyPurpose::ENCRYPT, "1234567890123456",
            AuthorizationSetBuilder().BlockMode(BlockMode::ECB).Padding(PaddingMode::NONE));
    return result;
}

ErrorCode KeyMintAidlTestBase::UseHmacKey(const vector<uint8_t>& hmacKeyBlob) {
    auto [result, mac] = ProcessMessage(
            hmacKeyBlob, KeyPurpose::SIGN, "1234567890123456",
            AuthorizationSetBuilder().Authorization(TAG_MAC_LENGTH, 128).Digest(Digest::SHA_2_256));
    return result;
}

ErrorCode KeyMintAidlTestBase::UseRsaKey(const vector<uint8_t>& rsaKeyBlob) {
    std::string message(2048 / 8, 'a');
    auto [result, signature] = ProcessMessage(
            rsaKeyBlob, KeyPurpose::SIGN, message,
            AuthorizationSetBuilder().Digest(Digest::NONE).Padding(PaddingMode::NONE));
    return result;
}

ErrorCode KeyMintAidlTestBase::UseEcdsaKey(const vector<uint8_t>& ecdsaKeyBlob) {
    auto [result, signature] = ProcessMessage(ecdsaKeyBlob, KeyPurpose::SIGN, "a",
                                              AuthorizationSetBuilder().Digest(Digest::SHA_2_256));
    return result;
}

void verify_serial(X509* cert, const uint64_t expected_serial) {
    BIGNUM_Ptr ser(BN_new());
    EXPECT_TRUE(ASN1_INTEGER_to_BN(X509_get_serialNumber(cert), ser.get()));

    uint64_t serial;
    EXPECT_TRUE(BN_get_u64(ser.get(), &serial));
    EXPECT_EQ(serial, expected_serial);
}

// Please set self_signed to true for fake certificates or self signed
// certificates
void verify_subject(const X509* cert,       //
                    const string& subject,  //
                    bool self_signed) {
    char* cert_issuer =  //
            X509_NAME_oneline(X509_get_issuer_name(cert), nullptr, 0);

    char* cert_subj = X509_NAME_oneline(X509_get_subject_name(cert), nullptr, 0);

    string expected_subject("/CN=");
    if (subject.empty()) {
        expected_subject.append("Android Keystore Key");
    } else {
        expected_subject.append(subject);
    }

    EXPECT_STREQ(expected_subject.c_str(), cert_subj) << "Cert has wrong subject." << cert_subj;

    if (self_signed) {
        EXPECT_STREQ(cert_issuer, cert_subj)
                << "Cert issuer and subject mismatch for self signed certificate.";
    }

    OPENSSL_free(cert_subj);
    OPENSSL_free(cert_issuer);
}

vector<uint8_t> build_serial_blob(const uint64_t serial_int) {
    BIGNUM_Ptr serial(BN_new());
    EXPECT_TRUE(BN_set_u64(serial.get(), serial_int));

    int len = BN_num_bytes(serial.get());
    vector<uint8_t> serial_blob(len);
    if (BN_bn2bin(serial.get(), serial_blob.data()) != len) {
        return {};
    }

    return serial_blob;
}

void verify_subject_and_serial(const Certificate& certificate,  //
                               const uint64_t expected_serial,  //
                               const string& subject, bool self_signed) {
    X509_Ptr cert(parse_cert_blob(certificate.encodedCertificate));
    ASSERT_TRUE(!!cert.get());

    verify_serial(cert.get(), expected_serial);
    verify_subject(cert.get(), subject, self_signed);
}

bool verify_attestation_record(const string& challenge,                //
                               const string& app_id,                   //
                               AuthorizationSet expected_sw_enforced,  //
                               AuthorizationSet expected_hw_enforced,  //
                               SecurityLevel security_level,
                               const vector<uint8_t>& attestation_cert) {
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
    vector<uint8_t> att_challenge;
    vector<uint8_t> att_unique_id;
    vector<uint8_t> att_app_id;

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

    EXPECT_EQ(att_attestation_version, 100U);
    vector<uint8_t> appId(app_id.begin(), app_id.end());

    // check challenge and app id only if we expects a non-fake certificate
    if (challenge.length() > 0) {
        EXPECT_EQ(challenge.length(), att_challenge.size());
        EXPECT_EQ(0, memcmp(challenge.data(), att_challenge.data(), challenge.length()));

        expected_sw_enforced.push_back(TAG_ATTESTATION_APPLICATION_ID, appId);
    }

    EXPECT_EQ(att_keymaster_version, 100U);
    EXPECT_EQ(security_level, att_keymaster_security_level);
    EXPECT_EQ(security_level, att_attestation_security_level);


    char property_value[PROPERTY_VALUE_MAX] = {};
    // TODO(b/136282179): When running under VTS-on-GSI the TEE-backed
    // keymaster implementation will report YYYYMM dates instead of YYYYMMDD
    // for the BOOT_PATCH_LEVEL.
    if (avb_verification_enabled()) {
        for (int i = 0; i < att_hw_enforced.size(); i++) {
            if (att_hw_enforced[i].tag == TAG_BOOT_PATCHLEVEL ||
                att_hw_enforced[i].tag == TAG_VENDOR_PATCHLEVEL) {
                std::string date =
                        std::to_string(att_hw_enforced[i].value.get<KeyParameterValue::integer>());
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

    // Check to make sure boolean values are properly encoded. Presence of a boolean tag
    // indicates true. A provided boolean tag that can be pulled back out of the certificate
    // indicates correct encoding. No need to check if it's in both lists, since the
    // AuthorizationSet compare below will handle mismatches of tags.
    if (security_level == SecurityLevel::SOFTWARE) {
        EXPECT_TRUE(expected_sw_enforced.Contains(TAG_NO_AUTH_REQUIRED));
    } else {
        EXPECT_TRUE(expected_hw_enforced.Contains(TAG_NO_AUTH_REQUIRED));
    }

    // Alternatively this checks the opposite - a false boolean tag (one that isn't provided in
    // the authorization list during key generation) isn't being attested to in the certificate.
    EXPECT_FALSE(expected_sw_enforced.Contains(TAG_TRUSTED_USER_PRESENCE_REQUIRED));
    EXPECT_FALSE(att_sw_enforced.Contains(TAG_TRUSTED_USER_PRESENCE_REQUIRED));
    EXPECT_FALSE(expected_hw_enforced.Contains(TAG_TRUSTED_USER_PRESENCE_REQUIRED));
    EXPECT_FALSE(att_hw_enforced.Contains(TAG_TRUSTED_USER_PRESENCE_REQUIRED));

    if (att_hw_enforced.Contains(TAG_ALGORITHM, Algorithm::EC)) {
        // For ECDSA keys, either an EC_CURVE or a KEY_SIZE can be specified, but one must be.
        EXPECT_TRUE(att_hw_enforced.Contains(TAG_EC_CURVE) ||
                    att_hw_enforced.Contains(TAG_KEY_SIZE));
    }

    // Test root of trust elements
    vector<uint8_t> verified_boot_key;
    VerifiedBoot verified_boot_state;
    bool device_locked;
    vector<uint8_t> verified_boot_hash;
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

        // Check that the device is locked if not debuggable, e.g., user build
        // images in CTS. For VTS, debuggable images are used to allow adb root
        // and the device is unlocked.
        if (!property_get_bool("ro.debuggable", false)) {
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
        EXPECT_EQ(verified_boot_state, VerifiedBoot::VERIFIED);
        EXPECT_NE(0, memcmp(verified_boot_key.data(), empty_boot_key.data(),
                            verified_boot_key.size()));
    } else if (!strcmp(property_value, "yellow")) {
        EXPECT_EQ(verified_boot_state, VerifiedBoot::SELF_SIGNED);
        EXPECT_NE(0, memcmp(verified_boot_key.data(), empty_boot_key.data(),
                            verified_boot_key.size()));
    } else if (!strcmp(property_value, "orange")) {
        EXPECT_EQ(verified_boot_state, VerifiedBoot::UNVERIFIED);
        EXPECT_EQ(0, memcmp(verified_boot_key.data(), empty_boot_key.data(),
                            verified_boot_key.size()));
    } else if (!strcmp(property_value, "red")) {
        EXPECT_EQ(verified_boot_state, VerifiedBoot::FAILED);
    } else {
        EXPECT_EQ(verified_boot_state, VerifiedBoot::UNVERIFIED);
        EXPECT_NE(0, memcmp(verified_boot_key.data(), empty_boot_key.data(),
                            verified_boot_key.size()));
    }

    att_sw_enforced.Sort();
    expected_sw_enforced.Sort();
    auto a = filtered_tags(expected_sw_enforced);
    auto b = filtered_tags(att_sw_enforced);
    EXPECT_EQ(a, b);

    att_hw_enforced.Sort();
    expected_hw_enforced.Sort();
    EXPECT_EQ(filtered_tags(expected_hw_enforced), filtered_tags(att_hw_enforced));

    return true;
}

string bin2hex(const vector<uint8_t>& data) {
    string retval;
    retval.reserve(data.size() * 2 + 1);
    for (uint8_t byte : data) {
        retval.push_back(nibble2hex[0x0F & (byte >> 4)]);
        retval.push_back(nibble2hex[0x0F & byte]);
    }
    return retval;
}

AuthorizationSet HwEnforcedAuthorizations(const vector<KeyCharacteristics>& key_characteristics) {
    AuthorizationSet authList;
    for (auto& entry : key_characteristics) {
        if (entry.securityLevel == SecurityLevel::STRONGBOX ||
            entry.securityLevel == SecurityLevel::TRUSTED_ENVIRONMENT) {
            authList.push_back(AuthorizationSet(entry.authorizations));
        }
    }
    return authList;
}

AuthorizationSet SwEnforcedAuthorizations(const vector<KeyCharacteristics>& key_characteristics) {
    AuthorizationSet authList;
    for (auto& entry : key_characteristics) {
        if (entry.securityLevel == SecurityLevel::SOFTWARE ||
            entry.securityLevel == SecurityLevel::KEYSTORE) {
            authList.push_back(AuthorizationSet(entry.authorizations));
        }
    }
    return authList;
}

AssertionResult ChainSignaturesAreValid(const vector<Certificate>& chain) {
    std::stringstream cert_data;

    for (size_t i = 0; i < chain.size(); ++i) {
        cert_data << bin2hex(chain[i].encodedCertificate) << std::endl;

        X509_Ptr key_cert(parse_cert_blob(chain[i].encodedCertificate));
        X509_Ptr signing_cert;
        if (i < chain.size() - 1) {
            signing_cert = parse_cert_blob(chain[i + 1].encodedCertificate);
        } else {
            signing_cert = parse_cert_blob(chain[i].encodedCertificate);
        }
        if (!key_cert.get() || !signing_cert.get()) return AssertionFailure() << cert_data.str();

        EVP_PKEY_Ptr signing_pubkey(X509_get_pubkey(signing_cert.get()));
        if (!signing_pubkey.get()) return AssertionFailure() << cert_data.str();

        if (!X509_verify(key_cert.get(), signing_pubkey.get())) {
            return AssertionFailure()
                   << "Verification of certificate " << i << " failed "
                   << "OpenSSL error string: " << ERR_error_string(ERR_get_error(), NULL) << '\n'
                   << cert_data.str();
        }

        string cert_issuer = x509NameToStr(X509_get_issuer_name(key_cert.get()));
        string signer_subj = x509NameToStr(X509_get_subject_name(signing_cert.get()));
        if (cert_issuer != signer_subj) {
            return AssertionFailure() << "Cert " << i << " has wrong issuer.\n"
                                      << " Signer subject is " << signer_subj
                                      << " Issuer subject is " << cert_issuer << endl
                                      << cert_data.str();
        }
    }

    if (KeyMintAidlTestBase::dump_Attestations) std::cout << cert_data.str();
    return AssertionSuccess();
}

X509_Ptr parse_cert_blob(const vector<uint8_t>& blob) {
    const uint8_t* p = blob.data();
    return X509_Ptr(d2i_X509(nullptr /* allocate new */, &p, blob.size()));
}

vector<uint8_t> make_name_from_str(const string& name) {
    X509_NAME_Ptr x509_name(X509_NAME_new());
    EXPECT_TRUE(x509_name.get() != nullptr);
    if (!x509_name) return {};

    EXPECT_EQ(1, X509_NAME_add_entry_by_txt(x509_name.get(),  //
                                            "CN",             //
                                            MBSTRING_ASC,
                                            reinterpret_cast<const uint8_t*>(name.c_str()),
                                            -1,  // len
                                            -1,  // loc
                                            0 /* set */));

    int len = i2d_X509_NAME(x509_name.get(), nullptr /* only return length */);
    EXPECT_GT(len, 0);

    vector<uint8_t> retval(len);
    uint8_t* p = retval.data();
    i2d_X509_NAME(x509_name.get(), &p);

    return retval;
}

namespace {

void check_cose_key(const vector<uint8_t>& data, bool testMode) {
    auto [parsedPayload, __, payloadParseErr] = cppbor::parse(data);
    ASSERT_TRUE(parsedPayload) << "Key parse failed: " << payloadParseErr;

    // The following check assumes that canonical CBOR encoding is used for the COSE_Key.
    if (testMode) {
        EXPECT_THAT(cppbor::prettyPrint(parsedPayload.get()),
                    MatchesRegex("{\n"
                                 "  1 : 2,\n"   // kty: EC2
                                 "  3 : -7,\n"  // alg: ES256
                                 "  -1 : 1,\n"  // EC id: P256
                                 // The regex {(0x[0-9a-f]{2}, ){31}0x[0-9a-f]{2}} matches a
                                 // sequence of 32 hexadecimal bytes, enclosed in braces and
                                 // separated by commas. In this case, some Ed25519 public key.
                                 "  -2 : {(0x[0-9a-f]{2}, ){31}0x[0-9a-f]{2}},\n"  // pub_x: data
                                 "  -3 : {(0x[0-9a-f]{2}, ){31}0x[0-9a-f]{2}},\n"  // pub_y: data
                                 "  -70000 : null,\n"                              // test marker
                                 "}"));
    } else {
        EXPECT_THAT(cppbor::prettyPrint(parsedPayload.get()),
                    MatchesRegex("{\n"
                                 "  1 : 2,\n"   // kty: EC2
                                 "  3 : -7,\n"  // alg: ES256
                                 "  -1 : 1,\n"  // EC id: P256
                                 // The regex {(0x[0-9a-f]{2}, ){31}0x[0-9a-f]{2}} matches a
                                 // sequence of 32 hexadecimal bytes, enclosed in braces and
                                 // separated by commas. In this case, some Ed25519 public key.
                                 "  -2 : {(0x[0-9a-f]{2}, ){31}0x[0-9a-f]{2}},\n"  // pub_x: data
                                 "  -3 : {(0x[0-9a-f]{2}, ){31}0x[0-9a-f]{2}},\n"  // pub_y: data
                                 "}"));
    }
}

}  // namespace

void check_maced_pubkey(const MacedPublicKey& macedPubKey, bool testMode,
                        vector<uint8_t>* payload_value) {
    auto [coseMac0, _, mac0ParseErr] = cppbor::parse(macedPubKey.macedKey);
    ASSERT_TRUE(coseMac0) << "COSE Mac0 parse failed " << mac0ParseErr;

    ASSERT_NE(coseMac0->asArray(), nullptr);
    ASSERT_EQ(coseMac0->asArray()->size(), kCoseMac0EntryCount);

    auto protParms = coseMac0->asArray()->get(kCoseMac0ProtectedParams)->asBstr();
    ASSERT_NE(protParms, nullptr);

    // Header label:value of 'alg': HMAC-256
    ASSERT_EQ(cppbor::prettyPrint(protParms->value()), "{\n  1 : 5,\n}");

    auto unprotParms = coseMac0->asArray()->get(kCoseMac0UnprotectedParams)->asMap();
    ASSERT_NE(unprotParms, nullptr);
    ASSERT_EQ(unprotParms->size(), 0);

    // The payload is a bstr holding an encoded COSE_Key
    auto payload = coseMac0->asArray()->get(kCoseMac0Payload)->asBstr();
    ASSERT_NE(payload, nullptr);
    check_cose_key(payload->value(), testMode);

    auto coseMac0Tag = coseMac0->asArray()->get(kCoseMac0Tag)->asBstr();
    ASSERT_TRUE(coseMac0Tag);
    auto extractedTag = coseMac0Tag->value();
    EXPECT_EQ(extractedTag.size(), 32U);

    // Compare with tag generated with kTestMacKey.  Should only match in test mode
    auto testTag = cppcose::generateCoseMac0Mac(remote_prov::kTestMacKey, {} /* external_aad */,
                                                payload->value());
    ASSERT_TRUE(testTag) << "Tag calculation failed: " << testTag.message();

    if (testMode) {
        EXPECT_EQ(*testTag, extractedTag);
    } else {
        EXPECT_NE(*testTag, extractedTag);
    }
    if (payload_value != nullptr) {
        *payload_value = payload->value();
    }
}

void p256_pub_key(const vector<uint8_t>& coseKeyData, EVP_PKEY_Ptr* signingKey) {
    // Extract x and y affine coordinates from the encoded Cose_Key.
    auto [parsedPayload, __, payloadParseErr] = cppbor::parse(coseKeyData);
    ASSERT_TRUE(parsedPayload) << "Key parse failed: " << payloadParseErr;
    auto coseKey = parsedPayload->asMap();
    const std::unique_ptr<cppbor::Item>& xItem = coseKey->get(cppcose::CoseKey::PUBKEY_X);
    ASSERT_NE(xItem->asBstr(), nullptr);
    vector<uint8_t> x = xItem->asBstr()->value();
    const std::unique_ptr<cppbor::Item>& yItem = coseKey->get(cppcose::CoseKey::PUBKEY_Y);
    ASSERT_NE(yItem->asBstr(), nullptr);
    vector<uint8_t> y = yItem->asBstr()->value();

    // Concatenate: 0x04 (uncompressed form marker) | x | y
    vector<uint8_t> pubKeyData{0x04};
    pubKeyData.insert(pubKeyData.end(), x.begin(), x.end());
    pubKeyData.insert(pubKeyData.end(), y.begin(), y.end());

    EC_KEY_Ptr ecKey = EC_KEY_Ptr(EC_KEY_new());
    ASSERT_NE(ecKey, nullptr);
    EC_GROUP_Ptr group = EC_GROUP_Ptr(EC_GROUP_new_by_curve_name(NID_X9_62_prime256v1));
    ASSERT_NE(group, nullptr);
    ASSERT_EQ(EC_KEY_set_group(ecKey.get(), group.get()), 1);
    EC_POINT_Ptr point = EC_POINT_Ptr(EC_POINT_new(group.get()));
    ASSERT_NE(point, nullptr);
    ASSERT_EQ(EC_POINT_oct2point(group.get(), point.get(), pubKeyData.data(), pubKeyData.size(),
                                 nullptr),
              1);
    ASSERT_EQ(EC_KEY_set_public_key(ecKey.get(), point.get()), 1);

    EVP_PKEY_Ptr pubKey = EVP_PKEY_Ptr(EVP_PKEY_new());
    ASSERT_NE(pubKey, nullptr);
    EVP_PKEY_assign_EC_KEY(pubKey.get(), ecKey.release());
    *signingKey = std::move(pubKey);
}

}  // namespace test

}  // namespace aidl::android::hardware::security::keymint
