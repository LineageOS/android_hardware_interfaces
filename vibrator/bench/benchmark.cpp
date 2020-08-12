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

#include "benchmark/benchmark.h"

#include <android/hardware/vibrator/1.3/IVibrator.h>
#include <android/hardware/vibrator/BnVibratorCallback.h>
#include <android/hardware/vibrator/IVibrator.h>
#include <binder/IServiceManager.h>

using ::android::enum_range;
using ::android::sp;
using ::android::hardware::hidl_enum_range;
using ::android::hardware::Return;
using ::android::hardware::details::hidl_enum_values;
using ::benchmark::Counter;
using ::benchmark::Fixture;
using ::benchmark::kMicrosecond;
using ::benchmark::State;
using ::benchmark::internal::Benchmark;
using ::std::chrono::duration;
using ::std::chrono::duration_cast;
using ::std::chrono::high_resolution_clock;

namespace Aidl = ::android::hardware::vibrator;
namespace V1_0 = ::android::hardware::vibrator::V1_0;
namespace V1_1 = ::android::hardware::vibrator::V1_1;
namespace V1_2 = ::android::hardware::vibrator::V1_2;
namespace V1_3 = ::android::hardware::vibrator::V1_3;

template <typename I>
class BaseBench : public Fixture {
  public:
    void TearDown(State& /*state*/) override {
        if (!mVibrator) {
            return;
        }
        mVibrator->off();
    }

    static void DefaultConfig(Benchmark* b) { b->Unit(kMicrosecond); }

    static void DefaultArgs(Benchmark* /*b*/) { /* none */
    }

  protected:
    auto getOtherArg(const State& state, std::size_t index) const { return state.range(index + 0); }

  protected:
    sp<I> mVibrator;
};

template <typename I>
class VibratorBench : public BaseBench<I> {
  public:
    void SetUp(State& /*state*/) override { this->mVibrator = I::getService(); }
};

enum class EmptyEnum : uint32_t;
template <>
inline constexpr std::array<EmptyEnum, 0> hidl_enum_values<EmptyEnum> = {};

template <typename T, typename U>
std::set<T> difference(const hidl_enum_range<T>& t, const hidl_enum_range<U>& u) {
    class Compare {
      public:
        bool operator()(const T& a, const U& b) { return a < static_cast<T>(b); }
        bool operator()(const U& a, const T& b) { return static_cast<T>(a) < b; }
    };
    std::set<T> ret;

    std::set_difference(t.begin(), t.end(), u.begin(), u.end(),
                        std::insert_iterator<decltype(ret)>(ret, ret.begin()), Compare());

    return ret;
}

template <typename I, typename E1, typename E2 = EmptyEnum>
class VibratorEffectsBench : public VibratorBench<I> {
  public:
    using Effect = E1;
    using EffectStrength = V1_0::EffectStrength;
    using Status = V1_0::Status;

  public:
    static void DefaultArgs(Benchmark* b) {
        b->ArgNames({"Effect", "Strength"});
        for (const auto& effect : difference(hidl_enum_range<E1>(), hidl_enum_range<E2>())) {
            for (const auto& strength : hidl_enum_range<EffectStrength>()) {
                b->Args({static_cast<long>(effect), static_cast<long>(strength)});
            }
        }
    }

    void performBench(State* state, Return<void> (I::*performApi)(Effect, EffectStrength,
                                                                  typename I::perform_cb)) {
        auto effect = getEffect(*state);
        auto strength = getStrength(*state);
        bool supported = true;

        (*this->mVibrator.*performApi)(effect, strength, [&](Status status, uint32_t /*lengthMs*/) {
            if (status == Status::UNSUPPORTED_OPERATION) {
                supported = false;
            }
        });

        if (!supported) {
            return;
        }

        for (auto _ : *state) {
            state->ResumeTiming();
            (*this->mVibrator.*performApi)(effect, strength,
                                           [](Status /*status*/, uint32_t /*lengthMs*/) {});
            state->PauseTiming();
            this->mVibrator->off();
        }
    }

  protected:
    auto getEffect(const State& state) const {
        return static_cast<Effect>(this->getOtherArg(state, 0));
    }

    auto getStrength(const State& state) const {
        return static_cast<EffectStrength>(this->getOtherArg(state, 1));
    }
};

#define BENCHMARK_WRAPPER(fixt, test, code) \
    BENCHMARK_DEFINE_F(fixt, test)          \
    /* NOLINTNEXTLINE */                    \
    (State & state) {                       \
        if (!mVibrator) {                   \
            return;                         \
        }                                   \
                                            \
        code                                \
    }                                       \
    BENCHMARK_REGISTER_F(fixt, test)->Apply(fixt::DefaultConfig)->Apply(fixt::DefaultArgs)

using VibratorBench_V1_0 = VibratorBench<V1_0::IVibrator>;

BENCHMARK_WRAPPER(VibratorBench_V1_0, on, {
    uint32_t ms = UINT32_MAX;

    for (auto _ : state) {
        state.ResumeTiming();
        mVibrator->on(ms);
        state.PauseTiming();
        mVibrator->off();
    }
});

BENCHMARK_WRAPPER(VibratorBench_V1_0, off, {
    uint32_t ms = UINT32_MAX;

    for (auto _ : state) {
        state.PauseTiming();
        mVibrator->on(ms);
        state.ResumeTiming();
        mVibrator->off();
    }
});

BENCHMARK_WRAPPER(VibratorBench_V1_0, supportsAmplitudeControl, {
    for (auto _ : state) {
        mVibrator->supportsAmplitudeControl();
    }
});

BENCHMARK_WRAPPER(VibratorBench_V1_0, setAmplitude, {
    uint8_t amplitude = UINT8_MAX;

    if (!mVibrator->supportsAmplitudeControl()) {
        return;
    }

    mVibrator->on(UINT32_MAX);

    for (auto _ : state) {
        mVibrator->setAmplitude(amplitude);
    }

    mVibrator->off();
});

using VibratorEffectsBench_V1_0 = VibratorEffectsBench<V1_0::IVibrator, V1_0::Effect>;

BENCHMARK_WRAPPER(VibratorEffectsBench_V1_0, perform,
                  { performBench(&state, &V1_0::IVibrator::perform); });

using VibratorEffectsBench_V1_1 =
        VibratorEffectsBench<V1_1::IVibrator, V1_1::Effect_1_1, V1_0::Effect>;

BENCHMARK_WRAPPER(VibratorEffectsBench_V1_1, perform_1_1,
                  { performBench(&state, &V1_1::IVibrator::perform_1_1); });

using VibratorEffectsBench_V1_2 =
        VibratorEffectsBench<V1_2::IVibrator, V1_2::Effect, V1_1::Effect_1_1>;

BENCHMARK_WRAPPER(VibratorEffectsBench_V1_2, perform_1_2,
                  { performBench(&state, &V1_2::IVibrator::perform_1_2); });

using VibratorBench_V1_3 = VibratorBench<V1_3::IVibrator>;

BENCHMARK_WRAPPER(VibratorBench_V1_3, supportsExternalControl, {
    for (auto _ : state) {
        mVibrator->supportsExternalControl();
    }
});

BENCHMARK_WRAPPER(VibratorBench_V1_3, setExternalControl, {
    bool enable = true;

    if (!mVibrator->supportsExternalControl()) {
        return;
    }

    for (auto _ : state) {
        state.ResumeTiming();
        mVibrator->setExternalControl(enable);
        state.PauseTiming();
        mVibrator->setExternalControl(false);
    }
});

BENCHMARK_WRAPPER(VibratorBench_V1_3, supportsExternalAmplitudeControl, {
    if (!mVibrator->supportsExternalControl()) {
        return;
    }

    mVibrator->setExternalControl(true);

    for (auto _ : state) {
        mVibrator->supportsAmplitudeControl();
    }

    mVibrator->setExternalControl(false);
});

BENCHMARK_WRAPPER(VibratorBench_V1_3, setExternalAmplitude, {
    uint8_t amplitude = UINT8_MAX;

    if (!mVibrator->supportsExternalControl()) {
        return;
    }

    mVibrator->setExternalControl(true);

    if (!mVibrator->supportsAmplitudeControl()) {
        return;
    }

    for (auto _ : state) {
        mVibrator->setAmplitude(amplitude);
    }

    mVibrator->setExternalControl(false);
});

using VibratorEffectsBench_V1_3 = VibratorEffectsBench<V1_3::IVibrator, V1_3::Effect, V1_2::Effect>;

BENCHMARK_WRAPPER(VibratorEffectsBench_V1_3, perform_1_3,
                  { performBench(&state, &V1_3::IVibrator::perform_1_3); });

class VibratorBench_Aidl : public BaseBench<Aidl::IVibrator> {
  public:
    void SetUp(State& /*state*/) override {
        this->mVibrator = android::waitForVintfService<Aidl::IVibrator>();
    }
};

class HalCallback : public Aidl::BnVibratorCallback {
  public:
    HalCallback() = default;
    ~HalCallback() = default;

    android::binder::Status onComplete() override { return android::binder::Status::ok(); }
};

BENCHMARK_WRAPPER(VibratorBench_Aidl, on, {
    int32_t capabilities = 0;
    mVibrator->getCapabilities(&capabilities);

    int32_t ms = INT32_MAX;
    auto cb = (capabilities & Aidl::IVibrator::CAP_ON_CALLBACK) ? new HalCallback() : nullptr;

    for (auto _ : state) {
        state.ResumeTiming();
        mVibrator->on(ms, cb);
        state.PauseTiming();
        mVibrator->off();
    }
});

BENCHMARK_WRAPPER(VibratorBench_Aidl, off, {
    for (auto _ : state) {
        state.PauseTiming();
        mVibrator->on(INT32_MAX, nullptr);
        state.ResumeTiming();
        mVibrator->off();
    }
});

BENCHMARK_WRAPPER(VibratorBench_Aidl, getCapabilities, {
    int32_t capabilities = 0;

    for (auto _ : state) {
        mVibrator->getCapabilities(&capabilities);
    }
});

BENCHMARK_WRAPPER(VibratorBench_Aidl, setAmplitude, {
    int32_t capabilities = 0;
    mVibrator->getCapabilities(&capabilities);
    if ((capabilities & Aidl::IVibrator::CAP_AMPLITUDE_CONTROL) == 0) {
        return;
    }

    float amplitude = 1.0f;
    mVibrator->on(INT32_MAX, nullptr);

    for (auto _ : state) {
        mVibrator->setAmplitude(amplitude);
    }

    mVibrator->off();
});

BENCHMARK_WRAPPER(VibratorBench_Aidl, setExternalControl, {
    int32_t capabilities = 0;
    mVibrator->getCapabilities(&capabilities);
    if ((capabilities & Aidl::IVibrator::CAP_EXTERNAL_CONTROL) == 0) {
        return;
    }

    for (auto _ : state) {
        state.ResumeTiming();
        mVibrator->setExternalControl(true);
        state.PauseTiming();
        mVibrator->setExternalControl(false);
    }
});

BENCHMARK_WRAPPER(VibratorBench_Aidl, setExternalAmplitude, {
    int32_t capabilities = 0;
    mVibrator->getCapabilities(&capabilities);
    if ((capabilities & Aidl::IVibrator::CAP_EXTERNAL_CONTROL) == 0 ||
        (capabilities & Aidl::IVibrator::CAP_EXTERNAL_AMPLITUDE_CONTROL) == 0) {
        return;
    }

    float amplitude = 1.0f;
    mVibrator->setExternalControl(true);

    for (auto _ : state) {
        mVibrator->setAmplitude(amplitude);
    }

    mVibrator->setExternalControl(false);
});

BENCHMARK_WRAPPER(VibratorBench_Aidl, getSupportedEffects, {
    std::vector<Aidl::Effect> supportedEffects;

    for (auto _ : state) {
        mVibrator->getSupportedEffects(&supportedEffects);
    }
});

BENCHMARK_WRAPPER(VibratorBench_Aidl, getSupportedAlwaysOnEffects, {
    std::vector<Aidl::Effect> supportedEffects;

    for (auto _ : state) {
        mVibrator->getSupportedAlwaysOnEffects(&supportedEffects);
    }
});

BENCHMARK_WRAPPER(VibratorBench_Aidl, getSupportedPrimitives, {
    std::vector<Aidl::CompositePrimitive> supportedPrimitives;

    for (auto _ : state) {
        mVibrator->getSupportedPrimitives(&supportedPrimitives);
    }
});

class VibratorEffectsBench_Aidl : public VibratorBench_Aidl {
  public:
    static void DefaultArgs(Benchmark* b) {
        b->ArgNames({"Effect", "Strength"});
        for (const auto& effect : enum_range<Aidl::Effect>()) {
            for (const auto& strength : enum_range<Aidl::EffectStrength>()) {
                b->Args({static_cast<long>(effect), static_cast<long>(strength)});
            }
        }
    }

  protected:
    auto getEffect(const State& state) const {
        return static_cast<Aidl::Effect>(this->getOtherArg(state, 0));
    }

    auto getStrength(const State& state) const {
        return static_cast<Aidl::EffectStrength>(this->getOtherArg(state, 1));
    }
};

BENCHMARK_WRAPPER(VibratorEffectsBench_Aidl, alwaysOnEnable, {
    int32_t capabilities = 0;
    mVibrator->getCapabilities(&capabilities);
    if ((capabilities & Aidl::IVibrator::CAP_ALWAYS_ON_CONTROL) == 0) {
        return;
    }

    int32_t id = 1;
    auto effect = getEffect(state);
    auto strength = getStrength(state);

    std::vector<Aidl::Effect> supported;
    mVibrator->getSupportedAlwaysOnEffects(&supported);
    if (std::find(supported.begin(), supported.end(), effect) == supported.end()) {
        return;
    }

    for (auto _ : state) {
        state.ResumeTiming();
        mVibrator->alwaysOnEnable(id, effect, strength);
        state.PauseTiming();
        mVibrator->alwaysOnDisable(id);
    }
});

BENCHMARK_WRAPPER(VibratorEffectsBench_Aidl, alwaysOnDisable, {
    int32_t capabilities = 0;
    mVibrator->getCapabilities(&capabilities);
    if ((capabilities & Aidl::IVibrator::CAP_ALWAYS_ON_CONTROL) == 0) {
        return;
    }

    int32_t id = 1;
    auto effect = getEffect(state);
    auto strength = getStrength(state);

    std::vector<Aidl::Effect> supported;
    mVibrator->getSupportedAlwaysOnEffects(&supported);
    if (std::find(supported.begin(), supported.end(), effect) == supported.end()) {
        return;
    }

    for (auto _ : state) {
        state.PauseTiming();
        mVibrator->alwaysOnEnable(id, effect, strength);
        state.ResumeTiming();
        mVibrator->alwaysOnDisable(id);
    }
});

BENCHMARK_WRAPPER(VibratorEffectsBench_Aidl, perform, {
    int32_t capabilities = 0;
    mVibrator->getCapabilities(&capabilities);

    auto effect = getEffect(state);
    auto strength = getStrength(state);
    auto cb = (capabilities & Aidl::IVibrator::CAP_PERFORM_CALLBACK) ? new HalCallback() : nullptr;
    int32_t lengthMs = 0;

    std::vector<Aidl::Effect> supported;
    mVibrator->getSupportedEffects(&supported);
    if (std::find(supported.begin(), supported.end(), effect) == supported.end()) {
        return;
    }

    for (auto _ : state) {
        state.ResumeTiming();
        mVibrator->perform(effect, strength, cb, &lengthMs);
        state.PauseTiming();
        mVibrator->off();
    }
});

class VibratorPrimitivesBench_Aidl : public VibratorBench_Aidl {
  public:
    static void DefaultArgs(Benchmark* b) {
        b->ArgNames({"Primitive"});
        for (const auto& primitive : enum_range<Aidl::CompositePrimitive>()) {
            b->Args({static_cast<long>(primitive)});
        }
    }

  protected:
    auto getPrimitive(const State& state) const {
        return static_cast<Aidl::CompositePrimitive>(this->getOtherArg(state, 0));
    }
};

BENCHMARK_WRAPPER(VibratorBench_Aidl, getCompositionDelayMax, {
    int32_t ms = 0;

    for (auto _ : state) {
        mVibrator->getCompositionDelayMax(&ms);
    }
});

BENCHMARK_WRAPPER(VibratorBench_Aidl, getCompositionSizeMax, {
    int32_t size = 0;

    for (auto _ : state) {
        mVibrator->getCompositionSizeMax(&size);
    }
});

BENCHMARK_WRAPPER(VibratorPrimitivesBench_Aidl, getPrimitiveDuration, {
    int32_t capabilities = 0;
    mVibrator->getCapabilities(&capabilities);
    if ((capabilities & Aidl::IVibrator::CAP_COMPOSE_EFFECTS) == 0) {
        return;
    }

    auto primitive = getPrimitive(state);
    int32_t ms = 0;

    std::vector<Aidl::CompositePrimitive> supported;
    mVibrator->getSupportedPrimitives(&supported);
    if (std::find(supported.begin(), supported.end(), primitive) == supported.end()) {
        return;
    }

    for (auto _ : state) {
        mVibrator->getPrimitiveDuration(primitive, &ms);
    }
});

BENCHMARK_WRAPPER(VibratorPrimitivesBench_Aidl, compose, {
    int32_t capabilities = 0;
    mVibrator->getCapabilities(&capabilities);
    if ((capabilities & Aidl::IVibrator::CAP_COMPOSE_EFFECTS) == 0) {
        return;
    }

    Aidl::CompositeEffect effect;
    effect.primitive = getPrimitive(state);
    effect.scale = 1.0f;
    effect.delayMs = 0;

    std::vector<Aidl::CompositePrimitive> supported;
    mVibrator->getSupportedPrimitives(&supported);
    if (std::find(supported.begin(), supported.end(), effect.primitive) == supported.end()) {
        return;
    }

    auto cb = new HalCallback();
    std::vector<Aidl::CompositeEffect> effects;
    effects.push_back(effect);

    for (auto _ : state) {
        state.ResumeTiming();
        mVibrator->compose(effects, cb);
        state.PauseTiming();
        mVibrator->off();
    }
});

BENCHMARK_MAIN();
