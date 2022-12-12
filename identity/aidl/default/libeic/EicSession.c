/*
 * Copyright 2020, The Android Open Source Project
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

#include <inttypes.h>

#include "EicCommon.h"
#include "EicSession.h"

// Global used for assigning ids for session objects.
//
static uint32_t gSessionLastIdAssigned = 0;

// The current session object or NULL if never initialized or if it has been shut down.
//
static EicSession* gSessionCurrent = NULL;

EicSession* eicSessionGetForId(uint32_t sessionId) {
    if (gSessionCurrent != NULL && gSessionCurrent->id == sessionId) {
        return gSessionCurrent;
    }
    return NULL;
}

bool eicSessionInit(EicSession* ctx) {
    eicMemSet(ctx, '\0', sizeof(EicSession));

    if (!eicNextId(&gSessionLastIdAssigned)) {
        eicDebug("Error getting id for object");
        return false;
    }
    ctx->id = gSessionLastIdAssigned;

    do {
        if (!eicOpsRandom((uint8_t*)&(ctx->authChallenge), sizeof(ctx->authChallenge))) {
            eicDebug("Failed generating random challenge");
            return false;
        }
    } while (ctx->authChallenge == EIC_KM_AUTH_CHALLENGE_UNSET);

    if (!eicOpsCreateEcKey(ctx->ephemeralPrivateKey, ctx->ephemeralPublicKey)) {
        eicDebug("Error creating ephemeral key-pair");
        return false;
    }

    gSessionCurrent = ctx;
    eicDebug("Initialized session with id %" PRIu32, ctx->id);
    return true;
}

bool eicSessionShutdown(EicSession* ctx) {
    if (ctx->id == 0) {
        eicDebug("Trying to shut down session with id 0");
        return false;
    }
    eicDebug("Shut down session with id %" PRIu32, ctx->id);
    eicMemSet(ctx, '\0', sizeof(EicSession));
    gSessionCurrent = NULL;
    return true;
}

bool eicSessionGetId(EicSession* ctx, uint32_t* outId) {
    *outId = ctx->id;
    return true;
}

bool eicSessionGetAuthChallenge(EicSession* ctx, uint64_t* outAuthChallenge) {
    *outAuthChallenge = ctx->authChallenge;
    return true;
}

bool eicSessionGetEphemeralKeyPair(EicSession* ctx,
                                   uint8_t ephemeralPrivateKey[EIC_P256_PRIV_KEY_SIZE]) {
    eicMemCpy(ephemeralPrivateKey, ctx->ephemeralPrivateKey, EIC_P256_PRIV_KEY_SIZE);
    ctx->getEphemeralKeyPairCalled = true;
    return true;
}

bool eicSessionSetReaderEphemeralPublicKey(
        EicSession* ctx, const uint8_t readerEphemeralPublicKey[EIC_P256_PUB_KEY_SIZE]) {
    eicMemCpy(ctx->readerEphemeralPublicKey, readerEphemeralPublicKey, EIC_P256_PUB_KEY_SIZE);
    ctx->readerEphemeralPublicKeySize = EIC_P256_PUB_KEY_SIZE;
    return true;
}

bool eicSessionSetSessionTranscript(EicSession* ctx, const uint8_t* sessionTranscript,
                                    size_t sessionTranscriptSize) {
    // If mdoc session encryption is in use, only accept the
    // SessionTranscript if X and Y from the ephemeral key we created
    // is somewhere in SessionTranscript...
    //
    if (ctx->getEphemeralKeyPairCalled) {
        if (eicMemMem(sessionTranscript, sessionTranscriptSize, ctx->ephemeralPublicKey,
                      EIC_P256_PUB_KEY_SIZE / 2) == NULL) {
            eicDebug("Error finding X from ephemeralPublicKey in sessionTranscript");
            return false;
        }
        if (eicMemMem(sessionTranscript, sessionTranscriptSize,
                      ctx->ephemeralPublicKey + EIC_P256_PUB_KEY_SIZE / 2,
                      EIC_P256_PUB_KEY_SIZE / 2) == NULL) {
            eicDebug("Error finding Y from ephemeralPublicKey in sessionTranscript");
            return false;
        }
    }

    // To save space we only store the SHA-256 of SessionTranscript
    //
    EicSha256Ctx shaCtx;
    eicOpsSha256Init(&shaCtx);
    eicOpsSha256Update(&shaCtx, sessionTranscript, sessionTranscriptSize);
    eicOpsSha256Final(&shaCtx, ctx->sessionTranscriptSha256);
    return true;
}
