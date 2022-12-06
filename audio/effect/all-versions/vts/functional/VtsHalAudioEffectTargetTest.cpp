/*
 * Copyright (C) 2018 The Android Open Source Project
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

#include <vector>

#define LOG_TAG "AudioEffectHidlHalTest"
#include <android-base/logging.h>
#if MAJOR_VERSION <= 6
#include <system/audio.h>
#endif

#include PATH(android/hardware/audio/effect/FILE_VERSION/IEffect.h)
#include PATH(android/hardware/audio/effect/FILE_VERSION/IEffectsFactory.h)
#include PATH(android/hardware/audio/effect/FILE_VERSION/IEqualizerEffect.h)
#include PATH(android/hardware/audio/effect/FILE_VERSION/ILoudnessEnhancerEffect.h)
#include PATH(android/hardware/audio/effect/FILE_VERSION/types.h)
#include <android/hidl/allocator/1.0/IAllocator.h>
#include <android/hidl/memory/1.0/IMemory.h>
#if MAJOR_VERSION >= 7
#include <android_audio_policy_configuration_V7_0-enums.h>
#endif

#include <common/all-versions/VersionUtils.h>

#include <cutils/properties.h>
#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>

using ::android::sp;
using ::android::hardware::hidl_handle;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::MQDescriptorSync;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::audio::common::utils::mkEnumBitfield;
using ::android::hidl::allocator::V1_0::IAllocator;
using ::android::hidl::memory::V1_0::IMemory;
using namespace ::android::hardware::audio::common::COMMON_TYPES_CPP_VERSION;
using namespace ::android::hardware::audio::effect::CPP_VERSION;
#if MAJOR_VERSION >= 7
// Make an alias for enumerations generated from the APM config XSD.
namespace xsd {
using namespace ::android::audio::policy::configuration::CPP_VERSION;
}
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*(a)))
#endif

class AudioEffectsFactoryHidlTest : public ::testing::TestWithParam<std::string> {
  public:
    void SetUp() override {
        effectsFactory = IEffectsFactory::getService(GetParam());
        ASSERT_NE(effectsFactory, nullptr);
    }
    void TearDown() override { effectsFactory.clear(); }

   protected:
    static void description(const std::string& description) {
        RecordProperty("description", description);
    }

    sp<IEffectsFactory> effectsFactory;
};

TEST_P(AudioEffectsFactoryHidlTest, EnumerateEffects) {
    description("Verify that EnumerateEffects returns at least one effect");
    Result retval = Result::NOT_INITIALIZED;
    size_t effectCount = 0;
    Return<void> ret =
        effectsFactory->getAllDescriptors([&](Result r, const hidl_vec<EffectDescriptor>& result) {
            retval = r;
            effectCount = result.size();
        });
    EXPECT_TRUE(ret.isOk());
    EXPECT_EQ(Result::OK, retval);
    EXPECT_GT(effectCount, 0u);
}

TEST_P(AudioEffectsFactoryHidlTest, CreateEffect) {
    description("Verify that an effect can be created via CreateEffect");
    bool gotEffect = false;
    Uuid effectUuid;
    Return<void> ret =
        effectsFactory->getAllDescriptors([&](Result r, const hidl_vec<EffectDescriptor>& result) {
            if (r == Result::OK && result.size() > 0) {
                gotEffect = true;
                effectUuid = result[0].uuid;
            }
        });
    ASSERT_TRUE(ret.isOk());
    ASSERT_TRUE(gotEffect);
    Result retval = Result::NOT_INITIALIZED;
    sp<IEffect> effect;
    ret = effectsFactory->createEffect(
            effectUuid, 1 /*session*/, 1 /*ioHandle*/,
#if MAJOR_VERSION >= 6
            0 /*device*/,
#endif
            [&](Result r, const sp<IEffect>& result, uint64_t /*effectId*/) {
                retval = r;
                if (r == Result::OK) {
                    effect = result;
                }
            });
    EXPECT_TRUE(ret.isOk());
    EXPECT_EQ(Result::OK, retval);
    EXPECT_NE(nullptr, effect.get());
}

TEST_P(AudioEffectsFactoryHidlTest, GetDescriptor) {
    description(
        "Verify that effects factory can provide an effect descriptor via "
        "GetDescriptor");
    hidl_vec<EffectDescriptor> allDescriptors;
    Return<void> ret =
        effectsFactory->getAllDescriptors([&](Result r, const hidl_vec<EffectDescriptor>& result) {
            if (r == Result::OK) {
                allDescriptors = result;
            }
        });
    ASSERT_TRUE(ret.isOk());
    ASSERT_GT(allDescriptors.size(), 0u);
    for (size_t i = 0; i < allDescriptors.size(); ++i) {
        ret = effectsFactory->getDescriptor(allDescriptors[i].uuid,
                                            [&](Result r, const EffectDescriptor& result) {
                                                EXPECT_EQ(r, Result::OK);
                                                EXPECT_EQ(result, allDescriptors[i]);
                                            });
    }
    EXPECT_TRUE(ret.isOk());
}

TEST_P(AudioEffectsFactoryHidlTest, DebugDumpInvalidArgument) {
    description("Verify that debugDump doesn't crash on invalid arguments");
#if MAJOR_VERSION == 2
    Return<void> ret = effectsFactory->debugDump(hidl_handle());
#elif MAJOR_VERSION >= 4
    Return<void> ret = effectsFactory->debug(hidl_handle(), {});
#endif
    ASSERT_TRUE(ret.isOk());
}

// Equalizer effect is required by CDD, but only the type is fixed.
// This is the same UUID as AudioEffect.EFFECT_TYPE_EQUALIZER in Java.
static const Uuid EQUALIZER_EFFECT_TYPE = {
    0x0bed4300, 0xddd6, 0x11db, 0x8f34,
    std::array<uint8_t, 6>{{0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b}}};
// Loudness Enhancer effect is required by CDD, but only the type is fixed.
// This is the same UUID as AudioEffect.EFFECT_TYPE_LOUDNESS_ENHANCER in Java.
static const Uuid LOUDNESS_ENHANCER_EFFECT_TYPE = {
    0xfe3199be, 0xaed0, 0x413f, 0x87bb,
    std::array<uint8_t, 6>{{0x11, 0x26, 0x0e, 0xb6, 0x3c, 0xf1}}};

enum { PARAM_FACTORY_NAME, PARAM_EFFECT_UUID };
using EffectParameter = std::tuple<std::string, Uuid>;

static inline std::string EffectParameterToString(
        const ::testing::TestParamInfo<EffectParameter>& info) {
    return ::android::hardware::PrintInstanceNameToString(::testing::TestParamInfo<std::string>{
            std::get<PARAM_FACTORY_NAME>(info.param), info.index});
}

// The main test class for Audio Effect HIDL HAL.
class AudioEffectHidlTest : public ::testing::TestWithParam<EffectParameter> {
  public:
    void SetUp() override {
        effectsFactory = IEffectsFactory::getService(std::get<PARAM_FACTORY_NAME>(GetParam()));
        ASSERT_NE(nullptr, effectsFactory.get());

        ASSERT_NO_FATAL_FAILURE(findAndCreateEffect(getEffectType()));
        ASSERT_NE(nullptr, effect.get());

        Return<Result> ret = effect->init();
        ASSERT_TRUE(ret.isOk());
        ASSERT_EQ(Result::OK, ret);
    }

    void TearDown() override {
        effect.clear();
        effectsFactory.clear();
    }

   protected:
    static void description(const std::string& description) {
        RecordProperty("description", description);
    }

    Uuid getEffectType() const { return std::get<PARAM_EFFECT_UUID>(GetParam()); }

    void findAndCreateEffect(const Uuid& type);
    void findEffectInstance(const Uuid& type, Uuid* uuid);
    void getChannelCount(uint32_t* channelCount);

    sp<IEffectsFactory> effectsFactory;
    sp<IEffect> effect;
};

void AudioEffectHidlTest::findAndCreateEffect(const Uuid& type) {
    Uuid effectUuid;
    ASSERT_NO_FATAL_FAILURE(findEffectInstance(type, &effectUuid));
    Return<void> ret = effectsFactory->createEffect(
            effectUuid, 1 /*session*/, 1 /*ioHandle*/,
#if MAJOR_VERSION >= 6
            0 /*device*/,
#endif
            [&](Result r, const sp<IEffect>& result, uint64_t /*effectId*/) {
                if (r == Result::OK) {
                    effect = result;
                }
            });
    ASSERT_TRUE(ret.isOk());
}

void AudioEffectHidlTest::findEffectInstance(const Uuid& type, Uuid* uuid) {
    bool effectFound = false;
    Return<void> ret =
        effectsFactory->getAllDescriptors([&](Result r, const hidl_vec<EffectDescriptor>& result) {
            if (r == Result::OK) {
                for (const auto& desc : result) {
                    if (desc.type == type) {
                        effectFound = true;
                        *uuid = desc.uuid;
                        break;
                    }
                }
            }
        });
    ASSERT_TRUE(ret.isOk());
    ASSERT_TRUE(effectFound);
}

void AudioEffectHidlTest::getChannelCount(uint32_t* channelCount) {
    Result retval;
    EffectConfig currentConfig;
    Return<void> ret = effect->getConfig([&](Result r, const EffectConfig& conf) {
        retval = r;
        if (r == Result::OK) {
            currentConfig = conf;
        }
    });
    ASSERT_TRUE(ret.isOk());
    ASSERT_EQ(Result::OK, retval);
#if MAJOR_VERSION <= 6
    ASSERT_TRUE(audio_channel_mask_is_valid(
        static_cast<audio_channel_mask_t>(currentConfig.outputCfg.channels)));
    *channelCount = audio_channel_count_from_out_mask(
        static_cast<audio_channel_mask_t>(currentConfig.outputCfg.channels));
#else
    ASSERT_EQ(AudioConfigBaseOptional::ChannelMask::hidl_discriminator::value,
              currentConfig.outputCfg.base.channelMask.getDiscriminator());
    *channelCount = android::audio::policy::configuration::V7_0::getChannelCount(
            currentConfig.outputCfg.base.channelMask.value());
    ASSERT_NE(*channelCount, 0);
#endif
}

TEST_P(AudioEffectHidlTest, Close) {
    description("Verify that an effect can be closed");
    Return<Result> ret = effect->close();
    EXPECT_TRUE(ret.isOk());
    EXPECT_EQ(Result::OK, ret);
}

TEST_P(AudioEffectHidlTest, GetDescriptor) {
    description("Verify that an effect can return its own descriptor via GetDescriptor");
    Result retval = Result::NOT_INITIALIZED;
    Uuid actualType;
    Return<void> ret = effect->getDescriptor([&](Result r, const EffectDescriptor& desc) {
        retval = r;
        if (r == Result::OK) {
            actualType = desc.type;
        }
    });
    EXPECT_TRUE(ret.isOk());
    EXPECT_EQ(Result::OK, retval);
    EXPECT_EQ(getEffectType(), actualType);
}

TEST_P(AudioEffectHidlTest, GetSetConfig) {
    description(
        "Verify that it is possible to manipulate effect config via Get / "
        "SetConfig");
    Result retval = Result::NOT_INITIALIZED;
    EffectConfig currentConfig;
    Return<void> ret = effect->getConfig([&](Result r, const EffectConfig& conf) {
        retval = r;
        if (r == Result::OK) {
            currentConfig = conf;
        }
    });
    EXPECT_TRUE(ret.isOk());
    EXPECT_EQ(Result::OK, retval);
    Return<Result> ret2 = effect->setConfig(currentConfig, nullptr, nullptr);
    EXPECT_TRUE(ret2.isOk());
    EXPECT_EQ(Result::OK, ret2);
}

#if MAJOR_VERSION >= 7
std::vector<EffectBufferConfig> generateInvalidConfigs(const EffectBufferConfig& src) {
    std::vector<EffectBufferConfig> result;
    EffectBufferConfig invalidFormat = src;
    invalidFormat.base.format.value("random_string");
    result.push_back(std::move(invalidFormat));
    EffectBufferConfig invalidChannelMask = src;
    invalidChannelMask.base.channelMask.value("random_string");
    result.push_back(std::move(invalidChannelMask));
    return result;
}

TEST_P(AudioEffectHidlTest, SetConfigInvalidArguments) {
    description("Verify that invalid arguments are rejected by SetConfig");
    Result retval = Result::NOT_INITIALIZED;
    EffectConfig currentConfig;
    Return<void> ret = effect->getConfig([&](Result r, const EffectConfig& conf) {
        retval = r;
        if (r == Result::OK) {
            currentConfig = conf;
        }
    });
    EXPECT_TRUE(ret.isOk());
    EXPECT_EQ(Result::OK, retval);
    for (const auto& invalidInputCfg : generateInvalidConfigs(currentConfig.inputCfg)) {
        EffectConfig invalidConfig = currentConfig;
        invalidConfig.inputCfg = invalidInputCfg;
        Return<Result> ret = effect->setConfig(invalidConfig, nullptr, nullptr);
        EXPECT_TRUE(ret.isOk());
        EXPECT_EQ(Result::INVALID_ARGUMENTS, ret);
    }
    for (const auto& invalidOutputCfg : generateInvalidConfigs(currentConfig.outputCfg)) {
        EffectConfig invalidConfig = currentConfig;
        invalidConfig.outputCfg = invalidOutputCfg;
        Return<Result> ret = effect->setConfig(invalidConfig, nullptr, nullptr);
        EXPECT_TRUE(ret.isOk());
        EXPECT_EQ(Result::INVALID_ARGUMENTS, ret);
    }
}
#endif

TEST_P(AudioEffectHidlTest, GetConfigReverse) {
    description("Verify that GetConfigReverse does not crash");
    Return<void> ret = effect->getConfigReverse([&](Result, const EffectConfig&) {});
    EXPECT_TRUE(ret.isOk());
}

TEST_P(AudioEffectHidlTest, GetSupportedAuxChannelsConfigs) {
    description("Verify that GetSupportedAuxChannelsConfigs does not crash");
    Return<void> ret = effect->getSupportedAuxChannelsConfigs(
        0, [&](Result, const hidl_vec<EffectAuxChannelsConfig>&) {});
    EXPECT_TRUE(ret.isOk());
}

TEST_P(AudioEffectHidlTest, GetAuxChannelsConfig) {
    description("Verify that GetAuxChannelsConfig does not crash");
    Return<void> ret = effect->getAuxChannelsConfig([&](Result, const EffectAuxChannelsConfig&) {});
    EXPECT_TRUE(ret.isOk());
}

TEST_P(AudioEffectHidlTest, SetAuxChannelsConfig) {
    description("Verify that SetAuxChannelsConfig does not crash");
    Return<Result> ret = effect->setAuxChannelsConfig(EffectAuxChannelsConfig());
    EXPECT_TRUE(ret.isOk());
}

// Not generated automatically because AudioBuffer contains
// instances of hidl_memory which can't be compared properly
// in general case due to presence of handles.
//
// However, in this particular case, handles must not present
// thus comparison is possible.
//
// operator== must be defined in the same namespace as the structures.
namespace android {
namespace hardware {
namespace audio {
namespace effect {
namespace CPP_VERSION {
inline bool operator==(const AudioBuffer& lhs, const AudioBuffer& rhs) {
    return lhs.id == rhs.id && lhs.frameCount == rhs.frameCount && lhs.data.handle() == nullptr &&
           rhs.data.handle() == nullptr;
}

#if MAJOR_VERSION <= 6
inline bool operator==(const EffectBufferConfig& lhs, const EffectBufferConfig& rhs) {
    return lhs.buffer == rhs.buffer &&
           lhs.samplingRateHz == rhs.samplingRateHz && lhs.channels == rhs.channels &&
           lhs.format == rhs.format &&
           lhs.accessMode == rhs.accessMode && lhs.mask == rhs.mask;
}
#else
inline bool operator==(const EffectBufferConfig& lhs, const EffectBufferConfig& rhs) {
    return lhs.buffer.getDiscriminator() == rhs.buffer.getDiscriminator() &&
           (lhs.buffer.getDiscriminator() ==
                    EffectBufferConfig::OptionalBuffer::hidl_discriminator::unspecified ||
            lhs.buffer.buf() == rhs.buffer.buf()) &&
           lhs.base == rhs.base && lhs.accessMode == rhs.accessMode;
}
#endif  // MAJOR_VERSION <= 6

inline bool operator==(const EffectConfig& lhs, const EffectConfig& rhs) {
    return lhs.inputCfg == rhs.inputCfg && lhs.outputCfg == rhs.outputCfg;
}
}  // namespace CPP_VERSION
}  // namespace effect
}  // namespace audio
}  // namespace hardware
}  // namespace android

TEST_P(AudioEffectHidlTest, Reset) {
    description("Verify that Reset preserves effect configuration");
    Result retval = Result::NOT_INITIALIZED;
    EffectConfig originalConfig;
    Return<void> ret = effect->getConfig([&](Result r, const EffectConfig& conf) {
        retval = r;
        if (r == Result::OK) {
            originalConfig = conf;
        }
    });
    ASSERT_TRUE(ret.isOk());
    ASSERT_EQ(Result::OK, retval);
    Return<Result> ret2 = effect->reset();
    EXPECT_TRUE(ret2.isOk());
    EXPECT_EQ(Result::OK, ret2);
    EffectConfig configAfterReset;
    ret = effect->getConfig([&](Result r, const EffectConfig& conf) {
        retval = r;
        if (r == Result::OK) {
            configAfterReset = conf;
        }
    });
    EXPECT_EQ(originalConfig, configAfterReset);
}

TEST_P(AudioEffectHidlTest, DisableEnableDisable) {
    description("Verify Disable -> Enable -> Disable sequence for an effect");
    Return<Result> ret = effect->disable();
    EXPECT_TRUE(ret.isOk());
    // Note: some legacy effects may return -EINVAL (INVALID_ARGUMENTS),
    //       more canonical is to return -ENOSYS (NOT_SUPPORTED)
    EXPECT_TRUE(ret == Result::NOT_SUPPORTED || ret == Result::INVALID_ARGUMENTS);
    ret = effect->enable();
    EXPECT_TRUE(ret.isOk());
    EXPECT_EQ(Result::OK, ret);
    ret = effect->disable();
    EXPECT_TRUE(ret.isOk());
    EXPECT_EQ(Result::OK, ret);
}

#if MAJOR_VERSION >= 7
TEST_P(AudioEffectHidlTest, SetDeviceInvalidDeviceAddress) {
    description("Verify that invalid device address is rejected by SetDevice");
    DeviceAddress device{.deviceType = "random_string"};
    Return<Result> ret = effect->setDevice(device);
    EXPECT_TRUE(ret.isOk());
    EXPECT_EQ(Result::INVALID_ARGUMENTS, ret);
}
#endif

TEST_P(AudioEffectHidlTest, SetDevice) {
    description("Verify that SetDevice works for an output chain effect");
#if MAJOR_VERSION <= 6
    Return<Result> ret = effect->setDevice(mkEnumBitfield(AudioDevice::OUT_SPEAKER));
#else
    DeviceAddress device{.deviceType = toString(xsd::AudioDevice::AUDIO_DEVICE_OUT_SPEAKER)};
    Return<Result> ret = effect->setDevice(device);
#endif
    EXPECT_TRUE(ret.isOk());
    EXPECT_EQ(Result::OK, ret);
}

TEST_P(AudioEffectHidlTest, SetAndGetVolume) {
    description("Verify that SetAndGetVolume method works for an effect");
    uint32_t channelCount;
    getChannelCount(&channelCount);
    hidl_vec<uint32_t> volumes;
    volumes.resize(channelCount);
    for (uint32_t i = 0; i < channelCount; ++i) {
        volumes[i] = 0;
    }
    Result retval = Result::NOT_INITIALIZED;
    Return<void> ret =
        effect->setAndGetVolume(volumes, [&](Result r, const hidl_vec<uint32_t>&) { retval = r; });
    EXPECT_TRUE(ret.isOk());
    EXPECT_EQ(Result::OK, retval);
}

TEST_P(AudioEffectHidlTest, VolumeChangeNotification) {
    description("Verify that effect accepts VolumeChangeNotification");
    uint32_t channelCount;
    getChannelCount(&channelCount);
    hidl_vec<uint32_t> volumes;
    volumes.resize(channelCount);
    for (uint32_t i = 0; i < channelCount; ++i) {
        volumes[i] = 0;
    }
    Return<Result> ret = effect->volumeChangeNotification(volumes);
    EXPECT_TRUE(ret.isOk());
    EXPECT_EQ(Result::OK, ret);
}

TEST_P(AudioEffectHidlTest, SetAudioMode) {
    description("Verify that SetAudioMode works for an effect");
    Return<Result> ret = effect->setAudioMode(AudioMode::NORMAL);
    EXPECT_TRUE(ret.isOk());
    EXPECT_EQ(Result::OK, ret);
}

TEST_P(AudioEffectHidlTest, SetConfigReverse) {
    description("Verify that SetConfigReverse does not crash");
    Return<Result> ret = effect->setConfigReverse(EffectConfig(), nullptr, nullptr);
    EXPECT_TRUE(ret.isOk());
}

#if MAJOR_VERSION >= 7
TEST_P(AudioEffectHidlTest, SetInputDeviceInvalidDeviceAddress) {
    description("Verify that invalid device address is rejected by SetInputDevice");
    DeviceAddress device{.deviceType = "random_string"};
    Return<Result> ret = effect->setInputDevice(device);
    EXPECT_TRUE(ret.isOk());
    EXPECT_TRUE(ret == Result::INVALID_ARGUMENTS || ret == Result::NOT_SUPPORTED)
            << ::testing::PrintToString(ret);
}
#endif

TEST_P(AudioEffectHidlTest, SetInputDevice) {
    description("Verify that SetInputDevice does not crash");
#if MAJOR_VERSION <= 6
    Return<Result> ret = effect->setInputDevice(mkEnumBitfield(AudioDevice::IN_BUILTIN_MIC));
#else
    DeviceAddress device{.deviceType = toString(xsd::AudioDevice::AUDIO_DEVICE_IN_BUILTIN_MIC)};
    Return<Result> ret = effect->setInputDevice(device);
#endif
    EXPECT_TRUE(ret.isOk());
}

#if MAJOR_VERSION >= 7
TEST_P(AudioEffectHidlTest, SetInvalidAudioSource) {
    description("Verify that an invalid audio source is rejected by SetAudioSource");
    Return<Result> ret = effect->setAudioSource("random_string");
    ASSERT_TRUE(ret.isOk());
    EXPECT_TRUE(ret == Result::INVALID_ARGUMENTS || ret == Result::NOT_SUPPORTED)
            << ::testing::PrintToString(ret);
}
#endif

TEST_P(AudioEffectHidlTest, SetAudioSource) {
    description("Verify that SetAudioSource does not crash");
#if MAJOR_VERSION <= 6
    Return<Result> ret = effect->setAudioSource(AudioSource::MIC);
#else
    Return<Result> ret = effect->setAudioSource(toString(xsd::AudioSource::AUDIO_SOURCE_MIC));
#endif
    EXPECT_TRUE(ret.isOk());
}

TEST_P(AudioEffectHidlTest, Offload) {
    description("Verify that calling Offload method does not crash");
    Return<Result> ret = effect->offload(EffectOffloadParameter{});
    EXPECT_TRUE(ret.isOk());
}

TEST_P(AudioEffectHidlTest, PrepareForProcessing) {
    description("Verify that PrepareForProcessing method works for an effect");
    Result retval = Result::NOT_INITIALIZED;
    Return<void> ret = effect->prepareForProcessing(
        [&](Result r, const MQDescriptorSync<Result>&) { retval = r; });
    EXPECT_TRUE(ret.isOk());
    EXPECT_EQ(Result::OK, retval);
}

TEST_P(AudioEffectHidlTest, SetProcessBuffers) {
    description("Verify that SetProcessBuffers works for an effect");
    sp<IAllocator> ashmem = IAllocator::getService("ashmem");
    ASSERT_NE(nullptr, ashmem.get());
    bool success = false;
    AudioBuffer buffer;
    Return<void> ret = ashmem->allocate(1024, [&](bool s, const hidl_memory& memory) {
        success = s;
        if (s) {
            buffer.data = memory;
        }
    });
    ASSERT_TRUE(ret.isOk());
    ASSERT_TRUE(success);
    Return<Result> ret2 = effect->setProcessBuffers(buffer, buffer);
    EXPECT_TRUE(ret2.isOk());
    EXPECT_EQ(Result::OK, ret2);
}

TEST_P(AudioEffectHidlTest, Command) {
    description("Verify that Command does not crash");
    Return<void> ret =
        effect->command(0, hidl_vec<uint8_t>(), 0, [&](int32_t, const hidl_vec<uint8_t>&) {});
    EXPECT_TRUE(ret.isOk());
}

TEST_P(AudioEffectHidlTest, SetParameter) {
    description("Verify that SetParameter does not crash");
    Return<Result> ret = effect->setParameter(hidl_vec<uint8_t>(), hidl_vec<uint8_t>());
    EXPECT_TRUE(ret.isOk());
}

TEST_P(AudioEffectHidlTest, GetParameter) {
    description("Verify that GetParameter does not crash");
    Return<void> ret =
        effect->getParameter(hidl_vec<uint8_t>(), 0, [&](Result, const hidl_vec<uint8_t>&) {});
    EXPECT_TRUE(ret.isOk());
}

TEST_P(AudioEffectHidlTest, GetParameterInvalidMaxReplySize) {
    description("Verify that GetParameter caps the maximum reply size");
    const bool isNewDeviceLaunchingOnTPlus = property_get_int32("ro.vendor.api_level", 0) >= 33;
    if (!isNewDeviceLaunchingOnTPlus) {
        GTEST_SKIP() << "The test only applies to devices launching on T or later";
    }
    // Use a non-empty parameter to avoid being rejected by any earlier checks.
    hidl_vec<uint8_t> parameter;
    parameter.resize(16);
    // Use very large size to ensure that the service does not crash. Since parameters
    // are specific to each effect, and some effects may not have parameters at all,
    // simply checking the return value would not reveal an issue of using an uncapped value.
    const uint32_t veryLargeReplySize = std::numeric_limits<uint32_t>::max() - 100;
    Result retval = Result::OK;
    Return<void> ret =
            effect->getParameter(parameter, veryLargeReplySize,
                                 [&](Result r, const hidl_vec<uint8_t>&) { retval = r; });
    EXPECT_TRUE(ret.isOk());
    EXPECT_EQ(Result::INVALID_ARGUMENTS, retval);
}

TEST_P(AudioEffectHidlTest, GetSupportedConfigsForFeature) {
    description("Verify that GetSupportedConfigsForFeature does not crash");
    Return<void> ret = effect->getSupportedConfigsForFeature(
        0, 0, 0, [&](Result, uint32_t, const hidl_vec<uint8_t>&) {});
    EXPECT_TRUE(ret.isOk());
}

TEST_P(AudioEffectHidlTest, GetCurrentConfigForFeature) {
    description("Verify that GetCurrentConfigForFeature does not crash");
    Return<void> ret =
        effect->getCurrentConfigForFeature(0, 0, [&](Result, const hidl_vec<uint8_t>&) {});
    EXPECT_TRUE(ret.isOk());
}

TEST_P(AudioEffectHidlTest, SetCurrentConfigForFeature) {
    description("Verify that SetCurrentConfigForFeature does not crash");
    Return<Result> ret = effect->setCurrentConfigForFeature(0, hidl_vec<uint8_t>());
    EXPECT_TRUE(ret.isOk());
}

TEST_P(AudioEffectHidlTest, GetSupportedConfigsForFeatureInvalidConfigSize) {
    description("Verify that GetSupportedConfigsForFeature caps the maximum config size");
    const bool isNewDeviceLaunchingOnTPlus = property_get_int32("ro.vendor.api_level", 0) >= 33;
    if (!isNewDeviceLaunchingOnTPlus) {
        GTEST_SKIP() << "The test only applies to devices launching on T or later";
    }
    // Use very large size to ensure that the service does not crash.
    const uint32_t veryLargeConfigSize = std::numeric_limits<uint32_t>::max() - 100;
    Result retval = Result::OK;
    Return<void> ret = effect->getSupportedConfigsForFeature(
            0, 1, veryLargeConfigSize,
            [&](Result r, uint32_t, const hidl_vec<uint8_t>&) { retval = r; });
    EXPECT_TRUE(ret.isOk());
    EXPECT_EQ(Result::INVALID_ARGUMENTS, retval);
}

TEST_P(AudioEffectHidlTest, GetCurrentConfigForFeatureInvalidConfigSize) {
    description("Verify that GetCurrentConfigForFeature caps the maximum config size");
    const bool isNewDeviceLaunchingOnTPlus = property_get_int32("ro.vendor.api_level", 0) >= 33;
    if (!isNewDeviceLaunchingOnTPlus) {
        GTEST_SKIP() << "The test only applies to devices launching on T or later";
    }
    // Use very large size to ensure that the service does not crash.
    const uint32_t veryLargeConfigSize = std::numeric_limits<uint32_t>::max() - 100;
    Result retval = Result::OK;
    Return<void> ret = effect->getCurrentConfigForFeature(
            0, veryLargeConfigSize, [&](Result r, const hidl_vec<uint8_t>&) { retval = r; });
    EXPECT_TRUE(ret.isOk());
    EXPECT_EQ(Result::INVALID_ARGUMENTS, retval);
}

// The main test class for Equalizer Audio Effect HIDL HAL.
class EqualizerAudioEffectHidlTest : public AudioEffectHidlTest {
  public:
    void SetUp() override {
        AudioEffectHidlTest::SetUp();
        equalizer = IEqualizerEffect::castFrom(effect);
        ASSERT_NE(nullptr, equalizer.get());
    }

    void TearDown() override {
        equalizer.clear();
        AudioEffectHidlTest::TearDown();
    }

  protected:
    void getNumBands(uint16_t* numBands);
    void getLevelRange(int16_t* minLevel, int16_t* maxLevel);
    void getBandFrequencyRange(uint16_t band, uint32_t* minFreq, uint32_t* centerFreq,
                               uint32_t* maxFreq);
    void getPresetCount(size_t* count);

    sp<IEqualizerEffect> equalizer;
};

void EqualizerAudioEffectHidlTest::getNumBands(uint16_t* numBands) {
    Result retval = Result::NOT_INITIALIZED;
    Return<void> ret = equalizer->getNumBands([&](Result r, uint16_t b) {
        retval = r;
        if (retval == Result::OK) {
            *numBands = b;
        }
    });
    ASSERT_TRUE(ret.isOk());
    ASSERT_EQ(Result::OK, retval);
}

void EqualizerAudioEffectHidlTest::getLevelRange(int16_t* minLevel, int16_t* maxLevel) {
    Result retval = Result::NOT_INITIALIZED;
    Return<void> ret = equalizer->getLevelRange([&](Result r, int16_t min, int16_t max) {
        retval = r;
        if (retval == Result::OK) {
            *minLevel = min;
            *maxLevel = max;
        }
    });
    ASSERT_TRUE(ret.isOk());
    ASSERT_EQ(Result::OK, retval);
}

void EqualizerAudioEffectHidlTest::getBandFrequencyRange(uint16_t band, uint32_t* minFreq,
                                                         uint32_t* centerFreq, uint32_t* maxFreq) {
    Result retval = Result::NOT_INITIALIZED;
    Return<void> ret =
        equalizer->getBandFrequencyRange(band, [&](Result r, uint32_t min, uint32_t max) {
            retval = r;
            if (retval == Result::OK) {
                *minFreq = min;
                *maxFreq = max;
            }
        });
    ASSERT_TRUE(ret.isOk());
    ASSERT_EQ(Result::OK, retval);
    ret = equalizer->getBandCenterFrequency(band, [&](Result r, uint32_t center) {
        retval = r;
        if (retval == Result::OK) {
            *centerFreq = center;
        }
    });
    ASSERT_TRUE(ret.isOk());
    ASSERT_EQ(Result::OK, retval);
}

void EqualizerAudioEffectHidlTest::getPresetCount(size_t* count) {
    Result retval = Result::NOT_INITIALIZED;
    Return<void> ret = equalizer->getPresetNames([&](Result r, const hidl_vec<hidl_string>& names) {
        retval = r;
        if (retval == Result::OK) {
            *count = names.size();
        }
    });
    ASSERT_TRUE(ret.isOk());
    ASSERT_EQ(Result::OK, retval);
}

TEST_P(EqualizerAudioEffectHidlTest, GetNumBands) {
    description("Verify that Equalizer effect reports at least one band");
    uint16_t numBands = 0;
    getNumBands(&numBands);
    EXPECT_GT(numBands, 0);
}

TEST_P(EqualizerAudioEffectHidlTest, GetLevelRange) {
    description("Verify that Equalizer effect reports adequate band level range");
    int16_t minLevel = 0x7fff, maxLevel = 0;
    getLevelRange(&minLevel, &maxLevel);
    EXPECT_GT(maxLevel, minLevel);
}

TEST_P(EqualizerAudioEffectHidlTest, GetSetBandLevel) {
    description("Verify that manipulating band levels works for Equalizer effect");
    uint16_t numBands = 0;
    getNumBands(&numBands);
    ASSERT_GT(numBands, 0);
    int16_t levels[3]{0x7fff, 0, 0};
    getLevelRange(&levels[0], &levels[2]);
    ASSERT_GT(levels[2], levels[0]);
    levels[1] = (levels[2] + levels[0]) / 2;
    for (uint16_t i = 0; i < numBands; ++i) {
        for (size_t j = 0; j < ARRAY_SIZE(levels); ++j) {
            Return<Result> ret = equalizer->setBandLevel(i, levels[j]);
            EXPECT_TRUE(ret.isOk());
            EXPECT_EQ(Result::OK, ret);
            Result retval = Result::NOT_INITIALIZED;
            int16_t actualLevel;
            Return<void> ret2 = equalizer->getBandLevel(i, [&](Result r, int16_t l) {
                retval = r;
                if (retval == Result::OK) {
                    actualLevel = l;
                }
            });
            EXPECT_TRUE(ret2.isOk());
            EXPECT_EQ(Result::OK, retval);
            EXPECT_EQ(levels[j], actualLevel);
        }
    }
}

TEST_P(EqualizerAudioEffectHidlTest, GetBandCenterFrequencyAndRange) {
    description("Verify that Equalizer effect reports adequate band frequency range");
    uint16_t numBands = 0;
    getNumBands(&numBands);
    ASSERT_GT(numBands, 0);
    for (uint16_t i = 0; i < numBands; ++i) {
        uint32_t minFreq = 0xffffffff, centerFreq = 0xffffffff, maxFreq = 0xffffffff;
        getBandFrequencyRange(i, &minFreq, &centerFreq, &maxFreq);
        // Note: NXP legacy implementation reports "1" as upper bound for last band,
        // so this check fails.
        EXPECT_GE(maxFreq, centerFreq);
        EXPECT_GE(centerFreq, minFreq);
    }
}

TEST_P(EqualizerAudioEffectHidlTest, GetBandForFrequency) {
    description("Verify that Equalizer effect supports GetBandForFrequency correctly");
    uint16_t numBands = 0;
    getNumBands(&numBands);
    ASSERT_GT(numBands, 0);
    for (uint16_t i = 0; i < numBands; ++i) {
        uint32_t freqs[3]{0, 0, 0};
        getBandFrequencyRange(i, &freqs[0], &freqs[1], &freqs[2]);
        // NXP legacy implementation reports "1" as upper bound for last band, some
        // of the checks fail.
        for (size_t j = 0; j < ARRAY_SIZE(freqs); ++j) {
            if (j == 0) {
                freqs[j]++;
            }  // Min frequency is an open interval.
            Result retval = Result::NOT_INITIALIZED;
            uint16_t actualBand = numBands + 1;
            Return<void> ret = equalizer->getBandForFrequency(freqs[j], [&](Result r, uint16_t b) {
                retval = r;
                if (retval == Result::OK) {
                    actualBand = b;
                }
            });
            EXPECT_TRUE(ret.isOk());
            EXPECT_EQ(Result::OK, retval);
            EXPECT_EQ(i, actualBand) << "Frequency: " << freqs[j];
        }
    }
}

TEST_P(EqualizerAudioEffectHidlTest, GetPresetNames) {
    description("Verify that Equalizer effect reports at least one preset");
    size_t presetCount;
    getPresetCount(&presetCount);
    EXPECT_GT(presetCount, 0u);
}

TEST_P(EqualizerAudioEffectHidlTest, GetSetCurrentPreset) {
    description("Verify that manipulating the current preset for Equalizer effect");
    size_t presetCount;
    getPresetCount(&presetCount);
    ASSERT_GT(presetCount, 0u);
    for (uint16_t i = 0; i < presetCount; ++i) {
        Return<Result> ret = equalizer->setCurrentPreset(i);
        EXPECT_TRUE(ret.isOk());
        EXPECT_EQ(Result::OK, ret);
        Result retval = Result::NOT_INITIALIZED;
        uint16_t actualPreset = 0xffff;
        Return<void> ret2 = equalizer->getCurrentPreset([&](Result r, uint16_t p) {
            retval = r;
            if (retval == Result::OK) {
                actualPreset = p;
            }
        });
        EXPECT_TRUE(ret2.isOk());
        EXPECT_EQ(Result::OK, retval);
        EXPECT_EQ(i, actualPreset);
    }
}

TEST_P(EqualizerAudioEffectHidlTest, GetSetAllProperties) {
    description(
        "Verify that setting band levels and presets works via Get / "
        "SetAllProperties for Equalizer effect");
    using AllProperties =
        ::android::hardware::audio::effect::CPP_VERSION::IEqualizerEffect::AllProperties;
    uint16_t numBands = 0;
    getNumBands(&numBands);
    ASSERT_GT(numBands, 0);
    AllProperties props;
    props.bandLevels.resize(numBands);
    for (size_t i = 0; i < numBands; ++i) {
        props.bandLevels[i] = 0;
    }

    AllProperties actualProps;
    Result retval = Result::NOT_INITIALIZED;

    // Verify setting of the band levels via properties.
    props.curPreset = -1;
    Return<Result> ret = equalizer->setAllProperties(props);
    EXPECT_TRUE(ret.isOk());
    EXPECT_EQ(Result::OK, ret);
    Return<void> ret2 = equalizer->getAllProperties([&](Result r, AllProperties p) {
        retval = r;
        if (retval == Result::OK) {
            actualProps = p;
        }
    });
    EXPECT_TRUE(ret2.isOk());
    EXPECT_EQ(Result::OK, retval);
    EXPECT_EQ(props.bandLevels, actualProps.bandLevels);

    // Verify setting of the current preset via properties.
    props.curPreset = 0;  // Assuming there is at least one preset.
    ret = equalizer->setAllProperties(props);
    EXPECT_TRUE(ret.isOk());
    EXPECT_EQ(Result::OK, ret);
    ret2 = equalizer->getAllProperties([&](Result r, AllProperties p) {
        retval = r;
        if (retval == Result::OK) {
            actualProps = p;
        }
    });
    EXPECT_TRUE(ret2.isOk());
    EXPECT_EQ(Result::OK, retval);
    EXPECT_EQ(props.curPreset, actualProps.curPreset);
}

// The main test class for Equalizer Audio Effect HIDL HAL.
class LoudnessEnhancerAudioEffectHidlTest : public AudioEffectHidlTest {
  public:
    void SetUp() override {
        AudioEffectHidlTest::SetUp();
        enhancer = ILoudnessEnhancerEffect::castFrom(effect);
        ASSERT_NE(nullptr, enhancer.get());
    }

    void TearDown() override {
        enhancer.clear();
        AudioEffectHidlTest::TearDown();
    }

  protected:
    sp<ILoudnessEnhancerEffect> enhancer;
};

TEST_P(LoudnessEnhancerAudioEffectHidlTest, GetSetTargetGain) {
    description(
        "Verify that manipulating the target gain works for Loudness Enhancer "
        "effect");
    const int32_t gain = 100;
    Return<Result> ret = enhancer->setTargetGain(gain);
    EXPECT_TRUE(ret.isOk());
    EXPECT_EQ(Result::OK, ret);
    int32_t actualGain = 0;
    Result retval;
    Return<void> ret2 = enhancer->getTargetGain([&](Result r, int32_t g) {
        retval = r;
        if (retval == Result::OK) {
            actualGain = g;
        }
    });
    EXPECT_TRUE(ret2.isOk());
    EXPECT_EQ(Result::OK, retval);
    EXPECT_EQ(gain, actualGain);
}

INSTANTIATE_TEST_SUITE_P(EffectsFactory, AudioEffectsFactoryHidlTest,
                         ::testing::ValuesIn(::android::hardware::getAllHalInstanceNames(
                                 IEffectsFactory::descriptor)),
                         ::android::hardware::PrintInstanceNameToString);
INSTANTIATE_TEST_SUITE_P(
        Equalizer_IEffect, AudioEffectHidlTest,
        ::testing::Combine(::testing::ValuesIn(::android::hardware::getAllHalInstanceNames(
                                   IEffectsFactory::descriptor)),
                           ::testing::Values(EQUALIZER_EFFECT_TYPE)),
        EffectParameterToString);
INSTANTIATE_TEST_SUITE_P(
        LoudnessEnhancer_IEffect, AudioEffectHidlTest,
        ::testing::Combine(::testing::ValuesIn(::android::hardware::getAllHalInstanceNames(
                                   IEffectsFactory::descriptor)),
                           ::testing::Values(LOUDNESS_ENHANCER_EFFECT_TYPE)),
        EffectParameterToString);
INSTANTIATE_TEST_SUITE_P(
        Equalizer, EqualizerAudioEffectHidlTest,
        ::testing::Combine(::testing::ValuesIn(::android::hardware::getAllHalInstanceNames(
                                   IEffectsFactory::descriptor)),
                           ::testing::Values(EQUALIZER_EFFECT_TYPE)),
        EffectParameterToString);
INSTANTIATE_TEST_SUITE_P(
        LoudnessEnhancer, LoudnessEnhancerAudioEffectHidlTest,
        ::testing::Combine(::testing::ValuesIn(::android::hardware::getAllHalInstanceNames(
                                   IEffectsFactory::descriptor)),
                           ::testing::Values(LOUDNESS_ENHANCER_EFFECT_TYPE)),
        EffectParameterToString);
// When the VTS test runs on a device lacking the corresponding HAL version the parameter
// list is empty, this isn't a problem.
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(AudioEffectsFactoryHidlTest);
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(AudioEffectHidlTest);
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(EqualizerAudioEffectHidlTest);
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(LoudnessEnhancerAudioEffectHidlTest);
