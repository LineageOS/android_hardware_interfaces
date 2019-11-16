/*
 * Copyright (C) 2019 The Android Open Source Project
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

#define LOG_TAG "keymaster_benchmark"

#include <android/hardware/keymaster/4.0/IKeymasterDevice.h>
#include <android/hardware/keymaster/4.0/types.h>
#include <keymaster/keymaster_configuration.h>
#include <keymasterV4_0/authorization_set.h>

#include <android/hidl/manager/1.0/IServiceManager.h>
#include <binder/IServiceManager.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <iostream>

#include <log/log.h>
#include <utils/StrongPointer.h>

#include <benchmark/benchmark.h>
#include <hidl/Status.h>

#include <base/command_line.h>

namespace android {
namespace hardware {
namespace keymaster {
namespace V4_0 {
namespace test {

// libutils:
using android::OK;
using android::sp;
using android::status_t;

// libhidl:
using android::hardware::hidl_vec;
using android::hardware::Return;
using android::hardware::Void;

// IKeymaster:
using android::IServiceManager;
using android::hardware::hidl_string;
using android::hardware::keymaster::V4_0::AuthorizationSet;
using android::hardware::keymaster::V4_0::AuthorizationSetBuilder;
using android::hardware::keymaster::V4_0::BlockMode;
using android::hardware::keymaster::V4_0::ErrorCode;
using android::hardware::keymaster::V4_0::IKeymasterDevice;
using android::hardware::keymaster::V4_0::KeyCharacteristics;
using android::hardware::keymaster::V4_0::SecurityLevel;

// Standard library:
using std::cerr;
using std::cout;
using std::endl;
using std::optional;
using std::string;
using std::unique_ptr;
using std::vector;

class HidlBuf : public hidl_vec<uint8_t> {
    typedef hidl_vec<uint8_t> super;

  public:
    HidlBuf() {}
    HidlBuf(const super& other) : super(other) {}
    HidlBuf(super&& other) : super(std::move(other)) {}
    explicit HidlBuf(const std::string& other) : HidlBuf() { *this = other; }

    HidlBuf& operator=(const super& other) {
        super::operator=(other);
        return *this;
    }

    HidlBuf& operator=(super&& other) {
        super::operator=(std::move(other));
        return *this;
    }

    HidlBuf& operator=(const string& other) {
        resize(other.size());
        std::copy(other.begin(), other.end(), begin());
        return *this;
    }

    string to_string() const { return string(reinterpret_cast<const char*>(data()), size()); }
};

#define SMALL_MESSAGE_SIZE 64
#define MEDIUM_MESSAGE_SIZE 1024
#define LARGE_MESSAGE_SIZE 131072

class KeymasterWrapper {
  private:
    sp<IKeymasterDevice> keymaster_;
    SecurityLevel securityLevel_;
    hidl_string name_;
    hidl_string author_;
    HidlBuf key_blob_;
    KeyCharacteristics key_characteristics_;
    ErrorCode error_;
    string key_transform_;
    string keymaster_name_;
    uint32_t os_version_;
    uint32_t os_patch_level_;
    std::vector<string> message_cache_;

    bool GenerateKey(const AuthorizationSet& authSet) {
        return (keymaster_
                        ->generateKey(
                                authSet.hidl_data(),
                                [&](ErrorCode hidl_error, const hidl_vec<uint8_t>& hidl_key_blob,
                                    const KeyCharacteristics& hidl_key_characteristics) {
                                    error_ = hidl_error;
                                    key_blob_ = hidl_key_blob;
                                    key_characteristics_ = std::move(hidl_key_characteristics);
                                })
                        .isOk() &&
                error_ == ErrorCode::OK);
    }

    bool GenerateKey(Algorithm algorithm, int keySize, Digest digest = Digest::NONE,
                     PaddingMode padding = PaddingMode::NONE, optional<BlockMode> blockMode = {}) {
        AuthorizationSetBuilder authSet = AuthorizationSetBuilder()
                                                  .Authorization(TAG_NO_AUTH_REQUIRED)
                                                  .Authorization(TAG_PURPOSE, KeyPurpose::ENCRYPT)
                                                  .Authorization(TAG_PURPOSE, KeyPurpose::DECRYPT)
                                                  .Authorization(TAG_PURPOSE, KeyPurpose::SIGN)
                                                  .Authorization(TAG_PURPOSE, KeyPurpose::VERIFY)
                                                  .Authorization(TAG_KEY_SIZE, keySize)
                                                  .Authorization(TAG_ALGORITHM, algorithm)
                                                  .Digest(digest)
                                                  .Authorization(TAG_MIN_MAC_LENGTH, 128)
                                                  .Padding(padding);
        if (blockMode) {
            authSet.BlockMode(*blockMode);
        }
        if (algorithm == Algorithm::RSA) {
            authSet.Authorization(TAG_RSA_PUBLIC_EXPONENT, 65537U);
        }
        return GenerateKey(authSet);
    }

    KeymasterWrapper(const sp<IKeymasterDevice> keymaster) {
        os_version_ = ::keymaster::GetOsVersion();
        os_patch_level_ = ::keymaster::GetOsPatchlevel();
        keymaster_ = keymaster;
        keymaster_->getHardwareInfo([&](SecurityLevel securityLevel, const hidl_string& name,
                                        const hidl_string& author) {
            securityLevel_ = securityLevel;
            name_ = name;
            author_ = author;
        });

        message_cache_.push_back(string(SMALL_MESSAGE_SIZE, 'x'));
        message_cache_.push_back(string(MEDIUM_MESSAGE_SIZE, 'x'));
        message_cache_.push_back(string(LARGE_MESSAGE_SIZE, 'x'));
    }

  public:
    static KeymasterWrapper* newInstance(const std::string& keymaster_name) {
        auto keymaster = IKeymasterDevice::getService(keymaster_name);
        if (!keymaster) {
            std::cerr << "Error: unable to find keymaster service named " << keymaster_name
                      << std::endl;
            return nullptr;
        }
        return new KeymasterWrapper(keymaster);
    }

    bool GenerateKey(string transform, int keySize, bool sign = false) {
        if (transform == key_transform_) {
            return true;
        } else if (key_transform_ != "") {
            // Deleting old key first
            if (!DeleteKey()) {
                return false;
            }
        }
        optional<Algorithm> algorithm = getAlgorithm(transform);
        if (!algorithm) {
            cerr << "Error: invalid algorithm " << transform << endl;
            return false;
        }
        key_transform_ = transform;
        return GenerateKey(*algorithm, keySize, getDigest(transform), getPadding(transform, sign),
                           getBlockMode(transform));
    }

    bool DeleteKey() {
        key_blob_ = HidlBuf();
        key_transform_ = "";
        return keymaster_->deleteKey(key_blob_).isOk();
    }

    AuthorizationSet getOperationParams(string transform, bool sign = false) {
        AuthorizationSetBuilder builder = AuthorizationSetBuilder()
                                                  .Padding(getPadding(transform, sign))
                                                  .Authorization(TAG_MAC_LENGTH, 128)
                                                  .Digest(getDigest(transform));
        optional<BlockMode> blockMode = getBlockMode(transform);
        if (blockMode) {
            builder.BlockMode(*blockMode);
        }
        return std::move(builder);
    }

    optional<OperationHandle> EncryptBegin(AuthorizationSet& in_params,
                                           AuthorizationSet* out_params = new AuthorizationSet) {
        return Begin(KeyPurpose::ENCRYPT, in_params, out_params);
    }

    optional<OperationHandle> DecryptBegin(AuthorizationSet& in_params,
                                           AuthorizationSet* out_params = new AuthorizationSet) {
        return Begin(KeyPurpose::DECRYPT, in_params, out_params);
    }

    optional<OperationHandle> SignBegin(AuthorizationSet& in_params,
                                        AuthorizationSet* out_params = new AuthorizationSet) {
        return Begin(KeyPurpose::SIGN, in_params, out_params);
    }

    optional<OperationHandle> VerifyBegin(AuthorizationSet& in_params,
                                          AuthorizationSet* out_params = new AuthorizationSet) {
        return Begin(KeyPurpose::VERIFY, in_params, out_params);
    }

    optional<OperationHandle> Begin(KeyPurpose operation, const AuthorizationSet& in_params,
                                    AuthorizationSet* out_params) {
        OperationHandle op_handle;
        if (!keymaster_
                     ->begin(operation, key_blob_, in_params.hidl_data(), HardwareAuthToken(),
                             [&](ErrorCode hidl_error,
                                 const hidl_vec<KeyParameter>& hidl_out_params,
                                 uint64_t hidl_op_handle) {
                                 error_ = hidl_error;
                                 out_params->push_back(AuthorizationSet(hidl_out_params));
                                 op_handle = hidl_op_handle;
                             })
                     .isOk() ||
            error_ != ErrorCode::OK) {
            keymaster_->abort(op_handle);
            return {};
        }
        return op_handle;
    }

    optional<string> ProcessMessage(const OperationHandle& op_handle, const string& message,
                                    const AuthorizationSet& in_params,
                                    AuthorizationSet* out_params = new AuthorizationSet,
                                    const string& signature = "") {
        static const int HIDL_BUFFER_LIMIT = 1 << 14;  // 16KB

        string output;
        size_t input_consumed = 0;
        while (message.length() - input_consumed > 0) {
            if (!keymaster_
                         ->update(op_handle, in_params.hidl_data(),
                                  HidlBuf(message.substr(input_consumed, HIDL_BUFFER_LIMIT)),
                                  HardwareAuthToken(), VerificationToken(),
                                  [&](ErrorCode hidl_error, uint32_t hidl_input_consumed,
                                      const hidl_vec<KeyParameter>& hidl_out_params,
                                      const HidlBuf& hidl_output) {
                                      error_ = hidl_error;
                                      out_params->push_back(AuthorizationSet(hidl_out_params));
                                      output.append(hidl_output.to_string());
                                      input_consumed += hidl_input_consumed;
                                  })
                         .isOk() ||
                error_ != ErrorCode::OK) {
                keymaster_->abort(op_handle);
                return {};
            }
        }

        if (!keymaster_
                     ->finish(op_handle, in_params.hidl_data(),
                              HidlBuf(message.substr(input_consumed)), HidlBuf(signature),
                              HardwareAuthToken(), VerificationToken(),
                              [&](ErrorCode hidl_error,
                                  const hidl_vec<KeyParameter>& hidl_out_params,
                                  const HidlBuf& hidl_output) {
                                  error_ = hidl_error;
                                  out_params->push_back(AuthorizationSet(hidl_out_params));
                                  output.append(hidl_output.to_string());
                              })
                     .isOk() ||
            error_ != ErrorCode::OK) {
            keymaster_->abort(op_handle);
            return {};
        }

        return output;
    }

    int getError() { return static_cast<int>(error_); }

    const string getHardwareName() { return name_; }

    SecurityLevel getSecurityLevel() { return securityLevel_; }

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
        cerr << "Can't find algorithm for " << transform << endl;
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
        }
        return Digest::NONE;
    }
};

KeymasterWrapper* keymaster;

static void settings(benchmark::internal::Benchmark* benchmark) {
    benchmark->Unit(benchmark::kMillisecond);
}

static void addDefaultLabel(benchmark::State& state) {
    string secLevel;
    switch (keymaster->getSecurityLevel()) {
        case SecurityLevel::STRONGBOX:
            secLevel = "STRONGBOX";
            break;
        case SecurityLevel::SOFTWARE:
            secLevel = "SOFTWARE";
            break;
        case SecurityLevel::TRUSTED_ENVIRONMENT:
            secLevel = "TEE";
            break;
    }
    state.SetLabel("hardware_name:" + keymaster->getHardwareName() + " sec_level:" + secLevel);
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
        keymaster->GenerateKey(transform, keySize);
        state.PauseTiming();
        keymaster->DeleteKey();
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
BENCHMARK_KM(keygen, Hmac, 1024);
BENCHMARK_KM(keygen, Hmac, 2048);
BENCHMARK_KM(keygen, Hmac, 4096);
BENCHMARK_KM(keygen, Hmac, 8192);

/*
 * ============= SIGNATURE TESTS ==================
 */

static void sign(benchmark::State& state, string transform, int keySize, int msgSize) {
    addDefaultLabel(state);
    if (!keymaster->GenerateKey(transform, keySize, true)) {
        state.SkipWithError(
                ("Key generation error, " + std::to_string(keymaster->getError())).c_str());
        return;
    }
    auto params = keymaster->getOperationParams(transform, true);
    string message = keymaster->GenerateMessage(msgSize);

    for (auto _ : state) {
        state.PauseTiming();
        auto opHandle = keymaster->SignBegin(params);
        if (!opHandle) {
            state.SkipWithError(
                    ("Error beginning sign, " + std::to_string(keymaster->getError())).c_str());
            return;
        }
        state.ResumeTiming();
        if (!keymaster->ProcessMessage(*opHandle, message, params)) {
            state.SkipWithError(("Sign error, " + std::to_string(keymaster->getError())).c_str());
            break;
        }
    }
}

static void verify(benchmark::State& state, string transform, int keySize, int msgSize) {
    addDefaultLabel(state);
    if (!keymaster->GenerateKey(transform, keySize, true)) {
        state.SkipWithError(
                ("Key generation error, " + std::to_string(keymaster->getError())).c_str());
        return;
    }
    AuthorizationSet out_params;
    AuthorizationSet in_params = keymaster->getOperationParams(transform, true);
    string message = keymaster->GenerateMessage(msgSize);
    auto opHandle = keymaster->SignBegin(in_params, &out_params);
    if (!opHandle) {
        state.SkipWithError(
                ("Error beginning sign, " + std::to_string(keymaster->getError())).c_str());
        return;
    }
    optional<string> signature =
            keymaster->ProcessMessage(*opHandle, message, in_params, &out_params);
    if (!signature) {
        state.SkipWithError(("Sign error, " + std::to_string(keymaster->getError())).c_str());
        return;
    }
    in_params.push_back(out_params);
    for (auto _ : state) {
        state.PauseTiming();
        opHandle = keymaster->VerifyBegin(in_params);
        if (!opHandle) {
            state.SkipWithError(
                    ("Verify begin error, " + std::to_string(keymaster->getError())).c_str());
            return;
        }
        state.ResumeTiming();
        if (!keymaster->ProcessMessage(*opHandle, message, in_params, &out_params, *signature)) {
            state.SkipWithError(("Verify error, " + std::to_string(keymaster->getError())).c_str());
            break;
        }
    }
}

// clang-format off
#define BENCHMARK_KM_SIGNATURE_ALL_HMAC_KEYS(transform) \
    BENCHMARK_KM_SIGNATURE_ALL_MSGS(transform, 64)      \
    BENCHMARK_KM_SIGNATURE_ALL_MSGS(transform, 128)     \
    BENCHMARK_KM_SIGNATURE_ALL_MSGS(transform, 256)     \
    BENCHMARK_KM_SIGNATURE_ALL_MSGS(transform, 512)     \
    BENCHMARK_KM_SIGNATURE_ALL_MSGS(transform, 1024)    \
    BENCHMARK_KM_SIGNATURE_ALL_MSGS(transform, 2024)    \
    BENCHMARK_KM_SIGNATURE_ALL_MSGS(transform, 4096)    \
    BENCHMARK_KM_SIGNATURE_ALL_MSGS(transform, 8192)

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
    if (!keymaster->GenerateKey(transform, keySize)) {
        state.SkipWithError(
                ("Key generation error, " + std::to_string(keymaster->getError())).c_str());
        return;
    }
    auto params = keymaster->getOperationParams(transform);
    string message = keymaster->GenerateMessage(msgSize);

    for (auto _ : state) {
        state.PauseTiming();
        auto opHandle = keymaster->EncryptBegin(params);
        if (!opHandle) {
            state.SkipWithError(
                    ("Encryption begin error, " + std::to_string(keymaster->getError())).c_str());
            return;
        }
        state.ResumeTiming();
        if (!keymaster->ProcessMessage(*opHandle, message, params)) {
            state.SkipWithError(
                    ("Encryption error, " + std::to_string(keymaster->getError())).c_str());
            break;
        }
    }
}

static void decrypt(benchmark::State& state, string transform, int keySize, int msgSize) {
    addDefaultLabel(state);
    if (!keymaster->GenerateKey(transform, keySize)) {
        state.SkipWithError(
                ("Key generation error, " + std::to_string(keymaster->getError())).c_str());
        return;
    }
    AuthorizationSet out_params;
    AuthorizationSet in_params = keymaster->getOperationParams(transform);
    string message = keymaster->GenerateMessage(msgSize);
    auto opHandle = keymaster->EncryptBegin(in_params, &out_params);
    if (!opHandle) {
        state.SkipWithError(
                ("Encryption begin error, " + std::to_string(keymaster->getError())).c_str());
        return;
    }
    auto encryptedMessage = keymaster->ProcessMessage(*opHandle, message, in_params, &out_params);
    if (!encryptedMessage) {
        state.SkipWithError(("Encryption error, " + std::to_string(keymaster->getError())).c_str());
        return;
    }
    in_params.push_back(out_params);
    for (auto _ : state) {
        state.PauseTiming();
        opHandle = keymaster->DecryptBegin(in_params);
        if (!opHandle) {
            state.SkipWithError(
                    ("Decryption begin error, " + std::to_string(keymaster->getError())).c_str());
            return;
        }
        state.ResumeTiming();
        if (!keymaster->ProcessMessage(*opHandle, *encryptedMessage, in_params)) {
            state.SkipWithError(
                    ("Decryption error, " + std::to_string(keymaster->getError())).c_str());
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

}  // namespace test
}  // namespace V4_0
}  // namespace keymaster
}  // namespace hardware
}  // namespace android

int main(int argc, char** argv) {
    ::benchmark::Initialize(&argc, argv);
    base::CommandLine::Init(argc, argv);
    base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
    auto service_name = command_line->GetSwitchValueASCII("service_name");
    if (service_name.empty()) {
        service_name = "default";
    }
    android::hardware::keymaster::V4_0::test::keymaster =
            android::hardware::keymaster::V4_0::test::KeymasterWrapper::newInstance(service_name);
    if (!android::hardware::keymaster::V4_0::test::keymaster) {
        return 1;
    }
    ::benchmark::RunSpecifiedBenchmarks();
}