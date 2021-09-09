/*
 * Copyright (C) 2021 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#include <fstream>
#include "keymaster4_common.h"

namespace android::hardware::keymaster::V4_0::fuzzer {

constexpr size_t kMaxVectorSize = 100;
constexpr size_t kMaxKeyParameter = 10;

constexpr Tag kTagArray[] = {Tag::INVALID,
                             Tag::PURPOSE,
                             Tag::ALGORITHM,
                             Tag::KEY_SIZE,
                             Tag::BLOCK_MODE,
                             Tag::DIGEST,
                             Tag::PADDING,
                             Tag::CALLER_NONCE,
                             Tag::MIN_MAC_LENGTH,
                             Tag::EC_CURVE,
                             Tag::RSA_PUBLIC_EXPONENT,
                             Tag::INCLUDE_UNIQUE_ID,
                             Tag::BLOB_USAGE_REQUIREMENTS,
                             Tag::BOOTLOADER_ONLY,
                             Tag::ROLLBACK_RESISTANCE,
                             Tag::HARDWARE_TYPE,
                             Tag::ACTIVE_DATETIME,
                             Tag::ORIGINATION_EXPIRE_DATETIME,
                             Tag::USAGE_EXPIRE_DATETIME,
                             Tag::MIN_SECONDS_BETWEEN_OPS,
                             Tag::MAX_USES_PER_BOOT,
                             Tag::USER_ID,
                             Tag::USER_SECURE_ID,
                             Tag::NO_AUTH_REQUIRED,
                             Tag::USER_AUTH_TYPE,
                             Tag::AUTH_TIMEOUT,
                             Tag::ALLOW_WHILE_ON_BODY,
                             Tag::TRUSTED_USER_PRESENCE_REQUIRED,
                             Tag::TRUSTED_CONFIRMATION_REQUIRED,
                             Tag::UNLOCKED_DEVICE_REQUIRED,
                             Tag::APPLICATION_ID,
                             Tag::APPLICATION_DATA,
                             Tag::CREATION_DATETIME,
                             Tag::ORIGIN,
                             Tag::ROOT_OF_TRUST,
                             Tag::OS_VERSION,
                             Tag::OS_PATCHLEVEL,
                             Tag::UNIQUE_ID,
                             Tag::ATTESTATION_CHALLENGE,
                             Tag::ATTESTATION_APPLICATION_ID,
                             Tag::ATTESTATION_ID_BRAND,
                             Tag::ATTESTATION_ID_DEVICE,
                             Tag::ATTESTATION_ID_PRODUCT,
                             Tag::ATTESTATION_ID_SERIAL,
                             Tag::ATTESTATION_ID_IMEI,
                             Tag::ATTESTATION_ID_MEID,
                             Tag::ATTESTATION_ID_MANUFACTURER,
                             Tag::ATTESTATION_ID_MODEL,
                             Tag::VENDOR_PATCHLEVEL,
                             Tag::BOOT_PATCHLEVEL,
                             Tag::ASSOCIATED_DATA,
                             Tag::NONCE,
                             Tag::MAC_LENGTH,
                             Tag::RESET_SINCE_ID_ROTATION,
                             Tag::CONFIRMATION_TOKEN};

class KeyMaster4AuthSetFuzzer {
  public:
    void process(const uint8_t* data, size_t size);

  private:
    void invokeAuthSetAPIs();
    std::unique_ptr<FuzzedDataProvider> mFdp = nullptr;
};

/**
 * @brief invokeAuthSetAPIs() function aims at calling functions of authorization_set.cpp
 * and authorization_set.h in order to get a good coverage for libkeymaster4support.
 */
void KeyMaster4AuthSetFuzzer::invokeAuthSetAPIs() {
    AuthorizationSet authSet = createAuthorizationSet(mFdp);
    while (mFdp->remaining_bytes() > 0) {
        uint32_t action = mFdp->ConsumeIntegralInRange<uint32_t>(0, 15);
        switch (action) {
            case 0: {
                authSet.Sort();
            } break;
            case 1: {
                authSet.Deduplicate();
            } break;
            case 2: {
                authSet.Union(createAuthorizationSet(mFdp));
            } break;
            case 3: {
                authSet.Subtract(createAuthorizationSet(mFdp));
            } break;
            case 4: {
                std::filebuf fbOut;
                fbOut.open("/dev/zero", std::ios::out);
                std::ostream out(&fbOut);
                authSet.Serialize(&out);
            } break;
            case 5: {
                std::filebuf fbIn;
                fbIn.open("/dev/zero", std::ios::in);
                std::istream in(&fbIn);
                authSet.Deserialize(&in);
            } break;
            case 6: {  // invoke push_back()
                AuthorizationSetBuilder builder = AuthorizationSetBuilder();
                for (const KeyParameter& tag : authSet) {
                    builder.push_back(tag);
                }
                AuthorizationSet params = createAuthorizationSet(mFdp);
                authSet.push_back(params);
            } break;
            case 7: {  // invoke copy constructor
                auto params = AuthorizationSetBuilder().Authorizations(authSet);
                authSet = params;
            } break;
            case 8: {  // invoke move constructor
                auto params = AuthorizationSetBuilder().Authorizations(authSet);
                authSet = std::move(params);
            } break;
            case 9: {  // invoke Constructor from hidl_vec<KeyParameter>
                hidl_vec<KeyParameter> keyParam;
                size_t numKeyParam = mFdp->ConsumeIntegralInRange<size_t>(1, kMaxKeyParameter);
                keyParam.resize(numKeyParam);
                for (size_t i = 0; i < numKeyParam - 1; ++i) {
                    keyParam[i].tag = mFdp->PickValueInArray(kTagArray);
                    std::vector<uint8_t> dataVector = mFdp->ConsumeBytes<uint8_t>(
                            mFdp->ConsumeIntegralInRange<size_t>(0, kMaxVectorSize));
                    keyParam[i].blob = dataVector;
                }
                if (mFdp->ConsumeBool()) {
                    AuthorizationSet auths(keyParam);
                    auths.push_back(AuthorizationSet(keyParam));
                } else {  // invoke operator=
                    AuthorizationSet auths = keyParam;
                }
            } break;
            case 10: {  // invoke 'Contains()'
                Tag tag;
                if (authSet.size() > 0) {
                    tag = authSet[mFdp->ConsumeIntegralInRange<size_t>(0, authSet.size() - 1)].tag;
                }
                authSet.Contains(mFdp->ConsumeBool() ? tag : mFdp->PickValueInArray(kTagArray));
            } break;
            case 11: {  // invoke 'GetTagCount()'
                Tag tag;
                if (authSet.size() > 0) {
                    tag = authSet[mFdp->ConsumeIntegralInRange<size_t>(0, authSet.size() - 1)].tag;
                }
                authSet.GetTagCount(mFdp->ConsumeBool() ? tag : mFdp->PickValueInArray(kTagArray));
            } break;
            case 12: {  // invoke 'empty()'
                authSet.empty();
            } break;
            case 13: {  // invoke 'data()'
                authSet.data();
            } break;
            case 14: {  // invoke 'hidl_data()'
                authSet.hidl_data();
            } break;
            case 15: {  // invoke 'erase()'
                if (authSet.size() > 0) {
                    authSet.erase(mFdp->ConsumeIntegralInRange<size_t>(0, authSet.size() - 1));
                }
            } break;
            default:
                break;
        };
    }
    authSet.Clear();
}

void KeyMaster4AuthSetFuzzer::process(const uint8_t* data, size_t size) {
    mFdp = std::make_unique<FuzzedDataProvider>(data, size);
    invokeAuthSetAPIs();
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    KeyMaster4AuthSetFuzzer km4AuthSetFuzzer;
    km4AuthSetFuzzer.process(data, size);
    return 0;
}

}  // namespace android::hardware::keymaster::V4_0::fuzzer
