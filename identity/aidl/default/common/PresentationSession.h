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

#ifndef ANDROID_HARDWARE_IDENTITY_PRESENTATIONSESSION_H
#define ANDROID_HARDWARE_IDENTITY_PRESENTATIONSESSION_H

#include <aidl/android/hardware/identity/BnPresentationSession.h>
#include <android/hardware/identity/support/IdentityCredentialSupport.h>

#include <vector>

#include <cppbor.h>

#include "IdentityCredentialStore.h"
#include "SecureHardwareProxy.h"

namespace aidl::android::hardware::identity {

using ::aidl::android::hardware::keymaster::HardwareAuthToken;
using ::aidl::android::hardware::keymaster::VerificationToken;
using ::android::sp;
using ::android::hardware::identity::SecureHardwareSessionProxy;
using ::std::vector;

class PresentationSession : public BnPresentationSession {
  public:
    PresentationSession(sp<SecureHardwareProxyFactory> hwProxyFactory,
                        sp<SecureHardwareSessionProxy> hwProxy,
                        HardwareInformation hardwareInformation)
        : hwProxyFactory_(std::move(hwProxyFactory)),
          hwProxy_(std::move(hwProxy)),
          hardwareInformation_(std::move(hardwareInformation)) {}

    virtual ~PresentationSession();

    // Creates ephemeral key and auth-challenge in TA. Returns a status code from
    // IIdentityCredentialStore. Must be called right after construction.
    int initialize();

    uint64_t getSessionId();

    vector<uint8_t> getSessionTranscript();
    vector<uint8_t> getReaderEphemeralPublicKey();

    // Methods from IPresentationSession follow.
    ndk::ScopedAStatus getEphemeralKeyPair(vector<uint8_t>* outKeyPair) override;
    ndk::ScopedAStatus getAuthChallenge(int64_t* outChallenge) override;
    ndk::ScopedAStatus setReaderEphemeralPublicKey(const vector<uint8_t>& publicKey) override;
    ndk::ScopedAStatus setSessionTranscript(const vector<uint8_t>& sessionTranscript) override;

    ndk::ScopedAStatus getCredential(const vector<uint8_t>& credentialData,
                                     shared_ptr<IIdentityCredential>* outCredential) override;

  private:
    // Set by constructor
    sp<SecureHardwareProxyFactory> hwProxyFactory_;
    sp<SecureHardwareSessionProxy> hwProxy_;
    HardwareInformation hardwareInformation_;

    // Set by initialize()
    uint64_t id_;
    uint64_t authChallenge_;

    // Set by getEphemeralKeyPair()
    vector<uint8_t> ephemeralKeyPair_;

    // Set by setReaderEphemeralPublicKey()
    vector<uint8_t> readerPublicKey_;

    // Set by setSessionTranscript()
    vector<uint8_t> sessionTranscript_;
};

}  // namespace aidl::android::hardware::identity

#endif  // ANDROID_HARDWARE_IDENTITY_PRESENTATIONSESSION_H
