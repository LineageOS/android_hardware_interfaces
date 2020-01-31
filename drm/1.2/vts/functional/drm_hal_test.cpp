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

#define LOG_TAG "drm_hal_test@1.2"

#include <gtest/gtest.h>
#include <hidl/HidlSupport.h>
#include <hidl/ServiceManagement.h>
#include <log/log.h>
#include <openssl/aes.h>
#include <vector>

#include "android/hardware/drm/1.2/vts/drm_hal_common.h"

using ::android::hardware::drm::V1_0::Status;
using ::android::hardware::drm::V1_1::KeyRequestType;
using ::android::hardware::drm::V1_1::SecurityLevel;
using ::android::hardware::drm::V1_2::HdcpLevel;
using ::android::hardware::drm::V1_2::KeySetId;
using ::android::hardware::drm::V1_2::KeyStatus;
using ::android::hardware::drm::V1_2::KeyStatusType;
using ::android::hardware::drm::V1_2::OfflineLicenseState;

using ::android::hardware::drm::V1_2::vts::DrmHalClearkeyTestV1_2;
using ::android::hardware::drm::V1_2::vts::DrmHalPluginListener;
using ::android::hardware::drm::V1_2::vts::DrmHalTest;
using ::android::hardware::drm::V1_2::vts::kCallbackLostState;
using ::android::hardware::drm::V1_2::vts::kCallbackKeysChange;

using ::android::hardware::hidl_array;
using ::android::hardware::hidl_string;

static const char* const kVideoMp4 = "video/mp4";
static const char* const kBadMime = "video/unknown";
static const char* const kDrmErrorTestKey = "drmErrorTest";
static const char* const kDrmErrorInvalidState = "invalidState";
static const char* const kDrmErrorResourceContention = "resourceContention";
static const SecurityLevel kSwSecureCrypto = SecurityLevel::SW_SECURE_CRYPTO;
static const SecurityLevel kHwSecureAll = SecurityLevel::HW_SECURE_ALL;

/**
 * Ensure drm factory supports module UUID Scheme
 */
TEST_P(DrmHalTest, VendorUuidSupported) {
    auto res = drmFactory->isCryptoSchemeSupported_1_2(getUUID(), kVideoMp4, kSwSecureCrypto);
    ALOGI("kVideoMp4 = %s res %d", kVideoMp4, (bool)res);
    EXPECT_TRUE(res);
}

/**
 * Ensure drm factory doesn't support an invalid scheme UUID
 */
TEST_P(DrmHalTest, InvalidPluginNotSupported) {
    const uint8_t kInvalidUUID[16] = {
        0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80,
        0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80};
    EXPECT_FALSE(drmFactory->isCryptoSchemeSupported_1_2(kInvalidUUID, kVideoMp4, kSwSecureCrypto));
}

/**
 * Ensure drm factory doesn't support an empty UUID
 */
TEST_P(DrmHalTest, EmptyPluginUUIDNotSupported) {
    hidl_array<uint8_t, 16> emptyUUID;
    memset(emptyUUID.data(), 0, 16);
    EXPECT_FALSE(drmFactory->isCryptoSchemeSupported_1_2(emptyUUID, kVideoMp4, kSwSecureCrypto));
}

/**
 * Ensure drm factory doesn't support an invalid mime type
 */
TEST_P(DrmHalTest, BadMimeNotSupported) {
    EXPECT_FALSE(drmFactory->isCryptoSchemeSupported_1_2(getUUID(), kBadMime, kSwSecureCrypto));
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
        StatusV1_0 err = StatusV1_0::OK;
        auto sid = openSession(level, &err);
        if (err == StatusV1_0::OK) {
            closeSession(sid);
        } else if (err == StatusV1_0::ERROR_DRM_CANNOT_HANDLE) {
            continue;
        } else {
            EXPECT_EQ(StatusV1_0::ERROR_DRM_NOT_PROVISIONED, err);
            provision();
        }
    }
}

/**
 * A get key request should fail if no sessionId is provided
 */
TEST_P(DrmHalTest, GetKeyRequestNoSession) {
    SessionId invalidSessionId;
    hidl_vec<uint8_t> initData;
    KeyedVector optionalParameters;
    auto res = drmPlugin->getKeyRequest_1_2(
            invalidSessionId, initData, kVideoMp4, KeyType::STREAMING,
            optionalParameters,
            [&](StatusV1_2 status, const hidl_vec<uint8_t>&, KeyRequestType,
                const hidl_string&) { EXPECT_EQ(StatusV1_2::BAD_VALUE, status); });
    EXPECT_OK(res);
}

/**
 * Test that the plugin returns the documented error for the
 * case of attempting to generate a key request using an
 * invalid mime type
 */
TEST_P(DrmHalTest, GetKeyRequestBadMime) {
    auto sessionId = openSession();
    hidl_vec<uint8_t> initData;
    KeyedVector optionalParameters;
    auto res = drmPlugin->getKeyRequest_1_2(
            sessionId, initData, kBadMime, KeyType::STREAMING,
            optionalParameters, [&](StatusV1_2 status, const hidl_vec<uint8_t>&,
                                    KeyRequestType, const hidl_string&) {
                EXPECT_NE(StatusV1_2::OK, status);
            });
    EXPECT_OK(res);
    closeSession(sessionId);
}

template <Status S, size_t N>
void checkKeySetIds(Status status, const hidl_vec<KeySetId>& keySetIds) {
    EXPECT_EQ(S, status);
    if (S == Status::OK) {
        EXPECT_EQ(N, keySetIds.size());
    }
}

template <Status S, OfflineLicenseState T>
void checkKeySetIdState(Status status, OfflineLicenseState state) {
    if (S == Status::OK) {
        EXPECT_TRUE(T == OfflineLicenseState::USABLE || T == OfflineLicenseState::INACTIVE);
    } else {
        EXPECT_TRUE(T == OfflineLicenseState::UNKNOWN);
    }
    EXPECT_EQ(S, status);
    EXPECT_EQ(T, state);
}

/**
 * Test drm plugin offline key support
 */
TEST_P(DrmHalTest, OfflineLicenseTest) {
    auto sessionId = openSession();
    hidl_vec<uint8_t> keySetId = loadKeys(sessionId, KeyType::OFFLINE);

    auto res = drmPlugin->getOfflineLicenseKeySetIds(
            [&](Status status, const hidl_vec<KeySetId>& keySetIds) {
                bool found = false;
                EXPECT_EQ(Status::OK, status);
                for (KeySetId keySetId2: keySetIds) {
                    if (keySetId == keySetId2) {
                        found = true;
                        break;
                    }
                }
                EXPECT_TRUE(found) << "keySetId not found";
            });
    EXPECT_OK(res);

    Status err = drmPlugin->removeOfflineLicense(keySetId);
    EXPECT_EQ(Status::OK, err);

    res = drmPlugin->getOfflineLicenseKeySetIds(
            [&](Status status, const hidl_vec<KeySetId>& keySetIds) {
                EXPECT_EQ(Status::OK, status);
                for (KeySetId keySetId2: keySetIds) {
                    EXPECT_NE(keySetId, keySetId2);
                }
            });
    EXPECT_OK(res);

    err = drmPlugin->removeOfflineLicense(keySetId);
    EXPECT_EQ(Status::BAD_VALUE, err);

    closeSession(sessionId);
}

/**
 * Test drm plugin offline key state
 */
TEST_P(DrmHalTest, OfflineLicenseStateTest) {
    auto sessionId = openSession();
    DrmHalVTSVendorModule_V1::ContentConfiguration content = getContent(KeyType::OFFLINE);
    hidl_vec<uint8_t> keySetId = loadKeys(sessionId, content, KeyType::OFFLINE);
    drmPlugin->getOfflineLicenseState(keySetId, checkKeySetIdState<Status::OK, OfflineLicenseState::USABLE>);

    hidl_vec<uint8_t> keyRequest = getKeyRequest(keySetId, content, KeyType::RELEASE);
    drmPlugin->getOfflineLicenseState(keySetId, checkKeySetIdState<Status::OK, OfflineLicenseState::INACTIVE>);

    /**
     * Get key response from vendor module
     */
    hidl_vec<uint8_t> keyResponse =
        vendorModule->handleKeyRequest(keyRequest, content.serverUrl);
    EXPECT_GT(keyResponse.size(), 0u);

    provideKeyResponse(keySetId, keyResponse);
    drmPlugin->getOfflineLicenseState(keySetId, checkKeySetIdState<Status::BAD_VALUE, OfflineLicenseState::UNKNOWN>);
    closeSession(sessionId);
}

/**
 * Negative offline license test. Remove empty keySetId
 */
TEST_P(DrmHalTest, RemoveEmptyKeySetId) {
    KeySetId emptyKeySetId;
    Status err = drmPlugin->removeOfflineLicense(emptyKeySetId);
    EXPECT_EQ(Status::BAD_VALUE, err);
}

/**
 * Negative offline license test. Get empty keySetId state
 */
TEST_P(DrmHalTest, GetEmptyKeySetIdState) {
    KeySetId emptyKeySetId;
    auto res = drmPlugin->getOfflineLicenseState(emptyKeySetId, checkKeySetIdState<Status::BAD_VALUE, OfflineLicenseState::UNKNOWN>);
    EXPECT_OK(res);
}

/**
 * Test that the plugin returns valid connected and max HDCP levels
 */
TEST_P(DrmHalTest, GetHdcpLevels) {
    auto res = drmPlugin->getHdcpLevels_1_2(
            [&](StatusV1_2 status, const HdcpLevel &connectedLevel,
                const HdcpLevel &maxLevel) {
                EXPECT_EQ(StatusV1_2::OK, status);
                EXPECT_GE(connectedLevel, HdcpLevel::HDCP_NONE);
                EXPECT_LE(maxLevel, HdcpLevel::HDCP_V2_3);
            });
    EXPECT_OK(res);
}

/**
 * Simulate the plugin sending keys change and make sure
 * the listener gets them.
 */
TEST_P(DrmHalTest, ListenerKeysChange) {
    sp<DrmHalPluginListener> listener = new DrmHalPluginListener();
    auto res = drmPlugin->setListener(listener);
    EXPECT_OK(res);

    auto sessionId = openSession();
    const hidl_vec<KeyStatus> keyStatusList = {
        {{1}, KeyStatusType::USABLE},
        {{2}, KeyStatusType::EXPIRED},
        {{3}, KeyStatusType::OUTPUTNOTALLOWED},
        {{4}, KeyStatusType::STATUSPENDING},
        {{5}, KeyStatusType::INTERNALERROR},
        {{6}, KeyStatusType::USABLEINFUTURE},
    };

    drmPlugin->sendKeysChange_1_2(sessionId, keyStatusList, true);
    auto result = listener->WaitForCallback(kCallbackKeysChange);
    EXPECT_TRUE(result.no_timeout);
    EXPECT_TRUE(result.args);
    EXPECT_EQ(sessionId, result.args->sessionId);
    EXPECT_EQ(keyStatusList, result.args->keyStatusList);
    closeSession(sessionId);
}

/**
 *  CryptoPlugin Decrypt tests
 */

/**
 * Positive decrypt test.  "Decrypt" a single clear segment
 */
TEST_P(DrmHalTest, ClearSegmentTest) {
    for (const auto& config : contentConfigurations) {
        for (const auto& key : config.keys) {
            const size_t kSegmentSize = 1024;
            vector<uint8_t> iv(AES_BLOCK_SIZE, 0);
            const Pattern noPattern = {0, 0};
            const vector<SubSample> subSamples = {{.numBytesOfClearData = kSegmentSize,
                                                   .numBytesOfEncryptedData = 0}};
            auto sessionId = openSession();
            loadKeys(sessionId, config);

            Status status = cryptoPlugin->setMediaDrmSession(sessionId);
            EXPECT_EQ(Status::OK, status);

            uint32_t byteCount = decrypt(Mode::UNENCRYPTED, key.isSecure, toHidlArray(key.keyId),
                    &iv[0], subSamples, noPattern, key.clearContentKey, StatusV1_2::OK);
            EXPECT_EQ(kSegmentSize, byteCount);

            closeSession(sessionId);
        }
    }
}

/**
 * Positive decrypt test.  Decrypt a single segment using aes_ctr.
 * Verify data matches.
 */
TEST_P(DrmHalTest, EncryptedAesCtrSegmentTest) {
    for (const auto& config : contentConfigurations) {
        for (const auto& key : config.keys) {
            const size_t kSegmentSize = 1024;
            vector<uint8_t> iv(AES_BLOCK_SIZE, 0);
            const Pattern noPattern = {0, 0};
            const vector<SubSample> subSamples = {{.numBytesOfClearData = kSegmentSize,
                                                   .numBytesOfEncryptedData = 0}};
            auto sessionId = openSession();
            loadKeys(sessionId, config);

            Status status = cryptoPlugin->setMediaDrmSession(sessionId);
            EXPECT_EQ(Status::OK, status);

            uint32_t byteCount = decrypt(Mode::AES_CTR, key.isSecure, toHidlArray(key.keyId),
                    &iv[0], subSamples, noPattern, key.clearContentKey, StatusV1_2::OK);
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
            const vector<SubSample> subSamples = {{.numBytesOfClearData = kSegmentSize,
                                                   .numBytesOfEncryptedData = 0}};
            auto sessionId = openSession();
            loadKeys(sessionId, config);

            Status status = cryptoPlugin->setMediaDrmSession(sessionId);
            EXPECT_EQ(Status::OK, status);

            decrypt(Mode::UNENCRYPTED, key.isSecure, toHidlArray(key.keyId),
                    &iv[0], subSamples, noPattern, key.clearContentKey, StatusV1_2::ERROR_DRM_FRAME_TOO_LARGE);

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
            const vector<SubSample> subSamples = {{.numBytesOfClearData = 256,
                                                   .numBytesOfEncryptedData = 256}};
            auto sessionId = openSession();

            Status status = cryptoPlugin->setMediaDrmSession(sessionId);
            EXPECT_EQ(Status::OK, status);

            uint32_t byteCount = decrypt(Mode::AES_CTR, key.isSecure,
                    toHidlArray(key.keyId), &iv[0], subSamples, noPattern,
                    key.clearContentKey, StatusV1_2::ERROR_DRM_NO_LICENSE);
            EXPECT_EQ(0u, byteCount);

            closeSession(sessionId);
        }
    }
}

/**
 * Ensure clearkey drm factory doesn't support security level higher than supported
 */
TEST_P(DrmHalClearkeyTestV1_2, BadLevelNotSupported) {
    EXPECT_FALSE(drmFactory->isCryptoSchemeSupported_1_2(getUUID(), kVideoMp4, kHwSecureAll));
}

/**
 * Test resource contention during attempt to generate key request
 */
TEST_P(DrmHalClearkeyTestV1_2, GetKeyRequestResourceContention) {
    Status status = drmPlugin->setPropertyString(kDrmErrorTestKey, kDrmErrorResourceContention);
    EXPECT_EQ(Status::OK, status);
    auto sessionId = openSession();
    hidl_vec<uint8_t> initData;
    KeyedVector optionalParameters;
    auto res = drmPlugin->getKeyRequest_1_2(
            sessionId, initData, kVideoMp4, KeyType::STREAMING,
            optionalParameters, [&](StatusV1_2 status, const hidl_vec<uint8_t>&,
                                    KeyRequestType, const hidl_string&) {
                EXPECT_EQ(StatusV1_2::ERROR_DRM_RESOURCE_CONTENTION, status);
            });
    EXPECT_OK(res);

    status = drmPlugin->closeSession(sessionId);
    EXPECT_NE(Status::OK, status);
}

/**
 * Test clearkey plugin offline key with mock error
 */
TEST_P(DrmHalClearkeyTestV1_2, OfflineLicenseInvalidState) {
    auto sessionId = openSession();
    hidl_vec<uint8_t> keySetId = loadKeys(sessionId, KeyType::OFFLINE);
    Status status = drmPlugin->setPropertyString(kDrmErrorTestKey, kDrmErrorInvalidState);
    EXPECT_EQ(Status::OK, status);

    // everything should start failing
    const Status kInvalidState = Status::ERROR_DRM_INVALID_STATE;
    const OfflineLicenseState kUnknownState = OfflineLicenseState::UNKNOWN;
    auto res = drmPlugin->getOfflineLicenseKeySetIds(checkKeySetIds<kInvalidState, 0u>);
    EXPECT_OK(res);
    res = drmPlugin->getOfflineLicenseState(keySetId, checkKeySetIdState<kInvalidState, kUnknownState>);
    EXPECT_OK(res);
    Status err = drmPlugin->removeOfflineLicense(keySetId);
    EXPECT_EQ(kInvalidState, err);
    closeSession(sessionId);
}

/**
 * Test SessionLostState is triggered on error
 */
TEST_P(DrmHalClearkeyTestV1_2, SessionLostState) {
    sp<DrmHalPluginListener> listener = new DrmHalPluginListener();
    auto res = drmPlugin->setListener(listener);
    EXPECT_OK(res);

    Status status = drmPlugin->setPropertyString(kDrmErrorTestKey, kDrmErrorInvalidState);
    EXPECT_EQ(Status::OK, status);

    auto sessionId = openSession();
    drmPlugin->closeSession(sessionId);

    auto result = listener->WaitForCallback(kCallbackLostState);
    EXPECT_TRUE(result.no_timeout);
    EXPECT_TRUE(result.args);
    EXPECT_EQ(sessionId, result.args->sessionId);
}

/**
 * Negative decrypt test. Decrypt with invalid key.
 */
TEST_P(DrmHalClearkeyTestV1_2, DecryptWithEmptyKey) {
    vector<uint8_t> iv(AES_BLOCK_SIZE, 0);
    const Pattern noPattern = {0, 0};
    const uint32_t kClearBytes = 512;
    const uint32_t kEncryptedBytes = 512;
    const vector<SubSample> subSamples = {
        {.numBytesOfClearData = kClearBytes,
         .numBytesOfEncryptedData = kEncryptedBytes}};

    // base 64 encoded JSON response string, must not contain padding character '='
    const hidl_string emptyKeyResponse =
            "{\"keys\":[" \
                "{" \
                    "\"kty\":\"oct\"" \
                    "\"alg\":\"A128KW2\"" \
                    "\"k\":\"SGVsbG8gRnJpZW5kIQ\"" \
                    "\"kid\":\"Y2xlYXJrZXlrZXlpZDAyAy\"" \
                "}" \
                "{" \
                    "\"kty\":\"oct\"," \
                    "\"alg\":\"A128KW2\"" \
                    "\"kid\":\"Y2xlYXJrZXlrZXlpZDAzAy\"," \
                    // empty key follows
                    "\"k\":\"R\"" \
                "}]" \
            "}";
    const size_t kEmptyKeyResponseSize = emptyKeyResponse.size();

    hidl_vec<uint8_t> invalidResponse;
    invalidResponse.resize(kEmptyKeyResponseSize);
    memcpy(invalidResponse.data(), emptyKeyResponse.c_str(), kEmptyKeyResponseSize);
    decryptWithInvalidKeys(invalidResponse, iv, noPattern, subSamples);
}

/**
 * Negative decrypt test. Decrypt with a key exceeds AES_BLOCK_SIZE.
 */
TEST_P(DrmHalClearkeyTestV1_2, DecryptWithKeyTooLong) {
    vector<uint8_t> iv(AES_BLOCK_SIZE, 0);
    const Pattern noPattern = {0, 0};
    const uint32_t kClearBytes = 512;
    const uint32_t kEncryptedBytes = 512;
    const vector<SubSample> subSamples = {
        {.numBytesOfClearData = kClearBytes,
         .numBytesOfEncryptedData = kEncryptedBytes}};

    // base 64 encoded JSON response string, must not contain padding character '='
    const hidl_string keyTooLongResponse =
            "{\"keys\":[" \
                "{" \
                    "\"kty\":\"oct\"," \
                    "\"alg\":\"A128KW2\"" \
                    "\"kid\":\"Y2xlYXJrZXlrZXlpZDAzAy\"," \
                    // key too long
                    "\"k\":\"V2lubmllIHRoZSBwb29oIVdpbm5pZSB0aGUgcG9vaCE=\"" \
                "}]" \
            "}";
    const size_t kKeyTooLongResponseSize = keyTooLongResponse.size();

    hidl_vec<uint8_t> invalidResponse;
    invalidResponse.resize(kKeyTooLongResponseSize);
    memcpy(invalidResponse.data(), keyTooLongResponse.c_str(), kKeyTooLongResponseSize);
    decryptWithInvalidKeys(invalidResponse, iv, noPattern, subSamples);
}
