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

#define LOG_TAG "mediacas_hidl_hal_test"

#include <VtsHalHidlTargetTestBase.h>
#include <android-base/logging.h>
#include <android/hardware/cas/1.0/ICas.h>
#include <android/hardware/cas/1.0/ICasListener.h>
#include <android/hardware/cas/1.0/IDescramblerBase.h>
#include <android/hardware/cas/1.0/IMediaCasService.h>
#include <hidl/HidlSupport.h>
#include <hidl/HidlTransportSupport.h>

#include <cinttypes>
#include <utility>

// CA System Ids used for testing
#define CLEAR_KEY_SYSTEM_ID 0xF6D8
#define INVALID_SYSTEM_ID 0

using android::Condition;
using android::hardware::cas::V1_0::ICas;
using android::hardware::cas::V1_0::ICasListener;
using android::hardware::cas::V1_0::IDescramblerBase;
using android::hardware::cas::V1_0::IMediaCasService;
using android::hardware::cas::V1_0::HidlCasPluginDescriptor;
using android::hardware::Void;
using android::hardware::hidl_vec;
using android::hardware::Return;
using android::Mutex;
using android::sp;

namespace {

class MediaCasHidlTest : public ::testing::VtsHalHidlTargetTestBase {
   public:
    virtual void SetUp() override {
        mService = ::testing::VtsHalHidlTargetTestBase::getService<IMediaCasService>();
        ASSERT_NE(mService, nullptr);
    }

    sp<IMediaCasService> mService;

   protected:
    static void description(const std::string& description) {
        RecordProperty("description", description);
    }
};

class MediaCasListener : public ICasListener {
   public:
    virtual ::android::hardware::Return<void> onEvent(
        int32_t event, int32_t arg, const ::android::hardware::hidl_vec<uint8_t>& data) override {
        ALOGI("Info: received event: %d, arg: %d, size: %zu", event, arg, data.size());

        return Void();
    }
};

TEST_F(MediaCasHidlTest, EnumeratePlugins) {
    description("Test enumerate plugins");
    hidl_vec<HidlCasPluginDescriptor> descriptors;
    EXPECT_TRUE(mService
                    ->enumeratePlugins([&descriptors](
                        hidl_vec<HidlCasPluginDescriptor> const& desc) { descriptors = desc; })
                    .isOk());

    if (descriptors.size() == 0) {
        ALOGW("[   WARN   ] enumeratePlugins list empty");
        return;
    }

    sp<MediaCasListener> casListener = new MediaCasListener();
    for (size_t i = 0; i < descriptors.size(); i++) {
        int32_t caSystemId = descriptors[i].caSystemId;
        bool status = mService->isSystemIdSupported(caSystemId);
        ASSERT_EQ(status, true);

        status = mService->isDescramblerSupported(caSystemId);
        ASSERT_EQ(status, true);

        sp<ICas> mediaCas = mService->createPlugin(caSystemId, casListener);
        ASSERT_NE(mediaCas, nullptr);

        sp<IDescramblerBase> descramblerBase = mService->createDescrambler(caSystemId);
        ASSERT_NE(descramblerBase, nullptr);
    }
}

TEST_F(MediaCasHidlTest, TestInvalidSystemIdFails) {
    description("Test failure for invalid system ID");
    sp<MediaCasListener> casListener = new MediaCasListener();

    ASSERT_FALSE(mService->isSystemIdSupported(INVALID_SYSTEM_ID));
    ASSERT_FALSE(mService->isDescramblerSupported(INVALID_SYSTEM_ID));

    sp<ICas> mediaCas = mService->createPlugin(INVALID_SYSTEM_ID, casListener);
    EXPECT_EQ(mediaCas, nullptr);

    sp<IDescramblerBase> descramblerBase = mService->createDescrambler(INVALID_SYSTEM_ID);
    EXPECT_EQ(descramblerBase, nullptr);
}

TEST_F(MediaCasHidlTest, TestClearKeyPluginInstalled) {
    description("Test if ClearKey plugin is installed");
    hidl_vec<HidlCasPluginDescriptor> descriptors;
    EXPECT_TRUE(mService
                    ->enumeratePlugins([&descriptors](
                        hidl_vec<HidlCasPluginDescriptor> const& _desc) { descriptors = _desc; })
                    .isOk());

    if (descriptors.size() == 0) {
        ALOGW("[   WARN   ] enumeratePlugins list empty");
    }

    for (size_t i = 0; i < descriptors.size(); i++) {
        int32_t caSystemId = descriptors[i].caSystemId;
        if (CLEAR_KEY_SYSTEM_ID == caSystemId) {
            return;
        }
    }

    ASSERT_TRUE(false) << "ClearKey plugin not installed";
}

}  // anonymous namespace

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    int status = RUN_ALL_TESTS();
    LOG(INFO) << "Test result = " << status;
    return status;
}
