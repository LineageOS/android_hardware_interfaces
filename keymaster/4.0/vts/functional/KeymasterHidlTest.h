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

#pragma once

#include <android/hardware/keymaster/4.0/IKeymasterDevice.h>
#include <android/hardware/keymaster/4.0/types.h>
#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>
#include <keymaster/keymaster_configuration.h>

#include <keymasterV4_0/authorization_set.h>

namespace android {
namespace hardware {
namespace keymaster {
namespace V4_0 {

::std::ostream& operator<<(::std::ostream& os, const AuthorizationSet& set);

namespace test {

using ::android::sp;
using ::std::string;
using hidl::base::V1_0::DebugInfo;

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

constexpr uint64_t kOpHandleSentinel = 0xFFFFFFFFFFFFFFFF;

class KeymasterHidlTest : public ::testing::TestWithParam<std::string> {
  public:
    void SetUp();
    void TearDown() override {
        if (key_blob_.size()) {
            CheckedDeleteKey();
        }
        AbortIfNeeded();
        keymaster_.clear();
        all_keymasters_.clear();
    }

    void InitializeKeymaster();

    IKeymasterDevice& keymaster() { return *keymaster_; }
    const std::vector<sp<IKeymasterDevice>>& all_keymasters() { return all_keymasters_; }
    uint32_t os_version() { return os_version_; }
    uint32_t os_patch_level() { return os_patch_level_; }

    ErrorCode GenerateKey(const AuthorizationSet& key_desc, HidlBuf* key_blob,
                          KeyCharacteristics* key_characteristics);
    ErrorCode GenerateKey(const AuthorizationSet& key_desc);

    ErrorCode ImportKey(const AuthorizationSet& key_desc, KeyFormat format,
                        const string& key_material, HidlBuf* key_blob,
                        KeyCharacteristics* key_characteristics);
    ErrorCode ImportKey(const AuthorizationSet& key_desc, KeyFormat format,
                        const string& key_material);

    ErrorCode ImportWrappedKey(string wrapped_key, string wrapping_key,
                               const AuthorizationSet& wrapping_key_desc, string masking_key,
                               const AuthorizationSet& unwrapping_params);

    ErrorCode ExportKey(KeyFormat format, const HidlBuf& key_blob, const HidlBuf& client_id,
                        const HidlBuf& app_data, HidlBuf* key_material);
    ErrorCode ExportKey(KeyFormat format, HidlBuf* key_material);

    ErrorCode DeleteKey(HidlBuf* key_blob, bool keep_key_blob = false);
    ErrorCode DeleteKey(bool keep_key_blob = false);

    ErrorCode DeleteAllKeys();

    void CheckedDeleteKey(HidlBuf* key_blob, bool keep_key_blob = false);
    void CheckedDeleteKey();

    void CheckGetCharacteristics(const HidlBuf& key_blob, const HidlBuf& client_id,
                                 const HidlBuf& app_data, KeyCharacteristics* key_characteristics);
    ErrorCode GetCharacteristics(const HidlBuf& key_blob, const HidlBuf& client_id,
                                 const HidlBuf& app_data, KeyCharacteristics* key_characteristics);
    ErrorCode GetCharacteristics(const HidlBuf& key_blob, KeyCharacteristics* key_characteristics);

    ErrorCode GetDebugInfo(DebugInfo* debug_info);

    ErrorCode Begin(KeyPurpose purpose, const HidlBuf& key_blob, const AuthorizationSet& in_params,
                    AuthorizationSet* out_params, OperationHandle* op_handle);
    ErrorCode Begin(KeyPurpose purpose, const AuthorizationSet& in_params,
                    AuthorizationSet* out_params);
    ErrorCode Begin(KeyPurpose purpose, const AuthorizationSet& in_params);

    ErrorCode Update(OperationHandle op_handle, const AuthorizationSet& in_params,
                     const string& input, AuthorizationSet* out_params, string* output,
                     size_t* input_consumed);
    ErrorCode Update(const string& input, string* out, size_t* input_consumed);

    ErrorCode Finish(OperationHandle op_handle, const AuthorizationSet& in_params,
                     const string& input, const string& signature, AuthorizationSet* out_params,
                     string* output);
    ErrorCode Finish(const string& message, string* output);
    ErrorCode Finish(const string& message, const string& signature, string* output);
    ErrorCode Finish(string* output) { return Finish(string(), output); }

    ErrorCode Abort(OperationHandle op_handle);

    void AbortIfNeeded();

    ErrorCode AttestKey(const HidlBuf& key_blob, const AuthorizationSet& attest_params,
                        hidl_vec<hidl_vec<uint8_t>>* cert_chain);
    ErrorCode AttestKey(const AuthorizationSet& attest_params,
                        hidl_vec<hidl_vec<uint8_t>>* cert_chain);

    string ProcessMessage(const HidlBuf& key_blob, KeyPurpose operation, const string& message,
                          const AuthorizationSet& in_params, AuthorizationSet* out_params);

    string SignMessage(const HidlBuf& key_blob, const string& message,
                       const AuthorizationSet& params);
    string SignMessage(const string& message, const AuthorizationSet& params);

    string MacMessage(const string& message, Digest digest, size_t mac_length);

    void CheckHmacTestVector(const string& key, const string& message, Digest digest,
                             const string& expected_mac);

    void CheckAesCtrTestVector(const string& key, const string& nonce, const string& message,
                               const string& expected_ciphertext);

    void CheckTripleDesTestVector(KeyPurpose purpose, BlockMode block_mode,
                                  PaddingMode padding_mode, const string& key, const string& iv,
                                  const string& input, const string& expected_output);

    void VerifyMessage(const HidlBuf& key_blob, const string& message, const string& signature,
                       const AuthorizationSet& params);
    void VerifyMessage(const string& message, const string& signature,
                       const AuthorizationSet& params);

    string EncryptMessage(const HidlBuf& key_blob, const string& message,
                          const AuthorizationSet& in_params, AuthorizationSet* out_params);
    string EncryptMessage(const string& message, const AuthorizationSet& params,
                          AuthorizationSet* out_params);
    string EncryptMessage(const string& message, const AuthorizationSet& params);
    string EncryptMessage(const string& message, BlockMode block_mode, PaddingMode padding);
    string EncryptMessage(const string& message, BlockMode block_mode, PaddingMode padding,
                          HidlBuf* iv_out);
    string EncryptMessage(const string& message, BlockMode block_mode, PaddingMode padding,
                          const HidlBuf& iv_in);
    string EncryptMessage(const string& message, BlockMode block_mode, PaddingMode padding,
                          uint8_t mac_length_bits, const HidlBuf& iv_in);

    string DecryptMessage(const HidlBuf& key_blob, const string& ciphertext,
                          const AuthorizationSet& params);
    string DecryptMessage(const string& ciphertext, const AuthorizationSet& params);
    string DecryptMessage(const string& ciphertext, BlockMode block_mode, PaddingMode padding_mode,
                          const HidlBuf& iv);

    std::pair<ErrorCode, HidlBuf> UpgradeKey(const HidlBuf& key_blob);

    bool IsSecure() { return securityLevel_ != SecurityLevel::SOFTWARE; }
    SecurityLevel SecLevel() { return securityLevel_; }

    std::vector<uint32_t> ValidKeySizes(Algorithm algorithm);
    std::vector<uint32_t> InvalidKeySizes(Algorithm algorithm);

    std::vector<EcCurve> ValidCurves();
    std::vector<EcCurve> InvalidCurves();

    std::vector<Digest> ValidDigests(bool withNone, bool withMD5);
    std::vector<Digest> InvalidDigests();

    HidlBuf key_blob_;
    KeyCharacteristics key_characteristics_;
    OperationHandle op_handle_ = kOpHandleSentinel;

   private:
     sp<IKeymasterDevice> keymaster_;
     std::vector<sp<IKeymasterDevice>> all_keymasters_;
     uint32_t os_version_;
     uint32_t os_patch_level_;

     SecurityLevel securityLevel_;
     hidl_string name_;
     hidl_string author_;
     string service_name_;
};

}  // namespace test
}  // namespace V4_0
}  // namespace keymaster
}  // namespace hardware
}  // namespace android
