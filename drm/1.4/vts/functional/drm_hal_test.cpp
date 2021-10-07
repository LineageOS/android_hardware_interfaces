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

#define LOG_TAG "drm_hal_test@1.4"

#include "android/hardware/drm/1.4/vts/drm_hal_test.h"

namespace android {
namespace hardware {
namespace drm {
namespace V1_4 {
namespace vts {

const char* const DrmHalTest::kVideoMp4 = "video/mp4";
const char* const DrmHalTest::kAudioMp4 = "audio/mp4";
const uint32_t DrmHalTest::kSecLevelDefault = DrmHalTest::kSecLevelMax + 1;

sp<drm::V1_4::IDrmPlugin> DrmHalTest::DrmPluginV1_4() const {
    sp<drm::V1_4::IDrmPlugin> plugin(drm::V1_4::IDrmPlugin::castFrom(drmPlugin));
    EXPECT_NE(nullptr, plugin.get());
    return plugin;
}

sp<V1_0::ICryptoPlugin> DrmHalTest::CryptoPlugin(const SessionId& sid) {
    sp<V1_0::ICryptoPlugin> crypto;
    auto res = cryptoFactory->createPlugin(
        getUUID(), sid,
        [&](V1_0::Status status, const sp<V1_0::ICryptoPlugin>& plugin) {
            EXPECT_EQ(V1_0::Status::OK, status);
            EXPECT_NE(nullptr, plugin.get());
            crypto = plugin;
        });
    EXPECT_OK(res);
    return crypto;
}

SessionId DrmHalTest::OpenSession(uint32_t level = kSecLevelDefault) {
    V1_0::Status err;
    SessionId sessionId;
    bool attemptedProvision = false;

    V1_0::IDrmPlugin::openSession_cb cb = [&](
            V1_0::Status status,
            const hidl_vec<unsigned char> &id) {
        err = status;
        sessionId = id;
    };

    while (true) {
        Return<void> res;
        if (level > kSecLevelMax) {
            res = drmPlugin->openSession(cb);
        } else if (level >= kSecLevelMin) {
            auto securityLevel = static_cast<SecurityLevel>(level);
            res = drmPlugin->openSession_1_1(securityLevel, cb);
        }
        EXPECT_OK(res);
        if (V1_0::Status::ERROR_DRM_NOT_PROVISIONED == err
                && !attemptedProvision) {
            // provision once if necessary
            provision();
            attemptedProvision = true;
            continue;
        } else if (V1_0::Status::ERROR_DRM_CANNOT_HANDLE == err) {
            // must be able to handle default level
            EXPECT_NE(kSecLevelDefault, level);
            sessionId = {};
        } else {
            EXPECT_EQ(V1_0::Status::OK, err);
            EXPECT_NE(sessionId.size(), 0u);
        }
        break;
    }

    return sessionId;
}

TEST_P(DrmHalTest, RequiresSecureDecoder) {
    for (uint32_t level : {kSecLevelMin, kSecLevelMax, kSecLevelDefault}) {
        for (auto mime : {kVideoMp4, kAudioMp4}) {
            auto sid = OpenSession(level);
            if (sid.size() == 0u) {
                continue;
            }
            auto drm = DrmPluginV1_4();
            sp<V1_0::ICryptoPlugin> crypto(CryptoPlugin(sid));
            if (drm == nullptr || crypto == nullptr) {
                continue;
            }
            bool r1 = crypto->requiresSecureDecoderComponent(mime);
            bool r2;
            if (level == kSecLevelDefault) {
                r2 = drm->requiresSecureDecoderDefault(mime);
            } else {
                auto sL = static_cast<SecurityLevel>(level);
                r2 = drm->requiresSecureDecoder(mime, sL);
            }
            EXPECT_EQ(r1, r2);
            closeSession(sid);
        }
    }
}

TEST_P(DrmHalTest, SetPlaybackId) {
    auto testInfo = ::testing::UnitTest::GetInstance()->current_test_info();
    auto testName = testInfo->name();
    const hidl_string& pbId{testName};
    auto sid = OpenSession();
    auto drm = DrmPluginV1_4();
    if (drm == nullptr) {
        return;
    }
    V1_0::Status err = drm->setPlaybackId(sid, pbId);
    EXPECT_EQ(V1_0::Status::OK, err);
    closeSession(sid);

    // search for playback id among metric attributes/values
    bool foundPbId = false;
    auto res = drmPlugin->getMetrics([&](
            V1_0::Status status,
            hidl_vec<V1_1::DrmMetricGroup> metricGroups) {
        EXPECT_EQ(V1_0::Status::OK, status);
        for (const auto& group : metricGroups) {
            for (const auto& metric : group.metrics) {
                for (const auto& value : metric.values) {
                    if (value.stringValue == pbId) {
                        foundPbId = true;
                        break;
                    }
                }
                for (const auto& attr : metric.attributes) {
                    if (attr.stringValue == pbId) {
                        foundPbId = true;
                        break;
                    }
                }
            }
        }
    });
    EXPECT_OK(res);
    EXPECT_TRUE(foundPbId);
}

TEST_P(DrmHalTest, GetLogMessages) {
    auto drm = DrmPluginV1_4();
    auto sid = OpenSession();
    auto crypto_1_0 = CryptoPlugin(sid);
    sp<V1_4::ICryptoPlugin> crypto(V1_4::ICryptoPlugin::castFrom(crypto_1_0));

    hidl_vec<uint8_t> initData;
    hidl_string mime{"text/plain"};
    V1_0::KeyedVector optionalParameters;
    auto res = drmPlugin->getKeyRequest_1_2(
            sid, initData, mime, V1_0::KeyType::STREAMING,
            optionalParameters, [&](V1_2::Status status, const hidl_vec<uint8_t>&,
                                    V1_1::KeyRequestType, const hidl_string&) {
                EXPECT_NE(V1_2::Status::OK, status);
            });
    EXPECT_OK(res);

    V1_4::IDrmPlugin::getLogMessages_cb cb = [&](
            V1_4::Status status,
            hidl_vec<V1_4::LogMessage> logs) {
        EXPECT_EQ(V1_4::Status::OK, status);
        EXPECT_NE(0, logs.size());
        for (auto log: logs) {
            ALOGI("priority=[%u] message='%s'", log.priority, log.message.c_str());
        }
    };

    auto res2 = drm->getLogMessages(cb);
    EXPECT_OK(res2);

    auto res3 = crypto->getLogMessages(cb);
    EXPECT_OK(res3);

    closeSession(sid);
}

}  // namespace vts
}  // namespace V1_4
}  // namespace drm
}  // namespace hardware
}  // namespace android
