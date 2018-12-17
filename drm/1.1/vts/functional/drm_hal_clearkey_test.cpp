/*
 * Copyright (C) 2017 The Android Open Source Project
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

#define LOG_TAG "drm_hal_clearkey_test@1.1"

#include <android/hardware/drm/1.1/ICryptoFactory.h>
#include <android/hardware/drm/1.0/ICryptoPlugin.h>
#include <android/hardware/drm/1.1/IDrmFactory.h>
#include <android/hardware/drm/1.0/IDrmPlugin.h>
#include <android/hardware/drm/1.1/IDrmPlugin.h>
#include <android/hardware/drm/1.0/types.h>
#include <android/hardware/drm/1.1/types.h>
#include <android/hidl/allocator/1.0/IAllocator.h>
#include <android/hidl/manager/1.0/IServiceManager.h>
#include <gtest/gtest.h>
#include <hidl/HidlSupport.h>
#include <hidl/ServiceManagement.h>
#include <hidlmemory/mapping.h>
#include <log/log.h>
#include <memory>
#include <openssl/aes.h>
#include <random>

#include "VtsHalHidlTargetTestBase.h"
#include "VtsHalHidlTargetTestEnvBase.h"

namespace drm = ::android::hardware::drm;
using ::android::hardware::drm::V1_0::BufferType;
using ::android::hardware::drm::V1_0::DestinationBuffer;
using ::android::hardware::drm::V1_0::ICryptoPlugin;
using ::android::hardware::drm::V1_0::KeyedVector;
using ::android::hardware::drm::V1_0::KeyValue;
using ::android::hardware::drm::V1_0::KeyType;
using ::android::hardware::drm::V1_0::Mode;
using ::android::hardware::drm::V1_0::Pattern;
using ::android::hardware::drm::V1_0::SecureStop;
using ::android::hardware::drm::V1_0::SecureStopId;
using ::android::hardware::drm::V1_0::SessionId;
using ::android::hardware::drm::V1_0::SharedBuffer;
using ::android::hardware::drm::V1_0::Status;
using ::android::hardware::drm::V1_0::SubSample;
using ::android::hardware::drm::V1_0::SubSample;

using ::android::hardware::drm::V1_1::DrmMetricGroup;
using ::android::hardware::drm::V1_1::HdcpLevel;
using ::android::hardware::drm::V1_1::ICryptoFactory;
using ::android::hardware::drm::V1_1::IDrmFactory;
using ::android::hardware::drm::V1_1::IDrmPlugin;
using ::android::hardware::drm::V1_1::KeyRequestType;
using ::android::hardware::drm::V1_1::SecureStopRelease;
using ::android::hardware::drm::V1_1::SecurityLevel;
using ::android::hardware::drm::V1_1::SecurityLevel;

using ::android::hardware::hidl_array;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hidl::allocator::V1_0::IAllocator;
using ::android::hidl::memory::V1_0::IMemory;
using ::android::sp;

using std::string;
using std::unique_ptr;
using std::random_device;
using std::map;
using std::mt19937;
using std::vector;

/**
 * These clearkey tests use white box knowledge of the legacy clearkey
 * plugin to verify that the HIDL HAL services and interfaces are working.
 * It is not intended to verify any vendor's HAL implementation. If you
 * are looking for vendor HAL tests, see drm_hal_vendor_test.cpp
 */
#define ASSERT_OK(ret) ASSERT_TRUE(ret.isOk())
#define EXPECT_OK(ret) EXPECT_TRUE(ret.isOk())

// To be used in mpd to specify drm scheme for players
static const uint8_t kClearKeyUUID[16] = {
    0xE2, 0x71, 0x9D, 0x58, 0xA9, 0x85, 0xB3, 0xC9,
    0x78, 0x1A, 0xB0, 0x30, 0xAF, 0x78, 0xD3, 0x0E};

// Test environment for drm
class DrmHidlEnvironment : public ::testing::VtsHalHidlTargetTestEnvBase {
   public:
    // get the test environment singleton
    static DrmHidlEnvironment* Instance() {
        static DrmHidlEnvironment* instance = new DrmHidlEnvironment;
        return instance;
    }

    virtual void HidlSetUp() override { ALOGI("SetUp DrmHidlEnvironment"); }

    virtual void HidlTearDown() override { ALOGI("TearDown DrmHidlEnvironment"); }

    void registerTestServices() override {
        registerTestService<ICryptoFactory>();
        registerTestService<IDrmFactory>();
        setServiceCombMode(::testing::HalServiceCombMode::NO_COMBINATION);
    }

   private:
    DrmHidlEnvironment() {}

    GTEST_DISALLOW_COPY_AND_ASSIGN_(DrmHidlEnvironment);
};


class DrmHalClearkeyTest : public ::testing::VtsHalHidlTargetTestBase {
public:
    virtual void SetUp() override {
        const ::testing::TestInfo* const test_info =
                ::testing::UnitTest::GetInstance()->current_test_info();

        ALOGD("DrmHalClearkeyTest: Running test %s.%s", test_info->test_case_name(),
                test_info->name());

        auto manager = android::hardware::defaultServiceManager();
        ASSERT_NE(nullptr, manager.get());
        manager->listByInterface(IDrmFactory::descriptor,
                [&](const hidl_vec<hidl_string> &registered) {
                    for (const auto &instance : registered) {
                        sp<IDrmFactory> drmFactory =
                                ::testing::VtsHalHidlTargetTestBase::getService<IDrmFactory>(instance);
                        drmPlugin = createDrmPlugin(drmFactory);
                        if (drmPlugin != nullptr) {
                            break;
                        }
                    }
                }
            );

        manager->listByInterface(ICryptoFactory::descriptor,
                [&](const hidl_vec<hidl_string> &registered) {
                    for (const auto &instance : registered) {
                        sp<ICryptoFactory> cryptoFactory =
                                ::testing::VtsHalHidlTargetTestBase::getService<ICryptoFactory>(instance);
                        cryptoPlugin = createCryptoPlugin(cryptoFactory);
                        if (cryptoPlugin != nullptr) {
                            break;
                        }
                    }
                }
            );

        ASSERT_NE(nullptr, drmPlugin.get()) << "Can't find clearkey drm@1.1 plugin";
        ASSERT_NE(nullptr, cryptoPlugin.get()) << "Can't find clearkey crypto@1.1 plugin";
    }


    virtual void TearDown() override {}

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
        auto res = drmFactory->createPlugin(
                kClearKeyUUID, "",
                        [&](Status status, const sp<drm::V1_0::IDrmPlugin>& pluginV1_0) {
                    EXPECT_EQ(Status::OK, status);
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
                kClearKeyUUID, initVec,
                        [&](Status status, const sp<drm::V1_0::ICryptoPlugin>& pluginV1_0) {
                    EXPECT_EQ(Status::OK, status);
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


/**
 * Helper method to open a session and verify that a non-empty
 * session ID is returned
 */
SessionId DrmHalClearkeyTest::openSession() {
    SessionId sessionId;

    auto res = drmPlugin->openSession(
            [&sessionId](Status status, const SessionId& id) {
                EXPECT_EQ(Status::OK, status);
                EXPECT_NE(0u, id.size());
                sessionId = id;
            });
    EXPECT_OK(res);
    return sessionId;
}

/**
 * Helper method to open as session using V1.1 API
 */
SessionId DrmHalClearkeyTest::openSession(SecurityLevel level) {
    SessionId sessionId;

    auto res = drmPlugin->openSession_1_1(level,
            [&sessionId](Status status, const SessionId& id) {
                EXPECT_EQ(Status::OK, status);
                EXPECT_NE(0u, id.size());
                sessionId = id;
            });
    EXPECT_OK(res);
    return sessionId;
}


/**
 * Helper method to close a session
 */
void DrmHalClearkeyTest::closeSession(const SessionId& sessionId) {
    EXPECT_TRUE(drmPlugin->closeSession(sessionId).isOk());
}

/**
 * Helper method to load keys for subsequent decrypt tests.
 * These tests use predetermined key request/response to
 * avoid requiring a round trip to a license server.
 */
hidl_vec<uint8_t> DrmHalClearkeyTest::loadKeys(
    const SessionId& sessionId, const KeyType& type = KeyType::STREAMING) {
    hidl_vec<uint8_t> initData = {
        // BMFF box header (4 bytes size + 'pssh')
        0x00, 0x00, 0x00, 0x34, 0x70, 0x73, 0x73, 0x68,
        // full box header (version = 1 flags = 0)
        0x01, 0x00, 0x00, 0x00,
        // system id
        0x10, 0x77, 0xef, 0xec, 0xc0, 0xb2, 0x4d, 0x02, 0xac, 0xe3, 0x3c,
        0x1e, 0x52, 0xe2, 0xfb, 0x4b,
        // number of key ids
        0x00, 0x00, 0x00, 0x01,
        // key id
        0x60, 0x06, 0x1e, 0x01, 0x7e, 0x47, 0x7e, 0x87, 0x7e, 0x57, 0xd0,
        0x0d, 0x1e, 0xd0, 0x0d, 0x1e,
        // size of data, must be zero
        0x00, 0x00, 0x00, 0x00};

    hidl_vec<uint8_t> expectedKeyRequest = {
        0x7b, 0x22, 0x6b, 0x69, 0x64, 0x73, 0x22, 0x3a, 0x5b, 0x22, 0x59, 0x41, 0x59, 0x65,
        0x41, 0x58, 0x35, 0x48, 0x66, 0x6f, 0x64, 0x2d, 0x56, 0x39, 0x41, 0x4e, 0x48, 0x74,
        0x41, 0x4e, 0x48, 0x67, 0x22, 0x5d, 0x2c, 0x22, 0x74, 0x79, 0x70, 0x65, 0x22, 0x3a,
        0x22, 0x74, 0x65, 0x6d, 0x70, 0x6f, 0x72, 0x61, 0x72, 0x79, 0x22, 0x7d};

    hidl_vec<uint8_t> knownKeyResponse = {
        0x7b, 0x22, 0x6b, 0x65, 0x79, 0x73, 0x22, 0x3a, 0x5b, 0x7b, 0x22, 0x6b, 0x74, 0x79, 0x22,
        0x3a, 0x22, 0x6f, 0x63, 0x74, 0x22, 0x2c, 0x22, 0x6b, 0x69, 0x64, 0x22, 0x3a, 0x22, 0x59,
        0x41, 0x59, 0x65, 0x41, 0x58, 0x35, 0x48, 0x66, 0x6f, 0x64, 0x2d, 0x56, 0x39, 0x41, 0x4e,
        0x48, 0x74, 0x41, 0x4e, 0x48, 0x67, 0x22, 0x2c, 0x22, 0x6b, 0x22, 0x3a, 0x22, 0x47, 0x6f,
        0x6f, 0x67, 0x6c, 0x65, 0x54, 0x65, 0x73, 0x74, 0x4b, 0x65, 0x79, 0x42, 0x61, 0x73, 0x65,
        0x36, 0x34, 0x67, 0x67, 0x67, 0x22, 0x7d, 0x5d, 0x7d, 0x0a};

    hidl_string mimeType = "video/mp4";
    KeyedVector optionalParameters;
    auto res = drmPlugin->getKeyRequest_1_1(
        sessionId, initData, mimeType, type, optionalParameters,
        [&](Status status, const hidl_vec<uint8_t>& request,
            KeyRequestType requestType, const hidl_string&) {
            EXPECT_EQ(Status::OK, status);
            EXPECT_EQ(KeyRequestType::INITIAL, requestType);
            EXPECT_EQ(request, expectedKeyRequest);
        });
    EXPECT_OK(res);

    hidl_vec<uint8_t> keySetId;
    res = drmPlugin->provideKeyResponse(
        sessionId, knownKeyResponse,
        [&](Status status, const hidl_vec<uint8_t>& myKeySetId) {
            EXPECT_EQ(Status::OK, status);
            EXPECT_EQ(0u, myKeySetId.size());
            keySetId = myKeySetId;
        });
    EXPECT_OK(res);
    return keySetId;
}

/**
 * Test openSession negative case: security level higher than supported
 */
TEST_F(DrmHalClearkeyTest, OpenSessionBadLevel) {
    auto res = drmPlugin->openSession_1_1(SecurityLevel::HW_SECURE_ALL,
            [&](Status status, const SessionId& /* id */) {
                EXPECT_EQ(Status::ERROR_DRM_CANNOT_HANDLE, status);
            });
    EXPECT_OK(res);
}

/**
 * Test getKeyRequest_1_1 via loadKeys
 */
TEST_F(DrmHalClearkeyTest, GetKeyRequest) {
    auto sessionId = openSession();
    loadKeys(sessionId);
    closeSession(sessionId);
}

/**
 * A get key request should fail if no sessionId is provided
 */
TEST_F(DrmHalClearkeyTest, GetKeyRequestNoSession) {
    SessionId invalidSessionId;
    hidl_vec<uint8_t> initData;
    hidl_string mimeType = "video/mp4";
    KeyedVector optionalParameters;
    auto res = drmPlugin->getKeyRequest_1_1(
            invalidSessionId, initData, mimeType, KeyType::STREAMING,
            optionalParameters,
            [&](Status status, const hidl_vec<uint8_t>&, KeyRequestType,
                const hidl_string&) { EXPECT_EQ(Status::BAD_VALUE, status); });
    EXPECT_OK(res);
}

/**
 * The clearkey plugin doesn't support offline key requests.
 * Test that the plugin returns the expected error code in
 * this case.
 */
TEST_F(DrmHalClearkeyTest, GetKeyRequestOfflineKeyTypeNotSupported) {
    auto sessionId = openSession();
    hidl_vec<uint8_t> initData;
    hidl_string mimeType = "video/mp4";
    KeyedVector optionalParameters;

    auto res = drmPlugin->getKeyRequest_1_1(
            sessionId, initData, mimeType, KeyType::OFFLINE, optionalParameters,
            [&](Status status, const hidl_vec<uint8_t>&, KeyRequestType,
                const hidl_string&) {
                // Clearkey plugin doesn't support offline key type
                EXPECT_EQ(Status::ERROR_DRM_CANNOT_HANDLE, status);
            });
    EXPECT_OK(res);
    closeSession(sessionId);
}

/**
 * Test that the plugin returns valid connected and max HDCP levels
 */
TEST_F(DrmHalClearkeyTest, GetHdcpLevels) {
    auto res = drmPlugin->getHdcpLevels(
            [&](Status status, const HdcpLevel &connectedLevel,
                const HdcpLevel &maxLevel) {
                EXPECT_EQ(Status::OK, status);
                EXPECT_GE(connectedLevel, HdcpLevel::HDCP_NONE);
                EXPECT_LE(maxLevel, HdcpLevel::HDCP_NO_OUTPUT);
            });
    EXPECT_OK(res);
}

/**
 * Since getHdcpLevels only queries information there are no
 * negative cases.
 */

/**
 * Test that the plugin returns default open and max session counts
 */
TEST_F(DrmHalClearkeyTest, GetDefaultSessionCounts) {
    auto res = drmPlugin->getNumberOfSessions(
            [&](Status status, uint32_t currentSessions,
                    uint32_t maxSessions) {
                EXPECT_EQ(Status::OK, status);
                EXPECT_GE(maxSessions, (uint32_t)8);
                EXPECT_GE(currentSessions, (uint32_t)0);
                EXPECT_LE(currentSessions, maxSessions);
            });
    EXPECT_OK(res);
}

/**
 * Test that the plugin returns valid open and max session counts
 * after a session is opened.
 */
TEST_F(DrmHalClearkeyTest, GetOpenSessionCounts) {
    uint32_t initialSessions = 0;
    auto res = drmPlugin->getNumberOfSessions(
            [&](Status status, uint32_t currentSessions,
                    uint32_t maxSessions) {
                EXPECT_EQ(Status::OK, status);
                EXPECT_GE(maxSessions, (uint32_t)8);
                EXPECT_GE(currentSessions, (uint32_t)0);
                EXPECT_LE(currentSessions, maxSessions);
                initialSessions = currentSessions;
            });
    EXPECT_OK(res);

    SessionId session = openSession();
    res = drmPlugin->getNumberOfSessions(
            [&](Status status, uint32_t currentSessions,
                    uint32_t /*maxSessions*/) {
                EXPECT_EQ(Status::OK, status);
                EXPECT_EQ(currentSessions, initialSessions + 1);
            });
    EXPECT_OK(res);

    closeSession(session);
    res = drmPlugin->getNumberOfSessions(
            [&](Status status, uint32_t currentSessions,
                    uint32_t /*maxSessions*/) {
                EXPECT_EQ(Status::OK, status);
                EXPECT_EQ(currentSessions, initialSessions);
            });
    EXPECT_OK(res);
}

/**
 * Since getNumberOfSessions only queries information there are no
 * negative cases.
 */

/**
 * Test that the plugin returns the same security level
 * by default as when it is requested explicitly
 */
TEST_F(DrmHalClearkeyTest, GetDefaultSecurityLevel) {
    SessionId session = openSession();
    SecurityLevel defaultLevel;
    auto res = drmPlugin->getSecurityLevel(session,
            [&](Status status, SecurityLevel level) {
                EXPECT_EQ(Status::OK, status);
                defaultLevel = level;
            });
    EXPECT_OK(res);
    closeSession(session);

    session = openSession(defaultLevel);
    res = drmPlugin->getSecurityLevel(session,
            [&](Status status, SecurityLevel level) {
                EXPECT_EQ(Status::OK, status);
                EXPECT_EQ(level, defaultLevel);
            });
    EXPECT_OK(res);
    closeSession(session);
}

/**
 * Test that the plugin returns the lowest security level
 * when it is requested
 */
TEST_F(DrmHalClearkeyTest, GetSecurityLevel) {
    SessionId session = openSession(SecurityLevel::SW_SECURE_CRYPTO);
    auto res = drmPlugin->getSecurityLevel(session,
            [&](Status status, SecurityLevel level) {
                EXPECT_EQ(Status::OK, status);
                EXPECT_EQ(level, SecurityLevel::SW_SECURE_CRYPTO);
            });
    EXPECT_OK(res);
    closeSession(session);
}

/**
 * Test that the plugin returns the documented error
 * when requesting the security level for an invalid sessionId
 */
TEST_F(DrmHalClearkeyTest, GetSecurityLevelInvalidSessionId) {
    SessionId session;
    auto res = drmPlugin->getSecurityLevel(session,
            [&](Status status, SecurityLevel /*level*/) {
                EXPECT_EQ(Status::BAD_VALUE, status);
            });
    EXPECT_OK(res);
}

/**
 * Test metrics are set appropriately for open and close operations.
 */
TEST_F(DrmHalClearkeyTest, GetMetricsOpenClose) {
    SessionId sessionId = openSession();
    // The first close should be successful.
    closeSession(sessionId);
    // The second close should fail (not opened).
    EXPECT_EQ(Status::ERROR_DRM_SESSION_NOT_OPENED, drmPlugin->closeSession(sessionId));

    auto res = drmPlugin->getMetrics([this](Status status, hidl_vec<DrmMetricGroup> metricGroups) {
        EXPECT_EQ(Status::OK, status);

        // Verify the open_session metric.
        EXPECT_TRUE(ValidateMetricAttributeAndValue(metricGroups, "open_session", "status",
                                                    (int64_t)0, "count", (int64_t)1));
        // Verify the close_session - success metric.
        EXPECT_TRUE(ValidateMetricAttributeAndValue(metricGroups, "close_session", "status",
                                                    (int64_t)0, "count", (int64_t)1));
        // Verify the close_session - error metric.
        EXPECT_TRUE(ValidateMetricAttributeAndValue(metricGroups, "close_session", "status",
                                                    (int64_t)Status::ERROR_DRM_SESSION_NOT_OPENED,
                                                    "count", (int64_t)1));
    });
    EXPECT_OK(res);
}

/**
 * Since getMetrics only queries information there are no
 * negative cases.
 */

/**
 * Test that there are no secure stop ids after clearing them
 */
TEST_F(DrmHalClearkeyTest, GetSecureStopIdsCleared) {
    auto stat = drmPlugin->removeAllSecureStops();
    EXPECT_OK(stat);

    bool ok = drmPlugin->getSecureStopIds(
            [&](Status status, const hidl_vec<SecureStopId>& ids) {
                EXPECT_EQ(Status::OK, status);
                EXPECT_EQ(0u, ids.size());
            }).isOk();
    EXPECT_TRUE(ok);
}

/**
 * Test that there are secure stop ids after loading keys once
 */
TEST_F(DrmHalClearkeyTest, GetSecureStopIdsOnce) {
    auto stat = drmPlugin->removeAllSecureStops();
    EXPECT_OK(stat);

    auto sessionId = openSession();
    loadKeys(sessionId);
    closeSession(sessionId);

    auto res = drmPlugin->getSecureStopIds(
            [&](Status status, const hidl_vec<SecureStopId>& ids) {
                EXPECT_EQ(Status::OK, status);
                EXPECT_EQ(1u, ids.size());
            });
    EXPECT_OK(res);

    stat = drmPlugin->removeAllSecureStops();
    EXPECT_OK(stat);

    res = drmPlugin->getSecureStopIds(
            [&](Status status, const hidl_vec<SecureStopId>& ids) {
                EXPECT_EQ(Status::OK, status);
                EXPECT_EQ(0u, ids.size());
            });
    EXPECT_OK(res);
}

/**
 * Since getSecureStopIds only queries information there are no
 * negative cases.
 */

/**
 * Test that the clearkey plugin reports no secure stops when
 * there are none.
 */
TEST_F(DrmHalClearkeyTest, GetNoSecureStops) {
    auto stat = drmPlugin->removeAllSecureStops();
    EXPECT_OK(stat);

    auto res = drmPlugin->getSecureStops(
            [&](Status status, const hidl_vec<SecureStop>& stops) {
                EXPECT_EQ(Status::OK, status);
                EXPECT_EQ(0u, stops.size());
            });
    EXPECT_OK(res);
}

/**
 * Test get/remove of one secure stop
 */
TEST_F(DrmHalClearkeyTest, GetOneSecureStopAndRemoveIt) {
    auto stat = drmPlugin->removeAllSecureStops();
    EXPECT_OK(stat);

    auto sessionId = openSession();
    loadKeys(sessionId);
    closeSession(sessionId);

    auto res = drmPlugin->getSecureStops(
            [&](Status status, const hidl_vec<SecureStop>& stops) {
                EXPECT_EQ(Status::OK, status);
                EXPECT_EQ(1u, stops.size());
            });
    EXPECT_OK(res);

    stat = drmPlugin->removeAllSecureStops();
    EXPECT_OK(stat);

    res = drmPlugin->getSecureStops(
            [&](Status status, const hidl_vec<SecureStop>& stops) {
                EXPECT_EQ(Status::OK, status);
                EXPECT_EQ(0u, stops.size());
            });
    EXPECT_OK(res);
}

/**
 * Since getSecureStops only queries information there are no
 * negative cases.
 */

/**
 * Test that there are no secure stops after clearing them
 */
TEST_F(DrmHalClearkeyTest, GetSecureStopsCleared) {
    auto stat = drmPlugin->removeAllSecureStops();
    EXPECT_OK(stat);

    auto res = drmPlugin->getSecureStops(
            [&](Status status, const hidl_vec<SecureStop>& stops) {
                EXPECT_EQ(Status::OK, status);
                EXPECT_EQ(0u, stops.size());
            });
    EXPECT_OK(res);
}

/**
 * Test that there are secure stops after loading keys once
 */
TEST_F(DrmHalClearkeyTest, GetSecureStopsOnce) {
    auto stat = drmPlugin->removeAllSecureStops();
    EXPECT_OK(stat);

    auto sessionId = openSession();
    loadKeys(sessionId);
    closeSession(sessionId);

    auto res = drmPlugin->getSecureStops(
            [&](Status status, const hidl_vec<SecureStop>& stops) {
                EXPECT_EQ(Status::OK, status);
                EXPECT_EQ(1u, stops.size());
            });
    EXPECT_OK(res);

    stat = drmPlugin->removeAllSecureStops();
    EXPECT_OK(stat);

    res = drmPlugin->getSecureStops(
            [&](Status status, const hidl_vec<SecureStop>& stops) {
                EXPECT_EQ(Status::OK, status);
                EXPECT_EQ(0u, stops.size());
            });
    EXPECT_OK(res);
}

/**
 * Since getSecureStops only queries information there are no
 * negative cases.
 */

/**
 * Test that releasing a secure stop with empty
 * release message fails with the documented error
 */
TEST_F(DrmHalClearkeyTest, ReleaseEmptySecureStop) {
    SecureStopRelease emptyRelease = {.opaqueData = hidl_vec<uint8_t>()};
    Status status = drmPlugin->releaseSecureStops(emptyRelease);
    EXPECT_EQ(Status::BAD_VALUE, status);
}

/**
 * Helper function to create a secure release message for
 * a secure stop. The clearkey secure stop release format
 * is just a count followed by the secure stop opaque data.
 */
SecureStopRelease makeSecureRelease(const SecureStop &stop) {
    std::vector<uint8_t> stopData = stop.opaqueData;
    std::vector<uint8_t> buffer;
    std::string count = "0001";

    auto it = buffer.insert(buffer.begin(), count.begin(), count.end());
    buffer.insert(it + count.size(), stopData.begin(), stopData.end());
    SecureStopRelease release = { .opaqueData = hidl_vec<uint8_t>(buffer) };
    return release;
}

/**
 * Test that releasing one secure stop works
 */
TEST_F(DrmHalClearkeyTest, ReleaseOneSecureStop) {

    auto stat = drmPlugin->removeAllSecureStops();
    EXPECT_OK(stat);

    auto sessionId = openSession();
    loadKeys(sessionId);
    closeSession(sessionId);

    SecureStopRelease release;
    auto res = drmPlugin->getSecureStops(
            [&](Status status, const hidl_vec<SecureStop>& stops) {
                EXPECT_EQ(Status::OK, status);
                EXPECT_EQ(1u, stops.size());
                release = makeSecureRelease(stops[0]);
            });
    EXPECT_OK(res);

    stat = drmPlugin->releaseSecureStops(release);
    EXPECT_OK(stat);

    res = drmPlugin->getSecureStops(
            [&](Status status, const hidl_vec<SecureStop>& stops) {
                EXPECT_EQ(Status::OK, status);
                EXPECT_EQ(0u, stops.size());
            });
    EXPECT_OK(res);
}


/**
 * Test that removing a secure stop with an empty ID returns
 * documented error
 */
TEST_F(DrmHalClearkeyTest, RemoveEmptySecureStopId) {
    hidl_vec<uint8_t> emptyId;
    auto stat = drmPlugin->removeSecureStop(emptyId);
    EXPECT_OK(stat);
    EXPECT_EQ(Status::BAD_VALUE, stat);
}

/**
 * Test that removing a secure stop after it has already
 * been removed fails with the documented error code.
 */
TEST_F(DrmHalClearkeyTest, RemoveRemovedSecureStopId) {
    auto stat = drmPlugin->removeAllSecureStops();
    EXPECT_OK(stat);

    auto sessionId = openSession();
    loadKeys(sessionId);
    closeSession(sessionId);
    SecureStopId ssid;

    auto res = drmPlugin->getSecureStopIds(
            [&](Status status, const hidl_vec<SecureStopId>& ids) {
                EXPECT_EQ(Status::OK, status);
                EXPECT_EQ(1u, ids.size());
                ssid = ids[0];
            });
    EXPECT_OK(res);

    stat = drmPlugin->removeAllSecureStops();
    EXPECT_OK(stat);

    Status status = drmPlugin->removeSecureStop(ssid);
    EXPECT_EQ(Status::BAD_VALUE, status);
}

/**
 * Test that removing a secure stop by id works
 */
TEST_F(DrmHalClearkeyTest, RemoveSecureStopById) {
    auto stat = drmPlugin->removeAllSecureStops();
    EXPECT_OK(stat);

    auto sessionId = openSession();
    loadKeys(sessionId);
    closeSession(sessionId);
    SecureStopId ssid;

    auto res = drmPlugin->getSecureStopIds(
            [&](Status status, const hidl_vec<SecureStopId>& ids) {
                EXPECT_EQ(Status::OK, status);
                EXPECT_EQ(1u, ids.size());
                ssid = ids[0];
            });
    EXPECT_OK(res);

    stat = drmPlugin->removeSecureStop(ssid);
    EXPECT_OK(stat);

    res = drmPlugin->getSecureStopIds(
            [&](Status status, const hidl_vec<SecureStopId>& ids) {
                EXPECT_EQ(Status::OK, status);
                EXPECT_EQ(0u, ids.size());
            });
    EXPECT_OK(res);
}


int main(int argc, char** argv) {
    ::testing::AddGlobalTestEnvironment(DrmHidlEnvironment::Instance());
    ::testing::InitGoogleTest(&argc, argv);
    DrmHidlEnvironment::Instance()->init(&argc, argv);
    int status = RUN_ALL_TESTS();
    ALOGI("Test result = %d", status);
    return status;
}
