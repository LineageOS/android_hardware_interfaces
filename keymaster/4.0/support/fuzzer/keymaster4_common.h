/*
 * Copyright (C) 2021 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#ifndef __KEYMASTER4_COMMON_H__
#define __KEYMASTER4_COMMON_H__

#include <fuzzer/FuzzedDataProvider.h>
#include <keymasterV4_0/authorization_set.h>

namespace android::hardware::keymaster::V4_0::fuzzer {

using ::android::hardware::hidl_vec;

constexpr uint32_t kKeySize = 2048;
constexpr uint32_t kPublicExponent = 65537;

constexpr EcCurve kCurve[] = {EcCurve::P_224, EcCurve::P_256, EcCurve::P_384, EcCurve::P_521};

constexpr PaddingMode kPaddingMode[] = {
        PaddingMode::NONE,
        PaddingMode::RSA_OAEP,
        PaddingMode::RSA_PSS,
        PaddingMode::RSA_PKCS1_1_5_ENCRYPT,
        PaddingMode::RSA_PKCS1_1_5_SIGN,
        PaddingMode::PKCS7,
};

constexpr Digest kDigest[] = {
        Digest::NONE,      Digest::MD5,       Digest::SHA1,      Digest::SHA_2_224,
        Digest::SHA_2_256, Digest::SHA_2_384, Digest::SHA_2_512,
};

enum AuthSet : uint32_t {
    RSA_SIGNING_KEY = 0u,
    RSA_ENCRYPRION_KEY,
    ECDSA_SIGNING_CURVE,
    ECDSA_SIGNING_KEY,
    AES_ENCRYPTION_KEY,
    TRIPLE_DES,
    HMAC,
    NO_DIGEST,
    ECB_MODE,
    GSM_MODE_MIN_MAC,
    GSM_MODE_MAC,
    BLOCK_MODE,
    kMaxValue = BLOCK_MODE
};

AuthorizationSet createAuthorizationSet(std::unique_ptr<FuzzedDataProvider>& dataProvider) {
    uint32_t authSet = dataProvider->ConsumeEnum<AuthSet>();
    switch (authSet) {
        case RSA_SIGNING_KEY: {
            Digest digest = dataProvider->PickValueInArray(kDigest);
            PaddingMode padding = dataProvider->PickValueInArray(kPaddingMode);
            return AuthorizationSetBuilder()
                    .Authorization(TAG_NO_AUTH_REQUIRED)
                    .RsaSigningKey(kKeySize, kPublicExponent)
                    .Digest(digest)
                    .Padding(padding)
                    .Authorization(TAG_INCLUDE_UNIQUE_ID);
        } break;
        case RSA_ENCRYPRION_KEY: {
            Digest digest = dataProvider->PickValueInArray(kDigest);
            PaddingMode padding = dataProvider->PickValueInArray(kPaddingMode);
            return AuthorizationSetBuilder()
                    .Authorization(TAG_NO_AUTH_REQUIRED)
                    .RsaEncryptionKey(kKeySize, kPublicExponent)
                    .Digest(digest)
                    .Padding(padding)
                    .Authorization(TAG_INCLUDE_UNIQUE_ID);
        } break;
        case ECDSA_SIGNING_CURVE: {
            EcCurve ecCurve = dataProvider->PickValueInArray(kCurve);
            Digest digest = dataProvider->PickValueInArray(kDigest);
            return AuthorizationSetBuilder()
                    .Authorization(TAG_NO_AUTH_REQUIRED)
                    .EcdsaSigningKey(ecCurve)
                    .Digest(digest)
                    .Authorization(TAG_INCLUDE_UNIQUE_ID);
        } break;
        case ECDSA_SIGNING_KEY: {
            Digest digest = dataProvider->PickValueInArray(kDigest);
            return AuthorizationSetBuilder()
                    .Authorization(TAG_NO_AUTH_REQUIRED)
                    .EcdsaSigningKey(kKeySize)
                    .Digest(digest)
                    .Authorization(TAG_INCLUDE_UNIQUE_ID);
        } break;
        case AES_ENCRYPTION_KEY: {
            Digest digest = dataProvider->PickValueInArray(kDigest);
            PaddingMode padding = dataProvider->PickValueInArray(kPaddingMode);
            return AuthorizationSetBuilder()
                    .Authorization(TAG_NO_AUTH_REQUIRED)
                    .AesEncryptionKey(kKeySize)
                    .Digest(digest)
                    .Padding(padding)
                    .Authorization(TAG_INCLUDE_UNIQUE_ID);
        } break;
        case TRIPLE_DES: {
            Digest digest = dataProvider->PickValueInArray(kDigest);
            PaddingMode padding = dataProvider->PickValueInArray(kPaddingMode);
            return AuthorizationSetBuilder()
                    .Authorization(TAG_NO_AUTH_REQUIRED)
                    .TripleDesEncryptionKey(kKeySize)
                    .Digest(digest)
                    .Padding(padding)
                    .Authorization(TAG_INCLUDE_UNIQUE_ID);
        } break;
        case HMAC: {
            Digest digest = dataProvider->PickValueInArray(kDigest);
            PaddingMode padding = dataProvider->PickValueInArray(kPaddingMode);
            return AuthorizationSetBuilder()
                    .Authorization(TAG_NO_AUTH_REQUIRED)
                    .HmacKey(kKeySize)
                    .Digest(digest)
                    .Padding(padding)
                    .Authorization(TAG_INCLUDE_UNIQUE_ID);
        } break;
        case NO_DIGEST: {
            Digest digest = dataProvider->PickValueInArray(kDigest);
            PaddingMode padding = dataProvider->PickValueInArray(kPaddingMode);
            return AuthorizationSetBuilder()
                    .Authorization(TAG_NO_AUTH_REQUIRED)
                    .NoDigestOrPadding()
                    .Digest(digest)
                    .Padding(padding)
                    .Authorization(TAG_INCLUDE_UNIQUE_ID);
        } break;
        case ECB_MODE: {
            Digest digest = dataProvider->PickValueInArray(kDigest);
            PaddingMode padding = dataProvider->PickValueInArray(kPaddingMode);
            return AuthorizationSetBuilder()
                    .Authorization(TAG_NO_AUTH_REQUIRED)
                    .EcbMode()
                    .Digest(digest)
                    .Padding(padding)
                    .Authorization(TAG_INCLUDE_UNIQUE_ID);
        } break;
        case GSM_MODE_MIN_MAC: {
            uint32_t minMacLength = dataProvider->ConsumeIntegral<uint32_t>();
            Digest digest = dataProvider->PickValueInArray(kDigest);
            PaddingMode padding = dataProvider->PickValueInArray(kPaddingMode);
            return AuthorizationSetBuilder()
                    .Authorization(TAG_NO_AUTH_REQUIRED)
                    .GcmModeMinMacLen(minMacLength)
                    .Digest(digest)
                    .Padding(padding)
                    .Authorization(TAG_INCLUDE_UNIQUE_ID);
        } break;
        case GSM_MODE_MAC: {
            uint32_t macLength = dataProvider->ConsumeIntegral<uint32_t>();
            Digest digest = dataProvider->PickValueInArray(kDigest);
            PaddingMode padding = dataProvider->PickValueInArray(kPaddingMode);
            return AuthorizationSetBuilder()
                    .Authorization(TAG_NO_AUTH_REQUIRED)
                    .GcmModeMacLen(macLength)
                    .Digest(digest)
                    .Padding(padding)
                    .Authorization(TAG_INCLUDE_UNIQUE_ID);
        } break;
        case BLOCK_MODE: {
            Digest digest = dataProvider->PickValueInArray(kDigest);
            PaddingMode padding = dataProvider->PickValueInArray(kPaddingMode);
            auto blockModes = {
                    BlockMode::ECB,
                    BlockMode::CBC,
                    BlockMode::CTR,
                    BlockMode::GCM,
            };
            return AuthorizationSetBuilder()
                    .Authorization(TAG_NO_AUTH_REQUIRED)
                    .BlockMode(blockModes)
                    .Digest(digest)
                    .Padding(padding)
                    .Authorization(TAG_INCLUDE_UNIQUE_ID);
        } break;
        default:
            break;
    };
    return AuthorizationSetBuilder();
}

}  // namespace android::hardware::keymaster::V4_0::fuzzer

#endif  // __KEYMASTER4_COMMON_H__
