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

#ifndef ANDROID_HARDWARE_AUDIO_CONVERSIONS_H_
#define ANDROID_HARDWARE_AUDIO_CONVERSIONS_H_

#include PATH(android/hardware/audio/FILE_VERSION/types.h)

#include <string>

#include <system/audio.h>

#include <VersionUtils.h>

namespace android {
namespace hardware {
namespace audio {
namespace CPP_VERSION {
namespace implementation {

using ::android::hardware::hidl_vec;
using namespace ::android::hardware::audio::common::CPP_VERSION;
using namespace ::android::hardware::audio::CPP_VERSION;

status_t deviceAddressToHal(const DeviceAddress& device, audio_devices_t* halDeviceType,
                            char* halDeviceAddress);
status_t deviceAddressFromHal(audio_devices_t halDeviceType, const char* halDeviceAddress,
                              DeviceAddress* device);

#if MAJOR_VERSION >= 4
bool halToMicrophoneCharacteristics(MicrophoneInfo* pDst,
                                    const struct audio_microphone_characteristic_t& src);
status_t sinkMetadataToHal(const SinkMetadata& sinkMetadata,
                           std::vector<record_track_metadata>* halTracks);
status_t sourceMetadataToHal(const SourceMetadata& sourceMetadata,
                             std::vector<playback_track_metadata_t>* halTracks);
#endif

#if MAJOR_VERSION <= 6
using AudioInputFlags =
        ::android::hardware::audio::common::CPP_VERSION::implementation::AudioInputFlagBitfield;
using AudioOutputFlags =
        ::android::hardware::audio::common::CPP_VERSION::implementation::AudioOutputFlagBitfield;

inline bool audioInputFlagsToHal(AudioInputFlags flags, audio_input_flags_t* halFlags) {
    *halFlags = static_cast<audio_input_flags_t>(flags);
    return true;
}

inline bool audioOutputFlagsToHal(AudioOutputFlags flags, audio_output_flags_t* halFlags) {
    *halFlags = static_cast<audio_output_flags_t>(flags);
    return true;
}
#else
bool audioInputFlagsToHal(const hidl_vec<AudioInOutFlag>& flags, audio_input_flags_t* halFlags);
bool audioOutputFlagsToHal(const hidl_vec<AudioInOutFlag>& flags, audio_output_flags_t* halFlags);
// Overloading isn't convenient when passing a nullptr.
status_t sinkMetadataToHalV7(const SinkMetadata& sinkMetadata,
                             std::vector<record_track_metadata_v7_t>* halTracks);
status_t sourceMetadataToHalV7(const SourceMetadata& sourceMetadata,
                               std::vector<playback_track_metadata_v7_t>* halTracks);
#endif

}  // namespace implementation
}  // namespace CPP_VERSION
}  // namespace audio
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_AUDIO_CONVERSIONS_H_
