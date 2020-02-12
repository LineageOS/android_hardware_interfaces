/*
 * Copyright 2017 The Android Open Source Project
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

#ifndef HARDWARE_INTERFACES_KEYMASTER_4_0_SUPPORT_OPENSSL_UTILS_H_
#define HARDWARE_INTERFACES_KEYMASTER_4_0_SUPPORT_OPENSSL_UTILS_H_

#include <android/hardware/keymaster/4.0/types.h>
#include <openssl/evp.h>
#include <openssl/x509.h>

template <typename T, void (*F)(T*)>
struct UniquePtrDeleter {
    void operator()(T* p) const { F(p); }
};

typedef UniquePtrDeleter<EVP_PKEY, EVP_PKEY_free> EVP_PKEY_Delete;

#define MAKE_OPENSSL_PTR_TYPE(type) \
    typedef std::unique_ptr<type, UniquePtrDeleter<type, type##_free>> type##_Ptr;

MAKE_OPENSSL_PTR_TYPE(ASN1_OBJECT)
MAKE_OPENSSL_PTR_TYPE(EVP_PKEY)
MAKE_OPENSSL_PTR_TYPE(RSA)
MAKE_OPENSSL_PTR_TYPE(X509)
MAKE_OPENSSL_PTR_TYPE(BN_CTX)

typedef std::unique_ptr<BIGNUM, UniquePtrDeleter<BIGNUM, BN_free>> BIGNUM_Ptr;

inline const EVP_MD* openssl_digest(android::hardware::keymaster::V4_0::Digest digest) {
    switch (digest) {
        case android::hardware::keymaster::V4_0::Digest::NONE:
            return nullptr;
        case android::hardware::keymaster::V4_0::Digest::MD5:
            return EVP_md5();
        case android::hardware::keymaster::V4_0::Digest::SHA1:
            return EVP_sha1();
        case android::hardware::keymaster::V4_0::Digest::SHA_2_224:
            return EVP_sha224();
        case android::hardware::keymaster::V4_0::Digest::SHA_2_256:
            return EVP_sha256();
        case android::hardware::keymaster::V4_0::Digest::SHA_2_384:
            return EVP_sha384();
        case android::hardware::keymaster::V4_0::Digest::SHA_2_512:
            return EVP_sha512();
    }
    return nullptr;
}

#endif  // HARDWARE_INTERFACES_KEYMASTER_4_0_SUPPORT_OPENSSL_UTILS_H_
