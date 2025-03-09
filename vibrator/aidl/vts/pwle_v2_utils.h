/*
 * Copyright (C) 2024 The Android Open Source Project
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

#ifndef VIBRATOR_HAL_PWLE_V2_UTILS_H
#define VIBRATOR_HAL_PWLE_V2_UTILS_H

#include <aidl/android/hardware/vibrator/IVibrator.h>
#include "test_utils.h"

using aidl::android::hardware::vibrator::FrequencyAccelerationMapEntry;
using aidl::android::hardware::vibrator::IVibrator;
using aidl::android::hardware::vibrator::PwleV2Primitive;

namespace aidl {
namespace android {
namespace hardware {
namespace vibrator {
namespace testing {
namespace pwlev2 {

static constexpr int32_t COMPOSE_PWLE_V2_MIN_REQUIRED_SIZE = 16;
static constexpr int32_t COMPOSE_PWLE_V2_MIN_REQUIRED_PRIMITIVE_MAX_DURATION_MS = 1000;
static constexpr int32_t COMPOSE_PWLE_V2_MAX_ALLOWED_PRIMITIVE_MIN_DURATION_MS = 20;
static constexpr int32_t COMPOSE_PWLE_V2_MIN_REQUIRED_SENSITIVITY_DB_SL = 10;

namespace {
/**
 * Returns a vector of (frequency in Hz, acceleration in dB) pairs, where the acceleration
 * value denotes the minimum output required at the corresponding frequency to be perceptible
 * by a human.
 */
static std::vector<std::pair<float, float>> getMinPerceptibleLevel() {
    return {{0.4f, -97.81f},   {2.0f, -69.86f},   {3.0f, -62.81f},    {4.0f, -58.81f},
            {5.0f, -56.69f},   {6.0f, -54.77f},   {7.2f, -52.85f},    {8.0f, -51.77f},
            {8.64f, -50.84f},  {10.0f, -48.90f},  {10.37f, -48.52f},  {12.44f, -46.50f},
            {14.93f, -44.43f}, {15.0f, -44.35f},  {17.92f, -41.96f},  {20.0f, -40.36f},
            {21.5f, -39.60f},  {25.0f, -37.48f},  {25.8f, -36.93f},   {30.0f, -34.31f},
            {35.0f, -33.13f},  {40.0f, -32.81f},  {50.0f, -31.94f},   {60.0f, -31.77f},
            {70.0f, -31.59f},  {72.0f, -31.55f},  {80.0f, -31.77f},   {86.4f, -31.94f},
            {90.0f, -31.73f},  {100.0f, -31.90f}, {103.68f, -31.77f}, {124.42f, -31.70f},
            {149.3f, -31.38f}, {150.0f, -31.35f}, {179.16f, -31.02f}, {200.0f, -30.86f},
            {215.0f, -30.35f}, {250.0f, -28.98f}, {258.0f, -28.68f},  {300.0f, -26.81f},
            {400.0f, -19.81f}};
}

static float interpolateLinearly(const std::vector<float>& xAxis, const std::vector<float>& yAxis,
                                 float x) {
    EXPECT_TRUE(!xAxis.empty());
    EXPECT_TRUE(xAxis.size() == yAxis.size());

    if (x <= xAxis.front()) return yAxis.front();
    if (x >= xAxis.back()) return yAxis.back();

    auto it = std::upper_bound(xAxis.begin(), xAxis.end(), x);
    int i = std::distance(xAxis.begin(), it) - 1;  // Index of the lower bound

    const float& x0 = xAxis[i];
    const float& y0 = yAxis[i];
    const float& x1 = xAxis[i + 1];
    const float& y1 = yAxis[i + 1];

    return y0 + (x - x0) * (y1 - y0) / (x1 - x0);
}

static float minPerceptibleDbCurve(float frequency) {
    // Initialize minPerceptibleMap only once
    static auto minPerceptibleMap = []() -> std::function<float(float)> {
        static std::vector<float> minPerceptibleFrequencies;
        static std::vector<float> minPerceptibleAccelerations;

        auto minPerceptibleLevel = getMinPerceptibleLevel();
        // Sort the 'minPerceptibleLevel' data in ascending order based on the
        // frequency values (first element of each pair).
        std::sort(minPerceptibleLevel.begin(), minPerceptibleLevel.end(),
                  [](const auto& a, const auto& b) { return a.first < b.first; });

        for (const auto& entry : minPerceptibleLevel) {
            minPerceptibleFrequencies.push_back(entry.first);
            minPerceptibleAccelerations.push_back(entry.second);
        }

        return [&](float freq) {
            return interpolateLinearly(minPerceptibleFrequencies, minPerceptibleAccelerations,
                                       freq);
        };
    }();

    return minPerceptibleMap(frequency);
}

static float convertSensitivityLevelToDecibel(int sl, float frequency) {
    return sl + minPerceptibleDbCurve(frequency);
}

static float convertDecibelToAcceleration(float db) {
    return std::pow(10.0f, db / 20.0f);
}
}  // namespace

static float convertSensitivityLevelToAcceleration(int sl, float frequency) {
    return pwlev2::convertDecibelToAcceleration(
            pwlev2::convertSensitivityLevelToDecibel(sl, frequency));
}

static float getPwleV2FrequencyMinHz(const std::shared_ptr<IVibrator>& vibrator) {
    std::vector<FrequencyAccelerationMapEntry> frequencyToOutputAccelerationMap;
    EXPECT_OK(vibrator->getFrequencyToOutputAccelerationMap(&frequencyToOutputAccelerationMap));
    EXPECT_TRUE(!frequencyToOutputAccelerationMap.empty());
    // We can't use ASSERT_TRUE() above because this is a non-void function,
    // but we need to return to assure we don't crash from a null dereference.
    if (frequencyToOutputAccelerationMap.empty()) {
        return std::numeric_limits<float>::quiet_NaN();
    }

    auto entry = std::min_element(
            frequencyToOutputAccelerationMap.begin(), frequencyToOutputAccelerationMap.end(),
            [](const auto& a, const auto& b) { return a.frequencyHz < b.frequencyHz; });

    return entry->frequencyHz;
}

static float getPwleV2FrequencyMaxHz(const std::shared_ptr<IVibrator>& vibrator) {
    std::vector<FrequencyAccelerationMapEntry> frequencyToOutputAccelerationMap;
    EXPECT_OK(vibrator->getFrequencyToOutputAccelerationMap(&frequencyToOutputAccelerationMap));
    EXPECT_TRUE(!frequencyToOutputAccelerationMap.empty());
    // We can't use ASSERT_TRUE() above because this is a non-void function,
    // but we need to return to assure we don't crash from a null dereference.
    if (frequencyToOutputAccelerationMap.empty()) {
        return std::numeric_limits<float>::quiet_NaN();
    }

    auto entry = std::max_element(
            frequencyToOutputAccelerationMap.begin(), frequencyToOutputAccelerationMap.end(),
            [](const auto& a, const auto& b) { return a.frequencyHz < b.frequencyHz; });

    return entry->frequencyHz;
}

static CompositePwleV2 composeValidPwleV2Effect(const std::shared_ptr<IVibrator>& vibrator) {
    int32_t minDurationMs;
    EXPECT_OK(vibrator->getPwleV2PrimitiveDurationMinMillis(&minDurationMs));
    int32_t maxDurationMs;
    EXPECT_OK(vibrator->getPwleV2PrimitiveDurationMaxMillis(&maxDurationMs));
    float minFrequency = getPwleV2FrequencyMinHz(vibrator);
    float maxFrequency = getPwleV2FrequencyMaxHz(vibrator);
    int32_t maxCompositionSize;
    EXPECT_OK(vibrator->getPwleV2CompositionSizeMax(&maxCompositionSize));

    CompositePwleV2 composite;

    composite.pwlePrimitives.emplace_back(0.1f, minFrequency, minDurationMs);
    composite.pwlePrimitives.emplace_back(0.5f, maxFrequency, maxDurationMs);

    float variedFrequency = (minFrequency + maxFrequency) / 2.0f;
    for (int i = 0; i < maxCompositionSize - 2; i++) {
        composite.pwlePrimitives.emplace_back(0.7f, variedFrequency, minDurationMs);
    }

    return composite;
}

static CompositePwleV2 composePwleV2EffectWithTooManyPoints(
        const std::shared_ptr<IVibrator>& vibrator) {
    int32_t minDurationMs, maxCompositionSize;
    EXPECT_OK(vibrator->getPwleV2PrimitiveDurationMinMillis(&minDurationMs));
    EXPECT_OK(vibrator->getPwleV2CompositionSizeMax(&maxCompositionSize));
    float maxFrequency = getPwleV2FrequencyMaxHz(vibrator);

    std::vector<PwleV2Primitive> pwleEffect(maxCompositionSize + 1);  // +1 to exceed the limit

    std::fill(pwleEffect.begin(), pwleEffect.end(),
              PwleV2Primitive(/*amplitude=*/0.2f, maxFrequency, minDurationMs));

    CompositePwleV2 composite;
    composite.pwlePrimitives = pwleEffect;

    return composite;
}

static std::pair<float, float> getPwleV2SharpnessRange(
        const std::shared_ptr<IVibrator>& vibrator,
        std::vector<FrequencyAccelerationMapEntry> freqToOutputAccelerationMap) {
    std::pair<float, float> sharpnessRange = {-1, -1};

    // Sort the entries by frequency in ascending order
    std::sort(freqToOutputAccelerationMap.begin(), freqToOutputAccelerationMap.end(),
              [](const auto& a, const auto& b) { return a.frequencyHz < b.frequencyHz; });

    for (const auto& entry : freqToOutputAccelerationMap) {
        float minAcceptableOutputAcceleration = convertSensitivityLevelToAcceleration(
                pwlev2::COMPOSE_PWLE_V2_MIN_REQUIRED_SENSITIVITY_DB_SL, entry.frequencyHz);

        if (sharpnessRange.first < 0 &&
            minAcceptableOutputAcceleration <= entry.maxOutputAccelerationGs) {
            sharpnessRange.first = entry.frequencyHz;  // Found the lower bound
        } else if (sharpnessRange.first >= 0 &&
                   minAcceptableOutputAcceleration >= entry.maxOutputAccelerationGs) {
            sharpnessRange.second = entry.frequencyHz;  // Found the upper bound
            return sharpnessRange;
        }
    }

    if (sharpnessRange.first >= 0) {
        // If only the lower bound was found, set the upper bound to the max frequency.
        sharpnessRange.second = getPwleV2FrequencyMaxHz(vibrator);
    }

    return sharpnessRange;
}
}  // namespace pwlev2
}  // namespace testing
}  // namespace vibrator
}  // namespace hardware
}  // namespace android
}  // namespace aidl
#endif  // VIBRATOR_HAL_PWLE_V2_UTILS_H
