/*
 * Copyright (C) 2023 The Android Open Source Project
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

#include <map>
#include <set>

#include <Utils.h>
#include <aidl/android/media/audio/common/AudioFormatType.h>
#include <aidl/android/media/audio/common/PcmType.h>

#include "UsbAlsaUtils.h"
#include "core-impl/utils.h"

using aidl::android::media::audio::common::AudioChannelLayout;
using aidl::android::media::audio::common::AudioFormatDescription;
using aidl::android::media::audio::common::AudioFormatType;
using aidl::android::media::audio::common::PcmType;
using android::hardware::audio::common::getChannelCount;

namespace aidl::android::hardware::audio::core::usb {

namespace {

using AudioChannelCountToMaskMap = std::map<unsigned int, AudioChannelLayout>;
using AudioFormatDescToPcmFormatMap = std::map<AudioFormatDescription, enum pcm_format>;
using PcmFormatToAudioFormatDescMap = std::map<enum pcm_format, AudioFormatDescription>;

static const AudioChannelLayout INVALID_CHANNEL_LAYOUT =
        AudioChannelLayout::make<AudioChannelLayout::Tag::invalid>(0);

#define DEFINE_CHANNEL_LAYOUT_MASK(n) \
    AudioChannelLayout::make<AudioChannelLayout::Tag::layoutMask>(AudioChannelLayout::LAYOUT_##n)

static const std::set<AudioChannelLayout> SUPPORTED_OUT_CHANNEL_LAYOUTS = {
        DEFINE_CHANNEL_LAYOUT_MASK(MONO),          DEFINE_CHANNEL_LAYOUT_MASK(STEREO),
        DEFINE_CHANNEL_LAYOUT_MASK(2POINT1),       DEFINE_CHANNEL_LAYOUT_MASK(QUAD),
        DEFINE_CHANNEL_LAYOUT_MASK(PENTA),         DEFINE_CHANNEL_LAYOUT_MASK(5POINT1),
        DEFINE_CHANNEL_LAYOUT_MASK(6POINT1),       DEFINE_CHANNEL_LAYOUT_MASK(7POINT1),
        DEFINE_CHANNEL_LAYOUT_MASK(7POINT1POINT4), DEFINE_CHANNEL_LAYOUT_MASK(22POINT2),
};

static const std::set<AudioChannelLayout> SUPPORTED_IN_CHANNEL_LAYOUTS = {
        DEFINE_CHANNEL_LAYOUT_MASK(MONO),
        DEFINE_CHANNEL_LAYOUT_MASK(STEREO),
};

#define DEFINE_CHANNEL_INDEX_MASK(n) \
    AudioChannelLayout::make<AudioChannelLayout::Tag::indexMask>(AudioChannelLayout::INDEX_MASK_##n)

static const std::set<AudioChannelLayout> SUPPORTED_INDEX_CHANNEL_LAYOUTS = {
        DEFINE_CHANNEL_INDEX_MASK(1),  DEFINE_CHANNEL_INDEX_MASK(2),  DEFINE_CHANNEL_INDEX_MASK(3),
        DEFINE_CHANNEL_INDEX_MASK(4),  DEFINE_CHANNEL_INDEX_MASK(5),  DEFINE_CHANNEL_INDEX_MASK(6),
        DEFINE_CHANNEL_INDEX_MASK(7),  DEFINE_CHANNEL_INDEX_MASK(8),  DEFINE_CHANNEL_INDEX_MASK(9),
        DEFINE_CHANNEL_INDEX_MASK(10), DEFINE_CHANNEL_INDEX_MASK(11), DEFINE_CHANNEL_INDEX_MASK(12),
        DEFINE_CHANNEL_INDEX_MASK(13), DEFINE_CHANNEL_INDEX_MASK(14), DEFINE_CHANNEL_INDEX_MASK(15),
        DEFINE_CHANNEL_INDEX_MASK(16), DEFINE_CHANNEL_INDEX_MASK(17), DEFINE_CHANNEL_INDEX_MASK(18),
        DEFINE_CHANNEL_INDEX_MASK(19), DEFINE_CHANNEL_INDEX_MASK(20), DEFINE_CHANNEL_INDEX_MASK(21),
        DEFINE_CHANNEL_INDEX_MASK(22), DEFINE_CHANNEL_INDEX_MASK(23), DEFINE_CHANNEL_INDEX_MASK(24),
};

static AudioChannelCountToMaskMap make_ChannelCountToMaskMap(
        const std::set<AudioChannelLayout>& channelMasks) {
    AudioChannelCountToMaskMap channelMaskToCountMap;
    for (const auto& channelMask : channelMasks) {
        channelMaskToCountMap.emplace(getChannelCount(channelMask), channelMask);
    }
    return channelMaskToCountMap;
}

const AudioChannelCountToMaskMap& getSupportedChannelOutLayoutMap() {
    static const AudioChannelCountToMaskMap outLayouts =
            make_ChannelCountToMaskMap(SUPPORTED_OUT_CHANNEL_LAYOUTS);
    return outLayouts;
}

const AudioChannelCountToMaskMap& getSupportedChannelInLayoutMap() {
    static const AudioChannelCountToMaskMap inLayouts =
            make_ChannelCountToMaskMap(SUPPORTED_IN_CHANNEL_LAYOUTS);
    return inLayouts;
}

const AudioChannelCountToMaskMap& getSupportedChannelIndexLayoutMap() {
    static const AudioChannelCountToMaskMap indexLayouts =
            make_ChannelCountToMaskMap(SUPPORTED_INDEX_CHANNEL_LAYOUTS);
    return indexLayouts;
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

const AudioFormatDescToPcmFormatMap& getAudioFormatDescriptorToPcmFormatMap() {
    static const AudioFormatDescToPcmFormatMap formatDescToPcmFormatMap = {
            {make_AudioFormatDescription(PcmType::UINT_8_BIT), PCM_FORMAT_S8},
            {make_AudioFormatDescription(PcmType::INT_16_BIT), PCM_FORMAT_S16_LE},
            {make_AudioFormatDescription(PcmType::INT_24_BIT), PCM_FORMAT_S24_LE},
            {make_AudioFormatDescription(PcmType::FIXED_Q_8_24), PCM_FORMAT_S24_3LE},
            {make_AudioFormatDescription(PcmType::INT_32_BIT), PCM_FORMAT_S32_LE},
            {make_AudioFormatDescription(PcmType::FLOAT_32_BIT), PCM_FORMAT_FLOAT_LE},
    };
    return formatDescToPcmFormatMap;
}

static PcmFormatToAudioFormatDescMap make_PcmFormatToAudioFormatDescMap(
        const AudioFormatDescToPcmFormatMap& formatDescToPcmFormatMap) {
    PcmFormatToAudioFormatDescMap result;
    for (const auto& formatPair : formatDescToPcmFormatMap) {
        result.emplace(formatPair.second, formatPair.first);
    }
    return result;
}

const PcmFormatToAudioFormatDescMap& getPcmFormatToAudioFormatDescMap() {
    static const PcmFormatToAudioFormatDescMap pcmFormatToFormatDescMap =
            make_PcmFormatToAudioFormatDescMap(getAudioFormatDescriptorToPcmFormatMap());
    return pcmFormatToFormatDescMap;
}

}  // namespace

AudioChannelLayout getChannelLayoutMaskFromChannelCount(unsigned int channelCount, int isInput) {
    return findValueOrDefault(
            isInput ? getSupportedChannelInLayoutMap() : getSupportedChannelOutLayoutMap(),
            channelCount, INVALID_CHANNEL_LAYOUT);
}

AudioChannelLayout getChannelIndexMaskFromChannelCount(unsigned int channelCount) {
    return findValueOrDefault(getSupportedChannelIndexLayoutMap(), channelCount,
                              INVALID_CHANNEL_LAYOUT);
}

unsigned int getChannelCountFromChannelMask(const AudioChannelLayout& channelMask, bool isInput) {
    switch (channelMask.getTag()) {
        case AudioChannelLayout::Tag::layoutMask: {
            return findKeyOrDefault(
                    isInput ? getSupportedChannelInLayoutMap() : getSupportedChannelOutLayoutMap(),
                    (unsigned int)getChannelCount(channelMask), 0u /*defaultValue*/);
        }
        case AudioChannelLayout::Tag::indexMask: {
            return findKeyOrDefault(getSupportedChannelIndexLayoutMap(),
                                    (unsigned int)getChannelCount(channelMask),
                                    0u /*defaultValue*/);
        }
        case AudioChannelLayout::Tag::none:
        case AudioChannelLayout::Tag::invalid:
        case AudioChannelLayout::Tag::voiceMask:
        default:
            return 0;
    }
}

AudioFormatDescription legacy2aidl_pcm_format_AudioFormatDescription(enum pcm_format legacy) {
    return findValueOrDefault(getPcmFormatToAudioFormatDescMap(), legacy, AudioFormatDescription());
}

pcm_format aidl2legacy_AudioFormatDescription_pcm_format(const AudioFormatDescription& aidl) {
    return findValueOrDefault(getAudioFormatDescriptorToPcmFormatMap(), aidl, PCM_FORMAT_INVALID);
}

}  // namespace aidl::android::hardware::audio::core::usb