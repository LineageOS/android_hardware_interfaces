/*
 * Copyright 2021, The Android Open Source Project
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

#if !defined(EIC_INSIDE_LIBEIC_H) && !defined(EIC_COMPILATION)
#error "Never include this file directly, include libeic.h instead."
#endif

#ifndef ANDROID_HARDWARE_IDENTITY_EIC_SESSION_H
#define ANDROID_HARDWARE_IDENTITY_EIC_SESSION_H

#include "EicOps.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    // A non-zero number unique for this EicSession instance
    uint32_t id;

    // Set to true iff eicSessionGetEphemeralKeyPair() has been called.
    bool getEphemeralKeyPairCalled;

    // The challenge generated at construction time by eicSessionInit().
    uint64_t authChallenge;

    uint8_t ephemeralPrivateKey[EIC_P256_PRIV_KEY_SIZE];
    uint8_t ephemeralPublicKey[EIC_P256_PUB_KEY_SIZE];

    uint8_t readerEphemeralPublicKey[EIC_P256_PUB_KEY_SIZE];

    uint8_t sessionTranscriptSha256[EIC_SHA256_DIGEST_SIZE];

    size_t readerEphemeralPublicKeySize;
} EicSession;

bool eicSessionInit(EicSession* ctx);

bool eicSessionShutdown(EicSession* ctx);

bool eicSessionGetId(EicSession* ctx, uint32_t* outId);

bool eicSessionGetAuthChallenge(EicSession* ctx, uint64_t* outAuthChallenge);

bool eicSessionGetEphemeralKeyPair(EicSession* ctx,
                                   uint8_t ephemeralPrivateKey[EIC_P256_PRIV_KEY_SIZE]);

bool eicSessionSetReaderEphemeralPublicKey(
        EicSession* ctx, const uint8_t readerEphemeralPublicKey[EIC_P256_PUB_KEY_SIZE]);

bool eicSessionSetSessionTranscript(EicSession* ctx, const uint8_t* sessionTranscript,
                                    size_t sessionTranscriptSize);

// Looks up an active session with the given id.
//
// Returns NULL if no active session with the given id is found.
//
EicSession* eicSessionGetForId(uint32_t sessionId);

#ifdef __cplusplus
}
#endif

#endif  // ANDROID_HARDWARE_IDENTITY_EIC_PRESENTATION_H
