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

#define LOG_TAG "drm_hal_common"

#include <gtest/gtest.h>
#include <log/log.h>
#include <openssl/aes.h>
#include <sys/mman.h>
#include <random>

#include <aidlcommonsupport/NativeHandle.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <android/sharedmem.h>
#include <cutils/native_handle.h>

#include "drm_hal_clearkey_module.h"
#include "drm_hal_common.h"

namespace aidl {
namespace android {
namespace hardware {
namespace drm {
namespace vts {

namespace clearkeydrm = ::android::hardware::drm::V1_2::vts;

using std::vector;
using ::aidl::android::hardware::common::Ashmem;
using ::aidl::android::hardware::drm::DecryptArgs;
using ::aidl::android::hardware::drm::DestinationBuffer;
using ::aidl::android::hardware::drm::EventType;
using ::aidl::android::hardware::drm::ICryptoPlugin;
using ::aidl::android::hardware::drm::IDrmPlugin;
using ::aidl::android::hardware::drm::KeyRequest;
using ::aidl::android::hardware::drm::KeyRequestType;
using ::aidl::android::hardware::drm::KeySetId;
using ::aidl::android::hardware::drm::KeyType;
using ::aidl::android::hardware::drm::KeyValue;
using ::aidl::android::hardware::drm::Mode;
using ::aidl::android::hardware::drm::Pattern;
using ::aidl::android::hardware::drm::ProvisionRequest;
using ::aidl::android::hardware::drm::ProvideProvisionResponseResult;
using ::aidl::android::hardware::drm::SecurityLevel;
using ::aidl::android::hardware::drm::Status;
using ::aidl::android::hardware::drm::SubSample;
using ::aidl::android::hardware::drm::Uuid;

Status DrmErr(const ::ndk::ScopedAStatus& ret) {
    return static_cast<Status>(ret.getServiceSpecificError());
}

std::string HalBaseName(const std::string& fullname) {
    auto idx = fullname.find('/');
    if (idx == std::string::npos) {
        return fullname;
    }
    return fullname.substr(idx + 1);
}

const char* kDrmIface = "android.hardware.drm.IDrmFactory";
const int MAX_OPEN_SESSION_ATTEMPTS = 3;

std::string HalFullName(const std::string& iface, const std::string& basename) {
    return iface + '/' + basename;
}

testing::AssertionResult IsOk(const ::ndk::ScopedAStatus& ret) {
    if (ret.isOk()) {
        return testing::AssertionSuccess();
    }
    return testing::AssertionFailure() << "ex: " << ret.getExceptionCode()
                                       << "; svc err: " << ret.getServiceSpecificError()
                                       << "; desc: " << ret.getDescription();
}

const char* kCallbackLostState = "LostState";
const char* kCallbackKeysChange = "KeysChange";

drm_vts::VendorModules* DrmHalTest::gVendorModules = nullptr;

/**
 * DrmHalPluginListener
 */
::ndk::ScopedAStatus DrmHalPluginListener::onEvent(
        EventType eventType,
        const vector<uint8_t>& sessionId,
        const vector<uint8_t>& data) {
    ListenerArgs args{};
    args.eventType = eventType;
    args.sessionId = sessionId;
    args.data = data;
    eventPromise.set_value(args);
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus DrmHalPluginListener::onExpirationUpdate(
        const vector<uint8_t>& sessionId,
        int64_t expiryTimeInMS) {
    ListenerArgs args{};
    args.sessionId = sessionId;
    args.expiryTimeInMS = expiryTimeInMS;
    expirationUpdatePromise.set_value(args);
    return ::ndk::ScopedAStatus::ok();

}

::ndk::ScopedAStatus DrmHalPluginListener::onSessionLostState(const vector<uint8_t>& sessionId) {
    ListenerArgs args{};
    args.sessionId = sessionId;
    sessionLostStatePromise.set_value(args);
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus DrmHalPluginListener::onKeysChange(
        const std::vector<uint8_t>& sessionId,
        const std::vector<::aidl::android::hardware::drm::KeyStatus>& keyStatusList,
        bool hasNewUsableKey) {
    ListenerArgs args{};
    args.sessionId = sessionId;
    args.keyStatusList = keyStatusList;
    args.hasNewUsableKey = hasNewUsableKey;
    keysChangePromise.set_value(args);
    return ::ndk::ScopedAStatus::ok();
}

ListenerArgs DrmHalPluginListener::getListenerArgs(std::promise<ListenerArgs>& promise) {
    auto future = promise.get_future();
    auto timeout = std::chrono::milliseconds(500);
    EXPECT_EQ(future.wait_for(timeout), std::future_status::ready);
    return future.get();
}

ListenerArgs DrmHalPluginListener::getEventArgs() {
    return getListenerArgs(eventPromise);
}

ListenerArgs DrmHalPluginListener::getExpirationUpdateArgs() {
    return getListenerArgs(expirationUpdatePromise);
}

ListenerArgs DrmHalPluginListener::getSessionLostStateArgs() {
    return getListenerArgs(sessionLostStatePromise);
}

ListenerArgs DrmHalPluginListener::getKeysChangeArgs() {
    return getListenerArgs(keysChangePromise);
}

static DrmHalVTSVendorModule_V1* getModuleForInstance(const std::string& instance) {
    if (instance.find("clearkey") != std::string::npos ||
        instance.find("default") != std::string::npos) {
        return new clearkeydrm::DrmHalVTSClearkeyModule();
    }

    return static_cast<DrmHalVTSVendorModule_V1*>(
            DrmHalTest::gVendorModules->getModuleByName(instance));
}

/**
 * DrmHalTest
 */

DrmHalTest::DrmHalTest() : vendorModule(getModuleForInstance(GetParamService())) {}

void DrmHalTest::SetUp() {
    const ::testing::TestInfo* const test_info =
            ::testing::UnitTest::GetInstance()->current_test_info();

    ALOGD("Running test %s.%s from (vendor) module %s", test_info->test_case_name(),
          test_info->name(), GetParamService().c_str());

    auto svc = GetParamService();
    const string drmInstance = HalFullName(kDrmIface, svc);

    if (!vendorModule) {
        ASSERT_NE(drmInstance, HalFullName(kDrmIface, "widevine")) << "Widevine requires vendor module.";
        ASSERT_NE(drmInstance, HalFullName(kDrmIface, "clearkey")) << "Clearkey requires vendor module.";
        GTEST_SKIP() << "No vendor module installed";
    }

    if (drmInstance.find("IDrmFactory") != std::string::npos) {
        drmFactory = IDrmFactory::fromBinder(
                ::ndk::SpAIBinder(AServiceManager_waitForService(drmInstance.c_str())));
        ASSERT_NE(drmFactory, nullptr);
        drmPlugin = createDrmPlugin();
        cryptoPlugin = createCryptoPlugin();
    }

    ASSERT_EQ(HalBaseName(drmInstance), vendorModule->getServiceName());
    contentConfigurations = vendorModule->getContentConfigurations();

    // If drm scheme not installed skip subsequent tests
    bool result = isCryptoSchemeSupported(getAidlUUID(), SecurityLevel::SW_SECURE_CRYPTO, "cenc");
    if (!result) {
        if (GetParamUUID() == std::array<uint8_t, 16>()) {
            GTEST_SKIP() << "vendor module drm scheme not supported";
        } else {
            FAIL() << "param scheme must be supported";
        }
    }

    ASSERT_NE(nullptr, drmPlugin.get())
            << "Can't find " << vendorModule->getServiceName() << " drm aidl plugin";
    ASSERT_NE(nullptr, cryptoPlugin.get())
            << "Can't find " << vendorModule->getServiceName() << " crypto aidl plugin";
}

std::shared_ptr<::aidl::android::hardware::drm::IDrmPlugin> DrmHalTest::createDrmPlugin() {
    if (drmFactory == nullptr) {
        return nullptr;
    }
    std::string packageName("aidl.android.hardware.drm.test");
    std::shared_ptr<::aidl::android::hardware::drm::IDrmPlugin> result;
    auto ret = drmFactory->createDrmPlugin(getAidlUUID(), packageName, &result);
    EXPECT_OK(ret) << "createDrmPlugin remote call failed";
    return result;
}

std::shared_ptr<::aidl::android::hardware::drm::ICryptoPlugin> DrmHalTest::createCryptoPlugin() {
    if (drmFactory == nullptr) {
        return nullptr;
    }
    vector<uint8_t> initVec;
    std::shared_ptr<::aidl::android::hardware::drm::ICryptoPlugin> result;
    auto ret = drmFactory->createCryptoPlugin(getAidlUUID(), initVec, &result);
    EXPECT_OK(ret) << "createCryptoPlugin remote call failed";
    return result;
}

::aidl::android::hardware::drm::Uuid DrmHalTest::getAidlUUID() {
    return toAidlUuid(getUUID());
}

std::vector<uint8_t> DrmHalTest::getUUID() {
    auto paramUUID = GetParamUUID();
    if (paramUUID == std::array<uint8_t, 16>()) {
        return getVendorUUID();
    }
    return std::vector(paramUUID.begin(), paramUUID.end());
}

std::vector<uint8_t> DrmHalTest::getVendorUUID() {
    if (vendorModule == nullptr) {
        ALOGW("vendor module for %s not found", GetParamService().c_str());
        return std::vector<uint8_t>(16);
    }
    return vendorModule->getUUID();
}

bool DrmHalTest::isCryptoSchemeSupported(Uuid uuid, SecurityLevel level, std::string mime) {
    if (drmFactory == nullptr) {
        return false;
    }
    CryptoSchemes schemes{};
    auto ret = drmFactory->getSupportedCryptoSchemes(&schemes);
    EXPECT_OK(ret);
    if (!ret.isOk() || !std::count(schemes.uuids.begin(), schemes.uuids.end(), uuid)) {
        return false;
    }
    if (mime.empty()) {
        EXPECT_THAT(level, AnyOf(Eq(SecurityLevel::DEFAULT), Eq(SecurityLevel::UNKNOWN)));
        return true;
    }
    for (auto ct : schemes.mimeTypes) {
        if (ct.mime != mime) {
            continue;
        }
        if (level == SecurityLevel::DEFAULT || level == SecurityLevel::UNKNOWN) {
            return true;
        }
        if (level <= ct.maxLevel && level >= ct.minLevel) {
            return true;
        }
    }
    return false;
}

void DrmHalTest::provision() {
    std::string certificateType;
    std::string certificateAuthority;
    vector<uint8_t> provisionRequest;
    std::string defaultUrl;
    ProvisionRequest result;
    auto ret = drmPlugin->getProvisionRequest(certificateType, certificateAuthority, &result);

    EXPECT_TXN(ret);
    if (ret.isOk()) {
        EXPECT_NE(result.request.size(), 0u);
        provisionRequest = result.request;
        defaultUrl = result.defaultUrl;
    } else if (DrmErr(ret) == Status::ERROR_DRM_CANNOT_HANDLE) {
        EXPECT_EQ(0u, result.request.size());
    }

    if (provisionRequest.size() > 0) {
        vector<uint8_t> response =
                vendorModule->handleProvisioningRequest(provisionRequest, defaultUrl);
        ASSERT_NE(0u, response.size());

        ProvideProvisionResponseResult result;
        auto ret = drmPlugin->provideProvisionResponse(response, &result);
        EXPECT_TXN(ret);
    }
}

SessionId DrmHalTest::openSession(SecurityLevel level, Status* err) {
    SessionId sessionId;
    auto ret = drmPlugin->openSession(level, &sessionId);
    EXPECT_TXN(ret);
    *err = DrmErr(ret);
    return sessionId;
}

/**
 * Helper method to open a session and verify that a non-empty
 * session ID is returned
 */
SessionId DrmHalTest::openSession() {
    SessionId sessionId;

    int attmpt = 0;
    while (attmpt++ < MAX_OPEN_SESSION_ATTEMPTS) {
        auto ret = drmPlugin->openSession(SecurityLevel::DEFAULT, &sessionId);
        if(DrmErr(ret) == Status::ERROR_DRM_NOT_PROVISIONED) {
            provision();
        } else {
            EXPECT_OK(ret);
            EXPECT_NE(0u, sessionId.size());
            break;
        }
    }

    return sessionId;
}

/**
 * Helper method to close a session
 */
void DrmHalTest::closeSession(const SessionId& sessionId) {
    auto ret = drmPlugin->closeSession(sessionId);
    EXPECT_OK(ret);
}

vector<uint8_t> DrmHalTest::getKeyRequest(
        const SessionId& sessionId,
        const DrmHalVTSVendorModule_V1::ContentConfiguration& configuration,
        const KeyType& type = KeyType::STREAMING) {
    KeyRequest result;
    auto ret = drmPlugin->getKeyRequest(sessionId, configuration.initData, configuration.mimeType,
                                        type, toAidlKeyedVector(configuration.optionalParameters),
                                        &result);
    EXPECT_OK(ret) << "Failed to get key request for configuration "
                   << configuration.name << " for key type "
                   << static_cast<int>(type);
    if (type == KeyType::RELEASE) {
        EXPECT_EQ(KeyRequestType::RELEASE, result.requestType);
    } else {
        EXPECT_EQ(KeyRequestType::INITIAL, result.requestType);
    }
    EXPECT_NE(result.request.size(), 0u) << "Expected key request size"
                                            " to have length > 0 bytes";
    return result.request;
}

DrmHalVTSVendorModule_V1::ContentConfiguration DrmHalTest::getContent(const KeyType& type) const {
    for (const auto& config : contentConfigurations) {
        if (type != KeyType::OFFLINE || config.policy.allowOffline) {
            return config;
        }
    }
    ADD_FAILURE() << "no content configurations found";
    return {};
}

vector<uint8_t> DrmHalTest::provideKeyResponse(const SessionId& sessionId,
                                               const vector<uint8_t>& keyResponse) {
    KeySetId result;
    auto ret = drmPlugin->provideKeyResponse(sessionId, keyResponse, &result);
    EXPECT_OK(ret) << "Failure providing key response for configuration ";
    return result.keySetId;
}

/**
 * Helper method to load keys for subsequent decrypt tests.
 * These tests use predetermined key request/response to
 * avoid requiring a round trip to a license server.
 */
vector<uint8_t> DrmHalTest::loadKeys(
        const SessionId& sessionId,
        const DrmHalVTSVendorModule_V1::ContentConfiguration& configuration, const KeyType& type) {
    vector<uint8_t> keyRequest = getKeyRequest(sessionId, configuration, type);

    /**
     * Get key response from vendor module
     */
    vector<uint8_t> keyResponse =
            vendorModule->handleKeyRequest(keyRequest, configuration.serverUrl);
    EXPECT_NE(keyResponse.size(), 0u) << "Expected key response size "
                                         "to have length > 0 bytes";

    return provideKeyResponse(sessionId, keyResponse);
}

vector<uint8_t> DrmHalTest::loadKeys(const SessionId& sessionId, const KeyType& type) {
    return loadKeys(sessionId, getContent(type), type);
}

std::array<uint8_t, 16> DrmHalTest::toStdArray(const vector<uint8_t>& vec) {
    EXPECT_EQ(16u, vec.size());
    std::array<uint8_t, 16> arr;
    std::copy_n(vec.begin(), vec.size(), arr.begin());
    return arr;
}

KeyedVector DrmHalTest::toAidlKeyedVector(const map<string, string>& params) {
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
 * getDecryptMemory allocates memory for decryption, then sets it
 * as a shared buffer base in the crypto hal. An output SharedBuffer
 * is updated via reference.
 *
 * @param size the size of the memory segment to allocate
 * @param the index of the memory segment which will be used
 * to refer to it for decryption.
 */
void DrmHalTest::getDecryptMemory(size_t size, size_t index, SharedBuffer& out) {
    out.bufferId = static_cast<int32_t>(index);
    out.offset = 0;
    out.size = static_cast<int64_t>(size);

    int fd = ASharedMemory_create("drmVtsSharedMemory", size);
    EXPECT_GE(fd, 0);
    EXPECT_EQ(size, ASharedMemory_getSize(fd));
    auto handle = native_handle_create(1, 0);
    handle->data[0] = fd;
    out.handle = ::android::makeToAidl(handle);

    EXPECT_OK(cryptoPlugin->setSharedBufferBase(out));
    native_handle_delete(handle);
}

uint8_t* DrmHalTest::fillRandom(const ::aidl::android::hardware::drm::SharedBuffer& buf) {
    std::random_device rd;
    std::mt19937 rand(rd());

    auto fd = buf.handle.fds[0].get();
    size_t size = buf.size;
    uint8_t* base = static_cast<uint8_t*>(
            mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
    EXPECT_NE(MAP_FAILED, base);
    for (size_t i = 0; i < size / sizeof(uint32_t); i++) {
        auto p = static_cast<uint32_t*>(static_cast<void*>(base));
        p[i] = rand();
    }
    return base;
}

uint32_t DrmHalTest::decrypt(Mode mode, bool isSecure, const std::array<uint8_t, 16>& keyId,
                             uint8_t* iv, const vector<SubSample>& subSamples,
                             const Pattern& pattern, const vector<uint8_t>& key,
                             Status expectedStatus) {
    const size_t kSegmentIndex = 0;

    uint8_t localIv[AES_BLOCK_SIZE];
    memcpy(localIv, iv, AES_BLOCK_SIZE);
    vector<uint8_t> ivVec(localIv, localIv + AES_BLOCK_SIZE);
    vector<uint8_t> keyIdVec(keyId.begin(), keyId.end());

    int64_t totalSize = 0;
    for (size_t i = 0; i < subSamples.size(); i++) {
        totalSize += subSamples[i].numBytesOfClearData;
        totalSize += subSamples[i].numBytesOfEncryptedData;
    }

    // The first totalSize bytes of shared memory is the encrypted
    // input, the second totalSize bytes (if exists) is the decrypted output.
    size_t factor = expectedStatus == Status::ERROR_DRM_FRAME_TOO_LARGE ? 1 : 2;
    SharedBuffer sourceBuffer;
    getDecryptMemory(totalSize * factor, kSegmentIndex, sourceBuffer);
    auto base = fillRandom(sourceBuffer);

    SharedBuffer sourceRange;
    sourceRange.bufferId = kSegmentIndex;
    sourceRange.offset = 0;
    sourceRange.size = totalSize;

    SharedBuffer destRange;
    destRange.bufferId = kSegmentIndex;
    destRange.offset = totalSize;
    destRange.size = totalSize;

    DecryptArgs args;
    args.secure = isSecure;
    args.keyId = keyIdVec;
    args.iv = ivVec;
    args.mode = mode;
    args.pattern = pattern;
    args.subSamples = subSamples;
    args.source = std::move(sourceRange);
    args.offset = 0;
    args.destination = std::move(destRange);

    int32_t bytesWritten = 0;
    auto ret = cryptoPlugin->decrypt(args, &bytesWritten);
    EXPECT_TXN(ret);
    EXPECT_EQ(expectedStatus, DrmErr(ret)) << "Unexpected decrypt status " << ret.getMessage();

    if (bytesWritten != totalSize) {
        return bytesWritten;
    }

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
            ADD_FAILURE() << "AES_CBC_CTS mode not supported";
            break;
    }

    // compare reference to decrypted data which is at base + total size
    EXPECT_EQ(0, memcmp(static_cast<void*>(&reference[0]), static_cast<void*>(base + totalSize),
                        totalSize))
            << "decrypt data mismatch";
    munmap(base, totalSize * factor);
    return totalSize;
}

/**
 * Decrypt a list of clear+encrypted subsamples using the specified key
 * in AES-CTR mode
 */
void DrmHalTest::aes_ctr_decrypt(uint8_t* dest, uint8_t* src, uint8_t* iv,
                                 const vector<SubSample>& subSamples, const vector<uint8_t>& key) {
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
            AES_ctr128_encrypt(src + offset, dest + offset, subSample.numBytesOfEncryptedData,
                               &decryptionKey, iv, previousEncryptedCounter, &blockOffset);
            offset += subSample.numBytesOfEncryptedData;
        }
    }
}

/**
 * Decrypt a list of clear+encrypted subsamples using the specified key
 * in AES-CBC mode
 */
void DrmHalTest::aes_cbc_decrypt(uint8_t* dest, uint8_t* src, uint8_t* iv,
                                 const vector<SubSample>& subSamples, const vector<uint8_t>& key) {
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
void DrmHalClearkeyTest::decryptWithInvalidKeys(vector<uint8_t>& invalidResponse,
                                                vector<uint8_t>& iv, const Pattern& noPattern,
                                                const vector<SubSample>& subSamples) {
    DrmHalVTSVendorModule_V1::ContentConfiguration content = getContent();
    if (content.keys.empty()) {
        FAIL() << "no keys";
    }

    const auto& key = content.keys[0];
    auto sessionId = openSession();
    KeySetId result;
    auto ret = drmPlugin->provideKeyResponse(sessionId, invalidResponse, &result);

    EXPECT_OK(ret);
    EXPECT_EQ(0u, result.keySetId.size());

    EXPECT_OK(cryptoPlugin->setMediaDrmSession(sessionId));

    uint32_t byteCount =
            decrypt(Mode::AES_CTR, key.isSecure, toStdArray(key.keyId), &iv[0], subSamples,
                    noPattern, key.clearContentKey, Status::ERROR_DRM_NO_LICENSE);
    EXPECT_EQ(0u, byteCount);

    closeSession(sessionId);
}

}  // namespace vts
}  // namespace drm
}  // namespace hardware
}  // namespace android
}  // namespace aidl
