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

#include "vendor_modules.h"
#define LOG_TAG "drm_hal_common@1.2"

#include <android/hidl/allocator/1.0/IAllocator.h>
#include <gtest/gtest.h>
#include <hidl/HidlSupport.h>
#include <hidlmemory/mapping.h>
#include <log/log.h>
#include <openssl/aes.h>
#include <random>

#include "drm_hal_clearkey_module.h"
#include "drm_hal_common.h"

using ::android::hardware::drm::V1_0::BufferType;
using ::android::hardware::drm::V1_0::DestinationBuffer;
using ICryptoPluginV1_0 = ::android::hardware::drm::V1_0::ICryptoPlugin;
using IDrmPluginV1_0 = ::android::hardware::drm::V1_0::IDrmPlugin;
using ::android::hardware::drm::V1_0::KeyValue;
using ::android::hardware::drm::V1_0::SharedBuffer;
using StatusV1_0 = ::android::hardware::drm::V1_0::Status;

using ::android::hardware::drm::V1_1::KeyRequestType;

using ::android::hardware::drm::V1_2::KeySetId;
using ::android::hardware::drm::V1_2::OfflineLicenseState;
using StatusV1_2 = ::android::hardware::drm::V1_2::Status;

using ::android::hardware::hidl_string;
using ::android::hardware::hidl_memory;

using ::android::hidl::allocator::V1_0::IAllocator;

using std::random_device;
using std::mt19937;

namespace android {
namespace hardware {
namespace drm {
namespace V1_2 {
namespace vts {

const char *kCallbackLostState = "LostState";
const char *kCallbackKeysChange = "KeysChange";

drm_vts::VendorModules *DrmHalTest::gVendorModules = nullptr;

/**
 * DrmHalPluginListener
 */

Return<void> DrmHalPluginListener::sendSessionLostState(const hidl_vec<uint8_t>& sessionId) {
    ListenerEventArgs args;
    args.sessionId = sessionId;
    NotifyFromCallback(kCallbackLostState, args);
    return Void();
}

Return<void> DrmHalPluginListener::sendKeysChange_1_2(const hidl_vec<uint8_t>& sessionId,
        const hidl_vec<KeyStatus>& keyStatusList, bool hasNewUsableKey) {
    ListenerEventArgs args;
    args.sessionId = sessionId;
    args.keyStatusList = keyStatusList;
    args.hasNewUsableKey = hasNewUsableKey;
    NotifyFromCallback(kCallbackKeysChange, args);
    return Void();
}

static DrmHalVTSVendorModule_V1* getModuleForInstance(const std::string& instance) {
    if (instance == "clearkey" || instance == "default") {
        return new DrmHalVTSClearkeyModule();
    }

    return static_cast<DrmHalVTSVendorModule_V1*>(DrmHalTest::gVendorModules->getModule(instance));
}

/**
 * DrmHalTest
 */

DrmHalTest::DrmHalTest() : vendorModule(getModuleForInstance(GetParam())) {}

void DrmHalTest::SetUp() {
    const ::testing::TestInfo* const test_info =
            ::testing::UnitTest::GetInstance()->current_test_info();

    ALOGD("Running test %s.%s from (vendor) module %s",
          test_info->test_case_name(), test_info->name(),
          GetParam().c_str());

    const string instance = GetParam();

    drmFactory = IDrmFactory::getService(instance);
    ASSERT_NE(drmFactory, nullptr);
    drmPlugin = createDrmPlugin();

    cryptoFactory = ICryptoFactory::getService(instance);
    ASSERT_NE(cryptoFactory, nullptr);
    cryptoPlugin = createCryptoPlugin();

    if (!vendorModule) {
        ASSERT_NE(instance, "widevine") << "Widevine requires vendor module.";
        ASSERT_NE(instance, "clearkey") << "Clearkey requires vendor module.";
        GTEST_SKIP() << "No vendor module installed";
    }

    if (instance == "clearkey") {
        // TODO(b/147449315)
        // Only the clearkey plugged into the "default" instance supports
        // this test. Currently the "clearkey" instance fails some tests
        // here.
        GTEST_SKIP() << "Clearkey tests don't work with 'clearkey' instance yet.";
    }

    ASSERT_EQ(instance, vendorModule->getServiceName());
    contentConfigurations = vendorModule->getContentConfigurations();

    // If drm scheme not installed skip subsequent tests
    if (drmFactory.get() == nullptr || !drmFactory->isCryptoSchemeSupported(getVendorUUID())) {
        vendorModule->setInstalled(false);
        return;
    }

    ASSERT_NE(nullptr, drmPlugin.get()) << "Can't find " << vendorModule->getServiceName() <<  " drm@1.2 plugin";
    ASSERT_NE(nullptr, cryptoPlugin.get()) << "Can't find " << vendorModule->getServiceName() <<  " crypto@1.2 plugin";

}

sp<IDrmPlugin> DrmHalTest::createDrmPlugin() {
    if (drmFactory == nullptr) {
        return nullptr;
    }
    sp<IDrmPlugin> plugin = nullptr;
    hidl_string packageName("android.hardware.drm.test");
    auto res =
            drmFactory->createPlugin(getVendorUUID(), packageName,
                                     [&](StatusV1_0 status, const sp<IDrmPluginV1_0>& pluginV1_0) {
                                         EXPECT_EQ(StatusV1_0::OK == status, pluginV1_0 != nullptr);
                                         plugin = IDrmPlugin::castFrom(pluginV1_0);
                                     });

    if (!res.isOk()) {
        ALOGE("createDrmPlugin remote call failed");
    }
    return plugin;
}

sp<ICryptoPlugin> DrmHalTest::createCryptoPlugin() {
    if (cryptoFactory == nullptr) {
        return nullptr;
    }
    sp<ICryptoPlugin> plugin = nullptr;
    hidl_vec<uint8_t> initVec;
    auto res = cryptoFactory->createPlugin(
            getVendorUUID(), initVec,
            [&](StatusV1_0 status, const sp<ICryptoPluginV1_0>& pluginV1_0) {
                EXPECT_EQ(StatusV1_0::OK == status, pluginV1_0 != nullptr);
                plugin = ICryptoPlugin::castFrom(pluginV1_0);
            });
    if (!res.isOk()) {
        ALOGE("createCryptoPlugin remote call failed");
    }
    return plugin;
}

hidl_array<uint8_t, 16> DrmHalTest::getVendorUUID() {
    if (vendorModule == nullptr) return {};
    vector<uint8_t> uuid = vendorModule->getUUID();
    return hidl_array<uint8_t, 16>(&uuid[0]);
}

/**
 * Helper method to open a session and verify that a non-empty
 * session ID is returned
 */
SessionId DrmHalTest::openSession() {
    SessionId sessionId;

    auto res = drmPlugin->openSession([&](StatusV1_0 status, const hidl_vec<unsigned char> &id) {
        EXPECT_EQ(StatusV1_0::OK, status);
        EXPECT_NE(id.size(), 0u);
        sessionId = id;
    });
    EXPECT_OK(res);
    return sessionId;
}

/**
 * Helper method to close a session
 */
void DrmHalTest::closeSession(const SessionId& sessionId) {
    StatusV1_0 status = drmPlugin->closeSession(sessionId);
    EXPECT_EQ(StatusV1_0::OK, status);
}

hidl_vec<uint8_t> DrmHalTest::getKeyRequest(
    const SessionId& sessionId,
    const DrmHalVTSVendorModule_V1::ContentConfiguration& configuration,
    const KeyType& type = KeyType::STREAMING) {
    hidl_vec<uint8_t> keyRequest;
    auto res = drmPlugin->getKeyRequest_1_2(
        sessionId, configuration.initData, configuration.mimeType, type,
        toHidlKeyedVector(configuration.optionalParameters),
        [&](Status status, const hidl_vec<uint8_t>& request,
            KeyRequestType requestType, const hidl_string&) {
            EXPECT_EQ(Status::OK, status) << "Failed to get "
                                             "key request for configuration "
                                          << configuration.name;
            if (type == KeyType::RELEASE) {
                EXPECT_EQ(KeyRequestType::RELEASE, requestType);
            } else {
                EXPECT_EQ(KeyRequestType::INITIAL, requestType);
            }
            EXPECT_NE(request.size(), 0u) << "Expected key request size"
                                             " to have length > 0 bytes";
            keyRequest = request;
        });
    EXPECT_OK(res);
    return keyRequest;
}

DrmHalVTSVendorModule_V1::ContentConfiguration DrmHalTest::getContent(const KeyType& type) const {
    for (const auto& config : contentConfigurations) {
        if (type != KeyType::OFFLINE || config.policy.allowOffline) {
            return config;
        }
    }
    EXPECT_TRUE(false) << "no content configurations found";
    return {};
}

hidl_vec<uint8_t> DrmHalTest::provideKeyResponse(
    const SessionId& sessionId,
    const hidl_vec<uint8_t>& keyResponse) {
    hidl_vec<uint8_t> keySetId;
    auto res = drmPlugin->provideKeyResponse(
        sessionId, keyResponse,
        [&](StatusV1_0 status, const hidl_vec<uint8_t>& myKeySetId) {
            EXPECT_EQ(StatusV1_0::OK, status) << "Failure providing "
                                                 "key response for configuration ";
            keySetId = myKeySetId;
        });
    EXPECT_OK(res);
    return keySetId;
}

/**
 * Helper method to load keys for subsequent decrypt tests.
 * These tests use predetermined key request/response to
 * avoid requiring a round trip to a license server.
 */
hidl_vec<uint8_t> DrmHalTest::loadKeys(
    const SessionId& sessionId,
    const DrmHalVTSVendorModule_V1::ContentConfiguration& configuration,
    const KeyType& type) {
    hidl_vec<uint8_t> keyRequest = getKeyRequest(sessionId, configuration, type);

    /**
     * Get key response from vendor module
     */
    hidl_vec<uint8_t> keyResponse =
        vendorModule->handleKeyRequest(keyRequest, configuration.serverUrl);
    EXPECT_NE(keyResponse.size(), 0u) << "Expected key response size "
                                         "to have length > 0 bytes";

    return provideKeyResponse(sessionId, keyResponse);
}

hidl_vec<uint8_t> DrmHalTest::loadKeys(
        const SessionId& sessionId,
        const KeyType& type) {
    return loadKeys(sessionId, getContent(type), type);
}

KeyedVector DrmHalTest::toHidlKeyedVector(
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

hidl_array<uint8_t, 16> DrmHalTest::toHidlArray(const vector<uint8_t>& vec) {
    EXPECT_EQ(16u, vec.size());
    return hidl_array<uint8_t, 16>(&vec[0]);
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
sp<IMemory> DrmHalTest::getDecryptMemory(size_t size, size_t index) {
    sp<IAllocator> ashmemAllocator = IAllocator::getService("ashmem");
    EXPECT_NE(nullptr, ashmemAllocator.get());

    hidl_memory hidlMemory;
    auto res = ashmemAllocator->allocate(
            size, [&](bool success, const hidl_memory& memory) {
                EXPECT_EQ(success, true);
                EXPECT_EQ(memory.size(), size);
                hidlMemory = memory;
            });

    EXPECT_OK(res);

    sp<IMemory> mappedMemory = mapMemory(hidlMemory);
    EXPECT_NE(nullptr, mappedMemory.get());
    res = cryptoPlugin->setSharedBufferBase(hidlMemory, index);
    EXPECT_OK(res);
    return mappedMemory;
}

void DrmHalTest::fillRandom(const sp<IMemory>& memory) {
    random_device rd;
    mt19937 rand(rd());
    for (size_t i = 0; i < memory->getSize() / sizeof(uint32_t); i++) {
        auto p = static_cast<uint32_t*>(
                static_cast<void*>(memory->getPointer()));
        p[i] = rand();
    }
}

uint32_t DrmHalTest::decrypt(Mode mode, bool isSecure,
        const hidl_array<uint8_t, 16>& keyId, uint8_t* iv,
        const hidl_vec<SubSample>& subSamples, const Pattern& pattern,
        const vector<uint8_t>& key, StatusV1_2 expectedStatus) {
    const size_t kSegmentIndex = 0;

    uint8_t localIv[AES_BLOCK_SIZE];
    memcpy(localIv, iv, AES_BLOCK_SIZE);

    size_t totalSize = 0;
    for (size_t i = 0; i < subSamples.size(); i++) {
        totalSize += subSamples[i].numBytesOfClearData;
        totalSize += subSamples[i].numBytesOfEncryptedData;
    }

    // The first totalSize bytes of shared memory is the encrypted
    // input, the second totalSize bytes (if exists) is the decrypted output.
    size_t factor = expectedStatus == StatusV1_2::ERROR_DRM_FRAME_TOO_LARGE ? 1 : 2;
    sp<IMemory> sharedMemory =
            getDecryptMemory(totalSize * factor, kSegmentIndex);

    const SharedBuffer sourceBuffer = {
        .bufferId = kSegmentIndex, .offset = 0, .size = totalSize};
    fillRandom(sharedMemory);

    const DestinationBuffer destBuffer = {.type = BufferType::SHARED_MEMORY,
                                          {.bufferId = kSegmentIndex,
                                           .offset = totalSize,
                                           .size = totalSize},
                                          .secureMemory = nullptr};
    const uint64_t offset = 0;
    uint32_t bytesWritten = 0;
    auto res = cryptoPlugin->decrypt_1_2(isSecure, keyId, localIv, mode, pattern,
            subSamples, sourceBuffer, offset, destBuffer,
            [&](StatusV1_2 status, uint32_t count, string detailedError) {
                EXPECT_EQ(expectedStatus, status) << "Unexpected decrypt status " <<
                detailedError;
                bytesWritten = count;
            });
    EXPECT_OK(res);

    if (bytesWritten != totalSize) {
        return bytesWritten;
    }
    uint8_t* base = static_cast<uint8_t*>(
            static_cast<void*>(sharedMemory->getPointer()));

    // generate reference vector
    vector<uint8_t> reference(totalSize);

    memcpy(localIv, iv, AES_BLOCK_SIZE);
    switch (mode) {
    case Mode::UNENCRYPTED:
        memcpy(&reference[0], base, totalSize);
        break;
    case Mode::AES_CTR:
        aes_ctr_decrypt(&reference[0], base, localIv, subSamples, key);
        break;
    case Mode::AES_CBC:
        aes_cbc_decrypt(&reference[0], base, localIv, subSamples, key);
        break;
    case Mode::AES_CBC_CTS:
        EXPECT_TRUE(false) << "AES_CBC_CTS mode not supported";
        break;
    }

    // compare reference to decrypted data which is at base + total size
    EXPECT_EQ(0, memcmp(static_cast<void *>(&reference[0]),
                        static_cast<void*>(base + totalSize), totalSize))
            << "decrypt data mismatch";
    return totalSize;
}

/**
 * Decrypt a list of clear+encrypted subsamples using the specified key
 * in AES-CTR mode
 */
void DrmHalTest::aes_ctr_decrypt(uint8_t* dest, uint8_t* src,
        uint8_t* iv, const hidl_vec<SubSample>& subSamples,
        const vector<uint8_t>& key) {
    AES_KEY decryptionKey;
    AES_set_encrypt_key(&key[0], 128, &decryptionKey);

    size_t offset = 0;
    unsigned int blockOffset = 0;
    uint8_t previousEncryptedCounter[AES_BLOCK_SIZE];
    memset(previousEncryptedCounter, 0, AES_BLOCK_SIZE);

    for (size_t i = 0; i < subSamples.size(); i++) {
        const SubSample& subSample = subSamples[i];

        if (subSample.numBytesOfClearData > 0) {
            memcpy(dest + offset, src + offset, subSample.numBytesOfClearData);
            offset += subSample.numBytesOfClearData;
        }

        if (subSample.numBytesOfEncryptedData > 0) {
            AES_ctr128_encrypt(src + offset, dest + offset,
                    subSample.numBytesOfEncryptedData, &decryptionKey,
                    iv, previousEncryptedCounter, &blockOffset);
            offset += subSample.numBytesOfEncryptedData;
        }
    }
}

/**
 * Decrypt a list of clear+encrypted subsamples using the specified key
 * in AES-CBC mode
 */
void DrmHalTest::aes_cbc_decrypt(uint8_t* dest, uint8_t* src,
        uint8_t* iv, const hidl_vec<SubSample>& subSamples,
        const vector<uint8_t>& key) {
    AES_KEY decryptionKey;
    AES_set_encrypt_key(&key[0], 128, &decryptionKey);

    size_t offset = 0;
    for (size_t i = 0; i < subSamples.size(); i++) {
        memcpy(dest + offset, src + offset, subSamples[i].numBytesOfClearData);
        offset += subSamples[i].numBytesOfClearData;

        AES_cbc_encrypt(src + offset, dest + offset, subSamples[i].numBytesOfEncryptedData,
                &decryptionKey, iv, 0 /* decrypt */);
        offset += subSamples[i].numBytesOfEncryptedData;
    }
}

/**
 * Helper method to test decryption with invalid keys is returned
 */
void DrmHalClearkeyTest::decryptWithInvalidKeys(
        hidl_vec<uint8_t>& invalidResponse,
        vector<uint8_t>& iv,
        const Pattern& noPattern,
        const vector<SubSample>& subSamples) {
    DrmHalVTSVendorModule_V1::ContentConfiguration content = getContent();
    if (content.keys.empty()) {
        FAIL() << "no keys";
    }

    const auto& key = content.keys[0];
    auto sessionId = openSession();
    auto res = drmPlugin->provideKeyResponse(
        sessionId, invalidResponse,
        [&](StatusV1_0 status, const hidl_vec<uint8_t>& myKeySetId) {
            EXPECT_EQ(StatusV1_0::OK, status);
            EXPECT_EQ(0u, myKeySetId.size());
        });
    EXPECT_OK(res);

    EXPECT_TRUE(cryptoPlugin->setMediaDrmSession(sessionId).isOk());

    uint32_t byteCount = decrypt(Mode::AES_CTR, key.isSecure,
            toHidlArray(key.keyId), &iv[0], subSamples, noPattern,
            key.clearContentKey, Status::ERROR_DRM_NO_LICENSE);
    EXPECT_EQ(0u, byteCount);

    closeSession(sessionId);
}

}  // namespace vts
}  // namespace V1_2
}  // namespace drm
}  // namespace hardware
}  // namespace android
