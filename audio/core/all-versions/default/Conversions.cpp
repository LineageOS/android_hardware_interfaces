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

#include "core/default/Conversions.h"

#include <stdio.h>

#if MAJOR_VERSION >= 7
#include <android_audio_policy_configuration_V7_0-enums.h>
#endif
#include <HidlUtils.h>
#include <log/log.h>

namespace android {
namespace hardware {
namespace audio {
namespace CPP_VERSION {
namespace implementation {

using ::android::hardware::audio::common::CPP_VERSION::implementation::HidlUtils;

#define CONVERT_CHECKED(expr, result)                   \
    if (status_t status = (expr); status != NO_ERROR) { \
        result = status;                                \
    }

status_t deviceAddressToHal(const DeviceAddress& device, audio_devices_t* halDeviceType,
                            char* halDeviceAddress) {
#if MAJOR_VERSION >= 5
    return HidlUtils::deviceAddressToHal(device, halDeviceType, halDeviceAddress);
#else
    return HidlUtils::deviceAddressToHalImpl(device, halDeviceType, halDeviceAddress);
#endif
}

status_t deviceAddressFromHal(audio_devices_t halDeviceType, const char* halDeviceAddress,
                              DeviceAddress* device) {
#if MAJOR_VERSION >= 5
    return HidlUtils::deviceAddressFromHal(halDeviceType, halDeviceAddress, device);
#else
    return HidlUtils::deviceAddressFromHalImpl(halDeviceType, halDeviceAddress, device);
#endif
}

#if MAJOR_VERSION >= 4
bool halToMicrophoneCharacteristics(MicrophoneInfo* pDst,
                                    const struct audio_microphone_characteristic_t& src) {
    bool status = false;
    if (pDst != NULL) {
        pDst->deviceId = src.device_id;

        if (deviceAddressFromHal(src.device, src.address, &pDst->deviceAddress) != OK) {
            return false;
        }
        pDst->channelMapping.resize(AUDIO_CHANNEL_COUNT_MAX);
        for (size_t ch = 0; ch < pDst->channelMapping.size(); ch++) {
            pDst->channelMapping[ch] = AudioMicrophoneChannelMapping(src.channel_mapping[ch]);
        }
        pDst->location = AudioMicrophoneLocation(src.location);
        pDst->group = (AudioMicrophoneGroup)src.group;
        pDst->indexInTheGroup = (uint32_t)src.index_in_the_group;
        pDst->sensitivity = src.sensitivity;
        pDst->maxSpl = src.max_spl;
        pDst->minSpl = src.min_spl;
        pDst->directionality = AudioMicrophoneDirectionality(src.directionality);
        pDst->frequencyResponse.resize(src.num_frequency_responses);
        for (size_t k = 0; k < src.num_frequency_responses; k++) {
            pDst->frequencyResponse[k].frequency = src.frequency_responses[0][k];
            pDst->frequencyResponse[k].level = src.frequency_responses[1][k];
        }
        pDst->position.x = src.geometric_location.x;
        pDst->position.y = src.geometric_location.y;
        pDst->position.z = src.geometric_location.z;

        pDst->orientation.x = src.orientation.x;
        pDst->orientation.y = src.orientation.y;
        pDst->orientation.z = src.orientation.z;

        status = true;
    }
    return status;
}

status_t sinkMetadataToHal(const SinkMetadata& sinkMetadata,
                           std::vector<record_track_metadata>* halTracks) {
    status_t result = NO_ERROR;
    if (halTracks != nullptr) {
        halTracks->reserve(sinkMetadata.tracks.size());
    }
    for (auto& metadata : sinkMetadata.tracks) {
        record_track_metadata halTrackMetadata{.gain = metadata.gain};
        CONVERT_CHECKED(HidlUtils::audioSourceToHal(metadata.source, &halTrackMetadata.source),
                        result);
#if MAJOR_VERSION >= 5
        if (metadata.destination.getDiscriminator() ==
            RecordTrackMetadata::Destination::hidl_discriminator::device) {
            CONVERT_CHECKED(
                    deviceAddressToHal(metadata.destination.device(), &halTrackMetadata.dest_device,
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

status_t sourceMetadataToHal(const SourceMetadata& sourceMetadata,
                             std::vector<playback_track_metadata_t>* halTracks) {
    status_t result = NO_ERROR;
    if (halTracks != nullptr) {
        halTracks->reserve(sourceMetadata.tracks.size());
    }
    for (auto& metadata : sourceMetadata.tracks) {
        playback_track_metadata_t halTrackMetadata{.gain = metadata.gain};
        CONVERT_CHECKED(HidlUtils::audioUsageToHal(metadata.usage, &halTrackMetadata.usage),
                        result);
        CONVERT_CHECKED(HidlUtils::audioContentTypeToHal(metadata.contentType,
                                                         &halTrackMetadata.content_type),
                        result);
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

bool audioInputFlagsToHal(const hidl_vec<AudioInOutFlag>& flags, audio_input_flags_t* halFlags) {
    bool success = true;
    *halFlags = {};
    for (const auto& flag : flags) {
        audio_input_flags_t halFlag;
        if (!xsd::isUnknownAudioInOutFlag(flag) &&
            audio_input_flag_from_string(flag.c_str(), &halFlag)) {
            *halFlags = static_cast<audio_input_flags_t>(*halFlags | halFlag);
        } else {
            ALOGE("Unknown audio input flag \"%s\"", flag.c_str());
            success = false;
        }
    }
    return success;
}

bool audioOutputFlagsToHal(const hidl_vec<AudioInOutFlag>& flags, audio_output_flags_t* halFlags) {
    bool success = true;
    *halFlags = {};
    for (const auto& flag : flags) {
        audio_output_flags_t halFlag;
        if (!xsd::isUnknownAudioInOutFlag(flag) &&
            audio_output_flag_from_string(flag.c_str(), &halFlag)) {
            *halFlags = static_cast<audio_output_flags_t>(*halFlags | halFlag);
        } else {
            ALOGE("Unknown audio output flag \"%s\"", flag.c_str());
            success = false;
        }
    }
    return success;
}

status_t sinkMetadataToHalV7(const SinkMetadata& sinkMetadata,
                             std::vector<record_track_metadata_v7_t>* halTracks) {
    std::vector<record_track_metadata> bases;
    status_t result = sinkMetadataToHal(sinkMetadata, halTracks != nullptr ? &bases : nullptr);
    if (halTracks != nullptr) {
        halTracks->reserve(bases.size());
    }
    auto baseIter = std::make_move_iterator(bases.begin());
    for (auto& metadata : sinkMetadata.tracks) {
        record_track_metadata_v7_t halTrackMetadata;
        CONVERT_CHECKED(HidlUtils::audioChannelMaskToHal(metadata.channelMask,
                                                         &halTrackMetadata.channel_mask),
                        result);
        CONVERT_CHECKED(HidlUtils::audioTagsToHal(metadata.tags, halTrackMetadata.tags), result);
        if (halTracks != nullptr) {
            halTrackMetadata.base = std::move(*baseIter++);
            halTracks->push_back(std::move(halTrackMetadata));
        }
    }
    return result;
}

status_t sourceMetadataToHalV7(const SourceMetadata& sourceMetadata,
                               std::vector<playback_track_metadata_v7_t>* halTracks) {
    std::vector<playback_track_metadata_t> bases;
    status_t result = sourceMetadataToHal(sourceMetadata, halTracks != nullptr ? &bases : nullptr);
    if (halTracks != nullptr) {
        halTracks->reserve(bases.size());
    }
    auto baseIter = std::make_move_iterator(bases.begin());
    for (auto& metadata : sourceMetadata.tracks) {
        playback_track_metadata_v7_t halTrackMetadata;
        CONVERT_CHECKED(HidlUtils::audioChannelMaskToHal(metadata.channelMask,
                                                         &halTrackMetadata.channel_mask),
                        result);
        CONVERT_CHECKED(HidlUtils::audioTagsToHal(metadata.tags, halTrackMetadata.tags), result);
        if (halTracks != nullptr) {
            halTrackMetadata.base = std::move(*baseIter++);
            halTracks->push_back(std::move(halTrackMetadata));
        }
    }
    return result;
}
#endif

}  // namespace implementation
}  // namespace CPP_VERSION
}  // namespace audio
}  // namespace hardware
}  // namespace android
