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

#define LOG_TAG "drm_hal_test"

#include <gtest/gtest.h>
#include <log/log.h>
#include <openssl/aes.h>

#include <memory>
#include <vector>

#include "drm_hal_common.h"

using ::aidl::android::hardware::drm::EventType;
using ::aidl::android::hardware::drm::HdcpLevels;
using ::aidl::android::hardware::drm::KeyRequest;
using ::aidl::android::hardware::drm::HdcpLevel;
using ::aidl::android::hardware::drm::IDrmPluginListener;
using ::aidl::android::hardware::drm::KeyRequestType;
using ::aidl::android::hardware::drm::KeySetId;
using ::aidl::android::hardware::drm::KeyStatus;
using ::aidl::android::hardware::drm::KeyStatusType;
using ::aidl::android::hardware::drm::KeyType;
using ::aidl::android::hardware::drm::Mode;
using ::aidl::android::hardware::drm::OfflineLicenseState;
using ::aidl::android::hardware::drm::Pattern;
using ::aidl::android::hardware::drm::SecurityLevel;
using ::aidl::android::hardware::drm::Status;
using ::aidl::android::hardware::drm::SubSample;
using ::aidl::android::hardware::drm::Uuid;

using ::aidl::android::hardware::drm::vts::DrmErr;
using ::aidl::android::hardware::drm::vts::DrmHalClearkeyTest;
using ::aidl::android::hardware::drm::vts::DrmHalPluginListener;
using ::aidl::android::hardware::drm::vts::DrmHalTest;
using ::aidl::android::hardware::drm::vts::ListenerArgs;
using ::aidl::android::hardware::drm::vts::kCallbackKeysChange;
using ::aidl::android::hardware::drm::vts::kCallbackLostState;

using std::string;
using std::vector;

static const char* const kVideoMp4 = "video/mp4";
static const char* const kBadMime = "video/unknown";
static const char* const kDrmErrorTestKey = "drmErrorTest";
static const char* const kDrmErrorInvalidState = "invalidState";
static const char* const kDrmErrorResourceContention = "resourceContention";
static constexpr SecurityLevel kSwSecureCrypto = SecurityLevel::SW_SECURE_CRYPTO;
static constexpr SecurityLevel kHwSecureAll = SecurityLevel::HW_SECURE_ALL;

/**
 * Ensure drm factory supports module UUID Scheme
 */
TEST_P(DrmHalTest, VendorUuidSupported) {
    bool result = isCryptoSchemeSupported(getAidlUUID(), kSwSecureCrypto, kVideoMp4);
    ALOGI("kVideoMp4 = %s res %d", kVideoMp4, result);
    EXPECT_TRUE(result);
}

/**
 * Ensure drm factory doesn't support an invalid scheme UUID
 */
TEST_P(DrmHalTest, InvalidPluginNotSupported) {
    const vector<uint8_t> kInvalidUUID = {0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80,
                                          0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80};
    auto result = isCryptoSchemeSupported(toAidlUuid(kInvalidUUID), kSwSecureCrypto, kVideoMp4);
    EXPECT_FALSE(result);
}

/**
 * Ensure drm factory doesn't support an empty UUID
 */
TEST_P(DrmHalTest, EmptyPluginUUIDNotSupported) {
    vector<uint8_t> emptyUUID(16);
    memset(emptyUUID.data(), 0, 16);
    auto result = isCryptoSchemeSupported(toAidlUuid(emptyUUID), kSwSecureCrypto, kVideoMp4);
    EXPECT_FALSE(result);
}

/**
 * Ensure drm factory doesn't support an invalid mime type
 */
TEST_P(DrmHalTest, BadMimeNotSupported) {
    auto result = isCryptoSchemeSupported(getAidlUUID(), kSwSecureCrypto, kBadMime);
    EXPECT_FALSE(result);
}

/**
 * getSupportedCryptoSchemes confidence check
 */
TEST_P(DrmHalTest, SupportedCryptoSchemes) {
    aidl::android::hardware::drm::CryptoSchemes schemes{};
    auto result = drmFactory->getSupportedCryptoSchemes(&schemes);
    EXPECT_FALSE(schemes.uuids.empty());
    for(auto ct : schemes.mimeTypes) {
        EXPECT_LE(ct.minLevel, ct.maxLevel);
    }
    EXPECT_OK(result);
}

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
TEST_P(DrmHalTest, DoProvisioning) {
    for (auto level : {kHwSecureAll, kSwSecureCrypto}) {
        Status err = Status::OK;
        auto sid = openSession(level, &err);
        if (err == Status::OK) {
            closeSession(sid);
        } else if (err == Status::ERROR_DRM_CANNOT_HANDLE) {
            continue;
        } else {
            EXPECT_EQ(Status::ERROR_DRM_NOT_PROVISIONED, err);
            provision();
        }
    }
}

/**
 * A get key request should fail if no sessionId is provided
 */
TEST_P(DrmHalTest, GetKeyRequestNoSession) {
    SessionId invalidSessionId;
    vector<uint8_t> initData;
    KeyedVector optionalParameters;
    KeyRequest result;
    auto ret = drmPlugin->getKeyRequest(invalidSessionId, initData, kVideoMp4, KeyType::STREAMING,
                                        optionalParameters, &result);
    EXPECT_TXN(ret);
    EXPECT_EQ(Status::BAD_VALUE, DrmErr(ret));
}

/**
 * Test that the plugin returns the documented error for the
 * case of attempting to generate a key request using an
 * invalid mime type
 */
TEST_P(DrmHalTest, GetKeyRequestBadMime) {
    auto sessionId = openSession();
    vector<uint8_t> initData;
    KeyedVector optionalParameters;
    KeyRequest result;
    auto ret = drmPlugin->getKeyRequest(sessionId, initData, kBadMime, KeyType::STREAMING,
                                        optionalParameters, &result);
    EXPECT_EQ(EX_SERVICE_SPECIFIC, ret.getExceptionCode());
    closeSession(sessionId);
}

/**
 * Test drm plugin offline key support
 */
TEST_P(DrmHalTest, OfflineLicenseTest) {
    auto sessionId = openSession();
    vector<uint8_t> keySetId = loadKeys(sessionId, KeyType::OFFLINE);
    closeSession(sessionId);

    vector<KeySetId> result;
    auto ret = drmPlugin->getOfflineLicenseKeySetIds(&result);
    EXPECT_OK(ret);
    bool found = false;
    for (KeySetId keySetId2 : result) {
        if (keySetId == keySetId2.keySetId) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found) << "keySetId not found";

    ret = drmPlugin->removeOfflineLicense({keySetId});
    EXPECT_OK(ret);

    ret = drmPlugin->getOfflineLicenseKeySetIds(&result);
    EXPECT_OK(ret);
    for (KeySetId keySetId2 : result) {
        EXPECT_NE(keySetId, keySetId2.keySetId);
    }

    for (auto level : {kHwSecureAll, kSwSecureCrypto}) {
        Status err = Status::OK;
        auto sid = openSession(level, &err);
        if (err == Status::OK) {
            closeSession(sid);
        } else if (err == Status::ERROR_DRM_CANNOT_HANDLE) {
            continue;
        } else {
            EXPECT_EQ(Status::ERROR_DRM_NOT_PROVISIONED, err);
            provision();
        }
    }
    ret = drmPlugin->removeOfflineLicense({keySetId});
    EXPECT_TXN(ret);
    EXPECT_EQ(Status::BAD_VALUE, DrmErr(ret));
}

/**
 * Test drm plugin offline key state
 */
TEST_P(DrmHalTest, OfflineLicenseStateTest) {
    auto sessionId = openSession();
    DrmHalVTSVendorModule_V1::ContentConfiguration content = getContent(KeyType::OFFLINE);
    vector<uint8_t> keySetId = loadKeys(sessionId, content, KeyType::OFFLINE);
    closeSession(sessionId);

    OfflineLicenseState result{};
    auto ret = drmPlugin->getOfflineLicenseState({keySetId}, &result);
    EXPECT_OK(ret);
    EXPECT_EQ(OfflineLicenseState::USABLE, result);

    vector<uint8_t> keyRequest = getKeyRequest(keySetId, content, KeyType::RELEASE);
    ret = drmPlugin->getOfflineLicenseState({keySetId}, &result);
    EXPECT_OK(ret);
    EXPECT_EQ(OfflineLicenseState::INACTIVE, result);

    /**
     * Get key response from vendor module
     */
    vector<uint8_t> keyResponse = vendorModule->handleKeyRequest(keyRequest, content.serverUrl);
    EXPECT_GT(keyResponse.size(), 0u);

    result = OfflineLicenseState::UNKNOWN;
    provideKeyResponse(keySetId, keyResponse);
    ret = drmPlugin->getOfflineLicenseState({keySetId}, &result);
    EXPECT_TXN(ret);
    EXPECT_EQ(Status::BAD_VALUE, DrmErr(ret));
    EXPECT_EQ(OfflineLicenseState::UNKNOWN, result);
}

/**
 * Negative offline license test. Remove empty keySetId
 */
TEST_P(DrmHalTest, RemoveEmptyKeySetId) {
    KeySetId emptyKeySetId;
    auto ret = drmPlugin->removeOfflineLicense(emptyKeySetId);
    EXPECT_TXN(ret);
    EXPECT_EQ(Status::BAD_VALUE, DrmErr(ret));
}

/**
 * Negative offline license test. Get empty keySetId state
 */
TEST_P(DrmHalTest, GetEmptyKeySetIdState) {
    KeySetId emptyKeySetId;
    OfflineLicenseState result;
    auto ret = drmPlugin->getOfflineLicenseState(emptyKeySetId, &result);
    EXPECT_TXN(ret);
    EXPECT_EQ(Status::BAD_VALUE, DrmErr(ret));
    EXPECT_EQ(OfflineLicenseState::UNKNOWN, result);
}

/**
 * Test that the plugin returns valid connected and max HDCP levels
 */
TEST_P(DrmHalTest, GetHdcpLevels) {
    HdcpLevels result;
    auto ret = drmPlugin->getHdcpLevels(&result);
    EXPECT_OK(ret);
    EXPECT_GE(result.connectedLevel, HdcpLevel::HDCP_NONE);
    EXPECT_LE(result.maxLevel, HdcpLevel::HDCP_V2_3);
}

/**
 *  CryptoPlugin Decrypt tests
 */

/**
 * Positive decrypt test. "Decrypt" a single clear segment
 */
TEST_P(DrmHalTest, ClearSegmentTest) {
    for (const auto& config : contentConfigurations) {
        for (const auto& key : config.keys) {
            const size_t kSegmentSize = 1024;
            vector<uint8_t> iv(AES_BLOCK_SIZE, 0);
            const Pattern noPattern = {0, 0};
            const vector<SubSample> subSamples = {
                    {.numBytesOfClearData = kSegmentSize, .numBytesOfEncryptedData = 0}};
            auto sessionId = openSession();
            loadKeys(sessionId, config);

            auto ret = cryptoPlugin->setMediaDrmSession(sessionId);
            EXPECT_OK(ret);

            uint32_t byteCount =
                    decrypt(Mode::UNENCRYPTED, key.isSecure, toStdArray(key.keyId), &iv[0],
                            subSamples, noPattern, key.clearContentKey, Status::OK);
            EXPECT_EQ(kSegmentSize, byteCount);

            closeSession(sessionId);
        }
    }
}

/**
 * Positive decrypt test. Decrypt a single segment using aes_ctr.
 * Verify data matches.
 */
TEST_P(DrmHalTest, EncryptedAesCtrSegmentTest) {
    for (const auto& config : contentConfigurations) {
        for (const auto& key : config.keys) {
            const size_t kSegmentSize = 1024;
            vector<uint8_t> iv(AES_BLOCK_SIZE, 0);
            const Pattern noPattern = {0, 0};
            const vector<SubSample> subSamples = {
                    {.numBytesOfClearData = kSegmentSize, .numBytesOfEncryptedData = 0}};
            auto sessionId = openSession();
            loadKeys(sessionId, config);

            auto ret = cryptoPlugin->setMediaDrmSession(sessionId);
            EXPECT_OK(ret);

            uint32_t byteCount = decrypt(Mode::AES_CTR, key.isSecure, toStdArray(key.keyId), &iv[0],
                                         subSamples, noPattern, key.clearContentKey, Status::OK);
            EXPECT_EQ(kSegmentSize, byteCount);

            closeSession(sessionId);
        }
    }
}

/**
 * Negative decrypt test.  Decrypted frame too large to fit in output buffer
 */
TEST_P(DrmHalTest, ErrorFrameTooLarge) {
    for (const auto& config : contentConfigurations) {
        for (const auto& key : config.keys) {
            const size_t kSegmentSize = 1024;
            vector<uint8_t> iv(AES_BLOCK_SIZE, 0);
            const Pattern noPattern = {0, 0};
            const vector<SubSample> subSamples = {
                    {.numBytesOfClearData = kSegmentSize, .numBytesOfEncryptedData = 0}};
            auto sessionId = openSession();
            loadKeys(sessionId, config);

            auto ret = cryptoPlugin->setMediaDrmSession(sessionId);
            EXPECT_OK(ret);

            decrypt(Mode::UNENCRYPTED, key.isSecure, toStdArray(key.keyId), &iv[0], subSamples,
                    noPattern, key.clearContentKey, Status::ERROR_DRM_FRAME_TOO_LARGE);

            closeSession(sessionId);
        }
    }
}

/**
 * Negative decrypt test. Decrypt without loading keys.
 */
TEST_P(DrmHalTest, EncryptedAesCtrSegmentTestNoKeys) {
    for (const auto& config : contentConfigurations) {
        for (const auto& key : config.keys) {
            vector<uint8_t> iv(AES_BLOCK_SIZE, 0);
            const Pattern noPattern = {0, 0};
            const vector<SubSample> subSamples = {
                    {.numBytesOfClearData = 256, .numBytesOfEncryptedData = 256}};
            auto sessionId = openSession();

            auto ret = cryptoPlugin->setMediaDrmSession(sessionId);
            EXPECT_OK(ret);

            uint32_t byteCount =
                    decrypt(Mode::AES_CTR, key.isSecure, toStdArray(key.keyId), &iv[0], subSamples,
                            noPattern, key.clearContentKey, Status::ERROR_DRM_NO_LICENSE);
            EXPECT_EQ(0u, byteCount);

            closeSession(sessionId);
        }
    }
}

/**
 * Ensure clearkey drm factory doesn't support security level higher than supported
 */
TEST_P(DrmHalClearkeyTest, BadLevelNotSupported) {
    auto result = isCryptoSchemeSupported(getAidlUUID(), kHwSecureAll, kVideoMp4);
    EXPECT_FALSE(result);
}

/**
 * Test resource contention during attempt to generate key request
 */
TEST_P(DrmHalClearkeyTest, GetKeyRequestResourceContention) {
    auto ret = drmPlugin->setPropertyString(kDrmErrorTestKey, kDrmErrorResourceContention);
    EXPECT_OK(ret);

    auto sessionId = openSession();
    vector<uint8_t> initData;
    KeyedVector optionalParameters;
    KeyRequest result;
    ret = drmPlugin->getKeyRequest(sessionId, initData, kVideoMp4, KeyType::STREAMING,
                                   optionalParameters, &result);
    EXPECT_TXN(ret);
    EXPECT_EQ(Status::ERROR_DRM_RESOURCE_CONTENTION, DrmErr(ret));

    ret = drmPlugin->closeSession(sessionId);
    EXPECT_TXN(ret);
    EXPECT_NE(Status::OK, DrmErr(ret));
}

/**
 * Test clearkey plugin offline key with mock error
 */
TEST_P(DrmHalClearkeyTest, OfflineLicenseInvalidState) {
    auto sessionId = openSession();
    vector<uint8_t> keySetId = loadKeys(sessionId, KeyType::OFFLINE);
    auto ret = drmPlugin->setPropertyString(kDrmErrorTestKey, kDrmErrorInvalidState);
    EXPECT_OK(ret);

    // everything should start failing
    const Status kInvalidState = Status::ERROR_DRM_INVALID_STATE;
    vector<KeySetId> result;
    ret = drmPlugin->getOfflineLicenseKeySetIds(&result);
    EXPECT_TXN(ret);
    EXPECT_EQ(kInvalidState, DrmErr(ret));
    EXPECT_EQ(0u, result.size());

    OfflineLicenseState state = OfflineLicenseState::UNKNOWN;
    ret = drmPlugin->getOfflineLicenseState({keySetId}, &state);
    EXPECT_TXN(ret);
    EXPECT_EQ(kInvalidState, DrmErr(ret));
    EXPECT_EQ(OfflineLicenseState::UNKNOWN, state);

    ret = drmPlugin->removeOfflineLicense({keySetId});
    EXPECT_TXN(ret);
    EXPECT_EQ(kInvalidState, DrmErr(ret));
    closeSession(sessionId);
}

/**
 * Test listener is triggered on key response
 */
TEST_P(DrmHalClearkeyTest, ListenerCallbacks) {
    auto listener = ndk::SharedRefBase::make<DrmHalPluginListener>();
    auto res = drmPlugin->setListener(listener);
    EXPECT_OK(res);

    auto sessionId = openSession();
    loadKeys(sessionId, KeyType::STREAMING);
    closeSession(sessionId);

    auto args = listener->getEventArgs();
    EXPECT_EQ(EventType::VENDOR_DEFINED, args.eventType);
    EXPECT_EQ(sessionId, args.data);
    EXPECT_EQ(sessionId, args.sessionId);

    args = listener->getExpirationUpdateArgs();
    EXPECT_EQ(sessionId, args.sessionId);
    EXPECT_EQ(100, args.expiryTimeInMS);

    args = listener->getKeysChangeArgs();
    const vector<KeyStatus> keyStatusList = {
            {{0xa, 0xb, 0xc}, KeyStatusType::USABLE},
            {{0xd, 0xe, 0xf}, KeyStatusType::EXPIRED},
            {{0x0, 0x1, 0x2}, KeyStatusType::USABLE_IN_FUTURE},
    };
    EXPECT_EQ(sessionId, args.sessionId);
    EXPECT_EQ(keyStatusList, args.keyStatusList);
    EXPECT_TRUE(args.hasNewUsableKey);
}

/**
 * Test SessionLostState is triggered on error
 */
TEST_P(DrmHalClearkeyTest, SessionLostState) {
    auto listener = ndk::SharedRefBase::make<DrmHalPluginListener>();
    auto res = drmPlugin->setListener(listener);
    EXPECT_OK(res);

    res = drmPlugin->setPropertyString(kDrmErrorTestKey, kDrmErrorInvalidState);
    EXPECT_OK(res);

    auto sessionId = openSession();
    auto ret = drmPlugin->closeSession(sessionId);

    auto args = listener->getSessionLostStateArgs();
    EXPECT_EQ(sessionId, args.sessionId);
}

/**
 * Negative decrypt test. Decrypt with invalid key.
 */
TEST_P(DrmHalClearkeyTest, DecryptWithEmptyKey) {
    vector<uint8_t> iv(AES_BLOCK_SIZE, 0);
    const Pattern noPattern = {0, 0};
    const uint32_t kClearBytes = 512;
    const uint32_t kEncryptedBytes = 512;
    const vector<SubSample> subSamples = {
            {.numBytesOfClearData = kClearBytes, .numBytesOfEncryptedData = kEncryptedBytes}};

    // base 64 encoded JSON response string, must not contain padding character '='
    const string emptyKeyResponse =
            "{\"keys\":["
            "{"
            "\"kty\":\"oct\""
            "\"alg\":\"A128KW2\""
            "\"k\":\"SGVsbG8gRnJpZW5kIQ\""
            "\"kid\":\"Y2xlYXJrZXlrZXlpZDAyAy\""
            "}"
            "{"
            "\"kty\":\"oct\","
            "\"alg\":\"A128KW2\""
            "\"kid\":\"Y2xlYXJrZXlrZXlpZDAzAy\","  // empty key follows
            "\"k\":\"R\""
            "}]"
            "}";
    const size_t kEmptyKeyResponseSize = emptyKeyResponse.size();

    vector<uint8_t> invalidResponse;
    invalidResponse.resize(kEmptyKeyResponseSize);
    memcpy(invalidResponse.data(), emptyKeyResponse.c_str(), kEmptyKeyResponseSize);
    decryptWithInvalidKeys(invalidResponse, iv, noPattern, subSamples);
}

/**
 * Negative decrypt test. Decrypt with a key exceeds AES_BLOCK_SIZE.
 */
TEST_P(DrmHalClearkeyTest, DecryptWithKeyTooLong) {
    vector<uint8_t> iv(AES_BLOCK_SIZE, 0);
    const Pattern noPattern = {0, 0};
    const uint32_t kClearBytes = 512;
    const uint32_t kEncryptedBytes = 512;
    const vector<SubSample> subSamples = {
            {.numBytesOfClearData = kClearBytes, .numBytesOfEncryptedData = kEncryptedBytes}};

    // base 64 encoded JSON response string, must not contain padding character '='
    const string keyTooLongResponse =
            "{\"keys\":["
            "{"
            "\"kty\":\"oct\","
            "\"alg\":\"A128KW2\""
            "\"kid\":\"Y2xlYXJrZXlrZXlpZDAzAy\","  // key too long
            "\"k\":\"V2lubmllIHRoZSBwb29oIVdpbm5pZSB0aGUgcG9vaCE=\""
            "}]"
            "}";
    const size_t kKeyTooLongResponseSize = keyTooLongResponse.size();

    vector<uint8_t> invalidResponse;
    invalidResponse.resize(kKeyTooLongResponseSize);
    memcpy(invalidResponse.data(), keyTooLongResponse.c_str(), kKeyTooLongResponseSize);
    decryptWithInvalidKeys(invalidResponse, iv, noPattern, subSamples);
}
