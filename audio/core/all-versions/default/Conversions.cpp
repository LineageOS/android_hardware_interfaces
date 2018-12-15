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

#include <log/log.h>

namespace android {
namespace hardware {
namespace audio {
namespace CPP_VERSION {
namespace implementation {

std::string deviceAddressToHal(const DeviceAddress& address) {
    // HAL assumes that the address is NUL-terminated.
    char halAddress[AUDIO_DEVICE_MAX_ADDRESS_LEN];
    memset(halAddress, 0, sizeof(halAddress));
    uint32_t halDevice = static_cast<uint32_t>(address.device);
    const bool isInput = (halDevice & AUDIO_DEVICE_BIT_IN) != 0;
    if (isInput) halDevice &= ~AUDIO_DEVICE_BIT_IN;
    if ((!isInput && (halDevice & AUDIO_DEVICE_OUT_ALL_A2DP) != 0) ||
        (isInput && (halDevice & AUDIO_DEVICE_IN_BLUETOOTH_A2DP) != 0)) {
        snprintf(halAddress, sizeof(halAddress), "%02X:%02X:%02X:%02X:%02X:%02X",
                 address.address.mac[0], address.address.mac[1], address.address.mac[2],
                 address.address.mac[3], address.address.mac[4], address.address.mac[5]);
    } else if ((!isInput && (halDevice & AUDIO_DEVICE_OUT_IP) != 0) ||
               (isInput && (halDevice & AUDIO_DEVICE_IN_IP) != 0)) {
        snprintf(halAddress, sizeof(halAddress), "%d.%d.%d.%d", address.address.ipv4[0],
                 address.address.ipv4[1], address.address.ipv4[2], address.address.ipv4[3]);
    } else if ((!isInput && (halDevice & AUDIO_DEVICE_OUT_ALL_USB) != 0) ||
               (isInput && (halDevice & AUDIO_DEVICE_IN_ALL_USB) != 0)) {
        snprintf(halAddress, sizeof(halAddress), "card=%d;device=%d", address.address.alsa.card,
                 address.address.alsa.device);
    } else if ((!isInput && (halDevice & AUDIO_DEVICE_OUT_BUS) != 0) ||
               (isInput && (halDevice & AUDIO_DEVICE_IN_BUS) != 0)) {
        snprintf(halAddress, sizeof(halAddress), "%s", address.busAddress.c_str());
    } else if ((!isInput && (halDevice & AUDIO_DEVICE_OUT_REMOTE_SUBMIX)) != 0 ||
               (isInput && (halDevice & AUDIO_DEVICE_IN_REMOTE_SUBMIX) != 0)) {
        snprintf(halAddress, sizeof(halAddress), "%s", address.rSubmixAddress.c_str());
    }
    return halAddress;
}

#if MAJOR_VERSION >= 4
status_t deviceAddressFromHal(audio_devices_t device, const char* halAddress,
                              DeviceAddress* address) {
    if (address == nullptr) {
        return BAD_VALUE;
    }
    address->device = AudioDevice(device);
    if (halAddress == nullptr || strnlen(halAddress, AUDIO_DEVICE_MAX_ADDRESS_LEN) == 0) {
        return OK;
    }

    const bool isInput = (device & AUDIO_DEVICE_BIT_IN) != 0;
    if (isInput) device &= ~AUDIO_DEVICE_BIT_IN;
    if ((!isInput && (device & AUDIO_DEVICE_OUT_ALL_A2DP) != 0) ||
        (isInput && (device & AUDIO_DEVICE_IN_BLUETOOTH_A2DP) != 0)) {
        int status =
            sscanf(halAddress, "%hhX:%hhX:%hhX:%hhX:%hhX:%hhX", &address->address.mac[0],
                   &address->address.mac[1], &address->address.mac[2], &address->address.mac[3],
                   &address->address.mac[4], &address->address.mac[5]);
        return status == 6 ? OK : BAD_VALUE;
    } else if ((!isInput && (device & AUDIO_DEVICE_OUT_IP) != 0) ||
               (isInput && (device & AUDIO_DEVICE_IN_IP) != 0)) {
        int status =
            sscanf(halAddress, "%hhu.%hhu.%hhu.%hhu", &address->address.ipv4[0],
                   &address->address.ipv4[1], &address->address.ipv4[2], &address->address.ipv4[3]);
        return status == 4 ? OK : BAD_VALUE;
    } else if ((!isInput && (device & AUDIO_DEVICE_OUT_ALL_USB)) != 0 ||
               (isInput && (device & AUDIO_DEVICE_IN_ALL_USB)) != 0) {
        int status = sscanf(halAddress, "card=%d;device=%d", &address->address.alsa.card,
                            &address->address.alsa.device);
        return status == 2 ? OK : BAD_VALUE;
    } else if ((!isInput && (device & AUDIO_DEVICE_OUT_BUS) != 0) ||
               (isInput && (device & AUDIO_DEVICE_IN_BUS) != 0)) {
        address->busAddress = halAddress;
        return OK;
    } else if ((!isInput && (device & AUDIO_DEVICE_OUT_REMOTE_SUBMIX)) != 0 ||
               (isInput && (device & AUDIO_DEVICE_IN_REMOTE_SUBMIX) != 0)) {
        address->rSubmixAddress = halAddress;
        return OK;
    }
    address->busAddress = halAddress;
    return OK;
}

AudioMicrophoneChannelMapping halToChannelMapping(audio_microphone_channel_mapping_t mapping) {
    switch (mapping) {
        case AUDIO_MICROPHONE_CHANNEL_MAPPING_UNUSED:
            return AudioMicrophoneChannelMapping::UNUSED;
        case AUDIO_MICROPHONE_CHANNEL_MAPPING_DIRECT:
            return AudioMicrophoneChannelMapping::DIRECT;
        case AUDIO_MICROPHONE_CHANNEL_MAPPING_PROCESSED:
            return AudioMicrophoneChannelMapping::PROCESSED;
        default:
            ALOGE("Invalid channel mapping type: %d", mapping);
            return AudioMicrophoneChannelMapping::UNUSED;
    }
}

AudioMicrophoneLocation halToLocation(audio_microphone_location_t location) {
    switch (location) {
        default:
        case AUDIO_MICROPHONE_LOCATION_UNKNOWN:
            return AudioMicrophoneLocation::UNKNOWN;
        case AUDIO_MICROPHONE_LOCATION_MAINBODY:
            return AudioMicrophoneLocation::MAINBODY;
        case AUDIO_MICROPHONE_LOCATION_MAINBODY_MOVABLE:
            return AudioMicrophoneLocation::MAINBODY_MOVABLE;
        case AUDIO_MICROPHONE_LOCATION_PERIPHERAL:
            return AudioMicrophoneLocation::PERIPHERAL;
    }
}

AudioMicrophoneDirectionality halToDirectionality(audio_microphone_directionality_t dir) {
    switch (dir) {
        default:
        case AUDIO_MICROPHONE_DIRECTIONALITY_UNKNOWN:
            return AudioMicrophoneDirectionality::UNKNOWN;
        case AUDIO_MICROPHONE_DIRECTIONALITY_OMNI:
            return AudioMicrophoneDirectionality::OMNI;
        case AUDIO_MICROPHONE_DIRECTIONALITY_BI_DIRECTIONAL:
            return AudioMicrophoneDirectionality::BI_DIRECTIONAL;
        case AUDIO_MICROPHONE_DIRECTIONALITY_CARDIOID:
            return AudioMicrophoneDirectionality::CARDIOID;
        case AUDIO_MICROPHONE_DIRECTIONALITY_HYPER_CARDIOID:
            return AudioMicrophoneDirectionality::HYPER_CARDIOID;
        case AUDIO_MICROPHONE_DIRECTIONALITY_SUPER_CARDIOID:
            return AudioMicrophoneDirectionality::SUPER_CARDIOID;
    }
}

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
            pDst->channelMapping[ch] = halToChannelMapping(src.channel_mapping[ch]);
        }
        pDst->location = halToLocation(src.location);
        pDst->group = (AudioMicrophoneGroup)src.group;
        pDst->indexInTheGroup = (uint32_t)src.index_in_the_group;
        pDst->sensitivity = src.sensitivity;
        pDst->maxSpl = src.max_spl;
        pDst->minSpl = src.min_spl;
        pDst->directionality = halToDirectionality(src.directionality);
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
#endif

}  // namespace implementation
}  // namespace CPP_VERSION
}  // namespace audio
}  // namespace hardware
}  // namespace android
