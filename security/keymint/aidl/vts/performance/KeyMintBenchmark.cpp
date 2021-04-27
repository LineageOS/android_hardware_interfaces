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

#define LOG_TAG "keymint_benchmark"

#include <base/command_line.h>
#include <benchmark/benchmark.h>
#include <iostream>

#include <aidl/Vintf.h>
#include <aidl/android/hardware/security/keymint/ErrorCode.h>
#include <aidl/android/hardware/security/keymint/IKeyMintDevice.h>
#include <android/binder_manager.h>
#include <binder/IServiceManager.h>
#include <keymint_support/authorization_set.h>

#define SMALL_MESSAGE_SIZE 64
#define MEDIUM_MESSAGE_SIZE 1024
#define LARGE_MESSAGE_SIZE 131072

namespace aidl::android::hardware::security::keymint::test {

::std::ostream& operator<<(::std::ostream& os, const keymint::AuthorizationSet& set);

using ::android::sp;
using Status = ::ndk::ScopedAStatus;
using ::std::optional;
using ::std::shared_ptr;
using ::std::string;
using ::std::vector;

class KeyMintBenchmarkTest {
  public:
    KeyMintBenchmarkTest() {
        message_cache_.push_back(string(SMALL_MESSAGE_SIZE, 'x'));
        message_cache_.push_back(string(MEDIUM_MESSAGE_SIZE, 'x'));
        message_cache_.push_back(string(LARGE_MESSAGE_SIZE, 'x'));
    }

    static KeyMintBenchmarkTest* newInstance(const char* instanceName) {
        if (AServiceManager_isDeclared(instanceName)) {
            ::ndk::SpAIBinder binder(AServiceManager_waitForService(instanceName));
            KeyMintBenchmarkTest* test = new KeyMintBenchmarkTest();
            test->InitializeKeyMint(IKeyMintDevice::fromBinder(binder));
            return test;
        } else {
            return nullptr;
        }
    }

    int getError() { return static_cast<int>(error_); }

    const string& GenerateMessage(int size) {
        for (const string& message : message_cache_) {
            if (message.size() == size) {
                return message;
            }
        }
        string message = string(size, 'x');
        message_cache_.push_back(message);
        return std::move(message);
    }

    optional<BlockMode> getBlockMode(string transform) {
        if (transform.find("/ECB") != string::npos) {
            return BlockMode::ECB;
        } else if (transform.find("/CBC") != string::npos) {
            return BlockMode::CBC;
        } else if (transform.find("/CTR") != string::npos) {
            return BlockMode::CTR;
        } else if (transform.find("/GCM") != string::npos) {
            return BlockMode::GCM;
        }
        return {};
    }

    PaddingMode getPadding(string transform, bool sign) {
        if (transform.find("/PKCS7") != string::npos) {
            return PaddingMode::PKCS7;
        } else if (transform.find("/PSS") != string::npos) {
            return PaddingMode::RSA_PSS;
        } else if (transform.find("/OAEP") != string::npos) {
            return PaddingMode::RSA_OAEP;
        } else if (transform.find("/PKCS1") != string::npos) {
            return sign ? PaddingMode::RSA_PKCS1_1_5_SIGN : PaddingMode::RSA_PKCS1_1_5_ENCRYPT;
        } else if (sign && transform.find("RSA") != string::npos) {
            // RSA defaults to PKCS1 for sign
            return PaddingMode::RSA_PKCS1_1_5_SIGN;
        }
        return PaddingMode::NONE;
    }

    optional<Algorithm> getAlgorithm(string transform) {
        if (transform.find("AES") != string::npos) {
            return Algorithm::AES;
        } else if (transform.find("Hmac") != string::npos) {
            return Algorithm::HMAC;
        } else if (transform.find("DESede") != string::npos) {
            return Algorithm::TRIPLE_DES;
        } else if (transform.find("RSA") != string::npos) {
            return Algorithm::RSA;
        } else if (transform.find("EC") != string::npos) {
            return Algorithm::EC;
        }
        std::cerr << "Can't find algorithm for " << transform << std::endl;
        return {};
    }

    Digest getDigest(string transform) {
        if (transform.find("MD5") != string::npos) {
            return Digest::MD5;
        } else if (transform.find("SHA1") != string::npos ||
                   transform.find("SHA-1") != string::npos) {
            return Digest::SHA1;
        } else if (transform.find("SHA224") != string::npos) {
            return Digest::SHA_2_224;
        } else if (transform.find("SHA256") != string::npos) {
            return Digest::SHA_2_256;
        } else if (transform.find("SHA384") != string::npos) {
            return Digest::SHA_2_384;
        } else if (transform.find("SHA512") != string::npos) {
            return Digest::SHA_2_512;
        } else if (transform.find("RSA") != string::npos &&
                   transform.find("OAEP") != string::npos) {
            return Digest::SHA1;
        } else if (transform.find("Hmac") != string::npos) {
            return Digest::SHA_2_256;
        }
        return Digest::NONE;
    }

    bool GenerateKey(string transform, int keySize, bool sign = false) {
        if (transform == key_transform_) {
            return true;
        } else if (key_transform_ != "") {
            // Deleting old key first
            key_transform_ = "";
            if (DeleteKey() != ErrorCode::OK) {
                return false;
            }
        }
        std::optional<Algorithm> algorithm = getAlgorithm(transform);
        if (!algorithm) {
            std::cerr << "Error: invalid algorithm " << transform << std::endl;
            return false;
        }
        key_transform_ = transform;
        AuthorizationSetBuilder authSet = AuthorizationSetBuilder()
                                                  .Authorization(TAG_NO_AUTH_REQUIRED)
                                                  .Authorization(TAG_PURPOSE, KeyPurpose::ENCRYPT)
                                                  .Authorization(TAG_PURPOSE, KeyPurpose::DECRYPT)
                                                  .Authorization(TAG_PURPOSE, KeyPurpose::SIGN)
                                                  .Authorization(TAG_PURPOSE, KeyPurpose::VERIFY)
                                                  .Authorization(TAG_KEY_SIZE, keySize)
                                                  .Authorization(TAG_ALGORITHM, algorithm.value())
                                                  .Digest(getDigest(transform))
                                                  .Padding(getPadding(transform, sign));
        std::optional<BlockMode> blockMode = getBlockMode(transform);
        if (blockMode) {
            authSet.BlockMode(blockMode.value());
            if (blockMode == BlockMode::GCM) {
                authSet.Authorization(TAG_MIN_MAC_LENGTH, 128);
            }
        }
        if (algorithm == Algorithm::HMAC) {
            authSet.Authorization(TAG_MIN_MAC_LENGTH, 128);
        }
        if (algorithm == Algorithm::RSA) {
            authSet.Authorization(TAG_RSA_PUBLIC_EXPONENT, 65537U);
            authSet.SetDefaultValidity();
        }
        if (algorithm == Algorithm::EC) {
            authSet.SetDefaultValidity();
        }
        error_ = GenerateKey(authSet);
        return error_ == ErrorCode::OK;
    }

    AuthorizationSet getOperationParams(string transform, bool sign = false) {
        AuthorizationSetBuilder builder = AuthorizationSetBuilder()
                                                  .Padding(getPadding(transform, sign))
                                                  .Digest(getDigest(transform));
        std::optional<BlockMode> blockMode = getBlockMode(transform);
        if (sign && (transform.find("Hmac") != string::npos)) {
            builder.Authorization(TAG_MAC_LENGTH, 128);
        }
        if (blockMode) {
            builder.BlockMode(*blockMode);
            if (blockMode == BlockMode::GCM) {
                builder.Authorization(TAG_MAC_LENGTH, 128);
            }
        }
        return std::move(builder);
    }

    optional<string> Process(const string& message, const string& signature = "") {
        ErrorCode result;

        string output;
        result = Finish(message, signature, &output);
        if (result != ErrorCode::OK) {
            error_ = result;
            return {};
        }
        return output;
    }

    ErrorCode DeleteKey() {
        Status result = keymint_->deleteKey(key_blob_);
        key_blob_ = vector<uint8_t>();
        return GetReturnErrorCode(result);
    }

    ErrorCode Begin(KeyPurpose purpose, const AuthorizationSet& in_params,
                    AuthorizationSet* out_params) {
        Status result;
        BeginResult out;
        result = keymint_->begin(purpose, key_blob_, in_params.vector_data(), std::nullopt, &out);
        if (result.isOk()) {
            *out_params = out.params;
            op_ = out.operation;
        }
        return GetReturnErrorCode(result);
    }

    SecurityLevel securityLevel_;
    string name_;

  private:
    ErrorCode GenerateKey(const AuthorizationSet& key_desc,
                          const optional<AttestationKey>& attest_key = std::nullopt) {
        key_blob_.clear();
        KeyCreationResult creationResult;
        Status result = keymint_->generateKey(key_desc.vector_data(), attest_key, &creationResult);
        if (result.isOk()) {
            key_blob_ = std::move(creationResult.keyBlob);
            creationResult.keyCharacteristics.clear();
            creationResult.certificateChain.clear();
        }
        return GetReturnErrorCode(result);
    }

    void InitializeKeyMint(std::shared_ptr<IKeyMintDevice> keyMint) {
        if (!keyMint) {
            std::cerr << "Trying initialize nullptr in InitializeKeyMint" << std::endl;
            return;
        }
        keymint_ = std::move(keyMint);
        KeyMintHardwareInfo info;
        Status result = keymint_->getHardwareInfo(&info);
        if (!result.isOk()) {
            std::cerr << "InitializeKeyMint: getHardwareInfo failed with "
                      << result.getServiceSpecificError() << std::endl;
        }
        securityLevel_ = info.securityLevel;
        name_.assign(info.keyMintName.begin(), info.keyMintName.end());
    }

    ErrorCode Finish(const string& input, const string& signature, string* output) {
        if (!op_) {
            std::cerr << "Finish: Operation is nullptr" << std::endl;
            return ErrorCode::UNEXPECTED_NULL_POINTER;
        }

        vector<uint8_t> oPut;
        Status result =
                op_->finish(vector<uint8_t>(input.begin(), input.end()),
                            vector<uint8_t>(signature.begin(), signature.end()), {} /* authToken */,
                            {} /* timestampToken */, {} /* confirmationToken */, &oPut);

        if (result.isOk()) output->append(oPut.begin(), oPut.end());

        op_.reset();
        return GetReturnErrorCode(result);
    }

    ErrorCode Update(const string& input, string* output) {
        Status result;
        if (!op_) {
            std::cerr << "Update: Operation is nullptr" << std::endl;
            return ErrorCode::UNEXPECTED_NULL_POINTER;
        }

        std::vector<uint8_t> o_put;
        result = op_->update(vector<uint8_t>(input.begin(), input.end()), {} /* authToken */,
                             {} /* timestampToken */, &o_put);

        if (result.isOk() && output) *output = {o_put.begin(), o_put.end()};
        return GetReturnErrorCode(result);
    }

    ErrorCode GetReturnErrorCode(const Status& result) {
        error_ = static_cast<ErrorCode>(result.getServiceSpecificError());
        if (result.isOk()) return ErrorCode::OK;

        if (result.getExceptionCode() == EX_SERVICE_SPECIFIC) {
            return static_cast<ErrorCode>(result.getServiceSpecificError());
        }

        return ErrorCode::UNKNOWN_ERROR;
    }

    std::shared_ptr<IKeyMintOperation> op_;
    vector<Certificate> cert_chain_;
    vector<uint8_t> key_blob_;
    vector<KeyCharacteristics> key_characteristics_;
    std::shared_ptr<IKeyMintDevice> keymint_;
    std::vector<string> message_cache_;
    std::string key_transform_;
    ErrorCode error_;
};

KeyMintBenchmarkTest* keymintTest;

static void settings(benchmark::internal::Benchmark* benchmark) {
    benchmark->Unit(benchmark::kMillisecond);
}

static void addDefaultLabel(benchmark::State& state) {
    std::string secLevel;
    switch (keymintTest->securityLevel_) {
        case SecurityLevel::STRONGBOX:
            secLevel = "STRONGBOX";
            break;
        case SecurityLevel::SOFTWARE:
            secLevel = "SOFTWARE";
            break;
        case SecurityLevel::TRUSTED_ENVIRONMENT:
            secLevel = "TEE";
            break;
        case SecurityLevel::KEYSTORE:
            secLevel = "KEYSTORE";
            break;
    }
    state.SetLabel("hardware_name:" + keymintTest->name_ + " sec_level:" + secLevel);
}

// clang-format off
#define BENCHMARK_KM(func, transform, keySize) \
    BENCHMARK_CAPTURE(func, transform/keySize, #transform "/" #keySize, keySize)->Apply(settings);
#define BENCHMARK_KM_MSG(func, transform, keySize, msgSize)                                      \
    BENCHMARK_CAPTURE(func, transform/keySize/msgSize, #transform "/" #keySize "/" #msgSize, \
                      keySize, msgSize)                                                          \
            ->Apply(settings);

#define BENCHMARK_KM_ALL_MSGS(func, transform, keySize)             \
    BENCHMARK_KM_MSG(func, transform, keySize, SMALL_MESSAGE_SIZE)  \
    BENCHMARK_KM_MSG(func, transform, keySize, MEDIUM_MESSAGE_SIZE) \
    BENCHMARK_KM_MSG(func, transform, keySize, LARGE_MESSAGE_SIZE)

#define BENCHMARK_KM_CIPHER(transform, keySize, msgSize)   \
    BENCHMARK_KM_MSG(encrypt, transform, keySize, msgSize) \
    BENCHMARK_KM_MSG(decrypt, transform, keySize, msgSize)

#define BENCHMARK_KM_CIPHER_ALL_MSGS(transform, keySize) \
    BENCHMARK_KM_ALL_MSGS(encrypt, transform, keySize)   \
    BENCHMARK_KM_ALL_MSGS(decrypt, transform, keySize)

#define BENCHMARK_KM_SIGNATURE_ALL_MSGS(transform, keySize) \
    BENCHMARK_KM_ALL_MSGS(sign, transform, keySize)         \
    BENCHMARK_KM_ALL_MSGS(verify, transform, keySize)
// clang-format on

/*
 * ============= KeyGen TESTS ==================
 */
static void keygen(benchmark::State& state, string transform, int keySize) {
    addDefaultLabel(state);
    for (auto _ : state) {
        if (!keymintTest->GenerateKey(transform, keySize)) {
            state.SkipWithError(
                    ("Key generation error, " + std::to_string(keymintTest->getError())).c_str());
        }
        state.PauseTiming();

        keymintTest->DeleteKey();
        state.ResumeTiming();
    }
}

BENCHMARK_KM(keygen, AES, 128);
BENCHMARK_KM(keygen, AES, 256);

BENCHMARK_KM(keygen, RSA, 2048);
BENCHMARK_KM(keygen, RSA, 3072);
BENCHMARK_KM(keygen, RSA, 4096);

BENCHMARK_KM(keygen, EC, 224);
BENCHMARK_KM(keygen, EC, 256);
BENCHMARK_KM(keygen, EC, 384);
BENCHMARK_KM(keygen, EC, 521);

BENCHMARK_KM(keygen, DESede, 168);

BENCHMARK_KM(keygen, Hmac, 64);
BENCHMARK_KM(keygen, Hmac, 128);
BENCHMARK_KM(keygen, Hmac, 256);
BENCHMARK_KM(keygen, Hmac, 512);

/*
 * ============= SIGNATURE TESTS ==================
 */

static void sign(benchmark::State& state, string transform, int keySize, int msgSize) {
    addDefaultLabel(state);
    if (!keymintTest->GenerateKey(transform, keySize, true)) {
        state.SkipWithError(
                ("Key generation error, " + std::to_string(keymintTest->getError())).c_str());
        return;
    }

    auto in_params = keymintTest->getOperationParams(transform, true);
    AuthorizationSet out_params;
    string message = keymintTest->GenerateMessage(msgSize);

    for (auto _ : state) {
        state.PauseTiming();
        ErrorCode error = keymintTest->Begin(KeyPurpose::SIGN, in_params, &out_params);
        if (error != ErrorCode::OK) {
            state.SkipWithError(
                    ("Error beginning sign, " + std::to_string(keymintTest->getError())).c_str());
            return;
        }
        state.ResumeTiming();
        out_params.Clear();
        if (!keymintTest->Process(message)) {
            state.SkipWithError(("Sign error, " + std::to_string(keymintTest->getError())).c_str());
            break;
        }
    }
}

static void verify(benchmark::State& state, string transform, int keySize, int msgSize) {
    addDefaultLabel(state);
    if (!keymintTest->GenerateKey(transform, keySize, true)) {
        state.SkipWithError(
                ("Key generation error, " + std::to_string(keymintTest->getError())).c_str());
        return;
    }
    AuthorizationSet out_params;
    auto in_params = keymintTest->getOperationParams(transform, true);
    string message = keymintTest->GenerateMessage(msgSize);
    ErrorCode error = keymintTest->Begin(KeyPurpose::SIGN, in_params, &out_params);
    if (error != ErrorCode::OK) {
        state.SkipWithError(
                ("Error beginning sign, " + std::to_string(keymintTest->getError())).c_str());
        return;
    }
    std::optional<string> signature = keymintTest->Process(message);
    if (!signature) {
        state.SkipWithError(("Sign error, " + std::to_string(keymintTest->getError())).c_str());
        return;
    }
    out_params.Clear();
    if (transform.find("Hmac") != string::npos) {
        in_params = keymintTest->getOperationParams(transform, false);
    }
    for (auto _ : state) {
        state.PauseTiming();
        error = keymintTest->Begin(KeyPurpose::VERIFY, in_params, &out_params);
        if (error != ErrorCode::OK) {
            state.SkipWithError(
                    ("Verify begin error, " + std::to_string(keymintTest->getError())).c_str());
            return;
        }
        state.ResumeTiming();
        if (!keymintTest->Process(message, *signature)) {
            state.SkipWithError(
                    ("Verify error, " + std::to_string(keymintTest->getError())).c_str());
            break;
        }
    }
}

// clang-format off
#define BENCHMARK_KM_SIGNATURE_ALL_HMAC_KEYS(transform) \
    BENCHMARK_KM_SIGNATURE_ALL_MSGS(transform, 64)      \
    BENCHMARK_KM_SIGNATURE_ALL_MSGS(transform, 128)     \
    BENCHMARK_KM_SIGNATURE_ALL_MSGS(transform, 256)     \
    BENCHMARK_KM_SIGNATURE_ALL_MSGS(transform, 512)

BENCHMARK_KM_SIGNATURE_ALL_HMAC_KEYS(HmacSHA1)
BENCHMARK_KM_SIGNATURE_ALL_HMAC_KEYS(HmacSHA256)
BENCHMARK_KM_SIGNATURE_ALL_HMAC_KEYS(HmacSHA224)
BENCHMARK_KM_SIGNATURE_ALL_HMAC_KEYS(HmacSHA256)
BENCHMARK_KM_SIGNATURE_ALL_HMAC_KEYS(HmacSHA384)
BENCHMARK_KM_SIGNATURE_ALL_HMAC_KEYS(HmacSHA512)

#define BENCHMARK_KM_SIGNATURE_ALL_ECDSA_KEYS(transform) \
    BENCHMARK_KM_SIGNATURE_ALL_MSGS(transform, 224)      \
    BENCHMARK_KM_SIGNATURE_ALL_MSGS(transform, 256)      \
    BENCHMARK_KM_SIGNATURE_ALL_MSGS(transform, 384)      \
    BENCHMARK_KM_SIGNATURE_ALL_MSGS(transform, 521)

BENCHMARK_KM_SIGNATURE_ALL_ECDSA_KEYS(NONEwithECDSA);
BENCHMARK_KM_SIGNATURE_ALL_ECDSA_KEYS(SHA1withECDSA);
BENCHMARK_KM_SIGNATURE_ALL_ECDSA_KEYS(SHA224withECDSA);
BENCHMARK_KM_SIGNATURE_ALL_ECDSA_KEYS(SHA256withECDSA);
BENCHMARK_KM_SIGNATURE_ALL_ECDSA_KEYS(SHA384withECDSA);
BENCHMARK_KM_SIGNATURE_ALL_ECDSA_KEYS(SHA512withECDSA);

#define BENCHMARK_KM_SIGNATURE_ALL_RSA_KEYS(transform) \
    BENCHMARK_KM_SIGNATURE_ALL_MSGS(transform, 2048)   \
    BENCHMARK_KM_SIGNATURE_ALL_MSGS(transform, 3072)   \
    BENCHMARK_KM_SIGNATURE_ALL_MSGS(transform, 4096)

BENCHMARK_KM_SIGNATURE_ALL_RSA_KEYS(MD5withRSA);
BENCHMARK_KM_SIGNATURE_ALL_RSA_KEYS(SHA1withRSA);
BENCHMARK_KM_SIGNATURE_ALL_RSA_KEYS(SHA224withRSA);
BENCHMARK_KM_SIGNATURE_ALL_RSA_KEYS(SHA384withRSA);
BENCHMARK_KM_SIGNATURE_ALL_RSA_KEYS(SHA512withRSA);

BENCHMARK_KM_SIGNATURE_ALL_RSA_KEYS(MD5withRSA/PSS);
BENCHMARK_KM_SIGNATURE_ALL_RSA_KEYS(SHA1withRSA/PSS);
BENCHMARK_KM_SIGNATURE_ALL_RSA_KEYS(SHA224withRSA/PSS);
BENCHMARK_KM_SIGNATURE_ALL_RSA_KEYS(SHA384withRSA/PSS);
BENCHMARK_KM_SIGNATURE_ALL_RSA_KEYS(SHA512withRSA/PSS);
// clang-format on

/*
 * ============= CIPHER TESTS ==================
 */

static void encrypt(benchmark::State& state, string transform, int keySize, int msgSize) {
    addDefaultLabel(state);
    if (!keymintTest->GenerateKey(transform, keySize)) {
        state.SkipWithError(
                ("Key generation error, " + std::to_string(keymintTest->getError())).c_str());
        return;
    }
    auto in_params = keymintTest->getOperationParams(transform);
    AuthorizationSet out_params;
    string message = keymintTest->GenerateMessage(msgSize);

    for (auto _ : state) {
        state.PauseTiming();
        auto error = keymintTest->Begin(KeyPurpose::ENCRYPT, in_params, &out_params);
        if (error != ErrorCode::OK) {
            state.SkipWithError(
                    ("Encryption begin error, " + std::to_string(keymintTest->getError())).c_str());
            return;
        }
        out_params.Clear();
        state.ResumeTiming();
        if (!keymintTest->Process(message)) {
            state.SkipWithError(
                    ("Encryption error, " + std::to_string(keymintTest->getError())).c_str());
            break;
        }
    }
}

static void decrypt(benchmark::State& state, string transform, int keySize, int msgSize) {
    addDefaultLabel(state);
    if (!keymintTest->GenerateKey(transform, keySize)) {
        state.SkipWithError(
                ("Key generation error, " + std::to_string(keymintTest->getError())).c_str());
        return;
    }
    AuthorizationSet out_params;
    AuthorizationSet in_params = keymintTest->getOperationParams(transform);
    string message = keymintTest->GenerateMessage(msgSize);
    auto error = keymintTest->Begin(KeyPurpose::ENCRYPT, in_params, &out_params);
    if (error != ErrorCode::OK) {
        state.SkipWithError(
                ("Encryption begin error, " + std::to_string(keymintTest->getError())).c_str());
        return;
    }
    auto encryptedMessage = keymintTest->Process(message);
    if (!encryptedMessage) {
        state.SkipWithError(
                ("Encryption error, " + std::to_string(keymintTest->getError())).c_str());
        return;
    }
    in_params.push_back(out_params);
    out_params.Clear();
    for (auto _ : state) {
        state.PauseTiming();
        error = keymintTest->Begin(KeyPurpose::DECRYPT, in_params, &out_params);
        if (error != ErrorCode::OK) {
            state.SkipWithError(
                    ("Decryption begin error, " + std::to_string(keymintTest->getError())).c_str());
            return;
        }
        state.ResumeTiming();
        if (!keymintTest->Process(*encryptedMessage)) {
            state.SkipWithError(
                    ("Decryption error, " + std::to_string(keymintTest->getError())).c_str());
            break;
        }
    }
}

// clang-format off
// AES
#define BENCHMARK_KM_CIPHER_ALL_AES_KEYS(transform) \
    BENCHMARK_KM_CIPHER_ALL_MSGS(transform, 128)    \
    BENCHMARK_KM_CIPHER_ALL_MSGS(transform, 256)

BENCHMARK_KM_CIPHER_ALL_AES_KEYS(AES/CBC/NoPadding);
BENCHMARK_KM_CIPHER_ALL_AES_KEYS(AES/CBC/PKCS7Padding);
BENCHMARK_KM_CIPHER_ALL_AES_KEYS(AES/CTR/NoPadding);
BENCHMARK_KM_CIPHER_ALL_AES_KEYS(AES/ECB/NoPadding);
BENCHMARK_KM_CIPHER_ALL_AES_KEYS(AES/ECB/PKCS7Padding);
BENCHMARK_KM_CIPHER_ALL_AES_KEYS(AES/GCM/NoPadding);

// Triple DES
BENCHMARK_KM_CIPHER_ALL_MSGS(DESede/CBC/NoPadding, 168);
BENCHMARK_KM_CIPHER_ALL_MSGS(DESede/CBC/PKCS7Padding, 168);
BENCHMARK_KM_CIPHER_ALL_MSGS(DESede/ECB/NoPadding, 168);
BENCHMARK_KM_CIPHER_ALL_MSGS(DESede/ECB/PKCS7Padding, 168);

#define BENCHMARK_KM_CIPHER_ALL_RSA_KEYS(transform, msgSize) \
    BENCHMARK_KM_CIPHER(transform, 2048, msgSize)            \
    BENCHMARK_KM_CIPHER(transform, 3072, msgSize)            \
    BENCHMARK_KM_CIPHER(transform, 4096, msgSize)

BENCHMARK_KM_CIPHER_ALL_RSA_KEYS(RSA/ECB/NoPadding, SMALL_MESSAGE_SIZE);
BENCHMARK_KM_CIPHER_ALL_RSA_KEYS(RSA/ECB/PKCS1Padding, SMALL_MESSAGE_SIZE);
BENCHMARK_KM_CIPHER_ALL_RSA_KEYS(RSA/ECB/OAEPPadding, SMALL_MESSAGE_SIZE);

// clang-format on
}  // namespace aidl::android::hardware::security::keymint::test

int main(int argc, char** argv) {
    ::benchmark::Initialize(&argc, argv);
    base::CommandLine::Init(argc, argv);
    base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
    auto service_name = command_line->GetSwitchValueASCII("service_name");
    if (service_name.empty()) {
        service_name =
                std::string(
                        aidl::android::hardware::security::keymint::IKeyMintDevice::descriptor) +
                "/default";
    }
    std::cerr << service_name << std::endl;
    aidl::android::hardware::security::keymint::test::keymintTest =
            aidl::android::hardware::security::keymint::test::KeyMintBenchmarkTest::newInstance(
                    service_name.c_str());
    if (!aidl::android::hardware::security::keymint::test::keymintTest) {
        return 1;
    }
    ::benchmark::RunSpecifiedBenchmarks();
}
