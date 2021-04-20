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

#pragma once

#include <AndroidKeyMintDevice.h>
#include <aidl/android/hardware/security/keymint/BnRemotelyProvisionedComponent.h>
#include <aidl/android/hardware/security/keymint/SecurityLevel.h>
#include <cppbor.h>
#include <keymaster/UniquePtr.h>
#include <keymaster/android_keymaster.h>

namespace aidl::android::hardware::security::keymint {

using ::ndk::ScopedAStatus;

class RemotelyProvisionedComponent : public BnRemotelyProvisionedComponent {
  public:
    explicit RemotelyProvisionedComponent(std::shared_ptr<keymint::AndroidKeyMintDevice> keymint);
    virtual ~RemotelyProvisionedComponent();

    ScopedAStatus generateEcdsaP256KeyPair(bool testMode, MacedPublicKey* macedPublicKey,
                                           std::vector<uint8_t>* privateKeyHandle) override;

    ScopedAStatus generateCertificateRequest(bool testMode,
                                             const std::vector<MacedPublicKey>& keysToSign,
                                             const std::vector<uint8_t>& endpointEncCertChain,
                                             const std::vector<uint8_t>& challenge,
                                             DeviceInfo* deviceInfo, ProtectedData* protectedData,
                                             std::vector<uint8_t>* keysToSignMac) override;

  private:
    std::shared_ptr<::keymaster::AndroidKeymaster> impl_;
};

}  // namespace aidl::android::hardware::security::keymint
