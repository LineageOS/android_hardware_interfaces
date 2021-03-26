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
using ::android::hardware::hidl_vec;
using namespace ::android::hardware::audio::common::CPP_VERSION;
using ::android::hardware::audio::common::CPP_VERSION::implementation::HidlUtils;
namespace xsd {
using namespace ::android::audio::policy::configuration::V7_0;
}

static constexpr audio_channel_mask_t kInvalidHalChannelMask = AUDIO_CHANNEL_INVALID;
static constexpr audio_content_type_t kInvalidHalContentType =
        static_cast<audio_content_type_t>(0xFFFFFFFFU);
static constexpr audio_devices_t kInvalidHalDevice = static_cast<audio_devices_t>(0xFFFFFFFFU);
static constexpr audio_format_t kInvalidHalFormat = AUDIO_FORMAT_INVALID;
static constexpr audio_gain_mode_t kInvalidHalGainMode =
        static_cast<audio_gain_mode_t>(0xFFFFFFFFU);
// AUDIO_SOURCE_INVALID is framework-only.
static constexpr audio_source_t kInvalidHalSource = static_cast<audio_source_t>(-1);
// AUDIO_STREAM_DEFAULT is framework-only
static constexpr audio_stream_type_t kInvalidHalStreamType = static_cast<audio_stream_type_t>(-2);
static constexpr audio_usage_t kInvalidHalUsage = static_cast<audio_usage_t>(0xFFFFFFFFU);
static constexpr audio_encapsulation_type_t kInvalidEncapsulationType =
        static_cast<audio_encapsulation_type_t>(0xFFFFFFFFU);
static constexpr audio_standard_t kInvalidAudioStandard =
        static_cast<audio_standard_t>(0xFFFFFFFFU);

TEST(HidlUtils, ConvertInvalidChannelMask) {
    AudioChannelMask invalid;
    EXPECT_EQ(BAD_VALUE, HidlUtils::audioChannelMaskFromHal(kInvalidHalChannelMask,
                                                            false /*isInput*/, &invalid));
    EXPECT_EQ(BAD_VALUE, HidlUtils::audioChannelMaskFromHal(kInvalidHalChannelMask,
                                                            true /*isInput*/, &invalid));
    audio_channel_mask_t halInvalid;
    EXPECT_EQ(BAD_VALUE, HidlUtils::audioChannelMaskToHal("", &halInvalid));
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

TEST(HidlUtils, ConvertInvalidChannelMasksFromHal) {
    std::vector<std::string> validAndInvalidChannelMasks = {
            toString(xsd::AudioChannelMask::AUDIO_CHANNEL_OUT_STEREO), "random string", ""};
    hidl_vec<AudioChannelMask> validChannelMask;
    EXPECT_EQ(BAD_VALUE,
              HidlUtils::audioChannelMasksFromHal(validAndInvalidChannelMasks, &validChannelMask));
    EXPECT_EQ(1, validChannelMask.size());
    EXPECT_EQ(validAndInvalidChannelMasks[0], validChannelMask[0]);

    std::vector<std::string> invalidChannelMasks = {"random string", ""};
    hidl_vec<AudioChannelMask> empty;
    EXPECT_EQ(BAD_VALUE, HidlUtils::audioChannelMasksFromHal(invalidChannelMasks, &empty));
    EXPECT_EQ(0, empty.size());
}

TEST(HidlUtils, ConvertChannelMasksFromHal) {
    std::vector<std::string> allHalChannelMasks;
    for (const auto enumVal : xsdc_enum_range<xsd::AudioChannelMask>{}) {
        allHalChannelMasks.push_back(toString(enumVal));
    }
    hidl_vec<AudioChannelMask> allChannelMasks;
    EXPECT_EQ(NO_ERROR, HidlUtils::audioChannelMasksFromHal(allHalChannelMasks, &allChannelMasks));
    EXPECT_EQ(allHalChannelMasks.size(), allChannelMasks.size());
    for (size_t i = 0; i < allHalChannelMasks.size(); ++i) {
        EXPECT_EQ(allHalChannelMasks[i], allChannelMasks[i]);
    }
}

static AudioConfigBase generateValidConfigBase(bool isInput) {
    AudioConfigBase configBase;
    configBase.sampleRateHz = 44100;
    configBase.format = toString(xsd::AudioFormat::AUDIO_FORMAT_PCM_16_BIT);
    configBase.channelMask = isInput ? toString(xsd::AudioChannelMask::AUDIO_CHANNEL_IN_STEREO)
                                     : toString(xsd::AudioChannelMask::AUDIO_CHANNEL_OUT_STEREO);
    return configBase;
}

TEST(HidlUtils, ConvertInvalidConfigBase) {
    AudioConfigBase invalid;
    audio_config_base_t halInvalidChannelMask = AUDIO_CONFIG_BASE_INITIALIZER;
    halInvalidChannelMask.channel_mask = kInvalidHalChannelMask;
    EXPECT_EQ(BAD_VALUE, HidlUtils::audioConfigBaseFromHal(halInvalidChannelMask, false /*isInput*/,
                                                           &invalid));
    EXPECT_EQ(BAD_VALUE,
              HidlUtils::audioConfigBaseFromHal(halInvalidChannelMask, true /*isInput*/, &invalid));
    audio_config_base_t halInvalidFormat = AUDIO_CONFIG_BASE_INITIALIZER;
    halInvalidFormat.format = kInvalidHalFormat;
    EXPECT_EQ(BAD_VALUE,
              HidlUtils::audioConfigBaseFromHal(halInvalidFormat, false /*isInput*/, &invalid));
    EXPECT_EQ(BAD_VALUE,
              HidlUtils::audioConfigBaseFromHal(halInvalidFormat, true /*isInput*/, &invalid));

    audio_config_base_t halInvalid;
    AudioConfigBase invalidChannelMask = generateValidConfigBase(false /*isInput*/);
    invalidChannelMask.channelMask = "random string";
    EXPECT_EQ(BAD_VALUE, HidlUtils::audioConfigBaseToHal(invalidChannelMask, &halInvalid));
    AudioConfigBase invalidFormat = generateValidConfigBase(false /*isInput*/);
    invalidFormat.format = "random string";
    EXPECT_EQ(BAD_VALUE, HidlUtils::audioConfigBaseToHal(invalidFormat, &halInvalid));
}

TEST(HidlUtils, ConvertConfigBaseDefault) {
    audio_config_base_t halBaseDefault = AUDIO_CONFIG_BASE_INITIALIZER;
    AudioConfigBase baseDefaultOut, baseDefaultIn;
    EXPECT_EQ(NO_ERROR, HidlUtils::audioConfigBaseFromHal(halBaseDefault, false /*isInput*/,
                                                          &baseDefaultOut));
    EXPECT_EQ(NO_ERROR,
              HidlUtils::audioConfigBaseFromHal(halBaseDefault, true /*isInput*/, &baseDefaultIn));
    EXPECT_EQ(baseDefaultOut, baseDefaultIn);
    audio_config_base_t halBaseDefaultBack;
    EXPECT_EQ(NO_ERROR, HidlUtils::audioConfigBaseToHal(baseDefaultOut, &halBaseDefaultBack));
    EXPECT_EQ(halBaseDefault.sample_rate, halBaseDefaultBack.sample_rate);
    EXPECT_EQ(halBaseDefault.channel_mask, halBaseDefaultBack.channel_mask);
    EXPECT_EQ(halBaseDefault.format, halBaseDefaultBack.format);
}

TEST(HidlUtils, ConvertConfigBase) {
    AudioConfigBase configBaseOut = generateValidConfigBase(false /*isInput*/);
    audio_config_base_t halConfigBaseOut;
    EXPECT_EQ(NO_ERROR, HidlUtils::audioConfigBaseToHal(configBaseOut, &halConfigBaseOut));
    AudioConfigBase configBaseOutBack;
    EXPECT_EQ(NO_ERROR, HidlUtils::audioConfigBaseFromHal(halConfigBaseOut, false /*isInput*/,
                                                          &configBaseOutBack));
    EXPECT_EQ(configBaseOut, configBaseOutBack);

    AudioConfigBase configBaseIn = generateValidConfigBase(true /*isInput*/);
    audio_config_base_t halConfigBaseIn;
    EXPECT_EQ(NO_ERROR, HidlUtils::audioConfigBaseToHal(configBaseIn, &halConfigBaseIn));
    AudioConfigBase configBaseInBack;
    EXPECT_EQ(NO_ERROR, HidlUtils::audioConfigBaseFromHal(halConfigBaseIn, true /*isInput*/,
                                                          &configBaseInBack));
    EXPECT_EQ(configBaseIn, configBaseInBack);
}

TEST(HidlUtils, ConvertInvalidConfigBaseOptional) {
    AudioConfigBaseOptional invalid;
    audio_config_base_t halInvalidChannelMask = AUDIO_CONFIG_BASE_INITIALIZER;
    halInvalidChannelMask.channel_mask = kInvalidHalChannelMask;
    EXPECT_EQ(BAD_VALUE,
              HidlUtils::audioConfigBaseOptionalFromHal(
                      halInvalidChannelMask, false /*isInput*/, false /*formatSpecified*/,
                      false /*sampleRateSpecified*/, true /*channelMaskSpecified*/, &invalid));
    EXPECT_EQ(BAD_VALUE,
              HidlUtils::audioConfigBaseOptionalFromHal(
                      halInvalidChannelMask, true /*isInput*/, false /*formatSpecified*/,
                      false /*sampleRateSpecified*/, true /*channelMaskSpecified*/, &invalid));
    // Unspecified invalid values are ignored
    AudioConfigBaseOptional unspecified;
    EXPECT_EQ(NO_ERROR,
              HidlUtils::audioConfigBaseOptionalFromHal(
                      halInvalidChannelMask, false /*isInput*/, false /*formatSpecified*/,
                      false /*sampleRateSpecified*/, false /*channelMaskSpecified*/, &unspecified));
    EXPECT_EQ(NO_ERROR,
              HidlUtils::audioConfigBaseOptionalFromHal(
                      halInvalidChannelMask, true /*isInput*/, false /*formatSpecified*/,
                      false /*sampleRateSpecified*/, false /*channelMaskSpecified*/, &unspecified));
    audio_config_base_t halInvalidFormat = AUDIO_CONFIG_BASE_INITIALIZER;
    halInvalidFormat.format = kInvalidHalFormat;
    EXPECT_EQ(BAD_VALUE,
              HidlUtils::audioConfigBaseOptionalFromHal(
                      halInvalidFormat, false /*isInput*/, true /*formatSpecified*/,
                      false /*sampleRateSpecified*/, false /*channelMaskSpecified*/, &invalid));
    EXPECT_EQ(BAD_VALUE,
              HidlUtils::audioConfigBaseOptionalFromHal(
                      halInvalidFormat, true /*isInput*/, true /*formatSpecified*/,
                      false /*sampleRateSpecified*/, false /*channelMaskSpecified*/, &invalid));
    EXPECT_EQ(NO_ERROR,
              HidlUtils::audioConfigBaseOptionalFromHal(
                      halInvalidFormat, false /*isInput*/, false /*formatSpecified*/,
                      false /*sampleRateSpecified*/, false /*channelMaskSpecified*/, &unspecified));
    EXPECT_EQ(NO_ERROR,
              HidlUtils::audioConfigBaseOptionalFromHal(
                      halInvalidFormat, true /*isInput*/, false /*formatSpecified*/,
                      false /*sampleRateSpecified*/, false /*channelMaskSpecified*/, &unspecified));

    audio_config_base_t halInvalid;
    AudioConfigBaseOptional invalidChannelMask;
    bool formatSpecified, sampleRateSpecified, channelMaskSpecified;
    invalidChannelMask.channelMask.value("random string");
    EXPECT_EQ(BAD_VALUE, HidlUtils::audioConfigBaseOptionalToHal(
                                 invalidChannelMask, &halInvalid, &formatSpecified,
                                 &sampleRateSpecified, &channelMaskSpecified));
    AudioConfigBaseOptional invalidFormat;
    invalidFormat.format.value("random string");
    EXPECT_EQ(BAD_VALUE,
              HidlUtils::audioConfigBaseOptionalToHal(invalidFormat, &halInvalid, &formatSpecified,
                                                      &sampleRateSpecified, &channelMaskSpecified));
}

TEST(HidlUtils, ConvertConfigBaseOptionalDefault) {
    audio_config_base_t halBaseDefault = AUDIO_CONFIG_BASE_INITIALIZER;
    AudioConfigBaseOptional baseDefaultUnspecOut, baseDefaultUnspecIn;
    EXPECT_EQ(NO_ERROR, HidlUtils::audioConfigBaseOptionalFromHal(
                                halBaseDefault, false /*isInput*/, false /*formatSpecified*/,
                                false /*sampleRateSpecified*/, false /*channelMaskSpecified*/,
                                &baseDefaultUnspecOut));
    EXPECT_EQ(NO_ERROR, HidlUtils::audioConfigBaseOptionalFromHal(
                                halBaseDefault, true /*isInput*/, false /*formatSpecified*/,
                                false /*sampleRateSpecified*/, false /*channelMaskSpecified*/,
                                &baseDefaultUnspecIn));
    EXPECT_EQ(baseDefaultUnspecOut, baseDefaultUnspecIn);
    audio_config_base_t halBaseDefaultUnspecBack = AUDIO_CONFIG_BASE_INITIALIZER;
    bool formatSpecified, sampleRateSpecified, channelMaskSpecified;
    EXPECT_EQ(NO_ERROR, HidlUtils::audioConfigBaseOptionalToHal(
                                baseDefaultUnspecOut, &halBaseDefaultUnspecBack, &formatSpecified,
                                &sampleRateSpecified, &channelMaskSpecified));
    EXPECT_FALSE(formatSpecified);
    EXPECT_FALSE(sampleRateSpecified);
    EXPECT_FALSE(channelMaskSpecified);
    EXPECT_EQ(halBaseDefault.sample_rate, halBaseDefaultUnspecBack.sample_rate);
    EXPECT_EQ(halBaseDefault.channel_mask, halBaseDefaultUnspecBack.channel_mask);
    EXPECT_EQ(halBaseDefault.format, halBaseDefaultUnspecBack.format);

    AudioConfigBaseOptional baseDefaultSpecOut, baseDefaultSpecIn;
    EXPECT_EQ(NO_ERROR, HidlUtils::audioConfigBaseOptionalFromHal(
                                halBaseDefault, false /*isInput*/, true /*formatSpecified*/,
                                true /*sampleRateSpecified*/, true /*channelMaskSpecified*/,
                                &baseDefaultSpecOut));
    EXPECT_EQ(NO_ERROR, HidlUtils::audioConfigBaseOptionalFromHal(
                                halBaseDefault, true /*isInput*/, true /*formatSpecified*/,
                                true /*sampleRateSpecified*/, true /*channelMaskSpecified*/,
                                &baseDefaultSpecIn));
    EXPECT_EQ(baseDefaultSpecOut, baseDefaultSpecIn);
    audio_config_base_t halBaseDefaultSpecBack;
    EXPECT_EQ(NO_ERROR, HidlUtils::audioConfigBaseOptionalToHal(
                                baseDefaultSpecOut, &halBaseDefaultSpecBack, &formatSpecified,
                                &sampleRateSpecified, &channelMaskSpecified));
    EXPECT_TRUE(formatSpecified);
    EXPECT_TRUE(sampleRateSpecified);
    EXPECT_TRUE(channelMaskSpecified);
    EXPECT_EQ(halBaseDefault.sample_rate, halBaseDefaultSpecBack.sample_rate);
    EXPECT_EQ(halBaseDefault.channel_mask, halBaseDefaultSpecBack.channel_mask);
    EXPECT_EQ(halBaseDefault.format, halBaseDefaultSpecBack.format);
}

TEST(HidlUtils, ConvertConfigBaseOptionalEmpty) {
    AudioConfigBaseOptional empty;
    bool formatSpecified, sampleRateSpecified, channelMaskSpecified;
    audio_config_base_t halEmpty = AUDIO_CONFIG_BASE_INITIALIZER;
    EXPECT_EQ(NO_ERROR,
              HidlUtils::audioConfigBaseOptionalToHal(empty, &halEmpty, &formatSpecified,
                                                      &sampleRateSpecified, &channelMaskSpecified));
    EXPECT_FALSE(formatSpecified);
    EXPECT_FALSE(sampleRateSpecified);
    EXPECT_FALSE(channelMaskSpecified);
    AudioConfigBaseOptional emptyOutBack, emptyInBack;
    EXPECT_EQ(NO_ERROR, HidlUtils::audioConfigBaseOptionalFromHal(
                                halEmpty, false /*isInput*/, formatSpecified, sampleRateSpecified,
                                channelMaskSpecified, &emptyOutBack));
    EXPECT_EQ(NO_ERROR, HidlUtils::audioConfigBaseOptionalFromHal(
                                halEmpty, true /*isInput*/, formatSpecified, sampleRateSpecified,
                                channelMaskSpecified, &emptyInBack));
    EXPECT_EQ(emptyOutBack, emptyInBack);
    EXPECT_EQ(empty, emptyOutBack);
}

TEST(HidlUtils, ConvertConfigBaseOptional) {
    AudioConfigBase validBaseOut = generateValidConfigBase(false /*isInput*/);
    AudioConfigBaseOptional configBaseOut;
    configBaseOut.format.value(validBaseOut.format);
    configBaseOut.sampleRateHz.value(validBaseOut.sampleRateHz);
    configBaseOut.channelMask.value(validBaseOut.channelMask);
    audio_config_base_t halConfigBaseOut;
    bool formatSpecified, sampleRateSpecified, channelMaskSpecified;
    EXPECT_EQ(NO_ERROR, HidlUtils::audioConfigBaseOptionalToHal(
                                configBaseOut, &halConfigBaseOut, &formatSpecified,
                                &sampleRateSpecified, &channelMaskSpecified));
    EXPECT_TRUE(formatSpecified);
    EXPECT_TRUE(sampleRateSpecified);
    EXPECT_TRUE(channelMaskSpecified);
    AudioConfigBaseOptional configBaseOutBack;
    EXPECT_EQ(NO_ERROR, HidlUtils::audioConfigBaseOptionalFromHal(
                                halConfigBaseOut, false /*isInput*/, formatSpecified,
                                sampleRateSpecified, channelMaskSpecified, &configBaseOutBack));
    EXPECT_EQ(configBaseOut, configBaseOutBack);

    AudioConfigBase validBaseIn = generateValidConfigBase(true /*isInput*/);
    AudioConfigBaseOptional configBaseIn;
    configBaseIn.format.value(validBaseIn.format);
    configBaseIn.sampleRateHz.value(validBaseIn.sampleRateHz);
    configBaseIn.channelMask.value(validBaseIn.channelMask);
    audio_config_base_t halConfigBaseIn;
    formatSpecified = false;
    sampleRateSpecified = false;
    channelMaskSpecified = false;
    EXPECT_EQ(NO_ERROR, HidlUtils::audioConfigBaseOptionalToHal(
                                configBaseIn, &halConfigBaseIn, &formatSpecified,
                                &sampleRateSpecified, &channelMaskSpecified));
    EXPECT_TRUE(formatSpecified);
    EXPECT_TRUE(sampleRateSpecified);
    EXPECT_TRUE(channelMaskSpecified);
    AudioConfigBaseOptional configBaseInBack;
    EXPECT_EQ(NO_ERROR, HidlUtils::audioConfigBaseOptionalFromHal(
                                halConfigBaseIn, true /*isInput*/, formatSpecified,
                                sampleRateSpecified, channelMaskSpecified, &configBaseInBack));
    EXPECT_EQ(configBaseIn, configBaseInBack);
}

TEST(HidlUtils, ConvertInvalidContentType) {
    AudioContentType invalid;
    EXPECT_EQ(BAD_VALUE, HidlUtils::audioContentTypeFromHal(kInvalidHalContentType, &invalid));
    audio_content_type_t halInvalid;
    EXPECT_EQ(BAD_VALUE, HidlUtils::audioContentTypeToHal("", &halInvalid));
    EXPECT_EQ(BAD_VALUE, HidlUtils::audioContentTypeToHal("random string", &halInvalid));
}

TEST(HidlUtils, ConvertContentType) {
    for (const auto enumVal : xsdc_enum_range<xsd::AudioContentType>{}) {
        const AudioContentType contentType = toString(enumVal);
        audio_content_type_t halContentType;
        AudioContentType contentTypeBack;
        EXPECT_EQ(NO_ERROR, HidlUtils::audioContentTypeToHal(contentType, &halContentType))
                << "Conversion of \"" << contentType << "\" failed";
        EXPECT_EQ(NO_ERROR, HidlUtils::audioContentTypeFromHal(halContentType, &contentTypeBack))
                << "Conversion of content type " << halContentType << " failed";
        EXPECT_EQ(contentType, contentTypeBack);
    }
}

TEST(HidlUtils, ConvertInvalidDeviceType) {
    AudioDevice invalid;
    EXPECT_EQ(BAD_VALUE, HidlUtils::audioDeviceTypeFromHal(kInvalidHalDevice, &invalid));
    audio_devices_t halInvalid;
    EXPECT_EQ(BAD_VALUE, HidlUtils::audioDeviceTypeToHal("", &halInvalid));
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
    EXPECT_TRUE(xsd::isVendorExtension("VX_QCM_SPK"));
    EXPECT_FALSE(xsd::isVendorExtension(""));
    EXPECT_FALSE(xsd::isVendorExtension("random string"));
    EXPECT_FALSE(xsd::isVendorExtension("VX_"));
    EXPECT_FALSE(xsd::isVendorExtension("VX_X"));
    EXPECT_FALSE(xsd::isVendorExtension("VX_X_"));
    EXPECT_FALSE(xsd::isVendorExtension("VX_X_X"));
    EXPECT_FALSE(xsd::isVendorExtension("VX_XX_X"));
    EXPECT_FALSE(xsd::isVendorExtension("VX_GOOGLE_$$"));
    EXPECT_FALSE(xsd::isVendorExtension("VX_$CM_SPK"));
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

    DeviceAddress micWithAddress;
    micWithAddress.deviceType = toString(xsd::AudioDevice::AUDIO_DEVICE_IN_BUILTIN_MIC);
    micWithAddress.address.id("bottom");
    ConvertDeviceAddress(micWithAddress);
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
    EXPECT_EQ(BAD_VALUE, HidlUtils::audioFormatToHal("", &halInvalid));
    // INVALID format is not in XSD thus it's not allowed for transfer over HIDL.
    EXPECT_EQ(BAD_VALUE, HidlUtils::audioFormatToHal("AUDIO_FORMAT_INVALID", &halInvalid));
    EXPECT_EQ(BAD_VALUE, HidlUtils::audioFormatToHal("random string", &halInvalid));
}

TEST(HidlUtils, ConvertFormat) {
    for (const auto enumVal : xsdc_enum_range<xsd::AudioFormat>{}) {
        const AudioFormat format = toString(enumVal);
        audio_format_t halFormat;
        AudioFormat formatBack;
        EXPECT_EQ(NO_ERROR, HidlUtils::audioFormatToHal(format, &halFormat))
                << "Conversion of \"" << format << "\" failed";
        EXPECT_EQ(enumVal != xsd::AudioFormat::AUDIO_FORMAT_DEFAULT,
                  audio_is_valid_format(halFormat))
                << "Validity of \"" << format << "\" is not as expected";
        EXPECT_EQ(NO_ERROR, HidlUtils::audioFormatFromHal(halFormat, &formatBack))
                << "Conversion of format " << halFormat << " failed";
        EXPECT_EQ(format, formatBack);
    }
}

TEST(HidlUtils, ConvertInvalidFormatsFromHal) {
    std::vector<std::string> validAndInvalidFormats = {
            toString(xsd::AudioFormat::AUDIO_FORMAT_PCM_16_BIT), "random string", ""};
    hidl_vec<AudioFormat> validFormat;
    EXPECT_EQ(BAD_VALUE, HidlUtils::audioFormatsFromHal(validAndInvalidFormats, &validFormat));
    EXPECT_EQ(1, validFormat.size());
    EXPECT_EQ(validAndInvalidFormats[0], validFormat[0]);

    std::vector<std::string> invalidFormats = {"random string", ""};
    hidl_vec<AudioFormat> empty;
    EXPECT_EQ(BAD_VALUE, HidlUtils::audioFormatsFromHal(invalidFormats, &empty));
    EXPECT_EQ(0, empty.size());
}

TEST(HidlUtils, ConvertFormatsFromHal) {
    std::vector<std::string> allHalFormats;
    for (const auto enumVal : xsdc_enum_range<xsd::AudioFormat>{}) {
        allHalFormats.push_back(toString(enumVal));
    }
    hidl_vec<AudioFormat> allFormats;
    EXPECT_EQ(NO_ERROR, HidlUtils::audioFormatsFromHal(allHalFormats, &allFormats));
    EXPECT_EQ(allHalFormats.size(), allFormats.size());
    for (size_t i = 0; i < allHalFormats.size(); ++i) {
        EXPECT_EQ(allHalFormats[i], allFormats[i]);
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
    EXPECT_EQ(BAD_VALUE, HidlUtils::audioSourceToHal("", &halInvalid));
    // INVALID source is not in XSD thus it's not allowed for transfer over HIDL.
    EXPECT_EQ(BAD_VALUE, HidlUtils::audioSourceToHal("AUDIO_SOURCE_INVALID", &halInvalid));
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

TEST(HidlUtils, ConvertDefaultStreamType) {
    AudioStreamType streamDefault = "";
    audio_stream_type_t halStreamDefault;
    EXPECT_EQ(NO_ERROR, HidlUtils::audioStreamTypeToHal(streamDefault, &halStreamDefault));
    AudioStreamType streamDefaultBack;
    EXPECT_EQ(NO_ERROR, HidlUtils::audioStreamTypeFromHal(halStreamDefault, &streamDefaultBack));
    EXPECT_EQ(streamDefault, streamDefaultBack);
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
    EXPECT_EQ(BAD_VALUE, HidlUtils::audioUsageToHal("", &halInvalid));
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
    halInvalid.channel_mask = kInvalidHalChannelMask;
    halInvalid.format = kInvalidHalFormat;
    EXPECT_EQ(BAD_VALUE, HidlUtils::audioOffloadInfoFromHal(halInvalid, &invalid));
    invalid.base.channelMask = "random string";
    invalid.base.format = "random string";
    EXPECT_EQ(BAD_VALUE, HidlUtils::audioOffloadInfoToHal(invalid, &halInvalid));
}

TEST(HidlUtils, ConvertOffloadInfo) {
    AudioOffloadInfo offloadInfo = {};
    offloadInfo.base = generateValidConfigBase(false /*isInput*/);
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
    audio_config_t halInvalidChannelMask = AUDIO_CONFIG_INITIALIZER;
    halInvalidChannelMask.channel_mask = kInvalidHalChannelMask;
    EXPECT_EQ(BAD_VALUE,
              HidlUtils::audioConfigFromHal(halInvalidChannelMask, false /*isInput*/, &invalid));
    EXPECT_EQ(BAD_VALUE,
              HidlUtils::audioConfigFromHal(halInvalidChannelMask, true /*isInput*/, &invalid));
    audio_config_t halInvalidFormat = AUDIO_CONFIG_INITIALIZER;
    halInvalidFormat.format = kInvalidHalFormat;
    EXPECT_EQ(BAD_VALUE,
              HidlUtils::audioConfigFromHal(halInvalidFormat, false /*isInput*/, &invalid));
    EXPECT_EQ(BAD_VALUE,
              HidlUtils::audioConfigFromHal(halInvalidFormat, true /*isInput*/, &invalid));

    AudioConfig invalidChannelMask;
    audio_config_t halInvalid;
    invalidChannelMask.base.channelMask = "random string";
    invalidChannelMask.base.format = toString(xsd::AudioFormat::AUDIO_FORMAT_DEFAULT);
    EXPECT_EQ(BAD_VALUE, HidlUtils::audioConfigToHal(invalidChannelMask, &halInvalid));
    AudioConfig invalidFormat;
    invalidFormat.base.format = "random string";
    invalidFormat.base.channelMask = toString(xsd::AudioChannelMask::AUDIO_CHANNEL_NONE);
    EXPECT_EQ(BAD_VALUE, HidlUtils::audioConfigToHal(invalidFormat, &halInvalid));
}

TEST(HidlUtils, ConvertConfigDefault) {
    audio_config_t halDefault = AUDIO_CONFIG_INITIALIZER;
    AudioConfig defaultOut, defaultIn;
    EXPECT_EQ(NO_ERROR, HidlUtils::audioConfigFromHal(halDefault, false /*isInput*/, &defaultOut));
    EXPECT_EQ(NO_ERROR, HidlUtils::audioConfigFromHal(halDefault, true /*isInput*/, &defaultIn));
    EXPECT_EQ(defaultOut, defaultIn);
    EXPECT_EQ(NO_ERROR, HidlUtils::audioConfigToHal(defaultOut, &halDefault));

    // Note: empty channel mask and config are not valid values.
    AudioConfig defaultCfg{};
    defaultCfg.base.channelMask = toString(xsd::AudioChannelMask::AUDIO_CHANNEL_NONE);
    defaultCfg.base.format = toString(xsd::AudioFormat::AUDIO_FORMAT_DEFAULT);
    audio_config_t halDefaultCfg;
    EXPECT_EQ(NO_ERROR, HidlUtils::audioConfigToHal(defaultCfg, &halDefaultCfg));
    AudioConfig defaultCfgBackOut, defaultCfgBackIn;
    EXPECT_EQ(NO_ERROR,
              HidlUtils::audioConfigFromHal(halDefaultCfg, false /*isInput*/, &defaultCfgBackOut));
    EXPECT_EQ(NO_ERROR,
              HidlUtils::audioConfigFromHal(halDefaultCfg, true /*isInput*/, &defaultCfgBackIn));
    EXPECT_EQ(defaultCfgBackOut, defaultCfgBackIn);
    EXPECT_EQ(defaultCfg, defaultCfgBackOut);
}

TEST(HidlUtils, ConvertConfig) {
    AudioConfig configOut{};
    configOut.base = generateValidConfigBase(false /*isInput*/);
    audio_config_t halConfigOut;
    EXPECT_EQ(NO_ERROR, HidlUtils::audioConfigToHal(configOut, &halConfigOut));
    AudioConfig configOutBack;
    EXPECT_EQ(NO_ERROR,
              HidlUtils::audioConfigFromHal(halConfigOut, false /*isInput*/, &configOutBack));
    EXPECT_EQ(configOut, configOutBack);

    AudioConfig configIn{};
    configIn.base = generateValidConfigBase(true /*isInput*/);
    audio_config_t halConfigIn;
    EXPECT_EQ(NO_ERROR, HidlUtils::audioConfigToHal(configIn, &halConfigIn));
    AudioConfig configInBack;
    EXPECT_EQ(NO_ERROR,
              HidlUtils::audioConfigFromHal(halConfigIn, true /*isInput*/, &configInBack));
    EXPECT_EQ(configIn, configInBack);
}

TEST(HidlUtils, ConvertConfigWithOffloadInfo) {
    AudioConfig config = {};
    config.base = generateValidConfigBase(false /*isInput*/);
    config.offloadInfo.info(
            AudioOffloadInfo{.base = config.base,
                             .streamType = toString(xsd::AudioStreamType::AUDIO_STREAM_MUSIC),
                             .bitRatePerSecond = 320,
                             .durationMicroseconds = -1,
                             .bitWidth = 16,
                             .bufferSize = 1024,
                             .usage = toString(xsd::AudioUsage::AUDIO_USAGE_MEDIA),
                             .encapsulationMode = AudioEncapsulationMode::ELEMENTARY_STREAM,
                             .contentId = 42,
                             .syncId = 13});
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
    halInvalid.channel_mask = kInvalidHalChannelMask;
    EXPECT_EQ(BAD_VALUE, HidlUtils::audioPortConfigFromHal(halInvalid, &invalid));
    invalid.base.channelMask.value("random string");
    EXPECT_EQ(BAD_VALUE, HidlUtils::audioPortConfigToHal(invalid, &halInvalid));
}

TEST(HidlUtils, ConvertAudioPortConfig) {
    AudioPortConfig config = {};
    config.id = 42;
    config.base.sampleRateHz.value(44100);
    config.base.channelMask.value(toString(xsd::AudioChannelMask::AUDIO_CHANNEL_OUT_STEREO));
    config.base.format.value(toString(xsd::AudioFormat::AUDIO_FORMAT_PCM_16_BIT));
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

TEST(HidlUtils, ConvertInvalidAudioTransports) {
    hidl_vec<AudioTransport> invalid;
    struct audio_port_v7 halInvalid = {};
    halInvalid.num_audio_profiles = 1;
    halInvalid.audio_profiles[0].format = kInvalidHalFormat;
    halInvalid.audio_profiles[0].encapsulation_type = kInvalidEncapsulationType;
    halInvalid.num_extra_audio_descriptors = 1;
    halInvalid.extra_audio_descriptors[0].standard = kInvalidAudioStandard;
    halInvalid.extra_audio_descriptors[0].descriptor_length = EXTRA_AUDIO_DESCRIPTOR_SIZE + 1;
    EXPECT_EQ(BAD_VALUE,
              HidlUtils::audioTransportsFromHal(halInvalid, false /*isInput*/, &invalid));
    invalid.resize(2);
    AudioProfile invalidProfile;
    invalidProfile.format = "random string";
    invalid[0].audioCapability.profile(invalidProfile);
    invalid[0].encapsulationType = "random string";
    invalid[0].audioCapability.edid(hidl_vec<uint8_t>(EXTRA_AUDIO_DESCRIPTOR_SIZE + 1));
    invalid[1].encapsulationType = "random string";
    EXPECT_EQ(BAD_VALUE, HidlUtils::audioTransportsToHal(invalid, &halInvalid));
}

TEST(HidlUtils, ConvertAudioTransports) {
    hidl_vec<AudioTransport> transports;
    transports.resize(2);
    AudioProfile profile;
    profile.format = toString(xsd::AudioFormat::AUDIO_FORMAT_PCM_16_BIT);
    profile.sampleRates.resize(2);
    profile.sampleRates[0] = 44100;
    profile.sampleRates[1] = 48000;
    profile.channelMasks.resize(2);
    profile.channelMasks[0] = toString(xsd::AudioChannelMask::AUDIO_CHANNEL_OUT_MONO);
    profile.channelMasks[1] = toString(xsd::AudioChannelMask::AUDIO_CHANNEL_OUT_STEREO);
    transports[0].audioCapability.profile(profile);
    hidl_vec<uint8_t> shortAudioDescriptor({0x11, 0x06, 0x01});
    transports[0].encapsulationType =
            toString(xsd::AudioEncapsulationType::AUDIO_ENCAPSULATION_TYPE_NONE);
    transports[1].audioCapability.edid(std::move(shortAudioDescriptor));
    transports[1].encapsulationType =
            toString(xsd::AudioEncapsulationType::AUDIO_ENCAPSULATION_TYPE_IEC61937);
    struct audio_port_v7 halPort;
    EXPECT_EQ(NO_ERROR, HidlUtils::audioTransportsToHal(transports, &halPort));
    hidl_vec<AudioTransport> transportsBack;
    EXPECT_EQ(NO_ERROR,
              HidlUtils::audioTransportsFromHal(halPort, false /*isInput*/, &transportsBack));
    EXPECT_EQ(transports, transportsBack);
}

TEST(HidlUtils, ConvertInvalidAudioPort) {
    AudioPort invalid;
    struct audio_port_v7 halInvalid = {};
    halInvalid.type = AUDIO_PORT_TYPE_MIX;
    halInvalid.role = AUDIO_PORT_ROLE_NONE;  // note: this is valid.
    halInvalid.num_audio_profiles = 1;
    halInvalid.audio_profiles[0].format = kInvalidHalFormat;
    EXPECT_EQ(BAD_VALUE, HidlUtils::audioPortFromHal(halInvalid, &invalid));
    invalid.transports.resize(1);
    AudioProfile invalidProfile;
    invalidProfile.format = "random string";
    invalid.transports[0].audioCapability.profile(invalidProfile);
    EXPECT_EQ(BAD_VALUE, HidlUtils::audioPortToHal(invalid, &halInvalid));
}

TEST(HidlUtils, ConvertAudioPort) {
    AudioPort port = {};
    port.id = 42;
    port.name = "test";
    port.transports.resize(2);
    AudioProfile profile;
    profile.format = toString(xsd::AudioFormat::AUDIO_FORMAT_PCM_16_BIT);
    profile.sampleRates.resize(2);
    profile.sampleRates[0] = 44100;
    profile.sampleRates[1] = 48000;
    profile.channelMasks.resize(2);
    profile.channelMasks[0] = toString(xsd::AudioChannelMask::AUDIO_CHANNEL_OUT_MONO);
    profile.channelMasks[1] = toString(xsd::AudioChannelMask::AUDIO_CHANNEL_OUT_STEREO);
    port.transports[0].audioCapability.profile(profile);
    port.transports[0].encapsulationType =
            toString(xsd::AudioEncapsulationType::AUDIO_ENCAPSULATION_TYPE_NONE);
    hidl_vec<uint8_t> shortAudioDescriptor({0x11, 0x06, 0x01});
    port.transports[1].audioCapability.edid(std::move(shortAudioDescriptor));
    port.transports[1].encapsulationType =
            toString(xsd::AudioEncapsulationType::AUDIO_ENCAPSULATION_TYPE_IEC61937);
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

TEST(HidlUtils, ConvertInvalidAudioTags) {
    char halTag[AUDIO_ATTRIBUTES_TAGS_MAX_SIZE] = {};

    hidl_vec<AudioTag> emptyTag = {{""}};
    EXPECT_EQ(BAD_VALUE, HidlUtils::audioTagsToHal(emptyTag, halTag));

    hidl_vec<AudioTag> longTag = {{std::string(AUDIO_ATTRIBUTES_TAGS_MAX_SIZE + 1, 'A')}};
    EXPECT_EQ(BAD_VALUE, HidlUtils::audioTagsToHal(longTag, halTag));

    hidl_vec<AudioTag> tagSeparator = {
            {std::string(AUDIO_ATTRIBUTES_TAGS_MAX_SIZE - 1, HidlUtils::sAudioTagSeparator)}};
    EXPECT_EQ(BAD_VALUE, HidlUtils::audioTagsToHal(tagSeparator, halTag));

    hidl_vec<AudioTag> notExtensions = {{"", "random string", "VX_", "VX_GOOGLE_$$"}};
    EXPECT_EQ(BAD_VALUE, HidlUtils::audioTagsToHal(notExtensions, halTag));
}

TEST(HidlUtils, ConvertAudioTags) {
    hidl_vec<AudioTag> emptyTags;
    char halEmptyTags[AUDIO_ATTRIBUTES_TAGS_MAX_SIZE] = {};
    EXPECT_EQ(NO_ERROR, HidlUtils::audioTagsToHal(emptyTags, halEmptyTags));
    hidl_vec<AudioTag> emptyTagsBack;
    EXPECT_EQ(NO_ERROR,
              HidlUtils::audioTagsFromHal(HidlUtils::splitAudioTags(halEmptyTags), &emptyTagsBack));
    EXPECT_EQ(emptyTags, emptyTagsBack);

    hidl_vec<AudioTag> oneTag = {{"VX_GOOGLE_VR"}};
    char halOneTag[AUDIO_ATTRIBUTES_TAGS_MAX_SIZE] = {};
    EXPECT_EQ(NO_ERROR, HidlUtils::audioTagsToHal(oneTag, halOneTag));
    hidl_vec<AudioTag> oneTagBack;
    EXPECT_EQ(NO_ERROR,
              HidlUtils::audioTagsFromHal(HidlUtils::splitAudioTags(halOneTag), &oneTagBack));
    EXPECT_EQ(oneTag, oneTagBack);

    hidl_vec<AudioTag> twoTags = {{"VX_GOOGLE_VR_42", "VX_GOOGLE_1E100"}};
    char halTwoTags[AUDIO_ATTRIBUTES_TAGS_MAX_SIZE] = {};
    EXPECT_EQ(NO_ERROR, HidlUtils::audioTagsToHal(twoTags, halTwoTags));
    hidl_vec<AudioTag> twoTagsBack;
    EXPECT_EQ(NO_ERROR,
              HidlUtils::audioTagsFromHal(HidlUtils::splitAudioTags(halTwoTags), &twoTagsBack));
    EXPECT_EQ(twoTags, twoTagsBack);
}

template <typename T>
class FilterTest : public ::testing::Test {};
using FilterTestTypeParams = ::testing::Types<hidl_vec<AudioTag>, std::vector<std::string>>;
TYPED_TEST_SUITE(FilterTest, FilterTestTypeParams);

TYPED_TEST(FilterTest, FilterOutNonVendorTags) {
    TypeParam emptyTags;
    EXPECT_EQ(emptyTags, HidlUtils::filterOutNonVendorTags(emptyTags));

    TypeParam allVendorTags = {{"VX_GOOGLE_VR_42", "VX_GOOGLE_1E100"}};
    EXPECT_EQ(allVendorTags, HidlUtils::filterOutNonVendorTags(allVendorTags));

    TypeParam oneVendorTag = {{"", "VX_GOOGLE_VR", "random_string"}};
    TypeParam oneVendorTagOnly = HidlUtils::filterOutNonVendorTags(oneVendorTag);
    EXPECT_EQ(1, oneVendorTagOnly.size());
    EXPECT_EQ(oneVendorTag[1], oneVendorTagOnly[0]);

    // The vendor extension isn't valid, however it must not be filtered out
    // so the converter can detect the issue.
    TypeParam oneMaybeVendorTag = {{"", "random string", "VX_GOOGLE_$$"}};
    TypeParam oneMaybeVendorTagOnly = HidlUtils::filterOutNonVendorTags(oneMaybeVendorTag);
    EXPECT_EQ(1, oneMaybeVendorTagOnly.size());
    EXPECT_EQ(oneMaybeVendorTag[2], oneMaybeVendorTagOnly[0]);

    TypeParam noVendorTags = {{"", "random string", "V_"}};
    EXPECT_EQ(emptyTags, HidlUtils::filterOutNonVendorTags(noVendorTags));
}
