/*
 * Copyright (C) 2017 The Android Open Source Project
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

#define LOG_TAG "VtsHalAudioV2_0TargetTest"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <limits>
#include <list>
#include <string>
#include <type_traits>
#include <vector>

#include <VtsHalHidlTargetTestBase.h>

#include <android-base/logging.h>

#include <android/hardware/audio/2.0/IDevice.h>
#include <android/hardware/audio/2.0/IDevicesFactory.h>
#include <android/hardware/audio/2.0/IPrimaryDevice.h>
#include <android/hardware/audio/2.0/types.h>
#include <android/hardware/audio/common/2.0/types.h>

#include "utility/ReturnIn.h"
#include "utility/AssertOk.h"
#include "utility/PrettyPrintAudioTypes.h"

using std::string;
using std::to_string;
using std::vector;

using ::android::sp;
using ::android::hardware::Return;
using ::android::hardware::hidl_handle;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::audio::V2_0::DeviceAddress;
using ::android::hardware::audio::V2_0::IDevice;
using ::android::hardware::audio::V2_0::IPrimaryDevice;
using TtyMode = ::android::hardware::audio::V2_0::IPrimaryDevice::TtyMode;
using ::android::hardware::audio::V2_0::IDevicesFactory;
using ::android::hardware::audio::V2_0::IStream;
using ::android::hardware::audio::V2_0::IStreamIn;
using ::android::hardware::audio::V2_0::IStreamOut;
using ::android::hardware::audio::V2_0::ParameterValue;
using ::android::hardware::audio::V2_0::Result;
using ::android::hardware::audio::common::V2_0::AudioChannelMask;
using ::android::hardware::audio::common::V2_0::AudioConfig;
using ::android::hardware::audio::common::V2_0::AudioDevice;
using ::android::hardware::audio::common::V2_0::AudioFormat;
using ::android::hardware::audio::common::V2_0::AudioHandleConsts;
using ::android::hardware::audio::common::V2_0::AudioInputFlag;
using ::android::hardware::audio::common::V2_0::AudioIoHandle;
using ::android::hardware::audio::common::V2_0::AudioMode;
using ::android::hardware::audio::common::V2_0::AudioOffloadInfo;
using ::android::hardware::audio::common::V2_0::AudioOutputFlag;
using ::android::hardware::audio::common::V2_0::AudioSource;

using utility::returnIn;

namespace doc {
/** Document the current test case.
 * Eg: calling `doc::test("Dump the state of the hal")` in the "debugDump" test will output:
 *   <testcase name="debugDump" status="run" time="6" classname="AudioPrimaryHidlTest"
            description="Dump the state of the hal." />
 * see https://github.com/google/googletest/blob/master/googletest/docs/AdvancedGuide.md#logging-additional-information
 */
void test(const std::string& testCaseDocumentation) {
    ::testing::Test::RecordProperty("description", testCaseDocumentation);
}

/** Document why a test was not fully run. Usually due to an optional feature not implemented. */
void partialTest(const std::string& reason) {
    ::testing::Test::RecordProperty("partialyRunTest", reason);
}
}

// Register callback for static object destruction
// Avoid destroying static objects after main return.
// Post main return destruction leads to incorrect gtest timing measurements as well as harder
// debuging if anything goes wrong during destruction.
class Environment : public ::testing::Environment {
public:
    using TearDownFunc = std::function<void ()>;
     void registerTearDown(TearDownFunc&& tearDown) {
         tearDowns.push_back(std::move(tearDown));
    }

private:
    void TearDown() override {
        // Call the tear downs in reverse order of insertion
        for (auto& tearDown : tearDowns) {
            tearDown();
        }
    }
    std::list<TearDownFunc> tearDowns;
};
// Instance to register global tearDown
static Environment* environment;

class HidlTest : public ::testing::VtsHalHidlTargetTestBase {
protected:
    // Convenient member to store results
    Result res;
};

//////////////////////////////////////////////////////////////////////////////
////////////////////// getService audio_devices_factory //////////////////////
//////////////////////////////////////////////////////////////////////////////

// Test all audio devices
class AudioHidlTest : public HidlTest {
public:
    void SetUp() override {
        ASSERT_NO_FATAL_FAILURE(HidlTest::SetUp()); // setup base

        if (devicesFactory == nullptr) {
            environment->registerTearDown([]{ devicesFactory.clear(); });
            devicesFactory = ::testing::VtsHalHidlTargetTestBase::getService<IDevicesFactory>();
        }
        ASSERT_TRUE(devicesFactory != nullptr);
    }

protected:
    // Cache the devicesFactory retrieval to speed up each test by ~0.5s
    static sp<IDevicesFactory> devicesFactory;
};
sp<IDevicesFactory> AudioHidlTest::devicesFactory;

TEST_F(AudioHidlTest, GetAudioDevicesFactoryService) {
    doc::test("test the getService (called in SetUp)");
}

//////////////////////////////////////////////////////////////////////////////
/////////////////////////////// openDevice primary ///////////////////////////
//////////////////////////////////////////////////////////////////////////////

// Test the primary device
class AudioPrimaryHidlTest : public AudioHidlTest {
public:
    /** Primary HAL test are NOT thread safe. */
    void SetUp() override {
        ASSERT_NO_FATAL_FAILURE(AudioHidlTest::SetUp()); // setup base

        if (device == nullptr) {
            IDevicesFactory::Result result;
            sp<IDevice> baseDevice;
            ASSERT_OK(devicesFactory->openDevice(IDevicesFactory::Device::PRIMARY,
                                                 returnIn(result, baseDevice)));
            ASSERT_TRUE(baseDevice != nullptr);

            environment->registerTearDown([]{ device.clear(); });
            device = IPrimaryDevice::castFrom(baseDevice);
            ASSERT_TRUE(device != nullptr);
        }
    }

protected:
    // Cache the device opening to speed up each test by ~0.5s
    static sp<IPrimaryDevice> device;
};
sp<IPrimaryDevice> AudioPrimaryHidlTest::device;

TEST_F(AudioPrimaryHidlTest, OpenPrimaryDevice) {
    doc::test("Test the openDevice (called in SetUp)");
}

TEST_F(AudioPrimaryHidlTest, Init) {
    doc::test("Test that the audio primary hal initialized correctly");
    ASSERT_OK(device->initCheck());
}

//////////////////////////////////////////////////////////////////////////////
///////////////////// {set,get}{Master,Mic}{Mute,Volume} /////////////////////
//////////////////////////////////////////////////////////////////////////////

template <class Property>
class AccessorPrimaryHidlTest : public AudioPrimaryHidlTest {
protected:

    /** Test a property getter and setter. */
    template <class Getter, class Setter>
    void testAccessors(const string& propertyName, const vector<Property>& valuesToTest,
                       Setter setter, Getter getter,
                       const vector<Property>& invalidValues = {}) {

        Property initialValue; // Save initial value to restore it at the end of the test
        ASSERT_OK((device.get()->*getter)(returnIn(res, initialValue)));
        ASSERT_OK(res);

        for (Property setValue : valuesToTest) {
            SCOPED_TRACE("Test " + propertyName + " getter and setter for " +
                         testing::PrintToString(setValue));
            ASSERT_OK((device.get()->*setter)(setValue));
            Property getValue;
            // Make sure the getter returns the same value just set
            ASSERT_OK((device.get()->*getter)(returnIn(res, getValue)));
            ASSERT_OK(res);
            EXPECT_EQ(setValue, getValue);
        }

        for (Property invalidValue : invalidValues) {
            SCOPED_TRACE("Try to set " + propertyName + " with the invalid value " +
                         testing::PrintToString(invalidValue));
            EXPECT_RESULT(Result::INVALID_ARGUMENTS, (device.get()->*setter)(invalidValue));
        }

        ASSERT_OK((device.get()->*setter)(initialValue)); // restore initial value
    }

    /** Test the getter and setter of an optional feature. */
    template <class Getter, class Setter>
    void testOptionalAccessors(const string& propertyName, const vector<Property>& valuesToTest,
                               Setter setter, Getter getter,
                               const vector<Property>& invalidValues = {}) {
        doc::test("Test the optional " + propertyName + " getters and setter");
        {
            SCOPED_TRACE("Test feature support by calling the getter");
            Property initialValue;
            ASSERT_OK((device.get()->*getter)(returnIn(res, initialValue)));
            if (res == Result::NOT_SUPPORTED) {
                doc::partialTest(propertyName + " getter is not supported");
                return;
            }
            ASSERT_OK(res); // If it is supported it must succeed
        }
        // The feature is supported, test it
        testAccessors(propertyName, valuesToTest, setter, getter, invalidValues);
    }
};

using BoolAccessorPrimaryHidlTest = AccessorPrimaryHidlTest<bool>;

TEST_F(BoolAccessorPrimaryHidlTest, MicMuteTest) {
    doc::test("Check that the mic can be muted and unmuted");
    testAccessors("mic mute", {true, false, true}, &IDevice::setMicMute, &IDevice::getMicMute);
    // TODO: check that the mic is really muted (all sample are 0)
}

TEST_F(BoolAccessorPrimaryHidlTest, MasterMuteTest) {
    doc::test("If master mute is supported, try to mute and unmute the master output");
    testOptionalAccessors("master mute", {true, false, true},
                          &IDevice::setMasterMute, &IDevice::getMasterMute);
    // TODO: check that the master volume is really muted
}

using FloatAccessorPrimaryHidlTest = AccessorPrimaryHidlTest<float>;
TEST_F(FloatAccessorPrimaryHidlTest, MasterVolumeTest) {
    doc::test("Test the master volume if supported");
    testOptionalAccessors("master volume",  {0, 0.5, 1},
                          &IDevice::setMasterVolume, &IDevice::getMasterVolume,
                          {-0.1, 1.1, NAN, INFINITY, -INFINITY,
                           1 + std::numeric_limits<float>::epsilon()});
    // TODO: check that the master volume is really changed
}

//////////////////////////////////////////////////////////////////////////////
//////////////// Required and recommended audio format support ///////////////
// From: https://source.android.com/compatibility/android-cdd.html#5_4_audio_recording
// From: https://source.android.com/compatibility/android-cdd.html#5_5_audio_playback
/////////// TODO: move to the beginning of the file for easier update ////////
//////////////////////////////////////////////////////////////////////////////

class AudioConfigPrimaryTest : public AudioPrimaryHidlTest {
public:
    // Cache result ?
    static const vector<AudioConfig> getRequiredSupportPlaybackAudioConfig() {
        return combineAudioConfig({AudioChannelMask::OUT_STEREO, AudioChannelMask::OUT_MONO},
                                  {8000, 11025, 16000, 22050, 32000, 44100},
                                  {AudioFormat::PCM_16_BIT});
    }

    static const vector<AudioConfig> getRecommendedSupportPlaybackAudioConfig() {
        return combineAudioConfig({AudioChannelMask::OUT_STEREO, AudioChannelMask::OUT_MONO},
                                  {24000, 48000},
                                  {AudioFormat::PCM_16_BIT});
    }

    static const vector<AudioConfig> getSupportedPlaybackAudioConfig() {
        // TODO: retrieve audio config supported by the platform
        // as declared in the policy configuration
        return {};
    }

    static const vector<AudioConfig> getRequiredSupportCaptureAudioConfig() {
        return combineAudioConfig({AudioChannelMask::IN_MONO},
                                  {8000, 11025, 16000, 44100},
                                  {AudioFormat::PCM_16_BIT});
    }
    static const vector<AudioConfig> getRecommendedSupportCaptureAudioConfig() {
        return combineAudioConfig({AudioChannelMask::IN_STEREO},
                                  {22050, 48000},
                                  {AudioFormat::PCM_16_BIT});
    }
    static const vector<AudioConfig> getSupportedCaptureAudioConfig() {
        // TODO: retrieve audio config supported by the platform
        // as declared in the policy configuration
        return {};
    }
private:
    static const vector<AudioConfig> combineAudioConfig(
            vector<AudioChannelMask> channelMasks,
            vector<uint32_t> sampleRates,
            vector<AudioFormat> formats) {
        vector<AudioConfig> configs;
        for (auto channelMask: channelMasks) {
            for (auto sampleRate : sampleRates) {
                for (auto format : formats) {
                    AudioConfig config{};
                    // leave offloadInfo to 0
                    config.channelMask = channelMask;
                    config.sampleRateHz = sampleRate;
                    config.format = format;
                    // FIXME: leave frameCount to 0 ?
                    configs.push_back(config);
                }
            }
        }
        return configs;
    }
};

/** Generate a test name based on an audio config.
 *
 * As the only parameter changing are channel mask and sample rate,
 * only print those ones in the test name.
 */
static string generateTestName(const testing::TestParamInfo<AudioConfig>& info) {
    const AudioConfig& config = info.param;
    return to_string(info.index) + "__" + to_string(config.sampleRateHz)+ "_" +
            // "MONO" is more clear than "FRONT_LEFT"
            ((config.channelMask == AudioChannelMask::OUT_MONO ||
              config.channelMask == AudioChannelMask::IN_MONO) ?
                    "MONO" : toString(config.channelMask));
}

//////////////////////////////////////////////////////////////////////////////
///////////////////////////// getInputBufferSize /////////////////////////////
//////////////////////////////////////////////////////////////////////////////

// FIXME: execute input test only if platform declares android.hardware.microphone
//        how to get this value ? is it a property ???

class AudioCaptureConfigPrimaryTest : public AudioConfigPrimaryTest,
                                      public ::testing::WithParamInterface<AudioConfig> {
protected:
    void inputBufferSizeTest(const AudioConfig& audioConfig, bool supportRequired) {
        uint64_t bufferSize;
        ASSERT_OK(device->getInputBufferSize(audioConfig, returnIn(res, bufferSize)));

        switch (res) {
            case Result::INVALID_ARGUMENTS:
                EXPECT_FALSE(supportRequired);
                break;
            case Result::OK:
                // Check that the buffer is of a sane size
                // For now only that it is > 0
                EXPECT_GT(bufferSize, uint64_t(0));
                break;
            default:
                FAIL() << "Invalid return status: " << ::testing::PrintToString(res);
        }
    }
};

// Test that the required capture config and those declared in the policy are indeed supported
class RequiredInputBufferSizeTest : public AudioCaptureConfigPrimaryTest {};
TEST_P(RequiredInputBufferSizeTest, RequiredInputBufferSizeTest) {
    doc::test("Input buffer size must be retrievable for a format with required support.");
    inputBufferSizeTest(GetParam(), true);
}
INSTANTIATE_TEST_CASE_P(
        RequiredInputBufferSize, RequiredInputBufferSizeTest,
        ::testing::ValuesIn(AudioConfigPrimaryTest::getRequiredSupportCaptureAudioConfig()),
         &generateTestName);
INSTANTIATE_TEST_CASE_P(
        SupportedInputBufferSize, RequiredInputBufferSizeTest,
        ::testing::ValuesIn(AudioConfigPrimaryTest::getSupportedCaptureAudioConfig()),
         &generateTestName);

// Test that the recommended capture config are supported or lead to a INVALID_ARGUMENTS return
class OptionalInputBufferSizeTest : public AudioCaptureConfigPrimaryTest {};
TEST_P(OptionalInputBufferSizeTest, OptionalInputBufferSizeTest) {
    doc::test("Input buffer size should be retrievable for a format with recommended support.");
    inputBufferSizeTest(GetParam(), false);
}
INSTANTIATE_TEST_CASE_P(
        RecommendedCaptureAudioConfigSupport, OptionalInputBufferSizeTest,
        ::testing::ValuesIn(AudioConfigPrimaryTest::getRecommendedSupportCaptureAudioConfig()),
         &generateTestName);

//////////////////////////////////////////////////////////////////////////////
/////////////////////////////// setScreenState ///////////////////////////////
//////////////////////////////////////////////////////////////////////////////

TEST_F(AudioPrimaryHidlTest, setScreenState) {
    doc::test("Check that the hal can receive the screen state");
    for (bool turnedOn : {false, true, true, false, false}) {
        auto ret = device->setScreenState(turnedOn);
        ASSERT_TRUE(ret.isOk());
        Result result = ret;
        ASSERT_TRUE(result == Result::OK || result == Result::NOT_SUPPORTED) << toString(result);
    }
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////// {get,set}Parameters /////////////////////////////
//////////////////////////////////////////////////////////////////////////////

TEST_F(AudioPrimaryHidlTest, getParameters) {
    doc::test("Check that the hal can set and get parameters");
    hidl_vec<hidl_string> keys;
    hidl_vec<ParameterValue> values;
    ASSERT_OK(device->getParameters(keys, returnIn(res, values)));
    ASSERT_OK(device->setParameters(values));
    values.resize(0);
    ASSERT_OK(device->setParameters(values));
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////// debugDebug //////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

TEST_F(AudioPrimaryHidlTest, debugDump) {
    doc::test("Check that the hal can dump its state without error");
    FILE* file = tmpfile();
    ASSERT_NE(nullptr, file) << errno;

    auto* nativeHandle = native_handle_create(1, 0);
    ASSERT_NE(nullptr, nativeHandle);
    nativeHandle->data[0] = fileno(file);

    hidl_handle handle;
    handle.setTo(nativeHandle, true /*take ownership*/);

    // TODO: debugDump does not return a Result.
    // This mean that the hal can not report that it not implementing the function.
    ASSERT_OK(device->debugDump(handle));

    rewind(file); // can not fail

    // Check that at least one bit was written by the hal
    char buff;
    ASSERT_EQ(size_t{1}, fread(&buff, sizeof(buff), 1, file));
    EXPECT_EQ(0, fclose(file)) << errno;
}

//////////////////////////////////////////////////////////////////////////////
////////////////////////// open{Output,Input}Stream //////////////////////////
//////////////////////////////////////////////////////////////////////////////

template <class Stream>
class OpenStreamTest : public AudioConfigPrimaryTest,
                       public ::testing::WithParamInterface<AudioConfig> {
protected:
    template <class Open>
    void testOpen(Open openStream, const AudioConfig& config) {
        // FIXME: Open a stream without an IOHandle
        //        This is not required to be accepted by hal implementations
        AudioIoHandle ioHandle = (AudioIoHandle)AudioHandleConsts::AUDIO_IO_HANDLE_NONE;
        AudioConfig suggestedConfig{};
        ASSERT_OK(openStream(ioHandle, config, returnIn(res, stream, suggestedConfig)));

        // TODO: only allow failure for RecommendedPlaybackAudioConfig
        switch (res) {
            case Result::OK:
                ASSERT_TRUE(stream != nullptr);
                audioConfig = config;
                break;
            case Result::INVALID_ARGUMENTS:
                ASSERT_TRUE(stream == nullptr);
                AudioConfig suggestedConfigRetry;
                // Could not open stream with config, try again with the suggested one
                ASSERT_OK(openStream(ioHandle, suggestedConfig,
                                     returnIn(res, stream, suggestedConfigRetry)));
                // This time it must succeed
                ASSERT_OK(res);
                ASSERT_TRUE(stream == nullptr);
                audioConfig = suggestedConfig;
                break;
            default:
                FAIL() << "Invalid return status: " << ::testing::PrintToString(res);
        }
        open = true;
    }

private:
    void TearDown() override {
        if (open) {
            ASSERT_OK(stream->close());
        }
    }

protected:

    AudioConfig audioConfig;
    sp<Stream> stream;
    bool open = false;
};

////////////////////////////// openOutputStream //////////////////////////////

class OutputStreamTest : public OpenStreamTest<IStreamOut> {
    virtual void SetUp() override {
        ASSERT_NO_FATAL_FAILURE(OpenStreamTest::SetUp()); // setup base

        const AudioConfig& config = GetParam();
        DeviceAddress deviceAddr{};  // Ignored by primary hal
        AudioOutputFlag flags = AudioOutputFlag::NONE; // TODO: test all flag combination
        testOpen([&](AudioIoHandle handle, AudioConfig config, auto cb)
                 { return device->openOutputStream(handle, deviceAddr, config, flags, cb); },
                 config);
    }
};
TEST_P(OutputStreamTest, OpenOutputStreamTest) {
    doc::test("Check that output streams can be open with the required and recommended config");
    // Open done in SetUp
}
INSTANTIATE_TEST_CASE_P(
        RequiredOutputStreamConfigSupport, OutputStreamTest,
        ::testing::ValuesIn(AudioConfigPrimaryTest::getRequiredSupportPlaybackAudioConfig()),
         &generateTestName);
INSTANTIATE_TEST_CASE_P(
        SupportedOutputStreamConfig, OutputStreamTest,
        ::testing::ValuesIn(AudioConfigPrimaryTest::getSupportedPlaybackAudioConfig()),
         &generateTestName);

INSTANTIATE_TEST_CASE_P(
        RecommendedOutputStreamConfigSupport, OutputStreamTest,
        ::testing::ValuesIn(AudioConfigPrimaryTest::getRecommendedSupportPlaybackAudioConfig()),
         &generateTestName);

////////////////////////////// openInputStream //////////////////////////////

class InputStreamTest : public OpenStreamTest<IStreamIn> {

    virtual void SetUp() override {
        ASSERT_NO_FATAL_FAILURE(OpenStreamTest::SetUp()); // setup base

        const AudioConfig& config = GetParam();
        DeviceAddress deviceAddr{}; // TODO: test all devices
        AudioInputFlag flags = AudioInputFlag::NONE; // TODO: test all flag combination
        AudioSource source = AudioSource::DEFAULT; // TODO: test all flag combination
        testOpen([&](AudioIoHandle handle, AudioConfig config, auto cb)
                 { return device->openInputStream(handle, deviceAddr, config, flags, source, cb); },
                 config);
    }
};

TEST_P(InputStreamTest, OpenInputStreamTest) {
    doc::test("Check that input streams can be open with the required and recommended config");
    // Open done in setup
}
INSTANTIATE_TEST_CASE_P(
        RequiredInputStreamConfigSupport, InputStreamTest,
        ::testing::ValuesIn(AudioConfigPrimaryTest::getRequiredSupportCaptureAudioConfig()),
         &generateTestName);
INSTANTIATE_TEST_CASE_P(
        SupportedInputStreamConfig, InputStreamTest,
        ::testing::ValuesIn(AudioConfigPrimaryTest::getSupportedCaptureAudioConfig()),
         &generateTestName);

INSTANTIATE_TEST_CASE_P(
        RecommendedInputStreamConfigSupport, InputStreamTest,
        ::testing::ValuesIn(AudioConfigPrimaryTest::getRecommendedSupportCaptureAudioConfig()),
         &generateTestName);

//////////////////////////////////////////////////////////////////////////////
////////////////////////////// IStream getters ///////////////////////////////
//////////////////////////////////////////////////////////////////////////////

/** Unpack the provided result.
 * If the result is not OK, register a failure and return an undefined value. */
template <class R>
static R extract(Return<R> ret) {
    if (!ret.isOk()) {
        ADD_FAILURE();
        return R{};
    }
    return ret;
}

template <class Property, class CapabilityGetter, class Getter, class Setter>
static void testCapabilityGetter(const string& name,IStream* stream, Property currentValue,
                                 CapabilityGetter capablityGetter, Getter getter, Setter setter) {
    hidl_vec<Property> capabilities;
    ASSERT_OK((stream->*capablityGetter)(returnIn(capabilities)));
    if (capabilities.size() == 0) {
        // The default hal should probably return a NOT_SUPPORTED if the hal does not expose
        // capability retrieval. For now it returns an empty list if not implemented
        doc::partialTest(name + " is not supported");
        return;
    };
    // TODO: This code has never been tested on a hal that supports getSupportedSampleRates
    EXPECT_NE(std::find(capabilities.begin(), capabilities.end(), currentValue),
             capabilities.end())
        << "current " << name << " is not in the list of the supported ones "
        << toString(capabilities);

    // Check that all declared supported values are indeed supported
    for (auto capability : capabilities) {
        ASSERT_OK((stream->*setter)(capability));
        ASSERT_EQ(capability, extract((stream->*getter)()));
    }
}

static void testGetAudioProperties(IStream* stream, AudioConfig expectedConfig) {
    uint32_t sampleRateHz;
    AudioChannelMask mask;
    AudioFormat format;

    stream->getAudioProperties(returnIn(sampleRateHz, mask, format));

    // FIXME: the qcom hal it does not currently negotiate the sampleRate & channel mask
    EXPECT_EQ(expectedConfig.sampleRateHz, sampleRateHz);
    EXPECT_EQ(expectedConfig.channelMask, mask);
    EXPECT_EQ(expectedConfig.format, format);
}

static void testAccessors(IStream* stream, AudioConfig audioConfig) {
    doc::test("Test IStream getters and setters that can be called in the stop state");

    auto frameCount = extract(stream->getFrameCount());
    ASSERT_EQ(audioConfig.frameCount, frameCount);

    auto sampleRate = extract(stream->getSampleRate());
    // FIXME: the qcom hal it does not currently negotiate the sampleRate
    ASSERT_EQ(audioConfig.sampleRateHz, sampleRate);

    auto channelMask = extract(stream->getChannelMask());
    // FIXME: the qcom hal it does not currently negotiate the channelMask
    ASSERT_EQ(audioConfig.channelMask, channelMask);

    auto frameSize = extract(stream->getFrameSize());
    ASSERT_GE(frameSize, 0U);

    auto bufferSize = extract(stream->getBufferSize());
    ASSERT_GE(bufferSize, frameSize);

    testCapabilityGetter("getSupportedsampleRate", stream, sampleRate,
                         &IStream::getSupportedSampleRates,
                         &IStream::getSampleRate, &IStream::setSampleRate);

    testCapabilityGetter("getSupportedChannelMask", stream, channelMask,
                         &IStream::getSupportedChannelMasks,
                         &IStream::getChannelMask, &IStream::setChannelMask);

    AudioFormat format = extract(stream->getFormat());
    ASSERT_EQ(audioConfig.format, format);

    testCapabilityGetter("getSupportedFormats", stream, format,
                         &IStream::getSupportedFormats, &IStream::getFormat, &IStream::setFormat);

    testGetAudioProperties(stream, audioConfig);

    auto ret = stream->getDevice();
    ASSERT_TRUE(ret.isOk());
    AudioDevice device = ret;
    ASSERT_EQ(AudioDevice::OUT_DEFAULT, device);
}

TEST_P(InputStreamTest, GettersTest) {
    testAccessors(stream.get(), audioConfig);
}
TEST_P(OutputStreamTest, GettersTest) {
    testAccessors(stream.get(), audioConfig);
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////// AudioPatches ////////////////////////////////
//////////////////////////////////////////////////////////////////////////////


TEST_F(AudioPrimaryHidlTest, AudioPatches) {
    doc::test("Test if audio patches are supported");
    auto result = device->supportsAudioPatches();
    ASSERT_TRUE(result.isOk());
    bool supportsAudioPatch = result;
    if (!supportsAudioPatch) {
        doc::partialTest("Audio patches are not supported");
        return;
    }
    // TODO: test audio patches
}

//////////////////////////////////////////////////////////////////////////////
/////////////////////////////// PrimaryDevice ////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

TEST_F(AudioPrimaryHidlTest, setVoiceVolume) {
    doc::test("Make sure setVoiceVolume only succeed if volume is in [0,1]");
    for (float volume : {0.0, 0.01, 0.5, 0.09, 1.0}) {
        SCOPED_TRACE("volume=" + to_string(volume));
        ASSERT_OK(device->setVoiceVolume(volume));
    }
    for (float volume : (float[]){-INFINITY,-1.0, -0.0,
                                  1.0 + std::numeric_limits<float>::epsilon(), 2.0, INFINITY,
                                  NAN}) {
        SCOPED_TRACE("volume=" + to_string(volume));
        // FIXME: NAN should never be accepted
        // FIXME: Missing api doc. What should the impl do if the volume is outside [0,1] ?
        ASSERT_RESULT(Result::INVALID_ARGUMENTS, device->setVoiceVolume(volume));
    }
}

TEST_F(AudioPrimaryHidlTest, setMode) {
    doc::test("Make sure setMode always succeeds if mode is valid");
    for (AudioMode mode : {AudioMode::IN_CALL, AudioMode::IN_COMMUNICATION,
                           AudioMode::RINGTONE, AudioMode::CURRENT,
                           AudioMode::NORMAL /* Make sure to leave the test in normal mode */ }) {
        SCOPED_TRACE("mode=" + toString(mode));
        ASSERT_OK(device->setMode(mode));
    }

    // FIXME: Missing api doc. What should the impl do if the mode is invalid ?
    ASSERT_RESULT(Result::INVALID_ARGUMENTS, device->setMode(AudioMode::INVALID));
}


TEST_F(BoolAccessorPrimaryHidlTest, BtScoNrecEnabled) {
    doc::test("Query and set the BT SCO NR&EC state");
    testOptionalAccessors("BtScoNrecEnabled", {true, false, true},
                         &IPrimaryDevice::setBtScoNrecEnabled,
                         &IPrimaryDevice::getBtScoNrecEnabled);
}

TEST_F(BoolAccessorPrimaryHidlTest, setGetBtScoWidebandEnabled) {
    doc::test("Query and set the SCO whideband state");
    testOptionalAccessors("BtScoWideband", {true, false, true},
                         &IPrimaryDevice::setBtScoWidebandEnabled,
                         &IPrimaryDevice::getBtScoWidebandEnabled);
}

using TtyModeAccessorPrimaryHidlTest = AccessorPrimaryHidlTest<TtyMode>;
TEST_F(TtyModeAccessorPrimaryHidlTest, setGetTtyMode) {
    doc::test("Query and set the TTY mode state");
    testOptionalAccessors("TTY mode", {TtyMode::OFF, TtyMode::HCO, TtyMode::VCO, TtyMode::FULL},
                          &IPrimaryDevice::setTtyMode, &IPrimaryDevice::getTtyMode);
}

TEST_F(BoolAccessorPrimaryHidlTest, setGetHac) {
    doc::test("Query and set the HAC state");
    testAccessors("HAC", {true, false, true},
                         &IPrimaryDevice::setHacEnabled,
                         &IPrimaryDevice::getHacEnabled);
}

//////////////////////////////////////////////////////////////////////////////
//////////////////// Clean caches on global tear down ////////////////////////
//////////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv) {
    environment = new Environment;
    ::testing::AddGlobalTestEnvironment(environment);
    ::testing::InitGoogleTest(&argc, argv);
    int status = RUN_ALL_TESTS();
    LOG(INFO) << "Test result = " << status;
    return status;
}
