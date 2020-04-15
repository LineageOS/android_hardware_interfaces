/*
 ** Copyright 2020, The Android Open Source Project
 **
 ** Licensed under the Apache License, Version 2.0 (the "License");
 ** you may not use this file except in compliance with the License.
 ** You may obtain a copy of the License at
 **
 **     http://www.apache.org/licenses/LICENSE-2.0
 **
 ** Unless required by applicable law or agreed to in writing, software
 ** distributed under the License is distributed on an "AS IS" BASIS,
 ** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 ** See the License for the specific language governing permissions and
 ** limitations under the License.
 */

#pragma once

#include "Keymaster.h"

namespace android::hardware::keymaster::V4_1::support {

using android::sp;

/**
 * This class can wrap either a V4_0 or V4_1 IKeymasterDevice.
 */
class Keymaster4 : public Keymaster {
  public:
    // This definition is used for device enumeration; enumerating 4.0 devices will also
    // enumerate 4.1. devices.
    using WrappedIKeymasterDevice = V4_0::IKeymasterDevice;

    Keymaster4(sp<V4_0::IKeymasterDevice> km4_0_dev, const hidl_string& instanceName)
        : Keymaster(V4_1::IKeymasterDevice::descriptor, instanceName),
          haveVersion_(false),
          km4_0_dev_(km4_0_dev),
          km4_1_dev_(V4_1::IKeymasterDevice::castFrom(km4_0_dev)) {}

    const VersionResult& halVersion() const override {
        const_cast<Keymaster4*>(this)->getVersionIfNeeded();
        return version_;
    }

    /**********************************
     * V4_0::IKeymasterDevice methods *
     *********************************/

    Return<void> getHardwareInfo(getHardwareInfo_cb _hidl_cb) override {
        return km4_0_dev_->getHardwareInfo(_hidl_cb);
    }

    Return<void> getHmacSharingParameters(getHmacSharingParameters_cb _hidl_cb) override {
        return km4_0_dev_->getHmacSharingParameters(_hidl_cb);
    }

    Return<void> computeSharedHmac(const hidl_vec<HmacSharingParameters>& params,
                                   computeSharedHmac_cb _hidl_cb) override {
        return km4_0_dev_->computeSharedHmac(params, _hidl_cb);
    }

    Return<void> verifyAuthorization(uint64_t operationHandle, const hidl_vec<KeyParameter>& params,
                                     const HardwareAuthToken& authToken,
                                     verifyAuthorization_cb _hidl_cb) override {
        return km4_0_dev_->verifyAuthorization(operationHandle, params, authToken, _hidl_cb);
    }

    Return<V4_0::ErrorCode> addRngEntropy(const hidl_vec<uint8_t>& data) override {
        return km4_0_dev_->addRngEntropy(data);
    }

    Return<void> generateKey(const hidl_vec<KeyParameter>& keyParams,
                             generateKey_cb _hidl_cb) override {
        return km4_0_dev_->generateKey(keyParams, _hidl_cb);
    }

    Return<void> getKeyCharacteristics(const hidl_vec<uint8_t>& keyBlob,
                                       const hidl_vec<uint8_t>& clientId,
                                       const hidl_vec<uint8_t>& appData,
                                       getKeyCharacteristics_cb _hidl_cb) override {
        return km4_0_dev_->getKeyCharacteristics(keyBlob, clientId, appData, _hidl_cb);
    }

    Return<void> importKey(const hidl_vec<KeyParameter>& params, KeyFormat keyFormat,
                           const hidl_vec<uint8_t>& keyData, importKey_cb _hidl_cb) override {
        return km4_0_dev_->importKey(params, keyFormat, keyData, _hidl_cb);
    }

    Return<void> importWrappedKey(const hidl_vec<uint8_t>& wrappedKeyData,
                                  const hidl_vec<uint8_t>& wrappingKeyBlob,
                                  const hidl_vec<uint8_t>& maskingKey,
                                  const hidl_vec<KeyParameter>& unwrappingParams,
                                  uint64_t passwordSid, uint64_t biometricSid,
                                  importWrappedKey_cb _hidl_cb) {
        return km4_0_dev_->importWrappedKey(wrappedKeyData, wrappingKeyBlob, maskingKey,
                                            unwrappingParams, passwordSid, biometricSid, _hidl_cb);
    }

    Return<void> exportKey(KeyFormat exportFormat, const hidl_vec<uint8_t>& keyBlob,
                           const hidl_vec<uint8_t>& clientId, const hidl_vec<uint8_t>& appData,
                           exportKey_cb _hidl_cb) override {
        return km4_0_dev_->exportKey(exportFormat, keyBlob, clientId, appData, _hidl_cb);
    }

    Return<void> attestKey(const hidl_vec<uint8_t>& keyToAttest,
                           const hidl_vec<KeyParameter>& attestParams,
                           attestKey_cb _hidl_cb) override {
        return km4_0_dev_->attestKey(keyToAttest, attestParams, _hidl_cb);
    }

    Return<void> upgradeKey(const hidl_vec<uint8_t>& keyBlobToUpgrade,
                            const hidl_vec<KeyParameter>& upgradeParams,
                            upgradeKey_cb _hidl_cb) override {
        return km4_0_dev_->upgradeKey(keyBlobToUpgrade, upgradeParams, _hidl_cb);
    }

    Return<V4_0::ErrorCode> deleteKey(const hidl_vec<uint8_t>& keyBlob) override {
        return km4_0_dev_->deleteKey(keyBlob);
    }

    Return<V4_0::ErrorCode> deleteAllKeys() override { return km4_0_dev_->deleteAllKeys(); }

    Return<V4_0::ErrorCode> destroyAttestationIds() override {
        return km4_0_dev_->destroyAttestationIds();
    }

    Return<void> begin(KeyPurpose purpose, const hidl_vec<uint8_t>& key,
                       const hidl_vec<KeyParameter>& inParams, const HardwareAuthToken& authToken,
                       begin_cb _hidl_cb) override {
        return km4_0_dev_->begin(purpose, key, inParams, authToken, _hidl_cb);
    }

    Return<void> update(uint64_t operationHandle, const hidl_vec<KeyParameter>& inParams,
                        const hidl_vec<uint8_t>& input, const HardwareAuthToken& authToken,
                        const VerificationToken& verificationToken, update_cb _hidl_cb) override {
        return km4_0_dev_->update(operationHandle, inParams, input, authToken, verificationToken,
                                  _hidl_cb);
    }

    Return<void> finish(uint64_t operationHandle, const hidl_vec<KeyParameter>& inParams,
                        const hidl_vec<uint8_t>& input, const hidl_vec<uint8_t>& signature,
                        const HardwareAuthToken& authToken,
                        const VerificationToken& verificationToken, finish_cb _hidl_cb) override {
        return km4_0_dev_->finish(operationHandle, inParams, input, signature, authToken,
                                  verificationToken, _hidl_cb);
    }

    Return<V4_0::ErrorCode> abort(uint64_t operationHandle) override {
        return km4_0_dev_->abort(operationHandle);
    }

    /**********************************
     * V4_1::IKeymasterDevice methods *
     *********************************/

    Return<ErrorCode> deviceLocked(bool passwordOnly,
                                   const VerificationToken& verificationToken) override {
        if (km4_1_dev_) return km4_1_dev_->deviceLocked(passwordOnly, verificationToken);
        return ErrorCode::UNIMPLEMENTED;
    }

    Return<ErrorCode> earlyBootEnded() override {
        if (km4_1_dev_) return km4_1_dev_->earlyBootEnded();
        return ErrorCode::UNIMPLEMENTED;
    }

  private:
    void getVersionIfNeeded();

    bool haveVersion_;
    VersionResult version_;
    sp<V4_0::IKeymasterDevice> km4_0_dev_;
    sp<V4_1::IKeymasterDevice> km4_1_dev_;
};  // namespace android::hardware::keymaster::V4_1::support

}  // namespace android::hardware::keymaster::V4_1::support
