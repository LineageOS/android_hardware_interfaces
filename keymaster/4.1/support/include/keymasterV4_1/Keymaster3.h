/*
 ** Copyright 2017, The Android Open Source Project
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

#include <android/hardware/keymaster/3.0/IKeymasterDevice.h>

#include "Keymaster.h"
#include "Operation.h"

namespace android::hardware::keymaster::V4_1::support {

using IKeymaster3Device = ::android::hardware::keymaster::V3_0::IKeymasterDevice;

using ::android::sp;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::details::return_status;

class Keymaster3 : public Keymaster {
  public:
    // This definition is used for device enumeration.
    using WrappedIKeymasterDevice = IKeymaster3Device;

    Keymaster3(sp<IKeymaster3Device> km3_dev, const hidl_string& instanceName)
        : Keymaster(IKeymaster3Device::descriptor, instanceName),
          km3_dev_(km3_dev),
          haveVersion_(false) {}

    const VersionResult& halVersion() const override {
        const_cast<Keymaster3*>(this)->getVersionIfNeeded();
        return version_;
    }

    Return<void> getHardwareInfo(getHardwareInfo_cb _hidl_cb);

    Return<void> getHmacSharingParameters(getHmacSharingParameters_cb _hidl_cb) override {
        _hidl_cb(V4_0::ErrorCode::UNIMPLEMENTED, {});
        return Void();
    }

    Return<void> computeSharedHmac(const hidl_vec<HmacSharingParameters>&,
                                   computeSharedHmac_cb _hidl_cb) override {
        _hidl_cb(V4_0::ErrorCode::UNIMPLEMENTED, {});
        return Void();
    }

    Return<void> verifyAuthorization(uint64_t, const hidl_vec<KeyParameter>&,
                                     const HardwareAuthToken&,
                                     verifyAuthorization_cb _hidl_cb) override {
        _hidl_cb(V4_0::ErrorCode::UNIMPLEMENTED, {});
        return Void();
    }

    Return<V4_0::ErrorCode> addRngEntropy(const hidl_vec<uint8_t>& data) override;
    Return<void> generateKey(const hidl_vec<KeyParameter>& keyParams,
                             generateKey_cb _hidl_cb) override;
    Return<void> getKeyCharacteristics(const hidl_vec<uint8_t>& keyBlob,
                                       const hidl_vec<uint8_t>& clientId,
                                       const hidl_vec<uint8_t>& appData,
                                       getKeyCharacteristics_cb _hidl_cb) override;
    Return<void> importKey(const hidl_vec<KeyParameter>& params, KeyFormat keyFormat,
                           const hidl_vec<uint8_t>& keyData, importKey_cb _hidl_cb) override;

    Return<void> importWrappedKey(const hidl_vec<uint8_t>& /* wrappedKeyData */,
                                  const hidl_vec<uint8_t>& /* wrappingKeyBlob */,
                                  const hidl_vec<uint8_t>& /* maskingKey */,
                                  const hidl_vec<KeyParameter>& /* unwrappingParams */,
                                  uint64_t /* passwordSid */, uint64_t /* biometricSid */,
                                  importWrappedKey_cb _hidl_cb) {
        _hidl_cb(V4_0::ErrorCode::UNIMPLEMENTED, {}, {});
        return Void();
    }

    Return<void> exportKey(KeyFormat exportFormat, const hidl_vec<uint8_t>& keyBlob,
                           const hidl_vec<uint8_t>& clientId, const hidl_vec<uint8_t>& appData,
                           exportKey_cb _hidl_cb) override;
    Return<void> attestKey(const hidl_vec<uint8_t>& keyToAttest,
                           const hidl_vec<KeyParameter>& attestParams,
                           attestKey_cb _hidl_cb) override;
    Return<void> upgradeKey(const hidl_vec<uint8_t>& keyBlobToUpgrade,
                            const hidl_vec<KeyParameter>& upgradeParams,
                            upgradeKey_cb _hidl_cb) override;
    Return<V4_0::ErrorCode> deleteKey(const hidl_vec<uint8_t>& keyBlob) override;
    Return<V4_0::ErrorCode> deleteAllKeys() override;
    Return<V4_0::ErrorCode> destroyAttestationIds() override;
    Return<void> begin(KeyPurpose purpose, const hidl_vec<uint8_t>& key,
                       const hidl_vec<KeyParameter>& inParams, const HardwareAuthToken& authToken,
                       begin_cb _hidl_cb) override;
    Return<void> update(uint64_t operationHandle, const hidl_vec<KeyParameter>& inParams,
                        const hidl_vec<uint8_t>& input, const HardwareAuthToken& authToken,
                        const VerificationToken& verificationToken, update_cb _hidl_cb) override;
    Return<void> finish(uint64_t operationHandle, const hidl_vec<KeyParameter>& inParams,
                        const hidl_vec<uint8_t>& input, const hidl_vec<uint8_t>& signature,
                        const HardwareAuthToken& authToken,
                        const VerificationToken& verificationToken, finish_cb _hidl_cb) override;
    Return<V4_0::ErrorCode> abort(uint64_t operationHandle) override;

    /**********************************
     * V4_1::IKeymasterDevice methods *
     *********************************/

    Return<ErrorCode> deviceLocked(bool /* passwordOnly */,
                                   const VerificationToken& /* verificationToken */) override {
        return ErrorCode::UNIMPLEMENTED;
    }

    Return<ErrorCode> earlyBootEnded() override { return ErrorCode::UNIMPLEMENTED; }

    Return<void> beginOp(KeyPurpose purpose, const hidl_vec<uint8_t>& keyBlob,
                         const hidl_vec<KeyParameter>& inParams, const HardwareAuthToken& authToken,
                         beginOp_cb _hidl_cb) override {
        return begin(purpose, keyBlob, inParams, authToken,
                     [&_hidl_cb](V4_0::ErrorCode errorCode, const hidl_vec<KeyParameter>& outParams,
                                 OperationHandle operationHandle) {
                         _hidl_cb(static_cast<ErrorCode>(errorCode), outParams,
                                  new Operation(operationHandle));
                     });
    }

  private:
    void getVersionIfNeeded();

    sp<IKeymaster3Device> km3_dev_;

    bool haveVersion_;
    VersionResult version_;
    bool supportsSymmetricCryptography_;
    bool supportsAttestation_;
    bool supportsAllDigests_;
};

}  // namespace android::hardware::keymaster::V4_1::support
