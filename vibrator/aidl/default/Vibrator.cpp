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

static constexpr int32_t kComposeDelayMaxMs = 1000;
static constexpr int32_t kComposeSizeMax = 256;
static constexpr int32_t kComposePwleSizeMax = 127;

static constexpr float kResonantFrequency = 150.0;
static constexpr float kQFactor = 11.0;
static constexpr int32_t COMPOSE_PWLE_PRIMITIVE_DURATION_MAX_MS = 16383;
static constexpr float PWLE_LEVEL_MIN = 0.0;
static constexpr float PWLE_LEVEL_MAX = 0.98256;
static constexpr float PWLE_FREQUENCY_RESOLUTION_HZ = 1.0;
static constexpr float PWLE_FREQUENCY_MIN_HZ = 140.0;
static constexpr float PWLE_FREQUENCY_MAX_HZ = 160.0;

ndk::ScopedAStatus Vibrator::getCapabilities(int32_t* _aidl_return) {
    LOG(INFO) << "Vibrator reporting capabilities";
    *_aidl_return = IVibrator::CAP_ON_CALLBACK | IVibrator::CAP_PERFORM_CALLBACK |
                    IVibrator::CAP_AMPLITUDE_CONTROL | IVibrator::CAP_EXTERNAL_CONTROL |
                    IVibrator::CAP_EXTERNAL_AMPLITUDE_CONTROL | IVibrator::CAP_COMPOSE_EFFECTS |
                    IVibrator::CAP_ALWAYS_ON_CONTROL | IVibrator::CAP_GET_RESONANT_FREQUENCY |
                    IVibrator::CAP_GET_Q_FACTOR | IVibrator::CAP_FREQUENCY_CONTROL |
                    IVibrator::CAP_COMPOSE_PWLE_EFFECTS;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::off() {
    LOG(INFO) << "Vibrator off";
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::on(int32_t timeoutMs,
                                const std::shared_ptr<IVibratorCallback>& callback) {
    LOG(INFO) << "Vibrator on for timeoutMs: " << timeoutMs;
    if (callback != nullptr) {
        std::thread([=] {
            LOG(INFO) << "Starting on on another thread";
            usleep(timeoutMs * 1000);
            LOG(INFO) << "Notifying on complete";
            if (!callback->onComplete().isOk()) {
                LOG(ERROR) << "Failed to call onComplete";
            }
        }).detach();
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::perform(Effect effect, EffectStrength strength,
                                     const std::shared_ptr<IVibratorCallback>& callback,
                                     int32_t* _aidl_return) {
    LOG(INFO) << "Vibrator perform";

    if (effect != Effect::CLICK && effect != Effect::TICK) {
        return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
    }
    if (strength != EffectStrength::LIGHT && strength != EffectStrength::MEDIUM &&
        strength != EffectStrength::STRONG) {
        return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
    }

    constexpr size_t kEffectMillis = 100;

    if (callback != nullptr) {
        std::thread([=] {
            LOG(INFO) << "Starting perform on another thread";
            usleep(kEffectMillis * 1000);
            LOG(INFO) << "Notifying perform complete";
            callback->onComplete();
        }).detach();
    }

    *_aidl_return = kEffectMillis;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::getSupportedEffects(std::vector<Effect>* _aidl_return) {
    *_aidl_return = {Effect::CLICK, Effect::TICK};
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::setAmplitude(float amplitude) {
    LOG(INFO) << "Vibrator set amplitude: " << amplitude;
    if (amplitude <= 0.0f || amplitude > 1.0f) {
        return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_ILLEGAL_ARGUMENT));
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::setExternalControl(bool enabled) {
    LOG(INFO) << "Vibrator set external control: " << enabled;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::getCompositionDelayMax(int32_t* maxDelayMs) {
    *maxDelayMs = kComposeDelayMaxMs;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::getCompositionSizeMax(int32_t* maxSize) {
    *maxSize = kComposeSizeMax;
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
    if (composite.size() > kComposeSizeMax) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }

    std::vector<CompositePrimitive> supported;
    getSupportedPrimitives(&supported);

    for (auto& e : composite) {
        if (e.delayMs > kComposeDelayMaxMs) {
            return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
        }
        if (e.scale < 0.0f || e.scale > 1.0f) {
            return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
        }
        if (std::find(supported.begin(), supported.end(), e.primitive) == supported.end()) {
            return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
        }
    }

    std::thread([=] {
        LOG(INFO) << "Starting compose on another thread";

        for (auto& e : composite) {
            if (e.delayMs) {
                usleep(e.delayMs * 1000);
            }
            LOG(INFO) << "triggering primitive " << static_cast<int>(e.primitive) << " @ scale "
                      << e.scale;

            int32_t durationMs;
            getPrimitiveDuration(e.primitive, &durationMs);
            usleep(durationMs * 1000);
        }

        if (callback != nullptr) {
            LOG(INFO) << "Notifying perform complete";
            callback->onComplete();
        }
    }).detach();

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
        LOG(INFO) << "Enabling always-on ID " << id << " with " << toString(effect) << "/"
                  << toString(strength);
        return ndk::ScopedAStatus::ok();
    }
}

ndk::ScopedAStatus Vibrator::alwaysOnDisable(int32_t id) {
    LOG(INFO) << "Disabling always-on ID " << id;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::getResonantFrequency(float *resonantFreqHz) {
    *resonantFreqHz = kResonantFrequency;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::getQFactor(float *qFactor) {
    *qFactor = kQFactor;
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
    // A valid array should be of size:
    //     (PWLE_FREQUENCY_MAX_HZ - PWLE_FREQUENCY_MIN_HZ) / PWLE_FREQUENCY_RESOLUTION_HZ
    *_aidl_return = {0.01, 0.02, 0.03, 0.04, 0.05, 0.06, 0.07, 0.08, 0.09, 0.10,
                     0.11, 0.12, 0.13, 0.14, 0.15, 0.16, 0.17, 0.18, 0.19, 0.20};
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::getPwlePrimitiveDurationMax(int32_t *durationMs) {
    *durationMs = COMPOSE_PWLE_PRIMITIVE_DURATION_MAX_MS;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::getPwleCompositionSizeMax(int32_t *maxSize) {
    *maxSize = kComposePwleSizeMax;
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

    std::thread([=] {
        LOG(INFO) << "Starting composePwle on another thread";
        usleep(totalDuration * 1000);
        if (callback != nullptr) {
            LOG(INFO) << "Notifying compose PWLE complete";
            callback->onComplete();
        }
    }).detach();

    return ndk::ScopedAStatus::ok();
}

}  // namespace vibrator
}  // namespace hardware
}  // namespace android
}  // namespace aidl
