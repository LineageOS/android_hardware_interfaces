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

#pragma once

#include <algorithm>
#include <array>
#include <initializer_list>
#include <regex>
#include <type_traits>

#include <aidl/android/media/audio/common/AudioChannelLayout.h>
#include <aidl/android/media/audio/common/AudioDeviceType.h>
#include <aidl/android/media/audio/common/AudioFormatDescription.h>
#include <aidl/android/media/audio/common/AudioInputFlags.h>
#include <aidl/android/media/audio/common/AudioMode.h>
#include <aidl/android/media/audio/common/AudioOutputFlags.h>
#include <aidl/android/media/audio/common/PcmType.h>
#include <android/binder_auto_utils.h>

namespace ndk {

// This enables use of 'error/expected_utils' for ScopedAStatus.

inline bool errorIsOk(const ScopedAStatus& s) {
    return s.isOk();
}

inline std::string errorToString(const ScopedAStatus& s) {
    return s.getDescription();
}

}  // namespace ndk

namespace aidl::android::hardware::audio::common {

// Some values are reserved for use by the system code only.
// HALs must not accept or emit values outside from the provided list.
constexpr std::array<::aidl::android::media::audio::common::AudioMode, 5> kValidAudioModes = {
        ::aidl::android::media::audio::common::AudioMode::NORMAL,
        ::aidl::android::media::audio::common::AudioMode::RINGTONE,
        ::aidl::android::media::audio::common::AudioMode::IN_CALL,
        ::aidl::android::media::audio::common::AudioMode::IN_COMMUNICATION,
        ::aidl::android::media::audio::common::AudioMode::CALL_SCREEN,
};

constexpr size_t getPcmSampleSizeInBytes(::aidl::android::media::audio::common::PcmType pcm) {
    using ::aidl::android::media::audio::common::PcmType;
    switch (pcm) {
        case PcmType::UINT_8_BIT:
            return 1;
        case PcmType::INT_16_BIT:
            return 2;
        case PcmType::INT_32_BIT:
            return 4;
        case PcmType::FIXED_Q_8_24:
            return 4;
        case PcmType::FLOAT_32_BIT:
            return 4;
        case PcmType::INT_24_BIT:
            return 3;
    }
    return 0;
}

constexpr size_t getChannelCount(
        const ::aidl::android::media::audio::common::AudioChannelLayout& layout,
        int32_t mask = std::numeric_limits<int32_t>::max()) {
    using Tag = ::aidl::android::media::audio::common::AudioChannelLayout::Tag;
    switch (layout.getTag()) {
        case Tag::none:
            return 0;
        case Tag::invalid:
            return 0;
        case Tag::indexMask:
            return __builtin_popcount(layout.get<Tag::indexMask>() & mask);
        case Tag::layoutMask:
            return __builtin_popcount(layout.get<Tag::layoutMask>() & mask);
        case Tag::voiceMask:
            return __builtin_popcount(layout.get<Tag::voiceMask>() & mask);
    }
    return 0;
}

constexpr size_t getFrameSizeInBytes(
        const ::aidl::android::media::audio::common::AudioFormatDescription& format,
        const ::aidl::android::media::audio::common::AudioChannelLayout& layout) {
    if (format == ::aidl::android::media::audio::common::AudioFormatDescription{}) {
        // Unspecified format.
        return 0;
    }
    using ::aidl::android::media::audio::common::AudioFormatType;
    if (format.type == AudioFormatType::PCM) {
        return getPcmSampleSizeInBytes(format.pcm) * getChannelCount(layout);
    } else if (format.type == AudioFormatType::NON_PCM) {
        // For non-PCM formats always use the underlying PCM size. The default value for
        // PCM is "UINT_8_BIT", thus non-encapsulated streams have the frame size of 1.
        return getPcmSampleSizeInBytes(format.pcm);
    }
    // Something unexpected.
    return 0;
}

constexpr bool isDefaultAudioFormat(
        const ::aidl::android::media::audio::common::AudioFormatDescription& desc) {
    return desc.type == ::aidl::android::media::audio::common::AudioFormatType::DEFAULT &&
           desc.pcm == ::aidl::android::media::audio::common::PcmType::DEFAULT &&
           desc.encoding.empty();
}

constexpr bool isTelephonyDeviceType(
        ::aidl::android::media::audio::common::AudioDeviceType device) {
    return device == ::aidl::android::media::audio::common::AudioDeviceType::IN_TELEPHONY_RX ||
           device == ::aidl::android::media::audio::common::AudioDeviceType::OUT_TELEPHONY_TX;
}

constexpr bool isValidAudioMode(::aidl::android::media::audio::common::AudioMode mode) {
    return std::find(kValidAudioModes.begin(), kValidAudioModes.end(), mode) !=
           kValidAudioModes.end();
}

static inline bool maybeVendorExtension(const std::string& s) {
    // Only checks whether the string starts with the "vendor prefix".
    static const std::string vendorPrefix = "VX_";
    return s.size() > vendorPrefix.size() && s.substr(0, vendorPrefix.size()) == vendorPrefix;
}

static inline bool isVendorExtension(const std::string& s) {
    // Must be the same as defined in {Playback|Record}TrackMetadata.aidl
    static const std::regex vendorExtension("VX_[A-Z0-9]{3,}_[_A-Z0-9]+");
    return std::regex_match(s.begin(), s.end(), vendorExtension);
}

// The helper functions defined below are only applicable to the case when an enum type
// specifies zero-based bit positions, not bit masks themselves. This is why instantiation
// is restricted to certain enum types.
template <typename E>
using is_bit_position_enum = std::integral_constant<
        bool, std::is_same_v<E, ::aidl::android::media::audio::common::AudioInputFlags> ||
                      std::is_same_v<E, ::aidl::android::media::audio::common::AudioOutputFlags>>;

template <typename E, typename U = std::underlying_type_t<E>,
          typename = std::enable_if_t<is_bit_position_enum<E>::value>>
constexpr U makeBitPositionFlagMask(E flag) {
    return 1 << static_cast<U>(flag);
}

template <typename E, typename U = std::underlying_type_t<E>,
          typename = std::enable_if_t<is_bit_position_enum<E>::value>>
constexpr bool isBitPositionFlagSet(U mask, E flag) {
    return (mask & makeBitPositionFlagMask(flag)) != 0;
}

template <typename E, typename U = std::underlying_type_t<E>,
          typename = std::enable_if_t<is_bit_position_enum<E>::value>>
constexpr U makeBitPositionFlagMask(std::initializer_list<E> flags) {
    U result = 0;
    for (const auto flag : flags) {
        result |= makeBitPositionFlagMask(flag);
    }
    return result;
}

constexpr int32_t frameCountFromDurationUs(long durationUs, int32_t sampleRateHz) {
    return (static_cast<long long>(durationUs) * sampleRateHz) / 1000000LL;
}

constexpr int32_t frameCountFromDurationMs(int32_t durationMs, int32_t sampleRateHz) {
    return frameCountFromDurationUs(durationMs * 1000, sampleRateHz);
}

}  // namespace aidl::android::hardware::audio::common
