/*
 * Copyright 2020 The Android Open Source Project
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

#include <aidl/android/hardware/security/keymint/Digest.h>

#include <openssl/evp.h>
#include <openssl/x509.h>

namespace aidl::android::hardware::security::keymint {

template <typename T, void (*F)(T*)>
struct UniquePtrDeleter {
    void operator()(T* p) const { F(p); }
};

typedef UniquePtrDeleter<EVP_PKEY, EVP_PKEY_free> EVP_PKEY_Delete;

#define MAKE_OPENSSL_PTR_TYPE(type) \
    typedef std::unique_ptr<type, UniquePtrDeleter<type, type##_free>> type##_Ptr;

MAKE_OPENSSL_PTR_TYPE(ASN1_OBJECT)
MAKE_OPENSSL_PTR_TYPE(BN_CTX)
MAKE_OPENSSL_PTR_TYPE(EC_GROUP)
MAKE_OPENSSL_PTR_TYPE(EC_KEY)
MAKE_OPENSSL_PTR_TYPE(EC_POINT)
MAKE_OPENSSL_PTR_TYPE(EVP_PKEY)
MAKE_OPENSSL_PTR_TYPE(EVP_PKEY_CTX)
MAKE_OPENSSL_PTR_TYPE(RSA)
MAKE_OPENSSL_PTR_TYPE(X509)
MAKE_OPENSSL_PTR_TYPE(X509_NAME)

typedef std::unique_ptr<BIGNUM, UniquePtrDeleter<BIGNUM, BN_free>> BIGNUM_Ptr;

inline const EVP_MD* openssl_digest(Digest digest) {
    switch (digest) {
        case Digest::NONE:
            return nullptr;
        case Digest::MD5:
            return EVP_md5();
        case Digest::SHA1:
            return EVP_sha1();
        case Digest::SHA_2_224:
            return EVP_sha224();
        case Digest::SHA_2_256:
            return EVP_sha256();
        case Digest::SHA_2_384:
            return EVP_sha384();
        case Digest::SHA_2_512:
            return EVP_sha512();
    }
    return nullptr;
}

}  // namespace aidl::android::hardware::security::keymint
