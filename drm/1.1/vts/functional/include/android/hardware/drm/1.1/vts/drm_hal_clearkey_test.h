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

#ifndef V1_1_DRM_HAL_CLEARKEY_TEST_H
#define V1_1_DRM_HAL_CLEARKEY_TEST_H

#include <android/hardware/drm/1.0/ICryptoPlugin.h>
#include <android/hardware/drm/1.0/IDrmPlugin.h>
#include <android/hardware/drm/1.0/types.h>
#include <android/hardware/drm/1.1/ICryptoFactory.h>
#include <android/hardware/drm/1.1/IDrmFactory.h>
#include <android/hardware/drm/1.1/IDrmPlugin.h>
#include <android/hardware/drm/1.1/types.h>
#include <android/hidl/allocator/1.0/IAllocator.h>
#include <android/hidl/manager/1.2/IServiceManager.h>

#include <gtest/gtest.h>
#include <hidl/HidlSupport.h>
#include <hidl/ServiceManagement.h>
#include <log/log.h>

#include <string>

#include "drm_vts_helper.h"

namespace drm = ::android::hardware::drm;

using ::android::hidl::allocator::V1_0::IAllocator;
using ::android::hidl::memory::V1_0::IMemory;

using drm_vts::DrmHalTestParam;
using drm_vts::PrintParamInstanceToString;

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
namespace V1_1 {
namespace vts {

using ::android::hardware::drm::V1_0::ICryptoPlugin;
using ::android::hardware::drm::V1_0::KeyedVector;
using ::android::hardware::drm::V1_0::KeyType;
using ::android::hardware::drm::V1_0::Mode;
using ::android::hardware::drm::V1_0::Pattern;
using ::android::hardware::drm::V1_0::SecureStop;
using ::android::hardware::drm::V1_0::SecureStopId;
using ::android::hardware::drm::V1_0::SessionId;
using ::android::hardware::drm::V1_0::Status;

// To be used in mpd to specify drm scheme for players
extern const uint8_t kClearKeyUUID[16];

class DrmHalClearkeyTest : public ::testing::TestWithParam<DrmHalTestParam> {
  public:
    void SetUp() override {
        const ::testing::TestInfo* const test_info =
                ::testing::UnitTest::GetInstance()->current_test_info();

        ALOGD("DrmHalClearkeyTest: Running test %s.%s", test_info->test_case_name(),
                test_info->name());

        const std::string instance = GetParam().instance_;

        sp<IDrmFactory> drmFactory = IDrmFactory::getService(instance);
        if (!drmFactory->isCryptoSchemeSupported(kClearKeyUUID)) {
            GTEST_SKIP() << instance << " does not support clearkey";
        }
        drmPlugin = createDrmPlugin(drmFactory);
        sp<ICryptoFactory> cryptoFactory = ICryptoFactory::getService(instance);
        cryptoPlugin = createCryptoPlugin(cryptoFactory);

        if (drmPlugin == nullptr || cryptoPlugin == nullptr) {
            if (instance == "clearkey") {
                ASSERT_NE(nullptr, drmPlugin.get()) << "Can't get clearkey drm@1.1 plugin";
                ASSERT_NE(nullptr, cryptoPlugin.get()) << "Can't get clearkey crypto@1.1 plugin";
            }
            GTEST_SKIP() << "Instance does not support clearkey";
        }
    }

    SessionId openSession();
    SessionId openSession(SecurityLevel level);
    void closeSession(const SessionId& sessionId);
    hidl_vec<uint8_t> loadKeys(const SessionId& sessionId, const KeyType& type);

  private:
    sp<IDrmPlugin> createDrmPlugin(sp<IDrmFactory> drmFactory) {
        if (drmFactory == nullptr) {
            return nullptr;
        }
        sp<IDrmPlugin> plugin = nullptr;
        auto res = drmFactory->createPlugin(GetParam().scheme_, "",
                [&](Status status, const sp<drm::V1_0::IDrmPlugin>& pluginV1_0) {
                    EXPECT_EQ(Status::OK == status, pluginV1_0 != nullptr);
                    plugin = IDrmPlugin::castFrom(pluginV1_0);
                });

        if (!res.isOk()) {
            ALOGE("createDrmPlugin remote call failed");
        }
        return plugin;
    }

    sp<ICryptoPlugin> createCryptoPlugin(sp<ICryptoFactory> cryptoFactory) {
        if (cryptoFactory == nullptr) {
            return nullptr;
        }
        sp<ICryptoPlugin> plugin = nullptr;
        hidl_vec<uint8_t> initVec;
        auto res = cryptoFactory->createPlugin(
                GetParam().scheme_, initVec,
                [&](Status status, const sp<drm::V1_0::ICryptoPlugin>& pluginV1_0) {
                    EXPECT_EQ(Status::OK == status, pluginV1_0 != nullptr);
                    plugin = pluginV1_0;
                });
        if (!res.isOk()) {
            ALOGE("createCryptoPlugin remote call failed");
        }
        return plugin;
    }

protected:
 template <typename CT>
 bool ValueEquals(DrmMetricGroup::ValueType type, const std::string& expected, const CT& actual) {
     return type == DrmMetricGroup::ValueType::STRING_TYPE && expected == actual.stringValue;
 }

 template <typename CT>
 bool ValueEquals(DrmMetricGroup::ValueType type, const int64_t expected, const CT& actual) {
     return type == DrmMetricGroup::ValueType::INT64_TYPE && expected == actual.int64Value;
 }

 template <typename CT>
 bool ValueEquals(DrmMetricGroup::ValueType type, const double expected, const CT& actual) {
     return type == DrmMetricGroup::ValueType::DOUBLE_TYPE && expected == actual.doubleValue;
 }

 template <typename AT, typename VT>
 bool ValidateMetricAttributeAndValue(const DrmMetricGroup::Metric& metric,
                                      const std::string& attributeName, const AT& attributeValue,
                                      const std::string& componentName, const VT& componentValue) {
     bool validAttribute = false;
     bool validComponent = false;
     for (const DrmMetricGroup::Attribute& attribute : metric.attributes) {
         if (attribute.name == attributeName &&
             ValueEquals(attribute.type, attributeValue, attribute)) {
             validAttribute = true;
         }
     }
     for (const DrmMetricGroup::Value& value : metric.values) {
         if (value.componentName == componentName &&
             ValueEquals(value.type, componentValue, value)) {
             validComponent = true;
         }
     }
     return validAttribute && validComponent;
 }

 template <typename AT, typename VT>
 bool ValidateMetricAttributeAndValue(const hidl_vec<DrmMetricGroup>& metricGroups,
                                      const std::string& metricName,
                                      const std::string& attributeName, const AT& attributeValue,
                                      const std::string& componentName, const VT& componentValue) {
     bool foundMetric = false;
     for (const auto& group : metricGroups) {
         for (const auto& metric : group.metrics) {
             if (metric.name == metricName) {
                 foundMetric = foundMetric || ValidateMetricAttributeAndValue(
                                                  metric, attributeName, attributeValue,
                                                  componentName, componentValue);
             }
         }
     }
     return foundMetric;
 }

 sp<IDrmPlugin> drmPlugin;
 sp<ICryptoPlugin> cryptoPlugin;
};

}  // namespace vts
}  // namespace V1_1
}  // namespace drm
}  // namespace hardware
}  // namespace android

#endif  // V1_1_DRM_HAL_CLEARKEY_TEST_H
