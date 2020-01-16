/*
 * Copyright (C) 2019 The Android Open Source Project
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

#ifndef DRM_HAL_COMMON_H
#define DRM_HAL_COMMON_H

#include <android/hardware/drm/1.2/ICryptoFactory.h>
#include <android/hardware/drm/1.2/ICryptoPlugin.h>
#include <android/hardware/drm/1.2/IDrmFactory.h>
#include <android/hardware/drm/1.2/IDrmPlugin.h>
#include <android/hardware/drm/1.2/IDrmPluginListener.h>
#include <android/hardware/drm/1.2/types.h>

#include <chrono>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "drm_hal_vendor_module_api.h"
#include "vendor_modules.h"
#include "VtsHalHidlTargetCallbackBase.h"

using ::android::hardware::drm::V1_0::EventType;
using ::android::hardware::drm::V1_0::KeyedVector;
using KeyStatusV1_0 = ::android::hardware::drm::V1_0::KeyStatus;
using ::android::hardware::drm::V1_0::KeyType;
using ::android::hardware::drm::V1_0::Mode;
using ::android::hardware::drm::V1_0::Pattern;
using ::android::hardware::drm::V1_0::SessionId;
using ::android::hardware::drm::V1_0::SubSample;

using ::android::hardware::drm::V1_1::ICryptoFactory;

using StatusV1_2 = ::android::hardware::drm::V1_2::Status;

using ::android::hardware::hidl_array;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;

using ::android::hidl::memory::V1_0::IMemory;
using ::android::sp;

using std::map;
using std::string;
using std::unique_ptr;
using std::vector;

#define EXPECT_OK(ret) EXPECT_TRUE(ret.isOk())

#define RETURN_IF_SKIPPED                                                                \
    if (!vendorModule->isInstalled()) {                                                  \
        GTEST_SKIP() << "This drm scheme not supported."                                 \
                     << " library:" << GetParam()                                        \
                     << " service-name:" << vendorModule->getServiceName() << std::endl; \
        return;                                                                          \
    }

namespace android {
namespace hardware {
namespace drm {
namespace V1_2 {
namespace vts {

class DrmHalTest : public ::testing::TestWithParam<std::string> {
   public:
    static drm_vts::VendorModules* gVendorModules;
    DrmHalTest();
    virtual void SetUp() override;
    virtual void TearDown() override {}

   protected:
    hidl_array<uint8_t, 16> getVendorUUID();
    SessionId openSession();
    void closeSession(const SessionId& sessionId);
    hidl_vec<uint8_t> loadKeys(const SessionId& sessionId,
                               const KeyType& type = KeyType::STREAMING);
    hidl_vec<uint8_t> loadKeys(const SessionId& sessionId,
                               const DrmHalVTSVendorModule_V1::ContentConfiguration&,
                               const KeyType& type = KeyType::STREAMING);
    hidl_vec<uint8_t> getKeyRequest(const SessionId& sessionId,
                               const DrmHalVTSVendorModule_V1::ContentConfiguration&,
                               const KeyType& type);
    hidl_vec<uint8_t> provideKeyResponse(const SessionId& sessionId,
                               const hidl_vec<uint8_t>& keyResponse);
    DrmHalVTSVendorModule_V1::ContentConfiguration getContent(
            const KeyType& type = KeyType::STREAMING) const;

    KeyedVector toHidlKeyedVector(const map<string, string>& params);
    hidl_array<uint8_t, 16> toHidlArray(const vector<uint8_t>& vec);

    void fillRandom(const sp<IMemory>& memory);
    sp<IMemory> getDecryptMemory(size_t size, size_t index);
    uint32_t decrypt(Mode mode, bool isSecure,
            const hidl_array<uint8_t, 16>& keyId, uint8_t* iv,
            const hidl_vec<SubSample>& subSamples, const Pattern& pattern,
            const vector<uint8_t>& key, StatusV1_2 expectedStatus);
    void aes_ctr_decrypt(uint8_t* dest, uint8_t* src, uint8_t* iv,
            const hidl_vec<SubSample>& subSamples, const vector<uint8_t>& key);
    void aes_cbc_decrypt(uint8_t* dest, uint8_t* src, uint8_t* iv,
            const hidl_vec<SubSample>& subSamples, const vector<uint8_t>& key);

    sp<IDrmFactory> drmFactory;
    sp<ICryptoFactory> cryptoFactory;
    sp<IDrmPlugin> drmPlugin;
    sp<ICryptoPlugin> cryptoPlugin;
    unique_ptr<DrmHalVTSVendorModule_V1> vendorModule;
    vector<DrmHalVTSVendorModule_V1::ContentConfiguration> contentConfigurations;

  private:
    sp<IDrmPlugin> createDrmPlugin();
    sp<ICryptoPlugin> createCryptoPlugin();

};

class DrmHalClearkeyTest : public DrmHalTest {
   public:
     virtual void SetUp() override {
         DrmHalTest::SetUp();

         if (vendorModule == nullptr) {
             GTEST_SKIP() << "Instance not supported";
         }
     }
    virtual void TearDown() override {}
    void decryptWithInvalidKeys(hidl_vec<uint8_t>& invalidResponse,
            vector<uint8_t>& iv, const Pattern& noPattern, const vector<SubSample>& subSamples);
};

/**
 *  Event Handling tests
 */
extern const char *kCallbackLostState;
extern const char *kCallbackKeysChange;

struct ListenerEventArgs {
    SessionId sessionId;
    hidl_vec<KeyStatus> keyStatusList;
    bool hasNewUsableKey;
};

class DrmHalPluginListener
    : public ::testing::VtsHalHidlTargetCallbackBase<ListenerEventArgs>,
      public IDrmPluginListener {
public:
    DrmHalPluginListener() {
        SetWaitTimeoutDefault(std::chrono::milliseconds(500));
    }
    virtual ~DrmHalPluginListener() {}

    virtual Return<void> sendEvent(EventType, const hidl_vec<uint8_t>&,
            const hidl_vec<uint8_t>& ) override { return Void(); }

    virtual Return<void> sendExpirationUpdate(const hidl_vec<uint8_t>&,
            int64_t) override { return Void(); }

    virtual Return<void> sendKeysChange(const hidl_vec<uint8_t>&,
            const hidl_vec<KeyStatusV1_0>&, bool) override { return Void(); }

    virtual Return<void> sendSessionLostState(const hidl_vec<uint8_t>& sessionId) override;

    virtual Return<void> sendKeysChange_1_2(const hidl_vec<uint8_t>&,
            const hidl_vec<KeyStatus>&, bool) override;

};

}  // namespace vts
}  // namespace V1_2
}  // namespace drm
}  // namespace hardware
}  // namespace android

#endif  // DRM_HAL_COMMON_H
