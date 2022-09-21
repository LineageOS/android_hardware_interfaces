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
#include <aidl/Gtest.h>
#include <aidl/Vintf.h>

#include <android/hardware/vibrator/BnVibratorCallback.h>
#include <android/hardware/vibrator/IVibrator.h>
#include <android/hardware/vibrator/IVibratorManager.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>

#include <cmath>
#include <future>

using android::ProcessState;
using android::sp;
using android::String16;
using android::binder::Status;
using android::hardware::vibrator::BnVibratorCallback;
using android::hardware::vibrator::CompositeEffect;
using android::hardware::vibrator::CompositePrimitive;
using android::hardware::vibrator::Effect;
using android::hardware::vibrator::EffectStrength;
using android::hardware::vibrator::IVibrator;
using android::hardware::vibrator::IVibratorManager;
using std::chrono::high_resolution_clock;

const std::vector<Effect> kEffects{android::enum_range<Effect>().begin(),
                                   android::enum_range<Effect>().end()};
const std::vector<EffectStrength> kEffectStrengths{android::enum_range<EffectStrength>().begin(),
                                                   android::enum_range<EffectStrength>().end()};
const std::vector<CompositePrimitive> kPrimitives{android::enum_range<CompositePrimitive>().begin(),
                                                  android::enum_range<CompositePrimitive>().end()};

class CompletionCallback : public BnVibratorCallback {
  public:
    CompletionCallback(const std::function<void()>& callback) : mCallback(callback) {}
    Status onComplete() override {
        mCallback();
        return Status::ok();
    }

  private:
    std::function<void()> mCallback;
};

class VibratorAidl : public testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        manager = android::waitForDeclaredService<IVibratorManager>(String16(GetParam().c_str()));
        ASSERT_NE(manager, nullptr);
        ASSERT_TRUE(manager->getCapabilities(&capabilities).isOk());
        EXPECT_TRUE(manager->getVibratorIds(&vibratorIds).isOk());
    }

    sp<IVibratorManager> manager;
    int32_t capabilities;
    std::vector<int32_t> vibratorIds;
};

inline bool isUnknownOrUnsupported(Status status) {
    return status.exceptionCode() == Status::EX_UNSUPPORTED_OPERATION ||
           status.transactionError() == android::UNKNOWN_TRANSACTION;
}

TEST_P(VibratorAidl, ValidateExistingVibrators) {
    sp<IVibrator> vibrator;
    for (auto& id : vibratorIds) {
        EXPECT_TRUE(manager->getVibrator(id, &vibrator).isOk());
        ASSERT_NE(vibrator, nullptr);
    }
}

TEST_P(VibratorAidl, GetVibratorWithInvalidId) {
    int32_t invalidId = *max_element(vibratorIds.begin(), vibratorIds.end()) + 1;
    sp<IVibrator> vibrator;
    EXPECT_EQ(Status::EX_ILLEGAL_ARGUMENT,
              manager->getVibrator(invalidId, &vibrator).exceptionCode());
    ASSERT_EQ(vibrator, nullptr);
}

TEST_P(VibratorAidl, ValidatePrepareSyncedExistingVibrators) {
    if (!(capabilities & IVibratorManager::CAP_SYNC)) return;
    if (vibratorIds.empty()) return;
    EXPECT_TRUE(manager->prepareSynced(vibratorIds).isOk());
    EXPECT_TRUE(manager->cancelSynced().isOk());
}

TEST_P(VibratorAidl, PrepareSyncedEmptySetIsInvalid) {
    if (!(capabilities & IVibratorManager::CAP_SYNC)) return;
    std::vector<int32_t> emptyIds;
    EXPECT_EQ(Status::EX_ILLEGAL_ARGUMENT, manager->prepareSynced(emptyIds).exceptionCode());
}

TEST_P(VibratorAidl, PrepareSyncedNotSupported) {
    if (!(capabilities & IVibratorManager::CAP_SYNC)) {
        Status status = manager->prepareSynced(vibratorIds);
        EXPECT_TRUE(isUnknownOrUnsupported(status)) << status;
    }
}

TEST_P(VibratorAidl, PrepareOnNotSupported) {
    if (vibratorIds.empty()) return;
    if (!(capabilities & IVibratorManager::CAP_SYNC)) return;
    if (!(capabilities & IVibratorManager::CAP_PREPARE_ON)) {
        uint32_t durationMs = 250;
        EXPECT_TRUE(manager->prepareSynced(vibratorIds).isOk());
        sp<IVibrator> vibrator;
        for (auto& id : vibratorIds) {
            EXPECT_TRUE(manager->getVibrator(id, &vibrator).isOk());
            ASSERT_NE(vibrator, nullptr);
            Status status = vibrator->on(durationMs, nullptr);
            EXPECT_TRUE(isUnknownOrUnsupported(status)) << status;
        }
        EXPECT_TRUE(manager->cancelSynced().isOk());
    }
}

TEST_P(VibratorAidl, PreparePerformNotSupported) {
    if (vibratorIds.empty()) return;
    if (!(capabilities & IVibratorManager::CAP_SYNC)) return;
    if (!(capabilities & IVibratorManager::CAP_PREPARE_ON)) {
        EXPECT_TRUE(manager->prepareSynced(vibratorIds).isOk());
        sp<IVibrator> vibrator;
        for (auto& id : vibratorIds) {
            EXPECT_TRUE(manager->getVibrator(id, &vibrator).isOk());
            ASSERT_NE(vibrator, nullptr);
            int32_t lengthMs = 0;
            Status status = vibrator->perform(kEffects[0], kEffectStrengths[0], nullptr, &lengthMs);
            EXPECT_TRUE(isUnknownOrUnsupported(status)) << status;
        }
        EXPECT_TRUE(manager->cancelSynced().isOk());
    }
}

TEST_P(VibratorAidl, PrepareComposeNotSupported) {
    if (vibratorIds.empty()) return;
    if (!(capabilities & IVibratorManager::CAP_SYNC)) return;
    if (!(capabilities & IVibratorManager::CAP_PREPARE_ON)) {
        std::vector<CompositeEffect> composite;
        CompositeEffect effect;
        effect.delayMs = 10;
        effect.primitive = kPrimitives[0];
        effect.scale = 1.0f;
        composite.emplace_back(effect);

        EXPECT_TRUE(manager->prepareSynced(vibratorIds).isOk());
        sp<IVibrator> vibrator;
        for (auto& id : vibratorIds) {
            EXPECT_TRUE(manager->getVibrator(id, &vibrator).isOk());
            ASSERT_NE(vibrator, nullptr);
            Status status = vibrator->compose(composite, nullptr);
            EXPECT_TRUE(isUnknownOrUnsupported(status)) << status;
        }
        EXPECT_TRUE(manager->cancelSynced().isOk());
    }
}

TEST_P(VibratorAidl, TriggerWithCallback) {
    if (!(capabilities & IVibratorManager::CAP_SYNC)) return;
    if (!(capabilities & IVibratorManager::CAP_PREPARE_ON)) return;
    if (!(capabilities & IVibratorManager::CAP_TRIGGER_CALLBACK)) return;
    if (vibratorIds.empty()) return;

    std::promise<void> completionPromise;
    std::future<void> completionFuture{completionPromise.get_future()};
    sp<CompletionCallback> callback =
            new CompletionCallback([&completionPromise] { completionPromise.set_value(); });
    uint32_t durationMs = 250;
    std::chrono::milliseconds timeout{durationMs * 2};

    EXPECT_TRUE(manager->prepareSynced(vibratorIds).isOk());
    sp<IVibrator> vibrator;
    for (auto& id : vibratorIds) {
        EXPECT_TRUE(manager->getVibrator(id, &vibrator).isOk());
        ASSERT_NE(vibrator, nullptr);
        EXPECT_TRUE(vibrator->on(durationMs, nullptr).isOk());
    }

    EXPECT_TRUE(manager->triggerSynced(callback).isOk());
    EXPECT_EQ(completionFuture.wait_for(timeout), std::future_status::ready);
    EXPECT_TRUE(manager->cancelSynced().isOk());
}

TEST_P(VibratorAidl, TriggerSyncNotSupported) {
    if (!(capabilities & IVibratorManager::CAP_SYNC)) {
        Status status = manager->triggerSynced(nullptr);
        EXPECT_TRUE(isUnknownOrUnsupported(status)) << status;
    }
}

TEST_P(VibratorAidl, TriggerCallbackNotSupported) {
    if (!(capabilities & IVibratorManager::CAP_SYNC)) return;
    if (!(capabilities & IVibratorManager::CAP_TRIGGER_CALLBACK)) {
        sp<CompletionCallback> callback = new CompletionCallback([] {});
        EXPECT_TRUE(manager->prepareSynced(vibratorIds).isOk());
        Status status = manager->triggerSynced(callback);
        EXPECT_TRUE(isUnknownOrUnsupported(status)) << status;
        EXPECT_TRUE(manager->cancelSynced().isOk());
    }
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(VibratorAidl);
INSTANTIATE_TEST_SUITE_P(
        Vibrator, VibratorAidl,
        testing::ValuesIn(android::getAidlHalInstanceNames(IVibratorManager::descriptor)),
        android::PrintInstanceNameToString);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ProcessState::self()->setThreadPoolMaxThreadCount(1);
    ProcessState::self()->startThreadPool();
    return RUN_ALL_TESTS();
}
