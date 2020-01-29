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

#ifndef DRM_HAL_CLEARKEY_TEST_H
#define DRM_HAL_CLEARKEY_TEST_H

#include <android/hardware/drm/1.0/ICryptoFactory.h>
#include <android/hardware/drm/1.0/ICryptoPlugin.h>
#include <android/hardware/drm/1.0/IDrmFactory.h>
#include <android/hardware/drm/1.0/IDrmPlugin.h>
#include <android/hardware/drm/1.0/types.h>
#include <android/hidl/allocator/1.0/IAllocator.h>

#include <gtest/gtest.h>
#include <hidl/HidlSupport.h>
#include <hidl/ServiceManagement.h>
#include <hidlmemory/mapping.h>
#include <log/log.h>

#include "drm_vts_helper.h"

using ::android::hidl::allocator::V1_0::IAllocator;
using ::android::hidl::memory::V1_0::IMemory;

using ::drm_vts::DrmHalTestParam;
using ::drm_vts::PrintParamInstanceToString;

using std::string;
using std::map;
using std::vector;

/**
 * These clearkey tests use white box knowledge of the legacy clearkey
 * plugin to verify that the HIDL HAL services and interfaces are working.
 * It is not intended to verify any vendor's HAL implementation. If you
 * are looking for vendor HAL tests, see drm_hal_vendor_test.cpp
 */
#define ASSERT_OK(ret) ASSERT_TRUE(ret.isOk())
#define EXPECT_OK(ret) EXPECT_TRUE(ret.isOk())

namespace android {
namespace hardware {
namespace drm {
namespace V1_0 {
namespace vts {

class DrmHalClearkeyFactoryTest : public ::testing::TestWithParam<DrmHalTestParam> {
  public:
    void SetUp() override {
        const ::testing::TestInfo* const test_info =
                ::testing::UnitTest::GetInstance()->current_test_info();
        ALOGD("Running test %s.%s", test_info->test_case_name(),
              test_info->name());

        const std::string instanceName = GetParam().instance_;
        drmFactory = IDrmFactory::getService(instanceName);
        ASSERT_NE(nullptr, drmFactory.get());
        cryptoFactory = ICryptoFactory::getService(instanceName);
        ASSERT_NE(nullptr, cryptoFactory.get());

        const bool drmClearKey = drmFactory->isCryptoSchemeSupported(kClearKeyUUID);
        const bool cryptoClearKey = cryptoFactory->isCryptoSchemeSupported(kClearKeyUUID);
        EXPECT_EQ(drmClearKey, cryptoClearKey);
        const bool supportsClearKey = drmClearKey && cryptoClearKey;

        const bool drmCommonPsshBox = drmFactory->isCryptoSchemeSupported(kCommonPsshBoxUUID);
        const bool cryptoCommonPsshBox = cryptoFactory->isCryptoSchemeSupported(kCommonPsshBoxUUID);
        EXPECT_EQ(drmCommonPsshBox, cryptoCommonPsshBox);
        const bool supportsCommonPsshBox = drmCommonPsshBox && cryptoCommonPsshBox;

        EXPECT_EQ(supportsClearKey, supportsCommonPsshBox);
        correspondsToThisTest = supportsClearKey && supportsCommonPsshBox;

        if (instanceName == "clearkey") {
            EXPECT_TRUE(correspondsToThisTest);

            // TODO(b/147449315)
            // Only the clearkey plugged into the "default" instance supports
            // this test. Currently the "clearkey" instance fails some tests
            // here.
            GTEST_SKIP() << "Clearkey tests don't work with 'clearkey' instance yet.";
        }

        if (!correspondsToThisTest) {
            GTEST_SKIP() << "Cannot test clearkey features on non-clearkey DRM modules";
        }
    }

   protected:
    static constexpr uint8_t kCommonPsshBoxUUID[16] = {
        0x10, 0x77, 0xEF, 0xEC, 0xC0, 0xB2, 0x4D, 0x02,
        0xAC, 0xE3, 0x3C, 0x1E, 0x52, 0xE2, 0xFB, 0x4B};

    // To be used in mpd to specify drm scheme for players
    static constexpr uint8_t kClearKeyUUID[16] = {
        0xE2, 0x71, 0x9D, 0x58, 0xA9, 0x85, 0xB3, 0xC9,
        0x78, 0x1A, 0xB0, 0x30, 0xAF, 0x78, 0xD3, 0x0E};

    sp<IDrmFactory> drmFactory;
    sp<ICryptoFactory> cryptoFactory;

    bool correspondsToThisTest;
};

class DrmHalClearkeyPluginTest : public DrmHalClearkeyFactoryTest {
   public:
    virtual void SetUp() override {
        // Create factories
        DrmHalClearkeyFactoryTest::SetUp();

        if (!correspondsToThisTest) {
            GTEST_SKIP() << "Cannot test clearkey features on non-clearkey DRM modules";
        }

        ASSERT_NE(nullptr, drmFactory.get());
        hidl_string packageName("android.hardware.drm.test");
        auto res = drmFactory->createPlugin(
                getUUID(), packageName,
                [this](Status status, const sp<IDrmPlugin>& plugin) {
                    EXPECT_EQ(Status::OK, status);
                    ASSERT_NE(nullptr, plugin.get());
                    drmPlugin = plugin;
                });
        ASSERT_OK(res);

        hidl_vec<uint8_t> initVec;
        res = cryptoFactory->createPlugin(
                getUUID(), initVec,
                [this](Status status, const sp<ICryptoPlugin>& plugin) {
                    EXPECT_EQ(Status::OK, status);
                    ASSERT_NE(nullptr, plugin.get());
                    cryptoPlugin = plugin;
                });
        ASSERT_OK(res);
    }

    SessionId openSession();
    void closeSession(const SessionId& sessionId);
    hidl_vec<uint8_t> loadKeys(const SessionId& sessionId, const KeyType& type);
    sp<IMemory> getDecryptMemory(size_t size, size_t index);

   protected:
    hidl_array<uint8_t, 16> getUUID() {
        if (GetParamUUID() == hidl_array<uint8_t, 16>()) {
            return kClearKeyUUID;
        }
        return GetParamUUID();
    }

    hidl_array<uint8_t, 16> GetParamUUID() {
        return GetParam().scheme_;
    }

    sp<IDrmPlugin> drmPlugin;
    sp<ICryptoPlugin> cryptoPlugin;
};

class DrmHalClearkeyDecryptTest : public DrmHalClearkeyPluginTest {
   public:
     void SetUp() override {
         DrmHalClearkeyPluginTest::SetUp();

         if (!correspondsToThisTest) {
             GTEST_SKIP() << "Cannot test clearkey features on non-clearkey DRM modules";
         }
     }
    void fillRandom(const sp<IMemory>& memory);
    hidl_array<uint8_t, 16> toHidlArray(const vector<uint8_t>& vec) {
        EXPECT_EQ(16u, vec.size());
        return hidl_array<uint8_t, 16>(&vec[0]);
    }
    uint32_t decrypt(Mode mode, uint8_t* iv, const hidl_vec<SubSample>& subSamples,
            const Pattern& pattern, Status status);
    void aes_ctr_decrypt(uint8_t* dest, uint8_t* src, uint8_t* iv,
            const hidl_vec<SubSample>& subSamples, const vector<uint8_t>& key);
    void aes_cbc_decrypt(uint8_t* dest, uint8_t* src, uint8_t* iv,
            const hidl_vec<SubSample>& subSamples, const vector<uint8_t>& key);
    void decryptWithInvalidKeys(hidl_vec<uint8_t>& invalidResponse,
            vector<uint8_t>& iv, const Pattern& noPattern, const vector<SubSample>& subSamples);
};

}  // namespace vts
}  // namespace V1_0
}  // namespace drm
}  // namespace hardware
}  // namespace android

#endif  // DRM_HAL_CLEARKEY_TEST_H
