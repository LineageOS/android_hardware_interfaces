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
#include <aidl/android/hardware/vibrator/BnVibratorCallback.h>
#include <aidl/android/hardware/vibrator/IVibrator.h>
#include <aidl/android/hardware/vibrator/IVibratorManager.h>

#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <android/persistable_bundle_aidl.h>

#include <cmath>
#include <cstdlib>
#include <ctime>
#include <future>
#include <iomanip>
#include <iostream>
#include <random>

#include "persistable_bundle_utils.h"
#include "pwle_v2_utils.h"
#include "test_utils.h"

using aidl::android::hardware::vibrator::ActivePwle;
using aidl::android::hardware::vibrator::BnVibratorCallback;
using aidl::android::hardware::vibrator::Braking;
using aidl::android::hardware::vibrator::BrakingPwle;
using aidl::android::hardware::vibrator::CompositeEffect;
using aidl::android::hardware::vibrator::CompositePrimitive;
using aidl::android::hardware::vibrator::CompositePwleV2;
using aidl::android::hardware::vibrator::Effect;
using aidl::android::hardware::vibrator::EffectStrength;
using aidl::android::hardware::vibrator::FrequencyAccelerationMapEntry;
using aidl::android::hardware::vibrator::IVibrator;
using aidl::android::hardware::vibrator::IVibratorManager;
using aidl::android::hardware::vibrator::PrimitivePwle;
using aidl::android::hardware::vibrator::PwleV2Primitive;
using aidl::android::hardware::vibrator::VendorEffect;
using aidl::android::os::PersistableBundle;
using std::chrono::high_resolution_clock;

using namespace ::std::chrono_literals;

namespace pwle_v2_utils = aidl::android::hardware::vibrator::testing::pwlev2;

const std::vector<Effect> kEffects{ndk::enum_range<Effect>().begin(),
                                   ndk::enum_range<Effect>().end()};
const std::vector<EffectStrength> kEffectStrengths{ndk::enum_range<EffectStrength>().begin(),
                                                   ndk::enum_range<EffectStrength>().end()};

const std::vector<Effect> kInvalidEffects = {
    static_cast<Effect>(static_cast<int32_t>(kEffects.front()) - 1),
    static_cast<Effect>(static_cast<int32_t>(kEffects.back()) + 1),
};

const std::vector<EffectStrength> kInvalidEffectStrengths = {
    static_cast<EffectStrength>(static_cast<int8_t>(kEffectStrengths.front()) - 1),
    static_cast<EffectStrength>(static_cast<int8_t>(kEffectStrengths.back()) + 1),
};

const std::vector<CompositePrimitive> kCompositePrimitives{
        ndk::enum_range<CompositePrimitive>().begin(), ndk::enum_range<CompositePrimitive>().end()};

const std::vector<CompositePrimitive> kRequiredPrimitives = {
        CompositePrimitive::CLICK,      CompositePrimitive::LIGHT_TICK,
        CompositePrimitive::QUICK_RISE, CompositePrimitive::SLOW_RISE,
        CompositePrimitive::QUICK_FALL,
};

const std::vector<CompositePrimitive> kInvalidPrimitives = {
    static_cast<CompositePrimitive>(static_cast<int32_t>(kCompositePrimitives.front()) - 1),
    static_cast<CompositePrimitive>(static_cast<int32_t>(kCompositePrimitives.back()) + 1),
};

// Timeout to wait for vibration callback completion.
static constexpr std::chrono::milliseconds VIBRATION_CALLBACK_TIMEOUT = 100ms;

static constexpr int32_t VENDOR_EFFECTS_MIN_VERSION = 3;
static constexpr int32_t PWLE_V2_MIN_VERSION = 3;

static std::vector<std::string> findVibratorManagerNames() {
    std::vector<std::string> names;
    constexpr auto callback = [](const char* instance, void* context) {
        auto fullName = std::string(IVibratorManager::descriptor) + "/" + instance;
        static_cast<std::vector<std::string>*>(context)->emplace_back(fullName);
    };
    AServiceManager_forEachDeclaredInstance(IVibratorManager::descriptor,
                                            static_cast<void*>(&names), callback);
    return names;
}

static std::vector<std::string> findUnmanagedVibratorNames() {
    std::vector<std::string> names;
    constexpr auto callback = [](const char* instance, void* context) {
        auto fullName = std::string(IVibrator::descriptor) + "/" + instance;
        static_cast<std::vector<std::string>*>(context)->emplace_back(fullName);
    };
    AServiceManager_forEachDeclaredInstance(IVibrator::descriptor, static_cast<void*>(&names),
                                            callback);
    return names;
}

class CompletionCallback : public BnVibratorCallback {
  public:
    CompletionCallback(const std::function<void()> &callback) : mCallback(callback) {}
    ndk::ScopedAStatus onComplete() override {
        mCallback();
        return ndk::ScopedAStatus::ok();
    }

  private:
    std::function<void()> mCallback;
};

class VibratorAidl : public testing::TestWithParam<std::tuple<int32_t, int32_t>> {
  public:
    virtual void SetUp() override {
        int32_t managerIdx = std::get<0>(GetParam());
        int32_t vibratorId = std::get<1>(GetParam());

        if (managerIdx < 0) {
            // Testing a unmanaged vibrator, using vibratorId as index from registered HALs
            std::vector<std::string> vibratorNames = findUnmanagedVibratorNames();
            ASSERT_LT(vibratorId, vibratorNames.size());
            vibrator = IVibrator::fromBinder(ndk::SpAIBinder(
                    AServiceManager_waitForService(vibratorNames[vibratorId].c_str())));
        } else {
            // Testing a managed vibrator, using vibratorId to retrieve it from the manager
            std::vector<std::string> managerNames = findVibratorManagerNames();
            ASSERT_LT(managerIdx, managerNames.size());
            auto vibratorManager = IVibratorManager::fromBinder(ndk::SpAIBinder(
                    AServiceManager_waitForService(managerNames[managerIdx].c_str())));
            EXPECT_OK(vibratorManager->getVibrator(vibratorId, &vibrator))
                    << "\n  For vibrator id: " << vibratorId;
        }

        ASSERT_NE(vibrator, nullptr);
        EXPECT_OK(vibrator->getInterfaceVersion(&version));
        EXPECT_OK(vibrator->getCapabilities(&capabilities));
    }

    virtual void TearDown() override {
        // Reset vibrator state between tests.
        EXPECT_OK(vibrator->off());
    }

    std::shared_ptr<IVibrator> vibrator;
    int32_t version;
    int32_t capabilities;
};

static float getResonantFrequencyHz(const std::shared_ptr<IVibrator>& vibrator,
                                    int32_t capabilities) {
    float resonantFrequencyHz;
    ndk::ScopedAStatus status = vibrator->getResonantFrequency(&resonantFrequencyHz);
    if (capabilities & IVibrator::CAP_GET_RESONANT_FREQUENCY) {
        EXPECT_OK(std::move(status));
        EXPECT_GT(resonantFrequencyHz, 0);
    } else {
        EXPECT_UNKNOWN_OR_UNSUPPORTED(std::move(status));
    }
    return resonantFrequencyHz;
}

static bool shouldValidateLegacyFrequencyControlResult(int32_t capabilities, int32_t version,
                                                       ndk::ScopedAStatus& status) {
    bool hasFrequencyControl = capabilities & IVibrator::CAP_FREQUENCY_CONTROL;
    // Legacy frequency control APIs deprecated with PWLE V2 feature.
    bool isDeprecated = version >= PWLE_V2_MIN_VERSION;
    bool isUnknownOrUnsupported = status.getExceptionCode() == EX_UNSUPPORTED_OPERATION ||
                                  status.getStatus() == STATUS_UNKNOWN_TRANSACTION;

    // Validate if older HAL or if result is provided, even after deprecation.
    return hasFrequencyControl && (!isDeprecated || !isUnknownOrUnsupported);
}

static float getFrequencyResolutionHz(const std::shared_ptr<IVibrator>& vibrator,
                                      int32_t capabilities, int32_t version) {
    float freqResolutionHz = -1;
    ndk::ScopedAStatus status = vibrator->getFrequencyResolution(&freqResolutionHz);
    if (shouldValidateLegacyFrequencyControlResult(capabilities, version, status)) {
        EXPECT_OK(std::move(status));
        EXPECT_GT(freqResolutionHz, 0);
    } else {
        EXPECT_UNKNOWN_OR_UNSUPPORTED(std::move(status));
    }
    return freqResolutionHz;
}

static float getFrequencyMinimumHz(const std::shared_ptr<IVibrator>& vibrator, int32_t capabilities,
                                   int32_t version) {
    float freqMinimumHz;
    ndk::ScopedAStatus status = vibrator->getFrequencyMinimum(&freqMinimumHz);
    if (shouldValidateLegacyFrequencyControlResult(capabilities, version, status)) {
        EXPECT_OK(std::move(status));

        float resonantFrequencyHz = getResonantFrequencyHz(vibrator, capabilities);

        EXPECT_GT(freqMinimumHz, 0);
        EXPECT_LE(freqMinimumHz, resonantFrequencyHz);
    } else {
        EXPECT_UNKNOWN_OR_UNSUPPORTED(std::move(status));
    }
    return freqMinimumHz;
}

static float getFrequencyMaximumHz(const std::shared_ptr<IVibrator>& vibrator, int32_t capabilities,
                                   int32_t version) {
    std::vector<float> bandwidthAmplitudeMap;
    ndk::ScopedAStatus status = vibrator->getBandwidthAmplitudeMap(&bandwidthAmplitudeMap);
    if (shouldValidateLegacyFrequencyControlResult(capabilities, version, status)) {
        EXPECT_OK(std::move(status));
    } else {
        EXPECT_UNKNOWN_OR_UNSUPPORTED(std::move(status));
    }

    float freqMaximumHz = ((bandwidthAmplitudeMap.size() - 1) *
                           getFrequencyResolutionHz(vibrator, capabilities, version)) +
                          getFrequencyMinimumHz(vibrator, capabilities, version);
    return freqMaximumHz;
}

static float getAmplitudeMin() {
    return 0.0;
}

static float getAmplitudeMax() {
    return 1.0;
}

static ActivePwle composeValidActivePwle(const std::shared_ptr<IVibrator>& vibrator,
                                         int32_t capabilities, int32_t version) {
    float frequencyHz;
    if (capabilities & IVibrator::CAP_GET_RESONANT_FREQUENCY) {
        frequencyHz = getResonantFrequencyHz(vibrator, capabilities);
    } else if (capabilities & IVibrator::CAP_FREQUENCY_CONTROL) {
        if (version < PWLE_V2_MIN_VERSION) {
            frequencyHz = getFrequencyMinimumHz(vibrator, capabilities, version);
        } else {
            frequencyHz = pwle_v2_utils::getPwleV2FrequencyMinHz(vibrator);
        }
    } else {
        frequencyHz = 150.0;  // default value commonly used
    }

    ActivePwle active;
    active.startAmplitude = (getAmplitudeMin() + getAmplitudeMax()) / 2;
    active.startFrequency = frequencyHz;
    active.endAmplitude = (getAmplitudeMin() + getAmplitudeMax()) / 2;
    active.endFrequency = frequencyHz;
    vibrator->getPwlePrimitiveDurationMax(&(active.duration));

    return active;
}

TEST_P(VibratorAidl, OnThenOffBeforeTimeout) {
    EXPECT_OK(vibrator->on(2000, nullptr /*callback*/));
    sleep(1);
    EXPECT_OK(vibrator->off());
}

TEST_P(VibratorAidl, OnWithCallback) {
    if (!(capabilities & IVibrator::CAP_ON_CALLBACK))
        return;

    std::promise<void> completionPromise;
    std::future<void> completionFuture{completionPromise.get_future()};
    auto callback = ndk::SharedRefBase::make<CompletionCallback>(
            [&completionPromise] { completionPromise.set_value(); });
    uint32_t durationMs = 250;
    auto timeout = std::chrono::milliseconds(durationMs) + VIBRATION_CALLBACK_TIMEOUT;
    EXPECT_OK(vibrator->on(durationMs, callback));
    EXPECT_EQ(completionFuture.wait_for(timeout), std::future_status::ready);
    EXPECT_OK(vibrator->off());
}

TEST_P(VibratorAidl, OnCallbackNotSupported) {
    if (!(capabilities & IVibrator::CAP_ON_CALLBACK)) {
        auto callback = ndk::SharedRefBase::make<CompletionCallback>([] {});
        EXPECT_UNKNOWN_OR_UNSUPPORTED(vibrator->on(250, callback));
    }
}

TEST_P(VibratorAidl, ValidateEffect) {
    std::vector<Effect> supported;
    EXPECT_OK(vibrator->getSupportedEffects(&supported));

    for (Effect effect : kEffects) {
        bool isEffectSupported =
            std::find(supported.begin(), supported.end(), effect) != supported.end();

        for (EffectStrength strength : kEffectStrengths) {
            int32_t lengthMs = 0;
            ndk::ScopedAStatus status =
                    vibrator->perform(effect, strength, nullptr /*callback*/, &lengthMs);

            if (isEffectSupported) {
                EXPECT_OK(std::move(status))
                        << "\n  For effect: " << toString(effect) << " " << toString(strength);
                EXPECT_GT(lengthMs, 0);
                usleep(lengthMs * 1000);
                EXPECT_OK(vibrator->off());
            } else {
                EXPECT_UNKNOWN_OR_UNSUPPORTED(std::move(status))
                        << "\n  For effect: " << toString(effect) << " " << toString(strength);
            }
        }
    }
}

TEST_P(VibratorAidl, ValidateEffectWithCallback) {
    if (!(capabilities & IVibrator::CAP_PERFORM_CALLBACK))
        return;

    std::vector<Effect> supported;
    EXPECT_OK(vibrator->getSupportedEffects(&supported));

    for (Effect effect : kEffects) {
        bool isEffectSupported =
            std::find(supported.begin(), supported.end(), effect) != supported.end();

        for (EffectStrength strength : kEffectStrengths) {
            std::promise<void> completionPromise;
            std::future<void> completionFuture{completionPromise.get_future()};
            auto callback = ndk::SharedRefBase::make<CompletionCallback>(
                    [&completionPromise] { completionPromise.set_value(); });
            int lengthMs = 0;
            ndk::ScopedAStatus status = vibrator->perform(effect, strength, callback, &lengthMs);

            if (isEffectSupported) {
                EXPECT_OK(std::move(status))
                        << "\n  For effect: " << toString(effect) << " " << toString(strength);
                EXPECT_GT(lengthMs, 0);
            } else {
                EXPECT_UNKNOWN_OR_UNSUPPORTED(std::move(status))
                        << "\n  For effect: " << toString(effect) << " " << toString(strength);
            }

            if (lengthMs <= 0) continue;

            auto timeout = std::chrono::milliseconds(lengthMs) + VIBRATION_CALLBACK_TIMEOUT;
            EXPECT_EQ(completionFuture.wait_for(timeout), std::future_status::ready);

            EXPECT_OK(vibrator->off());
        }
    }
}

TEST_P(VibratorAidl, ValidateEffectWithCallbackNotSupported) {
    if (capabilities & IVibrator::CAP_PERFORM_CALLBACK)
        return;

    for (Effect effect : kEffects) {
        for (EffectStrength strength : kEffectStrengths) {
            auto callback = ndk::SharedRefBase::make<CompletionCallback>([] {});
            int lengthMs;
            EXPECT_UNKNOWN_OR_UNSUPPORTED(vibrator->perform(effect, strength, callback, &lengthMs))
                    << "\n  For effect: " << toString(effect) << " " << toString(strength);
        }
    }
}

TEST_P(VibratorAidl, InvalidEffectsUnsupported) {
    for (Effect effect : kInvalidEffects) {
        for (EffectStrength strength : kEffectStrengths) {
            int32_t lengthMs;
            EXPECT_UNKNOWN_OR_UNSUPPORTED(
                    vibrator->perform(effect, strength, nullptr /*callback*/, &lengthMs))
                    << "\n  For effect: " << toString(effect) << " " << toString(strength);
        }
    }
    for (Effect effect : kEffects) {
        for (EffectStrength strength : kInvalidEffectStrengths) {
            int32_t lengthMs;
            EXPECT_UNKNOWN_OR_UNSUPPORTED(
                    vibrator->perform(effect, strength, nullptr /*callback*/, &lengthMs))
                    << "\n  For effect: " << toString(effect) << " " << toString(strength);
        }
    }
}

TEST_P(VibratorAidl, PerformVendorEffectSupported) {
    if ((capabilities & IVibrator::CAP_PERFORM_VENDOR_EFFECTS) == 0) return;

    float scale = 0.0f;
    float vendorScale = 0.0f;
    for (EffectStrength strength : kEffectStrengths) {
        PersistableBundle vendorData;
        ::aidl::android::hardware::vibrator::testing::fillBasicData(&vendorData);

        PersistableBundle nestedData;
        ::aidl::android::hardware::vibrator::testing::fillBasicData(&nestedData);
        vendorData.putPersistableBundle("test_nested_bundle", nestedData);

        VendorEffect effect;
        effect.vendorData = vendorData;
        effect.strength = strength;
        effect.scale = scale;
        effect.vendorScale = vendorScale;
        scale += 0.5f;
        vendorScale += 0.2f;

        auto callback = ndk::SharedRefBase::make<CompletionCallback>([] {});
        ndk::ScopedAStatus status = vibrator->performVendorEffect(effect, callback);

        // No expectations on the actual status, the effect might be refused with illegal argument
        // or the vendor might return a service-specific error code.
        EXPECT_TRUE(status.getExceptionCode() != EX_UNSUPPORTED_OPERATION &&
                    status.getStatus() != STATUS_UNKNOWN_TRANSACTION)
                << status << "\n For vendor effect with strength" << toString(strength)
                << " and scale " << effect.scale;

        if (status.isOk()) {
            // Generic vendor data should not trigger vibrations, but if it does trigger one
            // then we make sure the vibrator is reset by triggering off().
            EXPECT_OK(vibrator->off());
        }
    }
}

TEST_P(VibratorAidl, PerformVendorEffectStability) {
    if ((capabilities & IVibrator::CAP_PERFORM_VENDOR_EFFECTS) == 0) return;

    // Run some iterations of performVendorEffect with randomized vendor data to check basic
    // stability of the implementation.
    uint8_t iterations = 200;

    for (EffectStrength strength : kEffectStrengths) {
        float scale = 0.5f;
        float vendorScale = 0.2f;
        for (uint8_t i = 0; i < iterations; i++) {
            PersistableBundle vendorData;
            ::aidl::android::hardware::vibrator::testing::fillRandomData(&vendorData);

            VendorEffect effect;
            effect.vendorData = vendorData;
            effect.strength = strength;
            effect.scale = scale;
            effect.vendorScale = vendorScale;
            scale *= 2;
            vendorScale *= 1.5f;

            auto callback = ndk::SharedRefBase::make<CompletionCallback>([] {});
            ndk::ScopedAStatus status = vibrator->performVendorEffect(effect, callback);

            // No expectations on the actual status, the effect might be refused with illegal
            // argument or the vendor might return a service-specific error code.
            EXPECT_TRUE(status.getExceptionCode() != EX_UNSUPPORTED_OPERATION &&
                        status.getStatus() != STATUS_UNKNOWN_TRANSACTION)
                    << status << "\n For random vendor effect with strength " << toString(strength)
                    << " and scale " << effect.scale;

            if (status.isOk()) {
                // Random vendor data should not trigger vibrations, but if it does trigger one
                // then we make sure the vibrator is reset by triggering off().
                EXPECT_OK(vibrator->off());
            }
        }
    }
}

TEST_P(VibratorAidl, PerformVendorEffectEmptyVendorData) {
    if ((capabilities & IVibrator::CAP_PERFORM_VENDOR_EFFECTS) == 0) return;

    for (EffectStrength strength : kEffectStrengths) {
        VendorEffect effect;
        effect.strength = strength;
        effect.scale = 1.0f;
        effect.vendorScale = 1.0f;

        ndk::ScopedAStatus status = vibrator->performVendorEffect(effect, nullptr /*callback*/);

        EXPECT_TRUE(status.getExceptionCode() == EX_SERVICE_SPECIFIC)
                << status << "\n For vendor effect with strength " << toString(strength)
                << " and scale " << effect.scale;
    }
}

TEST_P(VibratorAidl, PerformVendorEffectInvalidScale) {
    if ((capabilities & IVibrator::CAP_PERFORM_VENDOR_EFFECTS) == 0) return;

    VendorEffect effect;
    effect.strength = EffectStrength::MEDIUM;

    effect.scale = -1.0f;
    effect.vendorScale = 1.0f;
    EXPECT_ILLEGAL_ARGUMENT(vibrator->performVendorEffect(effect, nullptr /*callback*/));

    effect.scale = 1.0f;
    effect.vendorScale = -1.0f;
    EXPECT_ILLEGAL_ARGUMENT(vibrator->performVendorEffect(effect, nullptr /*callback*/));
}

TEST_P(VibratorAidl, PerformVendorEffectUnsupported) {
    if (version < VENDOR_EFFECTS_MIN_VERSION) {
        EXPECT_EQ(capabilities & IVibrator::CAP_PERFORM_VENDOR_EFFECTS, 0)
                << "Vibrator version " << version << " should not report vendor effects capability";
    }
    if (capabilities & IVibrator::CAP_PERFORM_VENDOR_EFFECTS) return;

    for (EffectStrength strength : kEffectStrengths) {
        VendorEffect effect;
        effect.strength = strength;
        effect.scale = 1.0f;
        effect.vendorScale = 1.0f;

        EXPECT_UNKNOWN_OR_UNSUPPORTED(vibrator->performVendorEffect(effect, nullptr /*callback*/))
                << "\n  For vendor effect with strength " << toString(strength);
    }
}

TEST_P(VibratorAidl, ChangeVibrationAmplitude) {
    if (capabilities & IVibrator::CAP_AMPLITUDE_CONTROL) {
        EXPECT_OK(vibrator->setAmplitude(0.1f));
        EXPECT_OK(vibrator->on(2000, nullptr /*callback*/));
        EXPECT_OK(vibrator->setAmplitude(0.5f));
        sleep(1);
        EXPECT_OK(vibrator->setAmplitude(1.0f));
        sleep(1);
        EXPECT_OK(vibrator->off());
    }
}

TEST_P(VibratorAidl, AmplitudeOutsideRangeFails) {
    if (capabilities & IVibrator::CAP_AMPLITUDE_CONTROL) {
        EXPECT_ILLEGAL_ARGUMENT(vibrator->setAmplitude(-1));
        EXPECT_ILLEGAL_ARGUMENT(vibrator->setAmplitude(0));
        EXPECT_ILLEGAL_ARGUMENT(vibrator->setAmplitude(1.1));
    }
}

TEST_P(VibratorAidl, AmplitudeReturnsUnsupportedMatchingCapabilities) {
    if ((capabilities & IVibrator::CAP_AMPLITUDE_CONTROL) == 0) {
        EXPECT_UNKNOWN_OR_UNSUPPORTED(vibrator->setAmplitude(1));
    }
}

TEST_P(VibratorAidl, ChangeVibrationExternalControl) {
    if (capabilities & IVibrator::CAP_EXTERNAL_CONTROL) {
        EXPECT_OK(vibrator->setExternalControl(true));
        sleep(1);
        EXPECT_OK(vibrator->setExternalControl(false));
        sleep(1);
    }
}

TEST_P(VibratorAidl, ExternalAmplitudeControl) {
    const bool supportsExternalAmplitudeControl =
        (capabilities & IVibrator::CAP_EXTERNAL_AMPLITUDE_CONTROL) > 0;

    if (capabilities & IVibrator::CAP_EXTERNAL_CONTROL) {
        EXPECT_OK(vibrator->setExternalControl(true));

        if (supportsExternalAmplitudeControl) {
            EXPECT_OK(vibrator->setAmplitude(0.5));
        } else {
            EXPECT_UNKNOWN_OR_UNSUPPORTED(vibrator->setAmplitude(0.5));
        }

        EXPECT_OK(vibrator->setExternalControl(false));
    } else {
        EXPECT_FALSE(supportsExternalAmplitudeControl);
    }
}

TEST_P(VibratorAidl, ExternalControlUnsupportedMatchingCapabilities) {
    if ((capabilities & IVibrator::CAP_EXTERNAL_CONTROL) == 0) {
        EXPECT_UNKNOWN_OR_UNSUPPORTED(vibrator->setExternalControl(true));
    }
}

TEST_P(VibratorAidl, GetSupportedPrimitives) {
    if (capabilities & IVibrator::CAP_COMPOSE_EFFECTS) {
        std::vector<CompositePrimitive> supported;
        EXPECT_OK(vibrator->getSupportedPrimitives(&supported));

        for (CompositePrimitive primitive : kCompositePrimitives) {
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
        EXPECT_OK(vibrator->getSupportedPrimitives(&supported));

        for (CompositePrimitive primitive : kCompositePrimitives) {
            bool isPrimitiveSupported =
                std::find(supported.begin(), supported.end(), primitive) != supported.end();
            int32_t duration;

            if (isPrimitiveSupported) {
                EXPECT_OK(vibrator->getPrimitiveDuration(primitive, &duration))
                        << "\n  For primitive: " << toString(primitive) << " " << duration;
                if (primitive != CompositePrimitive::NOOP) {
                    ASSERT_GT(duration, 0)
                            << "\n  For primitive: " << toString(primitive) << " " << duration;
                }
            } else {
                EXPECT_UNKNOWN_OR_UNSUPPORTED(vibrator->getPrimitiveDuration(primitive, &duration))
                        << "\n  For primitive: " << toString(primitive);
            }
        }
    }
}

TEST_P(VibratorAidl, ComposeValidPrimitives) {
    if (!(capabilities & IVibrator::CAP_COMPOSE_EFFECTS)) {
        GTEST_SKIP() << "CAP_COMPOSE_EFFECTS not supported";
    }

    std::vector<CompositePrimitive> supported;
    int32_t maxDelay, maxSize;

    EXPECT_OK(vibrator->getSupportedPrimitives(&supported));
    EXPECT_OK(vibrator->getCompositionDelayMax(&maxDelay));
    EXPECT_OK(vibrator->getCompositionSizeMax(&maxSize));

    std::vector<CompositeEffect> composite;

    for (int i = 0; i < supported.size(); i++) {
        CompositePrimitive primitive = supported[i];
        float t = static_cast<float>(i + 1) / supported.size();
        CompositeEffect effect;

        effect.delayMs = maxDelay * t;
        effect.primitive = primitive;
        effect.scale = t;

        if (composite.size() == maxSize) {
            break;
        }
    }

    if (composite.size() != 0) {
        EXPECT_OK(vibrator->compose(composite, nullptr));
        EXPECT_OK(vibrator->off());
    }
}

TEST_P(VibratorAidl, ComposeUnsupportedPrimitives) {
    if (!(capabilities & IVibrator::CAP_COMPOSE_EFFECTS)) {
        GTEST_SKIP() << "CAP_COMPOSE_EFFECTS not supported";
    }

    std::vector<CompositePrimitive> unsupported(kInvalidPrimitives);
    std::vector<CompositePrimitive> supported;

    EXPECT_OK(vibrator->getSupportedPrimitives(&supported));

    for (CompositePrimitive primitive : kCompositePrimitives) {
        bool isPrimitiveSupported =
                std::find(supported.begin(), supported.end(), primitive) != supported.end();

        if (!isPrimitiveSupported) {
            unsupported.push_back(primitive);
        }
    }

    for (CompositePrimitive primitive : unsupported) {
        std::vector<CompositeEffect> composite(1);

        for (CompositeEffect& effect : composite) {
            effect.delayMs = 0;
            effect.primitive = primitive;
            effect.scale = 1.0f;
        }
        EXPECT_UNKNOWN_OR_UNSUPPORTED(vibrator->compose(composite, nullptr));
    }
}

TEST_P(VibratorAidl, ComposeScaleBoundary) {
    if (!(capabilities & IVibrator::CAP_COMPOSE_EFFECTS)) {
        GTEST_SKIP() << "CAP_COMPOSE_EFFECTS not supported";
    }

    std::vector<CompositeEffect> composite(1);
    CompositeEffect& effect = composite[0];

    effect.delayMs = 0;
    effect.primitive = CompositePrimitive::CLICK;

    effect.scale = std::nextafter(0.0f, -1.0f);
    EXPECT_ILLEGAL_ARGUMENT(vibrator->compose(composite, nullptr));

    effect.scale = 0.0f;
    EXPECT_OK(vibrator->compose(composite, nullptr));
    EXPECT_OK(vibrator->off());

    effect.scale = 1.0f;
    EXPECT_OK(vibrator->compose(composite, nullptr));
    EXPECT_OK(vibrator->off());

    effect.scale = std::nextafter(1.0f, 2.0f);
    EXPECT_ILLEGAL_ARGUMENT(vibrator->compose(composite, nullptr));
}

TEST_P(VibratorAidl, ComposeDelayBoundary) {
    if (!(capabilities & IVibrator::CAP_COMPOSE_EFFECTS)) {
        GTEST_SKIP() << "CAP_COMPOSE_EFFECTS not supported";
    }

    int32_t maxDelay;

    EXPECT_OK(vibrator->getCompositionDelayMax(&maxDelay));

    std::vector<CompositeEffect> composite(1);
    CompositeEffect& effect = composite[0];

    effect.primitive = CompositePrimitive::CLICK;
    effect.scale = 1.0f;

    effect.delayMs = 0;
    EXPECT_OK(vibrator->compose(composite, nullptr));
    EXPECT_OK(vibrator->off());

    effect.delayMs = 1;
    EXPECT_OK(vibrator->compose(composite, nullptr));
    EXPECT_OK(vibrator->off());

    effect.delayMs = maxDelay;
    EXPECT_OK(vibrator->compose(composite, nullptr));
    EXPECT_OK(vibrator->off());

    effect.delayMs = maxDelay + 1;
    EXPECT_ILLEGAL_ARGUMENT(vibrator->compose(composite, nullptr));
}

TEST_P(VibratorAidl, ComposeSizeBoundary) {
    if (!(capabilities & IVibrator::CAP_COMPOSE_EFFECTS)) {
        GTEST_SKIP() << "CAP_COMPOSE_EFFECTS not supported";
    }

    int32_t maxSize;

    EXPECT_OK(vibrator->getCompositionSizeMax(&maxSize));

    std::vector<CompositeEffect> composite(maxSize);
    CompositeEffect effect;

    effect.delayMs = 1;
    effect.primitive = CompositePrimitive::CLICK;
    effect.scale = 1.0f;

    std::fill(composite.begin(), composite.end(), effect);
    EXPECT_OK(vibrator->compose(composite, nullptr));
    EXPECT_OK(vibrator->off());

    composite.emplace_back(effect);
    EXPECT_ILLEGAL_ARGUMENT(vibrator->compose(composite, nullptr));
}

TEST_P(VibratorAidl, ComposeCallback) {
    if (!(capabilities & IVibrator::CAP_COMPOSE_EFFECTS)) {
        GTEST_SKIP() << "CAP_COMPOSE_EFFECTS not supported";
    }

    std::vector<CompositePrimitive> supported;
    EXPECT_OK(vibrator->getSupportedPrimitives(&supported));

    for (CompositePrimitive primitive : supported) {
        if (primitive == CompositePrimitive::NOOP) {
            continue;
        }

        std::promise<void> completionPromise;
        std::future<void> completionFuture{completionPromise.get_future()};
        auto callback = ndk::SharedRefBase::make<CompletionCallback>(
                [&completionPromise] { completionPromise.set_value(); });
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

        EXPECT_OK(vibrator->getPrimitiveDuration(primitive, &durationMs))
                << "\n  For primitive: " << toString(primitive);
        duration = std::chrono::milliseconds(durationMs);

        start = high_resolution_clock::now();
        EXPECT_OK(vibrator->compose(composite, callback))
                << "\n  For primitive: " << toString(primitive);

        EXPECT_EQ(completionFuture.wait_for(duration + VIBRATION_CALLBACK_TIMEOUT),
                  std::future_status::ready)
                << "\n  For primitive: " << toString(primitive);
        end = high_resolution_clock::now();

        elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        EXPECT_GE(elapsed.count(), duration.count())
                << "\n  For primitive: " << toString(primitive);

        EXPECT_OK(vibrator->off()) << "\n  For primitive: " << toString(primitive);
    }
}

TEST_P(VibratorAidl, AlwaysOn) {
    if (capabilities & IVibrator::CAP_ALWAYS_ON_CONTROL) {
        std::vector<Effect> supported;
        EXPECT_OK(vibrator->getSupportedAlwaysOnEffects(&supported));

        for (Effect effect : kEffects) {
            bool isEffectSupported =
                std::find(supported.begin(), supported.end(), effect) != supported.end();

            for (EffectStrength strength : kEffectStrengths) {
                ndk::ScopedAStatus status = vibrator->alwaysOnEnable(0, effect, strength);

                if (isEffectSupported) {
                    EXPECT_OK(std::move(status))
                            << "\n  For effect: " << toString(effect) << " " << toString(strength);
                } else {
                    EXPECT_UNKNOWN_OR_UNSUPPORTED(std::move(status))
                            << "\n  For effect: " << toString(effect) << " " << toString(strength);
                }
            }
        }

        EXPECT_OK(vibrator->alwaysOnDisable(0));
    }
}

TEST_P(VibratorAidl, GetResonantFrequency) {
    getResonantFrequencyHz(vibrator, capabilities);
}

TEST_P(VibratorAidl, GetQFactor) {
    float qFactor;
    ndk::ScopedAStatus status = vibrator->getQFactor(&qFactor);
    if (capabilities & IVibrator::CAP_GET_Q_FACTOR) {
        EXPECT_OK(std::move(status));
        ASSERT_GT(qFactor, 0);
    } else {
        EXPECT_UNKNOWN_OR_UNSUPPORTED(std::move(status));
    }
}

TEST_P(VibratorAidl, GetFrequencyResolution) {
    getFrequencyResolutionHz(vibrator, capabilities, version);
}

TEST_P(VibratorAidl, GetFrequencyMinimum) {
    getFrequencyMinimumHz(vibrator, capabilities, version);
}

TEST_P(VibratorAidl, GetBandwidthAmplitudeMap) {
    std::vector<float> bandwidthAmplitudeMap;
    ndk::ScopedAStatus status = vibrator->getBandwidthAmplitudeMap(&bandwidthAmplitudeMap);

    if (shouldValidateLegacyFrequencyControlResult(capabilities, version, status)) {
        EXPECT_OK(std::move(status));
        ASSERT_FALSE(bandwidthAmplitudeMap.empty());

        int minMapSize = (getResonantFrequencyHz(vibrator, capabilities) -
                          getFrequencyMinimumHz(vibrator, capabilities, version)) /
                         getFrequencyResolutionHz(vibrator, capabilities, version);
        ASSERT_GT(bandwidthAmplitudeMap.size(), minMapSize);

        for (float e : bandwidthAmplitudeMap) {
            ASSERT_GE(e, 0.0);
            ASSERT_LE(e, 1.0);
        }
    } else {
        EXPECT_UNKNOWN_OR_UNSUPPORTED(std::move(status));
    }
}

TEST_P(VibratorAidl, GetPwlePrimitiveDurationMax) {
    int32_t durationMs;
    ndk::ScopedAStatus status = vibrator->getPwlePrimitiveDurationMax(&durationMs);
    if (capabilities & IVibrator::CAP_COMPOSE_PWLE_EFFECTS) {
        EXPECT_OK(std::move(status));
        ASSERT_NE(durationMs, 0);
    } else {
        EXPECT_UNKNOWN_OR_UNSUPPORTED(std::move(status));
    }
}

TEST_P(VibratorAidl, GetPwleCompositionSizeMax) {
    int32_t maxSize;
    ndk::ScopedAStatus status = vibrator->getPwleCompositionSizeMax(&maxSize);
    if (capabilities & IVibrator::CAP_COMPOSE_PWLE_EFFECTS) {
        EXPECT_OK(std::move(status));
        ASSERT_NE(maxSize, 0);
    } else {
        EXPECT_UNKNOWN_OR_UNSUPPORTED(std::move(status));
    }
}

TEST_P(VibratorAidl, GetSupportedBraking) {
    std::vector<Braking> supported;
    ndk::ScopedAStatus status = vibrator->getSupportedBraking(&supported);
    if (capabilities & IVibrator::CAP_COMPOSE_PWLE_EFFECTS) {
        bool isDefaultNoneSupported =
            std::find(supported.begin(), supported.end(), Braking::NONE) != supported.end();
        EXPECT_OK(std::move(status));
        ASSERT_TRUE(isDefaultNoneSupported);
    } else {
        EXPECT_UNKNOWN_OR_UNSUPPORTED(std::move(status));
    }
}

TEST_P(VibratorAidl, ComposeValidPwle) {
    if (capabilities & IVibrator::CAP_COMPOSE_PWLE_EFFECTS) {
        ActivePwle firstActive = composeValidActivePwle(vibrator, capabilities, version);

        std::vector<Braking> supported;
        EXPECT_OK(vibrator->getSupportedBraking(&supported));
        bool isClabSupported =
            std::find(supported.begin(), supported.end(), Braking::CLAB) != supported.end();
        BrakingPwle firstBraking;
        firstBraking.braking = isClabSupported ? Braking::CLAB : Braking::NONE;
        firstBraking.duration = 100;

        ActivePwle secondActive = composeValidActivePwle(vibrator, capabilities, version);
        if (capabilities & IVibrator::CAP_FREQUENCY_CONTROL) {
            float minFrequencyHz = getFrequencyMinimumHz(vibrator, capabilities, version);
            float maxFrequencyHz = getFrequencyMaximumHz(vibrator, capabilities, version);
            float freqResolutionHz = getFrequencyResolutionHz(vibrator, capabilities, version);
            // As of API 16 these APIs are deprecated and no longer required to be implemented
            //  with frequency control capability.
            if (minFrequencyHz >= 0 && maxFrequencyHz >= 0 && freqResolutionHz >= 0) {
                secondActive.startFrequency = minFrequencyHz + (freqResolutionHz / 2.0f);
                secondActive.endFrequency = maxFrequencyHz - (freqResolutionHz / 3.0f);
            }
        }
        BrakingPwle secondBraking;
        secondBraking.braking = Braking::NONE;
        secondBraking.duration = 10;

        std::vector<PrimitivePwle> pwleQueue = {firstActive, firstBraking, secondActive,
                                                secondBraking};

        EXPECT_OK(vibrator->composePwle(pwleQueue, nullptr));
        EXPECT_OK(vibrator->off());
    }
}

TEST_P(VibratorAidl, ComposeValidPwleWithCallback) {
    if (!((capabilities & IVibrator::CAP_ON_CALLBACK) &&
          (capabilities & IVibrator::CAP_COMPOSE_PWLE_EFFECTS)))
        return;

    std::promise<void> completionPromise;
    std::future<void> completionFuture{completionPromise.get_future()};
    auto callback = ndk::SharedRefBase::make<CompletionCallback>(
            [&completionPromise] { completionPromise.set_value(); });
    int32_t segmentDurationMaxMs;
    vibrator->getPwlePrimitiveDurationMax(&segmentDurationMaxMs);
    uint32_t durationMs = segmentDurationMaxMs * 2 + 100;  // Sum of 2 active and 1 braking below
    auto timeout = std::chrono::milliseconds(durationMs) + VIBRATION_CALLBACK_TIMEOUT;

    ActivePwle active = composeValidActivePwle(vibrator, capabilities, version);

    std::vector<Braking> supported;
    EXPECT_OK(vibrator->getSupportedBraking(&supported));
    bool isClabSupported =
        std::find(supported.begin(), supported.end(), Braking::CLAB) != supported.end();
    BrakingPwle braking;
    braking.braking = isClabSupported ? Braking::CLAB : Braking::NONE;
    braking.duration = 100;

    std::vector<PrimitivePwle> pwleQueue = {active, braking, active};

    EXPECT_OK(vibrator->composePwle(pwleQueue, callback));
    EXPECT_EQ(completionFuture.wait_for(timeout), std::future_status::ready);
    EXPECT_OK(vibrator->off());
}

TEST_P(VibratorAidl, ComposePwleSegmentBoundary) {
    if (capabilities & IVibrator::CAP_COMPOSE_PWLE_EFFECTS) {
        std::vector<PrimitivePwle> pwleQueue;
        // test empty queue
        EXPECT_ILLEGAL_ARGUMENT(vibrator->composePwle(pwleQueue, nullptr));

        ActivePwle active = composeValidActivePwle(vibrator, capabilities, version);

        PrimitivePwle pwle;
        pwle = active;
        int segmentCountMax;
        vibrator->getPwleCompositionSizeMax(&segmentCountMax);

        // Create PWLE queue with more segments than allowed
        for (int i = 0; i < segmentCountMax + 10; i++) {
            pwleQueue.emplace_back(std::move(pwle));
        }

        EXPECT_ILLEGAL_ARGUMENT(vibrator->composePwle(pwleQueue, nullptr));
    }
}

TEST_P(VibratorAidl, ComposePwleAmplitudeParameterBoundary) {
    if (capabilities & IVibrator::CAP_COMPOSE_PWLE_EFFECTS) {
        ActivePwle active = composeValidActivePwle(vibrator, capabilities, version);
        active.startAmplitude = getAmplitudeMax() + 1.0;  // Amplitude greater than allowed
        active.endAmplitude = getAmplitudeMax() + 1.0;    // Amplitude greater than allowed

        std::vector<PrimitivePwle> pwleQueueGreater = {active};

        EXPECT_ILLEGAL_ARGUMENT(vibrator->composePwle(pwleQueueGreater, nullptr));

        active.startAmplitude = getAmplitudeMin() - 1.0;  // Amplitude less than allowed
        active.endAmplitude = getAmplitudeMin() - 1.0;    // Amplitude less than allowed

        std::vector<PrimitivePwle> pwleQueueLess = {active};

        EXPECT_ILLEGAL_ARGUMENT(vibrator->composePwle(pwleQueueLess, nullptr));
    }
}

TEST_P(VibratorAidl, ComposePwleFrequencyParameterBoundary) {
    if ((capabilities & IVibrator::CAP_COMPOSE_PWLE_EFFECTS) &&
        (capabilities & IVibrator::CAP_FREQUENCY_CONTROL)) {
        float freqMinimumHz = getFrequencyMinimumHz(vibrator, capabilities, version);
        float freqMaximumHz = getFrequencyMaximumHz(vibrator, capabilities, version);
        float freqResolutionHz = getFrequencyResolutionHz(vibrator, capabilities, version);

        // As of API 16 these APIs are deprecated and no longer required to be implemented with
        // frequency control capability.
        if (freqMinimumHz < 0 || freqMaximumHz < 0 || freqResolutionHz < 0) {
            GTEST_SKIP() << "PWLE V1 is not supported, skipping test";
            return;
        }

        ActivePwle active = composeValidActivePwle(vibrator, capabilities, version);
        active.startFrequency =
            freqMaximumHz + freqResolutionHz;                    // Frequency greater than allowed
        active.endFrequency = freqMaximumHz + freqResolutionHz;  // Frequency greater than allowed

        std::vector<PrimitivePwle> pwleQueueGreater = {active};

        EXPECT_ILLEGAL_ARGUMENT(vibrator->composePwle(pwleQueueGreater, nullptr));

        active.startFrequency = freqMinimumHz - freqResolutionHz;  // Frequency less than allowed
        active.endFrequency = freqMinimumHz - freqResolutionHz;    // Frequency less than allowed

        std::vector<PrimitivePwle> pwleQueueLess = {active};

        EXPECT_ILLEGAL_ARGUMENT(vibrator->composePwle(pwleQueueLess, nullptr));
    }
}

TEST_P(VibratorAidl, ComposePwleSegmentDurationBoundary) {
    if (capabilities & IVibrator::CAP_COMPOSE_PWLE_EFFECTS) {
        ActivePwle active = composeValidActivePwle(vibrator, capabilities, version);

        int32_t segmentDurationMaxMs;
        vibrator->getPwlePrimitiveDurationMax(&segmentDurationMaxMs);
        active.duration = segmentDurationMaxMs + 10;  // Segment duration greater than allowed

        std::vector<PrimitivePwle> pwleQueue = {active};

        EXPECT_ILLEGAL_ARGUMENT(vibrator->composePwle(pwleQueue, nullptr));
    }
}

TEST_P(VibratorAidl, FrequencyToOutputAccelerationMapHasValidFrequencyRange) {
    if (version < PWLE_V2_MIN_VERSION || !(capabilities & IVibrator::CAP_FREQUENCY_CONTROL)) {
        GTEST_SKIP() << "Frequency control is not supported, skipping test";
        return;
    }

    std::vector<FrequencyAccelerationMapEntry> frequencyToOutputAccelerationMap;
    ndk::ScopedAStatus status =
            vibrator->getFrequencyToOutputAccelerationMap(&frequencyToOutputAccelerationMap);
    EXPECT_OK(std::move(status));
    ASSERT_FALSE(frequencyToOutputAccelerationMap.empty());
    auto sharpnessRange =
            pwle_v2_utils::getPwleV2SharpnessRange(vibrator, frequencyToOutputAccelerationMap);
    // Validate the curve provides a usable sharpness range, which is a range of frequencies
    // that are supported by the device.
    ASSERT_TRUE(sharpnessRange.first >= 0);
    // Validate that the sharpness range is a valid interval, not a single point.
    ASSERT_TRUE(sharpnessRange.first < sharpnessRange.second);
}

TEST_P(VibratorAidl, FrequencyToOutputAccelerationMapUnsupported) {
    if ((capabilities & IVibrator::CAP_FREQUENCY_CONTROL)) return;

    std::vector<FrequencyAccelerationMapEntry> frequencyToOutputAccelerationMap;

    EXPECT_UNKNOWN_OR_UNSUPPORTED(
            vibrator->getFrequencyToOutputAccelerationMap(&frequencyToOutputAccelerationMap));
}

TEST_P(VibratorAidl, GetPwleV2PrimitiveDurationMaxMillis) {
    if (!(capabilities & IVibrator::CAP_COMPOSE_PWLE_EFFECTS_V2)) {
        GTEST_SKIP() << "PWLE V2 not supported, skipping test";
        return;
    }

    int32_t durationMs;
    ndk::ScopedAStatus status = vibrator->getPwleV2PrimitiveDurationMaxMillis(&durationMs);
    EXPECT_OK(std::move(status));
    ASSERT_GT(durationMs, 0);  // Ensure greater than zero
    ASSERT_GE(durationMs, pwle_v2_utils::COMPOSE_PWLE_V2_MIN_REQUIRED_PRIMITIVE_MAX_DURATION_MS);
}

TEST_P(VibratorAidl, GetPwleV2CompositionSizeMax) {
    if (!(capabilities & IVibrator::CAP_COMPOSE_PWLE_EFFECTS_V2)) {
        GTEST_SKIP() << "PWLE V2 not supported, skipping test";
        return;
    }

    int32_t maxSize;
    ndk::ScopedAStatus status = vibrator->getPwleV2CompositionSizeMax(&maxSize);
    EXPECT_OK(std::move(status));
    ASSERT_GT(maxSize, 0);  // Ensure greater than zero
    ASSERT_GE(maxSize, pwle_v2_utils::COMPOSE_PWLE_V2_MIN_REQUIRED_SIZE);
}

TEST_P(VibratorAidl, GetPwleV2PrimitiveDurationMinMillis) {
    if (!(capabilities & IVibrator::CAP_COMPOSE_PWLE_EFFECTS_V2)) {
        GTEST_SKIP() << "PWLE V2 not supported, skipping test";
        return;
    }

    int32_t durationMs;
    ndk::ScopedAStatus status = vibrator->getPwleV2PrimitiveDurationMinMillis(&durationMs);
    EXPECT_OK(std::move(status));
    ASSERT_GT(durationMs, 0);  // Ensure greater than zero
    ASSERT_LE(durationMs, pwle_v2_utils::COMPOSE_PWLE_V2_MAX_ALLOWED_PRIMITIVE_MIN_DURATION_MS);
}

TEST_P(VibratorAidl, ValidatePwleV2DependencyOnFrequencyControl) {
    if (!(capabilities & IVibrator::CAP_COMPOSE_PWLE_EFFECTS_V2)) {
        GTEST_SKIP() << "PWLE V2 not supported, skipping test";
        return;
    }

    // Check if frequency control is supported
    bool hasFrequencyControl = (capabilities & IVibrator::CAP_FREQUENCY_CONTROL) != 0;
    ASSERT_TRUE(hasFrequencyControl) << "Frequency control MUST be supported when PWLE V2 is.";
}

TEST_P(VibratorAidl, ComposeValidPwleV2Effect) {
    if (!(capabilities & IVibrator::CAP_COMPOSE_PWLE_EFFECTS_V2)) {
        GTEST_SKIP() << "PWLE V2 not supported, skipping test";
        return;
    }

    EXPECT_OK(vibrator->composePwleV2(pwle_v2_utils::composeValidPwleV2Effect(vibrator), nullptr));
    EXPECT_OK(vibrator->off());
}

TEST_P(VibratorAidl, ComposePwleV2Unsupported) {
    if (version < PWLE_V2_MIN_VERSION) {
        EXPECT_EQ(capabilities & IVibrator::CAP_COMPOSE_PWLE_EFFECTS_V2, 0)
                << "Vibrator version " << version << " should not report PWLE V2 capability.";
    }
    if ((capabilities & IVibrator::CAP_COMPOSE_PWLE_EFFECTS_V2)) return;

    CompositePwleV2 composite;
    composite.pwlePrimitives.emplace_back(/*amplitude=*/1.0f, /*frequencyHz=*/100.0f,
                                          /*timeMillis=*/50);

    EXPECT_UNKNOWN_OR_UNSUPPORTED(vibrator->composePwleV2(composite, nullptr));
}

TEST_P(VibratorAidl, ComposeValidPwleV2EffectWithCallback) {
    if (!(capabilities & IVibrator::CAP_COMPOSE_PWLE_EFFECTS_V2)) {
        GTEST_SKIP() << "PWLE V2 not supported, skipping test";
        return;
    }

    std::promise<void> completionPromise;
    std::future<void> completionFuture{completionPromise.get_future()};
    auto callback = ndk::SharedRefBase::make<CompletionCallback>(
            [&completionPromise] { completionPromise.set_value(); });

    int32_t minDuration;
    EXPECT_OK(vibrator->getPwleV2PrimitiveDurationMinMillis(&minDuration));
    auto timeout = std::chrono::milliseconds(minDuration) + VIBRATION_CALLBACK_TIMEOUT;
    float minFrequency = pwle_v2_utils::getPwleV2FrequencyMinHz(vibrator);

    CompositePwleV2 composite;
    composite.pwlePrimitives.emplace_back(/*amplitude=*/0.5, minFrequency, minDuration);

    EXPECT_OK(vibrator->composePwleV2(composite, callback));
    EXPECT_EQ(completionFuture.wait_for(timeout), std::future_status::ready);
    EXPECT_OK(vibrator->off());
}

TEST_P(VibratorAidl, composePwleV2EffectWithTooManyPoints) {
    if (!(capabilities & IVibrator::CAP_COMPOSE_PWLE_EFFECTS_V2)) {
        GTEST_SKIP() << "PWLE V2 not supported, skipping test";
        return;
    }

    EXPECT_ILLEGAL_ARGUMENT(vibrator->composePwleV2(
            pwle_v2_utils::composePwleV2EffectWithTooManyPoints(vibrator), nullptr));
}

TEST_P(VibratorAidl, composeInvalidPwleV2Effect) {
    if (!(capabilities & IVibrator::CAP_COMPOSE_PWLE_EFFECTS_V2)) {
        GTEST_SKIP() << "PWLE V2 not supported, skipping test";
        return;
    }

    // Retrieve min and max durations
    int32_t minDurationMs, maxDurationMs;
    EXPECT_OK(vibrator->getPwleV2PrimitiveDurationMinMillis(&minDurationMs));
    EXPECT_OK(vibrator->getPwleV2PrimitiveDurationMaxMillis(&maxDurationMs));

    CompositePwleV2 composePwle;

    // Negative amplitude
    composePwle.pwlePrimitives.push_back(
            PwleV2Primitive(/*amplitude=*/-0.8f, /*frequency=*/100, minDurationMs));
    EXPECT_ILLEGAL_ARGUMENT(vibrator->composePwleV2(composePwle, nullptr))
            << "Composing PWLE V2 effect with negative amplitude should fail";
    composePwle.pwlePrimitives.clear();

    // Amplitude exceeding 1.0
    composePwle.pwlePrimitives.push_back(
            PwleV2Primitive(/*amplitude=*/1.2f, /*frequency=*/100, minDurationMs));
    EXPECT_ILLEGAL_ARGUMENT(vibrator->composePwleV2(composePwle, nullptr))
            << "Composing PWLE V2 effect with amplitude greater than 1.0 should fail";
    composePwle.pwlePrimitives.clear();

    // Duration exceeding maximum
    composePwle.pwlePrimitives.push_back(
            PwleV2Primitive(/*amplitude=*/0.2f, /*frequency=*/100, maxDurationMs + 10));
    EXPECT_ILLEGAL_ARGUMENT(vibrator->composePwleV2(composePwle, nullptr))
            << "Composing PWLE V2 effect with duration exceeding maximum should fail";
    composePwle.pwlePrimitives.clear();

    // Negative duration
    composePwle.pwlePrimitives.push_back(
            PwleV2Primitive(/*amplitude=*/0.2f, /*frequency=*/100, /*time=*/-1));
    EXPECT_ILLEGAL_ARGUMENT(vibrator->composePwleV2(composePwle, nullptr))
            << "Composing PWLE V2 effect with negative duration should fail";
    composePwle.pwlePrimitives.clear();

    // Frequency below minimum
    float minFrequency = pwle_v2_utils::getPwleV2FrequencyMinHz(vibrator);
    composePwle.pwlePrimitives.push_back(
            PwleV2Primitive(/*amplitude=*/0.2f, minFrequency - 1, minDurationMs));
    EXPECT_ILLEGAL_ARGUMENT(vibrator->composePwleV2(composePwle, nullptr))
            << "Composing PWLE V2 effect with frequency below minimum should fail";
    composePwle.pwlePrimitives.clear();

    // Frequency above maximum
    float maxFrequency = pwle_v2_utils::getPwleV2FrequencyMaxHz(vibrator);
    composePwle.pwlePrimitives.push_back(
            PwleV2Primitive(/*amplitude=*/0.2f, maxFrequency + 1, minDurationMs));
    EXPECT_ILLEGAL_ARGUMENT(vibrator->composePwleV2(composePwle, nullptr))
            << "Composing PWLE V2 effect with frequency above maximum should fail";
}

std::vector<std::tuple<int32_t, int32_t>> GenerateVibratorMapping() {
    std::vector<std::tuple<int32_t, int32_t>> tuples;

    std::vector<std::string> managerNames = findVibratorManagerNames();
    std::vector<int32_t> vibratorIds;
    for (int i = 0; i < managerNames.size(); i++) {
        auto vibratorManager = IVibratorManager::fromBinder(
                ndk::SpAIBinder(AServiceManager_waitForService(managerNames[i].c_str())));
        if (vibratorManager->getVibratorIds(&vibratorIds).isOk()) {
            for (int32_t vibratorId : vibratorIds) {
                tuples.emplace_back(i, vibratorId);
            }
        }
    }

    std::vector<std::string> vibratorNames = findUnmanagedVibratorNames();
    for (int i = 0; i < vibratorNames.size(); i++) {
        tuples.emplace_back(-1, i);
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
    // Random values are used in the implementation.
    std::srand(std::time(nullptr));

    ::testing::InitGoogleTest(&argc, argv);
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
