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

#include <android-base/chrono_utils.h>

#include "Generators.h"

// pull in all the <= 6.0 tests
#include "6.0/AudioPrimaryHidlHalTest.cpp"

class InvalidInputConfigNoFlagsTest : public AudioHidlTestWithDeviceConfigParameter {};
TEST_P(InvalidInputConfigNoFlagsTest, InputBufferSizeTest) {
    doc::test("Verify that invalid config is rejected by IDevice::getInputBufferSize method.");
    uint64_t bufferSize;
    ASSERT_OK(getDevice()->getInputBufferSize(getConfig(), returnIn(res, bufferSize)));
    EXPECT_EQ(Result::INVALID_ARGUMENTS, res);
}
INSTANTIATE_TEST_CASE_P(
        InputBufferSizeInvalidConfig, InvalidInputConfigNoFlagsTest,
        ::testing::ValuesIn(getInputDeviceInvalidConfigParameters(false /*generateInvalidFlags*/)),
        &DeviceConfigParameterToString);
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(InvalidInputConfigNoFlagsTest);

static const DeviceAddress& getValidInputDeviceAddress() {
    static const DeviceAddress valid = {
            .deviceType = toString(xsd::AudioDevice::AUDIO_DEVICE_IN_DEFAULT)};
    return valid;
}

static const DeviceAddress& getValidOutputDeviceAddress() {
    static const DeviceAddress valid = {
            .deviceType = toString(xsd::AudioDevice::AUDIO_DEVICE_OUT_DEFAULT)};
    return valid;
}

static const DeviceAddress& getInvalidDeviceAddress() {
    static const DeviceAddress valid = {.deviceType = "random_string"};
    return valid;
}

TEST_P(AudioHidlDeviceTest, SetConnectedStateInvalidDeviceAddress) {
    doc::test("Check that invalid device address is rejected by IDevice::setConnectedState");
    EXPECT_RESULT(Result::INVALID_ARGUMENTS,
                  getDevice()->setConnectedState(getInvalidDeviceAddress(), true));
    EXPECT_RESULT(Result::INVALID_ARGUMENTS,
                  getDevice()->setConnectedState(getInvalidDeviceAddress(), false));
}

static std::vector<AudioPortConfig>& generatePortConfigs(bool valid) {
    enum {  // Note: This is for convenience when deriving "invalid" configs from "valid".
        PORT_CONF_MINIMAL,
        PORT_CONF_WITH_GAIN,
        PORT_CONF_EXT_DEVICE,
        PORT_CONF_EXT_MIX_SOURCE,
        PORT_CONF_EXT_MIX_SINK,
        PORT_CONF_EXT_SESSION
    };
    static std::vector<AudioPortConfig> valids = [] {
        std::vector<AudioPortConfig> result;
        result.reserve(PORT_CONF_EXT_SESSION + 1);
        result.push_back(AudioPortConfig{});
        AudioPortConfig configWithGain{};
        configWithGain.gain.config(AudioGainConfig{
                .index = 0,
                .mode = {toString(xsd::AudioGainMode::AUDIO_GAIN_MODE_JOINT)},
                .channelMask = toString(xsd::AudioChannelMask::AUDIO_CHANNEL_OUT_MONO),
                .rampDurationMs = 1});
        configWithGain.gain.config().values.resize(1);
        configWithGain.gain.config().values[0] = 1000;
        result.push_back(std::move(configWithGain));
        AudioPortConfig configWithPortExtDevice{};
        configWithPortExtDevice.ext.device(getValidOutputDeviceAddress());
        result.push_back(std::move(configWithPortExtDevice));
        AudioPortConfig configWithPortExtMixSource{};
        configWithPortExtMixSource.ext.mix({});
        configWithPortExtMixSource.ext.mix().useCase.stream(
                toString(xsd::AudioStreamType::AUDIO_STREAM_VOICE_CALL));
        result.push_back(std::move(configWithPortExtMixSource));
        AudioPortConfig configWithPortExtMixSink{};
        configWithPortExtMixSink.ext.mix({});
        configWithPortExtMixSink.ext.mix().useCase.source(
                toString(xsd::AudioSource::AUDIO_SOURCE_DEFAULT));
        result.push_back(std::move(configWithPortExtMixSink));
        AudioPortConfig configWithPortExtSession{};
        configWithPortExtSession.ext.session(
                static_cast<AudioSession>(AudioSessionConsts::OUTPUT_MIX));
        result.push_back(std::move(configWithPortExtSession));
        return result;
    }();
    static std::vector<AudioPortConfig> invalids = [&] {
        std::vector<AudioPortConfig> result;
        AudioPortConfig invalidBaseChannelMask = valids[PORT_CONF_MINIMAL];
        invalidBaseChannelMask.base.channelMask.value("random_string");
        result.push_back(std::move(invalidBaseChannelMask));
        AudioPortConfig invalidBaseFormat = valids[PORT_CONF_MINIMAL];
        invalidBaseFormat.base.format.value("random_string");
        result.push_back(std::move(invalidBaseFormat));
        AudioPortConfig invalidGainMode = valids[PORT_CONF_WITH_GAIN];
        invalidGainMode.gain.config().mode = {{"random_string"}};
        result.push_back(std::move(invalidGainMode));
        AudioPortConfig invalidGainChannelMask = valids[PORT_CONF_WITH_GAIN];
        invalidGainChannelMask.gain.config().channelMask = "random_string";
        result.push_back(std::move(invalidGainChannelMask));
        AudioPortConfig invalidDeviceType = valids[PORT_CONF_EXT_DEVICE];
        invalidDeviceType.ext.device().deviceType = "random_string";
        result.push_back(std::move(invalidDeviceType));
        AudioPortConfig invalidStreamType = valids[PORT_CONF_EXT_MIX_SOURCE];
        invalidStreamType.ext.mix().useCase.stream() = "random_string";
        result.push_back(std::move(invalidStreamType));
        AudioPortConfig invalidSource = valids[PORT_CONF_EXT_MIX_SINK];
        invalidSource.ext.mix().useCase.source() = "random_string";
        result.push_back(std::move(invalidSource));
        return result;
    }();
    return valid ? valids : invalids;
}

TEST_P(AudioHidlDeviceTest, SetAudioPortConfigInvalidArguments) {
    doc::test("Check that invalid port configs are rejected by IDevice::setAudioPortConfig");
    for (const auto& invalidConfig : generatePortConfigs(false /*valid*/)) {
        EXPECT_RESULT(invalidArgsOrNotSupported, getDevice()->setAudioPortConfig(invalidConfig))
                << ::testing::PrintToString(invalidConfig);
    }
}

TEST_P(AudioPatchHidlTest, CreatePatchInvalidArguments) {
    doc::test("Check that invalid port configs are rejected by IDevice::createAudioPatch");
    // Note that HAL actually might reject the proposed source / sink combo
    // due to other reasons than presence of invalid enum-strings.
    // TODO: Come up with a way to guarantee validity of a source / sink combo.
    for (const auto& validSource : generatePortConfigs(true /*valid*/)) {
        for (const auto& invalidSink : generatePortConfigs(false /*valid*/)) {
            AudioPatchHandle handle;
            EXPECT_OK(getDevice()->createAudioPatch(hidl_vec<AudioPortConfig>{validSource},
                                                    hidl_vec<AudioPortConfig>{invalidSink},
                                                    returnIn(res, handle)));
            EXPECT_EQ(Result::INVALID_ARGUMENTS, res)
                    << "Source: " << ::testing::PrintToString(validSource)
                    << "; Sink: " << ::testing::PrintToString(invalidSink);
        }
    }
    for (const auto& validSink : generatePortConfigs(true /*valid*/)) {
        for (const auto& invalidSource : generatePortConfigs(false /*valid*/)) {
            AudioPatchHandle handle;
            EXPECT_OK(getDevice()->createAudioPatch(hidl_vec<AudioPortConfig>{invalidSource},
                                                    hidl_vec<AudioPortConfig>{validSink},
                                                    returnIn(res, handle)));
            EXPECT_EQ(Result::INVALID_ARGUMENTS, res)
                    << "Source: " << ::testing::PrintToString(invalidSource)
                    << "; Sink: " << ::testing::PrintToString(validSink);
        }
    }
}

TEST_P(AudioPatchHidlTest, UpdatePatchInvalidArguments) {
    doc::test("Check that invalid port configs are rejected by IDevice::updateAudioPatch");
    // Note that HAL actually might reject the proposed source / sink combo
    // due to other reasons than presence of invalid enum-strings.
    // TODO: Come up with a way to guarantee validity of a source / sink combo.
    for (const auto& validSource : generatePortConfigs(true /*valid*/)) {
        for (const auto& invalidSink : generatePortConfigs(false /*valid*/)) {
            AudioPatchHandle handle{};
            EXPECT_OK(getDevice()->updateAudioPatch(handle, hidl_vec<AudioPortConfig>{validSource},
                                                    hidl_vec<AudioPortConfig>{invalidSink},
                                                    returnIn(res, handle)));
            EXPECT_EQ(Result::INVALID_ARGUMENTS, res)
                    << "Source: " << ::testing::PrintToString(validSource)
                    << "; Sink: " << ::testing::PrintToString(invalidSink);
        }
    }
    for (const auto& validSink : generatePortConfigs(true /*valid*/)) {
        for (const auto& invalidSource : generatePortConfigs(false /*valid*/)) {
            AudioPatchHandle handle{};
            EXPECT_OK(getDevice()->updateAudioPatch(
                    handle, hidl_vec<AudioPortConfig>{invalidSource},
                    hidl_vec<AudioPortConfig>{validSink}, returnIn(res, handle)));
            EXPECT_EQ(Result::INVALID_ARGUMENTS, res)
                    << "Source: " << ::testing::PrintToString(invalidSource)
                    << "; Sink: " << ::testing::PrintToString(validSink);
        }
    }
}

enum { PARAM_DEVICE_CONFIG, PARAM_ADDRESS, PARAM_METADATA };
enum { INDEX_SINK, INDEX_SOURCE };
using SinkOrSourceMetadata = std::variant<SinkMetadata, SourceMetadata>;
using StreamOpenParameter = std::tuple<DeviceConfigParameter, DeviceAddress, SinkOrSourceMetadata>;

static std::string StreamOpenParameterToString(
        const ::testing::TestParamInfo<StreamOpenParameter>& info) {
    return DeviceConfigParameterToString(::testing::TestParamInfo<DeviceConfigParameter>{
                   std::get<PARAM_DEVICE_CONFIG>(info.param), info.index}) +
           "__" +
           SanitizeStringForGTestName(
                   ::testing::PrintToString(std::get<PARAM_ADDRESS>(info.param))) +
           "__" +
           SanitizeStringForGTestName(std::visit(
                   [](auto&& arg) -> std::string { return ::testing::PrintToString(arg); },
                   std::get<PARAM_METADATA>(info.param)));
}

class StreamOpenTest : public HidlTest, public ::testing::WithParamInterface<StreamOpenParameter> {
  protected:
    void SetUp() override {
        ASSERT_NO_FATAL_FAILURE(HidlTest::SetUp());  // setup base
        ASSERT_TRUE(getDevicesFactory() != nullptr);
        ASSERT_TRUE(getDevice() != nullptr);
    }
    const std::string& getFactoryName() const override {
        return std::get<PARAM_FACTORY_NAME>(
                std::get<PARAM_DEVICE>(std::get<PARAM_DEVICE_CONFIG>(GetParam())));
    }
    const std::string& getDeviceName() const override {
        return std::get<PARAM_DEVICE_NAME>(
                std::get<PARAM_DEVICE>(std::get<PARAM_DEVICE_CONFIG>(GetParam())));
    }
    const AudioConfig& getConfig() const {
        return std::get<PARAM_CONFIG>(std::get<PARAM_DEVICE_CONFIG>(GetParam()));
    }
    const hidl_vec<AudioInOutFlag> getFlags() const {
        return std::get<PARAM_FLAGS>(std::get<PARAM_DEVICE_CONFIG>(GetParam()));
    }
    const DeviceAddress& getDeviceAddress() const { return std::get<PARAM_ADDRESS>(GetParam()); }
    bool isParamForInputStream() const {
        return std::get<PARAM_METADATA>(GetParam()).index() == INDEX_SINK;
    }
    const SinkMetadata& getSinkMetadata() const {
        return std::get<INDEX_SINK>(std::get<PARAM_METADATA>(GetParam()));
    }
    const SourceMetadata& getSourceMetadata() const {
        return std::get<INDEX_SOURCE>(std::get<PARAM_METADATA>(GetParam()));
    }
};

static const RecordTrackMetadata& getValidRecordTrackMetadata() {
    static const RecordTrackMetadata valid = {
            .source = toString(xsd::AudioSource::AUDIO_SOURCE_DEFAULT), .gain = 1};
    return valid;
}

static const RecordTrackMetadata& getValidRecordTrackMetadataWithDest() {
    static const RecordTrackMetadata valid = {
            .source = toString(xsd::AudioSource::AUDIO_SOURCE_DEFAULT),
            .gain = 1,
            .destination = [] {
                RecordTrackMetadata::Destination dest;
                dest.device(getValidOutputDeviceAddress());
                return dest;
            }()};
    return valid;
}

static const RecordTrackMetadata& getInvalidSourceRecordTrackMetadata() {
    static const RecordTrackMetadata invalid = {.source = "random_string", .gain = 1};
    return invalid;
}

static const RecordTrackMetadata& getRecordTrackMetadataWithInvalidDest() {
    static const RecordTrackMetadata invalid = {
            .source = toString(xsd::AudioSource::AUDIO_SOURCE_DEFAULT),
            .gain = 1,
            .destination = [] {
                RecordTrackMetadata::Destination dest;
                dest.device(getInvalidDeviceAddress());
                return dest;
            }()};
    return invalid;
}

static const RecordTrackMetadata& getInvalidChannelMaskRecordTrackMetadata() {
    static const RecordTrackMetadata invalid = {
            .source = toString(xsd::AudioSource::AUDIO_SOURCE_DEFAULT),
            .gain = 1,
            .channelMask = "random_string"};
    return invalid;
}

static const RecordTrackMetadata& getInvalidTagsRecordTrackMetadata() {
    static const RecordTrackMetadata invalid = {
            .source = toString(xsd::AudioSource::AUDIO_SOURCE_DEFAULT),
            .gain = 1,
            .tags = {{"random_string"}}};
    return invalid;
}

static const PlaybackTrackMetadata& getValidPlaybackTrackMetadata() {
    static const PlaybackTrackMetadata valid = {
            .usage = toString(xsd::AudioUsage::AUDIO_USAGE_MEDIA),
            .contentType = toString(xsd::AudioContentType::AUDIO_CONTENT_TYPE_MUSIC),
            .gain = 1};
    return valid;
}

static const PlaybackTrackMetadata& getInvalidUsagePlaybackTrackMetadata() {
    static const PlaybackTrackMetadata invalid = {
            .usage = "random_string",
            .contentType = toString(xsd::AudioContentType::AUDIO_CONTENT_TYPE_MUSIC),
            .gain = 1};
    return invalid;
}

static const PlaybackTrackMetadata& getInvalidContentTypePlaybackTrackMetadata() {
    static const PlaybackTrackMetadata invalid = {
            .usage = toString(xsd::AudioUsage::AUDIO_USAGE_MEDIA),
            .contentType = "random_string",
            .gain = 1};
    return invalid;
}

static const PlaybackTrackMetadata& getInvalidChannelMaskPlaybackTrackMetadata() {
    static const PlaybackTrackMetadata invalid = {
            .usage = toString(xsd::AudioUsage::AUDIO_USAGE_MEDIA),
            .contentType = toString(xsd::AudioContentType::AUDIO_CONTENT_TYPE_MUSIC),
            .gain = 1,
            .channelMask = "random_string"};
    return invalid;
}

static const PlaybackTrackMetadata& getInvalidTagsPlaybackTrackMetadata() {
    static const PlaybackTrackMetadata invalid = {
            .usage = toString(xsd::AudioUsage::AUDIO_USAGE_MEDIA),
            .contentType = toString(xsd::AudioContentType::AUDIO_CONTENT_TYPE_MUSIC),
            .gain = 1,
            .tags = {{"random_string"}}};
    return invalid;
}

static const std::vector<SourceMetadata>& getInvalidSourceMetadatas() {
    static const std::vector<SourceMetadata> invalids = {
            SourceMetadata{.tracks = {{getInvalidUsagePlaybackTrackMetadata()}}},
            SourceMetadata{.tracks = {{getInvalidContentTypePlaybackTrackMetadata()}}},
            SourceMetadata{.tracks = {{getInvalidChannelMaskPlaybackTrackMetadata()}}},
            SourceMetadata{.tracks = {{getInvalidTagsPlaybackTrackMetadata()}}},
            SourceMetadata{.tracks = {{getValidPlaybackTrackMetadata(),
                                       getInvalidUsagePlaybackTrackMetadata()}}},
            SourceMetadata{.tracks = {{getValidPlaybackTrackMetadata(),
                                       getInvalidContentTypePlaybackTrackMetadata()}}},
            SourceMetadata{.tracks = {{getValidPlaybackTrackMetadata(),
                                       getInvalidChannelMaskPlaybackTrackMetadata()}}},
            SourceMetadata{.tracks = {{getValidPlaybackTrackMetadata(),
                                       getInvalidTagsPlaybackTrackMetadata()}}}};
    return invalids;
}
static const std::vector<SinkMetadata>& getInvalidSinkMetadatas() {
    static const std::vector<SinkMetadata> invalids = {
            SinkMetadata{.tracks = {{getInvalidSourceRecordTrackMetadata()}}},
            SinkMetadata{.tracks = {{getRecordTrackMetadataWithInvalidDest()}}},
            SinkMetadata{.tracks = {{getInvalidChannelMaskRecordTrackMetadata()}}},
            SinkMetadata{.tracks = {{getInvalidTagsRecordTrackMetadata()}}},
            SinkMetadata{.tracks = {{getValidRecordTrackMetadata(),
                                     getInvalidSourceRecordTrackMetadata()}}},
            SinkMetadata{.tracks = {{getValidRecordTrackMetadata(),
                                     getRecordTrackMetadataWithInvalidDest()}}},
            SinkMetadata{.tracks = {{getValidRecordTrackMetadata(),
                                     getInvalidChannelMaskRecordTrackMetadata()}}},
            SinkMetadata{.tracks = {{getValidRecordTrackMetadata(),
                                     getInvalidTagsRecordTrackMetadata()}}}};
    return invalids;
}
template <typename T>
static inline std::vector<SinkOrSourceMetadata> wrapMetadata(const std::vector<T>& metadata) {
    return std::vector<SinkOrSourceMetadata>{metadata.begin(), metadata.end()};
}

TEST_P(StreamOpenTest, OpenInputOrOutputStreamTest) {
    doc::test(
            "Verify that invalid arguments are rejected by "
            "IDevice::open{Input|Output}Stream method.");
    AudioConfig suggestedConfig{};
    if (isParamForInputStream()) {
        sp<IStreamIn> stream;
        ASSERT_OK(getDevice()->openInputStream(AudioIoHandle{}, getDeviceAddress(), getConfig(),
                                               getFlags(), getSinkMetadata(),
                                               returnIn(res, stream, suggestedConfig)));
        ASSERT_TRUE(stream == nullptr);
    } else {
        sp<IStreamOut> stream;
        ASSERT_OK(getDevice()->openOutputStream(AudioIoHandle{}, getDeviceAddress(), getConfig(),
                                                getFlags(), getSourceMetadata(),
                                                returnIn(res, stream, suggestedConfig)));
        ASSERT_TRUE(stream == nullptr);
    }
    EXPECT_EQ(Result::INVALID_ARGUMENTS, res);
    EXPECT_EQ(AudioConfig{}, suggestedConfig);
}
INSTANTIATE_TEST_CASE_P(
        InputStreamInvalidConfig, StreamOpenTest,
        ::testing::Combine(::testing::ValuesIn(getInputDeviceInvalidConfigParameters()),
                           ::testing::Values(getValidInputDeviceAddress()),
                           ::testing::Values(SinkMetadata{
                                   .tracks = {{getValidRecordTrackMetadata(),
                                               getValidRecordTrackMetadataWithDest()}}})),
        &StreamOpenParameterToString);
INSTANTIATE_TEST_CASE_P(
        InputStreamInvalidAddress, StreamOpenTest,
        ::testing::Combine(::testing::ValuesIn(getInputDeviceSingleConfigParameters()),
                           ::testing::Values(getInvalidDeviceAddress()),
                           ::testing::Values(SinkMetadata{
                                   .tracks = {{getValidRecordTrackMetadata(),
                                               getValidRecordTrackMetadataWithDest()}}})),
        &StreamOpenParameterToString);
INSTANTIATE_TEST_CASE_P(
        InputStreamInvalidMetadata, StreamOpenTest,
        ::testing::Combine(::testing::ValuesIn(getInputDeviceSingleConfigParameters()),
                           ::testing::Values(getValidInputDeviceAddress()),
                           ::testing::ValuesIn(wrapMetadata(getInvalidSinkMetadatas()))),
        &StreamOpenParameterToString);
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(StreamOpenTest);

INSTANTIATE_TEST_CASE_P(
        OutputStreamInvalidConfig, StreamOpenTest,
        ::testing::Combine(::testing::ValuesIn(getOutputDeviceInvalidConfigParameters()),
                           ::testing::Values(getValidOutputDeviceAddress()),
                           ::testing::Values(SourceMetadata{
                                   .tracks = {{getValidPlaybackTrackMetadata()}}})),
        &StreamOpenParameterToString);
INSTANTIATE_TEST_CASE_P(
        OutputStreamInvalidAddress, StreamOpenTest,
        ::testing::Combine(::testing::ValuesIn(getOutputDeviceSingleConfigParameters()),
                           ::testing::Values(getInvalidDeviceAddress()),
                           ::testing::Values(SourceMetadata{
                                   .tracks = {{getValidPlaybackTrackMetadata()}}})),
        &StreamOpenParameterToString);
INSTANTIATE_TEST_CASE_P(
        OutputStreamInvalidMetadata, StreamOpenTest,
        ::testing::Combine(::testing::ValuesIn(getOutputDeviceSingleConfigParameters()),
                           ::testing::Values(getValidOutputDeviceAddress()),
                           ::testing::ValuesIn(wrapMetadata(getInvalidSourceMetadatas()))),
        &StreamOpenParameterToString);

#define TEST_SINGLE_CONFIG_IO_STREAM(test_name, documentation, code) \
    TEST_P(SingleConfigInputStreamTest, test_name) {                 \
        doc::test(documentation);                                    \
        code;                                                        \
    }                                                                \
    TEST_P(SingleConfigOutputStreamTest, test_name) {                \
        doc::test(documentation);                                    \
        code;                                                        \
    }

static void testSetDevicesInvalidDeviceAddress(IStream* stream) {
    ASSERT_RESULT(Result::INVALID_ARGUMENTS, stream->setDevices({getInvalidDeviceAddress()}));
}
TEST_SINGLE_CONFIG_IO_STREAM(
        SetDevicesInvalidDeviceAddress,
        "Verify that invalid device address is rejected by IStream::setDevices",
        areAudioPatchesSupported() ? doc::partialTest("Audio patches are supported")
                                   : testSetDevicesInvalidDeviceAddress(stream.get()));

static void testSetAudioPropertiesInvalidArguments(IStream* stream) {
    AudioConfigBaseOptional invalidFormat;
    invalidFormat.format.value("random_string");
    ASSERT_RESULT(invalidArgsOrNotSupported, stream->setAudioProperties(invalidFormat));

    AudioConfigBaseOptional invalidChannelMask;
    invalidChannelMask.channelMask.value("random_string");
    ASSERT_RESULT(invalidArgsOrNotSupported, stream->setAudioProperties(invalidChannelMask));
}
TEST_SINGLE_CONFIG_IO_STREAM(
        SetAudioPropertiesInvalidArguments,
        "Verify that invalid arguments are rejected by IStream::setAudioProperties",
        testSetAudioPropertiesInvalidArguments(stream.get()));

TEST_P(SingleConfigOutputStreamTest, UpdateInvalidSourceMetadata) {
    doc::test("Verify that invalid metadata is rejected by IStreamOut::updateSourceMetadata");
    for (const auto& metadata : getInvalidSourceMetadatas()) {
        ASSERT_RESULT(invalidArgsOrNotSupported, stream->updateSourceMetadata(metadata))
                << ::testing::PrintToString(metadata);
    }
}

TEST_P(SingleConfigInputStreamTest, UpdateInvalidSinkMetadata) {
    doc::test("Verify that invalid metadata is rejected by IStreamIn::updateSinkMetadata");
    for (const auto& metadata : getInvalidSinkMetadatas()) {
        ASSERT_RESULT(invalidArgsOrNotSupported, stream->updateSinkMetadata(metadata))
                << ::testing::PrintToString(metadata);
    }
}

static const std::vector<DeviceConfigParameter>& getOutputDevicePcmOnlyConfigParameters() {
    static const std::vector<DeviceConfigParameter> parameters = [] {
        auto allParams = getOutputDeviceConfigParameters();
        std::vector<DeviceConfigParameter> pcmParams;
        std::copy_if(allParams.begin(), allParams.end(), std::back_inserter(pcmParams), [](auto cfg) {
            const auto& flags = std::get<PARAM_FLAGS>(cfg);
            return xsd::isLinearPcm(std::get<PARAM_CONFIG>(cfg).base.format)
                   // MMAP NOIRQ and HW A/V Sync profiles use special writing protocols.
                   &&
                   std::find_if(flags.begin(), flags.end(), [](const auto& flag) {
                       return flag == toString(xsd::AudioInOutFlag::AUDIO_OUTPUT_FLAG_MMAP_NOIRQ) ||
                              flag == toString(xsd::AudioInOutFlag::AUDIO_OUTPUT_FLAG_HW_AV_SYNC);
                   }) == flags.end();
        });
        return pcmParams;
    }();
    return parameters;
}

class PcmOnlyConfigOutputStreamTest : public OutputStreamTest {
  public:
    void TearDown() override {
        releasePatchIfNeeded();
        OutputStreamTest::TearDown();
    }

    bool canQueryPresentationPosition() const {
        auto maybeSinkAddress =
                getCachedPolicyConfig().getSinkDeviceForMixPort(getDeviceName(), getMixPortName());
        // Returning 'true' when no sink is found so the test can fail later with a more clear
        // problem description.
        return !maybeSinkAddress.has_value() ||
               !xsd::isTelephonyDevice(maybeSinkAddress.value().deviceType);
    }

    void createPatchIfNeeded() {
        auto maybeSinkAddress =
                getCachedPolicyConfig().getSinkDeviceForMixPort(getDeviceName(), getMixPortName());
        ASSERT_TRUE(maybeSinkAddress.has_value())
                << "No sink device found for mix port " << getMixPortName() << " (module "
                << getDeviceName() << ")";
        if (areAudioPatchesSupported()) {
            AudioPortConfig source;
            source.base.format.value(getConfig().base.format);
            source.base.sampleRateHz.value(getConfig().base.sampleRateHz);
            source.base.channelMask.value(getConfig().base.channelMask);
            source.ext.mix({});
            source.ext.mix().ioHandle = helper.getIoHandle();
            source.ext.mix().useCase.stream({});
            AudioPortConfig sink;
            sink.ext.device(maybeSinkAddress.value());
            EXPECT_OK(getDevice()->createAudioPatch(hidl_vec<AudioPortConfig>{source},
                                                    hidl_vec<AudioPortConfig>{sink},
                                                    returnIn(res, mPatchHandle)));
            mHasPatch = res == Result::OK;
        } else {
            EXPECT_OK(stream->setDevices({maybeSinkAddress.value()}));
        }
    }

    void releasePatchIfNeeded() {
        if (areAudioPatchesSupported()) {
            if (mHasPatch) {
                EXPECT_OK(getDevice()->releaseAudioPatch(mPatchHandle));
                mHasPatch = false;
            }
        } else {
            EXPECT_OK(stream->setDevices({address}));
        }
    }

    const std::string& getMixPortName() const { return std::get<PARAM_PORT_NAME>(GetParam()); }

    void waitForPresentationPositionAdvance(StreamWriter& writer, uint64_t* firstPosition = nullptr,
                                            uint64_t* lastPosition = nullptr) {
        static constexpr int kWriteDurationUs = 50 * 1000;
        static constexpr std::chrono::milliseconds kPositionChangeTimeout{10000};
        uint64_t framesInitial;
        TimeSpec ts;
        // Starting / resuming of streams is asynchronous at HAL level.
        // Sometimes HAL doesn't have enough information until the audio data actually gets
        // consumed by the hardware.
        bool timedOut = false;
        res = Result::INVALID_STATE;
        for (android::base::Timer elapsed;
             res != Result::OK && !writer.hasError() &&
             !(timedOut = (elapsed.duration() >= kPositionChangeTimeout));) {
            usleep(kWriteDurationUs);
            ASSERT_OK(stream->getPresentationPosition(returnIn(res, framesInitial, ts)));
            ASSERT_RESULT(okOrInvalidState, res);
        }
        ASSERT_FALSE(writer.hasError());
        ASSERT_FALSE(timedOut);

        uint64_t frames = framesInitial;
        for (android::base::Timer elapsed;
             frames <= framesInitial && !writer.hasError() &&
             !(timedOut = (elapsed.duration() >= kPositionChangeTimeout));) {
            usleep(kWriteDurationUs);
            ASSERT_OK(stream->getPresentationPosition(returnIn(res, frames, ts)));
            ASSERT_RESULT(Result::OK, res);
        }
        EXPECT_FALSE(timedOut);
        EXPECT_FALSE(writer.hasError());
        EXPECT_GT(frames, framesInitial);
        if (firstPosition) *firstPosition = framesInitial;
        if (lastPosition) *lastPosition = frames;
    }

  private:
    AudioPatchHandle mPatchHandle = {};
    bool mHasPatch = false;
};

TEST_P(PcmOnlyConfigOutputStreamTest, Write) {
    doc::test("Check that output streams opened for PCM output accepts audio data");
    StreamWriter writer(stream.get(), stream->getBufferSize());
    ASSERT_TRUE(writer.start());
    EXPECT_TRUE(writer.waitForAtLeastOneCycle());
}

TEST_P(PcmOnlyConfigOutputStreamTest, PresentationPositionAdvancesWithWrites) {
    doc::test("Check that the presentation position advances with writes");
    if (!canQueryPresentationPosition()) {
        GTEST_SKIP() << "Presentation position retrieval is not possible";
    }

    ASSERT_NO_FATAL_FAILURE(createPatchIfNeeded());
    StreamWriter writer(stream.get(), stream->getBufferSize());
    ASSERT_TRUE(writer.start());
    ASSERT_TRUE(writer.waitForAtLeastOneCycle());
    ASSERT_NO_FATAL_FAILURE(waitForPresentationPositionAdvance(writer));

    writer.stop();
    releasePatchIfNeeded();
}

TEST_P(PcmOnlyConfigOutputStreamTest, PresentationPositionPreservedOnStandby) {
    doc::test("Check that the presentation position does not reset on standby");
    if (!canQueryPresentationPosition()) {
        GTEST_SKIP() << "Presentation position retrieval is not possible";
    }

    ASSERT_NO_FATAL_FAILURE(createPatchIfNeeded());
    StreamWriter writer(stream.get(), stream->getBufferSize());
    ASSERT_TRUE(writer.start());
    ASSERT_TRUE(writer.waitForAtLeastOneCycle());

    uint64_t framesInitial;
    ASSERT_NO_FATAL_FAILURE(waitForPresentationPositionAdvance(writer, nullptr, &framesInitial));
    writer.pause();
    ASSERT_OK(stream->standby());
    writer.resume();

    uint64_t frames;
    ASSERT_NO_FATAL_FAILURE(waitForPresentationPositionAdvance(writer, &frames));
    EXPECT_GT(frames, framesInitial);

    writer.stop();
    releasePatchIfNeeded();
}

INSTANTIATE_TEST_CASE_P(PcmOnlyConfigOutputStream, PcmOnlyConfigOutputStreamTest,
                        ::testing::ValuesIn(getOutputDevicePcmOnlyConfigParameters()),
                        &DeviceConfigParameterToString);
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(PcmOnlyConfigOutputStreamTest);

static const std::vector<DeviceConfigParameter>& getInputDevicePcmOnlyConfigParameters() {
    static const std::vector<DeviceConfigParameter> parameters = [] {
        auto allParams = getInputDeviceConfigParameters();
        std::vector<DeviceConfigParameter> pcmParams;
        std::copy_if(
                allParams.begin(), allParams.end(), std::back_inserter(pcmParams), [](auto cfg) {
                    const auto& flags = std::get<PARAM_FLAGS>(cfg);
                    return xsd::isLinearPcm(std::get<PARAM_CONFIG>(cfg).base.format)
                           // MMAP NOIRQ profiles use different reading protocol,
                           // reading h/w hotword might require Soundtrigger to be active.
                           &&
                           std::find_if(
                                   flags.begin(), flags.end(), [](const auto& flag) {
                                       return flag == toString(
                                                              xsd::AudioInOutFlag::
                                                                      AUDIO_INPUT_FLAG_MMAP_NOIRQ) ||
                                              flag == toString(xsd::AudioInOutFlag::
                                                                       AUDIO_INPUT_FLAG_HW_HOTWORD);
                                   }) == flags.end();
                });
        return pcmParams;
    }();
    return parameters;
}

class PcmOnlyConfigInputStreamTest : public InputStreamTest {
  public:
    void TearDown() override {
        releasePatchIfNeeded();
        InputStreamTest::TearDown();
    }

    bool canQueryCapturePosition() const {
        auto maybeSourceAddress = getCachedPolicyConfig().getSourceDeviceForMixPort(
                getDeviceName(), getMixPortName());
        // Returning 'true' when no source is found so the test can fail later with a more clear
        // problem description.
        return !maybeSourceAddress.has_value() ||
               !xsd::isTelephonyDevice(maybeSourceAddress.value().deviceType);
    }

    void createPatchIfNeeded() {
        auto maybeSourceAddress = getCachedPolicyConfig().getSourceDeviceForMixPort(
                getDeviceName(), getMixPortName());
        ASSERT_TRUE(maybeSourceAddress.has_value())
                << "No source device found for mix port " << getMixPortName() << " (module "
                << getDeviceName() << ")";
        if (areAudioPatchesSupported()) {
            AudioPortConfig source;
            source.ext.device(maybeSourceAddress.value());
            AudioPortConfig sink;
            sink.base.format.value(getConfig().base.format);
            sink.base.sampleRateHz.value(getConfig().base.sampleRateHz);
            sink.base.channelMask.value(getConfig().base.channelMask);
            sink.ext.mix({});
            sink.ext.mix().ioHandle = helper.getIoHandle();
            sink.ext.mix().useCase.source(toString(xsd::AudioSource::AUDIO_SOURCE_MIC));
            EXPECT_OK(getDevice()->createAudioPatch(hidl_vec<AudioPortConfig>{source},
                                                    hidl_vec<AudioPortConfig>{sink},
                                                    returnIn(res, mPatchHandle)));
            mHasPatch = res == Result::OK;
        } else {
            EXPECT_OK(stream->setDevices({maybeSourceAddress.value()}));
        }
    }

    void releasePatchIfNeeded() {
        if (areAudioPatchesSupported()) {
            if (mHasPatch) {
                EXPECT_OK(getDevice()->releaseAudioPatch(mPatchHandle));
                mHasPatch = false;
            }
        } else {
            EXPECT_OK(stream->setDevices({address}));
        }
    }

    void waitForCapturePositionAdvance(StreamReader& reader, uint64_t* firstPosition = nullptr,
                                       uint64_t* lastPosition = nullptr) {
        static constexpr int kReadDurationUs = 50 * 1000;
        static constexpr std::chrono::milliseconds kPositionChangeTimeout{10000};
        uint64_t framesInitial, ts;
        // Starting / resuming of streams is asynchronous at HAL level.
        // Sometimes HAL doesn't have enough information until the audio data actually has been
        // produced by the hardware. Legacy HALs might return NOT_SUPPORTED when they actually
        // mean INVALID_STATE.
        bool timedOut = false;
        res = Result::INVALID_STATE;
        for (android::base::Timer elapsed;
             res != Result::OK && !reader.hasError() &&
             !(timedOut = (elapsed.duration() >= kPositionChangeTimeout));) {
            usleep(kReadDurationUs);
            ASSERT_OK(stream->getCapturePosition(returnIn(res, framesInitial, ts)));
            ASSERT_RESULT(okOrInvalidStateOrNotSupported, res);
        }
        ASSERT_FALSE(reader.hasError());
        ASSERT_FALSE(timedOut);

        uint64_t frames = framesInitial;
        for (android::base::Timer elapsed;
             frames <= framesInitial && !reader.hasError() &&
             !(timedOut = (elapsed.duration() >= kPositionChangeTimeout));) {
            usleep(kReadDurationUs);
            ASSERT_OK(stream->getCapturePosition(returnIn(res, frames, ts)));
            ASSERT_RESULT(Result::OK, res);
        }
        EXPECT_FALSE(timedOut);
        EXPECT_FALSE(reader.hasError());
        EXPECT_GT(frames, framesInitial);
        if (firstPosition) *firstPosition = framesInitial;
        if (lastPosition) *lastPosition = frames;
    }

  private:
    AudioPatchHandle mPatchHandle = {};
    bool mHasPatch = false;
};

TEST_P(PcmOnlyConfigInputStreamTest, Read) {
    doc::test("Check that input streams opened for PCM input retrieve audio data");
    StreamReader reader(stream.get(), stream->getBufferSize());
    ASSERT_TRUE(reader.start());
    EXPECT_TRUE(reader.waitForAtLeastOneCycle());
}

TEST_P(PcmOnlyConfigInputStreamTest, CapturePositionAdvancesWithReads) {
    doc::test("Check that the capture position advances with reads");
    if (!canQueryCapturePosition()) {
        GTEST_SKIP() << "Capture position retrieval is not possible";
    }

    ASSERT_NO_FATAL_FAILURE(createPatchIfNeeded());
    StreamReader reader(stream.get(), stream->getBufferSize());
    ASSERT_TRUE(reader.start());
    EXPECT_TRUE(reader.waitForAtLeastOneCycle());
    ASSERT_NO_FATAL_FAILURE(waitForCapturePositionAdvance(reader));
}

TEST_P(PcmOnlyConfigInputStreamTest, CapturePositionPreservedOnStandby) {
    doc::test("Check that the capture position does not reset on standby");
    if (!canQueryCapturePosition()) {
        GTEST_SKIP() << "Capture position retrieval is not possible";
    }

    ASSERT_NO_FATAL_FAILURE(createPatchIfNeeded());
    StreamReader reader(stream.get(), stream->getBufferSize());
    ASSERT_TRUE(reader.start());
    EXPECT_TRUE(reader.waitForAtLeastOneCycle());

    uint64_t framesInitial;
    ASSERT_NO_FATAL_FAILURE(waitForCapturePositionAdvance(reader, nullptr, &framesInitial));
    reader.pause();
    ASSERT_OK(stream->standby());
    reader.resume();

    uint64_t frames;
    ASSERT_NO_FATAL_FAILURE(waitForCapturePositionAdvance(reader, &frames, nullptr));
    EXPECT_GT(frames, framesInitial);

    reader.stop();
    releasePatchIfNeeded();
}

INSTANTIATE_TEST_CASE_P(PcmOnlyConfigInputStream, PcmOnlyConfigInputStreamTest,
                        ::testing::ValuesIn(getInputDevicePcmOnlyConfigParameters()),
                        &DeviceConfigParameterToString);
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(PcmOnlyConfigInputStreamTest);

static const std::vector<DeviceConfigParameter>& getBuiltinMicConfigParameters() {
    static const std::vector<DeviceConfigParameter> parameters = [] {
        auto allParams = getInputDeviceConfigParameters();
        std::vector<DeviceConfigParameter> builtinMicParams;
        std::copy_if(allParams.begin(), allParams.end(), std::back_inserter(builtinMicParams),
                     [](auto cfg) {
                         // The built in mic may participate in various scenarios:
                         // FAST, HW_HOTWORD, MMAP NOIRQ, which are indicated by flags.
                         // We are only interested in testing the simplest scenario w/o any flags.
                         if (!std::get<PARAM_FLAGS>(cfg).empty()) return false;
                         auto maybeSourceDevice = getCachedPolicyConfig().getSourceDeviceForMixPort(
                                 std::get<PARAM_DEVICE_NAME>(std::get<PARAM_DEVICE>(cfg)),
                                 std::get<PARAM_PORT_NAME>(cfg));
                         return maybeSourceDevice.has_value() &&
                                xsd::stringToAudioDevice(maybeSourceDevice.value().deviceType) ==
                                        xsd::AudioDevice::AUDIO_DEVICE_IN_BUILTIN_MIC;
                     });
        return builtinMicParams;
    }();
    return parameters;
}

class MicrophoneInfoInputStreamTest : public InputStreamTest {};

TEST_P(MicrophoneInfoInputStreamTest, GetActiveMicrophones) {
    doc::test(
            "Make sure getActiveMicrophones always succeeds when recording "
            "from the built-in microphone.");
    hidl_vec<MicrophoneInfo> microphones;
    ASSERT_OK(getDevice()->getMicrophones(returnIn(res, microphones)));
    if (res == Result::NOT_SUPPORTED) {
        GTEST_SKIP() << "getMicrophones is not supported";  // returns
    }
    ASSERT_OK(res);

    auto maybeSourceAddress =
            getCachedPolicyConfig().getSourceDeviceForMixPort(getDeviceName(), getMixPortName());
    ASSERT_TRUE(maybeSourceAddress.has_value())
            << "No source device found for mix port " << getMixPortName() << " (module "
            << getDeviceName() << ")";

    for (auto microphone : microphones) {
        if (microphone.deviceAddress == maybeSourceAddress.value()) {
            StreamReader reader(stream.get(), stream->getBufferSize());
            ASSERT_TRUE(reader.start());
            reader.pause();  // This ensures that at least one read has happened.
            EXPECT_FALSE(reader.hasError());

            hidl_vec<MicrophoneInfo> activeMicrophones;
            ASSERT_OK(stream->getActiveMicrophones(returnIn(res, activeMicrophones)));
            ASSERT_OK(res);
            EXPECT_NE(0U, activeMicrophones.size());
        }
    }
}

INSTANTIATE_TEST_CASE_P(MicrophoneInfoInputStream, MicrophoneInfoInputStreamTest,
                        ::testing::ValuesIn(getBuiltinMicConfigParameters()),
                        &DeviceConfigParameterToString);
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(MicrophoneInfoInputStreamTest);
