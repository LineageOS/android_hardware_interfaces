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

// pull in all the <= 6.0 tests
#include "6.0/AudioPrimaryHidlHalTest.cpp"

static std::vector<AudioConfig> combineAudioConfig(std::vector<xsd::AudioChannelMask> channelMasks,
                                                   std::vector<int64_t> sampleRates,
                                                   const std::string& format) {
    std::vector<AudioConfig> configs;
    configs.reserve(channelMasks.size() * sampleRates.size());
    for (auto channelMask : channelMasks) {
        for (auto sampleRate : sampleRates) {
            AudioConfig config{};
            config.base.channelMask = toString(channelMask);
            config.base.sampleRateHz = sampleRate;
            config.base.format = format;
            configs.push_back(config);
        }
    }
    return configs;
}

static std::tuple<std::vector<AudioInOutFlag>, bool> generateOutFlags(
        const xsd::MixPorts::MixPort& mixPort) {
    static const std::vector<AudioInOutFlag> offloadFlags = {
            toString(xsd::AudioInOutFlag::AUDIO_OUTPUT_FLAG_COMPRESS_OFFLOAD),
            toString(xsd::AudioInOutFlag::AUDIO_OUTPUT_FLAG_DIRECT)};
    std::vector<AudioInOutFlag> flags;
    bool isOffload = false;
    if (mixPort.hasFlags()) {
        auto xsdFlags = mixPort.getFlags();
        isOffload = std::find(xsdFlags.begin(), xsdFlags.end(),
                              xsd::AudioInOutFlag::AUDIO_OUTPUT_FLAG_COMPRESS_OFFLOAD) !=
                    xsdFlags.end();
        if (!isOffload) {
            for (auto flag : xsdFlags) {
                if (flag != xsd::AudioInOutFlag::AUDIO_OUTPUT_FLAG_PRIMARY) {
                    flags.push_back(toString(flag));
                }
            }
        } else {
            flags = offloadFlags;
        }
    }
    return {flags, isOffload};
}

static AudioOffloadInfo generateOffloadInfo(const AudioConfigBase& base) {
    return AudioOffloadInfo{
            .base = base,
            .streamType = toString(xsd::AudioStreamType::AUDIO_STREAM_MUSIC),
            .usage = toString(xsd::AudioUsage::AUDIO_USAGE_MEDIA),
            .bitRatePerSecond = 320,
            .durationMicroseconds = -1,
            .bitWidth = 16,
            .bufferSize = 256  // arbitrary value
    };
}

static std::vector<DeviceConfigParameter> generateOutputDeviceConfigParameters(
        bool oneProfilePerDevice) {
    std::vector<DeviceConfigParameter> result;
    for (const auto& device : getDeviceParameters()) {
        auto module =
                getCachedPolicyConfig().getModuleFromName(std::get<PARAM_DEVICE_NAME>(device));
        if (!module || !module->getFirstMixPorts()) break;
        for (const auto& mixPort : module->getFirstMixPorts()->getMixPort()) {
            if (mixPort.getRole() != xsd::Role::source) continue;  // not an output profile
            auto [flags, isOffload] = generateOutFlags(mixPort);
            for (const auto& profile : mixPort.getProfile()) {
                auto configs = combineAudioConfig(profile.getChannelMasks(),
                                                  profile.getSamplingRates(), profile.getFormat());
                for (auto& config : configs) {
                    // Some combinations of flags declared in the config file require special
                    // treatment.
                    if (isOffload) {
                        config.offloadInfo.info(generateOffloadInfo(config.base));
                    }
                    result.emplace_back(device, config, flags);
                    if (oneProfilePerDevice) break;
                }
                if (oneProfilePerDevice) break;
            }
            if (oneProfilePerDevice) break;
        }
    }
    return result;
}

const std::vector<DeviceConfigParameter>& getOutputDeviceConfigParameters() {
    static std::vector<DeviceConfigParameter> parameters =
            generateOutputDeviceConfigParameters(false);
    return parameters;
}

const std::vector<DeviceConfigParameter>& getOutputDeviceSingleConfigParameters() {
    static std::vector<DeviceConfigParameter> parameters =
            generateOutputDeviceConfigParameters(true);
    return parameters;
}

const std::vector<DeviceConfigParameter>& getOutputDeviceInvalidConfigParameters(
        bool generateInvalidFlags = true) {
    static std::vector<DeviceConfigParameter> parameters = [&] {
        std::vector<DeviceConfigParameter> result;
        for (const auto& device : getDeviceParameters()) {
            auto module =
                    getCachedPolicyConfig().getModuleFromName(std::get<PARAM_DEVICE_NAME>(device));
            if (!module || !module->getFirstMixPorts()) break;
            bool hasRegularConfig = false, hasOffloadConfig = false;
            for (const auto& mixPort : module->getFirstMixPorts()->getMixPort()) {
                if (mixPort.getRole() != xsd::Role::source) continue;  // not an output profile
                auto [validFlags, isOffload] = generateOutFlags(mixPort);
                if ((!isOffload && hasRegularConfig) || (isOffload && hasOffloadConfig)) continue;
                for (const auto& profile : mixPort.getProfile()) {
                    if (!profile.hasFormat() || !profile.hasSamplingRates() ||
                        !profile.hasChannelMasks())
                        continue;
                    AudioConfigBase validBase = {
                            profile.getFormat(),
                            static_cast<uint32_t>(profile.getSamplingRates()[0]),
                            toString(profile.getChannelMasks()[0])};
                    {
                        AudioConfig config{.base = validBase};
                        config.base.channelMask = "random_string";
                        if (isOffload) {
                            config.offloadInfo.info(generateOffloadInfo(validBase));
                        }
                        result.emplace_back(device, config, validFlags);
                    }
                    {
                        AudioConfig config{.base = validBase};
                        config.base.format = "random_string";
                        if (isOffload) {
                            config.offloadInfo.info(generateOffloadInfo(validBase));
                        }
                        result.emplace_back(device, config, validFlags);
                    }
                    if (generateInvalidFlags) {
                        AudioConfig config{.base = validBase};
                        if (isOffload) {
                            config.offloadInfo.info(generateOffloadInfo(validBase));
                        }
                        std::vector<AudioInOutFlag> flags = {"random_string", ""};
                        result.emplace_back(device, config, flags);
                    }
                    if (isOffload) {
                        {
                            AudioConfig config{.base = validBase};
                            config.offloadInfo.info(generateOffloadInfo(validBase));
                            config.offloadInfo.info().base.channelMask = "random_string";
                        }
                        {
                            AudioConfig config{.base = validBase};
                            config.offloadInfo.info(generateOffloadInfo(validBase));
                            config.offloadInfo.info().base.format = "random_string";
                        }
                        {
                            AudioConfig config{.base = validBase};
                            config.offloadInfo.info(generateOffloadInfo(validBase));
                            config.offloadInfo.info().streamType = "random_string";
                        }
                        {
                            AudioConfig config{.base = validBase};
                            config.offloadInfo.info(generateOffloadInfo(validBase));
                            config.offloadInfo.info().usage = "random_string";
                        }
                        hasOffloadConfig = true;
                    } else {
                        hasRegularConfig = true;
                    }
                    break;
                }
                if (hasOffloadConfig && hasRegularConfig) break;
            }
        }
        return result;
    }();
    return parameters;
}

static std::vector<DeviceConfigParameter> generateInputDeviceConfigParameters(
        bool oneProfilePerDevice) {
    std::vector<DeviceConfigParameter> result;
    for (const auto& device : getDeviceParameters()) {
        auto module =
                getCachedPolicyConfig().getModuleFromName(std::get<PARAM_DEVICE_NAME>(device));
        if (!module || !module->getFirstMixPorts()) break;
        for (const auto& mixPort : module->getFirstMixPorts()->getMixPort()) {
            if (mixPort.getRole() != xsd::Role::sink) continue;  // not an input profile
            std::vector<AudioInOutFlag> flags;
            if (mixPort.hasFlags()) {
                std::transform(mixPort.getFlags().begin(), mixPort.getFlags().end(),
                               std::back_inserter(flags), [](auto flag) { return toString(flag); });
            }
            for (const auto& profile : mixPort.getProfile()) {
                auto configs = combineAudioConfig(profile.getChannelMasks(),
                                                  profile.getSamplingRates(), profile.getFormat());
                for (const auto& config : configs) {
                    result.emplace_back(device, config, flags);
                    if (oneProfilePerDevice) break;
                }
                if (oneProfilePerDevice) break;
            }
            if (oneProfilePerDevice) break;
        }
    }
    return result;
}

const std::vector<DeviceConfigParameter>& getInputDeviceConfigParameters() {
    static std::vector<DeviceConfigParameter> parameters =
            generateInputDeviceConfigParameters(false);
    return parameters;
}

const std::vector<DeviceConfigParameter>& getInputDeviceSingleConfigParameters() {
    static std::vector<DeviceConfigParameter> parameters =
            generateInputDeviceConfigParameters(true);
    return parameters;
}

const std::vector<DeviceConfigParameter>& getInputDeviceInvalidConfigParameters(
        bool generateInvalidFlags = true) {
    static std::vector<DeviceConfigParameter> parameters = [&] {
        std::vector<DeviceConfigParameter> result;
        for (const auto& device : getDeviceParameters()) {
            auto module =
                    getCachedPolicyConfig().getModuleFromName(std::get<PARAM_DEVICE_NAME>(device));
            if (!module || !module->getFirstMixPorts()) break;
            bool hasConfig = false;
            for (const auto& mixPort : module->getFirstMixPorts()->getMixPort()) {
                if (mixPort.getRole() != xsd::Role::sink) continue;  // not an input profile
                std::vector<AudioInOutFlag> validFlags;
                if (mixPort.hasFlags()) {
                    std::transform(mixPort.getFlags().begin(), mixPort.getFlags().end(),
                                   std::back_inserter(validFlags),
                                   [](auto flag) { return toString(flag); });
                }
                for (const auto& profile : mixPort.getProfile()) {
                    if (!profile.hasFormat() || !profile.hasSamplingRates() ||
                        !profile.hasChannelMasks())
                        continue;
                    AudioConfigBase validBase = {
                            profile.getFormat(),
                            static_cast<uint32_t>(profile.getSamplingRates()[0]),
                            toString(profile.getChannelMasks()[0])};
                    {
                        AudioConfig config{.base = validBase};
                        config.base.channelMask = "random_string";
                        result.emplace_back(device, config, validFlags);
                    }
                    {
                        AudioConfig config{.base = validBase};
                        config.base.format = "random_string";
                        result.emplace_back(device, config, validFlags);
                    }
                    if (generateInvalidFlags) {
                        AudioConfig config{.base = validBase};
                        std::vector<AudioInOutFlag> flags = {"random_string", ""};
                        result.emplace_back(device, config, flags);
                    }
                    hasConfig = true;
                    break;
                }
                if (hasConfig) break;
            }
        }
        return result;
    }();
    return parameters;
}

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
        invalidBaseChannelMask.base.channelMask = "random_string";
        result.push_back(std::move(invalidBaseChannelMask));
        AudioPortConfig invalidBaseFormat = valids[PORT_CONF_MINIMAL];
        invalidBaseFormat.base.format = "random_string";
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

static void testSetAudioPropertiesInvalidArguments(IStream* stream, const AudioConfigBase& base) {
    AudioConfigBase invalidFormat = base;
    invalidFormat.format = "random_string";
    ASSERT_RESULT(invalidArgsOrNotSupported, stream->setAudioProperties(invalidFormat));

    AudioConfigBase invalidChannelMask = base;
    invalidChannelMask.channelMask = "random_string";
    ASSERT_RESULT(invalidArgsOrNotSupported, stream->setAudioProperties(invalidChannelMask));
}
TEST_SINGLE_CONFIG_IO_STREAM(
        SetAudioPropertiesInvalidArguments,
        "Verify that invalid arguments are rejected by IStream::setAudioProperties",
        testSetAudioPropertiesInvalidArguments(stream.get(), audioConfig.base));

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
