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

#pragma once

// clang-format off
#include PATH(android/hardware/audio/FILE_VERSION/types.h)
// clang-format off

#include <vector>

#include <system/audio.h>

#include <common/all-versions/VersionUtils.h>
#include <VersionUtils.h>

namespace android {
namespace hardware {
namespace audio {
namespace CPP_VERSION {
namespace implementation {

using ::android::hardware::audio::common::utils::EnumBitfield;
using ::android::hardware::hidl_vec;
using namespace ::android::hardware::audio::common::CPP_VERSION;
using namespace ::android::hardware::audio::CPP_VERSION;

struct CoreUtils {
    // Note: the converters for DeviceAddress have to be in CoreUtils for HAL V4
    // because DeviceAddress used to be defined in the core HAL. For V5 and above
    // these functions simply delegate to HidlUtils.
    static status_t deviceAddressToHal(const DeviceAddress& device, audio_devices_t* halDeviceType,
            char* halDeviceAddress);
    static status_t deviceAddressFromHal(audio_devices_t halDeviceType, const char* halDeviceAddress,
            DeviceAddress* device);
#if MAJOR_VERSION >= 4
    static status_t microphoneInfoFromHal(const struct audio_microphone_characteristic_t& halMicInfo,
            MicrophoneInfo* micInfo);
    static status_t microphoneInfoToHal(const MicrophoneInfo& micInfo, audio_microphone_characteristic_t* halMicInfo);
    // Note: {Sink|Source}Metadata types are defined in 'common' (since V5), so they can be used
    // by the BT HAL. However, the converters are defined here, not in HidlUtils to avoid adding
    // conditionals to handle V4. The converters are only used by 'core' HAL anyways.
    static status_t sinkMetadataFromHal(const std::vector<record_track_metadata_t>& halTracks,
            SinkMetadata* sinkMetadata);
    static status_t sinkMetadataFromHalV7(const std::vector<record_track_metadata_v7_t>& halTracks,
            bool ignoreNonVendorTags, SinkMetadata* sinkMetadata);
    static status_t sinkMetadataToHal(const SinkMetadata& sinkMetadata,
            std::vector<record_track_metadata_t>* halTracks);
    static status_t sinkMetadataToHalV7(const SinkMetadata& sinkMetadata, bool ignoreNonVendorTags,
            std::vector<record_track_metadata_v7_t>* halTracks);
    static status_t sourceMetadataFromHal(const std::vector<playback_track_metadata_t>& halTracks,
            SourceMetadata* sourceMetadata);
    static status_t sourceMetadataFromHalV7(const std::vector<playback_track_metadata_v7_t>& halTracks,
            bool ignoreNonVendorTags, SourceMetadata* sourceMetadata);
    static status_t sourceMetadataToHal(const SourceMetadata& sourceMetadata,
            std::vector<playback_track_metadata_t>* halTracks);
    static status_t sourceMetadataToHalV7(const SourceMetadata& sourceMetadata, bool ignoreNonVendorTags,
            std::vector<playback_track_metadata_v7_t>* halTracks);
#endif

#if MAJOR_VERSION <= 6
    using AudioInputFlags =
            ::android::hardware::audio::common::CPP_VERSION::implementation::AudioInputFlagBitfield;
    using AudioOutputFlags =
            ::android::hardware::audio::common::CPP_VERSION::implementation::AudioOutputFlagBitfield;
    static inline status_t audioInputFlagsFromHal(audio_input_flags_t halFlagMask, AudioInputFlags* flags) {
        *flags = EnumBitfield<AudioInputFlag>(halFlagMask);
        return NO_ERROR;
    }
    static inline status_t audioInputFlagsToHal(AudioInputFlags flags, audio_input_flags_t* halFlagMask) {
        *halFlagMask = static_cast<audio_input_flags_t>(flags);
        return NO_ERROR;
    }
    static inline status_t audioOutputFlagsFromHal(audio_output_flags_t halFlagMask, AudioOutputFlags* flags) {
        *flags = EnumBitfield<AudioOutputFlag>(halFlagMask);
        return NO_ERROR;
    }
    static inline status_t audioOutputFlagsToHal(AudioOutputFlags flags, audio_output_flags_t* halFlagMask) {
        *halFlagMask = static_cast<audio_output_flags_t>(flags);
        return NO_ERROR;
    }
#else
    using AudioInputFlags = hidl_vec<::android::hardware::audio::CPP_VERSION::AudioInOutFlag>;
    using AudioOutputFlags = hidl_vec<::android::hardware::audio::CPP_VERSION::AudioInOutFlag>;
    static status_t audioInputFlagsFromHal(audio_input_flags_t halFlagMask, AudioInputFlags* flags);
    static status_t audioInputFlagsToHal(const AudioInputFlags& flags, audio_input_flags_t* halFlagMask);
    static status_t audioOutputFlagsFromHal(audio_output_flags_t halFlagMask, AudioOutputFlags* flags);
    static status_t audioOutputFlagsToHal(const AudioOutputFlags& flags, audio_output_flags_t* halFlagMask);
#endif

};

}  // namespace implementation
}  // namespace CPP_VERSION
}  // namespace audio
}  // namespace hardware
}  // namespace android
