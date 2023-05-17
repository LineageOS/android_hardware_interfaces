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

#include <Utils.h>
#include <aidl/android/media/audio/common/AudioChannelLayout.h>
#include <aidl/android/media/audio/common/AudioDeviceType.h>
#include <aidl/android/media/audio/common/AudioFormatDescription.h>
#include <aidl/android/media/audio/common/AudioFormatType.h>
#include <aidl/android/media/audio/common/AudioIoFlags.h>
#include <aidl/android/media/audio/common/AudioOutputFlags.h>
#include <media/stagefright/foundation/MediaDefs.h>

#include "core-impl/Configuration.h"

using aidl::android::hardware::audio::common::makeBitPositionFlagMask;
using aidl::android::media::audio::common::AudioChannelLayout;
using aidl::android::media::audio::common::AudioDeviceDescription;
using aidl::android::media::audio::common::AudioDeviceType;
using aidl::android::media::audio::common::AudioFormatDescription;
using aidl::android::media::audio::common::AudioFormatType;
using aidl::android::media::audio::common::AudioGainConfig;
using aidl::android::media::audio::common::AudioIoFlags;
using aidl::android::media::audio::common::AudioOutputFlags;
using aidl::android::media::audio::common::AudioPort;
using aidl::android::media::audio::common::AudioPortConfig;
using aidl::android::media::audio::common::AudioPortDeviceExt;
using aidl::android::media::audio::common::AudioPortExt;
using aidl::android::media::audio::common::AudioPortMixExt;
using aidl::android::media::audio::common::AudioProfile;
using aidl::android::media::audio::common::Int;
using aidl::android::media::audio::common::MicrophoneInfo;
using aidl::android::media::audio::common::PcmType;

namespace aidl::android::hardware::audio::core::internal {

static void fillProfile(AudioProfile* profile, const std::vector<int32_t>& channelLayouts,
                        const std::vector<int32_t>& sampleRates) {
    for (auto layout : channelLayouts) {
        profile->channelMasks.push_back(
                AudioChannelLayout::make<AudioChannelLayout::layoutMask>(layout));
    }
    profile->sampleRates.insert(profile->sampleRates.end(), sampleRates.begin(), sampleRates.end());
}

static AudioProfile createProfile(PcmType pcmType, const std::vector<int32_t>& channelLayouts,
                                  const std::vector<int32_t>& sampleRates) {
    AudioProfile profile;
    profile.format.type = AudioFormatType::PCM;
    profile.format.pcm = pcmType;
    fillProfile(&profile, channelLayouts, sampleRates);
    return profile;
}

static AudioProfile createProfile(const std::string& encodingType,
                                  const std::vector<int32_t>& channelLayouts,
                                  const std::vector<int32_t>& sampleRates) {
    AudioProfile profile;
    profile.format.encoding = encodingType;
    fillProfile(&profile, channelLayouts, sampleRates);
    return profile;
}

static AudioPortExt createDeviceExt(AudioDeviceType devType, int32_t flags,
                                    std::string connection = "") {
    AudioPortDeviceExt deviceExt;
    deviceExt.device.type.type = devType;
    if (devType == AudioDeviceType::IN_MICROPHONE && connection.empty()) {
        deviceExt.device.address = "bottom";
    } else if (devType == AudioDeviceType::IN_MICROPHONE_BACK && connection.empty()) {
        deviceExt.device.address = "back";
    } else if (devType == AudioDeviceType::IN_SUBMIX || devType == AudioDeviceType::OUT_SUBMIX) {
        deviceExt.device.address = "0";
    }
    deviceExt.device.type.connection = std::move(connection);
    deviceExt.flags = flags;
    return AudioPortExt::make<AudioPortExt::Tag::device>(deviceExt);
}

static AudioPortExt createPortMixExt(int32_t maxOpenStreamCount, int32_t maxActiveStreamCount) {
    AudioPortMixExt mixExt;
    mixExt.maxOpenStreamCount = maxOpenStreamCount;
    mixExt.maxActiveStreamCount = maxActiveStreamCount;
    return AudioPortExt::make<AudioPortExt::Tag::mix>(mixExt);
}

static AudioPort createPort(int32_t id, const std::string& name, int32_t flags, bool isInput,
                            const AudioPortExt& ext) {
    AudioPort port;
    port.id = id;
    port.name = name;
    port.flags = isInput ? AudioIoFlags::make<AudioIoFlags::Tag::input>(flags)
                         : AudioIoFlags::make<AudioIoFlags::Tag::output>(flags);
    port.ext = ext;
    return port;
}

static AudioPortConfig createPortConfig(int32_t id, int32_t portId, PcmType pcmType, int32_t layout,
                                        int32_t sampleRate, int32_t flags, bool isInput,
                                        const AudioPortExt& ext) {
    AudioPortConfig config;
    config.id = id;
    config.portId = portId;
    config.sampleRate = Int{.value = sampleRate};
    config.channelMask = AudioChannelLayout::make<AudioChannelLayout::layoutMask>(layout);
    config.format = AudioFormatDescription{.type = AudioFormatType::PCM, .pcm = pcmType};
    config.gain = AudioGainConfig();
    config.flags = isInput ? AudioIoFlags::make<AudioIoFlags::Tag::input>(flags)
                           : AudioIoFlags::make<AudioIoFlags::Tag::output>(flags);
    config.ext = ext;
    return config;
}

static AudioRoute createRoute(const std::vector<AudioPort>& sources, const AudioPort& sink) {
    AudioRoute route;
    route.sinkPortId = sink.id;
    std::transform(sources.begin(), sources.end(), std::back_inserter(route.sourcePortIds),
                   [](const auto& port) { return port.id; });
    return route;
}

// Primary (default) configuration:
//
// Device ports:
//  * "Speaker", OUT_SPEAKER, default
//    - no profiles specified
//  * "Built-in Mic", IN_MICROPHONE, default
//    - no profiles specified
//  * "Telephony Tx", OUT_TELEPHONY_TX
//    - no profiles specified
//  * "Telephony Rx", IN_TELEPHONY_RX
//    - no profiles specified
//  * "FM Tuner", IN_FM_TUNER
//    - no profiles specified
//  * "USB Out", OUT_DEVICE, CONNECTION_USB
//    - no profiles specified
//  * "USB In", IN_DEVICE, CONNECTION_USB
//    - no profiles specified
//
// Mix ports:
//  * "primary output", PRIMARY, 1 max open, 1 max active stream
//    - profile PCM 16-bit; MONO, STEREO; 8000, 11025, 16000, 32000, 44100, 48000
//    - profile PCM 24-bit; MONO, STEREO; 8000, 11025, 16000, 32000, 44100, 48000
//  * "compressed offload", DIRECT|COMPRESS_OFFLOAD|NON_BLOCKING, 1 max open, 1 max active stream
//    - profile MP3; MONO, STEREO; 44100, 48000
//  * "primary input", 2 max open, 2 max active streams
//    - profile PCM 16-bit; MONO, STEREO, FRONT_BACK;
//        8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000
//    - profile PCM 24-bit; MONO, STEREO, FRONT_BACK;
//        8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000
//  * "telephony_tx", 1 max open, 1 max active stream
//    - profile PCM 16-bit; MONO, STEREO; 8000, 11025, 16000, 32000, 44100, 48000
//    - profile PCM 24-bit; MONO, STEREO; 8000, 11025, 16000, 32000, 44100, 48000
//  * "telephony_rx", 1 max open, 1 max active stream
//    - profile PCM 16-bit; MONO, STEREO; 8000, 11025, 16000, 32000, 44100, 48000
//    - profile PCM 24-bit; MONO, STEREO; 8000, 11025, 16000, 32000, 44100, 48000
//  * "fm_tuner", 1 max open, 1 max active stream
//    - profile PCM 16-bit; MONO, STEREO; 8000, 11025, 16000, 32000, 44100, 48000
//    - profile PCM 24-bit; MONO, STEREO; 8000, 11025, 16000, 32000, 44100, 48000
//
// Routes:
//  "primary out", "compressed offload" -> "Speaker"
//  "primary out", "compressed offload" -> "USB Out"
//  "Built-in Mic", "USB In" -> "primary input"
//  "telephony_tx" -> "Telephony Tx"
//  "Telephony Rx" -> "telephony_rx"
//  "FM Tuner" -> "fm_tuner"
//
// Initial port configs:
//  * "Speaker" device port: PCM 24-bit; STEREO; 48000
//  * "Built-in Mic" device port: PCM 24-bit; MONO; 48000
//  * "Telephony Tx" device port: PCM 24-bit; MONO; 48000
//  * "Telephony Rx" device port: PCM 24-bit; MONO; 48000
//  * "FM Tuner" device port: PCM 24-bit; STEREO; 48000
//
// Profiles for device port connected state:
//  * USB Out":
//    - profile PCM 16-bit; MONO, STEREO; 8000, 11025, 16000, 32000, 44100, 48000
//    - profile PCM 24-bit; MONO, STEREO; 8000, 11025, 16000, 32000, 44100, 48000
//  * USB In":
//    - profile PCM 16-bit; MONO, STEREO; 8000, 11025, 16000, 32000, 44100, 48000
//    - profile PCM 24-bit; MONO, STEREO; 8000, 11025, 16000, 32000, 44100, 48000
//
std::unique_ptr<Configuration> getPrimaryConfiguration() {
    static const Configuration configuration = []() {
        const std::vector<AudioProfile> standardPcmAudioProfiles = {
                createProfile(PcmType::INT_16_BIT,
                              {AudioChannelLayout::LAYOUT_MONO, AudioChannelLayout::LAYOUT_STEREO},
                              {8000, 11025, 16000, 32000, 44100, 48000}),
                createProfile(PcmType::INT_24_BIT,
                              {AudioChannelLayout::LAYOUT_MONO, AudioChannelLayout::LAYOUT_STEREO},
                              {8000, 11025, 16000, 32000, 44100, 48000})};
        Configuration c;

        // Device ports

        AudioPort speakerOutDevice =
                createPort(c.nextPortId++, "Speaker", 0, false,
                           createDeviceExt(AudioDeviceType::OUT_SPEAKER,
                                           1 << AudioPortDeviceExt::FLAG_INDEX_DEFAULT_DEVICE));
        c.ports.push_back(speakerOutDevice);
        c.initialConfigs.push_back(
                createPortConfig(speakerOutDevice.id, speakerOutDevice.id, PcmType::INT_24_BIT,
                                 AudioChannelLayout::LAYOUT_STEREO, 48000, 0, false,
                                 createDeviceExt(AudioDeviceType::OUT_SPEAKER, 0)));

        AudioPort micInDevice =
                createPort(c.nextPortId++, "Built-in Mic", 0, true,
                           createDeviceExt(AudioDeviceType::IN_MICROPHONE,
                                           1 << AudioPortDeviceExt::FLAG_INDEX_DEFAULT_DEVICE));
        c.ports.push_back(micInDevice);
        c.initialConfigs.push_back(
                createPortConfig(micInDevice.id, micInDevice.id, PcmType::INT_24_BIT,
                                 AudioChannelLayout::LAYOUT_MONO, 48000, 0, true,
                                 createDeviceExt(AudioDeviceType::IN_MICROPHONE, 0)));

        AudioPort telephonyTxOutDevice =
                createPort(c.nextPortId++, "Telephony Tx", 0, false,
                           createDeviceExt(AudioDeviceType::OUT_TELEPHONY_TX, 0));
        c.ports.push_back(telephonyTxOutDevice);
        c.initialConfigs.push_back(
                createPortConfig(telephonyTxOutDevice.id, telephonyTxOutDevice.id,
                                 PcmType::INT_24_BIT, AudioChannelLayout::LAYOUT_MONO, 48000, 0,
                                 false, createDeviceExt(AudioDeviceType::OUT_TELEPHONY_TX, 0)));

        AudioPort telephonyRxInDevice =
                createPort(c.nextPortId++, "Telephony Rx", 0, true,
                           createDeviceExt(AudioDeviceType::IN_TELEPHONY_RX, 0));
        c.ports.push_back(telephonyRxInDevice);
        c.initialConfigs.push_back(
                createPortConfig(telephonyRxInDevice.id, telephonyRxInDevice.id,
                                 PcmType::INT_24_BIT, AudioChannelLayout::LAYOUT_MONO, 48000, 0,
                                 true, createDeviceExt(AudioDeviceType::IN_TELEPHONY_RX, 0)));

        AudioPort fmTunerInDevice = createPort(c.nextPortId++, "FM Tuner", 0, true,
                                               createDeviceExt(AudioDeviceType::IN_FM_TUNER, 0));
        c.ports.push_back(fmTunerInDevice);
        c.initialConfigs.push_back(
                createPortConfig(fmTunerInDevice.id, fmTunerInDevice.id, PcmType::INT_24_BIT,
                                 AudioChannelLayout::LAYOUT_STEREO, 48000, 0, true,
                                 createDeviceExt(AudioDeviceType::IN_FM_TUNER, 0)));

        AudioPort usbOutDevice =
                createPort(c.nextPortId++, "USB Out", 0, false,
                           createDeviceExt(AudioDeviceType::OUT_DEVICE, 0,
                                           AudioDeviceDescription::CONNECTION_USB));
        c.ports.push_back(usbOutDevice);
        c.connectedProfiles[usbOutDevice.id] = standardPcmAudioProfiles;

        AudioPort usbInDevice = createPort(c.nextPortId++, "USB In", 0, true,
                                           createDeviceExt(AudioDeviceType::IN_DEVICE, 0,
                                                           AudioDeviceDescription::CONNECTION_USB));
        c.ports.push_back(usbInDevice);
        c.connectedProfiles[usbInDevice.id] = standardPcmAudioProfiles;

        // Mix ports

        AudioPort primaryOutMix = createPort(c.nextPortId++, "primary output",
                                             makeBitPositionFlagMask(AudioOutputFlags::PRIMARY),
                                             false, createPortMixExt(1, 1));
        primaryOutMix.profiles.insert(primaryOutMix.profiles.begin(),
                                      standardPcmAudioProfiles.begin(),
                                      standardPcmAudioProfiles.end());
        c.ports.push_back(primaryOutMix);

        AudioPort compressedOffloadOutMix =
                createPort(c.nextPortId++, "compressed offload",
                           makeBitPositionFlagMask({AudioOutputFlags::DIRECT,
                                                    AudioOutputFlags::COMPRESS_OFFLOAD,
                                                    AudioOutputFlags::NON_BLOCKING}),
                           false, createPortMixExt(1, 1));
        compressedOffloadOutMix.profiles.push_back(
                createProfile(::android::MEDIA_MIMETYPE_AUDIO_MPEG,
                              {AudioChannelLayout::LAYOUT_MONO, AudioChannelLayout::LAYOUT_STEREO},
                              {44100, 48000}));
        c.ports.push_back(compressedOffloadOutMix);

        AudioPort primaryInMix =
                createPort(c.nextPortId++, "primary input", 0, true, createPortMixExt(2, 2));
        primaryInMix.profiles.push_back(
                createProfile(PcmType::INT_16_BIT,
                              {AudioChannelLayout::LAYOUT_MONO, AudioChannelLayout::LAYOUT_STEREO,
                               AudioChannelLayout::LAYOUT_FRONT_BACK},
                              {8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000}));
        primaryInMix.profiles.push_back(
                createProfile(PcmType::INT_24_BIT,
                              {AudioChannelLayout::LAYOUT_MONO, AudioChannelLayout::LAYOUT_STEREO,
                               AudioChannelLayout::LAYOUT_FRONT_BACK},
                              {8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000}));
        c.ports.push_back(primaryInMix);

        AudioPort telephonyTxOutMix =
                createPort(c.nextPortId++, "telephony_tx", 0, false, createPortMixExt(1, 1));
        telephonyTxOutMix.profiles.insert(telephonyTxOutMix.profiles.begin(),
                                          standardPcmAudioProfiles.begin(),
                                          standardPcmAudioProfiles.end());
        c.ports.push_back(telephonyTxOutMix);

        AudioPort telephonyRxInMix =
                createPort(c.nextPortId++, "telephony_rx", 0, true, createPortMixExt(1, 1));
        telephonyRxInMix.profiles.insert(telephonyRxInMix.profiles.begin(),
                                         standardPcmAudioProfiles.begin(),
                                         standardPcmAudioProfiles.end());
        c.ports.push_back(telephonyRxInMix);

        AudioPort fmTunerInMix =
                createPort(c.nextPortId++, "fm_tuner", 0, true, createPortMixExt(1, 1));
        fmTunerInMix.profiles.insert(fmTunerInMix.profiles.begin(),
                                     standardPcmAudioProfiles.begin(),
                                     standardPcmAudioProfiles.end());
        c.ports.push_back(fmTunerInMix);

        c.routes.push_back(createRoute({primaryOutMix, compressedOffloadOutMix}, speakerOutDevice));
        c.routes.push_back(createRoute({primaryOutMix, compressedOffloadOutMix}, usbOutDevice));
        c.routes.push_back(createRoute({micInDevice, usbInDevice}, primaryInMix));
        c.routes.push_back(createRoute({telephonyTxOutMix}, telephonyTxOutDevice));
        c.routes.push_back(createRoute({telephonyRxInDevice}, telephonyRxInMix));
        c.routes.push_back(createRoute({fmTunerInDevice}, fmTunerInMix));

        c.portConfigs.insert(c.portConfigs.end(), c.initialConfigs.begin(), c.initialConfigs.end());

        MicrophoneInfo mic;
        mic.id = "mic";
        mic.device = micInDevice.ext.get<AudioPortExt::Tag::device>().device;
        mic.group = 0;
        mic.indexInTheGroup = 0;
        c.microphones = std::vector<MicrophoneInfo>{mic};

        return c;
    }();
    return std::make_unique<Configuration>(configuration);
}

// Remote Submix configuration:
//
// Device ports:
//  * "Remote Submix Out", OUT_SUBMIX
//    - profile PCM 24-bit; STEREO; 48000
//  * "Remote Submix In", IN_SUBMIX
//    - profile PCM 24-bit; STEREO; 48000
//
// Mix ports:
//  * "r_submix output", stream count unlimited
//    - profile PCM 24-bit; STEREO; 48000
//  * "r_submix input", stream count unlimited
//    - profile PCM 24-bit; STEREO; 48000
//
// Routes:
//  "r_submix output" -> "Remote Submix Out"
//  "Remote Submix In" -> "r_submix input"
//
std::unique_ptr<Configuration> getRSubmixConfiguration() {
    static const Configuration configuration = []() {
        Configuration c;

        // Device ports

        AudioPort rsubmixOutDevice =
                createPort(c.nextPortId++, "Remote Submix Out", 0, false,
                           createDeviceExt(AudioDeviceType::OUT_SUBMIX, 0,
                                           AudioDeviceDescription::CONNECTION_VIRTUAL));
        rsubmixOutDevice.profiles.push_back(
                createProfile(PcmType::INT_24_BIT, {AudioChannelLayout::LAYOUT_STEREO}, {48000}));
        c.ports.push_back(rsubmixOutDevice);

        AudioPort rsubmixInDevice = createPort(c.nextPortId++, "Remote Submix In", 0, true,
                                               createDeviceExt(AudioDeviceType::IN_SUBMIX, 0));
        rsubmixInDevice.profiles.push_back(
                createProfile(PcmType::INT_24_BIT, {AudioChannelLayout::LAYOUT_STEREO}, {48000}));
        c.ports.push_back(rsubmixInDevice);

        // Mix ports

        AudioPort rsubmixOutMix =
                createPort(c.nextPortId++, "r_submix output", 0, false, createPortMixExt(0, 0));
        rsubmixOutMix.profiles.push_back(
                createProfile(PcmType::INT_24_BIT, {AudioChannelLayout::LAYOUT_STEREO}, {48000}));
        c.ports.push_back(rsubmixOutMix);

        AudioPort rsubmixInMix =
                createPort(c.nextPortId++, "r_submix input", 0, true, createPortMixExt(0, 0));
        rsubmixInMix.profiles.push_back(
                createProfile(PcmType::INT_24_BIT, {AudioChannelLayout::LAYOUT_STEREO}, {48000}));
        c.ports.push_back(rsubmixInMix);

        c.routes.push_back(createRoute({rsubmixOutMix}, rsubmixOutDevice));
        c.routes.push_back(createRoute({rsubmixInDevice}, rsubmixInMix));

        return c;
    }();
    return std::make_unique<Configuration>(configuration);
}

// Usb configuration:
//
// Device ports:
//  * "USB Headset Out", OUT_HEADSET, CONNECTION_USB
//    - no profiles specified
//  * "USB Headset In", IN_HEADSET, CONNECTION_USB
//    - no profiles specified
//
// Mix ports:
//  * "usb_headset output", 1 max open, 1 max active stream
//    - no profiles specified
//  * "usb_headset input", 1 max open, 1 max active stream
//    - no profiles specified
//
// Profiles for device port connected state:
//  * USB Headset Out":
//    - profile PCM 16-bit; MONO, STEREO, INDEX_MASK_1, INDEX_MASK_2; 44100, 48000
//    - profile PCM 24-bit; MONO, STEREO, INDEX_MASK_1, INDEX_MASK_2; 44100, 48000
//  * USB Headset In":
//    - profile PCM 16-bit; MONO, STEREO, INDEX_MASK_1, INDEX_MASK_2; 44100, 48000
//    - profile PCM 24-bit; MONO, STEREO, INDEX_MASK_1, INDEX_MASK_2; 44100, 48000
//
std::unique_ptr<Configuration> getUsbConfiguration() {
    static const Configuration configuration = []() {
        const std::vector<AudioProfile> standardPcmAudioProfiles = {
                createProfile(PcmType::INT_16_BIT,
                              {AudioChannelLayout::LAYOUT_MONO, AudioChannelLayout::LAYOUT_STEREO,
                               AudioChannelLayout::INDEX_MASK_1, AudioChannelLayout::INDEX_MASK_2},
                              {44100, 48000}),
                createProfile(PcmType::INT_24_BIT,
                              {AudioChannelLayout::LAYOUT_MONO, AudioChannelLayout::LAYOUT_STEREO,
                               AudioChannelLayout::INDEX_MASK_1, AudioChannelLayout::INDEX_MASK_2},
                              {44100, 48000})};
        Configuration c;

        // Device ports

        AudioPort usbOutHeadset =
                createPort(c.nextPortId++, "USB Headset Out", 0, false,
                           createDeviceExt(AudioDeviceType::OUT_HEADSET, 0,
                                           AudioDeviceDescription::CONNECTION_USB));
        c.ports.push_back(usbOutHeadset);
        c.connectedProfiles[usbOutHeadset.id] = standardPcmAudioProfiles;

        AudioPort usbInHeadset =
                createPort(c.nextPortId++, "USB Headset In", 0, true,
                           createDeviceExt(AudioDeviceType::IN_HEADSET, 0,
                                           AudioDeviceDescription::CONNECTION_USB));
        c.ports.push_back(usbInHeadset);
        c.connectedProfiles[usbInHeadset.id] = standardPcmAudioProfiles;

        // Mix ports

        AudioPort usbHeadsetOutMix =
                createPort(c.nextPortId++, "usb_headset output", 0, false, createPortMixExt(1, 1));
        c.ports.push_back(usbHeadsetOutMix);

        AudioPort usbHeadsetInMix =
                createPort(c.nextPortId++, "usb_headset input", 0, true, createPortMixExt(1, 1));
        c.ports.push_back(usbHeadsetInMix);

        c.routes.push_back(createRoute({usbHeadsetOutMix}, usbOutHeadset));
        c.routes.push_back(createRoute({usbInHeadset}, usbHeadsetInMix));

        return c;
    }();
    return std::make_unique<Configuration>(configuration);
}

}  // namespace aidl::android::hardware::audio::core::internal
