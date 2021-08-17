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
using android::hardware::vibrator::ActivePwle;
using android::hardware::vibrator::BnVibratorCallback;
using android::hardware::vibrator::Braking;
using android::hardware::vibrator::BrakingPwle;
using android::hardware::vibrator::CompositeEffect;
using android::hardware::vibrator::CompositePrimitive;
using android::hardware::vibrator::Effect;
using android::hardware::vibrator::EffectStrength;
using android::hardware::vibrator::IVibrator;
using android::hardware::vibrator::IVibratorManager;
using android::hardware::vibrator::PrimitivePwle;
using std::chrono::high_resolution_clock;

const std::vector<Effect> kEffects{android::enum_range<Effect>().begin(),
                                   android::enum_range<Effect>().end()};
const std::vector<EffectStrength> kEffectStrengths{android::enum_range<EffectStrength>().begin(),
                                                   android::enum_range<EffectStrength>().end()};

const std::vector<Effect> kInvalidEffects = {
    static_cast<Effect>(static_cast<int32_t>(kEffects.front()) - 1),
    static_cast<Effect>(static_cast<int32_t>(kEffects.back()) + 1),
};

const std::vector<EffectStrength> kInvalidEffectStrengths = {
    static_cast<EffectStrength>(static_cast<int8_t>(kEffectStrengths.front()) - 1),
    static_cast<EffectStrength>(static_cast<int8_t>(kEffectStrengths.back()) + 1),
};

const std::vector<CompositePrimitive> kCompositePrimitives{
    android::enum_range<CompositePrimitive>().begin(),
    android::enum_range<CompositePrimitive>().end()};

const std::vector<CompositePrimitive> kRequiredPrimitives = {
        CompositePrimitive::CLICK,      CompositePrimitive::LIGHT_TICK,
        CompositePrimitive::QUICK_RISE, CompositePrimitive::SLOW_RISE,
        CompositePrimitive::QUICK_FALL,
};

const std::vector<CompositePrimitive> kInvalidPrimitives = {
    static_cast<CompositePrimitive>(static_cast<int32_t>(kCompositePrimitives.front()) - 1),
    static_cast<CompositePrimitive>(static_cast<int32_t>(kCompositePrimitives.back()) + 1),
};

class CompletionCallback : public BnVibratorCallback {
  public:
    CompletionCallback(const std::function<void()> &callback) : mCallback(callback) {}
    Status onComplete() override {
        mCallback();
        return Status::ok();
    }

  private:
    std::function<void()> mCallback;
};

class VibratorAidl : public testing::TestWithParam<std::tuple<int32_t, int32_t>> {
  public:
    virtual void SetUp() override {
        int32_t managerIdx = std::get<0>(GetParam());
        int32_t vibratorId = std::get<1>(GetParam());
        auto managerAidlNames = android::getAidlHalInstanceNames(IVibratorManager::descriptor);

        if (managerIdx < 0) {
            // Testing a unmanaged vibrator, using vibratorId as index from registered HALs
            auto vibratorAidlNames = android::getAidlHalInstanceNames(IVibrator::descriptor);
            ASSERT_LT(vibratorId, vibratorAidlNames.size());
            auto vibratorName = String16(vibratorAidlNames[vibratorId].c_str());
            vibrator = android::waitForDeclaredService<IVibrator>(vibratorName);
        } else {
            // Testing a managed vibrator, using vibratorId to retrieve it from the manager
            ASSERT_LT(managerIdx, managerAidlNames.size());
            auto managerName = String16(managerAidlNames[managerIdx].c_str());
            auto vibratorManager = android::waitForDeclaredService<IVibratorManager>(managerName);
            auto vibratorResult = vibratorManager->getVibrator(vibratorId, &vibrator);
            ASSERT_TRUE(vibratorResult.isOk());
        }

        ASSERT_NE(vibrator, nullptr);
        ASSERT_TRUE(vibrator->getCapabilities(&capabilities).isOk());
    }

    sp<IVibrator> vibrator;
    int32_t capabilities;
};

inline bool isUnknownOrUnsupported(Status status) {
    return status.exceptionCode() == Status::EX_UNSUPPORTED_OPERATION ||
           status.transactionError() == android::UNKNOWN_TRANSACTION;
}

static float getResonantFrequencyHz(sp<IVibrator> vibrator, int32_t capabilities) {
    float resonantFrequencyHz;
    Status status = vibrator->getResonantFrequency(&resonantFrequencyHz);
    if (capabilities & IVibrator::CAP_GET_RESONANT_FREQUENCY) {
        EXPECT_GT(resonantFrequencyHz, 0);
        EXPECT_EQ(status.exceptionCode(), Status::EX_NONE);
    } else {
        EXPECT_TRUE(isUnknownOrUnsupported(status)) << status;
    }
    return resonantFrequencyHz;
}

static float getFrequencyResolutionHz(sp<IVibrator> vibrator, int32_t capabilities) {
    float freqResolutionHz;
    Status status = vibrator->getFrequencyResolution(&freqResolutionHz);
    if (capabilities & IVibrator::CAP_FREQUENCY_CONTROL) {
        EXPECT_GT(freqResolutionHz, 0);
        EXPECT_EQ(status.exceptionCode(), Status::EX_NONE);
    } else {
        EXPECT_TRUE(isUnknownOrUnsupported(status)) << status;
    }
    return freqResolutionHz;
}

static float getFrequencyMinimumHz(sp<IVibrator> vibrator, int32_t capabilities) {
    float freqMinimumHz;
    Status status = vibrator->getFrequencyMinimum(&freqMinimumHz);
    if (capabilities & IVibrator::CAP_FREQUENCY_CONTROL) {
        EXPECT_EQ(status.exceptionCode(), Status::EX_NONE);

        float resonantFrequencyHz = getResonantFrequencyHz(vibrator, capabilities);

        EXPECT_GT(freqMinimumHz, 0);
        EXPECT_LE(freqMinimumHz, resonantFrequencyHz);
    } else {
        EXPECT_TRUE(isUnknownOrUnsupported(status)) << status;
    }
    return freqMinimumHz;
}

static float getFrequencyMaximumHz(sp<IVibrator> vibrator, int32_t capabilities) {
    std::vector<float> bandwidthAmplitudeMap;
    Status status = vibrator->getBandwidthAmplitudeMap(&bandwidthAmplitudeMap);
    if (capabilities & IVibrator::CAP_FREQUENCY_CONTROL) {
        EXPECT_EQ(status.exceptionCode(), Status::EX_NONE);
    } else {
        EXPECT_TRUE(isUnknownOrUnsupported(status)) << status;
    }

    float freqMaximumHz =
        (bandwidthAmplitudeMap.size() * getFrequencyResolutionHz(vibrator, capabilities)) +
        getFrequencyMinimumHz(vibrator, capabilities);
    return freqMaximumHz;
}

static float getAmplitudeMin() {
    return 0.0;
}

static float getAmplitudeMax() {
    return 1.0;
}

static ActivePwle composeValidActivePwle(sp<IVibrator> vibrator, int32_t capabilities) {
    float frequencyHz;
    if (capabilities & IVibrator::CAP_GET_RESONANT_FREQUENCY) {
        frequencyHz = getResonantFrequencyHz(vibrator, capabilities);
    } else if (capabilities & IVibrator::CAP_FREQUENCY_CONTROL) {
        frequencyHz = getFrequencyMinimumHz(vibrator, capabilities);
    } else {
        frequencyHz = 150.0;  // default value commonly used
    }

    ActivePwle active;
    active.startAmplitude = (getAmplitudeMin() + getAmplitudeMax()) / 2;
    active.startFrequency = frequencyHz;
    active.endAmplitude = (getAmplitudeMin() + getAmplitudeMax()) / 2;
    active.endFrequency = frequencyHz;
    active.duration = 1000;

    return active;
}

TEST_P(VibratorAidl, OnThenOffBeforeTimeout) {
    EXPECT_TRUE(vibrator->on(2000, nullptr /*callback*/).isOk());
    sleep(1);
    EXPECT_TRUE(vibrator->off().isOk());
}

TEST_P(VibratorAidl, OnWithCallback) {
    if (!(capabilities & IVibrator::CAP_ON_CALLBACK))
        return;

    std::promise<void> completionPromise;
    std::future<void> completionFuture{completionPromise.get_future()};
    sp<CompletionCallback> callback =
        new CompletionCallback([&completionPromise] { completionPromise.set_value(); });
    uint32_t durationMs = 250;
    std::chrono::milliseconds timeout{durationMs * 2};
    EXPECT_TRUE(vibrator->on(durationMs, callback).isOk());
    EXPECT_EQ(completionFuture.wait_for(timeout), std::future_status::ready);
    EXPECT_TRUE(vibrator->off().isOk());
}

TEST_P(VibratorAidl, OnCallbackNotSupported) {
    if (!(capabilities & IVibrator::CAP_ON_CALLBACK)) {
        sp<CompletionCallback> callback = new CompletionCallback([] {});
        Status status = vibrator->on(250, callback);
        EXPECT_TRUE(isUnknownOrUnsupported(status)) << status;
    }
}

TEST_P(VibratorAidl, ValidateEffect) {
    std::vector<Effect> supported;
    ASSERT_TRUE(vibrator->getSupportedEffects(&supported).isOk());

    for (Effect effect : kEffects) {
        bool isEffectSupported =
            std::find(supported.begin(), supported.end(), effect) != supported.end();

        for (EffectStrength strength : kEffectStrengths) {
            int32_t lengthMs = 0;
            Status status = vibrator->perform(effect, strength, nullptr /*callback*/, &lengthMs);

            if (isEffectSupported) {
                EXPECT_TRUE(status.isOk()) << toString(effect) << " " << toString(strength);
                EXPECT_GT(lengthMs, 0);
                usleep(lengthMs * 1000);
            } else {
                EXPECT_TRUE(isUnknownOrUnsupported(status))
                        << status << " " << toString(effect) << " " << toString(strength);
            }
        }
    }
}

TEST_P(VibratorAidl, ValidateEffectWithCallback) {
    if (!(capabilities & IVibrator::CAP_PERFORM_CALLBACK))
        return;

    std::vector<Effect> supported;
    ASSERT_TRUE(vibrator->getSupportedEffects(&supported).isOk());

    for (Effect effect : kEffects) {
        bool isEffectSupported =
            std::find(supported.begin(), supported.end(), effect) != supported.end();

        for (EffectStrength strength : kEffectStrengths) {
            std::promise<void> completionPromise;
            std::future<void> completionFuture{completionPromise.get_future()};
            sp<CompletionCallback> callback =
                new CompletionCallback([&completionPromise] { completionPromise.set_value(); });
            int lengthMs = 0;
            Status status = vibrator->perform(effect, strength, callback, &lengthMs);

            if (isEffectSupported) {
                EXPECT_TRUE(status.isOk());
                EXPECT_GT(lengthMs, 0);
            } else {
                EXPECT_TRUE(isUnknownOrUnsupported(status)) << status;
            }

            if (!status.isOk())
                continue;

            //TODO(b/187207798): revert back to conservative timeout values once
            //latencies have been fixed
            std::chrono::milliseconds timeout{lengthMs * 8};
            EXPECT_EQ(completionFuture.wait_for(timeout), std::future_status::ready);
        }
    }
}

TEST_P(VibratorAidl, ValidateEffectWithCallbackNotSupported) {
    if (capabilities & IVibrator::CAP_PERFORM_CALLBACK)
        return;

    for (Effect effect : kEffects) {
        for (EffectStrength strength : kEffectStrengths) {
            sp<CompletionCallback> callback = new CompletionCallback([] {});
            int lengthMs;
            Status status = vibrator->perform(effect, strength, callback, &lengthMs);
            EXPECT_TRUE(isUnknownOrUnsupported(status)) << status;
        }
    }
}

TEST_P(VibratorAidl, InvalidEffectsUnsupported) {
    for (Effect effect : kInvalidEffects) {
        for (EffectStrength strength : kEffectStrengths) {
            int32_t lengthMs;
            Status status = vibrator->perform(effect, strength, nullptr /*callback*/, &lengthMs);
            EXPECT_TRUE(isUnknownOrUnsupported(status))
                    << status << toString(effect) << " " << toString(strength);
        }
    }
    for (Effect effect : kEffects) {
        for (EffectStrength strength : kInvalidEffectStrengths) {
            int32_t lengthMs;
            Status status = vibrator->perform(effect, strength, nullptr /*callback*/, &lengthMs);
            EXPECT_TRUE(isUnknownOrUnsupported(status))
                    << status << " " << toString(effect) << " " << toString(strength);
        }
    }
}

TEST_P(VibratorAidl, ChangeVibrationAmplitude) {
    if (capabilities & IVibrator::CAP_AMPLITUDE_CONTROL) {
        EXPECT_EQ(Status::EX_NONE, vibrator->setAmplitude(0.1f).exceptionCode());
        EXPECT_TRUE(vibrator->on(2000, nullptr /*callback*/).isOk());
        EXPECT_EQ(Status::EX_NONE, vibrator->setAmplitude(0.5f).exceptionCode());
        sleep(1);
        EXPECT_EQ(Status::EX_NONE, vibrator->setAmplitude(1.0f).exceptionCode());
        sleep(1);
    }
}

TEST_P(VibratorAidl, AmplitudeOutsideRangeFails) {
    if (capabilities & IVibrator::CAP_AMPLITUDE_CONTROL) {
        EXPECT_EQ(Status::EX_ILLEGAL_ARGUMENT, vibrator->setAmplitude(-1).exceptionCode());
        EXPECT_EQ(Status::EX_ILLEGAL_ARGUMENT, vibrator->setAmplitude(0).exceptionCode());
        EXPECT_EQ(Status::EX_ILLEGAL_ARGUMENT, vibrator->setAmplitude(1.1).exceptionCode());
    }
}

TEST_P(VibratorAidl, AmplitudeReturnsUnsupportedMatchingCapabilities) {
    if ((capabilities & IVibrator::CAP_AMPLITUDE_CONTROL) == 0) {
        Status status = vibrator->setAmplitude(1);
        EXPECT_TRUE(isUnknownOrUnsupported(status)) << status;
    }
}

TEST_P(VibratorAidl, ChangeVibrationExternalControl) {
    if (capabilities & IVibrator::CAP_EXTERNAL_CONTROL) {
        EXPECT_TRUE(vibrator->setExternalControl(true).isOk());
        sleep(1);
        EXPECT_TRUE(vibrator->setExternalControl(false).isOk());
        sleep(1);
    }
}

TEST_P(VibratorAidl, ExternalAmplitudeControl) {
    const bool supportsExternalAmplitudeControl =
        (capabilities & IVibrator::CAP_EXTERNAL_AMPLITUDE_CONTROL) > 0;

    if (capabilities & IVibrator::CAP_EXTERNAL_CONTROL) {
        EXPECT_TRUE(vibrator->setExternalControl(true).isOk());

        Status amplitudeStatus = vibrator->setAmplitude(0.5);
        if (supportsExternalAmplitudeControl) {
            EXPECT_TRUE(amplitudeStatus.isOk());
        } else {
            EXPECT_TRUE(isUnknownOrUnsupported(amplitudeStatus)) << amplitudeStatus;
        }
        EXPECT_TRUE(vibrator->setExternalControl(false).isOk());
    } else {
        EXPECT_FALSE(supportsExternalAmplitudeControl);
    }
}

TEST_P(VibratorAidl, ExternalControlUnsupportedMatchingCapabilities) {
    if ((capabilities & IVibrator::CAP_EXTERNAL_CONTROL) == 0) {
        Status status = vibrator->setExternalControl(true);
        EXPECT_TRUE(isUnknownOrUnsupported(status)) << status;
    }
}

TEST_P(VibratorAidl, GetSupportedPrimitives) {
    if (capabilities & IVibrator::CAP_COMPOSE_EFFECTS) {
        std::vector<CompositePrimitive> supported;

        EXPECT_EQ(Status::EX_NONE, vibrator->getSupportedPrimitives(&supported).exceptionCode());

        for (auto primitive : kCompositePrimitives) {
            bool isPrimitiveSupported =
                std::find(supported.begin(), supported.end(), primitive) != supported.end();
            bool isPrimitiveRequired =
                    std::find(kRequiredPrimitives.begin(), kRequiredPrimitives.end(), primitive) !=
                    kRequiredPrimitives.end();

            EXPECT_TRUE(isPrimitiveSupported || !isPrimitiveRequired) << toString(primitive);
        }
    }
}

TEST_P(VibratorAidl, GetPrimitiveDuration) {
    if (capabilities & IVibrator::CAP_COMPOSE_EFFECTS) {
        std::vector<CompositePrimitive> supported;
        ASSERT_TRUE(vibrator->getSupportedPrimitives(&supported).isOk());

        for (auto primitive : kCompositePrimitives) {
            bool isPrimitiveSupported =
                std::find(supported.begin(), supported.end(), primitive) != supported.end();
            int32_t duration;

            Status status = vibrator->getPrimitiveDuration(primitive, &duration);

            if (isPrimitiveSupported) {
                EXPECT_EQ(Status::EX_NONE, status.exceptionCode());
            } else {
                EXPECT_TRUE(isUnknownOrUnsupported(status)) << status;
            }
        }
    }
}

TEST_P(VibratorAidl, ComposeValidPrimitives) {
    if (capabilities & IVibrator::CAP_COMPOSE_EFFECTS) {
        std::vector<CompositePrimitive> supported;
        int32_t maxDelay, maxSize;

        ASSERT_TRUE(vibrator->getSupportedPrimitives(&supported).isOk());
        EXPECT_EQ(Status::EX_NONE, vibrator->getCompositionDelayMax(&maxDelay).exceptionCode());
        EXPECT_EQ(Status::EX_NONE, vibrator->getCompositionSizeMax(&maxSize).exceptionCode());

        std::vector<CompositeEffect> composite;

        for (auto primitive : supported) {
            CompositeEffect effect;

            effect.delayMs = std::rand() % (maxDelay + 1);
            effect.primitive = primitive;
            effect.scale = static_cast<float>(std::rand()) / RAND_MAX;
            composite.emplace_back(effect);

            if (composite.size() == maxSize) {
                EXPECT_EQ(Status::EX_NONE, vibrator->compose(composite, nullptr).exceptionCode());
                composite.clear();
                vibrator->off();
            }
        }

        if (composite.size() != 0) {
            EXPECT_EQ(Status::EX_NONE, vibrator->compose(composite, nullptr).exceptionCode());
            vibrator->off();
        }
    }
}

TEST_P(VibratorAidl, ComposeUnsupportedPrimitives) {
    if (capabilities & IVibrator::CAP_COMPOSE_EFFECTS) {
        auto unsupported = kInvalidPrimitives;
        std::vector<CompositePrimitive> supported;

        ASSERT_TRUE(vibrator->getSupportedPrimitives(&supported).isOk());

        for (auto primitive : kCompositePrimitives) {
            bool isPrimitiveSupported =
                std::find(supported.begin(), supported.end(), primitive) != supported.end();

            if (!isPrimitiveSupported) {
                unsupported.push_back(primitive);
            }
        }

        for (auto primitive : unsupported) {
            std::vector<CompositeEffect> composite(1);

            for (auto &effect : composite) {
                effect.delayMs = 0;
                effect.primitive = primitive;
                effect.scale = 1.0f;
            }
            Status status = vibrator->compose(composite, nullptr);
            EXPECT_TRUE(isUnknownOrUnsupported(status)) << status;
            vibrator->off();
        }
    }
}

TEST_P(VibratorAidl, ComposeScaleBoundary) {
    if (capabilities & IVibrator::CAP_COMPOSE_EFFECTS) {
        std::vector<CompositeEffect> composite(1);
        CompositeEffect &effect = composite[0];

        effect.delayMs = 0;
        effect.primitive = CompositePrimitive::CLICK;

        effect.scale = std::nextafter(0.0f, -1.0f);
        EXPECT_EQ(Status::EX_ILLEGAL_ARGUMENT,
                  vibrator->compose(composite, nullptr).exceptionCode());

        effect.scale = 0.0f;
        EXPECT_EQ(Status::EX_NONE, vibrator->compose(composite, nullptr).exceptionCode());

        effect.scale = 1.0f;
        EXPECT_EQ(Status::EX_NONE, vibrator->compose(composite, nullptr).exceptionCode());

        effect.scale = std::nextafter(1.0f, 2.0f);
        EXPECT_EQ(Status::EX_ILLEGAL_ARGUMENT,
                  vibrator->compose(composite, nullptr).exceptionCode());

        vibrator->off();
    }
}

TEST_P(VibratorAidl, ComposeDelayBoundary) {
    if (capabilities & IVibrator::CAP_COMPOSE_EFFECTS) {
        int32_t maxDelay;

        EXPECT_EQ(Status::EX_NONE, vibrator->getCompositionDelayMax(&maxDelay).exceptionCode());

        std::vector<CompositeEffect> composite(1);
        CompositeEffect effect;

        effect.delayMs = 1;
        effect.primitive = CompositePrimitive::CLICK;
        effect.scale = 1.0f;

        std::fill(composite.begin(), composite.end(), effect);
        EXPECT_EQ(Status::EX_NONE, vibrator->compose(composite, nullptr).exceptionCode());

        effect.delayMs = maxDelay + 1;

        std::fill(composite.begin(), composite.end(), effect);
        EXPECT_EQ(Status::EX_ILLEGAL_ARGUMENT,
                  vibrator->compose(composite, nullptr).exceptionCode());
        vibrator->off();
    }
}

TEST_P(VibratorAidl, ComposeSizeBoundary) {
    if (capabilities & IVibrator::CAP_COMPOSE_EFFECTS) {
        int32_t maxSize;

        EXPECT_EQ(Status::EX_NONE, vibrator->getCompositionSizeMax(&maxSize).exceptionCode());

        std::vector<CompositeEffect> composite(maxSize);
        CompositeEffect effect;

        effect.delayMs = 1;
        effect.primitive = CompositePrimitive::CLICK;
        effect.scale = 1.0f;

        std::fill(composite.begin(), composite.end(), effect);
        EXPECT_EQ(Status::EX_NONE, vibrator->compose(composite, nullptr).exceptionCode());

        composite.emplace_back(effect);
        EXPECT_EQ(Status::EX_ILLEGAL_ARGUMENT,
                  vibrator->compose(composite, nullptr).exceptionCode());
        vibrator->off();
    }
}

TEST_P(VibratorAidl, ComposeCallback) {
    if (capabilities & IVibrator::CAP_COMPOSE_EFFECTS) {
        std::vector<CompositePrimitive> supported;

        ASSERT_TRUE(vibrator->getSupportedPrimitives(&supported).isOk());

        for (auto primitive : supported) {
            if (primitive == CompositePrimitive::NOOP) {
                continue;
            }

            std::promise<void> completionPromise;
            std::future<void> completionFuture{completionPromise.get_future()};
            sp<CompletionCallback> callback =
                new CompletionCallback([&completionPromise] { completionPromise.set_value(); });
            CompositeEffect effect;
            std::vector<CompositeEffect> composite;
            int32_t durationMs;
            std::chrono::milliseconds duration;
            std::chrono::time_point<high_resolution_clock> start, end;
            std::chrono::milliseconds elapsed;

            effect.delayMs = 0;
            effect.primitive = primitive;
            effect.scale = 1.0f;
            composite.emplace_back(effect);

            EXPECT_EQ(Status::EX_NONE,
                      vibrator->getPrimitiveDuration(primitive, &durationMs).exceptionCode())
                << toString(primitive);
            duration = std::chrono::milliseconds(durationMs);

            start = high_resolution_clock::now();
            EXPECT_EQ(Status::EX_NONE, vibrator->compose(composite, callback).exceptionCode())
                << toString(primitive);

            //TODO(b/187207798): revert back to conservative timeout values once
            //latencies have been fixed
            EXPECT_EQ(completionFuture.wait_for(duration * 4), std::future_status::ready)
                << toString(primitive);
            end = high_resolution_clock::now();

            elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            EXPECT_GE(elapsed.count(), duration.count()) << toString(primitive);
        }
    }
}

TEST_P(VibratorAidl, AlwaysOn) {
    if (capabilities & IVibrator::CAP_ALWAYS_ON_CONTROL) {
        std::vector<Effect> supported;
        ASSERT_TRUE(vibrator->getSupportedAlwaysOnEffects(&supported).isOk());

        for (Effect effect : kEffects) {
            bool isEffectSupported =
                std::find(supported.begin(), supported.end(), effect) != supported.end();

            for (EffectStrength strength : kEffectStrengths) {
                Status status = vibrator->alwaysOnEnable(0, effect, strength);

                if (isEffectSupported) {
                    EXPECT_EQ(Status::EX_NONE, status.exceptionCode())
                        << toString(effect) << " " << toString(strength);
                } else {
                    EXPECT_TRUE(isUnknownOrUnsupported(status))
                            << status << " " << toString(effect) << " " << toString(strength);
                }
            }
        }

        EXPECT_EQ(Status::EX_NONE, vibrator->alwaysOnDisable(0).exceptionCode());
    }
}

TEST_P(VibratorAidl, GetResonantFrequency) {
    getResonantFrequencyHz(vibrator, capabilities);
}

TEST_P(VibratorAidl, GetQFactor) {
    float qFactor;
    Status status = vibrator->getQFactor(&qFactor);
    if (capabilities & IVibrator::CAP_GET_Q_FACTOR) {
        ASSERT_GT(qFactor, 0);
        EXPECT_EQ(status.exceptionCode(), Status::EX_NONE);
    } else {
        EXPECT_TRUE(isUnknownOrUnsupported(status)) << status;
    }
}

TEST_P(VibratorAidl, GetFrequencyResolution) {
    getFrequencyResolutionHz(vibrator, capabilities);
}

TEST_P(VibratorAidl, GetFrequencyMinimum) {
    getFrequencyMinimumHz(vibrator, capabilities);
}

TEST_P(VibratorAidl, GetBandwidthAmplitudeMap) {
    std::vector<float> bandwidthAmplitudeMap;
    Status status = vibrator->getBandwidthAmplitudeMap(&bandwidthAmplitudeMap);
    if (capabilities & IVibrator::CAP_FREQUENCY_CONTROL) {
        EXPECT_EQ(status.exceptionCode(), Status::EX_NONE);
        ASSERT_FALSE(bandwidthAmplitudeMap.empty());

        int minMapSize = (getResonantFrequencyHz(vibrator, capabilities) -
                          getFrequencyMinimumHz(vibrator, capabilities)) /
                         getFrequencyResolutionHz(vibrator, capabilities);
        ASSERT_GT(bandwidthAmplitudeMap.size(), minMapSize);

        for (float e : bandwidthAmplitudeMap) {
            ASSERT_GE(e, 0.0);
            ASSERT_LE(e, 1.0);
        }
    } else {
        EXPECT_TRUE(isUnknownOrUnsupported(status)) << status;
    }
}

TEST_P(VibratorAidl, GetPwlePrimitiveDurationMax) {
    int32_t durationMs;
    Status status = vibrator->getPwlePrimitiveDurationMax(&durationMs);
    if (capabilities & IVibrator::CAP_COMPOSE_PWLE_EFFECTS) {
        ASSERT_NE(durationMs, 0);
        EXPECT_EQ(status.exceptionCode(), Status::EX_NONE);
    } else {
        EXPECT_TRUE(isUnknownOrUnsupported(status)) << status;
    }
}

TEST_P(VibratorAidl, GetPwleCompositionSizeMax) {
    int32_t maxSize;
    Status status = vibrator->getPwleCompositionSizeMax(&maxSize);
    if (capabilities & IVibrator::CAP_COMPOSE_PWLE_EFFECTS) {
        ASSERT_NE(maxSize, 0);
        EXPECT_EQ(status.exceptionCode(), Status::EX_NONE);
    } else {
        EXPECT_TRUE(isUnknownOrUnsupported(status)) << status;
    }
}

TEST_P(VibratorAidl, GetSupportedBraking) {
    std::vector<Braking> supported;
    Status status = vibrator->getSupportedBraking(&supported);
    if (capabilities & IVibrator::CAP_COMPOSE_PWLE_EFFECTS) {
        bool isDefaultNoneSupported =
            std::find(supported.begin(), supported.end(), Braking::NONE) != supported.end();
        ASSERT_TRUE(isDefaultNoneSupported);
        EXPECT_EQ(status.exceptionCode(), Status::EX_NONE);
    } else {
        EXPECT_TRUE(isUnknownOrUnsupported(status)) << status;
    }
}

TEST_P(VibratorAidl, ComposeValidPwle) {
    if (capabilities & IVibrator::CAP_COMPOSE_PWLE_EFFECTS) {
        ActivePwle active = composeValidActivePwle(vibrator, capabilities);

        std::vector<Braking> supported;
        ASSERT_TRUE(vibrator->getSupportedBraking(&supported).isOk());
        bool isClabSupported =
            std::find(supported.begin(), supported.end(), Braking::CLAB) != supported.end();
        BrakingPwle braking;
        braking.braking = isClabSupported ? Braking::CLAB : Braking::NONE;
        braking.duration = 100;

        std::vector<PrimitivePwle> pwleQueue;
        PrimitivePwle pwle;
        pwle = active;
        pwleQueue.emplace_back(std::move(pwle));
        pwle = braking;
        pwleQueue.emplace_back(std::move(pwle));
        pwle = active;
        pwleQueue.emplace_back(std::move(pwle));

        EXPECT_EQ(Status::EX_NONE, vibrator->composePwle(pwleQueue, nullptr).exceptionCode());
        vibrator->off();
    }
}

TEST_P(VibratorAidl, ComposeValidPwleWithCallback) {
    if (!((capabilities & IVibrator::CAP_ON_CALLBACK) &&
          (capabilities & IVibrator::CAP_COMPOSE_PWLE_EFFECTS)))
        return;

    std::promise<void> completionPromise;
    std::future<void> completionFuture{completionPromise.get_future()};
    sp<CompletionCallback> callback =
        new CompletionCallback([&completionPromise] { completionPromise.set_value(); });
    uint32_t durationMs = 2100;  // Sum of 2 active and 1 braking below
    //TODO(b/187207798): revert back to conservative timeout values once
    //latencies have been fixed
    std::chrono::milliseconds timeout{durationMs * 4};

    ActivePwle active = composeValidActivePwle(vibrator, capabilities);

    std::vector<Braking> supported;
    ASSERT_TRUE(vibrator->getSupportedBraking(&supported).isOk());
    bool isClabSupported =
        std::find(supported.begin(), supported.end(), Braking::CLAB) != supported.end();
    BrakingPwle braking;
    braking.braking = isClabSupported ? Braking::CLAB : Braking::NONE;
    braking.duration = 100;

    std::vector<PrimitivePwle> pwleQueue;
    PrimitivePwle pwle;
    pwle = active;
    pwleQueue.emplace_back(std::move(pwle));
    pwle = braking;
    pwleQueue.emplace_back(std::move(pwle));
    pwle = active;
    pwleQueue.emplace_back(std::move(pwle));

    EXPECT_TRUE(vibrator->composePwle(pwleQueue, callback).isOk());
    EXPECT_EQ(completionFuture.wait_for(timeout), std::future_status::ready);
    EXPECT_TRUE(vibrator->off().isOk());
}

TEST_P(VibratorAidl, ComposePwleSegmentBoundary) {
    if (capabilities & IVibrator::CAP_COMPOSE_PWLE_EFFECTS) {
        std::vector<PrimitivePwle> pwleQueue;
        // test empty queue
        EXPECT_EQ(Status::EX_ILLEGAL_ARGUMENT,
                  vibrator->composePwle(pwleQueue, nullptr).exceptionCode());
        vibrator->off();

        ActivePwle active = composeValidActivePwle(vibrator, capabilities);

        PrimitivePwle pwle;
        pwle = active;
        int segmentCountMax;
        vibrator->getPwleCompositionSizeMax(&segmentCountMax);

        // Create PWLE queue with more segments than allowed
        for (int i = 0; i < segmentCountMax + 10; i++) {
            pwleQueue.emplace_back(std::move(pwle));
        }

        EXPECT_EQ(Status::EX_ILLEGAL_ARGUMENT,
                  vibrator->composePwle(pwleQueue, nullptr).exceptionCode());
        vibrator->off();
    }
}

TEST_P(VibratorAidl, ComposePwleAmplitudeParameterBoundary) {
    if (capabilities & IVibrator::CAP_COMPOSE_PWLE_EFFECTS) {
        ActivePwle active = composeValidActivePwle(vibrator, capabilities);
        active.startAmplitude = getAmplitudeMax() + 1.0;  // Amplitude greater than allowed
        active.endAmplitude = getAmplitudeMax() + 1.0;    // Amplitude greater than allowed

        std::vector<PrimitivePwle> pwleQueueGreater;
        PrimitivePwle pwle;
        pwle = active;
        pwleQueueGreater.emplace_back(std::move(pwle));

        EXPECT_EQ(Status::EX_ILLEGAL_ARGUMENT,
                  vibrator->composePwle(pwleQueueGreater, nullptr).exceptionCode());
        vibrator->off();

        active.startAmplitude = getAmplitudeMin() - 1.0;  // Amplitude less than allowed
        active.endAmplitude = getAmplitudeMin() - 1.0;    // Amplitude less than allowed

        std::vector<PrimitivePwle> pwleQueueLess;
        pwle = active;
        pwleQueueLess.emplace_back(std::move(pwle));

        EXPECT_EQ(Status::EX_ILLEGAL_ARGUMENT,
                  vibrator->composePwle(pwleQueueLess, nullptr).exceptionCode());
        vibrator->off();
    }
}

TEST_P(VibratorAidl, ComposePwleFrequencyParameterBoundary) {
    if ((capabilities & IVibrator::CAP_COMPOSE_PWLE_EFFECTS) &&
        (capabilities & IVibrator::CAP_FREQUENCY_CONTROL)) {
        float freqMinimumHz = getFrequencyMinimumHz(vibrator, capabilities);
        float freqMaximumHz = getFrequencyMaximumHz(vibrator, capabilities);
        float freqResolutionHz = getFrequencyResolutionHz(vibrator, capabilities);

        ActivePwle active = composeValidActivePwle(vibrator, capabilities);
        active.startFrequency =
            freqMaximumHz + freqResolutionHz;                    // Frequency greater than allowed
        active.endFrequency = freqMaximumHz + freqResolutionHz;  // Frequency greater than allowed

        std::vector<PrimitivePwle> pwleQueueGreater;
        PrimitivePwle pwle;
        pwle = active;
        pwleQueueGreater.emplace_back(std::move(pwle));

        EXPECT_EQ(Status::EX_ILLEGAL_ARGUMENT,
                  vibrator->composePwle(pwleQueueGreater, nullptr).exceptionCode());
        vibrator->off();

        active.startFrequency = freqMinimumHz - freqResolutionHz;  // Frequency less than allowed
        active.endFrequency = freqMinimumHz - freqResolutionHz;    // Frequency less than allowed

        std::vector<PrimitivePwle> pwleQueueLess;
        pwle = active;
        pwleQueueLess.emplace_back(std::move(pwle));

        EXPECT_EQ(Status::EX_ILLEGAL_ARGUMENT,
                  vibrator->composePwle(pwleQueueLess, nullptr).exceptionCode());
        vibrator->off();
    }
}

TEST_P(VibratorAidl, ComposePwleSegmentDurationBoundary) {
    if (capabilities & IVibrator::CAP_COMPOSE_PWLE_EFFECTS) {
        ActivePwle active = composeValidActivePwle(vibrator, capabilities);

        int segmentDurationMaxMs;
        vibrator->getPwlePrimitiveDurationMax(&segmentDurationMaxMs);
        active.duration = segmentDurationMaxMs + 10;  // Segment duration greater than allowed

        std::vector<PrimitivePwle> pwleQueue;
        PrimitivePwle pwle;
        pwle = active;
        pwleQueue.emplace_back(std::move(pwle));

        EXPECT_EQ(Status::EX_ILLEGAL_ARGUMENT,
                  vibrator->composePwle(pwleQueue, nullptr).exceptionCode());
        vibrator->off();
    }
}

std::vector<std::tuple<int32_t, int32_t>> GenerateVibratorMapping() {
    std::vector<std::tuple<int32_t, int32_t>> tuples;
    auto managerAidlNames = android::getAidlHalInstanceNames(IVibratorManager::descriptor);
    std::vector<int32_t> vibratorIds;

    for (int i = 0; i < managerAidlNames.size(); i++) {
        auto managerName = String16(managerAidlNames[i].c_str());
        auto vibratorManager = android::waitForDeclaredService<IVibratorManager>(managerName);
        if (vibratorManager->getVibratorIds(&vibratorIds).isOk()) {
            for (auto &vibratorId : vibratorIds) {
                tuples.push_back(std::make_tuple(i, vibratorId));
            }
        }
    }

    auto vibratorAidlNames = android::getAidlHalInstanceNames(IVibrator::descriptor);
    for (int i = 0; i < vibratorAidlNames.size(); i++) {
        tuples.push_back(std::make_tuple(-1, i));
    }

    return tuples;
}

std::string PrintGeneratedTest(const testing::TestParamInfo<VibratorAidl::ParamType> &info) {
    const auto &[managerIdx, vibratorId] = info.param;
    if (managerIdx < 0) {
        return std::string("TOP_LEVEL_VIBRATOR_") + std::to_string(vibratorId);
    }
    return std::string("MANAGER_") + std::to_string(managerIdx) + "_VIBRATOR_ID_" +
           std::to_string(vibratorId);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(VibratorAidl);
INSTANTIATE_TEST_SUITE_P(Vibrator, VibratorAidl, testing::ValuesIn(GenerateVibratorMapping()),
                         PrintGeneratedTest);

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ProcessState::self()->setThreadPoolMaxThreadCount(1);
    ProcessState::self()->startThreadPool();
    return RUN_ALL_TESTS();
}
