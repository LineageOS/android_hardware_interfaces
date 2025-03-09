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
#include <aidl/android/hardware/vibrator/BnVibratorCallback.h>
#include <aidl/android/hardware/vibrator/IVibrationSession.h>
#include <aidl/android/hardware/vibrator/IVibrator.h>
#include <aidl/android/hardware/vibrator/IVibratorManager.h>

#include <android/binder_manager.h>
#include <android/binder_process.h>

#include <algorithm>
#include <cmath>
#include <future>

#include "test_utils.h"

using aidl::android::hardware::vibrator::BnVibratorCallback;
using aidl::android::hardware::vibrator::CompositeEffect;
using aidl::android::hardware::vibrator::CompositePrimitive;
using aidl::android::hardware::vibrator::Effect;
using aidl::android::hardware::vibrator::EffectStrength;
using aidl::android::hardware::vibrator::IVibrationSession;
using aidl::android::hardware::vibrator::IVibrator;
using aidl::android::hardware::vibrator::IVibratorManager;
using aidl::android::hardware::vibrator::VibrationSessionConfig;
using std::chrono::high_resolution_clock;

using namespace ::std::chrono_literals;

const std::vector<Effect> kEffects{ndk::enum_range<Effect>().begin(),
                                   ndk::enum_range<Effect>().end()};
const std::vector<EffectStrength> kEffectStrengths{ndk::enum_range<EffectStrength>().begin(),
                                                   ndk::enum_range<EffectStrength>().end()};
const std::vector<CompositePrimitive> kPrimitives{ndk::enum_range<CompositePrimitive>().begin(),
                                                  ndk::enum_range<CompositePrimitive>().end()};

// Timeout to wait for vibration callback completion.
static constexpr std::chrono::milliseconds VIBRATION_CALLBACK_TIMEOUT = 100ms;

static constexpr int32_t VIBRATION_SESSIONS_MIN_VERSION = 3;

class CompletionCallback : public BnVibratorCallback {
  public:
    CompletionCallback(const std::function<void()>& callback) : mCallback(callback) {}
    ndk::ScopedAStatus onComplete() override {
        mCallback();
        return ndk::ScopedAStatus::ok();
    }

  private:
    std::function<void()> mCallback;
};

class VibratorAidl : public testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        auto serviceName = GetParam().c_str();
        manager = IVibratorManager::fromBinder(
                ndk::SpAIBinder(AServiceManager_waitForService(serviceName)));
        ASSERT_NE(manager, nullptr);
        EXPECT_OK(manager->getCapabilities(&capabilities));
        EXPECT_OK(manager->getVibratorIds(&vibratorIds));
        EXPECT_OK(manager->getInterfaceVersion(&version));
    }

    virtual void TearDown() override {
        // Reset manager state between tests.
        if (capabilities & IVibratorManager::CAP_SYNC) {
            manager->cancelSynced();
        }
        if (capabilities & IVibratorManager::CAP_START_SESSIONS) {
            manager->clearSessions();
        }
        // Reset all managed vibrators.
        for (int32_t id : vibratorIds) {
            std::shared_ptr<IVibrator> vibrator;
            EXPECT_OK(manager->getVibrator(id, &vibrator));
            ASSERT_NE(vibrator, nullptr);
            EXPECT_OK(vibrator->off());
        }
    }

    std::shared_ptr<IVibratorManager> manager;
    std::shared_ptr<IVibrationSession> session;
    int32_t version;
    int32_t capabilities;
    std::vector<int32_t> vibratorIds;
};

TEST_P(VibratorAidl, ValidateExistingVibrators) {
    std::shared_ptr<IVibrator> vibrator;
    for (int32_t id : vibratorIds) {
        EXPECT_OK(manager->getVibrator(id, &vibrator));
        ASSERT_NE(vibrator, nullptr);
    }
}

TEST_P(VibratorAidl, GetVibratorWithInvalidId) {
    int32_t invalidId = *max_element(vibratorIds.begin(), vibratorIds.end()) + 1;
    std::shared_ptr<IVibrator> vibrator;
    EXPECT_ILLEGAL_ARGUMENT(manager->getVibrator(invalidId, &vibrator));
    ASSERT_EQ(vibrator, nullptr);
}

TEST_P(VibratorAidl, ValidatePrepareSyncedExistingVibrators) {
    if (!(capabilities & IVibratorManager::CAP_SYNC)) return;
    if (vibratorIds.empty()) return;
    EXPECT_OK(manager->prepareSynced(vibratorIds));
    EXPECT_OK(manager->cancelSynced());
}

TEST_P(VibratorAidl, PrepareSyncedEmptySetIsInvalid) {
    if (!(capabilities & IVibratorManager::CAP_SYNC)) return;
    std::vector<int32_t> emptyIds;
    EXPECT_ILLEGAL_ARGUMENT(manager->prepareSynced(emptyIds));
}

TEST_P(VibratorAidl, PrepareSyncedNotSupported) {
    if (!(capabilities & IVibratorManager::CAP_SYNC)) {
        EXPECT_UNKNOWN_OR_UNSUPPORTED(manager->prepareSynced(vibratorIds));
    }
}

TEST_P(VibratorAidl, PrepareOnNotSupported) {
    if (vibratorIds.empty()) return;
    if (!(capabilities & IVibratorManager::CAP_SYNC)) return;
    if (!(capabilities & IVibratorManager::CAP_PREPARE_ON)) {
        int32_t durationMs = 250;
        EXPECT_OK(manager->prepareSynced(vibratorIds));
        std::shared_ptr<IVibrator> vibrator;
        for (int32_t id : vibratorIds) {
            EXPECT_OK(manager->getVibrator(id, &vibrator));
            ASSERT_NE(vibrator, nullptr);
            EXPECT_UNKNOWN_OR_UNSUPPORTED(vibrator->on(durationMs, nullptr));
        }
        EXPECT_OK(manager->cancelSynced());
    }
}

TEST_P(VibratorAidl, PreparePerformNotSupported) {
    if (vibratorIds.empty()) return;
    if (!(capabilities & IVibratorManager::CAP_SYNC)) return;
    if (!(capabilities & IVibratorManager::CAP_PREPARE_ON)) {
        EXPECT_OK(manager->prepareSynced(vibratorIds));
        std::shared_ptr<IVibrator> vibrator;
        for (int32_t id : vibratorIds) {
            EXPECT_OK(manager->getVibrator(id, &vibrator));
            ASSERT_NE(vibrator, nullptr);
            int32_t lengthMs = 0;
            EXPECT_UNKNOWN_OR_UNSUPPORTED(
                    vibrator->perform(kEffects[0], kEffectStrengths[0], nullptr, &lengthMs));
        }
        EXPECT_OK(manager->cancelSynced());
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

        EXPECT_OK(manager->prepareSynced(vibratorIds));
        std::shared_ptr<IVibrator> vibrator;
        for (int32_t id : vibratorIds) {
            EXPECT_OK(manager->getVibrator(id, &vibrator));
            ASSERT_NE(vibrator, nullptr);
            EXPECT_UNKNOWN_OR_UNSUPPORTED(vibrator->compose(composite, nullptr));
        }
        EXPECT_OK(manager->cancelSynced());
    }
}

TEST_P(VibratorAidl, TriggerWithCallback) {
    if (!(capabilities & IVibratorManager::CAP_SYNC)) return;
    if (!(capabilities & IVibratorManager::CAP_PREPARE_ON)) return;
    if (!(capabilities & IVibratorManager::CAP_TRIGGER_CALLBACK)) return;
    if (vibratorIds.empty()) return;

    std::promise<void> completionPromise;
    std::future<void> completionFuture{completionPromise.get_future()};
    auto callback = ndk::SharedRefBase::make<CompletionCallback>(
            [&completionPromise] { completionPromise.set_value(); });
    int32_t durationMs = 250;
    std::chrono::milliseconds timeout{durationMs * 2};

    EXPECT_OK(manager->prepareSynced(vibratorIds));
    std::shared_ptr<IVibrator> vibrator;
    for (int32_t id : vibratorIds) {
        EXPECT_OK(manager->getVibrator(id, &vibrator));
        ASSERT_NE(vibrator, nullptr);
        EXPECT_OK(vibrator->on(durationMs, nullptr));
    }

    EXPECT_OK(manager->triggerSynced(callback));
    EXPECT_EQ(completionFuture.wait_for(timeout), std::future_status::ready);
    EXPECT_OK(manager->cancelSynced());
}

TEST_P(VibratorAidl, TriggerSyncNotSupported) {
    if (!(capabilities & IVibratorManager::CAP_SYNC)) {
        EXPECT_UNKNOWN_OR_UNSUPPORTED(manager->triggerSynced(nullptr));
    }
}

TEST_P(VibratorAidl, TriggerCallbackNotSupported) {
    if (!(capabilities & IVibratorManager::CAP_SYNC)) return;
    if (!(capabilities & IVibratorManager::CAP_TRIGGER_CALLBACK)) {
        auto callback = ndk::SharedRefBase::make<CompletionCallback>([] {});
        EXPECT_OK(manager->prepareSynced(vibratorIds));
        EXPECT_UNKNOWN_OR_UNSUPPORTED(manager->triggerSynced(callback));
        EXPECT_OK(manager->cancelSynced());
    }
}

TEST_P(VibratorAidl, VibrationSessionsSupported) {
    if (!(capabilities & IVibratorManager::CAP_START_SESSIONS)) return;
    if (vibratorIds.empty()) return;

    std::promise<void> sessionPromise;
    std::future<void> sessionFuture{sessionPromise.get_future()};
    auto sessionCallback = ndk::SharedRefBase::make<CompletionCallback>(
            [&sessionPromise] { sessionPromise.set_value(); });

    VibrationSessionConfig sessionConfig;
    EXPECT_OK(manager->startSession(vibratorIds, sessionConfig, sessionCallback, &session));
    ASSERT_NE(session, nullptr);

    int32_t durationMs = 250;
    std::vector<std::promise<void>> vibrationPromises;
    std::vector<std::future<void>> vibrationFutures;
    for (int32_t id : vibratorIds) {
        std::shared_ptr<IVibrator> vibrator;
        EXPECT_OK(manager->getVibrator(id, &vibrator));
        ASSERT_NE(vibrator, nullptr);

        std::promise<void>& vibrationPromise = vibrationPromises.emplace_back();
        vibrationFutures.push_back(vibrationPromise.get_future());
        auto vibrationCallback = ndk::SharedRefBase::make<CompletionCallback>(
                [&vibrationPromise] { vibrationPromise.set_value(); });
        EXPECT_OK(vibrator->on(durationMs, vibrationCallback));
    }

    auto timeout = std::chrono::milliseconds(durationMs) + VIBRATION_CALLBACK_TIMEOUT;
    for (std::future<void>& future : vibrationFutures) {
        EXPECT_EQ(future.wait_for(timeout), std::future_status::ready);
    }

    // Session callback not triggered.
    EXPECT_EQ(sessionFuture.wait_for(VIBRATION_CALLBACK_TIMEOUT), std::future_status::timeout);

    // Ending a session should not take long since the vibration was already completed
    EXPECT_OK(session->close());
    EXPECT_EQ(sessionFuture.wait_for(VIBRATION_CALLBACK_TIMEOUT), std::future_status::ready);
}

TEST_P(VibratorAidl, VibrationSessionInterrupted) {
    if (!(capabilities & IVibratorManager::CAP_START_SESSIONS)) return;
    if (vibratorIds.empty()) return;

    std::promise<void> sessionPromise;
    std::future<void> sessionFuture{sessionPromise.get_future()};
    auto sessionCallback = ndk::SharedRefBase::make<CompletionCallback>(
            [&sessionPromise] { sessionPromise.set_value(); });

    VibrationSessionConfig sessionConfig;
    EXPECT_OK(manager->startSession(vibratorIds, sessionConfig, sessionCallback, &session));
    ASSERT_NE(session, nullptr);

    std::vector<std::promise<void>> vibrationPromises;
    std::vector<std::future<void>> vibrationFutures;
    for (int32_t id : vibratorIds) {
        std::shared_ptr<IVibrator> vibrator;
        EXPECT_OK(manager->getVibrator(id, &vibrator));
        ASSERT_NE(vibrator, nullptr);

        std::promise<void>& vibrationPromise = vibrationPromises.emplace_back();
        vibrationFutures.push_back(vibrationPromise.get_future());
        auto vibrationCallback = ndk::SharedRefBase::make<CompletionCallback>(
                [&vibrationPromise] { vibrationPromise.set_value(); });

        // Vibration longer than test timeout.
        EXPECT_OK(vibrator->on(2000, vibrationCallback));
    }

    // Session callback not triggered.
    EXPECT_EQ(sessionFuture.wait_for(VIBRATION_CALLBACK_TIMEOUT), std::future_status::timeout);

    // Interrupt vibrations and session.
    EXPECT_OK(session->abort());

    // Both callbacks triggered.
    EXPECT_EQ(sessionFuture.wait_for(VIBRATION_CALLBACK_TIMEOUT), std::future_status::ready);
    for (std::future<void>& future : vibrationFutures) {
        EXPECT_EQ(future.wait_for(VIBRATION_CALLBACK_TIMEOUT), std::future_status::ready);
    }
}

TEST_P(VibratorAidl, VibrationSessionEndingInterrupted) {
    if (!(capabilities & IVibratorManager::CAP_START_SESSIONS)) return;
    if (vibratorIds.empty()) return;

    std::promise<void> sessionPromise;
    std::future<void> sessionFuture{sessionPromise.get_future()};
    auto sessionCallback = ndk::SharedRefBase::make<CompletionCallback>(
            [&sessionPromise] { sessionPromise.set_value(); });

    VibrationSessionConfig sessionConfig;
    EXPECT_OK(manager->startSession(vibratorIds, sessionConfig, sessionCallback, &session));
    ASSERT_NE(session, nullptr);

    std::vector<std::promise<void>> vibrationPromises;
    std::vector<std::future<void>> vibrationFutures;
    for (int32_t id : vibratorIds) {
        std::shared_ptr<IVibrator> vibrator;
        EXPECT_OK(manager->getVibrator(id, &vibrator));
        ASSERT_NE(vibrator, nullptr);

        std::promise<void>& vibrationPromise = vibrationPromises.emplace_back();
        vibrationFutures.push_back(vibrationPromise.get_future());
        auto vibrationCallback = ndk::SharedRefBase::make<CompletionCallback>(
                [&vibrationPromise] { vibrationPromise.set_value(); });

        // Vibration longer than test timeout.
        EXPECT_OK(vibrator->on(2000, vibrationCallback));
    }

    // Session callback not triggered.
    EXPECT_EQ(sessionFuture.wait_for(VIBRATION_CALLBACK_TIMEOUT), std::future_status::timeout);

    // End session, this might take a while
    EXPECT_OK(session->close());

    // Interrupt ending session.
    EXPECT_OK(session->abort());

    // Both callbacks triggered.
    EXPECT_EQ(sessionFuture.wait_for(VIBRATION_CALLBACK_TIMEOUT), std::future_status::ready);
    for (std::future<void>& future : vibrationFutures) {
        EXPECT_EQ(future.wait_for(VIBRATION_CALLBACK_TIMEOUT), std::future_status::ready);
    }
}

TEST_P(VibratorAidl, VibrationSessionCleared) {
    if (!(capabilities & IVibratorManager::CAP_START_SESSIONS)) return;
    if (vibratorIds.empty()) return;

    std::promise<void> sessionPromise;
    std::future<void> sessionFuture{sessionPromise.get_future()};
    auto sessionCallback = ndk::SharedRefBase::make<CompletionCallback>(
            [&sessionPromise] { sessionPromise.set_value(); });

    VibrationSessionConfig sessionConfig;
    EXPECT_OK(manager->startSession(vibratorIds, sessionConfig, sessionCallback, &session));
    ASSERT_NE(session, nullptr);

    int32_t durationMs = 250;
    std::vector<std::promise<void>> vibrationPromises;
    std::vector<std::future<void>> vibrationFutures;
    for (int32_t id : vibratorIds) {
        std::shared_ptr<IVibrator> vibrator;
        EXPECT_OK(manager->getVibrator(id, &vibrator));
        ASSERT_NE(vibrator, nullptr);

        std::promise<void>& vibrationPromise = vibrationPromises.emplace_back();
        vibrationFutures.push_back(vibrationPromise.get_future());
        auto vibrationCallback = ndk::SharedRefBase::make<CompletionCallback>(
                [&vibrationPromise] { vibrationPromise.set_value(); });
        EXPECT_OK(vibrator->on(durationMs, vibrationCallback));
    }

    // Session callback not triggered.
    EXPECT_EQ(sessionFuture.wait_for(VIBRATION_CALLBACK_TIMEOUT), std::future_status::timeout);

    // Clearing sessions should abort ongoing session
    EXPECT_OK(manager->clearSessions());

    EXPECT_EQ(sessionFuture.wait_for(VIBRATION_CALLBACK_TIMEOUT), std::future_status::ready);
    for (std::future<void>& future : vibrationFutures) {
        EXPECT_EQ(future.wait_for(VIBRATION_CALLBACK_TIMEOUT), std::future_status::ready);
    }
}

TEST_P(VibratorAidl, VibrationSessionsClearedWithoutSession) {
    if (!(capabilities & IVibratorManager::CAP_START_SESSIONS)) return;

    EXPECT_OK(manager->clearSessions());
}

TEST_P(VibratorAidl, VibrationSessionsWithSyncedVibrations) {
    if (!(capabilities & IVibratorManager::CAP_START_SESSIONS)) return;
    if (!(capabilities & IVibratorManager::CAP_SYNC)) return;
    if (!(capabilities & IVibratorManager::CAP_PREPARE_ON)) return;
    if (!(capabilities & IVibratorManager::CAP_TRIGGER_CALLBACK)) return;
    if (vibratorIds.empty()) return;

    std::promise<void> sessionPromise;
    std::future<void> sessionFuture{sessionPromise.get_future()};
    auto sessionCallback = ndk::SharedRefBase::make<CompletionCallback>(
            [&sessionPromise] { sessionPromise.set_value(); });

    VibrationSessionConfig sessionConfig;
    EXPECT_OK(manager->startSession(vibratorIds, sessionConfig, sessionCallback, &session));
    ASSERT_NE(session, nullptr);

    EXPECT_OK(manager->prepareSynced(vibratorIds));

    int32_t durationMs = 250;
    std::vector<std::promise<void>> vibrationPromises;
    std::vector<std::future<void>> vibrationFutures;
    for (int32_t id : vibratorIds) {
        std::shared_ptr<IVibrator> vibrator;
        EXPECT_OK(manager->getVibrator(id, &vibrator));
        ASSERT_NE(vibrator, nullptr);

        std::promise<void>& vibrationPromise = vibrationPromises.emplace_back();
        vibrationFutures.push_back(vibrationPromise.get_future());
        auto vibrationCallback = ndk::SharedRefBase::make<CompletionCallback>(
                [&vibrationPromise] { vibrationPromise.set_value(); });
        EXPECT_OK(vibrator->on(durationMs, vibrationCallback));
    }

    std::promise<void> triggerPromise;
    std::future<void> triggerFuture{triggerPromise.get_future()};
    auto triggerCallback = ndk::SharedRefBase::make<CompletionCallback>(
            [&triggerPromise] { triggerPromise.set_value(); });

    EXPECT_OK(manager->triggerSynced(triggerCallback));

    auto timeout = std::chrono::milliseconds(durationMs) + VIBRATION_CALLBACK_TIMEOUT;
    EXPECT_EQ(triggerFuture.wait_for(timeout), std::future_status::ready);
    for (std::future<void>& future : vibrationFutures) {
        EXPECT_EQ(future.wait_for(timeout), std::future_status::ready);
    }

    // Session callback not triggered.
    EXPECT_EQ(sessionFuture.wait_for(VIBRATION_CALLBACK_TIMEOUT), std::future_status::timeout);

    // Ending a session should not take long since the vibration was already completed
    EXPECT_OK(session->close());
    EXPECT_EQ(sessionFuture.wait_for(VIBRATION_CALLBACK_TIMEOUT), std::future_status::ready);
}

TEST_P(VibratorAidl, VibrationSessionWithMultipleIndependentVibrations) {
    if (!(capabilities & IVibratorManager::CAP_START_SESSIONS)) return;
    if (vibratorIds.empty()) return;

    std::promise<void> sessionPromise;
    std::future<void> sessionFuture{sessionPromise.get_future()};
    auto sessionCallback = ndk::SharedRefBase::make<CompletionCallback>(
            [&sessionPromise] { sessionPromise.set_value(); });

    VibrationSessionConfig sessionConfig;
    EXPECT_OK(manager->startSession(vibratorIds, sessionConfig, sessionCallback, &session));
    ASSERT_NE(session, nullptr);

    for (int32_t id : vibratorIds) {
        std::shared_ptr<IVibrator> vibrator;
        EXPECT_OK(manager->getVibrator(id, &vibrator));
        ASSERT_NE(vibrator, nullptr);

        EXPECT_OK(vibrator->on(100, nullptr));
        EXPECT_OK(vibrator->on(200, nullptr));
        EXPECT_OK(vibrator->on(300, nullptr));
    }

    // Session callback not triggered.
    EXPECT_EQ(sessionFuture.wait_for(VIBRATION_CALLBACK_TIMEOUT), std::future_status::timeout);

    EXPECT_OK(session->close());

    int32_t maxDurationMs = 100 + 200 + 300;
    auto timeout = std::chrono::milliseconds(maxDurationMs) + VIBRATION_CALLBACK_TIMEOUT;
    EXPECT_EQ(sessionFuture.wait_for(timeout), std::future_status::ready);
}

TEST_P(VibratorAidl, VibrationSessionsIgnoresSecondSessionWhenFirstIsOngoing) {
    if (!(capabilities & IVibratorManager::CAP_START_SESSIONS)) return;
    if (vibratorIds.empty()) return;

    std::promise<void> sessionPromise;
    std::future<void> sessionFuture{sessionPromise.get_future()};
    auto sessionCallback = ndk::SharedRefBase::make<CompletionCallback>(
            [&sessionPromise] { sessionPromise.set_value(); });

    VibrationSessionConfig sessionConfig;
    EXPECT_OK(manager->startSession(vibratorIds, sessionConfig, sessionCallback, &session));
    ASSERT_NE(session, nullptr);

    std::shared_ptr<IVibrationSession> secondSession;
    EXPECT_ILLEGAL_STATE(
            manager->startSession(vibratorIds, sessionConfig, nullptr, &secondSession));
    EXPECT_EQ(secondSession, nullptr);

    // First session was not cancelled.
    EXPECT_EQ(sessionFuture.wait_for(VIBRATION_CALLBACK_TIMEOUT), std::future_status::timeout);

    // First session still ongoing, we can still vibrate.
    int32_t durationMs = 100;
    for (int32_t id : vibratorIds) {
        std::shared_ptr<IVibrator> vibrator;
        EXPECT_OK(manager->getVibrator(id, &vibrator));
        ASSERT_NE(vibrator, nullptr);
        EXPECT_OK(vibrator->on(durationMs, nullptr));
    }

    EXPECT_OK(session->close());

    auto timeout = std::chrono::milliseconds(durationMs) + VIBRATION_CALLBACK_TIMEOUT;
    EXPECT_EQ(sessionFuture.wait_for(timeout), std::future_status::ready);
}

TEST_P(VibratorAidl, VibrationSessionEndMultipleTimes) {
    if (!(capabilities & IVibratorManager::CAP_START_SESSIONS)) return;
    if (vibratorIds.empty()) return;

    std::promise<void> sessionPromise;
    std::future<void> sessionFuture{sessionPromise.get_future()};
    auto sessionCallback = ndk::SharedRefBase::make<CompletionCallback>(
            [&sessionPromise] { sessionPromise.set_value(); });

    VibrationSessionConfig sessionConfig;
    EXPECT_OK(manager->startSession(vibratorIds, sessionConfig, sessionCallback, &session));
    ASSERT_NE(session, nullptr);

    int32_t durationMs = 250;
    std::vector<std::promise<void>> vibrationPromises;
    std::vector<std::future<void>> vibrationFutures;
    for (int32_t id : vibratorIds) {
        std::shared_ptr<IVibrator> vibrator;
        EXPECT_OK(manager->getVibrator(id, &vibrator));
        ASSERT_NE(vibrator, nullptr);

        std::promise<void>& vibrationPromise = vibrationPromises.emplace_back();
        vibrationFutures.push_back(vibrationPromise.get_future());
        auto vibrationCallback = ndk::SharedRefBase::make<CompletionCallback>(
                [&vibrationPromise] { vibrationPromise.set_value(); });
        EXPECT_OK(vibrator->on(durationMs, vibrationCallback));
    }

    // Session callback not triggered.
    EXPECT_EQ(sessionFuture.wait_for(VIBRATION_CALLBACK_TIMEOUT), std::future_status::timeout);

    // End session, this might take a while
    EXPECT_OK(session->close());

    // End session again
    EXPECT_OK(session->close());

    // Both callbacks triggered within timeout.
    auto timeout = std::chrono::milliseconds(durationMs) + VIBRATION_CALLBACK_TIMEOUT;
    EXPECT_EQ(sessionFuture.wait_for(timeout), std::future_status::ready);
    for (std::future<void>& future : vibrationFutures) {
        EXPECT_EQ(future.wait_for(timeout), std::future_status::ready);
    }
}

TEST_P(VibratorAidl, VibrationSessionDeletedAfterEnded) {
    if (!(capabilities & IVibratorManager::CAP_START_SESSIONS)) return;
    if (vibratorIds.empty()) return;

    std::promise<void> sessionPromise;
    std::future<void> sessionFuture{sessionPromise.get_future()};
    auto sessionCallback = ndk::SharedRefBase::make<CompletionCallback>(
            [&sessionPromise] { sessionPromise.set_value(); });

    VibrationSessionConfig sessionConfig;
    EXPECT_OK(manager->startSession(vibratorIds, sessionConfig, sessionCallback, &session));
    ASSERT_NE(session, nullptr);

    int32_t durationMs = 250;
    std::vector<std::promise<void>> vibrationPromises;
    std::vector<std::future<void>> vibrationFutures;
    for (int32_t id : vibratorIds) {
        std::shared_ptr<IVibrator> vibrator;
        EXPECT_OK(manager->getVibrator(id, &vibrator));
        ASSERT_NE(vibrator, nullptr);

        std::promise<void>& vibrationPromise = vibrationPromises.emplace_back();
        vibrationFutures.push_back(vibrationPromise.get_future());
        auto vibrationCallback = ndk::SharedRefBase::make<CompletionCallback>(
                [&vibrationPromise] { vibrationPromise.set_value(); });
        EXPECT_OK(vibrator->on(durationMs, vibrationCallback));
    }

    // Session callback not triggered.
    EXPECT_EQ(sessionFuture.wait_for(VIBRATION_CALLBACK_TIMEOUT), std::future_status::timeout);

    // End session, this might take a while
    EXPECT_OK(session->close());

    session.reset();

    // Both callbacks triggered within timeout, even after session was deleted.
    auto timeout = std::chrono::milliseconds(durationMs) + VIBRATION_CALLBACK_TIMEOUT;
    EXPECT_EQ(sessionFuture.wait_for(timeout), std::future_status::ready);
    for (std::future<void>& future : vibrationFutures) {
        EXPECT_EQ(future.wait_for(timeout), std::future_status::ready);
    }
}

TEST_P(VibratorAidl, VibrationSessionWrongVibratorIdsFail) {
    if (!(capabilities & IVibratorManager::CAP_START_SESSIONS)) return;

    auto maxIdIt = std::max_element(vibratorIds.begin(), vibratorIds.end());
    int32_t wrongId = maxIdIt == vibratorIds.end() ? 0 : *maxIdIt + 1;

    std::vector<int32_t> emptyIds;
    std::vector<int32_t> wrongIds{wrongId};
    VibrationSessionConfig sessionConfig;
    EXPECT_ILLEGAL_ARGUMENT(manager->startSession(emptyIds, sessionConfig, nullptr, &session));
    EXPECT_ILLEGAL_ARGUMENT(manager->startSession(wrongIds, sessionConfig, nullptr, &session));
    EXPECT_EQ(session, nullptr);
}

TEST_P(VibratorAidl, VibrationSessionDuringPrepareSyncedFails) {
    if (!(capabilities & IVibratorManager::CAP_SYNC)) return;
    if (!(capabilities & IVibratorManager::CAP_START_SESSIONS)) return;
    if (vibratorIds.empty()) return;

    EXPECT_OK(manager->prepareSynced(vibratorIds));

    VibrationSessionConfig sessionConfig;
    EXPECT_ILLEGAL_STATE(manager->startSession(vibratorIds, sessionConfig, nullptr, &session));
    EXPECT_EQ(session, nullptr);

    EXPECT_OK(manager->cancelSynced());
}

TEST_P(VibratorAidl, VibrationSessionsUnsupported) {
    if (version < VIBRATION_SESSIONS_MIN_VERSION) {
        EXPECT_EQ(capabilities & IVibratorManager::CAP_START_SESSIONS, 0)
                << "Vibrator manager version " << version
                << " should not report start session capability";
    }
    if (capabilities & IVibratorManager::CAP_START_SESSIONS) return;

    VibrationSessionConfig sessionConfig;
    EXPECT_UNKNOWN_OR_UNSUPPORTED(
            manager->startSession(vibratorIds, sessionConfig, nullptr, &session));
    EXPECT_EQ(session, nullptr);
    EXPECT_UNKNOWN_OR_UNSUPPORTED(manager->clearSessions());
}

std::vector<std::string> FindVibratorManagerNames() {
    std::vector<std::string> names;
    constexpr auto callback = [](const char* instance, void* context) {
        std::string fullName = std::string(IVibratorManager::descriptor) + "/" + instance;
        static_cast<std::vector<std::string>*>(context)->emplace_back(fullName);
    };
    AServiceManager_forEachDeclaredInstance(IVibratorManager::descriptor,
                                            static_cast<void*>(&names), callback);
    return names;
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(VibratorAidl);
INSTANTIATE_TEST_SUITE_P(Vibrator, VibratorAidl, testing::ValuesIn(FindVibratorManagerNames()),
                         android::PrintInstanceNameToString);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ABinderProcess_setThreadPoolMaxThreadCount(2);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
