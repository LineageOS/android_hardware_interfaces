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
using aidl::android::media::audio::common::PcmType;
using Configuration = aidl::android::hardware::audio::core::Module::Configuration;

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

static AudioPortConfig createDynamicPortConfig(int32_t id, int32_t portId, int32_t flags,
                                               bool isInput, const AudioPortExt& ext) {
    AudioPortConfig config;
    config.id = id;
    config.portId = portId;
    config.gain = AudioGainConfig();
    config.flags = isInput ? AudioIoFlags::make<AudioIoFlags::Tag::input>(flags)
                           : AudioIoFlags::make<AudioIoFlags::Tag::output>(flags);
    config.ext = ext;
    return config;
}

static AudioPortConfig createPortConfig(int32_t id, int32_t portId, PcmType pcmType, int32_t layout,
                                        int32_t sampleRate, int32_t flags, bool isInput,
                                        const AudioPortExt& ext) {
    AudioPortConfig config = createDynamicPortConfig(id, portId, flags, isInput, ext);
    config.sampleRate = Int{.value = sampleRate};
    config.channelMask = AudioChannelLayout::make<AudioChannelLayout::layoutMask>(layout);
    config.format = AudioFormatDescription{.type = AudioFormatType::PCM, .pcm = pcmType};
    return config;
}

static AudioRoute createRoute(const std::vector<AudioPort>& sources, const AudioPort& sink) {
    AudioRoute route;
    route.sinkPortId = sink.id;
    std::transform(sources.begin(), sources.end(), std::back_inserter(route.sourcePortIds),
                   [](const auto& port) { return port.id; });
    return route;
}

std::vector<AudioProfile> getStandard16And24BitPcmAudioProfiles() {
    auto createStdPcmAudioProfile = [](const PcmType& pcmType) {
        return AudioProfile{
                .format = AudioFormatDescription{.type = AudioFormatType::PCM, .pcm = pcmType},
                .channelMasks = {AudioChannelLayout::make<AudioChannelLayout::layoutMask>(
                                         AudioChannelLayout::LAYOUT_MONO),
                                 AudioChannelLayout::make<AudioChannelLayout::layoutMask>(
                                         AudioChannelLayout::LAYOUT_STEREO)},
                .sampleRates = {8000, 11025, 16000, 32000, 44100, 48000}};
    };
    return {
            createStdPcmAudioProfile(PcmType::INT_16_BIT),
            createStdPcmAudioProfile(PcmType::INT_24_BIT),
    };
}

// Primary (default) configuration:
//
// Device ports:
//  * "Speaker", OUT_SPEAKER, default
//    - no profiles specified
//  * "Built-In Mic", IN_MICROPHONE, default
//    - no profiles specified
//  * "Telephony Tx", OUT_TELEPHONY_TX
//    - no profiles specified
//  * "Telephony Rx", IN_TELEPHONY_RX
//    - no profiles specified
//  * "FM Tuner", IN_FM_TUNER
//    - no profiles specified
//
// Mix ports:
//  * "primary output", PRIMARY, 1 max open, 1 max active stream
//    - profile PCM 16-bit; MONO, STEREO; 8000, 11025, 16000, 32000, 44100, 48000
//  * "primary input", 1 max open, 1 max active stream
//    - profile PCM 16-bit; MONO, STEREO; 8000, 11025, 16000, 32000, 44100, 48000
//  * "telephony_tx", 1 max open, 1 max active stream
//    - profile PCM 16-bit; MONO, STEREO; 8000, 11025, 16000, 32000, 44100, 48000
//  * "telephony_rx", 1 max open, 1 max active stream
//    - profile PCM 16-bit; MONO, STEREO; 8000, 11025, 16000, 32000, 44100, 48000
//  * "fm_tuner", 1 max open, 1 max active stream
//    - profile PCM 16-bit; MONO, STEREO; 8000, 11025, 16000, 32000, 44100, 48000
//
// Routes:
//  "primary out" -> "Speaker"
//  "Built-In Mic" -> "primary input"
//  "Telephony Rx" -> "telephony_rx"
//  "telephony_tx" -> "Telephony Tx"
//  "FM Tuner" -> "fm_tuner"
//
// Initial port configs:
//  * "Speaker" device port: dynamic configuration
//  * "Built-In Mic" device port: dynamic configuration
//  * "Telephony Tx" device port: dynamic configuration
//  * "Telephony Rx" device port: dynamic configuration
//  * "FM Tuner" device port: dynamic configuration
//
std::unique_ptr<Configuration> getPrimaryConfiguration() {
    static const Configuration configuration = []() {
        const std::vector<AudioProfile> standardPcmAudioProfiles = {
                createProfile(PcmType::INT_16_BIT,
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
                createDynamicPortConfig(speakerOutDevice.id, speakerOutDevice.id, 0, false,
                                        createDeviceExt(AudioDeviceType::OUT_SPEAKER, 0)));

        AudioPort micInDevice =
                createPort(c.nextPortId++, "Built-In Mic", 0, true,
                           createDeviceExt(AudioDeviceType::IN_MICROPHONE,
                                           1 << AudioPortDeviceExt::FLAG_INDEX_DEFAULT_DEVICE));
        c.ports.push_back(micInDevice);
        c.initialConfigs.push_back(
                createDynamicPortConfig(micInDevice.id, micInDevice.id, 0, true,
                                        createDeviceExt(AudioDeviceType::IN_MICROPHONE, 0)));

        AudioPort telephonyTxOutDevice =
                createPort(c.nextPortId++, "Telephony Tx", 0, false,
                           createDeviceExt(AudioDeviceType::OUT_TELEPHONY_TX, 0));
        c.ports.push_back(telephonyTxOutDevice);
        c.initialConfigs.push_back(
                createDynamicPortConfig(telephonyTxOutDevice.id, telephonyTxOutDevice.id, 0, false,
                                        createDeviceExt(AudioDeviceType::OUT_TELEPHONY_TX, 0)));

        AudioPort telephonyRxInDevice =
                createPort(c.nextPortId++, "Telephony Rx", 0, true,
                           createDeviceExt(AudioDeviceType::IN_TELEPHONY_RX, 0));
        c.ports.push_back(telephonyRxInDevice);
        c.initialConfigs.push_back(
                createDynamicPortConfig(telephonyRxInDevice.id, telephonyRxInDevice.id, 0, true,
                                        createDeviceExt(AudioDeviceType::IN_TELEPHONY_RX, 0)));

        AudioPort fmTunerInDevice = createPort(c.nextPortId++, "FM Tuner", 0, true,
                                               createDeviceExt(AudioDeviceType::IN_FM_TUNER, 0));
        c.ports.push_back(fmTunerInDevice);
        c.initialConfigs.push_back(
                createDynamicPortConfig(fmTunerInDevice.id, fmTunerInDevice.id, 0, true,
                                        createDeviceExt(AudioDeviceType::IN_FM_TUNER, 0)));

        // Mix ports

        AudioPort primaryOutMix = createPort(c.nextPortId++, "primary output",
                                             makeBitPositionFlagMask(AudioOutputFlags::PRIMARY),
                                             false, createPortMixExt(0, 0));
        primaryOutMix.profiles.insert(primaryOutMix.profiles.begin(),
                                      standardPcmAudioProfiles.begin(),
                                      standardPcmAudioProfiles.end());
        c.ports.push_back(primaryOutMix);

        AudioPort primaryInMix =
                createPort(c.nextPortId++, "primary input", 0, true, createPortMixExt(0, 1));
        primaryInMix.profiles.push_back(
                createProfile(PcmType::INT_16_BIT,
                              {AudioChannelLayout::LAYOUT_MONO, AudioChannelLayout::LAYOUT_STEREO},
                              {8000, 11025, 16000, 32000, 44100, 48000}));
        c.ports.push_back(primaryInMix);

        AudioPort telephonyTxOutMix =
                createPort(c.nextPortId++, "telephony_tx", 0, false, createPortMixExt(1, 1));
        telephonyTxOutMix.profiles.insert(telephonyTxOutMix.profiles.begin(),
                                          standardPcmAudioProfiles.begin(),
                                          standardPcmAudioProfiles.end());
        c.ports.push_back(telephonyTxOutMix);

        AudioPort telephonyRxInMix =
                createPort(c.nextPortId++, "telephony_rx", 0, true, createPortMixExt(0, 1));
        telephonyRxInMix.profiles.insert(telephonyRxInMix.profiles.begin(),
                                         standardPcmAudioProfiles.begin(),
                                         standardPcmAudioProfiles.end());
        c.ports.push_back(telephonyRxInMix);

        AudioPort fmTunerInMix =
                createPort(c.nextPortId++, "fm_tuner", 0, true, createPortMixExt(0, 1));
        fmTunerInMix.profiles.insert(fmTunerInMix.profiles.begin(),
                                     standardPcmAudioProfiles.begin(),
                                     standardPcmAudioProfiles.end());
        c.ports.push_back(fmTunerInMix);

        c.routes.push_back(createRoute({primaryOutMix}, speakerOutDevice));
        c.routes.push_back(createRoute({micInDevice}, primaryInMix));
        c.routes.push_back(createRoute({telephonyRxInDevice}, telephonyRxInMix));
        c.routes.push_back(createRoute({telephonyTxOutMix}, telephonyTxOutDevice));
        c.routes.push_back(createRoute({fmTunerInDevice}, fmTunerInMix));

        c.portConfigs.insert(c.portConfigs.end(), c.initialConfigs.begin(), c.initialConfigs.end());

        return c;
    }();
    return std::make_unique<Configuration>(configuration);
}

// Note: When transitioning to loading of XML configs, either keep the configuration
// of the remote submix sources from this static configuration, or update the XML
// config to match it. There are several reasons for that:
//   1. The "Remote Submix In" device is listed in the XML config as "attached",
//      however in the AIDL scheme its device type has a "virtual" connection.
//   2. The canonical r_submix configuration only lists 'STEREO' and '48000',
//      however the framework attempts to open streams for other sample rates
//      as well. The legacy r_submix implementation allowed that, but libaudiohal@aidl
//      will not find a mix port to use. Because of that, list all sample rates that
//      the legacy implementation allowed (note that mono was not allowed, the framework
//      is expected to upmix mono tracks into stereo if needed).
//   3. The legacy implementation had a hard limit on the number of routes (10),
//      and this is checked indirectly by AudioPlaybackCaptureTest#testPlaybackCaptureDoS
//      CTS test. Instead of hardcoding the number of routes, we can use
//      "maxOpen/ActiveStreamCount" to enforce a similar limit. However, the canonical
//      XML file lacks this specification.
//
// Remote Submix configuration:
//
// Device ports:
//  * "Remote Submix Out", OUT_SUBMIX
//    - no profiles specified
//  * "Remote Submix In", IN_SUBMIX
//    - no profiles specified
//
// Mix ports:
//  * "r_submix output", maximum 10 opened streams, maximum 10 active streams
//    - profile PCM 16-bit; STEREO; 8000, 11025, 16000, 32000, 44100, 48000
//  * "r_submix input", maximum 10 opened streams, maximum 10 active streams
//    - profile PCM 16-bit; STEREO; 8000, 11025, 16000, 32000, 44100, 48000
//
// Routes:
//  "r_submix output" -> "Remote Submix Out"
//  "Remote Submix In" -> "r_submix input"
//
std::unique_ptr<Configuration> getRSubmixConfiguration() {
    static const Configuration configuration = []() {
        Configuration c;
        const std::vector<AudioProfile> remoteSubmixPcmAudioProfiles{
                createProfile(PcmType::INT_16_BIT, {AudioChannelLayout::LAYOUT_STEREO},
                              {8000, 11025, 16000, 32000, 44100, 48000})};

        // Device ports

        AudioPort rsubmixOutDevice =
                createPort(c.nextPortId++, "Remote Submix Out", 0, false,
                           createDeviceExt(AudioDeviceType::OUT_SUBMIX, 0,
                                           AudioDeviceDescription::CONNECTION_VIRTUAL));
        c.ports.push_back(rsubmixOutDevice);
        c.connectedProfiles[rsubmixOutDevice.id] = remoteSubmixPcmAudioProfiles;

        AudioPort rsubmixInDevice =
                createPort(c.nextPortId++, "Remote Submix In", 0, true,
                           createDeviceExt(AudioDeviceType::IN_SUBMIX, 0,
                                           AudioDeviceDescription::CONNECTION_VIRTUAL));
        c.ports.push_back(rsubmixInDevice);
        c.connectedProfiles[rsubmixInDevice.id] = remoteSubmixPcmAudioProfiles;

        // Mix ports

        AudioPort rsubmixOutMix =
                createPort(c.nextPortId++, "r_submix output", 0, false, createPortMixExt(10, 10));
        rsubmixOutMix.profiles = remoteSubmixPcmAudioProfiles;
        c.ports.push_back(rsubmixOutMix);

        AudioPort rsubmixInMix =
                createPort(c.nextPortId++, "r_submix input", 0, true, createPortMixExt(10, 10));
        rsubmixInMix.profiles = remoteSubmixPcmAudioProfiles;
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
//  * "USB Device Out", OUT_DEVICE, CONNECTION_USB
//    - no profiles specified
//  * "USB Headset Out", OUT_HEADSET, CONNECTION_USB
//    - no profiles specified
//  * "USB Device In", IN_DEVICE, CONNECTION_USB
//    - no profiles specified
//  * "USB Headset In", IN_HEADSET, CONNECTION_USB
//    - no profiles specified
//
// Mix ports:
//  * "usb_device output", 1 max open, 1 max active stream
//    - no profiles specified
//  * "usb_device input", 1 max open, 1 max active stream
//    - no profiles specified
//
// Routes:
//  * "usb_device output" -> "USB Device Out"
//  * "usb_device output" -> "USB Headset Out"
//  * "USB Device In", "USB Headset In" -> "usb_device input"
//
// Profiles for device port connected state (when simulating connections):
//  * "USB Device Out", "USB Headset Out":
//    - profile PCM 16-bit; MONO, STEREO, INDEX_MASK_1, INDEX_MASK_2; 44100, 48000
//    - profile PCM 24-bit; MONO, STEREO, INDEX_MASK_1, INDEX_MASK_2; 44100, 48000
//  * "USB Device In", "USB Headset In":
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

        AudioPort usbOutDevice =
                createPort(c.nextPortId++, "USB Device Out", 0, false,
                           createDeviceExt(AudioDeviceType::OUT_DEVICE, 0,
                                           AudioDeviceDescription::CONNECTION_USB));
        c.ports.push_back(usbOutDevice);
        c.connectedProfiles[usbOutDevice.id] = standardPcmAudioProfiles;

        AudioPort usbOutHeadset =
                createPort(c.nextPortId++, "USB Headset Out", 0, false,
                           createDeviceExt(AudioDeviceType::OUT_HEADSET, 0,
                                           AudioDeviceDescription::CONNECTION_USB));
        c.ports.push_back(usbOutHeadset);
        c.connectedProfiles[usbOutHeadset.id] = standardPcmAudioProfiles;

        AudioPort usbInDevice = createPort(c.nextPortId++, "USB Device In", 0, true,
                                           createDeviceExt(AudioDeviceType::IN_DEVICE, 0,
                                                           AudioDeviceDescription::CONNECTION_USB));
        c.ports.push_back(usbInDevice);
        c.connectedProfiles[usbInDevice.id] = standardPcmAudioProfiles;

        AudioPort usbInHeadset =
                createPort(c.nextPortId++, "USB Headset In", 0, true,
                           createDeviceExt(AudioDeviceType::IN_HEADSET, 0,
                                           AudioDeviceDescription::CONNECTION_USB));
        c.ports.push_back(usbInHeadset);
        c.connectedProfiles[usbInHeadset.id] = standardPcmAudioProfiles;

        // Mix ports

        AudioPort usbDeviceOutMix =
                createPort(c.nextPortId++, "usb_device output", 0, false, createPortMixExt(1, 1));
        c.ports.push_back(usbDeviceOutMix);

        AudioPort usbDeviceInMix =
                createPort(c.nextPortId++, "usb_device input", 0, true, createPortMixExt(0, 1));
        c.ports.push_back(usbDeviceInMix);

        c.routes.push_back(createRoute({usbDeviceOutMix}, usbOutDevice));
        c.routes.push_back(createRoute({usbDeviceOutMix}, usbOutHeadset));
        c.routes.push_back(createRoute({usbInDevice, usbInHeadset}, usbDeviceInMix));

        return c;
    }();
    return std::make_unique<Configuration>(configuration);
}

// Stub configuration:
//
// Device ports:
//  * "Test Out", OUT_AFE_PROXY
//    - no profiles specified
//  * "Test In", IN_AFE_PROXY
//    - no profiles specified
//  * "Wired Headset", OUT_HEADSET
//    - no profiles specified
//  * "Wired Headset Mic", IN_HEADSET
//    - no profiles specified
//
// Mix ports:
//  * "test output", 1 max open, 1 max active stream
//    - profile PCM 24-bit; MONO, STEREO; 8000, 11025, 16000, 32000, 44100, 48000
//  * "test fast output", 1 max open, 1 max active stream
//    - profile PCM 24-bit; STEREO; 44100, 48000
//  * "test compressed offload", DIRECT|COMPRESS_OFFLOAD|NON_BLOCKING, 1 max open, 1 max active
//  stream
//    - profile MP3; MONO, STEREO; 44100, 48000
//  * "test input", 2 max open, 2 max active streams
//    - profile PCM 24-bit; MONO, STEREO, FRONT_BACK;
//        8000, 11025, 16000, 22050, 32000, 44100, 48000
//
// Routes:
//  "test output", "test fast output", "test compressed offload" -> "Test Out"
//  "test output" -> "Wired Headset"
//  "Test In", "Wired Headset Mic" -> "test input"
//
// Initial port configs:
//  * "Test Out" device port: PCM 24-bit; STEREO; 48000
//  * "Test In" device port: PCM 24-bit; MONO; 48000
//
// Profiles for device port connected state (when simulating connections):
//  * "Wired Headset": dynamic profiles
//  * "Wired Headset Mic": dynamic profiles
//
std::unique_ptr<Configuration> getStubConfiguration() {
    static const Configuration configuration = []() {
        Configuration c;

        // Device ports

        AudioPort testOutDevice = createPort(c.nextPortId++, "Test Out", 0, false,
                                             createDeviceExt(AudioDeviceType::OUT_AFE_PROXY, 0));
        c.ports.push_back(testOutDevice);
        c.initialConfigs.push_back(
                createPortConfig(testOutDevice.id, testOutDevice.id, PcmType::INT_24_BIT,
                                 AudioChannelLayout::LAYOUT_STEREO, 48000, 0, false,
                                 createDeviceExt(AudioDeviceType::OUT_AFE_PROXY, 0)));

        AudioPort headsetOutDevice =
                createPort(c.nextPortId++, "Wired Headset", 0, false,
                           createDeviceExt(AudioDeviceType::OUT_HEADSET, 0,
                                           AudioDeviceDescription::CONNECTION_ANALOG));
        c.ports.push_back(headsetOutDevice);

        AudioPort testInDevice = createPort(c.nextPortId++, "Test In", 0, true,
                                            createDeviceExt(AudioDeviceType::IN_AFE_PROXY, 0));
        c.ports.push_back(testInDevice);
        c.initialConfigs.push_back(
                createPortConfig(testInDevice.id, testInDevice.id, PcmType::INT_24_BIT,
                                 AudioChannelLayout::LAYOUT_MONO, 48000, 0, true,
                                 createDeviceExt(AudioDeviceType::IN_AFE_PROXY, 0)));

        AudioPort headsetInDevice =
                createPort(c.nextPortId++, "Wired Headset Mic", 0, true,
                           createDeviceExt(AudioDeviceType::IN_HEADSET, 0,
                                           AudioDeviceDescription::CONNECTION_ANALOG));
        c.ports.push_back(headsetInDevice);

        // Mix ports

        AudioPort testOutMix =
                createPort(c.nextPortId++, "test output", 0, false, createPortMixExt(1, 1));
        testOutMix.profiles.push_back(
                createProfile(PcmType::INT_24_BIT,
                              {AudioChannelLayout::LAYOUT_MONO, AudioChannelLayout::LAYOUT_STEREO},
                              {8000, 11025, 16000, 32000, 44100, 48000}));
        c.ports.push_back(testOutMix);

        AudioPort testFastOutMix = createPort(c.nextPortId++, "test fast output",
                                              makeBitPositionFlagMask({AudioOutputFlags::FAST}),
                                              false, createPortMixExt(1, 1));
        testFastOutMix.profiles.push_back(createProfile(
                PcmType::INT_24_BIT, {AudioChannelLayout::LAYOUT_STEREO}, {44100, 48000}));
        c.ports.push_back(testFastOutMix);

        AudioPort compressedOffloadOutMix =
                createPort(c.nextPortId++, "test compressed offload",
                           makeBitPositionFlagMask({AudioOutputFlags::DIRECT,
                                                    AudioOutputFlags::COMPRESS_OFFLOAD,
                                                    AudioOutputFlags::NON_BLOCKING}),
                           false, createPortMixExt(1, 1));
        compressedOffloadOutMix.profiles.push_back(
                createProfile(::android::MEDIA_MIMETYPE_AUDIO_MPEG,
                              {AudioChannelLayout::LAYOUT_MONO, AudioChannelLayout::LAYOUT_STEREO},
                              {44100, 48000}));
        c.ports.push_back(compressedOffloadOutMix);

        AudioPort testInMix =
                createPort(c.nextPortId++, "test input", 0, true, createPortMixExt(2, 2));
        testInMix.profiles.push_back(
                createProfile(PcmType::INT_16_BIT,
                              {AudioChannelLayout::LAYOUT_MONO, AudioChannelLayout::LAYOUT_STEREO,
                               AudioChannelLayout::LAYOUT_FRONT_BACK},
                              {8000, 11025, 16000, 22050, 32000, 44100, 48000}));
        testInMix.profiles.push_back(
                createProfile(PcmType::INT_24_BIT,
                              {AudioChannelLayout::LAYOUT_MONO, AudioChannelLayout::LAYOUT_STEREO,
                               AudioChannelLayout::LAYOUT_FRONT_BACK},
                              {8000, 11025, 16000, 22050, 32000, 44100, 48000}));
        c.ports.push_back(testInMix);

        c.routes.push_back(
                createRoute({testOutMix, testFastOutMix, compressedOffloadOutMix}, testOutDevice));
        c.routes.push_back(createRoute({testOutMix}, headsetOutDevice));
        c.routes.push_back(createRoute({testInDevice, headsetInDevice}, testInMix));

        c.portConfigs.insert(c.portConfigs.end(), c.initialConfigs.begin(), c.initialConfigs.end());

        return c;
    }();
    return std::make_unique<Configuration>(configuration);
}

// Bluetooth configuration:
//
// Device ports:
//  * "BT A2DP Out", OUT_DEVICE, CONNECTION_BT_A2DP
//    - profile PCM 16-bit; STEREO; 44100, 48000, 88200, 96000
//  * "BT A2DP Headphones", OUT_HEADPHONE, CONNECTION_BT_A2DP
//    - profile PCM 16-bit; STEREO; 44100, 48000, 88200, 96000
//  * "BT A2DP Speaker", OUT_SPEAKER, CONNECTION_BT_A2DP
//    - profile PCM 16-bit; STEREO; 44100, 48000, 88200, 96000
//  * "BT Hearing Aid Out", OUT_HEARING_AID, CONNECTION_WIRELESS
//    - no profiles specified
//
// Mix ports:
//  * "a2dp output", 1 max open, 1 max active stream
//    - no profiles specified
//  * "hearing aid output", 1 max open, 1 max active stream
//    - profile PCM 16-bit; STEREO; 16000, 24000
//
// Routes:
//  "a2dp output" -> "BT A2DP Out"
//  "a2dp output" -> "BT A2DP Headphones"
//  "a2dp output" -> "BT A2DP Speaker"
//  "hearing aid output" -> "BT Hearing Aid Out"
//
// Profiles for device port connected state (when simulating connections):
//  * "BT A2DP Out", "BT A2DP Headphones", "BT A2DP Speaker":
//    - profile PCM 16-bit; STEREO; 44100, 48000, 88200, 96000
//  * "BT Hearing Aid Out":
//    - profile PCM 16-bit; STEREO; 16000, 24000
//
std::unique_ptr<Configuration> getBluetoothConfiguration() {
    static const Configuration configuration = []() {
        const std::vector<AudioProfile> standardPcmAudioProfiles = {
                createProfile(PcmType::INT_16_BIT, {AudioChannelLayout::LAYOUT_STEREO},
                              {44100, 48000, 88200, 96000})};
        const std::vector<AudioProfile> hearingAidAudioProfiles = {createProfile(
                PcmType::INT_16_BIT, {AudioChannelLayout::LAYOUT_STEREO}, {16000, 24000})};
        Configuration c;

        // Device ports
        AudioPort btOutDevice =
                createPort(c.nextPortId++, "BT A2DP Out", 0, false,
                           createDeviceExt(AudioDeviceType::OUT_DEVICE, 0,
                                           AudioDeviceDescription::CONNECTION_BT_A2DP));
        btOutDevice.profiles.insert(btOutDevice.profiles.begin(), standardPcmAudioProfiles.begin(),
                                    standardPcmAudioProfiles.end());
        c.ports.push_back(btOutDevice);
        c.connectedProfiles[btOutDevice.id] = standardPcmAudioProfiles;

        AudioPort btOutHeadphone =
                createPort(c.nextPortId++, "BT A2DP Headphones", 0, false,
                           createDeviceExt(AudioDeviceType::OUT_HEADPHONE, 0,
                                           AudioDeviceDescription::CONNECTION_BT_A2DP));
        btOutHeadphone.profiles.insert(btOutHeadphone.profiles.begin(),
                                       standardPcmAudioProfiles.begin(),
                                       standardPcmAudioProfiles.end());
        c.ports.push_back(btOutHeadphone);
        c.connectedProfiles[btOutHeadphone.id] = standardPcmAudioProfiles;

        AudioPort btOutSpeaker =
                createPort(c.nextPortId++, "BT A2DP Speaker", 0, false,
                           createDeviceExt(AudioDeviceType::OUT_SPEAKER, 0,
                                           AudioDeviceDescription::CONNECTION_BT_A2DP));
        btOutSpeaker.profiles.insert(btOutSpeaker.profiles.begin(),
                                     standardPcmAudioProfiles.begin(),
                                     standardPcmAudioProfiles.end());
        c.ports.push_back(btOutSpeaker);
        c.connectedProfiles[btOutSpeaker.id] = standardPcmAudioProfiles;

        AudioPort btOutHearingAid =
                createPort(c.nextPortId++, "BT Hearing Aid Out", 0, false,
                           createDeviceExt(AudioDeviceType::OUT_HEARING_AID, 0,
                                           AudioDeviceDescription::CONNECTION_WIRELESS));
        c.ports.push_back(btOutHearingAid);
        c.connectedProfiles[btOutHearingAid.id] = hearingAidAudioProfiles;

        // Mix ports
        AudioPort btOutMix =
                createPort(c.nextPortId++, "a2dp output", 0, false, createPortMixExt(1, 1));
        c.ports.push_back(btOutMix);

        AudioPort btHearingOutMix =
                createPort(c.nextPortId++, "hearing aid output", 0, false, createPortMixExt(1, 1));
        btHearingOutMix.profiles = hearingAidAudioProfiles;
        c.ports.push_back(btHearingOutMix);

        c.routes.push_back(createRoute({btOutMix}, btOutDevice));
        c.routes.push_back(createRoute({btOutMix}, btOutHeadphone));
        c.routes.push_back(createRoute({btOutMix}, btOutSpeaker));
        c.routes.push_back(createRoute({btHearingOutMix}, btOutHearingAid));

        return c;
    }();
    return std::make_unique<Configuration>(configuration);
}

std::unique_ptr<Module::Configuration> getConfiguration(Module::Type moduleType) {
    switch (moduleType) {
        case Module::Type::DEFAULT:
            return getPrimaryConfiguration();
        case Module::Type::R_SUBMIX:
            return getRSubmixConfiguration();
        case Module::Type::STUB:
            return getStubConfiguration();
        case Module::Type::USB:
            return getUsbConfiguration();
        case Module::Type::BLUETOOTH:
            return getBluetoothConfiguration();
    }
}

}  // namespace aidl::android::hardware::audio::core::internal
