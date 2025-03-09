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

#include "vibrator-impl/Vibrator.h"

#include <android-base/logging.h>
#include <thread>

namespace aidl {
namespace android {
namespace hardware {
namespace vibrator {

static constexpr int32_t COMPOSE_DELAY_MAX_MS = 1000;
static constexpr int32_t COMPOSE_SIZE_MAX = 256;
static constexpr int32_t COMPOSE_PWLE_SIZE_MAX = 127;
static constexpr int32_t COMPOSE_PWLE_V2_SIZE_MAX = 16;

static constexpr float Q_FACTOR = 11.0;
static constexpr int32_t COMPOSE_PWLE_PRIMITIVE_DURATION_MAX_MS = 16383;
static constexpr int32_t COMPOSE_PWLE_V2_PRIMITIVE_DURATION_MAX_MS = 1000;
static constexpr int32_t COMPOSE_PWLE_V2_PRIMITIVE_DURATION_MIN_MS = 20;
static constexpr float PWLE_LEVEL_MIN = 0.0;
static constexpr float PWLE_LEVEL_MAX = 1.0;
static constexpr float PWLE_FREQUENCY_RESOLUTION_HZ = 1.0;
static constexpr float PWLE_FREQUENCY_MIN_HZ = 140.0;
static constexpr float RESONANT_FREQUENCY_HZ = 150.0;
static constexpr float PWLE_FREQUENCY_MAX_HZ = 160.0;
static constexpr float PWLE_BW_MAP_SIZE =
        1 + ((PWLE_FREQUENCY_MAX_HZ - PWLE_FREQUENCY_MIN_HZ) / PWLE_FREQUENCY_RESOLUTION_HZ);

// Service specific error code used for vendor vibration effects.
static constexpr int32_t ERROR_CODE_INVALID_DURATION = 1;

void Vibrator::dispatchVibrate(int32_t timeoutMs,
                               const std::shared_ptr<IVibratorCallback>& callback) {
    std::lock_guard lock(mMutex);
    if (mIsVibrating) {
        // Already vibrating, ignore new request.
        return;
    }
    mVibrationCallback = callback;
    mIsVibrating = true;
    // Note that thread lambdas aren't using implicit capture [=], to avoid capturing "this",
    // which may be asynchronously destructed.
    std::thread([timeoutMs, callback, sharedThis = this->ref<Vibrator>()] {
        LOG(VERBOSE) << "Starting delayed callback on another thread";
        usleep(timeoutMs * 1000);

        if (sharedThis) {
            std::lock_guard lock(sharedThis->mMutex);
            sharedThis->mIsVibrating = false;
            if (sharedThis->mVibrationCallback && (callback == sharedThis->mVibrationCallback)) {
                LOG(VERBOSE) << "Notifying callback onComplete";
                if (!sharedThis->mVibrationCallback->onComplete().isOk()) {
                    LOG(ERROR) << "Failed to call onComplete";
                }
                sharedThis->mVibrationCallback = nullptr;
            }
            if (sharedThis->mGlobalVibrationCallback) {
                LOG(VERBOSE) << "Notifying global callback onComplete";
                if (!sharedThis->mGlobalVibrationCallback->onComplete().isOk()) {
                    LOG(ERROR) << "Failed to call onComplete";
                }
                sharedThis->mGlobalVibrationCallback = nullptr;
            }
        }
    }).detach();
}

void Vibrator::setGlobalVibrationCallback(const std::shared_ptr<IVibratorCallback>& callback) {
    std::lock_guard lock(mMutex);
    if (mIsVibrating) {
        mGlobalVibrationCallback = callback;
    } else if (callback) {
        std::thread([callback] {
            LOG(VERBOSE) << "Notifying global callback onComplete";
            if (!callback->onComplete().isOk()) {
                LOG(ERROR) << "Failed to call onComplete";
            }
        }).detach();
    }
}

ndk::ScopedAStatus Vibrator::getCapabilities(int32_t* _aidl_return) {
    LOG(VERBOSE) << "Vibrator reporting capabilities";
    std::lock_guard lock(mMutex);
    if (mCapabilities == 0) {
        int32_t version;
        if (!getInterfaceVersion(&version).isOk()) {
            return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_ILLEGAL_STATE));
        }
        mCapabilities = IVibrator::CAP_ON_CALLBACK | IVibrator::CAP_PERFORM_CALLBACK |
                        IVibrator::CAP_AMPLITUDE_CONTROL | IVibrator::CAP_EXTERNAL_CONTROL |
                        IVibrator::CAP_EXTERNAL_AMPLITUDE_CONTROL | IVibrator::CAP_COMPOSE_EFFECTS |
                        IVibrator::CAP_ALWAYS_ON_CONTROL | IVibrator::CAP_GET_RESONANT_FREQUENCY |
                        IVibrator::CAP_GET_Q_FACTOR | IVibrator::CAP_FREQUENCY_CONTROL |
                        IVibrator::CAP_COMPOSE_PWLE_EFFECTS;

        if (version >= 3) {
            mCapabilities |=
                    IVibrator::CAP_PERFORM_VENDOR_EFFECTS | IVibrator::CAP_COMPOSE_PWLE_EFFECTS_V2;
        }
    }

    *_aidl_return = mCapabilities;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::off() {
    LOG(VERBOSE) << "Vibrator off";
    std::lock_guard lock(mMutex);
    std::shared_ptr<IVibratorCallback> callback = mVibrationCallback;
    std::shared_ptr<IVibratorCallback> globalCallback = mGlobalVibrationCallback;
    mIsVibrating = false;
    mVibrationCallback = nullptr;
    mGlobalVibrationCallback = nullptr;
    if (callback || globalCallback) {
        std::thread([callback, globalCallback] {
            if (callback) {
                LOG(VERBOSE) << "Notifying callback onComplete";
                if (!callback->onComplete().isOk()) {
                    LOG(ERROR) << "Failed to call onComplete";
                }
            }
            if (globalCallback) {
                LOG(VERBOSE) << "Notifying global callback onComplete";
                if (!globalCallback->onComplete().isOk()) {
                    LOG(ERROR) << "Failed to call onComplete";
                }
            }
        }).detach();
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::on(int32_t timeoutMs,
                                const std::shared_ptr<IVibratorCallback>& callback) {
    LOG(VERBOSE) << "Vibrator on for timeoutMs: " << timeoutMs;
    dispatchVibrate(timeoutMs, callback);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::perform(Effect effect, EffectStrength strength,
                                     const std::shared_ptr<IVibratorCallback>& callback,
                                     int32_t* _aidl_return) {
    LOG(VERBOSE) << "Vibrator perform";

    if (effect != Effect::CLICK && effect != Effect::TICK) {
        return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
    }
    if (strength != EffectStrength::LIGHT && strength != EffectStrength::MEDIUM &&
        strength != EffectStrength::STRONG) {
        return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
    }

    constexpr size_t kEffectMillis = 100;
    dispatchVibrate(kEffectMillis, callback);
    *_aidl_return = kEffectMillis;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::performVendorEffect(
        const VendorEffect& effect, const std::shared_ptr<IVibratorCallback>& callback) {
    LOG(VERBOSE) << "Vibrator perform vendor effect";
    int32_t capabilities = 0;
    if (!getCapabilities(&capabilities).isOk()) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    if ((capabilities & IVibrator::CAP_PERFORM_VENDOR_EFFECTS) == 0) {
        return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
    }
    EffectStrength strength = effect.strength;
    if (strength != EffectStrength::LIGHT && strength != EffectStrength::MEDIUM &&
        strength != EffectStrength::STRONG) {
        return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_ILLEGAL_ARGUMENT));
    }
    float scale = effect.scale;
    if (scale <= 0) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
    float vendorScale = effect.vendorScale;
    if (vendorScale <= 0) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }

    int32_t durationMs = 0;
    if (!effect.vendorData.getInt("DURATION_MS", &durationMs) || durationMs <= 0) {
        return ndk::ScopedAStatus::fromServiceSpecificError(ERROR_CODE_INVALID_DURATION);
    }

    dispatchVibrate(durationMs, callback);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::getSupportedEffects(std::vector<Effect>* _aidl_return) {
    *_aidl_return = {Effect::CLICK, Effect::TICK};
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::setAmplitude(float amplitude) {
    LOG(VERBOSE) << "Vibrator set amplitude: " << amplitude;
    if (amplitude <= 0.0f || amplitude > 1.0f) {
        return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_ILLEGAL_ARGUMENT));
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::setExternalControl(bool enabled) {
    LOG(VERBOSE) << "Vibrator set external control: " << enabled;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::getCompositionDelayMax(int32_t* maxDelayMs) {
    *maxDelayMs = COMPOSE_DELAY_MAX_MS;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::getCompositionSizeMax(int32_t* maxSize) {
    *maxSize = COMPOSE_SIZE_MAX;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::getSupportedPrimitives(std::vector<CompositePrimitive>* supported) {
    *supported = {
            CompositePrimitive::NOOP,       CompositePrimitive::CLICK,
            CompositePrimitive::THUD,       CompositePrimitive::SPIN,
            CompositePrimitive::QUICK_RISE, CompositePrimitive::SLOW_RISE,
            CompositePrimitive::QUICK_FALL, CompositePrimitive::LIGHT_TICK,
            CompositePrimitive::LOW_TICK,
    };
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::getPrimitiveDuration(CompositePrimitive primitive,
                                                  int32_t* durationMs) {
    std::vector<CompositePrimitive> supported;
    getSupportedPrimitives(&supported);
    if (std::find(supported.begin(), supported.end(), primitive) == supported.end()) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }
    if (primitive != CompositePrimitive::NOOP) {
        *durationMs = 100;
    } else {
        *durationMs = 0;
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::compose(const std::vector<CompositeEffect>& composite,
                                     const std::shared_ptr<IVibratorCallback>& callback) {
    if (composite.size() > COMPOSE_SIZE_MAX) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }

    std::vector<CompositePrimitive> supported;
    getSupportedPrimitives(&supported);

    for (auto& e : composite) {
        if (e.delayMs > COMPOSE_DELAY_MAX_MS) {
            return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
        }
        if (e.scale < 0.0f || e.scale > 1.0f) {
            return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
        }
        if (std::find(supported.begin(), supported.end(), e.primitive) == supported.end()) {
            return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
        }
    }

    int32_t totalDuration = 0;
    for (auto& e : composite) {
        int32_t durationMs;
        getPrimitiveDuration(e.primitive, &durationMs);
        totalDuration += e.delayMs + durationMs;
    }

    dispatchVibrate(totalDuration, callback);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::getSupportedAlwaysOnEffects(std::vector<Effect>* _aidl_return) {
    return getSupportedEffects(_aidl_return);
}

ndk::ScopedAStatus Vibrator::alwaysOnEnable(int32_t id, Effect effect, EffectStrength strength) {
    std::vector<Effect> effects;
    getSupportedAlwaysOnEffects(&effects);

    if (std::find(effects.begin(), effects.end(), effect) == effects.end()) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    } else {
        LOG(VERBOSE) << "Enabling always-on ID " << id << " with " << toString(effect) << "/"
                     << toString(strength);
        return ndk::ScopedAStatus::ok();
    }
}

ndk::ScopedAStatus Vibrator::alwaysOnDisable(int32_t id) {
    LOG(VERBOSE) << "Disabling always-on ID " << id;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::getResonantFrequency(float *resonantFreqHz) {
    *resonantFreqHz = RESONANT_FREQUENCY_HZ;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::getQFactor(float *qFactor) {
    *qFactor = Q_FACTOR;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::getFrequencyResolution(float *freqResolutionHz) {
    *freqResolutionHz = PWLE_FREQUENCY_RESOLUTION_HZ;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::getFrequencyMinimum(float *freqMinimumHz) {
    *freqMinimumHz = PWLE_FREQUENCY_MIN_HZ;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::getBandwidthAmplitudeMap(std::vector<float> *_aidl_return) {
    // The output BandwidthAmplitudeMap will be as below and the maximum
    // amplitude 1.0 will be set on RESONANT_FREQUENCY_HZ
    // {0.9, 0.91, 0.92, 0.93, 0.94, 0.95, 0.96, 0.97, 0.98, 0.99, 1, 0.99, 0.98, 0.97,
    // 0.96, 0.95, 0.94, 0.93, 0.92, 0.91, 0.9}
    int32_t capabilities = 0;
    int halfMapSize = PWLE_BW_MAP_SIZE / 2;
    Vibrator::getCapabilities(&capabilities);
    if (capabilities & IVibrator::CAP_FREQUENCY_CONTROL) {
        std::vector<float> bandwidthAmplitudeMap(PWLE_BW_MAP_SIZE, PWLE_LEVEL_MAX);
        for (int i = 0; i < halfMapSize; ++i) {
            bandwidthAmplitudeMap[halfMapSize + i + 1] =
                    bandwidthAmplitudeMap[halfMapSize + i] - 0.01;
            bandwidthAmplitudeMap[halfMapSize - i - 1] =
                    bandwidthAmplitudeMap[halfMapSize - i] - 0.01;
        }
        *_aidl_return = bandwidthAmplitudeMap;
        return ndk::ScopedAStatus::ok();
    } else {
        return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }
}

ndk::ScopedAStatus Vibrator::getPwlePrimitiveDurationMax(int32_t *durationMs) {
    *durationMs = COMPOSE_PWLE_PRIMITIVE_DURATION_MAX_MS;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::getPwleCompositionSizeMax(int32_t *maxSize) {
    *maxSize = COMPOSE_PWLE_SIZE_MAX;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::getSupportedBraking(std::vector<Braking> *supported) {
    *supported = {
            Braking::NONE,
            Braking::CLAB,
    };
    return ndk::ScopedAStatus::ok();
}

void resetPreviousEndAmplitudeEndFrequency(float &prevEndAmplitude, float &prevEndFrequency) {
    const float reset = -1.0;
    prevEndAmplitude = reset;
    prevEndFrequency = reset;
}

void incrementIndex(int &index) {
    index += 1;
}

void constructActiveDefaults(std::ostringstream &pwleBuilder, const int &segmentIdx) {
    pwleBuilder << ",C" << segmentIdx << ":1";
    pwleBuilder << ",B" << segmentIdx << ":0";
    pwleBuilder << ",AR" << segmentIdx << ":0";
    pwleBuilder << ",V" << segmentIdx << ":0";
}

void constructActiveSegment(std::ostringstream &pwleBuilder, const int &segmentIdx, int duration,
                            float amplitude, float frequency) {
    pwleBuilder << ",T" << segmentIdx << ":" << duration;
    pwleBuilder << ",L" << segmentIdx << ":" << amplitude;
    pwleBuilder << ",F" << segmentIdx << ":" << frequency;
    constructActiveDefaults(pwleBuilder, segmentIdx);
}

void constructBrakingSegment(std::ostringstream &pwleBuilder, const int &segmentIdx, int duration,
                             Braking brakingType) {
    pwleBuilder << ",T" << segmentIdx << ":" << duration;
    pwleBuilder << ",L" << segmentIdx << ":" << 0;
    pwleBuilder << ",F" << segmentIdx << ":" << 0;
    pwleBuilder << ",C" << segmentIdx << ":0";
    pwleBuilder << ",B" << segmentIdx << ":"
                << static_cast<std::underlying_type<Braking>::type>(brakingType);
    pwleBuilder << ",AR" << segmentIdx << ":0";
    pwleBuilder << ",V" << segmentIdx << ":0";
}

ndk::ScopedAStatus Vibrator::composePwle(const std::vector<PrimitivePwle> &composite,
                                         const std::shared_ptr<IVibratorCallback> &callback) {
    std::ostringstream pwleBuilder;
    std::string pwleQueue;

    int compositionSizeMax;
    getPwleCompositionSizeMax(&compositionSizeMax);
    if (composite.size() <= 0 || composite.size() > compositionSizeMax) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }

    float prevEndAmplitude;
    float prevEndFrequency;
    resetPreviousEndAmplitudeEndFrequency(prevEndAmplitude, prevEndFrequency);

    int segmentIdx = 0;
    uint32_t totalDuration = 0;

    pwleBuilder << "S:0,WF:4,RP:0,WT:0";

    for (auto &e : composite) {
        switch (e.getTag()) {
            case PrimitivePwle::active: {
                auto active = e.get<PrimitivePwle::active>();
                if (active.duration < 0 ||
                    active.duration > COMPOSE_PWLE_PRIMITIVE_DURATION_MAX_MS) {
                    return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
                }
                if (active.startAmplitude < PWLE_LEVEL_MIN ||
                    active.startAmplitude > PWLE_LEVEL_MAX ||
                    active.endAmplitude < PWLE_LEVEL_MIN || active.endAmplitude > PWLE_LEVEL_MAX) {
                    return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
                }
                if (active.startFrequency < PWLE_FREQUENCY_MIN_HZ ||
                    active.startFrequency > PWLE_FREQUENCY_MAX_HZ ||
                    active.endFrequency < PWLE_FREQUENCY_MIN_HZ ||
                    active.endFrequency > PWLE_FREQUENCY_MAX_HZ) {
                    return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
                }

                if (!((active.startAmplitude == prevEndAmplitude) &&
                      (active.startFrequency == prevEndFrequency))) {
                    constructActiveSegment(pwleBuilder, segmentIdx, 0, active.startAmplitude,
                                           active.startFrequency);
                    incrementIndex(segmentIdx);
                }

                constructActiveSegment(pwleBuilder, segmentIdx, active.duration,
                                       active.endAmplitude, active.endFrequency);
                incrementIndex(segmentIdx);

                prevEndAmplitude = active.endAmplitude;
                prevEndFrequency = active.endFrequency;
                totalDuration += active.duration;
                break;
            }
            case PrimitivePwle::braking: {
                auto braking = e.get<PrimitivePwle::braking>();
                if (braking.braking > Braking::CLAB) {
                    return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
                }
                if (braking.duration > COMPOSE_PWLE_PRIMITIVE_DURATION_MAX_MS) {
                    return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
                }

                constructBrakingSegment(pwleBuilder, segmentIdx, 0, braking.braking);
                incrementIndex(segmentIdx);

                constructBrakingSegment(pwleBuilder, segmentIdx, braking.duration, braking.braking);
                incrementIndex(segmentIdx);

                resetPreviousEndAmplitudeEndFrequency(prevEndAmplitude, prevEndFrequency);
                totalDuration += braking.duration;
                break;
            }
        }
    }

    dispatchVibrate(totalDuration, callback);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::getFrequencyToOutputAccelerationMap(
        std::vector<FrequencyAccelerationMapEntry>* _aidl_return) {
    int32_t capabilities = 0;
    if (!getCapabilities(&capabilities).isOk()) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    if (!(capabilities & IVibrator::CAP_FREQUENCY_CONTROL)) {
        return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
    }

    std::vector<FrequencyAccelerationMapEntry> frequencyToOutputAccelerationMap;

    std::vector<std::pair<float, float>> frequencyToOutputAccelerationData = {
            {30.0f, 0.01f},  {46.0f, 0.09f},  {50.0f, 0.1f},   {55.0f, 0.12f},  {62.0f, 0.66f},
            {83.0f, 0.82f},  {85.0f, 0.85f},  {92.0f, 1.05f},  {107.0f, 1.63f}, {115.0f, 1.72f},
            {123.0f, 1.81f}, {135.0f, 2.23f}, {144.0f, 2.47f}, {145.0f, 2.5f},  {150.0f, 3.0f},
            {175.0f, 2.51f}, {181.0f, 2.41f}, {190.0f, 2.28f}, {200.0f, 2.08f}, {204.0f, 1.96f},
            {205.0f, 1.9f},  {224.0f, 1.7f},  {235.0f, 1.5f},  {242.0f, 1.46f}, {253.0f, 1.41f},
            {263.0f, 1.39f}, {65.0f, 1.38f},  {278.0f, 1.37f}, {294.0f, 1.35f}, {300.0f, 1.34f}};
    for (const auto& entry : frequencyToOutputAccelerationData) {
        frequencyToOutputAccelerationMap.push_back(
                FrequencyAccelerationMapEntry(/*frequency=*/entry.first,
                                              /*maxOutputAcceleration=*/entry.second));
    }

    *_aidl_return = frequencyToOutputAccelerationMap;

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::getPwleV2PrimitiveDurationMaxMillis(int32_t* maxDurationMs) {
    *maxDurationMs = COMPOSE_PWLE_V2_PRIMITIVE_DURATION_MAX_MS;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::getPwleV2CompositionSizeMax(int32_t* maxSize) {
    *maxSize = COMPOSE_PWLE_V2_SIZE_MAX;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::getPwleV2PrimitiveDurationMinMillis(int32_t* minDurationMs) {
    *minDurationMs = COMPOSE_PWLE_V2_PRIMITIVE_DURATION_MIN_MS;
    return ndk::ScopedAStatus::ok();
}

float getPwleV2FrequencyMinHz(
        std::vector<FrequencyAccelerationMapEntry> frequencyToOutputAccelerationMap) {
    if (frequencyToOutputAccelerationMap.empty()) {
        return 0.0f;
    }

    float minFrequency = frequencyToOutputAccelerationMap[0].frequencyHz;

    for (const auto& entry : frequencyToOutputAccelerationMap) {
        if (entry.frequencyHz < minFrequency) {
            minFrequency = entry.frequencyHz;
        }
    }

    return minFrequency;
}

float getPwleV2FrequencyMaxHz(
        std::vector<FrequencyAccelerationMapEntry> frequencyToOutputAccelerationMap) {
    if (frequencyToOutputAccelerationMap.empty()) {
        return 0.0f;
    }

    float maxFrequency = frequencyToOutputAccelerationMap[0].frequencyHz;

    for (const auto& entry : frequencyToOutputAccelerationMap) {
        if (entry.frequencyHz > maxFrequency) {
            maxFrequency = entry.frequencyHz;
        }
    }

    return maxFrequency;
}

ndk::ScopedAStatus Vibrator::composePwleV2(const CompositePwleV2& composite,
                                           const std::shared_ptr<IVibratorCallback>& callback) {
    LOG(VERBOSE) << "Vibrator compose PWLE V2";
    int32_t capabilities = 0;
    if (!getCapabilities(&capabilities).isOk()) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    if (!(capabilities & IVibrator::CAP_COMPOSE_PWLE_EFFECTS_V2) ||
        !(capabilities & IVibrator::CAP_FREQUENCY_CONTROL)) {
        return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
    }

    int compositionSizeMax;
    getPwleV2CompositionSizeMax(&compositionSizeMax);
    if (composite.pwlePrimitives.empty() || composite.pwlePrimitives.size() > compositionSizeMax) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }

    int32_t totalEffectDuration = 0;
    std::vector<FrequencyAccelerationMapEntry> frequencyToOutputAccelerationMap;
    getFrequencyToOutputAccelerationMap(&frequencyToOutputAccelerationMap);
    float minFrequency = getPwleV2FrequencyMinHz(frequencyToOutputAccelerationMap);
    float maxFrequency = getPwleV2FrequencyMaxHz(frequencyToOutputAccelerationMap);

    for (auto& e : composite.pwlePrimitives) {
        if (e.timeMillis < 0.0f || e.timeMillis > COMPOSE_PWLE_V2_PRIMITIVE_DURATION_MAX_MS) {
            return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
        }
        if (e.amplitude < 0.0f || e.amplitude > 1.0f) {
            return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
        }
        if (e.frequencyHz < minFrequency || e.frequencyHz > maxFrequency) {
            return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
        }
        totalEffectDuration += e.timeMillis;
    }

    dispatchVibrate(totalEffectDuration, callback);
    return ndk::ScopedAStatus::ok();
}

}  // namespace vibrator
}  // namespace hardware
}  // namespace android
}  // namespace aidl
