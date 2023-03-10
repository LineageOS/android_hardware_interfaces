/*
 * Copyright (C) 2022 The Android Open Source Project
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

#include <cstdint>
#include <limits>
#include <type_traits>
#include <utility>
#include <vector>

#include <Utils.h>

#include <gtest/gtest.h>
#define LOG_TAG "Utils_Test"
#include <log/log.h>

using aidl::android::hardware::audio::common::getChannelCount;
using aidl::android::hardware::audio::common::getFrameSizeInBytes;
using aidl::android::hardware::audio::common::getPcmSampleSizeInBytes;
using aidl::android::media::audio::common::AudioChannelLayout;
using aidl::android::media::audio::common::AudioFormatDescription;
using aidl::android::media::audio::common::AudioFormatType;
using aidl::android::media::audio::common::PcmType;

TEST(UtilsTest, ChannelCountOddCases) {
    using Tag = AudioChannelLayout::Tag;
    EXPECT_EQ(0UL, getChannelCount(AudioChannelLayout{}));
    EXPECT_EQ(0UL, getChannelCount(AudioChannelLayout::make<Tag::invalid>(0)));
    EXPECT_EQ(0UL, getChannelCount(AudioChannelLayout::make<Tag::invalid>(-1)));
}

TEST(UtilsTest, ChannelCountForIndexMask) {
    using Tag = AudioChannelLayout::Tag;
    EXPECT_EQ(0UL, getChannelCount(AudioChannelLayout::make<Tag::indexMask>(0)));
#define VERIFY_INDEX_MASK(N)                                                                  \
    {                                                                                         \
        const auto l =                                                                        \
                AudioChannelLayout::make<Tag::indexMask>(AudioChannelLayout::INDEX_MASK_##N); \
        EXPECT_EQ(N##UL, getChannelCount(l)) << l.toString();                                 \
    }
    VERIFY_INDEX_MASK(1);
    VERIFY_INDEX_MASK(2);
    VERIFY_INDEX_MASK(3);
    VERIFY_INDEX_MASK(4);
    VERIFY_INDEX_MASK(5);
    VERIFY_INDEX_MASK(6);
    VERIFY_INDEX_MASK(7);
    VERIFY_INDEX_MASK(8);
    VERIFY_INDEX_MASK(9);
    VERIFY_INDEX_MASK(10);
    VERIFY_INDEX_MASK(11);
    VERIFY_INDEX_MASK(12);
    VERIFY_INDEX_MASK(13);
    VERIFY_INDEX_MASK(14);
    VERIFY_INDEX_MASK(15);
    VERIFY_INDEX_MASK(16);
    VERIFY_INDEX_MASK(17);
    VERIFY_INDEX_MASK(18);
    VERIFY_INDEX_MASK(19);
    VERIFY_INDEX_MASK(20);
    VERIFY_INDEX_MASK(21);
    VERIFY_INDEX_MASK(22);
    VERIFY_INDEX_MASK(23);
    VERIFY_INDEX_MASK(24);
#undef VERIFY_INDEX_MASK
}

TEST(UtilsTest, ChannelCountForLayoutMask) {
    using Tag = AudioChannelLayout::Tag;
    const std::vector<std::pair<size_t, int32_t>> kTestLayouts = {
            std::make_pair(0UL, 0),
            std::make_pair(1UL, AudioChannelLayout::LAYOUT_MONO),
            std::make_pair(2UL, AudioChannelLayout::LAYOUT_STEREO),
            std::make_pair(6UL, AudioChannelLayout::LAYOUT_5POINT1),
            std::make_pair(8UL, AudioChannelLayout::LAYOUT_7POINT1),
            std::make_pair(16UL, AudioChannelLayout::LAYOUT_9POINT1POINT6),
            std::make_pair(13UL, AudioChannelLayout::LAYOUT_13POINT_360RA),
            std::make_pair(24UL, AudioChannelLayout::LAYOUT_22POINT2),
            std::make_pair(3UL, AudioChannelLayout::LAYOUT_STEREO_HAPTIC_A),
            std::make_pair(4UL, AudioChannelLayout::LAYOUT_STEREO_HAPTIC_AB)};
    for (const auto& [expected_count, layout] : kTestLayouts) {
        const auto l = AudioChannelLayout::make<Tag::layoutMask>(layout);
        EXPECT_EQ(expected_count, getChannelCount(l)) << l.toString();
    }
}

TEST(UtilsTest, ChannelCountForVoiceMask) {
    using Tag = AudioChannelLayout::Tag;
    // clang-format off
    const std::vector<std::pair<size_t, int32_t>> kTestLayouts = {
            std::make_pair(0UL, 0),
            std::make_pair(1UL, AudioChannelLayout::VOICE_UPLINK_MONO),
            std::make_pair(1UL, AudioChannelLayout::VOICE_DNLINK_MONO),
            std::make_pair(2UL, AudioChannelLayout::VOICE_CALL_MONO)};
    // clang-format on
    for (const auto& [expected_count, layout] : kTestLayouts) {
        const auto l = AudioChannelLayout::make<Tag::voiceMask>(layout);
        EXPECT_EQ(expected_count, getChannelCount(l)) << l.toString();
    }
}

namespace {

AudioChannelLayout make_AudioChannelLayout_Mono() {
    return AudioChannelLayout::make<AudioChannelLayout::Tag::layoutMask>(
            AudioChannelLayout::LAYOUT_MONO);
}

AudioChannelLayout make_AudioChannelLayout_Stereo() {
    return AudioChannelLayout::make<AudioChannelLayout::Tag::layoutMask>(
            AudioChannelLayout::LAYOUT_STEREO);
}

AudioFormatDescription make_AudioFormatDescription(AudioFormatType type) {
    AudioFormatDescription result;
    result.type = type;
    return result;
}

AudioFormatDescription make_AudioFormatDescription(PcmType pcm) {
    auto result = make_AudioFormatDescription(AudioFormatType::PCM);
    result.pcm = pcm;
    return result;
}

AudioFormatDescription make_AudioFormatDescription(const std::string& encoding) {
    AudioFormatDescription result;
    result.encoding = encoding;
    return result;
}

AudioFormatDescription make_AudioFormatDescription(PcmType transport, const std::string& encoding) {
    auto result = make_AudioFormatDescription(encoding);
    result.pcm = transport;
    return result;
}

}  // namespace

TEST(UtilsTest, FrameSize) {
    EXPECT_EQ(0UL, getFrameSizeInBytes(AudioFormatDescription{}, AudioChannelLayout{}));
    EXPECT_EQ(sizeof(int16_t), getFrameSizeInBytes(make_AudioFormatDescription(PcmType::INT_16_BIT),
                                                   make_AudioChannelLayout_Mono()));
    EXPECT_EQ(2 * sizeof(int16_t),
              getFrameSizeInBytes(make_AudioFormatDescription(PcmType::INT_16_BIT),
                                  make_AudioChannelLayout_Stereo()));
    EXPECT_EQ(sizeof(int32_t), getFrameSizeInBytes(make_AudioFormatDescription(PcmType::INT_32_BIT),
                                                   make_AudioChannelLayout_Mono()));
    EXPECT_EQ(2 * sizeof(int32_t),
              getFrameSizeInBytes(make_AudioFormatDescription(PcmType::INT_32_BIT),
                                  make_AudioChannelLayout_Stereo()));
    EXPECT_EQ(sizeof(float), getFrameSizeInBytes(make_AudioFormatDescription(PcmType::FLOAT_32_BIT),
                                                 make_AudioChannelLayout_Mono()));
    EXPECT_EQ(2 * sizeof(float),
              getFrameSizeInBytes(make_AudioFormatDescription(PcmType::FLOAT_32_BIT),
                                  make_AudioChannelLayout_Stereo()));
    EXPECT_EQ(sizeof(uint8_t),
              getFrameSizeInBytes(make_AudioFormatDescription("bitstream"), AudioChannelLayout{}));
    EXPECT_EQ(sizeof(int16_t),
              getFrameSizeInBytes(make_AudioFormatDescription(PcmType::INT_16_BIT, "encapsulated"),
                                  AudioChannelLayout{}));
}

TEST(UtilsTest, PcmSampleSize) {
    EXPECT_EQ(1UL, getPcmSampleSizeInBytes(PcmType{}));
    EXPECT_EQ(sizeof(uint8_t), getPcmSampleSizeInBytes(PcmType::UINT_8_BIT));
    EXPECT_EQ(sizeof(int16_t), getPcmSampleSizeInBytes(PcmType::INT_16_BIT));
    EXPECT_EQ(sizeof(int32_t), getPcmSampleSizeInBytes(PcmType::INT_32_BIT));
    EXPECT_EQ(sizeof(int32_t), getPcmSampleSizeInBytes(PcmType::FIXED_Q_8_24));
    EXPECT_EQ(sizeof(float), getPcmSampleSizeInBytes(PcmType::FLOAT_32_BIT));
    EXPECT_EQ(3UL, getPcmSampleSizeInBytes(PcmType::INT_24_BIT));
    EXPECT_EQ(0UL, getPcmSampleSizeInBytes(PcmType(-1)));
    using PcmTypeUnderlyingType = std::underlying_type_t<PcmType>;
    EXPECT_EQ(0UL,
              getPcmSampleSizeInBytes(PcmType(std::numeric_limits<PcmTypeUnderlyingType>::min())));
    EXPECT_EQ(0UL,
              getPcmSampleSizeInBytes(PcmType(std::numeric_limits<PcmTypeUnderlyingType>::max())));
}
