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

#ifndef DRM_HAL_VENDOR_TEST_H
#define DRM_HAL_VENDOR_TEST_H

#include <android/hardware/drm/1.0/ICryptoFactory.h>
#include <android/hardware/drm/1.0/ICryptoPlugin.h>
#include <android/hardware/drm/1.0/IDrmFactory.h>
#include <android/hardware/drm/1.0/IDrmPlugin.h>
#include <android/hardware/drm/1.0/IDrmPluginListener.h>
#include <android/hardware/drm/1.0/types.h>
#include <android/hidl/allocator/1.0/IAllocator.h>

#include <gtest/gtest.h>
#include <hidl/HidlSupport.h>
#include <hidl/ServiceManagement.h>
#include <hidlmemory/mapping.h>
#include <log/log.h>

#include <memory>
#include <set>
#include <vector>

#include "drm_hal_vendor_module_api.h"
#include "drm_vts_helper.h"
#include "vendor_modules.h"
#include <VtsHalHidlTargetCallbackBase.h>

using ::android::hidl::allocator::V1_0::IAllocator;
using ::android::hidl::memory::V1_0::IMemory;

using ::drm_vts::DrmHalTestParam;
using ::drm_vts::PrintParamInstanceToString;

using std::string;
using std::unique_ptr;
using std::map;
using std::vector;

using ContentConfiguration = ::DrmHalVTSVendorModule_V1::ContentConfiguration;
using Key = ::DrmHalVTSVendorModule_V1::ContentConfiguration::Key;

#define ASSERT_OK(ret) ASSERT_TRUE(ret.isOk())
#define EXPECT_OK(ret) EXPECT_TRUE(ret.isOk())

#define RETURN_IF_SKIPPED                                                                  \
    if (vendorModule == nullptr || !vendorModule->isInstalled()) {                         \
        GTEST_SKIP() << "This drm scheme not supported."                                   \
                     << " library:" << GetParam() << " service-name:"                      \
                     << (vendorModule == nullptr ? "N/A" : vendorModule->getServiceName()) \
                     << std::endl;                                                         \
        return;                                                                            \
    }

namespace android {
namespace hardware {
namespace drm {
namespace V1_0 {
namespace vts {

class DrmHalVendorFactoryTest : public testing::TestWithParam<DrmHalTestParam> {
   public:
     DrmHalVendorFactoryTest();
     virtual ~DrmHalVendorFactoryTest() {}

     virtual void SetUp() {
         const ::testing::TestInfo* const test_info =
                 ::testing::UnitTest::GetInstance()->current_test_info();
         ALOGD("Running test %s.%s from vendor module %s", test_info->test_case_name(),
               test_info->name(), GetParam().instance_.c_str());

         const std::string instance = GetParam().instance_;
         if (instance == "widevine") {
             ASSERT_NE(nullptr, vendorModule.get());
         }

         if (vendorModule == nullptr) {
             GTEST_SKIP() << "No vendor module available";
         } else {
             ASSERT_EQ(instance, vendorModule->getServiceName());
             contentConfigurations = vendorModule->getContentConfigurations();
         }

         drmFactory = IDrmFactory::getService(instance);
         ASSERT_NE(nullptr, drmFactory.get());
         cryptoFactory = ICryptoFactory::getService(instance);
         ASSERT_NE(nullptr, cryptoFactory.get());

         // If drm scheme not installed skip subsequent tests
         if (!drmFactory->isCryptoSchemeSupported(getUUID())) {
             // no GTEST_SKIP since only some tests require the module
             vendorModule->setInstalled(false);
             hidl_array<uint8_t, 16> noUUID;
             ASSERT_EQ(GetParamUUID(), noUUID) << "param uuid unsupported";
         }
     }

   protected:
    hidl_array<uint8_t, 16> getUUID() {
        if (GetParamUUID() == hidl_array<uint8_t, 16>()) {
            return getVendorUUID();
        }
        return GetParamUUID();
    }

    hidl_array<uint8_t, 16> getVendorUUID() {
        if (vendorModule == nullptr) return {};
        vector<uint8_t> uuid = vendorModule->getUUID();
        return hidl_array<uint8_t, 16>(&uuid[0]);
    }

    hidl_array<uint8_t, 16> GetParamUUID() {
        return GetParam().scheme_;
    }

    sp<IDrmFactory> drmFactory;
    sp<ICryptoFactory> cryptoFactory;
    unique_ptr<DrmHalVTSVendorModule_V1> vendorModule;
    vector<ContentConfiguration> contentConfigurations;
};

class DrmHalVendorPluginTest : public DrmHalVendorFactoryTest {
   public:
    virtual ~DrmHalVendorPluginTest() {}
    virtual void SetUp() override {
        // Create factories
        DrmHalVendorFactoryTest::SetUp();
        RETURN_IF_SKIPPED;

        hidl_string packageName("android.hardware.drm.test");
        auto res = drmFactory->createPlugin(
                getVendorUUID(), packageName,
                [this](Status status, const sp<IDrmPlugin>& plugin) {
                    EXPECT_EQ(Status::OK, status);
                    ASSERT_NE(nullptr, plugin.get());
                    drmPlugin = plugin;
                });
        ASSERT_OK(res);

        hidl_vec<uint8_t> initVec;
        res = cryptoFactory->createPlugin(
                getVendorUUID(), initVec,
                [this](Status status, const sp<ICryptoPlugin>& plugin) {
                    EXPECT_EQ(Status::OK, status);
                    ASSERT_NE(nullptr, plugin.get());
                    cryptoPlugin = plugin;
                });
        ASSERT_OK(res);
    }

    virtual void TearDown() override {}

    SessionId openSession();
    void closeSession(const SessionId& sessionId);
    sp<IMemory> getDecryptMemory(size_t size, size_t index);
    KeyedVector toHidlKeyedVector(const map<string, string>& params);
    hidl_vec<uint8_t> loadKeys(const SessionId& sessionId,
                               const ContentConfiguration& configuration,
                               const KeyType& type);

   protected:
    sp<IDrmPlugin> drmPlugin;
    sp<ICryptoPlugin> cryptoPlugin;
};

class DrmHalVendorDecryptTest : public DrmHalVendorPluginTest {
   public:
    DrmHalVendorDecryptTest() = default;
    virtual ~DrmHalVendorDecryptTest() {}

   protected:
    void fillRandom(const sp<IMemory>& memory);
    hidl_array<uint8_t, 16> toHidlArray(const vector<uint8_t>& vec) {
        EXPECT_EQ(vec.size(), 16u);
        return hidl_array<uint8_t, 16>(&vec[0]);
    }
    hidl_vec<KeyValue> queryKeyStatus(SessionId sessionId);
    void removeKeys(SessionId sessionId);
    uint32_t decrypt(Mode mode, bool isSecure,
            const hidl_array<uint8_t, 16>& keyId, uint8_t* iv,
            const hidl_vec<SubSample>& subSamples, const Pattern& pattern,
            const vector<uint8_t>& key, Status expectedStatus);
    void aes_ctr_decrypt(uint8_t* dest, uint8_t* src, uint8_t* iv,
            const hidl_vec<SubSample>& subSamples, const vector<uint8_t>& key);
    void aes_cbc_decrypt(uint8_t* dest, uint8_t* src, uint8_t* iv,
            const hidl_vec<SubSample>& subSamples, const vector<uint8_t>& key);
};

}  // namespace vts
}  // namespace V1_0
}  // namespace drm
}  // namespace hardware
}  // namespace android

#endif  // DRM_HAL_VENDOR_TEST_H
