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

#if MAJOR_VERSION >= 7
#include <android_audio_policy_configuration_V7_0-enums.h>
#endif
#include <HidlUtils.h>
#include <log/log.h>

#include "util/CoreUtils.h"

using ::android::hardware::audio::common::CPP_VERSION::implementation::HidlUtils;
#if MAJOR_VERSION >= 7
namespace xsd {
using namespace ::android::audio::policy::configuration::V7_0;
}
#endif

namespace android {
namespace hardware {
namespace audio {
namespace CPP_VERSION {
namespace implementation {

#define CONVERT_CHECKED(expr, result)                   \
    if (status_t status = (expr); status != NO_ERROR) { \
        result = status;                                \
    }

status_t CoreUtils::deviceAddressToHal(const DeviceAddress& device, audio_devices_t* halDeviceType,
                                       char* halDeviceAddress) {
#if MAJOR_VERSION >= 5
    return HidlUtils::deviceAddressToHal(device, halDeviceType, halDeviceAddress);
#else
    return HidlUtils::deviceAddressToHalImpl(device, halDeviceType, halDeviceAddress);
#endif
}

status_t CoreUtils::deviceAddressFromHal(audio_devices_t halDeviceType,
                                         const char* halDeviceAddress, DeviceAddress* device) {
#if MAJOR_VERSION >= 5
    return HidlUtils::deviceAddressFromHal(halDeviceType, halDeviceAddress, device);
#else
    return HidlUtils::deviceAddressFromHalImpl(halDeviceType, halDeviceAddress, device);
#endif
}

#if MAJOR_VERSION >= 4
status_t CoreUtils::microphoneInfoFromHal(
        const struct audio_microphone_characteristic_t& halMicInfo, MicrophoneInfo* micInfo) {
    status_t result = NO_ERROR;
    micInfo->deviceId = halMicInfo.device_id;
    CONVERT_CHECKED(
            deviceAddressFromHal(halMicInfo.device, halMicInfo.address, &micInfo->deviceAddress),
            result);
    int chCount;
    for (chCount = AUDIO_CHANNEL_COUNT_MAX - 1; chCount >= 0; --chCount) {
        if (halMicInfo.channel_mapping[chCount] != AUDIO_MICROPHONE_CHANNEL_MAPPING_UNUSED) {
            break;
        }
    }
    micInfo->channelMapping.resize(chCount + 1);
    for (size_t ch = 0; ch < micInfo->channelMapping.size(); ch++) {
        micInfo->channelMapping[ch] = AudioMicrophoneChannelMapping(halMicInfo.channel_mapping[ch]);
    }
    micInfo->location = AudioMicrophoneLocation(halMicInfo.location);
    micInfo->group = AudioMicrophoneGroup(halMicInfo.group);
    micInfo->indexInTheGroup = static_cast<uint32_t>(halMicInfo.index_in_the_group);
    micInfo->sensitivity = halMicInfo.sensitivity;
    micInfo->maxSpl = halMicInfo.max_spl;
    micInfo->minSpl = halMicInfo.min_spl;
    micInfo->directionality = AudioMicrophoneDirectionality(halMicInfo.directionality);
    micInfo->frequencyResponse.resize(halMicInfo.num_frequency_responses);
    for (size_t k = 0; k < halMicInfo.num_frequency_responses; k++) {
        micInfo->frequencyResponse[k].frequency = halMicInfo.frequency_responses[0][k];
        micInfo->frequencyResponse[k].level = halMicInfo.frequency_responses[1][k];
    }
    micInfo->position.x = halMicInfo.geometric_location.x;
    micInfo->position.y = halMicInfo.geometric_location.y;
    micInfo->position.z = halMicInfo.geometric_location.z;
    micInfo->orientation.x = halMicInfo.orientation.x;
    micInfo->orientation.y = halMicInfo.orientation.y;
    micInfo->orientation.z = halMicInfo.orientation.z;
    return result;
}

status_t CoreUtils::microphoneInfoToHal(const MicrophoneInfo& micInfo,
                                        audio_microphone_characteristic_t* halMicInfo) {
    status_t result = NO_ERROR;
    strncpy(halMicInfo->device_id, micInfo.deviceId.c_str(), AUDIO_MICROPHONE_ID_MAX_LEN);
    halMicInfo->device_id[AUDIO_MICROPHONE_ID_MAX_LEN - 1] = '\0';
    if (micInfo.deviceId.size() >= AUDIO_MICROPHONE_ID_MAX_LEN) {
        ALOGE("HIDL MicrophoneInfo device ID is too long: %zu", micInfo.deviceId.size());
        result = BAD_VALUE;
    }
    CONVERT_CHECKED(
            deviceAddressToHal(micInfo.deviceAddress, &halMicInfo->device, halMicInfo->address),
            result);
    if (micInfo.channelMapping.size() > AUDIO_CHANNEL_COUNT_MAX) {
        ALOGE("HIDL MicrophoneInfo has too many channelMapping elements: %zu",
              micInfo.channelMapping.size());
        result = BAD_VALUE;
    }
    size_t ch;
    for (ch = 0; ch < micInfo.channelMapping.size() && ch < AUDIO_CHANNEL_COUNT_MAX; ch++) {
        halMicInfo->channel_mapping[ch] =
                static_cast<audio_microphone_channel_mapping_t>(micInfo.channelMapping[ch]);
    }
    for (; ch < AUDIO_CHANNEL_COUNT_MAX; ch++) {
        halMicInfo->channel_mapping[ch] = AUDIO_MICROPHONE_CHANNEL_MAPPING_UNUSED;
    }
    halMicInfo->location = static_cast<audio_microphone_location_t>(micInfo.location);
    halMicInfo->group = static_cast<audio_microphone_group_t>(micInfo.group);
    halMicInfo->index_in_the_group = static_cast<unsigned int>(micInfo.indexInTheGroup);
    halMicInfo->sensitivity = micInfo.sensitivity;
    halMicInfo->max_spl = micInfo.maxSpl;
    halMicInfo->min_spl = micInfo.minSpl;
    halMicInfo->directionality =
            static_cast<audio_microphone_directionality_t>(micInfo.directionality);
    halMicInfo->num_frequency_responses =
            static_cast<unsigned int>(micInfo.frequencyResponse.size());
    if (halMicInfo->num_frequency_responses > AUDIO_MICROPHONE_MAX_FREQUENCY_RESPONSES) {
        ALOGE("HIDL MicrophoneInfo has too many frequency responses: %u",
              halMicInfo->num_frequency_responses);
        halMicInfo->num_frequency_responses = AUDIO_MICROPHONE_MAX_FREQUENCY_RESPONSES;
        result = BAD_VALUE;
    }
    for (size_t k = 0; k < halMicInfo->num_frequency_responses; k++) {
        halMicInfo->frequency_responses[0][k] = micInfo.frequencyResponse[k].frequency;
        halMicInfo->frequency_responses[1][k] = micInfo.frequencyResponse[k].level;
    }
    halMicInfo->geometric_location.x = micInfo.position.x;
    halMicInfo->geometric_location.y = micInfo.position.y;
    halMicInfo->geometric_location.z = micInfo.position.z;
    halMicInfo->orientation.x = micInfo.orientation.x;
    halMicInfo->orientation.y = micInfo.orientation.y;
    halMicInfo->orientation.z = micInfo.orientation.z;
    return result;
}

status_t CoreUtils::sinkMetadataFromHal(const std::vector<record_track_metadata_t>& halTracks,
                                        SinkMetadata* sinkMetadata) {
    status_t result = NO_ERROR;
    sinkMetadata->tracks.resize(halTracks.size());
    for (size_t i = 0; i < sinkMetadata->tracks.size(); ++i) {
        const auto& halTrackMetadata = halTracks[i];
        RecordTrackMetadata trackMetadata{};
        CONVERT_CHECKED(
                HidlUtils::audioSourceFromHal(halTrackMetadata.source, &trackMetadata.source),
                result);
        trackMetadata.gain = halTrackMetadata.gain;
#if MAJOR_VERSION >= 5
        if (halTrackMetadata.dest_device != AUDIO_DEVICE_NONE) {
            DeviceAddress address;
            if (status_t status =
                        deviceAddressFromHal(halTrackMetadata.dest_device,
                                             halTrackMetadata.dest_device_address, &address);
                status == NO_ERROR) {
                trackMetadata.destination.device(std::move(address));
            } else {
                result = status;
            }
        }
#if MAJOR_VERSION >= 7
        trackMetadata.channelMask = toString(xsd::AudioChannelMask::AUDIO_CHANNEL_NONE);
#endif
#endif  // MAJOR_VERSION >= 5
        sinkMetadata->tracks[i] = std::move(trackMetadata);
    }
    return result;
}

status_t CoreUtils::sinkMetadataFromHalV7(const std::vector<record_track_metadata_v7_t>& halTracks,
                                          bool ignoreNonVendorTags, SinkMetadata* sinkMetadata) {
    std::vector<record_track_metadata_t> bases;
    bases.reserve(halTracks.size());
    std::transform(halTracks.begin(), halTracks.end(), std::back_inserter(bases),
                   [](const record_track_metadata_v7_t& src) -> record_track_metadata_t {
                       record_track_metadata_t result;
                       record_track_metadata_from_v7(&result, &src);
                       return result;
                   });
    status_t result = sinkMetadataFromHal(bases, sinkMetadata);
#if MAJOR_VERSION >= 7
    for (size_t i = 0; i < halTracks.size(); ++i) {
        auto& trackMetadata = sinkMetadata->tracks[i];
        const auto& halTrackMetadata = halTracks[i];
        CONVERT_CHECKED(
                HidlUtils::audioChannelMaskFromHal(halTrackMetadata.channel_mask, true /*isInput*/,
                                                   &trackMetadata.channelMask),
                result);
        std::vector<std::string> strTags = HidlUtils::splitAudioTags(halTrackMetadata.tags);
        if (ignoreNonVendorTags) {
            strTags = HidlUtils::filterOutNonVendorTags(strTags);
        }
        CONVERT_CHECKED(HidlUtils::audioTagsFromHal(strTags, &trackMetadata.tags), result);
    }
#else
    (void)ignoreNonVendorTags;
#endif
    return result;
}

status_t CoreUtils::sinkMetadataToHal(const SinkMetadata& sinkMetadata,
                                      std::vector<record_track_metadata_t>* halTracks) {
    status_t result = NO_ERROR;
    if (halTracks != nullptr) {
        halTracks->reserve(sinkMetadata.tracks.size());
    }
    for (auto& trackMetadata : sinkMetadata.tracks) {
        record_track_metadata halTrackMetadata{.gain = trackMetadata.gain};
        CONVERT_CHECKED(HidlUtils::audioSourceToHal(trackMetadata.source, &halTrackMetadata.source),
                        result);
#if MAJOR_VERSION >= 5
        if (trackMetadata.destination.getDiscriminator() ==
            RecordTrackMetadata::Destination::hidl_discriminator::device) {
            CONVERT_CHECKED(deviceAddressToHal(trackMetadata.destination.device(),
                                               &halTrackMetadata.dest_device,
                                               halTrackMetadata.dest_device_address),
                            result);
        }
#endif
        if (halTracks != nullptr) {
            halTracks->push_back(std::move(halTrackMetadata));
        }
    }
    return result;
}

status_t CoreUtils::sinkMetadataToHalV7(const SinkMetadata& sinkMetadata, bool ignoreNonVendorTags,
                                        std::vector<record_track_metadata_v7_t>* halTracks) {
    std::vector<record_track_metadata> bases;
    status_t result = sinkMetadataToHal(sinkMetadata, halTracks != nullptr ? &bases : nullptr);
    if (halTracks != nullptr) {
        halTracks->reserve(sinkMetadata.tracks.size());
    }
    for (size_t i = 0; i < sinkMetadata.tracks.size(); ++i) {
        record_track_metadata_v7_t halTrackMetadata;
        if (halTracks != nullptr) {
            record_track_metadata_to_v7(&halTrackMetadata, &bases[i]);
        }
#if MAJOR_VERSION >= 7
        const auto& trackMetadata = sinkMetadata.tracks[i];
        CONVERT_CHECKED(HidlUtils::audioChannelMaskToHal(trackMetadata.channelMask,
                                                         &halTrackMetadata.channel_mask),
                        result);
        if (ignoreNonVendorTags) {
            CONVERT_CHECKED(
                    HidlUtils::audioTagsToHal(HidlUtils::filterOutNonVendorTags(trackMetadata.tags),
                                              halTrackMetadata.tags),
                    result);
        } else {
            CONVERT_CHECKED(HidlUtils::audioTagsToHal(trackMetadata.tags, halTrackMetadata.tags),
                            result);
        }
#else
        (void)ignoreNonVendorTags;
#endif
        if (halTracks != nullptr) {
            halTracks->push_back(std::move(halTrackMetadata));
        }
    }
    return result;
}

status_t CoreUtils::sourceMetadataFromHal(const std::vector<playback_track_metadata_t>& halTracks,
                                          SourceMetadata* sourceMetadata) {
    status_t result = NO_ERROR;
    sourceMetadata->tracks.resize(halTracks.size());
    for (size_t i = 0; i < sourceMetadata->tracks.size(); ++i) {
        const auto& halTrackMetadata = halTracks[i];
        PlaybackTrackMetadata trackMetadata{};
        CONVERT_CHECKED(HidlUtils::audioUsageFromHal(halTrackMetadata.usage, &trackMetadata.usage),
                        result);
        CONVERT_CHECKED(HidlUtils::audioContentTypeFromHal(halTrackMetadata.content_type,
                                                           &trackMetadata.contentType),
                        result);
        trackMetadata.gain = halTrackMetadata.gain;
#if MAJOR_VERSION >= 7
        trackMetadata.channelMask = toString(xsd::AudioChannelMask::AUDIO_CHANNEL_NONE);
#endif
        sourceMetadata->tracks[i] = std::move(trackMetadata);
    }
    return result;
}

status_t CoreUtils::sourceMetadataFromHalV7(
        const std::vector<playback_track_metadata_v7_t>& halTracks, bool ignoreNonVendorTags,
        SourceMetadata* sourceMetadata) {
    std::vector<playback_track_metadata_t> bases;
    bases.reserve(halTracks.size());
    std::transform(halTracks.begin(), halTracks.end(), std::back_inserter(bases),
                   [](const playback_track_metadata_v7_t& src) -> playback_track_metadata_t {
                       playback_track_metadata_t result;
                       playback_track_metadata_from_v7(&result, &src);
                       return result;
                   });
    status_t result = sourceMetadataFromHal(bases, sourceMetadata);
#if MAJOR_VERSION >= 7
    for (size_t i = 0; i < halTracks.size(); ++i) {
        auto& trackMetadata = sourceMetadata->tracks[i];
        const auto& halTrackMetadata = halTracks[i];
        CONVERT_CHECKED(
                HidlUtils::audioChannelMaskFromHal(halTrackMetadata.channel_mask, false /*isInput*/,
                                                   &trackMetadata.channelMask),
                result);
        std::vector<std::string> strTags = HidlUtils::splitAudioTags(halTrackMetadata.tags);
        if (ignoreNonVendorTags) {
            strTags = HidlUtils::filterOutNonVendorTags(strTags);
        }
        CONVERT_CHECKED(HidlUtils::audioTagsFromHal(strTags, &trackMetadata.tags), result);
    }
#else
    (void)ignoreNonVendorTags;
#endif
    return result;
}

status_t CoreUtils::sourceMetadataToHal(const SourceMetadata& sourceMetadata,
                                        std::vector<playback_track_metadata_t>* halTracks) {
    status_t result = NO_ERROR;
    if (halTracks != nullptr) {
        halTracks->reserve(sourceMetadata.tracks.size());
    }
    for (auto& trackMetadata : sourceMetadata.tracks) {
        playback_track_metadata_t halTrackMetadata{.gain = trackMetadata.gain};
        CONVERT_CHECKED(HidlUtils::audioUsageToHal(trackMetadata.usage, &halTrackMetadata.usage),
                        result);
        CONVERT_CHECKED(HidlUtils::audioContentTypeToHal(trackMetadata.contentType,
                                                         &halTrackMetadata.content_type),
                        result);
        if (halTracks != nullptr) {
            halTracks->push_back(std::move(halTrackMetadata));
        }
    }
    return result;
}

status_t CoreUtils::sourceMetadataToHalV7(const SourceMetadata& sourceMetadata,
                                          bool ignoreNonVendorTags,
                                          std::vector<playback_track_metadata_v7_t>* halTracks) {
    std::vector<playback_track_metadata_t> bases;
    status_t result = sourceMetadataToHal(sourceMetadata, halTracks != nullptr ? &bases : nullptr);
    if (halTracks != nullptr) {
        halTracks->reserve(sourceMetadata.tracks.size());
    }
    for (size_t i = 0; i < sourceMetadata.tracks.size(); ++i) {
        playback_track_metadata_v7_t halTrackMetadata;
        if (halTracks != nullptr) {
            playback_track_metadata_to_v7(&halTrackMetadata, &bases[i]);
        }
#if MAJOR_VERSION >= 7
        const auto& trackMetadata = sourceMetadata.tracks[i];
        CONVERT_CHECKED(HidlUtils::audioChannelMaskToHal(trackMetadata.channelMask,
                                                         &halTrackMetadata.channel_mask),
                        result);
        if (ignoreNonVendorTags) {
            CONVERT_CHECKED(
                    HidlUtils::audioTagsToHal(HidlUtils::filterOutNonVendorTags(trackMetadata.tags),
                                              halTrackMetadata.tags),
                    result);
        } else {
            CONVERT_CHECKED(HidlUtils::audioTagsToHal(trackMetadata.tags, halTrackMetadata.tags),
                            result);
        }
#else
        (void)ignoreNonVendorTags;
#endif
        if (halTracks != nullptr) {
            halTracks->push_back(std::move(halTrackMetadata));
        }
    }
    return result;
}
#endif  // MAJOR_VERSION >= 4

#if MAJOR_VERSION >= 7
namespace xsd {
using namespace ::android::audio::policy::configuration::V7_0;
}

status_t CoreUtils::audioInputFlagsFromHal(audio_input_flags_t halFlagMask,
                                           AudioInputFlags* flags) {
    status_t status = NO_ERROR;
    std::vector<AudioInOutFlag> result;
    for (uint32_t bit = 0; halFlagMask != 0 && bit < sizeof(audio_input_flags_t) * 8; ++bit) {
        audio_input_flags_t flag = static_cast<audio_input_flags_t>(1u << bit);
        if ((flag & halFlagMask) == flag) {
            AudioInOutFlag flagStr = audio_input_flag_to_string(flag);
            if (!flagStr.empty() && !xsd::isUnknownAudioInOutFlag(flagStr)) {
                result.push_back(flagStr);
            } else {
                ALOGE("Unknown audio input flag value 0x%X", flag);
                status = BAD_VALUE;
            }
            halFlagMask = static_cast<audio_input_flags_t>(halFlagMask & ~flag);
        }
    }
    *flags = result;
    return status;
}

status_t CoreUtils::audioInputFlagsToHal(const AudioInputFlags& flags,
                                         audio_input_flags_t* halFlagMask) {
    status_t status = NO_ERROR;
    *halFlagMask = {};
    for (const auto& flag : flags) {
        audio_input_flags_t halFlag;
        if (!xsd::isUnknownAudioInOutFlag(flag) &&
            audio_input_flag_from_string(flag.c_str(), &halFlag)) {
            *halFlagMask = static_cast<audio_input_flags_t>(*halFlagMask | halFlag);
        } else {
            ALOGE("Unknown audio input flag \"%s\"", flag.c_str());
            status = BAD_VALUE;
        }
    }
    return status;
}

status_t CoreUtils::audioOutputFlagsFromHal(audio_output_flags_t halFlagMask,
                                            AudioOutputFlags* flags) {
    status_t status = NO_ERROR;
    std::vector<AudioInOutFlag> result;
    for (uint32_t bit = 0; halFlagMask != 0 && bit < sizeof(audio_output_flags_t) * 8; ++bit) {
        audio_output_flags_t flag = static_cast<audio_output_flags_t>(1u << bit);
        if ((flag & halFlagMask) == flag) {
            AudioInOutFlag flagStr = audio_output_flag_to_string(flag);
            if (!flagStr.empty() && !xsd::isUnknownAudioInOutFlag(flagStr)) {
                result.push_back(flagStr);
            } else {
                ALOGE("Unknown audio output flag value 0x%X", flag);
                status = BAD_VALUE;
            }
            halFlagMask = static_cast<audio_output_flags_t>(halFlagMask & ~flag);
        }
    }
    *flags = result;
    return status;
}

status_t CoreUtils::audioOutputFlagsToHal(const AudioOutputFlags& flags,
                                          audio_output_flags_t* halFlagMask) {
    status_t status = NO_ERROR;
    *halFlagMask = {};
    for (const auto& flag : flags) {
        audio_output_flags_t halFlag;
        if (!xsd::isUnknownAudioInOutFlag(flag) &&
            audio_output_flag_from_string(flag.c_str(), &halFlag)) {
            *halFlagMask = static_cast<audio_output_flags_t>(*halFlagMask | halFlag);
        } else {
            ALOGE("Unknown audio output flag \"%s\"", flag.c_str());
            status = BAD_VALUE;
        }
    }
    return status;
}
#endif

}  // namespace implementation
}  // namespace CPP_VERSION
}  // namespace audio
}  // namespace hardware
}  // namespace android
