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

#include <keymint_support/key_param_output.h>
#include <keymint_support/keymint_utils.h>

namespace aidl::android::hardware::security::keymint {

using namespace std::literals::chrono_literals;
using std::endl;
using std::optional;

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

// Predicate for testing basic characteristics validity in generation or import.
bool KeyCharacteristicsBasicallyValid(SecurityLevel secLevel,
                                      const vector<KeyCharacteristics>& key_characteristics) {
    if (key_characteristics.empty()) return false;

    std::unordered_set<SecurityLevel> levels_seen;
    for (auto& entry : key_characteristics) {
        if (entry.authorizations.empty()) return false;

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

}  // namespace

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
                                           vector<uint8_t>* key_blob,
                                           vector<KeyCharacteristics>* key_characteristics) {
    EXPECT_NE(key_blob, nullptr) << "Key blob pointer must not be null.  Test bug";
    EXPECT_NE(key_characteristics, nullptr)
            << "Previous characteristics not deleted before generating key.  Test bug.";

    // Aidl does not clear these output parameters if the function returns
    // error.  This is different from hal where output parameter is always
    // cleared due to hal returning void.  So now we need to do our own clearing
    // of the output variables prior to calling keyMint aidl libraries.
    key_blob->clear();
    key_characteristics->clear();
    cert_chain_.clear();

    KeyCreationResult creationResult;
    Status result = keymint_->generateKey(key_desc.vector_data(), &creationResult);

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

ErrorCode KeyMintAidlTestBase::GenerateKey(const AuthorizationSet& key_desc) {
    return GenerateKey(key_desc, &key_blob_, &key_characteristics_);
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
                                 &creationResult);

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

ErrorCode KeyMintAidlTestBase::Update(const AuthorizationSet& in_params, const string& input,
                                      AuthorizationSet* out_params, string* output,
                                      int32_t* input_consumed) {
    SCOPED_TRACE("Update");

    Status result;
    EXPECT_NE(op_, nullptr);
    if (!op_) {
        return ErrorCode::UNEXPECTED_NULL_POINTER;
    }

    KeyParameterArray key_params;
    key_params.params = in_params.vector_data();

    KeyParameterArray in_keyParams;
    in_keyParams.params = in_params.vector_data();

    optional<KeyParameterArray> out_keyParams;
    optional<ByteArray> o_put;
    result = op_->update(in_keyParams, vector<uint8_t>(input.begin(), input.end()), {}, {},
                         &out_keyParams, &o_put, input_consumed);

    if (result.isOk()) {
        if (o_put) {
            output->append(o_put->data.begin(), o_put->data.end());
        }

        if (out_keyParams) {
            out_params->push_back(AuthorizationSet(out_keyParams->params));
        }
    }

    return GetReturnErrorCode(result);
}

ErrorCode KeyMintAidlTestBase::Update(const string& input, string* out, int32_t* input_consumed) {
    SCOPED_TRACE("Update");
    AuthorizationSet out_params;
    ErrorCode result =
            Update(AuthorizationSet() /* in_params */, input, &out_params, out, input_consumed);
    EXPECT_TRUE(out_params.empty());
    return result;
}

ErrorCode KeyMintAidlTestBase::Finish(const AuthorizationSet& in_params, const string& input,
                                      const string& signature, AuthorizationSet* out_params,
                                      string* output) {
    SCOPED_TRACE("Finish");
    Status result;

    EXPECT_NE(op_, nullptr);
    if (!op_) {
        return ErrorCode::UNEXPECTED_NULL_POINTER;
    }

    KeyParameterArray key_params;
    key_params.params = in_params.vector_data();

    KeyParameterArray in_keyParams;
    in_keyParams.params = in_params.vector_data();

    optional<KeyParameterArray> out_keyParams;
    optional<vector<uint8_t>> o_put;

    vector<uint8_t> oPut;
    result = op_->finish(in_keyParams, vector<uint8_t>(input.begin(), input.end()),
                         vector<uint8_t>(signature.begin(), signature.end()), {}, {},
                         &out_keyParams, &oPut);

    if (result.isOk()) {
        if (out_keyParams) {
            out_params->push_back(AuthorizationSet(out_keyParams->params));
        }

        output->append(oPut.begin(), oPut.end());
    }

    op_.reset();
    return GetReturnErrorCode(result);
}

ErrorCode KeyMintAidlTestBase::Finish(const string& message, string* output) {
    SCOPED_TRACE("Finish");
    AuthorizationSet out_params;
    string finish_output;
    ErrorCode result = Finish(AuthorizationSet() /* in_params */, message, "" /* signature */,
                              &out_params, output);
    if (result != ErrorCode::OK) {
        return result;
    }
    EXPECT_EQ(0U, out_params.size());
    return result;
}

ErrorCode KeyMintAidlTestBase::Finish(const string& message, const string& signature,
                                      string* output) {
    SCOPED_TRACE("Finish");
    AuthorizationSet out_params;
    ErrorCode result =
            Finish(AuthorizationSet() /* in_params */, message, signature, &out_params, output);

    if (result != ErrorCode::OK) {
        return result;
    }

    EXPECT_EQ(0U, out_params.size());
    return result;
}

ErrorCode KeyMintAidlTestBase::Abort(const std::shared_ptr<IKeyMintOperation>& op) {
    SCOPED_TRACE("Abort");

    EXPECT_NE(op, nullptr);
    if (!op) {
        return ErrorCode::UNEXPECTED_NULL_POINTER;
    }

    Status retval = op->abort();
    EXPECT_TRUE(retval.isOk());
    return static_cast<ErrorCode>(retval.getServiceSpecificError());
}

ErrorCode KeyMintAidlTestBase::Abort() {
    SCOPED_TRACE("Abort");

    EXPECT_NE(op_, nullptr);
    if (!op_) {
        return ErrorCode::UNEXPECTED_NULL_POINTER;
    }

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

string KeyMintAidlTestBase::ProcessMessage(const vector<uint8_t>& key_blob, KeyPurpose operation,
                                           const string& message, const AuthorizationSet& in_params,
                                           AuthorizationSet* out_params) {
    SCOPED_TRACE("ProcessMessage");
    AuthorizationSet begin_out_params;
    ErrorCode result = Begin(operation, key_blob, in_params, &begin_out_params);
    EXPECT_EQ(ErrorCode::OK, result);
    if (result != ErrorCode::OK) {
        return "";
    }

    string output;
    int32_t consumed = 0;
    AuthorizationSet update_params;
    AuthorizationSet update_out_params;
    result = Update(update_params, message, &update_out_params, &output, &consumed);
    EXPECT_EQ(ErrorCode::OK, result);
    if (result != ErrorCode::OK) {
        return "";
    }

    string unused;
    AuthorizationSet finish_params;
    AuthorizationSet finish_out_params;
    EXPECT_EQ(ErrorCode::OK,
              Finish(finish_params, message.substr(consumed), unused, &finish_out_params, &output));

    out_params->push_back(begin_out_params);
    out_params->push_back(finish_out_params);
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
    AuthorizationSet update_params;
    AuthorizationSet update_out_params;
    int32_t consumed;
    ASSERT_EQ(ErrorCode::OK,
              Update(update_params, message, &update_out_params, &output, &consumed));
    EXPECT_TRUE(output.empty());
    EXPECT_GT(consumed, 0U);

    string unused;
    AuthorizationSet finish_params;
    AuthorizationSet finish_out_params;
    EXPECT_EQ(ErrorCode::OK, Finish(finish_params, message.substr(consumed), signature,
                                    &finish_out_params, &output));
    op_.reset();
    EXPECT_TRUE(output.empty());
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

const vector<KeyParameter>& KeyMintAidlTestBase::HwEnforcedAuthorizations(
        const vector<KeyCharacteristics>& key_characteristics) {
    auto found =
            std::find_if(key_characteristics.begin(), key_characteristics.end(), [](auto& entry) {
                return entry.securityLevel == SecurityLevel::STRONGBOX ||
                       entry.securityLevel == SecurityLevel::TRUSTED_ENVIRONMENT;
            });
    return (found == key_characteristics.end()) ? kEmptyAuthList : found->authorizations;
}

const vector<KeyParameter>& KeyMintAidlTestBase::SwEnforcedAuthorizations(
        const vector<KeyCharacteristics>& key_characteristics) {
    auto found = std::find_if(
            key_characteristics.begin(), key_characteristics.end(),
            [](auto& entry) { return entry.securityLevel == SecurityLevel::SOFTWARE; });
    return (found == key_characteristics.end()) ? kEmptyAuthList : found->authorizations;
}

}  // namespace test

}  // namespace aidl::android::hardware::security::keymint
