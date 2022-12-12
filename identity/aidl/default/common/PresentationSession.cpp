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

#define LOG_TAG "PresentationSession"

#include "PresentationSession.h"
#include "IdentityCredentialStore.h"

#include <android/hardware/identity/support/IdentityCredentialSupport.h>

#include <string.h>

#include <android-base/logging.h>
#include <android-base/stringprintf.h>

#include <cppbor.h>
#include <cppbor_parse.h>

#include "FakeSecureHardwareProxy.h"
#include "IdentityCredential.h"
#include "PresentationSession.h"

namespace aidl::android::hardware::identity {

using ::std::optional;

using namespace ::android::hardware::identity;

PresentationSession::~PresentationSession() {}

int PresentationSession::initialize() {
    if (!hwProxy_->initialize()) {
        LOG(ERROR) << "hwProxy->initialize failed";
        return IIdentityCredentialStore::STATUS_FAILED;
    }

    optional<uint64_t> id = hwProxy_->getId();
    if (!id) {
        LOG(ERROR) << "Error getting id for session";
        return IIdentityCredentialStore::STATUS_FAILED;
    }
    id_ = id.value();

    optional<uint64_t> authChallenge = hwProxy_->getAuthChallenge();
    if (!authChallenge) {
        LOG(ERROR) << "Error getting authChallenge for session";
        return IIdentityCredentialStore::STATUS_FAILED;
    }
    authChallenge_ = authChallenge.value();

    return IIdentityCredentialStore::STATUS_OK;
}

ndk::ScopedAStatus PresentationSession::getEphemeralKeyPair(vector<uint8_t>* outKeyPair) {
    if (ephemeralKeyPair_.size() == 0) {
        optional<vector<uint8_t>> ephemeralKeyPriv = hwProxy_->getEphemeralKeyPair();
        if (!ephemeralKeyPriv) {
            LOG(ERROR) << "Error getting ephemeral private key for session";
            return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                    IIdentityCredentialStore::STATUS_FAILED,
                    "Error getting ephemeral private key for session"));
        }
        optional<vector<uint8_t>> ephemeralKeyPair =
                support::ecPrivateKeyToKeyPair(ephemeralKeyPriv.value());
        if (!ephemeralKeyPair) {
            LOG(ERROR) << "Error creating ephemeral key-pair";
            return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                    IIdentityCredentialStore::STATUS_FAILED, "Error creating ephemeral key-pair"));
        }
        ephemeralKeyPair_ = ephemeralKeyPair.value();
    }
    *outKeyPair = ephemeralKeyPair_;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus PresentationSession::getAuthChallenge(int64_t* outChallenge) {
    *outChallenge = authChallenge_;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus PresentationSession::setReaderEphemeralPublicKey(
        const vector<uint8_t>& publicKey) {
    // We expect the reader ephemeral public key to be same size and curve
    // as the ephemeral key we generated (e.g. P-256 key), otherwise ECDH
    // won't work. So its length should be 65 bytes and it should be
    // starting with 0x04.
    if (publicKey.size() != 65 || publicKey[0] != 0x04) {
        return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                IIdentityCredentialStore::STATUS_FAILED,
                "Reader public key is not in expected format"));
    }
    readerPublicKey_ = publicKey;
    vector<uint8_t> pubKeyP256(publicKey.begin() + 1, publicKey.end());
    if (!hwProxy_->setReaderEphemeralPublicKey(pubKeyP256)) {
        return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                IIdentityCredentialStore::STATUS_FAILED,
                "Error setting readerEphemeralPublicKey for session"));
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus PresentationSession::setSessionTranscript(
        const vector<uint8_t>& sessionTranscript) {
    sessionTranscript_ = sessionTranscript;
    if (!hwProxy_->setSessionTranscript(sessionTranscript)) {
        return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                IIdentityCredentialStore::STATUS_FAILED,
                "Error setting SessionTranscript for session"));
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus PresentationSession::getCredential(
        const vector<uint8_t>& credentialData, shared_ptr<IIdentityCredential>* outCredential) {
    shared_ptr<PresentationSession> p = ref<PresentationSession>();
    shared_ptr<IdentityCredential> credential = ndk::SharedRefBase::make<IdentityCredential>(
            hwProxyFactory_, credentialData, p, hardwareInformation_);
    int ret = credential->initialize();
    if (ret != IIdentityCredentialStore::STATUS_OK) {
        return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                ret, "Error initializing IdentityCredential"));
    }
    *outCredential = std::move(credential);

    return ndk::ScopedAStatus::ok();
}

uint64_t PresentationSession::getSessionId() {
    return id_;
}

vector<uint8_t> PresentationSession::getSessionTranscript() {
    return sessionTranscript_;
}

vector<uint8_t> PresentationSession::getReaderEphemeralPublicKey() {
    return readerPublicKey_;
}

}  // namespace aidl::android::hardware::identity
