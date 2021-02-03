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

#include <string>
#include <vector>

#include <gtest/gtest.h>

#define LOG_TAG "CoreUtils_Test"
#include <log/log.h>

#include <android_audio_policy_configuration_V7_0-enums.h>
#include <system/audio.h>
#include <util/CoreUtils.h>
#include <xsdc/XsdcSupport.h>

using namespace android;
using namespace ::android::hardware::audio::common::CPP_VERSION;
using namespace ::android::hardware::audio::CPP_VERSION;
using ::android::hardware::hidl_vec;
using ::android::hardware::audio::CPP_VERSION::implementation::CoreUtils;
namespace xsd {
using namespace ::android::audio::policy::configuration::V7_0;
}

static constexpr audio_channel_mask_t kInvalidHalChannelMask = AUDIO_CHANNEL_INVALID;
static constexpr audio_content_type_t kInvalidHalContentType =
        static_cast<audio_content_type_t>(0xFFFFFFFFU);
static constexpr audio_devices_t kInvalidHalDevice = static_cast<audio_devices_t>(0xFFFFFFFFU);
static constexpr audio_input_flags_t kInvalidInputFlags =
        static_cast<audio_input_flags_t>(0xFFFFFFFFU);
static constexpr audio_output_flags_t kInvalidOutputFlags =
        static_cast<audio_output_flags_t>(0xFFFFFFFFU);
// AUDIO_SOURCE_INVALID is framework-only.
static constexpr audio_source_t kInvalidHalSource = static_cast<audio_source_t>(-1);
static constexpr audio_usage_t kInvalidHalUsage = static_cast<audio_usage_t>(0xFFFFFFFFU);

static bool isInputFlag(xsd::AudioInOutFlag flag) {
    return toString(flag).find("_INPUT_FLAG_") != std::string::npos;
}

static bool isOutputFlag(xsd::AudioInOutFlag flag) {
    return toString(flag).find("_OUTPUT_FLAG_") != std::string::npos;
}

TEST(CoreUtils, ConvertInvalidInputFlagMask) {
    CoreUtils::AudioInputFlags invalid;
    EXPECT_EQ(BAD_VALUE, CoreUtils::audioInputFlagsFromHal(kInvalidInputFlags, &invalid));
    audio_input_flags_t halInvalid;
    invalid.resize(1);
    invalid[0] = "random string";
    EXPECT_EQ(BAD_VALUE, CoreUtils::audioInputFlagsToHal(invalid, &halInvalid));
}

TEST(CoreUtils, ConvertInputFlagMask) {
    CoreUtils::AudioInputFlags emptyInputFlags;
    audio_input_flags_t halEmptyInputFlags;
    EXPECT_EQ(NO_ERROR, CoreUtils::audioInputFlagsToHal(emptyInputFlags, &halEmptyInputFlags));
    EXPECT_EQ(AUDIO_INPUT_FLAG_NONE, halEmptyInputFlags);
    CoreUtils::AudioInputFlags emptyInputFlagsBack;
    EXPECT_EQ(NO_ERROR,
              CoreUtils::audioInputFlagsFromHal(halEmptyInputFlags, &emptyInputFlagsBack));
    EXPECT_EQ(emptyInputFlags, emptyInputFlagsBack);
    CoreUtils::AudioInputFlags emptyInputFlagsFromNone;
    EXPECT_EQ(NO_ERROR,
              CoreUtils::audioInputFlagsFromHal(AUDIO_INPUT_FLAG_NONE, &emptyInputFlagsFromNone));
    EXPECT_EQ(emptyInputFlags, emptyInputFlagsFromNone);

    std::vector<std::string> allEnumValues;
    for (const auto enumVal : xsdc_enum_range<xsd::AudioInOutFlag>{}) {
        if (isInputFlag(enumVal)) {
            allEnumValues.push_back(toString(enumVal));
        }
    }
    CoreUtils::AudioInputFlags allInputFlags;
    allInputFlags.resize(allEnumValues.size());
    for (size_t i = 0; i < allEnumValues.size(); ++i) {
        allInputFlags[i] = allEnumValues[i];
    }
    audio_input_flags_t halAllInputFlags;
    EXPECT_EQ(NO_ERROR, CoreUtils::audioInputFlagsToHal(allInputFlags, &halAllInputFlags));
    CoreUtils::AudioInputFlags allInputFlagsBack;
    EXPECT_EQ(NO_ERROR, CoreUtils::audioInputFlagsFromHal(halAllInputFlags, &allInputFlagsBack));
    EXPECT_EQ(allInputFlags, allInputFlagsBack);
}

TEST(CoreUtils, ConvertInvalidOutputFlagMask) {
    CoreUtils::AudioOutputFlags invalid;
    EXPECT_EQ(BAD_VALUE, CoreUtils::audioOutputFlagsFromHal(kInvalidOutputFlags, &invalid));
    audio_output_flags_t halInvalid;
    invalid.resize(1);
    invalid[0] = "random string";
    EXPECT_EQ(BAD_VALUE, CoreUtils::audioOutputFlagsToHal(invalid, &halInvalid));
}

TEST(CoreUtils, ConvertOutputFlagMask) {
    CoreUtils::AudioOutputFlags emptyOutputFlags;
    audio_output_flags_t halEmptyOutputFlags;
    EXPECT_EQ(NO_ERROR, CoreUtils::audioOutputFlagsToHal(emptyOutputFlags, &halEmptyOutputFlags));
    EXPECT_EQ(AUDIO_OUTPUT_FLAG_NONE, halEmptyOutputFlags);
    CoreUtils::AudioOutputFlags emptyOutputFlagsBack;
    EXPECT_EQ(NO_ERROR,
              CoreUtils::audioOutputFlagsFromHal(halEmptyOutputFlags, &emptyOutputFlagsBack));
    EXPECT_EQ(emptyOutputFlags, emptyOutputFlagsBack);
    CoreUtils::AudioOutputFlags emptyOutputFlagsFromNone;
    EXPECT_EQ(NO_ERROR, CoreUtils::audioOutputFlagsFromHal(AUDIO_OUTPUT_FLAG_NONE,
                                                           &emptyOutputFlagsFromNone));
    EXPECT_EQ(emptyOutputFlags, emptyOutputFlagsFromNone);

    std::vector<std::string> allEnumValues;
    for (const auto enumVal : xsdc_enum_range<xsd::AudioInOutFlag>{}) {
        if (isOutputFlag(enumVal)) {
            allEnumValues.push_back(toString(enumVal));
        }
    }
    CoreUtils::AudioOutputFlags allOutputFlags;
    allOutputFlags.resize(allEnumValues.size());
    for (size_t i = 0; i < allEnumValues.size(); ++i) {
        allOutputFlags[i] = allEnumValues[i];
    }
    audio_output_flags_t halAllOutputFlags;
    EXPECT_EQ(NO_ERROR, CoreUtils::audioOutputFlagsToHal(allOutputFlags, &halAllOutputFlags));
    CoreUtils::AudioOutputFlags allOutputFlagsBack;
    EXPECT_EQ(NO_ERROR, CoreUtils::audioOutputFlagsFromHal(halAllOutputFlags, &allOutputFlagsBack));
    EXPECT_EQ(allOutputFlags, allOutputFlagsBack);
}

static MicrophoneInfo generateValidMicrophoneInfo() {
    MicrophoneInfo micInfo{};
    micInfo.deviceAddress.deviceType = toString(xsd::AudioDevice::AUDIO_DEVICE_IN_BUILTIN_MIC);
    micInfo.channelMapping.resize(1);
    micInfo.channelMapping[0] = AudioMicrophoneChannelMapping::DIRECT;
    micInfo.location = AudioMicrophoneLocation::MAINBODY_MOVABLE;
    micInfo.group = 42;
    micInfo.indexInTheGroup = 13;
    micInfo.sensitivity = 65.5;
    micInfo.maxSpl = 100.5;
    micInfo.minSpl = 36.6;
    micInfo.directionality = AudioMicrophoneDirectionality::HYPER_CARDIOID;
    micInfo.frequencyResponse.resize(1);
    micInfo.frequencyResponse[0].frequency = 1000;
    micInfo.frequencyResponse[0].level = 85;
    micInfo.position.x = 0;
    micInfo.position.y = 1;
    micInfo.position.z = 0;
    micInfo.orientation.x = 0;
    micInfo.orientation.y = 0;
    micInfo.orientation.z = 1;
    return micInfo;
}

TEST(CoreUtils, ConvertInvalidMicrophoneInfo) {
    MicrophoneInfo invalid;
    audio_microphone_characteristic_t halInvalid{};
    halInvalid.device = kInvalidHalDevice;
    EXPECT_EQ(BAD_VALUE, CoreUtils::microphoneInfoFromHal(halInvalid, &invalid));

    MicrophoneInfo oversizeDeviceId = generateValidMicrophoneInfo();
    oversizeDeviceId.deviceId = std::string(AUDIO_MICROPHONE_ID_MAX_LEN + 1, 'A');
    EXPECT_EQ(BAD_VALUE, CoreUtils::microphoneInfoToHal(oversizeDeviceId, &halInvalid));
    MicrophoneInfo invalidDeviceType = generateValidMicrophoneInfo();
    invalidDeviceType.deviceAddress.deviceType = "random string";
    EXPECT_EQ(BAD_VALUE, CoreUtils::microphoneInfoToHal(invalidDeviceType, &halInvalid));
    MicrophoneInfo oversizeChannelMapping = generateValidMicrophoneInfo();
    oversizeChannelMapping.channelMapping.resize(AUDIO_CHANNEL_COUNT_MAX + 1);
    EXPECT_EQ(BAD_VALUE, CoreUtils::microphoneInfoToHal(oversizeChannelMapping, &halInvalid));
    MicrophoneInfo oversizeFrequencyResponses = generateValidMicrophoneInfo();
    oversizeFrequencyResponses.frequencyResponse.resize(AUDIO_MICROPHONE_MAX_FREQUENCY_RESPONSES +
                                                        1);
    EXPECT_EQ(BAD_VALUE, CoreUtils::microphoneInfoToHal(oversizeFrequencyResponses, &halInvalid));
}

TEST(CoreUtils, ConvertMicrophoneInfo) {
    MicrophoneInfo micInfo = generateValidMicrophoneInfo();
    audio_microphone_characteristic_t halMicInfo;
    EXPECT_EQ(NO_ERROR, CoreUtils::microphoneInfoToHal(micInfo, &halMicInfo));
    MicrophoneInfo micInfoBack;
    EXPECT_EQ(NO_ERROR, CoreUtils::microphoneInfoFromHal(halMicInfo, &micInfoBack));
    EXPECT_EQ(micInfo, micInfoBack);
}

static RecordTrackMetadata generateMinimalRecordTrackMetadata() {
    RecordTrackMetadata metadata{};
    metadata.source = toString(xsd::AudioSource::AUDIO_SOURCE_DEFAULT);
    metadata.channelMask = toString(xsd::AudioChannelMask::AUDIO_CHANNEL_NONE);
    return metadata;
}

static RecordTrackMetadata generateValidRecordTrackMetadata() {
    RecordTrackMetadata metadata = generateMinimalRecordTrackMetadata();
    metadata.tags.resize(1);
    metadata.tags[0] = "VX_GOOGLE_42";
    metadata.channelMask = toString(xsd::AudioChannelMask::AUDIO_CHANNEL_IN_MONO);
    metadata.gain = 1.0;
    return metadata;
}

static RecordTrackMetadata generateValidRecordTrackMetadataWithDevice() {
    RecordTrackMetadata metadata = generateValidRecordTrackMetadata();
    metadata.destination.device({});
    metadata.destination.device().deviceType = toString(xsd::AudioDevice::AUDIO_DEVICE_OUT_SPEAKER);
    return metadata;
}

using SinkTracks = hidl_vec<RecordTrackMetadata>;

TEST(CoreUtils, ConvertInvalidSinkMetadata) {
    SinkMetadata invalidSource;
    invalidSource.tracks = SinkTracks{generateMinimalRecordTrackMetadata()};
    invalidSource.tracks[0].source = "random string";
    EXPECT_EQ(BAD_VALUE, CoreUtils::sinkMetadataToHal(invalidSource, nullptr));
    EXPECT_EQ(BAD_VALUE, CoreUtils::sinkMetadataToHalV7(invalidSource,
                                                        false /*ignoreNonVendorTags*/, nullptr));
    EXPECT_EQ(BAD_VALUE,
              CoreUtils::sinkMetadataToHalV7(invalidSource, true /*ignoreNonVendorTags*/, nullptr));
    SinkMetadata invalidDeviceType;
    invalidDeviceType.tracks = SinkTracks{generateValidRecordTrackMetadataWithDevice()};
    invalidDeviceType.tracks[0].destination.device().deviceType = "random string";
    EXPECT_EQ(BAD_VALUE, CoreUtils::sinkMetadataToHal(invalidDeviceType, nullptr));
    EXPECT_EQ(BAD_VALUE, CoreUtils::sinkMetadataToHalV7(invalidDeviceType,
                                                        false /*ignoreNonVendorTags*/, nullptr));
    EXPECT_EQ(BAD_VALUE, CoreUtils::sinkMetadataToHalV7(invalidDeviceType,
                                                        true /*ignoreNonVendorTags*/, nullptr));
    SinkMetadata invalidChannelMask;
    invalidChannelMask.tracks = SinkTracks{generateValidRecordTrackMetadata()};
    invalidChannelMask.tracks[0].channelMask = "random string";
    // Channel mask is sliced away by 'sinkMetadataToHal'
    EXPECT_EQ(NO_ERROR, CoreUtils::sinkMetadataToHal(invalidChannelMask, nullptr));
    EXPECT_EQ(BAD_VALUE, CoreUtils::sinkMetadataToHalV7(invalidChannelMask,
                                                        false /*ignoreNonVendorTags*/, nullptr));
    EXPECT_EQ(BAD_VALUE, CoreUtils::sinkMetadataToHalV7(invalidChannelMask,
                                                        true /*ignoreNonVendorTags*/, nullptr));
    SinkMetadata invalidTags;
    invalidTags.tracks = SinkTracks{generateValidRecordTrackMetadata()};
    invalidTags.tracks[0].tags[0] = "random string";
    // Tags are sliced away by 'sinkMetadataToHal'
    EXPECT_EQ(NO_ERROR, CoreUtils::sinkMetadataToHal(invalidTags, nullptr));
    EXPECT_EQ(BAD_VALUE,
              CoreUtils::sinkMetadataToHalV7(invalidTags, false /*ignoreNonVendorTags*/, nullptr));
    // Non-vendor tags should be filtered out.
    EXPECT_EQ(NO_ERROR,
              CoreUtils::sinkMetadataToHalV7(invalidTags, true /*ignoreNonVendorTags*/, nullptr));

    // Verify that a default-initialized metadata is valid.
    std::vector<record_track_metadata_t> halValid(1, record_track_metadata_t{});
    std::vector<record_track_metadata_v7_t> halValidV7(1, record_track_metadata_v7_t{});
    SinkMetadata valid;
    EXPECT_EQ(NO_ERROR, CoreUtils::sinkMetadataFromHal(halValid, &valid));
    EXPECT_EQ(NO_ERROR,
              CoreUtils::sinkMetadataFromHalV7(halValidV7, false /*ignoreNonVendorTags*/, &valid));
    EXPECT_EQ(NO_ERROR,
              CoreUtils::sinkMetadataFromHalV7(halValidV7, true /*ignoreNonVendorTags*/, &valid));

    std::vector<record_track_metadata_t> halInvalidSource = {{.source = kInvalidHalSource}};
    std::vector<record_track_metadata_v7_t> halInvalidSourceV7 = {
            {.base = {.source = kInvalidHalSource}}};
    EXPECT_EQ(BAD_VALUE, CoreUtils::sinkMetadataFromHal(halInvalidSource, &invalidSource));
    EXPECT_EQ(BAD_VALUE,
              CoreUtils::sinkMetadataFromHalV7(halInvalidSourceV7, false /*ignoreNonVendorTags*/,
                                               &invalidSource));
    EXPECT_EQ(BAD_VALUE, CoreUtils::sinkMetadataFromHalV7(
                                 halInvalidSourceV7, true /*ignoreNonVendorTags*/, &invalidSource));
    std::vector<record_track_metadata_t> halInvalidDeviceType = {
            {.dest_device = kInvalidHalDevice}};
    std::vector<record_track_metadata_v7_t> halInvalidDeviceTypeV7 = {
            {.base = {.dest_device = kInvalidHalDevice}}};
    EXPECT_EQ(BAD_VALUE, CoreUtils::sinkMetadataFromHal(halInvalidDeviceType, &invalidDeviceType));
    EXPECT_EQ(BAD_VALUE,
              CoreUtils::sinkMetadataFromHalV7(halInvalidDeviceTypeV7,
                                               false /*ignoreNonVendorTags*/, &invalidDeviceType));
    EXPECT_EQ(BAD_VALUE,
              CoreUtils::sinkMetadataFromHalV7(halInvalidDeviceTypeV7, true /*ignoreNonVendorTags*/,
                                               &invalidDeviceType));
    std::vector<record_track_metadata_v7_t> halInvalidChannelMaskV7 = {
            {.channel_mask = kInvalidHalChannelMask}};
    EXPECT_EQ(BAD_VALUE,
              CoreUtils::sinkMetadataFromHalV7(halInvalidChannelMaskV7,
                                               false /*ignoreNonVendorTags*/, &invalidChannelMask));
    EXPECT_EQ(BAD_VALUE,
              CoreUtils::sinkMetadataFromHalV7(halInvalidChannelMaskV7,
                                               true /*ignoreNonVendorTags*/, &invalidChannelMask));
    std::vector<record_track_metadata_v7_t> halInvalidTagsV7 = {{.tags = "random string"}};
    EXPECT_EQ(BAD_VALUE, CoreUtils::sinkMetadataFromHalV7(
                                 halInvalidTagsV7, false /*ignoreNonVendorTags*/, &invalidTags));
    // Non-vendor tags should be filtered out.
    EXPECT_EQ(NO_ERROR, CoreUtils::sinkMetadataFromHalV7(
                                halInvalidTagsV7, true /*ignoreNonVendorTags*/, &invalidTags));
}

TEST(CoreUtils, ConvertEmptySinkMetadata) {
    SinkMetadata emptySinkMetadata;
    std::vector<record_track_metadata_t> halEmptySinkMetadata;
    EXPECT_EQ(NO_ERROR, CoreUtils::sinkMetadataToHal(emptySinkMetadata, &halEmptySinkMetadata));
    EXPECT_TRUE(halEmptySinkMetadata.empty());
    SinkMetadata emptySinkMetadataBack;
    EXPECT_EQ(NO_ERROR,
              CoreUtils::sinkMetadataFromHal(halEmptySinkMetadata, &emptySinkMetadataBack));
    EXPECT_EQ(emptySinkMetadata, emptySinkMetadataBack);
    std::vector<record_track_metadata_v7_t> halEmptySinkMetadataV7;
    EXPECT_EQ(NO_ERROR,
              CoreUtils::sinkMetadataToHalV7(emptySinkMetadata, false /*ignoreNonVendorTags*/,
                                             &halEmptySinkMetadataV7));
    EXPECT_TRUE(halEmptySinkMetadataV7.empty());
    SinkMetadata emptySinkMetadataBackFromV7;
    EXPECT_EQ(NO_ERROR, CoreUtils::sinkMetadataFromHalV7(halEmptySinkMetadataV7,
                                                         false /*ignoreNonVendorTags*/,
                                                         &emptySinkMetadataBackFromV7));
    EXPECT_EQ(emptySinkMetadata, emptySinkMetadataBackFromV7);
}

class SinkMetadataConvertTest : public ::testing::TestWithParam<SinkTracks> {};

TEST_P(SinkMetadataConvertTest, ToFromHal) {
    SinkMetadata sinkMetadata;
    sinkMetadata.tracks = GetParam();
    std::vector<record_track_metadata_t> halSinkMetadata;
    EXPECT_EQ(NO_ERROR, CoreUtils::sinkMetadataToHal(sinkMetadata, &halSinkMetadata));
    EXPECT_EQ(sinkMetadata.tracks.size(), halSinkMetadata.size());
    SinkMetadata sinkMetadataBackTrimmed;
    EXPECT_EQ(NO_ERROR, CoreUtils::sinkMetadataFromHal(halSinkMetadata, &sinkMetadataBackTrimmed));
    // Can't compare 'sinkMetadata' to 'sinkMetadataBackTrimmed'
    std::vector<record_track_metadata_v7_t> halSinkMetadataV7;
    EXPECT_EQ(NO_ERROR, CoreUtils::sinkMetadataToHalV7(sinkMetadata, false /*ignoreNonVendorTags*/,
                                                       &halSinkMetadataV7));
    EXPECT_EQ(sinkMetadata.tracks.size(), halSinkMetadataV7.size());
    SinkMetadata sinkMetadataBackFromV7;
    EXPECT_EQ(NO_ERROR,
              CoreUtils::sinkMetadataFromHalV7(halSinkMetadataV7, false /*ignoreNonVendorTags*/,
                                               &sinkMetadataBackFromV7));
    EXPECT_EQ(sinkMetadata, sinkMetadataBackFromV7);
    std::vector<record_track_metadata_v7_t> halSinkMetadataV7FromTrimmed;
    EXPECT_EQ(NO_ERROR,
              CoreUtils::sinkMetadataToHalV7(sinkMetadataBackTrimmed, false /*ignoreNonVendorTags*/,
                                             &halSinkMetadataV7FromTrimmed));
    EXPECT_EQ(sinkMetadata.tracks.size(), halSinkMetadataV7FromTrimmed.size());
    SinkMetadata sinkMetadataBackFromV7Trimmed;
    EXPECT_EQ(NO_ERROR, CoreUtils::sinkMetadataFromHalV7(halSinkMetadataV7FromTrimmed,
                                                         false /*ignoreNonVendorTags*/,
                                                         &sinkMetadataBackFromV7Trimmed));
    EXPECT_EQ(sinkMetadataBackTrimmed, sinkMetadataBackFromV7Trimmed);
}

INSTANTIATE_TEST_SUITE_P(ValidRecordTrackMetadatas, SinkMetadataConvertTest,
                         ::testing::Values(SinkTracks{generateMinimalRecordTrackMetadata()},
                                           SinkTracks{generateValidRecordTrackMetadata()},
                                           SinkTracks{generateValidRecordTrackMetadataWithDevice()},
                                           SinkTracks{
                                                   generateMinimalRecordTrackMetadata(),
                                                   generateValidRecordTrackMetadata(),
                                                   generateValidRecordTrackMetadataWithDevice()}));

static PlaybackTrackMetadata generateMinimalPlaybackTrackMetadata() {
    PlaybackTrackMetadata metadata{};
    metadata.usage = toString(xsd::AudioUsage::AUDIO_USAGE_UNKNOWN);
    metadata.contentType = toString(xsd::AudioContentType::AUDIO_CONTENT_TYPE_UNKNOWN);
    metadata.channelMask = toString(xsd::AudioChannelMask::AUDIO_CHANNEL_NONE);
    return metadata;
}

static PlaybackTrackMetadata generateValidPlaybackTrackMetadata() {
    PlaybackTrackMetadata metadata = generateMinimalPlaybackTrackMetadata();
    metadata.tags.resize(1);
    metadata.tags[0] = "VX_GOOGLE_42";
    metadata.channelMask = toString(xsd::AudioChannelMask::AUDIO_CHANNEL_OUT_MONO);
    metadata.gain = 1.0;
    return metadata;
}

using SourceTracks = hidl_vec<PlaybackTrackMetadata>;

TEST(CoreUtils, ConvertInvalidSourceMetadata) {
    SourceMetadata invalidUsage;
    invalidUsage.tracks = SourceTracks{generateMinimalPlaybackTrackMetadata()};
    invalidUsage.tracks[0].usage = "random string";
    EXPECT_EQ(BAD_VALUE, CoreUtils::sourceMetadataToHal(invalidUsage, nullptr));
    EXPECT_EQ(BAD_VALUE, CoreUtils::sourceMetadataToHalV7(invalidUsage,
                                                          false /*ignoreNonVendorTags*/, nullptr));
    SourceMetadata invalidContentType;
    invalidContentType.tracks = SourceTracks{generateMinimalPlaybackTrackMetadata()};
    invalidContentType.tracks[0].contentType = "random string";
    EXPECT_EQ(BAD_VALUE, CoreUtils::sourceMetadataToHal(invalidContentType, nullptr));
    EXPECT_EQ(BAD_VALUE, CoreUtils::sourceMetadataToHalV7(invalidContentType,
                                                          false /*ignoreNonVendorTags*/, nullptr));
    EXPECT_EQ(BAD_VALUE, CoreUtils::sourceMetadataToHalV7(invalidContentType,
                                                          true /*ignoreNonVendorTags*/, nullptr));
    SourceMetadata invalidChannelMask;
    invalidChannelMask.tracks = SourceTracks{generateValidPlaybackTrackMetadata()};
    invalidChannelMask.tracks[0].channelMask = "random string";
    // Channel mask is sliced away by 'sourceMetadataToHal'
    EXPECT_EQ(NO_ERROR, CoreUtils::sourceMetadataToHal(invalidChannelMask, nullptr));
    EXPECT_EQ(BAD_VALUE, CoreUtils::sourceMetadataToHalV7(invalidChannelMask,
                                                          false /*ignoreNonVendorTags*/, nullptr));
    EXPECT_EQ(BAD_VALUE, CoreUtils::sourceMetadataToHalV7(invalidChannelMask,
                                                          true /*ignoreNonVendorTags*/, nullptr));
    SourceMetadata invalidTags;
    invalidTags.tracks = SourceTracks{generateValidPlaybackTrackMetadata()};
    invalidTags.tracks[0].tags[0] = "random string";
    // Tags are sliced away by 'sourceMetadataToHal'
    EXPECT_EQ(NO_ERROR, CoreUtils::sourceMetadataToHal(invalidTags, nullptr));
    EXPECT_EQ(BAD_VALUE, CoreUtils::sourceMetadataToHalV7(invalidTags,
                                                          false /*ignoreNonVendorTags*/, nullptr));
    // Non-vendor tags should be filtered out.
    EXPECT_EQ(NO_ERROR,
              CoreUtils::sourceMetadataToHalV7(invalidTags, true /*ignoreNonVendorTags*/, nullptr));

    // Verify that a default-initialized metadata is valid.
    std::vector<playback_track_metadata_t> halValid(1, playback_track_metadata_t{});
    std::vector<playback_track_metadata_v7_t> halValidV7(1, playback_track_metadata_v7_t{});
    SourceMetadata valid;
    EXPECT_EQ(NO_ERROR, CoreUtils::sourceMetadataFromHal(halValid, &valid));
    EXPECT_EQ(NO_ERROR, CoreUtils::sourceMetadataFromHalV7(halValidV7,
                                                           false /*ignoreNonVendorTags*/, &valid));
    EXPECT_EQ(NO_ERROR,
              CoreUtils::sourceMetadataFromHalV7(halValidV7, true /*ignoreNonVendorTags*/, &valid));

    std::vector<playback_track_metadata_t> halInvalidUsage = {{.usage = kInvalidHalUsage}};
    std::vector<playback_track_metadata_v7_t> halInvalidUsageV7 = {
            {.base = {.usage = kInvalidHalUsage}}};
    EXPECT_EQ(BAD_VALUE, CoreUtils::sourceMetadataFromHal(halInvalidUsage, &invalidUsage));
    EXPECT_EQ(BAD_VALUE, CoreUtils::sourceMetadataFromHalV7(
                                 halInvalidUsageV7, false /*ignoreNonVendorTags*/, &invalidUsage));
    EXPECT_EQ(BAD_VALUE, CoreUtils::sourceMetadataFromHalV7(
                                 halInvalidUsageV7, true /*ignoreNonVendorTags*/, &invalidUsage));
    std::vector<playback_track_metadata_t> halInvalidContentType = {
            {.content_type = kInvalidHalContentType}};
    std::vector<playback_track_metadata_v7_t> halInvalidContentTypeV7 = {
            {.base = {.content_type = kInvalidHalContentType}}};
    EXPECT_EQ(BAD_VALUE,
              CoreUtils::sourceMetadataFromHal(halInvalidContentType, &invalidContentType));
    EXPECT_EQ(BAD_VALUE,
              CoreUtils::sourceMetadataFromHalV7(
                      halInvalidContentTypeV7, false /*ignoreNonVendorTags*/, &invalidContentType));
    EXPECT_EQ(BAD_VALUE,
              CoreUtils::sourceMetadataFromHalV7(
                      halInvalidContentTypeV7, true /*ignoreNonVendorTags*/, &invalidContentType));
    std::vector<playback_track_metadata_v7_t> halInvalidChannelMaskV7 = {
            {.channel_mask = kInvalidHalChannelMask}};
    EXPECT_EQ(BAD_VALUE,
              CoreUtils::sourceMetadataFromHalV7(
                      halInvalidChannelMaskV7, false /*ignoreNonVendorTags*/, &invalidChannelMask));
    EXPECT_EQ(BAD_VALUE,
              CoreUtils::sourceMetadataFromHalV7(
                      halInvalidChannelMaskV7, true /*ignoreNonVendorTags*/, &invalidChannelMask));
    std::vector<playback_track_metadata_v7_t> halInvalidTagsV7 = {{.tags = "random string"}};
    EXPECT_EQ(BAD_VALUE, CoreUtils::sourceMetadataFromHalV7(
                                 halInvalidTagsV7, false /*ignoreNonVendorTags*/, &invalidTags));
    // Non-vendor tags should be filtered out.
    EXPECT_EQ(NO_ERROR, CoreUtils::sourceMetadataFromHalV7(
                                halInvalidTagsV7, true /*ignoreNonVendorTags*/, &invalidTags));
}

TEST(CoreUtils, ConvertEmptySourceMetadata) {
    SourceMetadata emptySourceMetadata;
    std::vector<playback_track_metadata_t> halEmptySourceMetadata;
    EXPECT_EQ(NO_ERROR,
              CoreUtils::sourceMetadataToHal(emptySourceMetadata, &halEmptySourceMetadata));
    EXPECT_TRUE(halEmptySourceMetadata.empty());
    SourceMetadata emptySourceMetadataBack;
    EXPECT_EQ(NO_ERROR,
              CoreUtils::sourceMetadataFromHal(halEmptySourceMetadata, &emptySourceMetadataBack));
    EXPECT_EQ(emptySourceMetadata, emptySourceMetadataBack);
    std::vector<playback_track_metadata_v7_t> halEmptySourceMetadataV7;
    EXPECT_EQ(NO_ERROR,
              CoreUtils::sourceMetadataToHalV7(emptySourceMetadata, false /*ignoreNonVendorTags*/,
                                               &halEmptySourceMetadataV7));
    EXPECT_TRUE(halEmptySourceMetadataV7.empty());
    SourceMetadata emptySourceMetadataBackFromV7;
    EXPECT_EQ(NO_ERROR, CoreUtils::sourceMetadataFromHalV7(halEmptySourceMetadataV7,
                                                           false /*ignoreNonVendorTags*/,
                                                           &emptySourceMetadataBackFromV7));
    EXPECT_EQ(emptySourceMetadata, emptySourceMetadataBackFromV7);
}

class SourceMetadataConvertTest : public ::testing::TestWithParam<SourceTracks> {};

TEST_P(SourceMetadataConvertTest, ToFromHal) {
    SourceMetadata sourceMetadata;
    sourceMetadata.tracks = GetParam();
    std::vector<playback_track_metadata_t> halSourceMetadata;
    EXPECT_EQ(NO_ERROR, CoreUtils::sourceMetadataToHal(sourceMetadata, &halSourceMetadata));
    EXPECT_EQ(sourceMetadata.tracks.size(), halSourceMetadata.size());
    SourceMetadata sourceMetadataBackTrimmed;
    EXPECT_EQ(NO_ERROR,
              CoreUtils::sourceMetadataFromHal(halSourceMetadata, &sourceMetadataBackTrimmed));
    // Can't compare 'sourceMetadata' to 'sourceMetadataBackTrimmed'
    std::vector<playback_track_metadata_v7_t> halSourceMetadataV7;
    EXPECT_EQ(NO_ERROR,
              CoreUtils::sourceMetadataToHalV7(sourceMetadata, false /*ignoreNonVendorTags*/,
                                               &halSourceMetadataV7));
    EXPECT_EQ(sourceMetadata.tracks.size(), halSourceMetadataV7.size());
    SourceMetadata sourceMetadataBackFromV7;
    EXPECT_EQ(NO_ERROR,
              CoreUtils::sourceMetadataFromHalV7(halSourceMetadataV7, false /*ignoreNonVendorTags*/,
                                                 &sourceMetadataBackFromV7));
    EXPECT_EQ(sourceMetadata, sourceMetadataBackFromV7);
    std::vector<playback_track_metadata_v7_t> halSourceMetadataV7FromTrimmed;
    EXPECT_EQ(NO_ERROR, CoreUtils::sourceMetadataToHalV7(sourceMetadataBackTrimmed,
                                                         false /*ignoreNonVendorTags*/,
                                                         &halSourceMetadataV7FromTrimmed));
    EXPECT_EQ(sourceMetadata.tracks.size(), halSourceMetadataV7FromTrimmed.size());
    SourceMetadata sourceMetadataBackFromV7Trimmed;
    EXPECT_EQ(NO_ERROR, CoreUtils::sourceMetadataFromHalV7(halSourceMetadataV7FromTrimmed,
                                                           false /*ignoreNonVendorTags*/,
                                                           &sourceMetadataBackFromV7Trimmed));
    EXPECT_EQ(sourceMetadataBackTrimmed, sourceMetadataBackFromV7Trimmed);
}

INSTANTIATE_TEST_SUITE_P(ValidPlaybackTrackMetadatas, SourceMetadataConvertTest,
                         ::testing::Values(SourceTracks{generateMinimalPlaybackTrackMetadata()},
                                           SourceTracks{generateValidPlaybackTrackMetadata()},
                                           SourceTracks{generateMinimalPlaybackTrackMetadata(),
                                                        generateValidPlaybackTrackMetadata()}));
