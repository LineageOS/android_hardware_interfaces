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
#include <android/binder_manager.h>
#include <fuzzer/FuzzedDataProvider.h>
#include <remote_prov/remote_prov_utils.h>
#include <utils/Log.h>

namespace android::hardware::security::keymint_support::fuzzer {

using namespace cppcose;
using namespace aidl::android::hardware::security::keymint;
using namespace aidl::android::hardware::security::keymint::remote_prov;

constexpr size_t kMinSize = 0;
constexpr size_t kSupportedNumKeys = 4;
constexpr size_t kChallengeSize = 64;
constexpr size_t kMaxBytes = 128;
const std::string kServiceName =
        "android.hardware.security.keymint.IRemotelyProvisionedComponent/default";

std::shared_ptr<IRemotelyProvisionedComponent> gRPC = nullptr;

class KeyMintRemoteProv {
  public:
    KeyMintRemoteProv(const uint8_t* data, size_t size) : mFdp(data, size){};
    void process();

  private:
    std::vector<uint8_t> ExtractPayloadValue(const MacedPublicKey& macedPubKey);
    FuzzedDataProvider mFdp;
};

std::vector<uint8_t> KeyMintRemoteProv::ExtractPayloadValue(const MacedPublicKey& macedPubKey) {
    std::vector<uint8_t> payloadValue;

    auto [coseMac0, _, mac0ParseErr] = cppbor::parse(macedPubKey.macedKey);
    if (coseMac0) {
        // The payload is a bstr holding an encoded COSE_Key
        auto payload = coseMac0->asArray()->get(kCoseMac0Payload)->asBstr();
        if (payload != nullptr) {
            payloadValue = payload->value();
        }
    }
    return payloadValue;
}

void KeyMintRemoteProv::process() {
    std::vector<MacedPublicKey> keysToSign = std::vector<MacedPublicKey>(
            mFdp.ConsumeIntegralInRange<uint8_t>(kMinSize, kSupportedNumKeys));
    cppbor::Array cborKeysToSign;
    for (auto& key : keysToSign) {
        // TODO: b/350649166 - Randomize keysToSign
        std::vector<uint8_t> privateKeyBlob;
        gRPC->generateEcdsaP256KeyPair(false /* testMode */, &key, &privateKeyBlob);

        std::vector<uint8_t> payloadValue = ExtractPayloadValue(key);
        cborKeysToSign.add(cppbor::EncodedItem(payloadValue));
    }

    uint8_t challengeSize = mFdp.ConsumeIntegralInRange<uint8_t>(kMinSize, kChallengeSize);
    std::vector<uint8_t> challenge = mFdp.ConsumeBytes<uint8_t>(challengeSize);

    RpcHardwareInfo rpcHardwareInfo;
    gRPC->getHardwareInfo(&rpcHardwareInfo);

    std::vector<uint8_t> csr;
    gRPC->generateCertificateRequestV2(keysToSign, challenge, &csr);

    while (mFdp.remaining_bytes()) {
        auto invokeProvAPI = mFdp.PickValueInArray<const std::function<void()>>({
                [&]() {
                    verifyFactoryCsr(cborKeysToSign, csr, rpcHardwareInfo, kServiceName, challenge);
                },
                [&]() {
                    verifyProductionCsr(cborKeysToSign, csr, rpcHardwareInfo, kServiceName,
                                        challenge);
                },
                [&]() { isCsrWithProperDiceChain(csr, kServiceName); },
        });
        invokeProvAPI();
    }
}

extern "C" int LLVMFuzzerInitialize(int /* *argc */, char /* ***argv */) {
    ::ndk::SpAIBinder binder(AServiceManager_waitForService(kServiceName.c_str()));
    gRPC = IRemotelyProvisionedComponent::fromBinder(binder);
    LOG_ALWAYS_FATAL_IF(!gRPC, "Failed to get IRemotelyProvisionedComponent instance.");
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    KeyMintRemoteProv kmRemoteProv(data, size);
    kmRemoteProv.process();
    return 0;
}

}  // namespace android::hardware::security::keymint_support::fuzzer
