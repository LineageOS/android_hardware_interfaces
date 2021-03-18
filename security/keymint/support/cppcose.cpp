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

#include <cppcose/cppcose.h>

#include <stdio.h>
#include <iostream>

#include <cppbor.h>
#include <cppbor_parse.h>

#include <openssl/err.h>

namespace cppcose {

namespace {

ErrMsgOr<bssl::UniquePtr<EVP_CIPHER_CTX>> aesGcmInitAndProcessAad(const bytevec& key,
                                                                  const bytevec& nonce,
                                                                  const bytevec& aad,
                                                                  bool encrypt) {
    if (key.size() != kAesGcmKeySize) return "Invalid key size";

    bssl::UniquePtr<EVP_CIPHER_CTX> ctx(EVP_CIPHER_CTX_new());
    if (!ctx) return "Failed to allocate cipher context";

    if (!EVP_CipherInit_ex(ctx.get(), EVP_aes_256_gcm(), nullptr /* engine */, key.data(),
                           nonce.data(), encrypt ? 1 : 0)) {
        return "Failed to initialize cipher";
    }

    int outlen;
    if (!aad.empty() && !EVP_CipherUpdate(ctx.get(), nullptr /* out; null means AAD */, &outlen,
                                          aad.data(), aad.size())) {
        return "Failed to process AAD";
    }

    return std::move(ctx);
}

}  // namespace

ErrMsgOr<bytevec> generateCoseMac0Mac(const bytevec& macKey, const bytevec& externalAad,
                                      const bytevec& payload) {
    auto macStructure = cppbor::Array()
                                .add("MAC0")
                                .add(cppbor::Map().add(ALGORITHM, HMAC_256).canonicalize().encode())
                                .add(externalAad)
                                .add(payload)
                                .encode();

    bytevec macTag(SHA256_DIGEST_LENGTH);
    uint8_t* out = macTag.data();
    unsigned int outLen;
    out = HMAC(EVP_sha256(),                              //
               macKey.data(), macKey.size(),              //
               macStructure.data(), macStructure.size(),  //
               out, &outLen);

    assert(out != nullptr && outLen == macTag.size());
    if (out == nullptr || outLen != macTag.size()) {
        return "Error computing public key MAC";
    }

    return macTag;
}

ErrMsgOr<cppbor::Array> constructCoseMac0(const bytevec& macKey, const bytevec& externalAad,
                                          const bytevec& payload) {
    auto tag = generateCoseMac0Mac(macKey, externalAad, payload);
    if (!tag) return tag.moveMessage();

    return cppbor::Array()
            .add(cppbor::Map().add(ALGORITHM, HMAC_256).canonicalize().encode())
            .add(cppbor::Map() /* unprotected */)
            .add(payload)
            .add(tag.moveValue());
}

ErrMsgOr<bytevec /* payload */> parseCoseMac0(const cppbor::Item* macItem) {
    auto mac = macItem ? macItem->asArray() : nullptr;
    if (!mac || mac->size() != kCoseMac0EntryCount) {
        return "Invalid COSE_Mac0";
    }

    auto protectedParms = mac->get(kCoseMac0ProtectedParams)->asBstr();
    auto unprotectedParms = mac->get(kCoseMac0UnprotectedParams)->asMap();
    auto payload = mac->get(kCoseMac0Payload)->asBstr();
    auto tag = mac->get(kCoseMac0Tag)->asBstr();
    if (!protectedParms || !unprotectedParms || !payload || !tag) {
        return "Invalid COSE_Mac0 contents";
    }

    return payload->value();
}

ErrMsgOr<bytevec /* payload */> verifyAndParseCoseMac0(const cppbor::Item* macItem,
                                                       const bytevec& macKey) {
    auto mac = macItem ? macItem->asArray() : nullptr;
    if (!mac || mac->size() != kCoseMac0EntryCount) {
        return "Invalid COSE_Mac0";
    }

    auto protectedParms = mac->get(kCoseMac0ProtectedParams)->asBstr();
    auto unprotectedParms = mac->get(kCoseMac0UnprotectedParams)->asMap();
    auto payload = mac->get(kCoseMac0Payload)->asBstr();
    auto tag = mac->get(kCoseMac0Tag)->asBstr();
    if (!protectedParms || !unprotectedParms || !payload || !tag) {
        return "Invalid COSE_Mac0 contents";
    }

    auto [protectedMap, _, errMsg] = cppbor::parse(protectedParms);
    if (!protectedMap || !protectedMap->asMap()) {
        return "Invalid Mac0 protected: " + errMsg;
    }
    auto& algo = protectedMap->asMap()->get(ALGORITHM);
    if (!algo || !algo->asInt() || algo->asInt()->value() != HMAC_256) {
        return "Unsupported Mac0 algorithm";
    }

    auto macTag = generateCoseMac0Mac(macKey, {} /* external_aad */, payload->value());
    if (!macTag) return macTag.moveMessage();

    if (macTag->size() != tag->value().size() ||
        CRYPTO_memcmp(macTag->data(), tag->value().data(), macTag->size()) != 0) {
        return "MAC tag mismatch";
    }

    return payload->value();
}

ErrMsgOr<bytevec> createCoseSign1Signature(const bytevec& key, const bytevec& protectedParams,
                                           const bytevec& payload, const bytevec& aad) {
    bytevec signatureInput = cppbor::Array()
                                     .add("Signature1")  //
                                     .add(protectedParams)
                                     .add(aad)
                                     .add(payload)
                                     .encode();

    if (key.size() != ED25519_PRIVATE_KEY_LEN) return "Invalid signing key";
    bytevec signature(ED25519_SIGNATURE_LEN);
    if (!ED25519_sign(signature.data(), signatureInput.data(), signatureInput.size(), key.data())) {
        return "Signing failed";
    }

    return signature;
}

ErrMsgOr<cppbor::Array> constructCoseSign1(const bytevec& key, cppbor::Map protectedParams,
                                           const bytevec& payload, const bytevec& aad) {
    bytevec protParms = protectedParams.add(ALGORITHM, EDDSA).canonicalize().encode();
    auto signature = createCoseSign1Signature(key, protParms, payload, aad);
    if (!signature) return signature.moveMessage();

    return cppbor::Array()
            .add(protParms)
            .add(cppbor::Map() /* unprotected parameters */)
            .add(payload)
            .add(*signature);
}

ErrMsgOr<cppbor::Array> constructCoseSign1(const bytevec& key, const bytevec& payload,
                                           const bytevec& aad) {
    return constructCoseSign1(key, {} /* protectedParams */, payload, aad);
}

ErrMsgOr<bytevec> verifyAndParseCoseSign1(bool ignoreSignature, const cppbor::Array* coseSign1,
                                          const bytevec& signingCoseKey, const bytevec& aad) {
    if (!coseSign1 || coseSign1->size() != kCoseSign1EntryCount) {
        return "Invalid COSE_Sign1";
    }

    const cppbor::Bstr* protectedParams = coseSign1->get(kCoseSign1ProtectedParams)->asBstr();
    const cppbor::Map* unprotectedParams = coseSign1->get(kCoseSign1UnprotectedParams)->asMap();
    const cppbor::Bstr* payload = coseSign1->get(kCoseSign1Payload)->asBstr();
    const cppbor::Bstr* signature = coseSign1->get(kCoseSign1Signature)->asBstr();

    if (!protectedParams || !unprotectedParams || !payload || !signature) {
        return "Invalid COSE_Sign1";
    }

    auto [parsedProtParams, _, errMsg] = cppbor::parse(protectedParams);
    if (!parsedProtParams) {
        return errMsg + " when parsing protected params.";
    }
    if (!parsedProtParams->asMap()) {
        return "Protected params must be a map";
    }

    auto& algorithm = parsedProtParams->asMap()->get(ALGORITHM);
    if (!algorithm || !algorithm->asInt() || algorithm->asInt()->value() != EDDSA) {
        return "Unsupported signature algorithm";
    }

    if (!ignoreSignature) {
        bool selfSigned = signingCoseKey.empty();
        auto key = CoseKey::parseEd25519(selfSigned ? payload->value() : signingCoseKey);
        if (!key) return "Bad signing key: " + key.moveMessage();

        bytevec signatureInput = cppbor::Array()
                                         .add("Signature1")
                                         .add(*protectedParams)
                                         .add(aad)
                                         .add(*payload)
                                         .encode();

        if (!ED25519_verify(signatureInput.data(), signatureInput.size(), signature->value().data(),
                            key->getBstrValue(CoseKey::PUBKEY_X)->data())) {
            return "Signature verification failed";
        }
    }

    return payload->value();
}

ErrMsgOr<bytevec> createCoseEncryptCiphertext(const bytevec& key, const bytevec& nonce,
                                              const bytevec& protectedParams,
                                              const bytevec& plaintextPayload, const bytevec& aad) {
    auto ciphertext = aesGcmEncrypt(key, nonce,
                                    cppbor::Array()                // Enc strucure as AAD
                                            .add("Encrypt")        // Context
                                            .add(protectedParams)  // Protected
                                            .add(aad)              // External AAD
                                            .encode(),
                                    plaintextPayload);

    if (!ciphertext) return ciphertext.moveMessage();
    return ciphertext.moveValue();
}

ErrMsgOr<cppbor::Array> constructCoseEncrypt(const bytevec& key, const bytevec& nonce,
                                             const bytevec& plaintextPayload, const bytevec& aad,
                                             cppbor::Array recipients) {
    auto encryptProtectedHeader = cppbor::Map()  //
                                          .add(ALGORITHM, AES_GCM_256)
                                          .canonicalize()
                                          .encode();

    auto ciphertext =
            createCoseEncryptCiphertext(key, nonce, encryptProtectedHeader, plaintextPayload, aad);
    if (!ciphertext) return ciphertext.moveMessage();

    return cppbor::Array()
            .add(encryptProtectedHeader)                       // Protected
            .add(cppbor::Map().add(IV, nonce).canonicalize())  // Unprotected
            .add(*ciphertext)                                  // Payload
            .add(std::move(recipients));
}

ErrMsgOr<std::pair<bytevec /* pubkey */, bytevec /* key ID */>> getSenderPubKeyFromCoseEncrypt(
        const cppbor::Item* coseEncrypt) {
    if (!coseEncrypt || !coseEncrypt->asArray() ||
        coseEncrypt->asArray()->size() != kCoseEncryptEntryCount) {
        return "Invalid COSE_Encrypt";
    }

    auto& recipients = coseEncrypt->asArray()->get(kCoseEncryptRecipients);
    if (!recipients || !recipients->asArray() || recipients->asArray()->size() != 1) {
        return "Invalid recipients list";
    }

    auto& recipient = recipients->asArray()->get(0);
    if (!recipient || !recipient->asArray() || recipient->asArray()->size() != 3) {
        return "Invalid COSE_recipient";
    }

    auto& ciphertext = recipient->asArray()->get(2);
    if (!ciphertext->asSimple() || !ciphertext->asSimple()->asNull()) {
        return "Unexpected value in recipients ciphertext field " +
               cppbor::prettyPrint(ciphertext.get());
    }

    auto& protParms = recipient->asArray()->get(0);
    if (!protParms || !protParms->asBstr()) return "Invalid protected params";
    auto [parsedProtParms, _, errMsg] = cppbor::parse(protParms->asBstr());
    if (!parsedProtParms) return "Failed to parse protected params: " + errMsg;
    if (!parsedProtParms->asMap()) return "Invalid protected params";

    auto& algorithm = parsedProtParms->asMap()->get(ALGORITHM);
    if (!algorithm || !algorithm->asInt() || algorithm->asInt()->value() != ECDH_ES_HKDF_256) {
        return "Invalid algorithm";
    }

    auto& unprotParms = recipient->asArray()->get(1);
    if (!unprotParms || !unprotParms->asMap()) return "Invalid unprotected params";

    auto& senderCoseKey = unprotParms->asMap()->get(COSE_KEY);
    if (!senderCoseKey || !senderCoseKey->asMap()) return "Invalid sender COSE_Key";

    auto& keyType = senderCoseKey->asMap()->get(CoseKey::KEY_TYPE);
    if (!keyType || !keyType->asInt() || keyType->asInt()->value() != OCTET_KEY_PAIR) {
        return "Invalid key type";
    }

    auto& curve = senderCoseKey->asMap()->get(CoseKey::CURVE);
    if (!curve || !curve->asInt() || curve->asInt()->value() != X25519) {
        return "Unsupported curve";
    }

    auto& pubkey = senderCoseKey->asMap()->get(CoseKey::PUBKEY_X);
    if (!pubkey || !pubkey->asBstr() ||
        pubkey->asBstr()->value().size() != X25519_PUBLIC_VALUE_LEN) {
        return "Invalid X25519 public key";
    }

    auto& key_id = unprotParms->asMap()->get(KEY_ID);
    if (key_id && key_id->asBstr()) {
        return std::make_pair(pubkey->asBstr()->value(), key_id->asBstr()->value());
    }

    // If no key ID, just return an empty vector.
    return std::make_pair(pubkey->asBstr()->value(), bytevec{});
}

ErrMsgOr<bytevec> decryptCoseEncrypt(const bytevec& key, const cppbor::Item* coseEncrypt,
                                     const bytevec& external_aad) {
    if (!coseEncrypt || !coseEncrypt->asArray() ||
        coseEncrypt->asArray()->size() != kCoseEncryptEntryCount) {
        return "Invalid COSE_Encrypt";
    }

    auto& protParms = coseEncrypt->asArray()->get(kCoseEncryptProtectedParams);
    auto& unprotParms = coseEncrypt->asArray()->get(kCoseEncryptUnprotectedParams);
    auto& ciphertext = coseEncrypt->asArray()->get(kCoseEncryptPayload);
    auto& recipients = coseEncrypt->asArray()->get(kCoseEncryptRecipients);

    if (!protParms || !protParms->asBstr() || !unprotParms || !ciphertext || !recipients) {
        return "Invalid COSE_Encrypt";
    }

    auto [parsedProtParams, _, errMsg] = cppbor::parse(protParms->asBstr()->value());
    if (!parsedProtParams) {
        return errMsg + " when parsing protected params.";
    }
    if (!parsedProtParams->asMap()) {
        return "Protected params must be a map";
    }

    auto& algorithm = parsedProtParams->asMap()->get(ALGORITHM);
    if (!algorithm || !algorithm->asInt() || algorithm->asInt()->value() != AES_GCM_256) {
        return "Unsupported encryption algorithm";
    }

    if (!unprotParms->asMap() || unprotParms->asMap()->size() != 1) {
        return "Invalid unprotected params";
    }

    auto& nonce = unprotParms->asMap()->get(IV);
    if (!nonce || !nonce->asBstr() || nonce->asBstr()->value().size() != kAesGcmNonceLength) {
        return "Invalid nonce";
    }

    if (!ciphertext->asBstr()) return "Invalid ciphertext";

    auto aad = cppbor::Array()                             // Enc strucure as AAD
                       .add("Encrypt")                     // Context
                       .add(protParms->asBstr()->value())  // Protected
                       .add(external_aad)                  // External AAD
                       .encode();

    return aesGcmDecrypt(key, nonce->asBstr()->value(), aad, ciphertext->asBstr()->value());
}

ErrMsgOr<bytevec> x25519_HKDF_DeriveKey(const bytevec& pubKeyA, const bytevec& privKeyA,
                                        const bytevec& pubKeyB, bool senderIsA) {
    bytevec rawSharedKey(X25519_SHARED_KEY_LEN);
    if (!::X25519(rawSharedKey.data(), privKeyA.data(), pubKeyB.data())) {
        return "ECDH operation failed";
    }

    bytevec kdfContext = cppbor::Array()
                                 .add(AES_GCM_256)
                                 .add(cppbor::Array()  // Sender Info
                                              .add(cppbor::Bstr("client"))
                                              .add(bytevec{} /* nonce */)
                                              .add(senderIsA ? pubKeyA : pubKeyB))
                                 .add(cppbor::Array()  // Recipient Info
                                              .add(cppbor::Bstr("server"))
                                              .add(bytevec{} /* nonce */)
                                              .add(senderIsA ? pubKeyB : pubKeyA))
                                 .add(cppbor::Array()           // SuppPubInfo
                                              .add(128)         // output key length
                                              .add(bytevec{}))  // protected
                                 .encode();

    bytevec retval(SHA256_DIGEST_LENGTH);
    bytevec salt{};
    if (!HKDF(retval.data(), retval.size(),              //
              EVP_sha256(),                              //
              rawSharedKey.data(), rawSharedKey.size(),  //
              salt.data(), salt.size(),                  //
              kdfContext.data(), kdfContext.size())) {
        return "ECDH HKDF failed";
    }

    return retval;
}

ErrMsgOr<bytevec> aesGcmEncrypt(const bytevec& key, const bytevec& nonce, const bytevec& aad,
                                const bytevec& plaintext) {
    auto ctx = aesGcmInitAndProcessAad(key, nonce, aad, true /* encrypt */);
    if (!ctx) return ctx.moveMessage();

    bytevec ciphertext(plaintext.size() + kAesGcmTagSize);
    int outlen;
    if (!EVP_CipherUpdate(ctx->get(), ciphertext.data(), &outlen, plaintext.data(),
                          plaintext.size())) {
        return "Failed to encrypt plaintext";
    }
    assert(plaintext.size() == outlen);

    if (!EVP_CipherFinal_ex(ctx->get(), ciphertext.data() + outlen, &outlen)) {
        return "Failed to finalize encryption";
    }
    assert(outlen == 0);

    if (!EVP_CIPHER_CTX_ctrl(ctx->get(), EVP_CTRL_GCM_GET_TAG, kAesGcmTagSize,
                             ciphertext.data() + plaintext.size())) {
        return "Failed to retrieve tag";
    }

    return ciphertext;
}

ErrMsgOr<bytevec> aesGcmDecrypt(const bytevec& key, const bytevec& nonce, const bytevec& aad,
                                const bytevec& ciphertextWithTag) {
    auto ctx = aesGcmInitAndProcessAad(key, nonce, aad, false /* encrypt */);
    if (!ctx) return ctx.moveMessage();

    if (ciphertextWithTag.size() < kAesGcmTagSize) return "Missing tag";

    bytevec plaintext(ciphertextWithTag.size() - kAesGcmTagSize);
    int outlen;
    if (!EVP_CipherUpdate(ctx->get(), plaintext.data(), &outlen, ciphertextWithTag.data(),
                          ciphertextWithTag.size() - kAesGcmTagSize)) {
        return "Failed to decrypt plaintext";
    }
    assert(plaintext.size() == outlen);

    bytevec tag(ciphertextWithTag.end() - kAesGcmTagSize, ciphertextWithTag.end());
    if (!EVP_CIPHER_CTX_ctrl(ctx->get(), EVP_CTRL_GCM_SET_TAG, kAesGcmTagSize, tag.data())) {
        return "Failed to set tag: " + std::to_string(ERR_peek_last_error());
    }

    if (!EVP_CipherFinal_ex(ctx->get(), nullptr, &outlen)) {
        return "Failed to finalize encryption";
    }
    assert(outlen == 0);

    return plaintext;
}

}  // namespace cppcose
