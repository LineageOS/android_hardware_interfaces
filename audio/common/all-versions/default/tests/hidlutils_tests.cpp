/*
 * Copyright (C) 2020 The Android Open Source Project
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

#include <array>
#include <string>

#include <gtest/gtest.h>

#define LOG_TAG "HidlUtils_Test"
#include <log/log.h>

#include <HidlUtils.h>
#include <android_audio_policy_configuration_V7_0-enums.h>
#include <system/audio.h>
#include <xsdc/XsdcSupport.h>

using namespace android;
using namespace ::android::hardware::audio::common::CPP_VERSION;
using ::android::hardware::audio::common::CPP_VERSION::implementation::HidlUtils;
namespace xsd {
using namespace ::android::audio::policy::configuration::V7_0;
}

static constexpr audio_channel_mask_t kInvalidHalChannelMask =
        static_cast<audio_channel_mask_t>(0xFFFFFFFFU);
static constexpr audio_devices_t kInvalidHalDevice = static_cast<audio_devices_t>(0xFFFFFFFFU);
static constexpr audio_format_t kInvalidHalFormat = static_cast<audio_format_t>(0xFFFFFFFFU);
static constexpr audio_gain_mode_t kInvalidHalGainMode =
        static_cast<audio_gain_mode_t>(0xFFFFFFFFU);
static constexpr audio_source_t kInvalidHalSource = static_cast<audio_source_t>(0xFFFFFFFFU);
static constexpr audio_stream_type_t kInvalidHalStreamType =
        static_cast<audio_stream_type_t>(0xFFFFFFFFU);
static constexpr audio_usage_t kInvalidHalUsage = static_cast<audio_usage_t>(0xFFFFFFFFU);

TEST(HidlUtils, ConvertInvalidChannelMask) {
    AudioChannelMask invalid;
    EXPECT_EQ(BAD_VALUE, HidlUtils::audioChannelMaskFromHal(AUDIO_CHANNEL_INVALID,
                                                            false /*isInput*/, &invalid));
    EXPECT_EQ(BAD_VALUE, HidlUtils::audioChannelMaskFromHal(AUDIO_CHANNEL_INVALID, true /*isInput*/,
                                                            &invalid));
    EXPECT_EQ(BAD_VALUE, HidlUtils::audioChannelMaskFromHal(kInvalidHalChannelMask,
                                                            false /*isInput*/, &invalid));
    EXPECT_EQ(BAD_VALUE, HidlUtils::audioChannelMaskFromHal(kInvalidHalChannelMask,
                                                            true /*isInput*/, &invalid));
    audio_channel_mask_t halInvalid;
    // INVALID channel mask is not in XSD thus it's not allowed for transfer over HIDL.
    EXPECT_EQ(BAD_VALUE, HidlUtils::audioChannelMaskToHal("AUDIO_CHANNEL_INVALID", &halInvalid));
    EXPECT_EQ(BAD_VALUE, HidlUtils::audioChannelMaskToHal("random string", &halInvalid));
}

// Might move these to the audio_policy_configuration_V7_0-enums library
// if there would be usages in the default wrapper code. In that case,
// it would be better to reimplement these methods using a proper switch statement
// over all known enum values.
static bool isInputChannelMask(xsd::AudioChannelMask channelMask) {
    return toString(channelMask).find("_CHANNEL_IN_") != std::string::npos;
}

static bool isOutputChannelMask(xsd::AudioChannelMask channelMask) {
    return toString(channelMask).find("_CHANNEL_OUT_") != std::string::npos;
}

static bool isIndexChannelMask(xsd::AudioChannelMask channelMask) {
    return toString(channelMask).find("_CHANNEL_INDEX_") != std::string::npos;
}

TEST(HidlUtils, ConvertChannelMask) {
    for (const auto enumVal : xsdc_enum_range<xsd::AudioChannelMask>{}) {
        const AudioChannelMask channelMask = toString(enumVal);
        audio_channel_mask_t halChannelMask, halChannelMaskBack;
        AudioChannelMask channelMaskBack;
        EXPECT_EQ(NO_ERROR, HidlUtils::audioChannelMaskToHal(channelMask, &halChannelMask))
                << "Conversion of \"" << channelMask << "\" failed";
        EXPECT_EQ(enumVal != xsd::AudioChannelMask::AUDIO_CHANNEL_NONE,
                  audio_channel_mask_is_valid(halChannelMask))
                << "Validity of \"" << channelMask << "\" is not as expected";
        if (bool isInput = isInputChannelMask(enumVal); isInput || isOutputChannelMask(enumVal)) {
            EXPECT_EQ(NO_ERROR,
                      HidlUtils::audioChannelMaskFromHal(halChannelMask, isInput, &channelMaskBack))
                    << "Conversion of " << (isInput ? "input" : "output") << " channel mask "
                    << halChannelMask << " failed";
            // Due to aliased values, the result of 'fromHal' might not be the same
            // as 'channelMask', thus we need to compare the results of 'toHal' conversion instead.
            EXPECT_EQ(NO_ERROR,
                      HidlUtils::audioChannelMaskToHal(channelMaskBack, &halChannelMaskBack))
                    << "Conversion of \"" << channelMaskBack << "\" failed";
            EXPECT_EQ(halChannelMask, halChannelMaskBack);
        } else if (isIndexChannelMask(enumVal) ||
                   enumVal == xsd::AudioChannelMask::AUDIO_CHANNEL_NONE) {
            // Conversions for indexed masks and "none" must not depend on the provided direction.
            EXPECT_EQ(NO_ERROR, HidlUtils::audioChannelMaskFromHal(halChannelMask, true /*isInput*/,
                                                                   &channelMaskBack))
                    << "Conversion of indexed / none channel mask " << halChannelMask
                    << " failed (as input channel mask)";
            EXPECT_EQ(channelMask, channelMaskBack);
            EXPECT_EQ(NO_ERROR, HidlUtils::audioChannelMaskFromHal(
                                        halChannelMask, false /*isInput*/, &channelMaskBack))
                    << "Conversion of indexed / none channel mask " << halChannelMask
                    << " failed (as output channel mask)";
            EXPECT_EQ(channelMask, channelMaskBack);
        } else {
            FAIL() << "Unrecognized channel mask \"" << channelMask << "\"";
        }
    }
}

TEST(HidlUtils, ConvertInvalidConfigBase) {
    AudioConfigBase invalid;
    EXPECT_EQ(BAD_VALUE, HidlUtils::audioConfigBaseFromHal({.sample_rate = 0,
                                                            .channel_mask = kInvalidHalChannelMask,
                                                            .format = kInvalidHalFormat},
                                                           false /*isInput*/, &invalid));
    EXPECT_EQ(BAD_VALUE, HidlUtils::audioConfigBaseFromHal({.sample_rate = 0,
                                                            .channel_mask = kInvalidHalChannelMask,
                                                            .format = kInvalidHalFormat},
                                                           true /*isInput*/, &invalid));
    audio_config_base_t halInvalid;
    invalid.sampleRateHz = 0;
    invalid.channelMask = "random string";
    invalid.format = "random string";
    EXPECT_EQ(BAD_VALUE, HidlUtils::audioConfigBaseToHal(invalid, &halInvalid));
}

TEST(HidlUtils, ConvertConfigBase) {
    AudioConfigBase configBase;
    configBase.sampleRateHz = 44100;
    configBase.channelMask = toString(xsd::AudioChannelMask::AUDIO_CHANNEL_OUT_STEREO);
    configBase.format = toString(xsd::AudioFormat::AUDIO_FORMAT_PCM_16_BIT);
    audio_config_base_t halConfigBase;
    EXPECT_EQ(NO_ERROR, HidlUtils::audioConfigBaseToHal(configBase, &halConfigBase));
    AudioConfigBase configBaseBack;
    EXPECT_EQ(NO_ERROR,
              HidlUtils::audioConfigBaseFromHal(halConfigBase, false /*isInput*/, &configBaseBack));
    EXPECT_EQ(configBase, configBaseBack);
}

TEST(HidlUtils, ConvertInvalidDeviceType) {
    AudioDevice invalid;
    EXPECT_EQ(BAD_VALUE, HidlUtils::audioDeviceTypeFromHal(kInvalidHalDevice, &invalid));
    audio_devices_t halInvalid;
    EXPECT_EQ(BAD_VALUE, HidlUtils::audioDeviceTypeToHal("random string", &halInvalid));
}

TEST(HidlUtils, ConvertDeviceType) {
    for (const auto enumVal : xsdc_enum_range<xsd::AudioDevice>{}) {
        const AudioDevice deviceType = toString(enumVal);
        audio_devices_t halDeviceType, halDeviceTypeBack;
        AudioDevice deviceTypeBack;
        EXPECT_EQ(NO_ERROR, HidlUtils::audioDeviceTypeToHal(deviceType, &halDeviceType))
                << "Conversion of \"" << deviceType << "\" failed";
        if (enumVal != xsd::AudioDevice::AUDIO_DEVICE_NONE) {
            EXPECT_TRUE(audio_is_input_device(halDeviceType) ||
                        audio_is_output_device(halDeviceType))
                    << "Device \"" << deviceType << "\" is neither input, nor output device";
        } else {
            EXPECT_FALSE(audio_is_input_device(halDeviceType));
            EXPECT_FALSE(audio_is_output_device(halDeviceType));
        }
        EXPECT_EQ(NO_ERROR, HidlUtils::audioDeviceTypeFromHal(halDeviceType, &deviceTypeBack))
                << "Conversion of device type " << halDeviceType << " failed";
        // Due to aliased values, the result of 'fromHal' might not be the same
        // as 'deviceType', thus we need to compare the results of 'toHal' conversion instead.
        EXPECT_EQ(NO_ERROR, HidlUtils::audioDeviceTypeToHal(deviceTypeBack, &halDeviceTypeBack))
                << "Conversion of \"" << deviceTypeBack << "\" failed";
        EXPECT_EQ(halDeviceType, halDeviceTypeBack);
    }
}

// The enums module is too small to have unit tests on its own.
TEST(HidlUtils, VendorExtension) {
    EXPECT_TRUE(xsd::isVendorExtension("VX_GOOGLE_VR_42"));
    EXPECT_FALSE(xsd::isVendorExtension("random string"));
    EXPECT_FALSE(xsd::isVendorExtension("VX_"));
    EXPECT_FALSE(xsd::isVendorExtension("VX_GOOGLE_$$"));
}

TEST(HidlUtils, ConvertInvalidDeviceAddress) {
    DeviceAddress invalid;
    EXPECT_EQ(BAD_VALUE, HidlUtils::deviceAddressFromHal(AUDIO_DEVICE_OUT_BLUETOOTH_A2DP_SPEAKER,
                                                         nullptr, &invalid));
    EXPECT_EQ(BAD_VALUE, HidlUtils::deviceAddressFromHal(AUDIO_DEVICE_OUT_BLUETOOTH_A2DP_SPEAKER,
                                                         "", &invalid));
    EXPECT_EQ(BAD_VALUE, HidlUtils::deviceAddressFromHal(AUDIO_DEVICE_OUT_IP, nullptr, &invalid));
    EXPECT_EQ(BAD_VALUE, HidlUtils::deviceAddressFromHal(AUDIO_DEVICE_OUT_IP, "", &invalid));
    EXPECT_EQ(BAD_VALUE,
              HidlUtils::deviceAddressFromHal(AUDIO_DEVICE_OUT_USB_HEADSET, nullptr, &invalid));
    EXPECT_EQ(BAD_VALUE,
              HidlUtils::deviceAddressFromHal(AUDIO_DEVICE_OUT_USB_HEADSET, "", &invalid));

    audio_devices_t halInvalid;
    char halAddress[AUDIO_DEVICE_MAX_ADDRESS_LEN] = {};
    invalid = {};
    invalid.deviceType = toString(xsd::AudioDevice::AUDIO_DEVICE_OUT_BLUETOOTH_A2DP_SPEAKER);
    EXPECT_EQ(BAD_VALUE, HidlUtils::deviceAddressToHal(invalid, &halInvalid, halAddress));
    invalid.deviceType = toString(xsd::AudioDevice::AUDIO_DEVICE_OUT_IP);
    EXPECT_EQ(BAD_VALUE, HidlUtils::deviceAddressToHal(invalid, &halInvalid, halAddress));
    invalid.deviceType = toString(xsd::AudioDevice::AUDIO_DEVICE_OUT_USB_HEADSET);
    EXPECT_EQ(BAD_VALUE, HidlUtils::deviceAddressToHal(invalid, &halInvalid, halAddress));
}

static void ConvertDeviceAddress(const DeviceAddress& device) {
    audio_devices_t halDeviceType;
    char halDeviceAddress[AUDIO_DEVICE_MAX_ADDRESS_LEN] = {};
    EXPECT_EQ(NO_ERROR, HidlUtils::deviceAddressToHal(device, &halDeviceType, halDeviceAddress));
    DeviceAddress deviceBack;
    EXPECT_EQ(NO_ERROR,
              HidlUtils::deviceAddressFromHal(halDeviceType, halDeviceAddress, &deviceBack));
    EXPECT_EQ(device, deviceBack);
}

TEST(HidlUtils, ConvertUniqueDeviceAddress) {
    DeviceAddress speaker;
    speaker.deviceType = toString(xsd::AudioDevice::AUDIO_DEVICE_OUT_SPEAKER);
    ConvertDeviceAddress(speaker);
}

TEST(HidlUtils, ConvertA2dpDeviceAddress) {
    DeviceAddress a2dpSpeaker;
    a2dpSpeaker.deviceType = toString(xsd::AudioDevice::AUDIO_DEVICE_OUT_BLUETOOTH_A2DP_SPEAKER);
    a2dpSpeaker.address.mac(std::array<uint8_t, 6>{1, 2, 3, 4, 5, 6});
    ConvertDeviceAddress(a2dpSpeaker);
}

TEST(HidlUtils, ConvertIpv4DeviceAddress) {
    DeviceAddress ipv4;
    ipv4.deviceType = toString(xsd::AudioDevice::AUDIO_DEVICE_OUT_IP);
    ipv4.address.ipv4(std::array<uint8_t, 4>{1, 2, 3, 4});
    ConvertDeviceAddress(ipv4);
}

TEST(HidlUtils, ConvertUsbDeviceAddress) {
    DeviceAddress usbHeadset;
    usbHeadset.deviceType = toString(xsd::AudioDevice::AUDIO_DEVICE_OUT_USB_HEADSET);
    usbHeadset.address.alsa({1, 2});
    ConvertDeviceAddress(usbHeadset);
}

TEST(HidlUtils, ConvertBusDeviceAddress) {
    DeviceAddress bus;
    bus.deviceType = toString(xsd::AudioDevice::AUDIO_DEVICE_OUT_BUS);
    bus.address.id("bus_device");
    ConvertDeviceAddress(bus);
}

TEST(HidlUtils, ConvertRSubmixDeviceAddress) {
    DeviceAddress rSubmix;
    rSubmix.deviceType = toString(xsd::AudioDevice::AUDIO_DEVICE_OUT_REMOTE_SUBMIX);
    rSubmix.address.id(AUDIO_REMOTE_SUBMIX_DEVICE_ADDRESS);
    ConvertDeviceAddress(rSubmix);
}

TEST(HidlUtils, ConvertVendorDeviceAddress) {
    // The address part is not mandatory, both cases must work.
    {
        DeviceAddress vendor;
        vendor.deviceType = "VX_GOOGLE_VR";
        audio_devices_t halDeviceType;
        char halDeviceAddress[AUDIO_DEVICE_MAX_ADDRESS_LEN] = {};
        // Ignore the result. Vendors will also add the extended device into
        // the list of devices in audio-hal-enums.h. Without that, the conversion
        // officially fails, but it still maps the device type to NONE.
        (void)HidlUtils::deviceAddressToHal(vendor, &halDeviceType, halDeviceAddress);
        EXPECT_EQ(AUDIO_DEVICE_NONE, halDeviceType);
        EXPECT_EQ(0, strnlen(halDeviceAddress, AUDIO_DEVICE_MAX_ADDRESS_LEN));
    }
    {
        DeviceAddress vendor;
        vendor.deviceType = "VX_GOOGLE_VR";
        vendor.address.id("vr1");
        audio_devices_t halDeviceType;
        char halDeviceAddress[AUDIO_DEVICE_MAX_ADDRESS_LEN] = {};
        // Ignore the result. Vendors will also add the extended device into
        // the list of devices in audio-hal-enums.h. Without that, the conversion
        // officially fails, but it still maps the device type to NONE and converts
        // the address.
        (void)HidlUtils::deviceAddressToHal(vendor, &halDeviceType, halDeviceAddress);
        EXPECT_EQ(AUDIO_DEVICE_NONE, halDeviceType);
        EXPECT_EQ(0, strncmp("vr1", halDeviceAddress, AUDIO_DEVICE_MAX_ADDRESS_LEN));
    }
}

TEST(HidlUtils, ConvertInvalidFormat) {
    AudioFormat invalid;
    EXPECT_EQ(BAD_VALUE, HidlUtils::audioFormatFromHal(kInvalidHalFormat, &invalid));
    audio_format_t halInvalid;
    EXPECT_EQ(BAD_VALUE, HidlUtils::audioFormatToHal("random string", &halInvalid));
}

TEST(HidlUtils, ConvertFormat) {
    for (const auto enumVal : xsdc_enum_range<xsd::AudioFormat>{}) {
        const AudioFormat format = toString(enumVal);
        audio_format_t halFormat;
        AudioFormat formatBack;
        EXPECT_EQ(NO_ERROR, HidlUtils::audioFormatToHal(format, &halFormat))
                << "Conversion of \"" << format << "\" failed";
        EXPECT_TRUE(audio_is_valid_format(halFormat))
                << "Converted format \"" << format << "\" is invalid";
        EXPECT_EQ(NO_ERROR, HidlUtils::audioFormatFromHal(halFormat, &formatBack))
                << "Conversion of format " << halFormat << " failed";
        EXPECT_EQ(format, formatBack);
    }
}

TEST(HidlUtils, ConvertInvalidGainModeMask) {
    hidl_vec<AudioGainMode> invalid;
    EXPECT_EQ(BAD_VALUE, HidlUtils::audioGainModeMaskFromHal(kInvalidHalGainMode, &invalid));
    audio_gain_mode_t halInvalid;
    invalid.resize(1);
    invalid[0] = "random string";
    EXPECT_EQ(BAD_VALUE, HidlUtils::audioGainModeMaskToHal(invalid, &halInvalid));
}

TEST(HidlUtils, ConvertGainModeMask) {
    hidl_vec<AudioGainMode> emptyGainModes;
    audio_gain_mode_t halEmptyGainModes;
    EXPECT_EQ(NO_ERROR, HidlUtils::audioGainModeMaskToHal(emptyGainModes, &halEmptyGainModes));
    hidl_vec<AudioGainMode> emptyGainModesBack;
    EXPECT_EQ(NO_ERROR,
              HidlUtils::audioGainModeMaskFromHal(halEmptyGainModes, &emptyGainModesBack));
    EXPECT_EQ(emptyGainModes, emptyGainModesBack);

    std::vector<std::string> allEnumValues;
    for (const auto enumVal : xsdc_enum_range<xsd::AudioGainMode>{}) {
        allEnumValues.push_back(toString(enumVal));
    }
    hidl_vec<AudioGainMode> allGainModes;
    allGainModes.resize(allEnumValues.size());
    for (size_t i = 0; i < allEnumValues.size(); ++i) {
        allGainModes[i] = allEnumValues[i];
    }
    audio_gain_mode_t halAllGainModes;
    EXPECT_EQ(NO_ERROR, HidlUtils::audioGainModeMaskToHal(allGainModes, &halAllGainModes));
    hidl_vec<AudioGainMode> allGainModesBack;
    EXPECT_EQ(NO_ERROR, HidlUtils::audioGainModeMaskFromHal(halAllGainModes, &allGainModesBack));
    EXPECT_EQ(allGainModes, allGainModesBack);
}

TEST(HidlUtils, ConvertInvalidSource) {
    AudioSource invalid;
    EXPECT_EQ(BAD_VALUE, HidlUtils::audioSourceFromHal(kInvalidHalSource, &invalid));
    audio_source_t halInvalid;
    EXPECT_EQ(BAD_VALUE, HidlUtils::audioSourceToHal("random string", &halInvalid));
}

TEST(HidlUtils, ConvertSource) {
    for (const auto enumVal : xsdc_enum_range<xsd::AudioSource>{}) {
        const AudioSource source = toString(enumVal);
        audio_source_t halSource;
        AudioSource sourceBack;
        EXPECT_EQ(NO_ERROR, HidlUtils::audioSourceToHal(source, &halSource))
                << "Conversion of \"" << source << "\" failed";
        EXPECT_EQ(enumVal != xsd::AudioSource::AUDIO_SOURCE_DEFAULT,
                  audio_is_valid_audio_source(halSource))
                << "Validity of \"" << source << "\" is not as expected";
        EXPECT_EQ(NO_ERROR, HidlUtils::audioSourceFromHal(halSource, &sourceBack))
                << "Conversion of source " << halSource << " failed";
        EXPECT_EQ(source, sourceBack);
    }
}

TEST(HidlUtils, ConvertInvalidStreamType) {
    AudioStreamType invalid;
    EXPECT_EQ(BAD_VALUE, HidlUtils::audioStreamTypeFromHal(kInvalidHalStreamType, &invalid));
    audio_stream_type_t halInvalid;
    EXPECT_EQ(BAD_VALUE, HidlUtils::audioStreamTypeToHal("random string", &halInvalid));
}

TEST(HidlUtils, ConvertStreamType) {
    for (const auto enumVal : xsdc_enum_range<xsd::AudioStreamType>{}) {
        const AudioStreamType streamType = toString(enumVal);
        audio_stream_type_t halStreamType;
        AudioStreamType streamTypeBack;
        EXPECT_EQ(NO_ERROR, HidlUtils::audioStreamTypeToHal(streamType, &halStreamType))
                << "Conversion of \"" << streamType << "\" failed";
        EXPECT_EQ(NO_ERROR, HidlUtils::audioStreamTypeFromHal(halStreamType, &streamTypeBack))
                << "Conversion of stream type " << halStreamType << " failed";
        EXPECT_EQ(streamType, streamTypeBack);
    }
}

TEST(HidlUtils, ConvertInvalidGain) {
    AudioGain invalid;
    EXPECT_EQ(BAD_VALUE, HidlUtils::audioGainFromHal({.mode = kInvalidHalGainMode},
                                                     false /*isInput*/, &invalid));
    EXPECT_EQ(BAD_VALUE, HidlUtils::audioGainFromHal({.mode = kInvalidHalGainMode},
                                                     true /*isInput*/, &invalid));
    struct audio_gain halInvalid;
    invalid.mode.resize(1);
    invalid.mode[0] = "random string";
    EXPECT_EQ(BAD_VALUE, HidlUtils::audioGainToHal(invalid, &halInvalid));
}

TEST(HidlUtils, ConvertGain) {
    AudioGain gain = {};
    gain.channelMask = toString(xsd::AudioChannelMask::AUDIO_CHANNEL_OUT_STEREO);
    struct audio_gain halGain;
    EXPECT_EQ(NO_ERROR, HidlUtils::audioGainToHal(gain, &halGain));
    AudioGain gainBack;
    EXPECT_EQ(NO_ERROR, HidlUtils::audioGainFromHal(halGain, false /*isInput*/, &gainBack));
    EXPECT_EQ(gain, gainBack);
    struct audio_gain halGainBack;
    EXPECT_EQ(NO_ERROR, HidlUtils::audioGainToHal(gainBack, &halGainBack));
    EXPECT_TRUE(audio_gains_are_equal(&halGain, &halGainBack));
}

TEST(HidlUtils, ConvertInvalidGainConfig) {
    AudioGainConfig invalid;
    EXPECT_EQ(BAD_VALUE, HidlUtils::audioGainConfigFromHal({.mode = kInvalidHalGainMode},
                                                           false /*isInput*/, &invalid));
    EXPECT_EQ(BAD_VALUE, HidlUtils::audioGainConfigFromHal({.mode = kInvalidHalGainMode},
                                                           true /*isInput*/, &invalid));
    struct audio_gain_config halInvalid;
    invalid.mode.resize(1);
    invalid.mode[0] = "random string";
    EXPECT_EQ(BAD_VALUE, HidlUtils::audioGainConfigToHal(invalid, &halInvalid));
}

TEST(HidlUtils, ConvertGainConfig) {
    AudioGainConfig gainConfig = {};
    gainConfig.channelMask = toString(xsd::AudioChannelMask::AUDIO_CHANNEL_OUT_STEREO);
    struct audio_gain_config halGainConfig;
    EXPECT_EQ(NO_ERROR, HidlUtils::audioGainConfigToHal(gainConfig, &halGainConfig));
    AudioGainConfig gainConfigBack;
    EXPECT_EQ(NO_ERROR,
              HidlUtils::audioGainConfigFromHal(halGainConfig, false /*isInput*/, &gainConfigBack));
    EXPECT_EQ(gainConfig, gainConfigBack);
    struct audio_gain_config halGainConfigBack;
    EXPECT_EQ(NO_ERROR, HidlUtils::audioGainConfigToHal(gainConfigBack, &halGainConfigBack));
    EXPECT_TRUE(audio_gain_config_are_equal(&halGainConfig, &halGainConfigBack));
}

TEST(HidlUtils, ConvertInvalidUsage) {
    AudioUsage invalid;
    EXPECT_EQ(BAD_VALUE, HidlUtils::audioUsageFromHal(kInvalidHalUsage, &invalid));
    audio_usage_t halInvalid;
    EXPECT_EQ(BAD_VALUE, HidlUtils::audioUsageToHal("random string", &halInvalid));
}

TEST(HidlUtils, ConvertUsage) {
    for (const auto enumVal : xsdc_enum_range<xsd::AudioUsage>{}) {
        const AudioUsage usage = toString(enumVal);
        audio_usage_t halUsage;
        AudioUsage usageBack;
        EXPECT_EQ(NO_ERROR, HidlUtils::audioUsageToHal(usage, &halUsage))
                << "Conversion of \"" << usage << "\" failed";
        EXPECT_EQ(NO_ERROR, HidlUtils::audioUsageFromHal(halUsage, &usageBack))
                << "Conversion of usage " << halUsage << " failed";
        EXPECT_EQ(usage, usageBack);
    }
}

TEST(HidlUtils, ConvertInvalidOffloadInfo) {
    AudioOffloadInfo invalid;
    audio_offload_info_t halInvalid = AUDIO_INFO_INITIALIZER;
    halInvalid.channel_mask = AUDIO_CHANNEL_INVALID;
    halInvalid.format = kInvalidHalFormat;
    EXPECT_EQ(BAD_VALUE, HidlUtils::audioOffloadInfoFromHal(halInvalid, &invalid));
    invalid.base.channelMask = "random string";
    invalid.base.format = "random string";
    EXPECT_EQ(BAD_VALUE, HidlUtils::audioOffloadInfoToHal(invalid, &halInvalid));
}

TEST(HidlUtils, ConvertOffloadInfo) {
    AudioOffloadInfo offloadInfo = {};
    offloadInfo.base.sampleRateHz = 44100;
    offloadInfo.base.channelMask = toString(xsd::AudioChannelMask::AUDIO_CHANNEL_OUT_STEREO);
    offloadInfo.base.format = toString(xsd::AudioFormat::AUDIO_FORMAT_PCM_16_BIT);
    offloadInfo.streamType = toString(xsd::AudioStreamType::AUDIO_STREAM_MUSIC);
    offloadInfo.bitRatePerSecond = 320;
    offloadInfo.durationMicroseconds = -1;
    offloadInfo.bitWidth = 16;
    offloadInfo.bufferSize = 1024;
    offloadInfo.usage = toString(xsd::AudioUsage::AUDIO_USAGE_MEDIA);
    offloadInfo.encapsulationMode = AudioEncapsulationMode::ELEMENTARY_STREAM;
    offloadInfo.contentId = 42;
    offloadInfo.syncId = 13;
    audio_offload_info_t halOffloadInfo;
    EXPECT_EQ(NO_ERROR, HidlUtils::audioOffloadInfoToHal(offloadInfo, &halOffloadInfo));
    AudioOffloadInfo offloadInfoBack;
    EXPECT_EQ(NO_ERROR, HidlUtils::audioOffloadInfoFromHal(halOffloadInfo, &offloadInfoBack));
    EXPECT_EQ(offloadInfo, offloadInfoBack);
}

TEST(HidlUtils, ConvertInvalidConfig) {
    AudioConfig invalid;
    audio_config_t halInvalid = AUDIO_CONFIG_INITIALIZER;
    halInvalid.channel_mask = AUDIO_CHANNEL_INVALID;
    halInvalid.format = kInvalidHalFormat;
    EXPECT_EQ(BAD_VALUE, HidlUtils::audioConfigFromHal(halInvalid, false /*isInput*/, &invalid));
    EXPECT_EQ(BAD_VALUE, HidlUtils::audioConfigFromHal(halInvalid, true /*isInput*/, &invalid));
    invalid.base.channelMask = "random string";
    invalid.base.format = "random string";
    EXPECT_EQ(BAD_VALUE, HidlUtils::audioConfigToHal(invalid, &halInvalid));
}

TEST(HidlUtils, ConvertConfig) {
    AudioConfig config = {};
    config.base.sampleRateHz = 44100;
    config.base.channelMask = toString(xsd::AudioChannelMask::AUDIO_CHANNEL_OUT_STEREO);
    config.base.format = toString(xsd::AudioFormat::AUDIO_FORMAT_PCM_16_BIT);
    config.offloadInfo.base = config.base;
    config.offloadInfo.streamType = toString(xsd::AudioStreamType::AUDIO_STREAM_MUSIC);
    config.offloadInfo.bitRatePerSecond = 320;
    config.offloadInfo.durationMicroseconds = -1;
    config.offloadInfo.bitWidth = 16;
    config.offloadInfo.bufferSize = 1024;
    config.offloadInfo.usage = toString(xsd::AudioUsage::AUDIO_USAGE_MEDIA);
    config.offloadInfo.encapsulationMode = AudioEncapsulationMode::ELEMENTARY_STREAM;
    config.offloadInfo.contentId = 42;
    config.offloadInfo.syncId = 13;
    audio_config_t halConfig;
    EXPECT_EQ(NO_ERROR, HidlUtils::audioConfigToHal(config, &halConfig));
    AudioConfig configBack;
    EXPECT_EQ(NO_ERROR, HidlUtils::audioConfigFromHal(halConfig, false /*isInput*/, &configBack));
    EXPECT_EQ(config, configBack);
}

TEST(HidlUtils, ConvertInvalidAudioProfile) {
    AudioProfile invalid;
    struct audio_profile halInvalid = {};
    halInvalid.format = kInvalidHalFormat;
    halInvalid.num_sample_rates = 0;
    halInvalid.num_channel_masks = 1;
    halInvalid.channel_masks[0] = kInvalidHalChannelMask;
    EXPECT_EQ(BAD_VALUE, HidlUtils::audioProfileFromHal(halInvalid, false /*isInput*/, &invalid));
    EXPECT_EQ(BAD_VALUE, HidlUtils::audioProfileFromHal(halInvalid, true /*isInput*/, &invalid));
    invalid.format = "random string";
    EXPECT_EQ(BAD_VALUE, HidlUtils::audioProfileToHal(invalid, &halInvalid));
}

TEST(HidlUtils, ConvertAudioProfile) {
    AudioProfile profile = {};
    profile.format = toString(xsd::AudioFormat::AUDIO_FORMAT_PCM_16_BIT);
    profile.sampleRates.resize(2);
    profile.sampleRates[0] = 44100;
    profile.sampleRates[1] = 48000;
    profile.channelMasks.resize(2);
    profile.channelMasks[0] = toString(xsd::AudioChannelMask::AUDIO_CHANNEL_OUT_MONO);
    profile.channelMasks[1] = toString(xsd::AudioChannelMask::AUDIO_CHANNEL_OUT_STEREO);
    struct audio_profile halProfile;
    EXPECT_EQ(NO_ERROR, HidlUtils::audioProfileToHal(profile, &halProfile));
    AudioProfile profileBack;
    EXPECT_EQ(NO_ERROR,
              HidlUtils::audioProfileFromHal(halProfile, false /*isInput*/, &profileBack));
    EXPECT_EQ(profile, profileBack);
}

TEST(HidlUtils, ConvertInvalidAudioPortConfig) {
    AudioPortConfig invalid;
    struct audio_port_config halInvalid = {};
    halInvalid.type = AUDIO_PORT_TYPE_MIX;
    halInvalid.role = AUDIO_PORT_ROLE_NONE;  // note: this is valid.
    halInvalid.config_mask = AUDIO_PORT_CONFIG_CHANNEL_MASK;
    halInvalid.channel_mask = AUDIO_CHANNEL_INVALID;
    EXPECT_EQ(BAD_VALUE, HidlUtils::audioPortConfigFromHal(halInvalid, &invalid));
    invalid.base.channelMask = "random string";
    EXPECT_EQ(BAD_VALUE, HidlUtils::audioPortConfigToHal(invalid, &halInvalid));
}

TEST(HidlUtils, ConvertAudioPortConfig) {
    AudioPortConfig config = {};
    config.id = 42;
    config.base.sampleRateHz = 44100;
    config.base.channelMask = toString(xsd::AudioChannelMask::AUDIO_CHANNEL_OUT_STEREO);
    config.base.format = toString(xsd::AudioFormat::AUDIO_FORMAT_PCM_16_BIT);
    config.gain.config({});
    config.gain.config().channelMask = toString(xsd::AudioChannelMask::AUDIO_CHANNEL_OUT_STEREO);
    config.ext.device({});
    config.ext.device().deviceType = toString(xsd::AudioDevice::AUDIO_DEVICE_OUT_SPEAKER);
    struct audio_port_config halConfig;
    EXPECT_EQ(NO_ERROR, HidlUtils::audioPortConfigToHal(config, &halConfig));
    AudioPortConfig configBack;
    EXPECT_EQ(NO_ERROR, HidlUtils::audioPortConfigFromHal(halConfig, &configBack));
    EXPECT_EQ(config, configBack);
    struct audio_port_config halConfigBack;
    EXPECT_EQ(NO_ERROR, HidlUtils::audioPortConfigToHal(configBack, &halConfigBack));
    EXPECT_TRUE(audio_port_configs_are_equal(&halConfig, &halConfigBack));
}

TEST(HidlUtils, ConvertInvalidAudioPort) {
    AudioPort invalid;
    struct audio_port_v7 halInvalid = {};
    halInvalid.type = AUDIO_PORT_TYPE_MIX;
    halInvalid.role = AUDIO_PORT_ROLE_NONE;  // note: this is valid.
    halInvalid.num_audio_profiles = 1;
    halInvalid.audio_profiles[0].format = kInvalidHalFormat;
    EXPECT_EQ(BAD_VALUE, HidlUtils::audioPortFromHal(halInvalid, &invalid));
    invalid.profiles.resize(1);
    invalid.profiles[0].format = "random string";
    EXPECT_EQ(BAD_VALUE, HidlUtils::audioPortToHal(invalid, &halInvalid));
}

TEST(HidlUtils, ConvertAudioPort) {
    AudioPort port = {};
    port.id = 42;
    port.name = "test";
    port.profiles.resize(1);
    port.profiles[0].format = toString(xsd::AudioFormat::AUDIO_FORMAT_PCM_16_BIT);
    port.profiles[0].sampleRates.resize(2);
    port.profiles[0].sampleRates[0] = 44100;
    port.profiles[0].sampleRates[1] = 48000;
    port.profiles[0].channelMasks.resize(2);
    port.profiles[0].channelMasks[0] = toString(xsd::AudioChannelMask::AUDIO_CHANNEL_OUT_MONO);
    port.profiles[0].channelMasks[1] = toString(xsd::AudioChannelMask::AUDIO_CHANNEL_OUT_STEREO);
    port.gains.resize(1);
    port.gains[0].channelMask = toString(xsd::AudioChannelMask::AUDIO_CHANNEL_OUT_STEREO);
    port.ext.device({});
    port.ext.device().deviceType = toString(xsd::AudioDevice::AUDIO_DEVICE_OUT_SPEAKER);
    // active config left unspecified.
    struct audio_port_v7 halPort;
    EXPECT_EQ(NO_ERROR, HidlUtils::audioPortToHal(port, &halPort));
    AudioPort portBack;
    EXPECT_EQ(NO_ERROR, HidlUtils::audioPortFromHal(halPort, &portBack));
    EXPECT_EQ(port, portBack);
    struct audio_port_v7 halPortBack;
    EXPECT_EQ(NO_ERROR, HidlUtils::audioPortToHal(portBack, &halPortBack));
    EXPECT_TRUE(audio_ports_v7_are_equal(&halPort, &halPortBack));
}
