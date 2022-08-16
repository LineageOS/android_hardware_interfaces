/*
 * Copyright (C) 2021 The Android Open Source Project
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

#include <aidl/android/hardware/common/Ashmem.h>
#include <aidl/android/hardware/drm/BnDrmPluginListener.h>
#include <aidl/android/hardware/drm/ICryptoPlugin.h>
#include <aidl/android/hardware/drm/IDrmFactory.h>
#include <aidl/android/hardware/drm/IDrmPlugin.h>
#include <aidl/android/hardware/drm/IDrmPluginListener.h>
#include <aidl/android/hardware/drm/Status.h>
#include <android/binder_auto_utils.h>

#include <gmock/gmock.h>

#include <array>
#include <algorithm>
#include <chrono>
#include <future>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "VtsHalHidlTargetCallbackBase.h"
#include "drm_hal_vendor_module_api.h"
#include "drm_vts_helper.h"
#include "vendor_modules.h"

using drm_vts::DrmHalTestParam;

namespace {
typedef vector<::aidl::android::hardware::drm::KeyValue> KeyedVector;
typedef std::vector<uint8_t> SessionId;
}  // namespace

#define EXPECT_OK(ret) EXPECT_TRUE(::aidl::android::hardware::drm::vts::IsOk(ret))
#define EXPECT_TXN(ret) EXPECT_TRUE(ret.isOk() || ret.getExceptionCode() == EX_SERVICE_SPECIFIC)

namespace aidl {
namespace android {
namespace hardware {
namespace drm {
namespace vts {

using ::testing::AnyOf;
using ::testing::Eq;

::aidl::android::hardware::drm::Status DrmErr(const ::ndk::ScopedAStatus& ret);
std::string HalBaseName(const std::string& fullname);
std::string HalFullName(const std::string& iface, const std::string& basename);
testing::AssertionResult IsOk(const ::ndk::ScopedAStatus& ret);

extern const char* kDrmIface;
extern const char* kCryptoIface;

class DrmHalTest : public ::testing::TestWithParam<DrmHalTestParam> {
  public:
    static drm_vts::VendorModules* gVendorModules;
    DrmHalTest();
    virtual void SetUp() override;
    virtual void TearDown() override {}

  protected:
    ::aidl::android::hardware::drm::Uuid getAidlUUID();
    std::vector<uint8_t> getUUID();
    std::vector<uint8_t> getVendorUUID();
    std::array<uint8_t, 16> GetParamUUID() { return GetParam().scheme_; }
    std::string GetParamService() { return GetParam().instance_; }
    ::aidl::android::hardware::drm::Uuid toAidlUuid(const std::vector<uint8_t>& in_uuid) {
        std::array<uint8_t, 16> a;
        std::copy_n(in_uuid.begin(), a.size(), a.begin());
        return {a};
    }

    bool isCryptoSchemeSupported(::aidl::android::hardware::drm::Uuid uuid,
                                 ::aidl::android::hardware::drm::SecurityLevel level,
                                 std::string mime);
    void provision();
    SessionId openSession(::aidl::android::hardware::drm::SecurityLevel level,
                          ::aidl::android::hardware::drm::Status* err);
    SessionId openSession();
    void closeSession(const SessionId& sessionId);
    std::vector<uint8_t> loadKeys(
            const SessionId& sessionId,
            const ::aidl::android::hardware::drm::KeyType& type = KeyType::STREAMING);
    std::vector<uint8_t> loadKeys(
            const SessionId& sessionId, const DrmHalVTSVendorModule_V1::ContentConfiguration&,
            const ::aidl::android::hardware::drm::KeyType& type = KeyType::STREAMING);
    std::vector<uint8_t> getKeyRequest(const SessionId& sessionId,
                                       const DrmHalVTSVendorModule_V1::ContentConfiguration&,
                                       const ::aidl::android::hardware::drm::KeyType& type);
    std::vector<uint8_t> provideKeyResponse(const SessionId& sessionId,
                                            const std::vector<uint8_t>& keyResponse);
    DrmHalVTSVendorModule_V1::ContentConfiguration getContent(
            const ::aidl::android::hardware::drm::KeyType& type = KeyType::STREAMING) const;

    KeyedVector toAidlKeyedVector(const std::map<std::string, std::string>& params);
    std::array<uint8_t, 16> toStdArray(const std::vector<uint8_t>& vec);
    uint8_t* fillRandom(const ::aidl::android::hardware::drm::SharedBuffer& buf);
    void getDecryptMemory(size_t size, size_t index, SharedBuffer& buf);

    uint32_t decrypt(::aidl::android::hardware::drm::Mode mode, bool isSecure,
                     const std::array<uint8_t, 16>& keyId, uint8_t* iv,
                     const std::vector<::aidl::android::hardware::drm::SubSample>& subSamples,
                     const ::aidl::android::hardware::drm::Pattern& pattern,
                     const std::vector<uint8_t>& key,
                     ::aidl::android::hardware::drm::Status expectedStatus);
    void aes_ctr_decrypt(uint8_t* dest, uint8_t* src, uint8_t* iv,
                         const std::vector<::aidl::android::hardware::drm::SubSample>& subSamples,
                         const std::vector<uint8_t>& key);
    void aes_cbc_decrypt(uint8_t* dest, uint8_t* src, uint8_t* iv,
                         const std::vector<::aidl::android::hardware::drm::SubSample>& subSamples,
                         const std::vector<uint8_t>& key);

    std::shared_ptr<::aidl::android::hardware::drm::IDrmFactory> drmFactory;
    std::shared_ptr<::aidl::android::hardware::drm::IDrmPlugin> drmPlugin;
    std::shared_ptr<::aidl::android::hardware::drm::ICryptoPlugin> cryptoPlugin;

    unique_ptr<DrmHalVTSVendorModule_V1> vendorModule;
    std::vector<DrmHalVTSVendorModule_V1::ContentConfiguration> contentConfigurations;

  private:
    std::shared_ptr<::aidl::android::hardware::drm::IDrmPlugin> createDrmPlugin();
    std::shared_ptr<::aidl::android::hardware::drm::ICryptoPlugin> createCryptoPlugin();
};

class DrmHalClearkeyTest : public DrmHalTest {
  public:
    virtual void SetUp() override {
        DrmHalTest::SetUp();
        auto kClearKeyUUID = toAidlUuid({0xE2, 0x71, 0x9D, 0x58, 0xA9, 0x85, 0xB3, 0xC9,
                                         0x78, 0x1A, 0xB0, 0x30, 0xAF, 0x78, 0xD3, 0x0E});
        static const std::string kMimeType = "video/mp4";
        static constexpr ::aidl::android::hardware::drm::SecurityLevel kSecurityLevel =
                ::aidl::android::hardware::drm::SecurityLevel::SW_SECURE_CRYPTO;

        if (!isCryptoSchemeSupported(kClearKeyUUID, kSecurityLevel, kMimeType)) {
            GTEST_SKIP() << "ClearKey not supported by " << GetParamService();
        }
    }
    virtual void TearDown() override {}
    void decryptWithInvalidKeys(
            std::vector<uint8_t>& invalidResponse, std::vector<uint8_t>& iv,
            const ::aidl::android::hardware::drm::Pattern& noPattern,
            const std::vector<::aidl::android::hardware::drm::SubSample>& subSamples);
};

/**
 *  Event Handling tests
 */
extern const char* kCallbackLostState;
extern const char* kCallbackKeysChange;

struct ListenerArgs {
    EventType eventType;
    SessionId sessionId;
    int64_t expiryTimeInMS;
    std::vector<uint8_t> data;
    std::vector<KeyStatus> keyStatusList;
    bool hasNewUsableKey;
};

class DrmHalPluginListener : public BnDrmPluginListener {
  public:
    DrmHalPluginListener() {}
    virtual ~DrmHalPluginListener() {}

    virtual ::ndk::ScopedAStatus onEvent(
            ::aidl::android::hardware::drm::EventType in_eventType,
            const std::vector<uint8_t>& in_sessionId,
            const std::vector<uint8_t>& in_data) override;

    virtual ::ndk::ScopedAStatus onExpirationUpdate(
            const std::vector<uint8_t>& in_sessionId,
            int64_t in_expiryTimeInMS) override;

    virtual ::ndk::ScopedAStatus onSessionLostState(
            const std::vector<uint8_t>& in_sessionId) override;

    virtual ::ndk::ScopedAStatus onKeysChange(
            const std::vector<uint8_t>& in_sessionId,
            const std::vector<::aidl::android::hardware::drm::KeyStatus>& in_keyStatusList,
            bool in_hasNewUsableKey) override;

    ListenerArgs getEventArgs();
    ListenerArgs getExpirationUpdateArgs();
    ListenerArgs getSessionLostStateArgs();
    ListenerArgs getKeysChangeArgs();

  private:
    ListenerArgs getListenerArgs(std::promise<ListenerArgs>& promise);
    std::promise<ListenerArgs> eventPromise, expirationUpdatePromise,
            sessionLostStatePromise, keysChangePromise;
};

}  // namespace vts
}  // namespace drm
}  // namespace hardware
}  // namespace android
}  // namespace aidl
