/*
 * Copyright 2023, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "MacsecPskPlugin.h"
#include <openssl/cipher.h>
#include <openssl/mem.h>

#include <android-base/format.h>
#include <android-base/logging.h>

namespace aidl::android::hardware::macsec {

constexpr auto ok = &ndk::ScopedAStatus::ok;

// vendor should hide the key in TEE/TA
// CAK key can be either 16 / 32 bytes
const std::vector<uint8_t> CAK_ID_1 = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
const std::vector<uint8_t> CAK_KEY_1 = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
                                        0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};
std::vector<uint8_t> CKN_1 = {0x31, 0x32, 0x33, 0x34};  // maximum 16 bytes

const std::vector<uint8_t> CAK_ID_2 = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02};
const std::vector<uint8_t> CAK_KEY_2 = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
                                        0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
                                        0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
                                        0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};
std::vector<uint8_t> CKN_2 = {0x35, 0x36, 0x37, 0x38};  // maximum 16 bytes

static ndk::ScopedAStatus resultToStatus(binder_exception_t res, const std::string& msg = "") {
    if (msg.empty()) {
        return ndk::ScopedAStatus::fromExceptionCode(res);
    }
    return ndk::ScopedAStatus::fromExceptionCodeWithMessage(res, msg.c_str());
}

static int omac1_aes(CMAC_CTX* ctx, const uint8_t* data, size_t data_len,
                     uint8_t* mac /* 16 bytes */) {
    size_t outlen;

    // Just reuse same key in ctx
    if (!CMAC_Reset(ctx)) {
        return -1;
    }

    if (!CMAC_Update(ctx, data, data_len)) {
        return -1;
    }

    if (!CMAC_Final(ctx, mac, &outlen) || outlen != 16) {
        return -1;
    }
    return 0;
}

static void put_be16(uint8_t* addr, uint16_t value) {
    *addr++ = value >> 8;
    *addr = value & 0xff;
}

/* IEEE Std 802.1X-2010, 6.2.1 KDF */
static int aes_kdf(CMAC_CTX* ctx, const char* label, const uint8_t* context, int ctx_bits,
                   int ret_bits, uint8_t* ret) {
    const int h = 128;
    const int r = 8;
    int i, n;
    int lab_len, ctx_len, ret_len, buf_len;
    uint8_t* buf;

    lab_len = strlen(label);
    ctx_len = (ctx_bits + 7) / 8;
    ret_len = ((ret_bits & 0xffff) + 7) / 8;
    buf_len = lab_len + ctx_len + 4;

    memset(ret, 0, ret_len);

    n = (ret_bits + h - 1) / h;
    if (n > ((0x1 << r) - 1)) return -1;

    buf = (uint8_t*)calloc(1, buf_len);
    if (buf == NULL) return -1;

    memcpy(buf + 1, label, lab_len);
    memcpy(buf + lab_len + 2, context, ctx_len);
    put_be16(&buf[buf_len - 2], ret_bits);

    for (i = 0; i < n; i++) {
        int res;

        buf[0] = (uint8_t)(i + 1);
        res = omac1_aes(ctx, buf, buf_len, ret);
        if (res) {
            free(buf);
            return -1;
        }
        ret = ret + h / 8;
    }
    free(buf);
    return 0;
}

MacsecPskPlugin::MacsecPskPlugin() {
    // always make sure ckn is 16 bytes, zero padded
    CKN_1.resize(16);
    CKN_2.resize(16);

    addTestKey(CAK_ID_1, CAK_KEY_1, CKN_1);
    addTestKey(CAK_ID_2, CAK_KEY_2, CKN_2);
}

MacsecPskPlugin::~MacsecPskPlugin() {
    for (auto s : mKeys) {
        OPENSSL_cleanse(&s.kekEncCtx, sizeof(AES_KEY));
        OPENSSL_cleanse(&s.kekDecCtx, sizeof(AES_KEY));
        CMAC_CTX_free(s.ickCtx);
        CMAC_CTX_free(s.cakCtx);
    }
}

ndk::ScopedAStatus MacsecPskPlugin::addTestKey(const std::vector<uint8_t>& keyId,
                                               const std::vector<uint8_t>& CAK,
                                               const std::vector<uint8_t>& CKN) {
    if (CAK.size() != 16 && CAK.size() != 32) {
        return resultToStatus(EX_ILLEGAL_ARGUMENT, "CAK length must be 16 or 32 bytes");
    }

    if (keyId.size() != CAK.size()) {
        return resultToStatus(EX_ILLEGAL_ARGUMENT, "Key ID must be same as CAK length");
    }

    std::vector<uint8_t> ckn;
    ckn = CKN;
    ckn.resize(16);  // make sure it is always zero padded with maximum length of
                     // 16 bytes

    AES_KEY kekEncCtx;
    AES_KEY kekDecCtx;
    CMAC_CTX* ickCtx;
    CMAC_CTX* cakCtx;

    // Create the CAK openssl context
    cakCtx = CMAC_CTX_new();

    CMAC_Init(cakCtx, CAK.data(), CAK.size(),
              CAK.size() == 16 ? EVP_aes_128_cbc() : EVP_aes_256_cbc(), NULL);

    // derive KEK from CAK (ieee802_1x_kek_aes_cmac)
    std::vector<uint8_t> kek;
    kek.resize(CAK.size());

    aes_kdf(cakCtx, "IEEE8021 KEK", (const uint8_t*)ckn.data(), ckn.size() * 8, 8 * kek.size(),
            kek.data());

    AES_set_encrypt_key(kek.data(), kek.size() << 3, &kekEncCtx);
    AES_set_decrypt_key(kek.data(), kek.size() << 3, &kekDecCtx);

    // derive ICK from CAK (ieee802_1x_ick_aes_cmac)
    std::vector<uint8_t> ick;
    ick.resize(CAK.size());

    aes_kdf(cakCtx, "IEEE8021 ICK", (const uint8_t*)CKN.data(), CKN.size() * 8, 8 * ick.size(),
            ick.data());

    ickCtx = CMAC_CTX_new();

    CMAC_Init(ickCtx, ick.data(), ick.size(),
              ick.size() == 16 ? EVP_aes_128_cbc() : EVP_aes_256_cbc(), NULL);

    mKeys.push_back({keyId, kekEncCtx, kekDecCtx, ickCtx, cakCtx});

    return ok();
}

ndk::ScopedAStatus MacsecPskPlugin::calcIcv(const std::vector<uint8_t>& keyId,
                                            const std::vector<uint8_t>& data,
                                            std::vector<uint8_t>* out) {
    CMAC_CTX* ctx = NULL;

    for (auto s : mKeys) {
        if (s.keyId == keyId) {
            ctx = s.ickCtx;
            break;
        }
    }

    if (ctx == NULL) {
        return resultToStatus(EX_ILLEGAL_ARGUMENT, "Key not exist");
    }

    out->resize(16);
    if (omac1_aes(ctx, data.data(), data.size(), out->data()) != 0) {
        return resultToStatus(EX_SERVICE_SPECIFIC, "Internal error");
    }

    return ok();
}

ndk::ScopedAStatus MacsecPskPlugin::generateSak(const std::vector<uint8_t>& keyId,
                                                const std::vector<uint8_t>& data,
                                                const int sakLength, std::vector<uint8_t>* out) {
    CMAC_CTX* ctx = NULL;

    if ((sakLength != 16) && (sakLength != 32)) {
        return resultToStatus(EX_ILLEGAL_ARGUMENT, "Invalid SAK length");
    }

    if (data.size() < sakLength) {
        return resultToStatus(EX_ILLEGAL_ARGUMENT, "Invalid data length");
    }

    for (auto s : mKeys) {
        if (s.keyId == keyId) {
            ctx = s.cakCtx;
            break;
        }
    }

    if (ctx == NULL) {
        return resultToStatus(EX_ILLEGAL_ARGUMENT, "Key not exist");
    }

    out->resize(sakLength);

    if (aes_kdf(ctx, "IEEE8021 SAK", data.data(), data.size() * 8, out->size() * 8, out->data()) !=
        0) {
        return resultToStatus(EX_SERVICE_SPECIFIC, "Internal error");
    }

    return ok();
}

ndk::ScopedAStatus MacsecPskPlugin::wrapSak(const std::vector<uint8_t>& keyId,
                                            const std::vector<uint8_t>& sak,
                                            std::vector<uint8_t>* out) {
    if (sak.size() == 0 || sak.size() % 8 != 0) {
        return resultToStatus(EX_ILLEGAL_ARGUMENT,
                              "SAK length not multiple of 8 or greater than 0");
    }

    AES_KEY* ctx = NULL;

    for (auto s : mKeys) {
        if (s.keyId == keyId) {
            ctx = &s.kekEncCtx;
            break;
        }
    }

    if (ctx == NULL) {
        return resultToStatus(EX_ILLEGAL_ARGUMENT, "Key not exist");
    }

    out->resize(sak.size() + 8);

    if (AES_wrap_key(ctx, NULL, out->data(), sak.data(), sak.size()) > 0) {
        return ok();
    }

    return resultToStatus(EX_SERVICE_SPECIFIC, "Internal error");
}

ndk::ScopedAStatus MacsecPskPlugin::unwrapSak(const std::vector<uint8_t>& keyId,
                                              const std::vector<uint8_t>& sak,
                                              std::vector<uint8_t>* out) {
    if (sak.size() <= 8 || sak.size() % 8 != 0) {
        return resultToStatus(EX_ILLEGAL_ARGUMENT,
                              "SAK length not multiple of 8 or greater than 0");
    }

    AES_KEY* ctx = NULL;

    for (auto s : mKeys) {
        if (s.keyId == keyId) {
            ctx = &s.kekDecCtx;
            break;
        }
    }

    if (ctx == NULL) {
        return resultToStatus(EX_ILLEGAL_ARGUMENT, "Key not exist");
    }

    out->resize(sak.size() - 8);

    if (AES_unwrap_key(ctx, NULL, out->data(), sak.data(), sak.size()) > 0) {
        return ok();
    }

    return resultToStatus(EX_SERVICE_SPECIFIC, "Internal error");
}

}  // namespace aidl::android::hardware::macsec
