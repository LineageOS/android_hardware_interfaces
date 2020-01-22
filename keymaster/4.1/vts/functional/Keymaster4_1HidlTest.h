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

#pragma once

#include <android/hardware/keymaster/4.1/IKeymasterDevice.h>

#include <KeymasterHidlTest.h>
#include <keymasterV4_1/authorization_set.h>

namespace android::hardware::keymaster::V4_1::test {

using V4_0::test::HidlBuf;

class Keymaster4_1HidlTest : public V4_0::test::KeymasterHidlTest {
  public:
    using super = V4_0::test::KeymasterHidlTest;

    ErrorCode convert(V4_0::ErrorCode error_code) { return static_cast<ErrorCode>(error_code); }

    // These methods hide the base class versions.
    void SetUp();
    IKeymasterDevice& keymaster() { return *keymaster41_; };

    struct KeyData {
        HidlBuf blob;
        KeyCharacteristics characteristics;
    };

    std::tuple<ErrorCode, KeyData> GenerateKeyData(const AuthorizationSet& keyDescription) {
        KeyData keyData;
        ErrorCode errorCode = convert(
                super::GenerateKey(keyDescription, &keyData.blob, &keyData.characteristics));
        return {errorCode, keyData};
    }

    void CheckedDeleteKeyData(KeyData* keyData) { CheckedDeleteKey(&keyData->blob); }

    template <typename TagType>
    std::tuple<KeyData /* aesKey */, KeyData /* hmacKey */, KeyData /* rsaKey */,
               KeyData /* ecdsaKey */>
    CreateTestKeys(TagType tagToTest, ErrorCode expectedReturn) {
        ErrorCode errorCode;

        /* AES */
        KeyData aesKeyData;
        std::tie(errorCode, aesKeyData) =
                GenerateKeyData(AuthorizationSetBuilder()
                                        .AesEncryptionKey(128)
                                        .Authorization(tagToTest)
                                        .BlockMode(BlockMode::ECB)
                                        .Padding(PaddingMode::NONE)
                                        .Authorization(TAG_NO_AUTH_REQUIRED));
        EXPECT_EQ(expectedReturn, errorCode);

        /* HMAC */
        KeyData hmacKeyData;
        std::tie(errorCode, hmacKeyData) =
                GenerateKeyData(AuthorizationSetBuilder()
                                        .HmacKey(128)
                                        .Authorization(tagToTest)
                                        .Digest(Digest::SHA_2_256)
                                        .Authorization(TAG_MIN_MAC_LENGTH, 128)
                                        .Authorization(TAG_NO_AUTH_REQUIRED));
        EXPECT_EQ(expectedReturn, errorCode);

        /* RSA */
        KeyData rsaKeyData;
        std::tie(errorCode, rsaKeyData) =
                GenerateKeyData(AuthorizationSetBuilder()
                                        .RsaSigningKey(2048, 65537)
                                        .Authorization(tagToTest)
                                        .Digest(Digest::NONE)
                                        .Padding(PaddingMode::NONE)
                                        .Authorization(TAG_NO_AUTH_REQUIRED));
        EXPECT_EQ(expectedReturn, errorCode);

        /* ECDSA */
        KeyData ecdsaKeyData;
        std::tie(errorCode, ecdsaKeyData) =
                GenerateKeyData(AuthorizationSetBuilder()
                                        .EcdsaSigningKey(256)
                                        .Authorization(tagToTest)
                                        .Digest(Digest::SHA_2_256)
                                        .Authorization(TAG_NO_AUTH_REQUIRED));
        EXPECT_EQ(expectedReturn, errorCode);

        return {aesKeyData, hmacKeyData, rsaKeyData, ecdsaKeyData};
    }

    std::tuple<ErrorCode, std::string /* processedMessage */, AuthorizationSet /* out_params */>
    ProcessMessage(const HidlBuf& key_blob, KeyPurpose operation, const std::string& message,
                   const AuthorizationSet& in_params);

    ErrorCode UseAesKey(const HidlBuf& aesKeyBlob) {
        auto [result, ciphertext, out_params] = ProcessMessage(
                aesKeyBlob, KeyPurpose::ENCRYPT, "1234567890123456",
                AuthorizationSetBuilder().BlockMode(BlockMode::ECB).Padding(PaddingMode::NONE));
        return result;
    }

    ErrorCode UseHmacKey(const HidlBuf& hmacKeyBlob) {
        auto [result, mac, out_params] =
                ProcessMessage(hmacKeyBlob, KeyPurpose::SIGN, "1234567890123456",
                               AuthorizationSetBuilder().Authorization(TAG_MAC_LENGTH, 128));
        return result;
    }

    ErrorCode UseRsaKey(const HidlBuf& rsaKeyBlob) {
        std::string message(2048 / 8, 'a');
        auto [result, signature, out_params] = ProcessMessage(
                rsaKeyBlob, KeyPurpose::SIGN, message,
                AuthorizationSetBuilder().Digest(Digest::NONE).Padding(PaddingMode::NONE));
        return result;
    }

    ErrorCode UseEcdsaKey(const HidlBuf& ecdsaKeyBlob) {
        auto [result, signature, out_params] =
                ProcessMessage(ecdsaKeyBlob, KeyPurpose::SIGN, "a",
                               AuthorizationSetBuilder().Digest(Digest::SHA_2_256));
        return result;
    }

    static std::vector<std::string> build_params() {
        auto params = android::hardware::getAllHalInstanceNames(IKeymasterDevice::descriptor);
        return params;
    }

  private:
    sp<IKeymasterDevice> keymaster41_;
};

template <typename TypedTag>
bool contains(hidl_vec<KeyParameter>& set, TypedTag typedTag) {
    return std::find_if(set.begin(), set.end(), [&](const KeyParameter& param) {
               return param.tag == static_cast<V4_0::Tag>(typedTag);
           }) != set.end();
}

#define INSTANTIATE_KEYMASTER_4_1_HIDL_TEST(name)                                     \
    INSTANTIATE_TEST_SUITE_P(PerInstance, name,                                       \
                             testing::ValuesIn(Keymaster4_1HidlTest::build_params()), \
                             android::hardware::PrintInstanceNameToString)

}  // namespace android::hardware::keymaster::V4_1::test
