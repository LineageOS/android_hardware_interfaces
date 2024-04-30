/*
 * Copyright (C) 2024 The Android Open Source Project
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
#include <keymint_common.h>
#include <fstream>

namespace android::hardware::security::keymint_support::fuzzer {

constexpr size_t kMinAction = 0;
constexpr size_t kMaxAction = 10;
constexpr size_t kMinKeyParameter = 1;
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
                             Tag::RSA_OAEP_MGF_DIGEST,
                             Tag::BOOTLOADER_ONLY,
                             Tag::ROLLBACK_RESISTANCE,
                             Tag::HARDWARE_TYPE,
                             Tag::EARLY_BOOT_ONLY,
                             Tag::ACTIVE_DATETIME,
                             Tag::ORIGINATION_EXPIRE_DATETIME,
                             Tag::USAGE_EXPIRE_DATETIME,
                             Tag::MIN_SECONDS_BETWEEN_OPS,
                             Tag::MAX_USES_PER_BOOT,
                             Tag::USAGE_COUNT_LIMIT,
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
                             Tag::DEVICE_UNIQUE_ATTESTATION,
                             Tag::IDENTITY_CREDENTIAL_KEY,
                             Tag::STORAGE_KEY,
                             Tag::ASSOCIATED_DATA,
                             Tag::NONCE,
                             Tag::MAC_LENGTH,
                             Tag::RESET_SINCE_ID_ROTATION,
                             Tag::CONFIRMATION_TOKEN,
                             Tag::CERTIFICATE_SERIAL,
                             Tag::CERTIFICATE_SUBJECT,
                             Tag::CERTIFICATE_NOT_BEFORE,
                             Tag::CERTIFICATE_NOT_AFTER,
                             Tag::MAX_BOOT_LEVEL};

class KeyMintAuthSetFuzzer {
  public:
    KeyMintAuthSetFuzzer(const uint8_t* data, size_t size) : mFdp(data, size){};
    void process();

  private:
    FuzzedDataProvider mFdp;
};

void KeyMintAuthSetFuzzer::process() {
    AuthorizationSet authSet = createAuthorizationSet(&mFdp);
    while (mFdp.remaining_bytes()) {
        auto invokeAuthSetAPI = mFdp.PickValueInArray<const std::function<void()>>({
                [&]() { authSet.Sort(); },
                [&]() { authSet.Deduplicate(); },
                [&]() { authSet.Union(createAuthorizationSet(&mFdp)); },
                [&]() { authSet.Subtract(createAuthorizationSet(&mFdp)); },
                [&]() {  // invoke push_back()
                    AuthorizationSetBuilder builder = AuthorizationSetBuilder();
                    for (const KeyParameter& tag : authSet) {
                        builder.push_back(tag);
                    }
                    AuthorizationSet params = createAuthorizationSet(&mFdp);
                    authSet.push_back(params);
                },
                [&]() {  // invoke copy constructor
                    auto params = AuthorizationSetBuilder().Authorizations(authSet);
                    authSet = params;
                },
                [&]() {  // invoke move constructor
                    auto params = AuthorizationSetBuilder().Authorizations(authSet);
                    authSet = std::move(params);
                },
                [&]() {  // invoke Constructor from vector<KeyParameter>
                    vector<KeyParameter> keyParam;
                    size_t numKeyParam =
                            mFdp.ConsumeIntegralInRange<size_t>(kMinKeyParameter, kMaxKeyParameter);
                    keyParam.resize(numKeyParam);
                    for (size_t idx = 0; idx < numKeyParam - 1; ++idx) {
                        keyParam[idx].tag = mFdp.PickValueInArray(kTagArray);
                    }
                    if (mFdp.ConsumeBool()) {
                        AuthorizationSet auths(keyParam);
                        auths.push_back(AuthorizationSet(keyParam));
                    } else {  // invoke operator=
                        AuthorizationSet auths = keyParam;
                    }
                },
                [&]() {  // invoke 'Contains()'
                    Tag tag = Tag::INVALID;
                    if (authSet.size() > 0) {
                        tag = authSet[mFdp.ConsumeIntegralInRange<size_t>(0, authSet.size() - 1)]
                                      .tag;
                    }
                    authSet.Contains(mFdp.ConsumeBool() ? tag : mFdp.PickValueInArray(kTagArray));
                },
                [&]() {  // invoke 'GetTagCount()'
                    Tag tag = Tag::INVALID;
                    if (authSet.size() > 0) {
                        tag = authSet[mFdp.ConsumeIntegralInRange<size_t>(0, authSet.size() - 1)]
                                      .tag;
                    }
                    authSet.GetTagCount(mFdp.ConsumeBool() ? tag
                                                           : mFdp.PickValueInArray(kTagArray));
                },
                [&]() {  // invoke 'erase()'
                    if (authSet.size() > 0) {
                        authSet.erase(mFdp.ConsumeIntegralInRange<size_t>(0, authSet.size() - 1));
                    }
                },
        });
        invokeAuthSetAPI();
    }
    authSet.Clear();
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    KeyMintAuthSetFuzzer kmAuthSetFuzzer(data, size);
    kmAuthSetFuzzer.process();
    return 0;
}

}  // namespace android::hardware::security::keymint_support::fuzzer
