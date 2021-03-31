/*
 * Copyright (C) 2021 The Android Open Source Project
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

#include <string>

#include <gtest/gtest.h>

#define LOG_TAG "EffectUtils_Test"
#include <log/log.h>

#include <android_audio_policy_configuration_V7_0-enums.h>
#include <system/audio_effect.h>
#include <util/EffectUtils.h>
#include <xsdc/XsdcSupport.h>

using namespace android;
using namespace ::android::hardware::audio::common::CPP_VERSION;
using namespace ::android::hardware::audio::effect::CPP_VERSION;
using ::android::hardware::audio::effect::CPP_VERSION::implementation::EffectUtils;
namespace xsd {
using namespace ::android::audio::policy::configuration::V7_0;
}

static constexpr audio_channel_mask_t kInvalidHalChannelMask = AUDIO_CHANNEL_INVALID;
static constexpr audio_format_t kInvalidHalFormat = AUDIO_FORMAT_INVALID;

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

inline bool operator==(const EffectBufferConfig& lhs, const EffectBufferConfig& rhs) {
    return lhs.buffer.getDiscriminator() == rhs.buffer.getDiscriminator() &&
           (lhs.buffer.getDiscriminator() ==
                    EffectBufferConfig::OptionalBuffer::hidl_discriminator::unspecified ||
            lhs.buffer.buf() == rhs.buffer.buf()) &&
           lhs.base == rhs.base && lhs.accessMode == rhs.accessMode;
}

inline bool operator==(const EffectConfig& lhs, const EffectConfig& rhs) {
    return lhs.inputCfg == rhs.inputCfg && lhs.outputCfg == rhs.outputCfg;
}
}  // namespace CPP_VERSION
}  // namespace effect
}  // namespace audio
}  // namespace hardware
}  // namespace android

TEST(EffectUtils, ConvertInvalidBufferConfig) {
    buffer_config_t halInvalid;
    EffectBufferConfig invalidChannelMask;
    invalidChannelMask.base.channelMask.value("random string");
    EXPECT_EQ(BAD_VALUE, EffectUtils::effectBufferConfigToHal(invalidChannelMask, &halInvalid));
    EffectBufferConfig invalidFormat;
    invalidFormat.base.format.value("random string");
    EXPECT_EQ(BAD_VALUE, EffectUtils::effectBufferConfigToHal(invalidFormat, &halInvalid));

    buffer_config_t halInvalidChannelMask;
    EffectBufferConfig invalid;
    halInvalidChannelMask.channels = kInvalidHalChannelMask;
    halInvalidChannelMask.mask = EFFECT_CONFIG_CHANNELS;
    EXPECT_EQ(BAD_VALUE, EffectUtils::effectBufferConfigFromHal(halInvalidChannelMask,
                                                                false /*isInput*/, &invalid));
    EXPECT_EQ(BAD_VALUE, EffectUtils::effectBufferConfigFromHal(halInvalidChannelMask,
                                                                true /*isInput*/, &invalid));
    buffer_config_t halInvalidFormat;
    halInvalidFormat.format = (uint8_t)kInvalidHalFormat;
    halInvalidFormat.mask = EFFECT_CONFIG_FORMAT;
    EXPECT_EQ(BAD_VALUE, EffectUtils::effectBufferConfigFromHal(halInvalidFormat, false /*isInput*/,
                                                                &invalid));
    EXPECT_EQ(BAD_VALUE,
              EffectUtils::effectBufferConfigFromHal(halInvalidFormat, true /*isInput*/, &invalid));
}

TEST(EffectUtils, ConvertBufferConfig) {
    EffectBufferConfig empty;
    buffer_config_t halEmpty;
    EXPECT_EQ(NO_ERROR, EffectUtils::effectBufferConfigToHal(empty, &halEmpty));
    EffectBufferConfig emptyBackOut;
    EXPECT_EQ(NO_ERROR,
              EffectUtils::effectBufferConfigFromHal(halEmpty, false /*isInput*/, &emptyBackOut));
    EXPECT_EQ(empty, emptyBackOut);
    EffectBufferConfig emptyBackIn;
    EXPECT_EQ(NO_ERROR,
              EffectUtils::effectBufferConfigFromHal(halEmpty, true /*isInput*/, &emptyBackIn));
    EXPECT_EQ(empty, emptyBackIn);

    EffectBufferConfig chanMask;
    chanMask.base.channelMask.value(toString(xsd::AudioChannelMask::AUDIO_CHANNEL_OUT_STEREO));
    buffer_config_t halChanMask;
    EXPECT_EQ(NO_ERROR, EffectUtils::effectBufferConfigToHal(chanMask, &halChanMask));
    EffectBufferConfig chanMaskBack;
    EXPECT_EQ(NO_ERROR, EffectUtils::effectBufferConfigFromHal(halChanMask, false /*isInput*/,
                                                               &chanMaskBack));
    EXPECT_EQ(chanMask, chanMaskBack);

    EffectBufferConfig format;
    format.base.format.value(toString(xsd::AudioFormat::AUDIO_FORMAT_PCM_16_BIT));
    buffer_config_t halFormat;
    EXPECT_EQ(NO_ERROR, EffectUtils::effectBufferConfigToHal(format, &halFormat));
    EffectBufferConfig formatBackOut;
    EXPECT_EQ(NO_ERROR,
              EffectUtils::effectBufferConfigFromHal(halFormat, false /*isInput*/, &formatBackOut));
    EXPECT_EQ(format, formatBackOut);
    EffectBufferConfig formatBackIn;
    EXPECT_EQ(NO_ERROR,
              EffectUtils::effectBufferConfigFromHal(halFormat, true /*isInput*/, &formatBackIn));
    EXPECT_EQ(format, formatBackIn);
}

TEST(EffectUtils, ConvertInvalidDescriptor) {
    effect_descriptor_t halDesc;
    EffectDescriptor longName{};
    longName.name = std::string(EFFECT_STRING_LEN_MAX, 'x');
    EXPECT_EQ(BAD_VALUE, EffectUtils::effectDescriptorToHal(longName, &halDesc));
    EffectDescriptor longImplementor{};
    longImplementor.implementor = std::string(EFFECT_STRING_LEN_MAX, 'x');
    EXPECT_EQ(BAD_VALUE, EffectUtils::effectDescriptorToHal(longImplementor, &halDesc));
}

TEST(EffectUtils, ConvertDescriptor) {
    EffectDescriptor desc{};
    desc.name = "test";
    desc.implementor = "foo";
    effect_descriptor_t halDesc;
    EXPECT_EQ(NO_ERROR, EffectUtils::effectDescriptorToHal(desc, &halDesc));
    EffectDescriptor descBack;
    EXPECT_EQ(NO_ERROR, EffectUtils::effectDescriptorFromHal(halDesc, &descBack));
    EXPECT_EQ(desc, descBack);
}

TEST(EffectUtils, ConvertNameAndImplementor) {
    for (size_t i = 0; i < EFFECT_STRING_LEN_MAX; ++i) {
        effect_descriptor_t halDesc{};
        for (size_t c = 0; c < i; ++c) {  // '<' to accommodate NUL terminator.
            halDesc.name[c] = halDesc.implementor[c] = 'A' + static_cast<char>(c);
        }
        EffectDescriptor desc;
        EXPECT_EQ(NO_ERROR, EffectUtils::effectDescriptorFromHal(halDesc, &desc));
        effect_descriptor_t halDescBack;
        EXPECT_EQ(NO_ERROR, EffectUtils::effectDescriptorToHal(desc, &halDescBack));
        EXPECT_EQ(i, strlen(halDescBack.name));
        EXPECT_EQ(i, strlen(halDescBack.implementor));
        EXPECT_EQ(0, strcmp(halDesc.name, halDescBack.name));
        EXPECT_EQ(0, strcmp(halDesc.implementor, halDescBack.implementor));
    }
}
