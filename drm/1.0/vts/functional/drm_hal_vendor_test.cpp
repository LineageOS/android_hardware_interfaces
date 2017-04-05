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

#define LOG_TAG "drm_hal_vendor_test@1.0"

#include <android-base/logging.h>
#include <android/hardware/drm/1.0/ICryptoFactory.h>
#include <android/hardware/drm/1.0/ICryptoPlugin.h>
#include <android/hardware/drm/1.0/IDrmFactory.h>
#include <android/hardware/drm/1.0/IDrmPlugin.h>
#include <android/hardware/drm/1.0/types.h>
#include <android/hidl/allocator/1.0/IAllocator.h>
#include <gtest/gtest.h>
#include <hidlmemory/mapping.h>
#include <memory>
#include <random>

#include "VtsHalHidlTargetTestBase.h"
#include "drm_hal_vendor_module_api.h"
#include "vendor_modules.h"

using ::android::hardware::drm::V1_0::BufferType;
using ::android::hardware::drm::V1_0::DestinationBuffer;
using ::android::hardware::drm::V1_0::ICryptoFactory;
using ::android::hardware::drm::V1_0::ICryptoPlugin;
using ::android::hardware::drm::V1_0::IDrmFactory;
using ::android::hardware::drm::V1_0::IDrmPlugin;
using ::android::hardware::drm::V1_0::KeyedVector;
using ::android::hardware::drm::V1_0::KeyValue;
using ::android::hardware::drm::V1_0::KeyRequestType;
using ::android::hardware::drm::V1_0::KeyType;
using ::android::hardware::drm::V1_0::Mode;
using ::android::hardware::drm::V1_0::Pattern;
using ::android::hardware::drm::V1_0::SecureStop;
using ::android::hardware::drm::V1_0::SecureStopId;
using ::android::hardware::drm::V1_0::SessionId;
using ::android::hardware::drm::V1_0::SharedBuffer;
using ::android::hardware::drm::V1_0::Status;
using ::android::hardware::drm::V1_0::SubSample;

using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
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

#define ASSERT_OK(ret) ASSERT_TRUE(ret.isOk())
#define EXPECT_OK(ret) EXPECT_TRUE(ret.isOk())

static const uint8_t kInvalidUUID[16] = {
        0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80,
        0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80,
};

static drm_vts::VendorModules* gVendorModules = nullptr;

class DrmHalVendorFactoryTest : public testing::TestWithParam<std::string> {
   public:
    DrmHalVendorFactoryTest()
        : vendorModule(gVendorModules ? static_cast<DrmHalVTSVendorModule_V1*>(
                                                gVendorModules->getVendorModule(
                                                        GetParam()))
                                      : nullptr) {}

    virtual ~DrmHalVendorFactoryTest() {}

    virtual void SetUp() {
        const ::testing::TestInfo* const test_info =
                ::testing::UnitTest::GetInstance()->current_test_info();
        ALOGD("Running test %s.%s from vendor module %s",
              test_info->test_case_name(), test_info->name(),
              GetParam().c_str());

        ASSERT_NE(vendorModule, nullptr);
        string name = vendorModule->getServiceName();
        drmFactory =
                ::testing::VtsHalHidlTargetTestBase::getService<IDrmFactory>(
                        name != "default" ? name : "drm");
        ASSERT_NE(drmFactory, nullptr);
        cryptoFactory =
                ::testing::VtsHalHidlTargetTestBase::getService<ICryptoFactory>(
                        name != "default" ? name : "crypto");
        ASSERT_NE(cryptoFactory, nullptr);
    }

    virtual void TearDown() override {}

   protected:
    hidl_array<uint8_t, 16> getVendorUUID() {
        vector<uint8_t> uuid = vendorModule->getUUID();
        return hidl_array<uint8_t, 16>(&uuid[0]);
    }

    sp<IDrmFactory> drmFactory;
    sp<ICryptoFactory> cryptoFactory;
    unique_ptr<DrmHalVTSVendorModule_V1> vendorModule;
};

/**
 * Ensure the factory supports its scheme UUID
 */
TEST_P(DrmHalVendorFactoryTest, VendorPluginSupported) {
    EXPECT_TRUE(drmFactory->isCryptoSchemeSupported(getVendorUUID()));
    EXPECT_TRUE(cryptoFactory->isCryptoSchemeSupported(getVendorUUID()));
}

/**
 * Ensure the factory doesn't support an invalid scheme UUID
 */
TEST_P(DrmHalVendorFactoryTest, InvalidPluginNotSupported) {
    EXPECT_FALSE(drmFactory->isCryptoSchemeSupported(kInvalidUUID));
    EXPECT_FALSE(cryptoFactory->isCryptoSchemeSupported(kInvalidUUID));
}

/**
 * Ensure vendor drm plugin can be created
 */
TEST_P(DrmHalVendorFactoryTest, CreateVendorDrmPlugin) {
    hidl_string packageName("android.hardware.drm.test");
    auto res = drmFactory->createPlugin(
            getVendorUUID(), packageName,
            [&](Status status, const sp<IDrmPlugin>& plugin) {
                EXPECT_EQ(Status::OK, status);
                EXPECT_NE(plugin, nullptr);
            });
    EXPECT_OK(res);
}

/**
 * Ensure vendor crypto plugin can be created
 */
TEST_P(DrmHalVendorFactoryTest, CreateVendorCryptoPlugin) {
    hidl_vec<uint8_t> initVec;
    auto res = cryptoFactory->createPlugin(
            getVendorUUID(), initVec,
            [&](Status status, const sp<ICryptoPlugin>& plugin) {
                EXPECT_EQ(Status::OK, status);
                EXPECT_NE(plugin, nullptr);
            });
    EXPECT_OK(res);
}

/**
 * Ensure invalid drm plugin can't be created
 */
TEST_P(DrmHalVendorFactoryTest, CreateInvalidDrmPlugin) {
    hidl_string packageName("android.hardware.drm.test");
    auto res = drmFactory->createPlugin(
            kInvalidUUID, packageName,
            [&](Status status, const sp<IDrmPlugin>& plugin) {
                EXPECT_EQ(Status::ERROR_DRM_CANNOT_HANDLE, status);
                EXPECT_EQ(plugin, nullptr);
            });
    EXPECT_OK(res);
}

/**
 * Ensure invalid crypto plugin can't be created
 */
TEST_P(DrmHalVendorFactoryTest, CreateInvalidCryptoPlugin) {
    hidl_vec<uint8_t> initVec;
    auto res = cryptoFactory->createPlugin(
            kInvalidUUID, initVec,
            [&](Status status, const sp<ICryptoPlugin>& plugin) {
                EXPECT_EQ(Status::ERROR_DRM_CANNOT_HANDLE, status);
                EXPECT_EQ(plugin, nullptr);
            });
    EXPECT_OK(res);
}

class DrmHalVendorPluginTest : public DrmHalVendorFactoryTest {
   public:
    virtual ~DrmHalVendorPluginTest() {}
    virtual void SetUp() override {
        // Create factories
        DrmHalVendorFactoryTest::SetUp();

        hidl_string packageName("android.hardware.drm.test");
        auto res = drmFactory->createPlugin(
                getVendorUUID(), packageName,
                [this](Status status, const sp<IDrmPlugin>& plugin) {
                    EXPECT_EQ(Status::OK, status);
                    ASSERT_NE(plugin, nullptr);
                    drmPlugin = plugin;
                });
        ASSERT_OK(res);

        hidl_vec<uint8_t> initVec;
        res = cryptoFactory->createPlugin(
                getVendorUUID(), initVec,
                [this](Status status, const sp<ICryptoPlugin>& plugin) {
                    EXPECT_EQ(Status::OK, status);
                    ASSERT_NE(plugin, nullptr);
                    cryptoPlugin = plugin;
                });
        ASSERT_OK(res);
    }

    virtual void TearDown() override {}

    SessionId openSession();
    void closeSession(const SessionId& sessionId);
    sp<IMemory> getDecryptMemory(size_t size, size_t index);

   protected:
    sp<IDrmPlugin> drmPlugin;
    sp<ICryptoPlugin> cryptoPlugin;
};

/**
 *  DrmPlugin tests
 */

/**
 * Test that a DRM plugin can handle provisioning.  While
 * it is not required that a DRM scheme require provisioning,
 * it should at least return appropriate status values. If
 * a provisioning request is returned, it is passed to the
 * vendor module which should provide a provisioning response
 * that is delivered back to the HAL.
 */

TEST_P(DrmHalVendorPluginTest, DoProvisioning) {
    hidl_string certificateType;
    hidl_string certificateAuthority;
    hidl_vec<uint8_t> provisionRequest;
    hidl_string defaultUrl;
    auto res = drmPlugin->getProvisionRequest(
            certificateType, certificateAuthority,
            [&](Status status, const hidl_vec<uint8_t>& request,
                const hidl_string& url) {
                if (status == Status::OK) {
                    EXPECT_NE(request.size(), 0u);
                    provisionRequest = request;
                    defaultUrl = url;
                } else if (status == Status::ERROR_DRM_CANNOT_HANDLE) {
                    EXPECT_EQ(0u, request.size());
                }
            });
    EXPECT_OK(res);

    if (provisionRequest.size() > 0) {
        vector<uint8_t> response = vendorModule->handleProvisioningRequest(
                provisionRequest, defaultUrl);
        ASSERT_NE(0u, response.size());

        auto res = drmPlugin->provideProvisionResponse(
                response, [&](Status status, const hidl_vec<uint8_t>&,
                              const hidl_vec<uint8_t>&) {
                    EXPECT_EQ(Status::OK, status);
                });
        EXPECT_OK(res);
    }
}

/**
 * The DRM HAL should return BAD_VALUE if an empty provisioning
 * response is provided.
 */
TEST_P(DrmHalVendorPluginTest, ProvideEmptyProvisionResponse) {
    hidl_vec<uint8_t> response;
    auto res = drmPlugin->provideProvisionResponse(
            response, [&](Status status, const hidl_vec<uint8_t>&,
                          const hidl_vec<uint8_t>&) {
                EXPECT_EQ(Status::BAD_VALUE, status);
            });
    EXPECT_OK(res);
}

/**
 * Helper method to open a session and verify that a non-empty
 * session ID is returned
 */
SessionId DrmHalVendorPluginTest::openSession() {
    SessionId sessionId;

    auto res = drmPlugin->openSession([&](Status status, const SessionId& id) {
        EXPECT_EQ(Status::OK, status);
        EXPECT_NE(id.size(), 0u);
        sessionId = id;
    });
    EXPECT_OK(res);
    return sessionId;
}

/**
 * Helper method to close a session
 */
void DrmHalVendorPluginTest::closeSession(const SessionId& sessionId) {
    Status status = drmPlugin->closeSession(sessionId);
    EXPECT_EQ(Status::OK, status);
}

/**
 * Test that a session can be opened and closed
 */
TEST_P(DrmHalVendorPluginTest, OpenCloseSession) {
    auto sessionId = openSession();
    closeSession(sessionId);
}

/**
 * Test that attempting to close an invalid (empty) sessionId
 * is prohibited with the documented error code.
 */
TEST_P(DrmHalVendorPluginTest, CloseInvalidSession) {
    SessionId invalidSessionId;
    Status status = drmPlugin->closeSession(invalidSessionId);
    EXPECT_EQ(Status::BAD_VALUE, status);
}

/**
 * Test that attempting to close a valid session twice
 * is prohibited with the documented error code.
 */
TEST_P(DrmHalVendorPluginTest, CloseClosedSession) {
    auto sessionId = openSession();
    closeSession(sessionId);
    Status status = drmPlugin->closeSession(sessionId);
    EXPECT_EQ(Status::ERROR_DRM_SESSION_NOT_OPENED, status);
}

/**
 * A get key request should fail if no sessionId is provided
 */
TEST_P(DrmHalVendorPluginTest, GetKeyRequestNoSession) {
    SessionId invalidSessionId;
    hidl_vec<uint8_t> initData;
    hidl_string mimeType = "video/mp4";
    KeyedVector optionalParameters;
    auto res = drmPlugin->getKeyRequest(
            invalidSessionId, initData, mimeType, KeyType::STREAMING,
            optionalParameters,
            [&](Status status, const hidl_vec<uint8_t>&, KeyRequestType,
                const hidl_string&) { EXPECT_EQ(Status::BAD_VALUE, status); });
    EXPECT_OK(res);
}

/**
 * Test that an empty sessionID returns BAD_VALUE
 */
TEST_P(DrmHalVendorPluginTest, ProvideKeyResponseEmptySessionId) {
    SessionId session;

    hidl_vec<uint8_t> keyResponse = {0x7b, 0x22, 0x6b, 0x65,
                                     0x79, 0x73, 0x22, 0x3a};
    auto res = drmPlugin->provideKeyResponse(
            session, keyResponse,
            [&](Status status, const hidl_vec<uint8_t>& keySetId) {
                EXPECT_EQ(Status::BAD_VALUE, status);
                EXPECT_EQ(keySetId.size(), 0u);
            });
    EXPECT_OK(res);
}

/**
 * Test that an empty key response returns BAD_VALUE
 */
TEST_P(DrmHalVendorPluginTest, ProvideKeyResponseEmptyResponse) {
    SessionId session = openSession();
    hidl_vec<uint8_t> emptyResponse;
    auto res = drmPlugin->provideKeyResponse(
            session, emptyResponse,
            [&](Status status, const hidl_vec<uint8_t>& keySetId) {
                EXPECT_EQ(Status::BAD_VALUE, status);
                EXPECT_EQ(keySetId.size(), 0u);
            });
    EXPECT_OK(res);
    closeSession(session);
}

/**
 * Test that the plugin either doesn't support getting
 * secure stops, or has no secure stops available after
 * clearing them.
 */
TEST_P(DrmHalVendorPluginTest, GetSecureStops) {
    // There may be secure stops, depending on if there were keys
    // loaded and unloaded previously. Clear them to get to a known
    // state, then make sure there are none.
    auto res = drmPlugin->getSecureStops(
            [&](Status status, const hidl_vec<SecureStop>&) {
                if (status != Status::OK) {
                    EXPECT_EQ(Status::ERROR_DRM_CANNOT_HANDLE, status);
                }
            });
    EXPECT_OK(res);

    res = drmPlugin->getSecureStops(
            [&](Status status, const hidl_vec<SecureStop>& secureStops) {
                if (status == Status::OK) {
                    EXPECT_EQ(secureStops.size(), 0u);
                } else {
                    EXPECT_EQ(Status::ERROR_DRM_CANNOT_HANDLE, status);
                }
            });
    EXPECT_OK(res);
}

/**
 * Test that the clearkey plugin returns BAD_VALUE if
 * an empty ssid is provided.
 */
TEST_P(DrmHalVendorPluginTest, GetSecureStopEmptySSID) {
    SecureStopId ssid;
    auto res = drmPlugin->getSecureStop(
            ssid, [&](Status status, const SecureStop&) {
                EXPECT_EQ(Status::BAD_VALUE, status);
            });
    EXPECT_OK(res);
}

/**
 * Test that releasing all secure stops either isn't supported
 * or is completed successfully
 */
TEST_P(DrmHalVendorPluginTest, ReleaseAllSecureStops) {
    Status status = drmPlugin->releaseAllSecureStops();
    EXPECT_TRUE(status == Status::OK ||
                status == Status::ERROR_DRM_CANNOT_HANDLE);
}

/**
 * Releasing a secure stop without first getting one and sending it to the
 * server to get a valid SSID should return ERROR_DRM_INVALID_STATE.
 * This is an optional API so it can also return CANNOT_HANDLE.
 */
TEST_P(DrmHalVendorPluginTest, ReleaseSecureStopSequenceError) {
    SecureStopId ssid = {1, 2, 3, 4};
    Status status = drmPlugin->releaseSecureStop(ssid);
    EXPECT_TRUE(status == Status::ERROR_DRM_INVALID_STATE ||
                status == Status::ERROR_DRM_CANNOT_HANDLE);
}

/**
 * Test that releasing a specific secure stop with an empty ssid
 * return BAD_VALUE. This is an optional API so it can also return
 * CANNOT_HANDLE.
 */
TEST_P(DrmHalVendorPluginTest, ReleaseSecureStopEmptySSID) {
    SecureStopId ssid;
    Status status = drmPlugin->releaseSecureStop(ssid);
    EXPECT_TRUE(status == Status::BAD_VALUE ||
                status == Status::ERROR_DRM_CANNOT_HANDLE);
}

/**
 * The following five tests verify that the properties
 * defined in the MediaDrm API are supported by
 * the plugin.
 */
TEST_P(DrmHalVendorPluginTest, GetVendorProperty) {
    auto res = drmPlugin->getPropertyString(
            "vendor", [&](Status status, const hidl_string& value) {
                EXPECT_EQ(Status::OK, status);
                EXPECT_NE(value.size(), 0u);
            });
    EXPECT_OK(res);
}

TEST_P(DrmHalVendorPluginTest, GetVersionProperty) {
    auto res = drmPlugin->getPropertyString(
            "version", [&](Status status, const hidl_string& value) {
                EXPECT_EQ(Status::OK, status);
                EXPECT_NE(value.size(), 0u);
            });
    EXPECT_OK(res);
}

TEST_P(DrmHalVendorPluginTest, GetDescriptionProperty) {
    auto res = drmPlugin->getPropertyString(
            "description", [&](Status status, const hidl_string& value) {
                EXPECT_EQ(Status::OK, status);
                EXPECT_NE(value.size(), 0u);
            });
    EXPECT_OK(res);
}

TEST_P(DrmHalVendorPluginTest, GetAlgorithmsProperty) {
    auto res = drmPlugin->getPropertyString(
            "algorithms", [&](Status status, const hidl_string& value) {
                if (status == Status::OK) {
                    EXPECT_NE(value.size(), 0u);
                } else {
                    EXPECT_EQ(Status::ERROR_DRM_CANNOT_HANDLE, status);
                }
            });
    EXPECT_OK(res);
}

TEST_P(DrmHalVendorPluginTest, GetPropertyUniqueDeviceID) {
    auto res = drmPlugin->getPropertyByteArray(
            "deviceUniqueId",
            [&](Status status, const hidl_vec<uint8_t>& value) {
                if (status == Status::OK) {
                    EXPECT_NE(value.size(), 0u);
                } else {
                    EXPECT_EQ(Status::ERROR_DRM_CANNOT_HANDLE, status);
                }
            });
    EXPECT_OK(res);
}

/**
 * Test that attempting to read invalid string and byte array
 * properties returns the documented error code.
 */
TEST_P(DrmHalVendorPluginTest, GetInvalidStringProperty) {
    auto res = drmPlugin->getPropertyString(
            "invalid", [&](Status status, const hidl_string&) {
                EXPECT_EQ(Status::ERROR_DRM_CANNOT_HANDLE, status);
            });
    EXPECT_OK(res);
}

TEST_P(DrmHalVendorPluginTest, GetInvalidByteArrayProperty) {
    auto res = drmPlugin->getPropertyByteArray(
            "invalid", [&](Status status, const hidl_vec<uint8_t>&) {
                EXPECT_EQ(Status::ERROR_DRM_CANNOT_HANDLE, status);
            });
    EXPECT_OK(res);
}

/**
 * Test that setting invalid string and byte array properties returns
 * the expected status value.
 */
TEST_P(DrmHalVendorPluginTest, SetStringPropertyNotSupported) {
    EXPECT_EQ(drmPlugin->setPropertyString("awefijaeflijwef", "value"),
              Status::ERROR_DRM_CANNOT_HANDLE);
}

TEST_P(DrmHalVendorPluginTest, SetByteArrayPropertyNotSupported) {
    hidl_vec<uint8_t> value;
    EXPECT_EQ(drmPlugin->setPropertyByteArray("awefijaeflijwef", value),
              Status::ERROR_DRM_CANNOT_HANDLE);
}

/**
 * Test that setting an invalid cipher algorithm returns
 * the expected status value.
 */
TEST_P(DrmHalVendorPluginTest, SetCipherInvalidAlgorithm) {
    SessionId session = openSession();
    hidl_string algorithm;
    Status status = drmPlugin->setCipherAlgorithm(session, algorithm);
    EXPECT_EQ(Status::BAD_VALUE, status);
    closeSession(session);
}

/**
 * Test that setting a cipher algorithm with no session returns
 * the expected status value.
 */
TEST_P(DrmHalVendorPluginTest, SetCipherAlgorithmNoSession) {
    SessionId session;
    hidl_string algorithm = "AES/CBC/NoPadding";
    Status status = drmPlugin->setCipherAlgorithm(session, algorithm);
    EXPECT_EQ(Status::BAD_VALUE, status);
}

/**
 * Test that setting a valid cipher algorithm returns
 * the expected status value. It is not required that all
 * vendor modules support this algorithm, but they must
 * either accept it or return ERROR_DRM_CANNOT_HANDLE
 */
TEST_P(DrmHalVendorPluginTest, SetCipherAlgorithm) {
    SessionId session = openSession();
    ;
    hidl_string algorithm = "AES/CBC/NoPadding";
    Status status = drmPlugin->setCipherAlgorithm(session, algorithm);
    EXPECT_TRUE(status == Status::OK ||
                status == Status::ERROR_DRM_CANNOT_HANDLE);
    closeSession(session);
}

/**
 * Test that setting an invalid mac algorithm returns
 * the expected status value.
 */
TEST_P(DrmHalVendorPluginTest, SetMacInvalidAlgorithm) {
    SessionId session = openSession();
    hidl_string algorithm;
    Status status = drmPlugin->setMacAlgorithm(session, algorithm);
    EXPECT_EQ(Status::BAD_VALUE, status);
    closeSession(session);
}

/**
 * Test that setting a mac algorithm with no session returns
 * the expected status value.
 */
TEST_P(DrmHalVendorPluginTest, SetMacNullAlgorithmNoSession) {
    SessionId session;
    hidl_string algorithm = "HmacSHA256";
    Status status = drmPlugin->setMacAlgorithm(session, algorithm);
    EXPECT_EQ(Status::BAD_VALUE, status);
}

/**
 * Test that setting a valid mac algorithm returns
 * the expected status value. It is not required that all
 * vendor modules support this algorithm, but they must
 * either accept it or return ERROR_DRM_CANNOT_HANDLE
 */
TEST_P(DrmHalVendorPluginTest, SetMacAlgorithm) {
    SessionId session = openSession();
    hidl_string algorithm = "HmacSHA256";
    Status status = drmPlugin->setMacAlgorithm(session, algorithm);
    EXPECT_TRUE(status == Status::OK ||
                status == Status::ERROR_DRM_CANNOT_HANDLE);
    closeSession(session);
}

/**
 * The Generic* methods provide general purpose crypto operations
 * that may be used for applications other than DRM. They leverage
 * the hardware root of trust and secure key distribution mechanisms
 * of a DRM system to enable app-specific crypto functionality where
 * the crypto keys are not exposed outside of the trusted execution
 * environment.
 *
 * Generic encrypt/decrypt/sign/verify should fail on invalid
 * inputs, e.g. empty sessionId
 */
TEST_P(DrmHalVendorPluginTest, GenericEncryptNoSession) {
    SessionId session;
    hidl_vec<uint8_t> keyId, input, iv;
    auto res = drmPlugin->encrypt(
            session, keyId, input, iv,
            [&](Status status, const hidl_vec<uint8_t>&) {
                EXPECT_EQ(Status::ERROR_DRM_SESSION_NOT_OPENED, status);
            });
    EXPECT_OK(res);
}

TEST_P(DrmHalVendorPluginTest, GenericDecryptNoSession) {
    SessionId session;
    hidl_vec<uint8_t> keyId, input, iv;
    auto res = drmPlugin->decrypt(
            session, keyId, input, iv,
            [&](Status status, const hidl_vec<uint8_t>&) {
                EXPECT_EQ(Status::ERROR_DRM_SESSION_NOT_OPENED, status);
            });
    EXPECT_OK(res);
}

TEST_P(DrmHalVendorPluginTest, GenericSignNoSession) {
    SessionId session;
    hidl_vec<uint8_t> keyId, message;
    auto res = drmPlugin->sign(
            session, keyId, message,
            [&](Status status, const hidl_vec<uint8_t>&) {
                EXPECT_EQ(Status::ERROR_DRM_SESSION_NOT_OPENED, status);
            });
    EXPECT_OK(res);
}

TEST_P(DrmHalVendorPluginTest, GenericVerifyNoSession) {
    SessionId session;
    hidl_vec<uint8_t> keyId, message, signature;
    auto res = drmPlugin->verify(
            session, keyId, message, signature, [&](Status status, bool) {
                EXPECT_EQ(Status::ERROR_DRM_SESSION_NOT_OPENED, status);
            });
    EXPECT_OK(res);
}

TEST_P(DrmHalVendorPluginTest, GenericSignRSANoSession) {
    SessionId session;
    hidl_string algorithm;
    hidl_vec<uint8_t> message, wrappedKey;
    auto res = drmPlugin->signRSA(session, algorithm, message, wrappedKey,
                                  [&](Status status, const hidl_vec<uint8_t>&) {
                                      EXPECT_EQ(Status::BAD_VALUE, status);
                                  });
    EXPECT_OK(res);
}

/**
 * Exercise the requiresSecureDecoderComponent method. Additional tests
 * will verify positive cases with specific vendor content configurations.
 * Below we just test the negative cases.
 */

/**
 * Verify that requiresSecureDecoderComponent handles empty mimetype.
 */
TEST_P(DrmHalVendorPluginTest, RequiresSecureDecoderEmptyMimeType) {
    EXPECT_FALSE(cryptoPlugin->requiresSecureDecoderComponent(""));
}

/**
 * Verify that requiresSecureDecoderComponent handles invalid mimetype.
 */
TEST_P(DrmHalVendorPluginTest, RequiresSecureDecoderInvalidMimeType) {
    EXPECT_FALSE(cryptoPlugin->requiresSecureDecoderComponent("bad"));
}

/**
 *  CryptoPlugin tests
 */

/**
 * Exercise the NotifyResolution API. There is no observable result,
 * just call the method for coverage.
 */
TEST_P(DrmHalVendorPluginTest, NotifyResolution) {
    cryptoPlugin->notifyResolution(1920, 1080);
}

/**
 * getDecryptMemory allocates memory for decryption, then sets it
 * as a shared buffer base in the crypto hal.  The allocated and
 * mapped IMemory is returned.
 *
 * @param size the size of the memory segment to allocate
 * @param the index of the memory segment which will be used
 * to refer to it for decryption.
 */
sp<IMemory> DrmHalVendorPluginTest::getDecryptMemory(size_t size,
                                                     size_t index) {
    sp<IAllocator> ashmemAllocator = IAllocator::getService("ashmem");
    EXPECT_NE(ashmemAllocator, nullptr);

    hidl_memory hidlMemory;
    auto res = ashmemAllocator->allocate(
            size, [&](bool success, const hidl_memory& memory) {
                EXPECT_EQ(success, true);
                EXPECT_EQ(memory.size(), size);
                hidlMemory = memory;
            });

    EXPECT_OK(res);

    sp<IMemory> mappedMemory = mapMemory(hidlMemory);
    EXPECT_NE(mappedMemory, nullptr);
    res = cryptoPlugin->setSharedBufferBase(hidlMemory, index);
    EXPECT_OK(res);
    return mappedMemory;
}

/**
 * Exercise the setMediaDrmSession method. setMediaDrmSession
 * is used to associate a drm session with a crypto session.
 */
TEST_P(DrmHalVendorPluginTest, SetMediaDrmSession) {
    auto sessionId = openSession();
    Status status = cryptoPlugin->setMediaDrmSession(sessionId);
    EXPECT_EQ(Status::OK, status);
    closeSession(sessionId);
}

/**
 * setMediaDrmSession with a closed session id
 */
TEST_P(DrmHalVendorPluginTest, SetMediaDrmSessionClosedSession) {
    auto sessionId = openSession();
    closeSession(sessionId);
    Status status = cryptoPlugin->setMediaDrmSession(sessionId);
    EXPECT_EQ(Status::ERROR_DRM_SESSION_NOT_OPENED, status);
}

/**
 * Decrypt tests
 */

class DrmHalVendorDecryptTest : public DrmHalVendorPluginTest {
   public:
    DrmHalVendorDecryptTest() = default;
    virtual ~DrmHalVendorDecryptTest() {}

   protected:
    void loadKeys(const SessionId& sessionId,
                  const DrmHalVTSVendorModule_V1::ContentConfiguration&
                          configuration);
    void fillRandom(const sp<IMemory>& memory);
    KeyedVector toHidlKeyedVector(const map<string, string>& params);
    hidl_array<uint8_t, 16> toHidlArray(const vector<uint8_t>& vec) {
        EXPECT_EQ(vec.size(), 16u);
        return hidl_array<uint8_t, 16>(&vec[0]);
    }
};

KeyedVector DrmHalVendorDecryptTest::toHidlKeyedVector(
        const map<string, string>& params) {
    std::vector<KeyValue> stdKeyedVector;
    for (auto it = params.begin(); it != params.end(); ++it) {
        KeyValue keyValue;
        keyValue.key = it->first;
        keyValue.value = it->second;
        stdKeyedVector.push_back(keyValue);
    }
    return KeyedVector(stdKeyedVector);
}

/**
 * Helper method to load keys for subsequent decrypt tests.
 * These tests use predetermined key request/response to
 * avoid requiring a round trip to a license server.
 */
void DrmHalVendorDecryptTest::loadKeys(
        const SessionId& sessionId,
        const DrmHalVTSVendorModule_V1::ContentConfiguration& configuration) {
    hidl_vec<uint8_t> keyRequest;
    auto res = drmPlugin->getKeyRequest(
            sessionId, configuration.initData, configuration.mimeType,
            KeyType::STREAMING,
            toHidlKeyedVector(configuration.optionalParameters),
            [&](Status status, const hidl_vec<uint8_t>& request,
                KeyRequestType type, const hidl_string&) {
                EXPECT_EQ(Status::OK, status)
                        << "Failed to get "
                           "key request for configuration "
                        << configuration.name;
                EXPECT_EQ(type, KeyRequestType::INITIAL);
                EXPECT_NE(request.size(), 0u) << "Expected key request size"
                                                 " to have length > 0 bytes";
                keyRequest = request;
            });
    EXPECT_OK(res);

    /**
     * Get key response from vendor module
     */
    hidl_vec<uint8_t> keyResponse =
            vendorModule->handleKeyRequest(keyRequest, configuration.serverUrl);

    EXPECT_NE(keyResponse.size(), 0u) << "Expected key response size "
                                         "to have length > 0 bytes";

    res = drmPlugin->provideKeyResponse(
            sessionId, keyResponse,
            [&](Status status, const hidl_vec<uint8_t>&) {
                EXPECT_EQ(Status::OK, status)
                        << "Failure providing "
                           "key response for configuration "
                        << configuration.name;
            });
    EXPECT_OK(res);
}

void DrmHalVendorDecryptTest::fillRandom(const sp<IMemory>& memory) {
    random_device rd;
    mt19937 rand(rd());
    for (size_t i = 0; i < memory->getSize() / sizeof(uint32_t); i++) {
        auto p = static_cast<uint32_t*>(
                static_cast<void*>(memory->getPointer()));
        p[i] = rand();
    }
}

TEST_P(DrmHalVendorDecryptTest, ValidateConfigurations) {
    vector<DrmHalVTSVendorModule_V1::ContentConfiguration> configurations =
            vendorModule->getContentConfigurations();
    const char* kVendorStr = "Vendor module ";
    for (auto config : configurations) {
        ASSERT_TRUE(config.name.size() > 0) << kVendorStr << "has no name";
        ASSERT_TRUE(config.serverUrl.size() > 0) << kVendorStr
                                                 << "has no serverUrl";
        ASSERT_TRUE(config.initData.size() > 0) << kVendorStr
                                                << "has no init data";
        ASSERT_TRUE(config.mimeType.size() > 0) << kVendorStr
                                                << "has no mime type";
        ASSERT_TRUE(config.keys.size() >= 1) << kVendorStr << "has no keys";
        for (auto key : config.keys) {
            ASSERT_TRUE(key.keyId.size() > 0) << kVendorStr
                                              << " has zero length keyId";
            ASSERT_TRUE(key.keyId.size() > 0) << kVendorStr
                                              << " has zero length key value";
        }
    }
}

/**
 * Positive decrypt test.  "Decrypt" a single clear
 * segment.  Verify data matches.
 */
TEST_P(DrmHalVendorDecryptTest, ClearSegmentTest) {
    vector<DrmHalVTSVendorModule_V1::ContentConfiguration> configurations =
            vendorModule->getContentConfigurations();
    for (auto config : configurations) {
        const size_t kSegmentSize = 1024;
        const size_t kSegmentIndex = 0;
        uint8_t iv[16] = {0};

        sp<IMemory> sharedMemory =
                getDecryptMemory(kSegmentSize * 2, kSegmentIndex);

        SharedBuffer sourceBuffer = {
                .bufferId = kSegmentIndex, .offset = 0, .size = kSegmentSize};
        fillRandom(sharedMemory);

        DestinationBuffer destBuffer = {.type = BufferType::SHARED_MEMORY,
                                        {.bufferId = kSegmentIndex,
                                         .offset = kSegmentSize,
                                         .size = kSegmentSize},
                                        .secureMemory = nullptr};

        Pattern noPattern = {0, 0};
        vector<SubSample> subSamples = {{.numBytesOfClearData = kSegmentSize,
                                         .numBytesOfEncryptedData = 0}};
        uint64_t offset = 0;

        auto sessionId = openSession();
        loadKeys(sessionId, config);

        Status status = cryptoPlugin->setMediaDrmSession(sessionId);
        EXPECT_EQ(Status::OK, status);

        const bool kNotSecure = false;
        auto res = cryptoPlugin->decrypt(
                kNotSecure, toHidlArray(config.keys[0].keyId), iv,
                Mode::UNENCRYPTED, noPattern, subSamples, sourceBuffer, offset,
                destBuffer, [&](Status status, uint32_t bytesWritten,
                                string detailedError) {
                    EXPECT_EQ(Status::OK, status) << "Failure in decryption "
                                                     "for configuration "
                                                  << config.name << ": "
                                                  << detailedError;
                    EXPECT_EQ(bytesWritten, kSegmentSize);
                });
        EXPECT_OK(res);
        uint8_t* base = static_cast<uint8_t*>(
                static_cast<void*>(sharedMemory->getPointer()));

        EXPECT_EQ(0,
                  memcmp(static_cast<void*>(base),
                         static_cast<void*>(base + kSegmentSize), kSegmentSize))
                << "decrypt data mismatch";
        closeSession(sessionId);
    }
}

/**
 * Instantiate the set of test cases for each vendor module
 */

INSTANTIATE_TEST_CASE_P(
        DrmHalVendorFactoryTestCases, DrmHalVendorFactoryTest,
        testing::ValuesIn(gVendorModules->getVendorModulePaths()));

INSTANTIATE_TEST_CASE_P(
        DrmHalVendorPluginTestCases, DrmHalVendorPluginTest,
        testing::ValuesIn(gVendorModules->getVendorModulePaths()));

INSTANTIATE_TEST_CASE_P(
        DrmHalVendorDecryptTestCases, DrmHalVendorDecryptTest,
        testing::ValuesIn(gVendorModules->getVendorModulePaths()));

int main(int argc, char** argv) {
#if defined(__LP64__)
    const char *kModulePath = "/data/local/tmp/64/lib";
#else
    const char *kModulePath = "/data/local/tmp/32/lib";
#endif
    gVendorModules = new drm_vts::VendorModules(kModulePath);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
