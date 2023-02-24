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

#define LOG_TAG "VtsHalAudioVTargetTest"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <initializer_list>
#include <limits>
#include <list>
#include <map>
#include <set>
#include <string>
#include <variant>
#include <vector>

#include <fcntl.h>
#include <unistd.h>

#include <hwbinder/IPCThreadState.h>

#include <android-base/expected.h>
#include <android-base/logging.h>
#include <system/audio_config.h>

// clang-format off
#include PATH(android/hardware/audio/FILE_VERSION/IDevice.h)
#include PATH(android/hardware/audio/FILE_VERSION/IDevicesFactory.h)
#include PATH(android/hardware/audio/FILE_VERSION/IPrimaryDevice.h)
#include PATH(android/hardware/audio/CORE_TYPES_FILE_VERSION/types.h)
#include PATH(android/hardware/audio/common/COMMON_TYPES_FILE_VERSION/types.h)
#if MAJOR_VERSION >= 7
#include PATH(APM_XSD_ENUMS_H_FILENAME)
#include PATH(APM_XSD_H_FILENAME)
#endif
// clang-format on

#include <fmq/EventFlag.h>
#include <fmq/MessageQueue.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>

#include <common/all-versions/VersionUtils.h>

#include "utility/AssertOk.h"
#include "utility/Documentation.h"
#include "utility/ReturnIn.h"
#include "utility/ValidateXml.h"

#include "AudioTestDefinitions.h"
/** Provide version specific functions that are used in the generic tests */
#if MAJOR_VERSION == 2
#include "2.0/AudioPrimaryHidlHalUtils.h"
#elif MAJOR_VERSION >= 4
#include "4.0/AudioPrimaryHidlHalUtils.h"
#endif

using ::android::NO_INIT;
using ::android::OK;
using ::android::sp;
using ::android::status_t;
using ::android::hardware::EventFlag;
using ::android::hardware::hidl_bitfield;
using ::android::hardware::hidl_enum_range;
using ::android::hardware::hidl_handle;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::IPCThreadState;
using ::android::hardware::kSynchronizedReadWrite;
using ::android::hardware::MessageQueue;
using ::android::hardware::MQDescriptorSync;
using ::android::hardware::Return;
using ::android::hardware::audio::common::utils::EnumBitfield;
using ::android::hardware::audio::common::utils::mkEnumBitfield;
using ::android::hardware::details::toHexString;

using namespace ::android::hardware::audio::common::COMMON_TYPES_CPP_VERSION;
using namespace ::android::hardware::audio::common::test::utility;
using namespace ::android::hardware::audio::CPP_VERSION;
using ReadParameters =
        ::android::hardware::audio::CORE_TYPES_CPP_VERSION::IStreamIn::ReadParameters;
using ReadStatus = ::android::hardware::audio::CORE_TYPES_CPP_VERSION::IStreamIn::ReadStatus;
using WriteCommand = ::android::hardware::audio::CPP_VERSION::IStreamOut::WriteCommand;
using WriteStatus = ::android::hardware::audio::CPP_VERSION::IStreamOut::WriteStatus;
#if MAJOR_VERSION >= 7
// Make an alias for enumerations generated from the APM config XSD.
namespace xsd {
using namespace ::android::audio::policy::configuration::CPP_VERSION;
}
#endif

// Typical accepted results from interface methods
static auto okOrNotSupported = {Result::OK, Result::NOT_SUPPORTED};
static auto okOrNotSupportedOrInvalidArgs = {Result::OK, Result::NOT_SUPPORTED,
                                             Result::INVALID_ARGUMENTS};
static auto okOrInvalidState = {Result::OK, Result::INVALID_STATE};
static auto okOrInvalidStateOrNotSupported = {Result::OK, Result::INVALID_STATE,
                                              Result::NOT_SUPPORTED};
static auto invalidArgsOrNotSupported = {Result::INVALID_ARGUMENTS, Result::NOT_SUPPORTED};
static auto invalidStateOrNotSupported = {Result::INVALID_STATE, Result::NOT_SUPPORTED};

#include "DeviceManager.h"
#if MAJOR_VERSION <= 6
#include "PolicyConfig.h"
#if MAJOR_VERSION == 6
#include "6.0/Generators.h"
#endif
#elif MAJOR_VERSION >= 7
#include "7.0/Generators.h"
#include "7.0/PolicyConfig.h"
#endif
#include "StreamWorker.h"

class HidlTest : public ::testing::Test {
  public:
    using IDevice = ::android::hardware::audio::CPP_VERSION::IDevice;
    using IDevicesFactory = ::android::hardware::audio::CPP_VERSION::IDevicesFactory;

    static android::base::expected<std::vector<std::string>, std::string> getAllFactoryInstances() {
        using ::android::hardware::audio::CPP_VERSION::IDevicesFactory;
        const std::string factoryDescriptor = IDevicesFactory::descriptor;
        // Make sure that the instance is the exact minor version.
        // Using a 7.1 factory for 7.0 test is not always possible because
        // 7.1 can be configured via the XML config to use features that are
        // absent in 7.0.
        auto instances = ::android::hardware::getAllHalInstanceNames(factoryDescriptor);
        if (instances.empty()) return instances;
        // Use the default instance for checking the implementation version.
        auto defaultInstance = IDevicesFactory::getService("default");
        if (defaultInstance == nullptr) {
            return ::android::base::unexpected("Failed to obtain IDevicesFactory/default");
        }
        std::string actualDescriptor;
        auto intDescRet = defaultInstance->interfaceDescriptor(
                [&](const auto& descriptor) { actualDescriptor = descriptor; });
        if (!intDescRet.isOk()) {
            return ::android::base::unexpected("Failed to obtain interface descriptor: " +
                                               intDescRet.description());
        }
        if (factoryDescriptor == actualDescriptor)
            return instances;
        else
            return {};
    }

    virtual ~HidlTest() = default;
    // public access to avoid annoyances when using this method in template classes
    // derived from test classes
    sp<IDevice> getDevice() const {
        return DeviceManager::getInstance().get(getFactoryName(), getDeviceName());
    }

  protected:
    // Factory and device name getters to be overridden in subclasses.
    virtual const std::string& getFactoryName() const = 0;
    virtual const std::string& getDeviceName() const = 0;

    sp<IDevicesFactory> getDevicesFactory() const {
        return DevicesFactoryManager::getInstance().get(getFactoryName());
    }
    bool resetDevice() const {
        return DeviceManager::getInstance().reset(getFactoryName(), getDeviceName());
    }
    bool areAudioPatchesSupported() { return extract(getDevice()->supportsAudioPatches()); }

    // Convenient member to store results
    Result res;
};

//////////////////////////////////////////////////////////////////////////////
////////////////////////// Audio policy configuration ////////////////////////
//////////////////////////////////////////////////////////////////////////////

// Stringify the argument.
#define QUOTE(x) #x
#define STRINGIFY(x) QUOTE(x)

static constexpr char kConfigFileName[] = "audio_policy_configuration.xml";

// Cached policy config after parsing for faster test startup
const PolicyConfig& getCachedPolicyConfig() {
    static std::unique_ptr<PolicyConfig> policyConfig = [] {
        auto config = std::make_unique<PolicyConfig>(kConfigFileName);
        return config;
    }();
    return *policyConfig;
}

TEST(CheckConfig, audioPolicyConfigurationValidation) {
    const auto factories = HidlTest::getAllFactoryInstances();
    if (!factories.ok()) {
        FAIL() << factories.error();
    }
    if (factories.value().size() == 0) {
        GTEST_SKIP() << "Skipping audioPolicyConfigurationValidation because no factory instances "
                        "are found.";
    }
    RecordProperty("description",
                   "Verify that the audio policy configuration file "
                   "is valid according to the schema");

    const char* xsd = "/data/local/tmp/audio_policy_configuration_" STRINGIFY(CPP_VERSION) ".xsd";
    EXPECT_ONE_VALID_XML_MULTIPLE_LOCATIONS(kConfigFileName,
                                            android::audio_get_configuration_paths(), xsd);
}

//////////////////////////////////////////////////////////////////////////////
//////////////////// Test parameter types and definitions ////////////////////
//////////////////////////////////////////////////////////////////////////////

static inline std::string DeviceParameterToString(
        const ::testing::TestParamInfo<DeviceParameter>& info) {
    const auto& deviceName = std::get<PARAM_DEVICE_NAME>(info.param);
    const auto factoryName =
            ::android::hardware::PrintInstanceNameToString(::testing::TestParamInfo<std::string>{
                    std::get<PARAM_FACTORY_NAME>(info.param), info.index});
    return !deviceName.empty() ? factoryName + "_" + deviceName : factoryName;
}

const std::vector<DeviceParameter>& getDeviceParameters() {
    static std::vector<DeviceParameter> parameters = [] {
        std::vector<DeviceParameter> result;
        const auto factories = HidlTest::getAllFactoryInstances();
        if (!factories.ok()) return result;
        const auto devices = getCachedPolicyConfig().getModulesWithDevicesNames();
        result.reserve(devices.size());
        for (const auto& factoryName : factories.value()) {
            for (const auto& deviceName : devices) {
                if (DeviceManager::getInstance().get(factoryName, deviceName) != nullptr) {
                    result.emplace_back(factoryName, deviceName);
                }
            }
        }
        return result;
    }();
    return parameters;
}

const std::vector<DeviceParameter>& getDeviceParametersForFactoryTests() {
    static std::vector<DeviceParameter> parameters = [] {
        std::vector<DeviceParameter> result;
        const auto factories = HidlTest::getAllFactoryInstances();
        if (!factories.ok()) return result;
        for (const auto& factoryName : factories.value()) {
            result.emplace_back(factoryName,
                                DeviceManager::getInstance().getPrimary(factoryName) != nullptr
                                        ? DeviceManager::kPrimaryDevice
                                        : "");
        }
        return result;
    }();
    return parameters;
}

const std::vector<DeviceParameter>& getDeviceParametersForPrimaryDeviceTests() {
    static std::vector<DeviceParameter> parameters = [] {
        std::vector<DeviceParameter> result;
        const auto primary = std::find_if(
                getDeviceParameters().begin(), getDeviceParameters().end(), [](const auto& elem) {
                    return std::get<PARAM_DEVICE_NAME>(elem) == DeviceManager::kPrimaryDevice;
                });
        if (primary != getDeviceParameters().end()) result.push_back(*primary);
        return result;
    }();
    return parameters;
}

class AudioHidlTestWithDeviceParameter : public HidlTest,
                                         public ::testing::WithParamInterface<DeviceParameter> {
  protected:
    const std::string& getFactoryName() const override {
        return std::get<PARAM_FACTORY_NAME>(GetParam());
    }
    const std::string& getDeviceName() const override {
        return std::get<PARAM_DEVICE_NAME>(GetParam());
    }
};

class AudioPolicyConfigTest : public AudioHidlTestWithDeviceParameter {
  public:
    void SetUp() override {
        ASSERT_NO_FATAL_FAILURE(AudioHidlTestWithDeviceParameter::SetUp());  // setup base
        auto& policyConfig = getCachedPolicyConfig();
        ASSERT_EQ(0, policyConfig.getStatus()) << policyConfig.getError();
    }
};

TEST_P(AudioPolicyConfigTest, LoadAudioPolicyXMLConfiguration) {
    doc::test("Test parsing audio_policy_configuration.xml (called in SetUp)");
}

TEST_P(AudioPolicyConfigTest, HasPrimaryModule) {
    auto& policyConfig = getCachedPolicyConfig();
    ASSERT_TRUE(policyConfig.getPrimaryModule() != nullptr)
            << "Could not find primary module in configuration file: "
            << policyConfig.getFilePath();
}

INSTANTIATE_TEST_CASE_P(AudioHidl, AudioPolicyConfigTest,
                        ::testing::ValuesIn(getDeviceParametersForFactoryTests()),
                        &DeviceParameterToString);
// When the VTS test runs on a device lacking the corresponding HAL version the parameter
// list is empty, this isn't a problem.
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(AudioPolicyConfigTest);

//////////////////////////////////////////////////////////////////////////////
////////////////////// getService audio_devices_factory //////////////////////
//////////////////////////////////////////////////////////////////////////////

// Test audio devices factory
class AudioHidlTest : public AudioHidlTestWithDeviceParameter {
  public:
    using IPrimaryDevice = ::android::hardware::audio::CPP_VERSION::IPrimaryDevice;

    void SetUp() override {
        ASSERT_NO_FATAL_FAILURE(AudioHidlTestWithDeviceParameter::SetUp());  // setup base
        ASSERT_TRUE(getDevicesFactory() != nullptr);
    }
};

TEST_P(AudioHidlTest, GetAudioDevicesFactoryService) {
    doc::test("Test the getService");
}

TEST_P(AudioHidlTest, OpenDeviceInvalidParameter) {
    doc::test("Test passing an invalid parameter to openDevice");
    Result result;
    sp<::android::hardware::audio::CORE_TYPES_CPP_VERSION::IDevice> device;
#if MAJOR_VERSION == 2
    auto invalidDevice = IDevicesFactory::Device(-1);
#elif MAJOR_VERSION >= 4
    auto invalidDevice = "Non existing device";
#endif
    ASSERT_OK(getDevicesFactory()->openDevice(invalidDevice, returnIn(result, device)));
    ASSERT_EQ(Result::INVALID_ARGUMENTS, result);
    ASSERT_TRUE(device == nullptr);
}

INSTANTIATE_TEST_CASE_P(AudioHidl, AudioHidlTest,
                        ::testing::ValuesIn(getDeviceParametersForFactoryTests()),
                        &DeviceParameterToString);
// When the VTS test runs on a device lacking the corresponding HAL version the parameter
// list is empty, this isn't a problem.
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(AudioHidlTest);

//////////////////////////////////////////////////////////////////////////////
/////////////////////////////// openDevice ///////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

// Test all audio devices
class AudioHidlDeviceTest : public AudioHidlTest {
  public:
    void SetUp() override {
        ASSERT_NO_FATAL_FAILURE(AudioHidlTest::SetUp());  // setup base
        ASSERT_TRUE(getDevice() != nullptr);
    }
};

TEST_P(AudioHidlDeviceTest, OpenDevice) {
    doc::test("Test openDevice (called during setup)");
}

TEST_P(AudioHidlDeviceTest, Init) {
    doc::test("Test that the audio hal initialized correctly");
    ASSERT_OK(getDevice()->initCheck());
}

INSTANTIATE_TEST_CASE_P(AudioHidlDevice, AudioHidlDeviceTest,
                        ::testing::ValuesIn(getDeviceParameters()), &DeviceParameterToString);
// When the VTS test runs on a device lacking the corresponding HAL version the parameter
// list is empty, this isn't a problem.
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(AudioHidlDeviceTest);

//////////////////////////////////////////////////////////////////////////////
/////////////////////////////// openDevice primary ///////////////////////////
//////////////////////////////////////////////////////////////////////////////

// Test the primary device
class AudioPrimaryHidlTest : public AudioHidlDeviceTest {
  public:
    void SetUp() override {
        ASSERT_NO_FATAL_FAILURE(AudioHidlDeviceTest::SetUp());  // setup base
        ASSERT_TRUE(getDevice() != nullptr);
    }

    // public access to avoid annoyances when using this method in template classes
    // derived from test classes
    sp<IPrimaryDevice> getDevice() const {
        return DeviceManager::getInstance().getPrimary(getFactoryName());
    }
};

TEST_P(AudioPrimaryHidlTest, OpenPrimaryDevice) {
    doc::test("Test openPrimaryDevice (called during setup)");
}

INSTANTIATE_TEST_CASE_P(AudioPrimaryHidl, AudioPrimaryHidlTest,
                        ::testing::ValuesIn(getDeviceParametersForPrimaryDeviceTests()),
                        &DeviceParameterToString);
// When the VTS test runs on a device lacking the corresponding HAL version the parameter
// list is empty, this isn't a problem.
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(AudioPrimaryHidlTest);

//////////////////////////////////////////////////////////////////////////////
///////////////////// {set,get}{Master,Mic}{Mute,Volume} /////////////////////
//////////////////////////////////////////////////////////////////////////////

template <class Property, class BaseTestClass = AudioHidlDeviceTest>
class AccessorHidlTest : public BaseTestClass {
  protected:
    enum Optionality { REQUIRED, OPTIONAL };
    struct Initial {  // Initial property value
        Initial(Property value, Optionality check = REQUIRED) : value(value), check(check) {}
        Property value;
        Optionality check;  // If this initial value should be checked
    };
    using BaseTestClass::res;
    /** Test a property getter and setter.
     *  The getter and/or the setter may return NOT_SUPPORTED if optionality == OPTIONAL.
     */
    template <Optionality optionality = REQUIRED, class IUTGetter, class Getter, class Setter>
    void testAccessors(IUTGetter iutGetter, const std::string& propertyName,
                       const Initial expectedInitial, std::list<Property> valuesToTest,
                       Setter setter, Getter getter,
                       const std::vector<Property>& invalidValues = {}) {
        const auto expectedResults = {Result::OK,
                                      optionality == OPTIONAL ? Result::NOT_SUPPORTED : Result::OK};

        Property initialValue = expectedInitial.value;
        ASSERT_OK(((this->*iutGetter)().get()->*getter)(returnIn(res, initialValue)));
        ASSERT_RESULT(expectedResults, res);
        if (res == Result::OK && expectedInitial.check == REQUIRED) {
            EXPECT_EQ(expectedInitial.value, initialValue);
        }

        valuesToTest.push_front(expectedInitial.value);
        valuesToTest.push_back(initialValue);
        for (Property setValue : valuesToTest) {
            SCOPED_TRACE("Test " + propertyName + " getter and setter for " +
                         testing::PrintToString(setValue));
            auto ret = ((this->*iutGetter)().get()->*setter)(setValue);
            ASSERT_RESULT(expectedResults, ret);
            if (ret == Result::NOT_SUPPORTED) {
                doc::partialTest(propertyName + " setter is not supported");
                break;
            }
            Property getValue;
            // Make sure the getter returns the same value just set
            ASSERT_OK(((this->*iutGetter)().get()->*getter)(returnIn(res, getValue)));
            ASSERT_RESULT(expectedResults, res);
            if (res == Result::NOT_SUPPORTED) {
                doc::partialTest(propertyName + " getter is not supported");
                continue;
            }
            EXPECT_EQ(setValue, getValue);
        }

        for (Property invalidValue : invalidValues) {
            SCOPED_TRACE("Try to set " + propertyName + " with the invalid value " +
                         testing::PrintToString(invalidValue));
            EXPECT_RESULT(invalidArgsOrNotSupported,
                          ((this->*iutGetter)().get()->*setter)(invalidValue));
        }

        // Restore initial value
        EXPECT_RESULT(expectedResults, ((this->*iutGetter)().get()->*setter)(initialValue));
    }
    template <Optionality optionality = REQUIRED, class Getter, class Setter>
    void testAccessors(const std::string& propertyName, const Initial expectedInitial,
                       std::list<Property> valuesToTest, Setter setter, Getter getter,
                       const std::vector<Property>& invalidValues = {}) {
        testAccessors<optionality>(&BaseTestClass::getDevice, propertyName, expectedInitial,
                                   valuesToTest, setter, getter, invalidValues);
    }
};

using BoolAccessorHidlTest = AccessorHidlTest<bool>;
using BoolAccessorPrimaryHidlTest = AccessorHidlTest<bool, AudioPrimaryHidlTest>;

TEST_P(BoolAccessorHidlTest, MicMuteTest) {
    doc::test("Check that the mic can be muted and unmuted");
    testAccessors<OPTIONAL>("mic mute", Initial{false}, {true}, &IDevice::setMicMute,
                            &IDevice::getMicMute);
    // TODO: check that the mic is really muted (all sample are 0)
}

TEST_P(BoolAccessorHidlTest, MasterMuteTest) {
    doc::test("If master mute is supported, try to mute and unmute the master output");
    testAccessors<OPTIONAL>("master mute", Initial{false}, {true}, &IDevice::setMasterMute,
                            &IDevice::getMasterMute);
    // TODO: check that the master volume is really muted
}

INSTANTIATE_TEST_CASE_P(BoolAccessorHidl, BoolAccessorHidlTest,
                        ::testing::ValuesIn(getDeviceParameters()), &DeviceParameterToString);
INSTANTIATE_TEST_CASE_P(BoolAccessorPrimaryHidl, BoolAccessorPrimaryHidlTest,
                        ::testing::ValuesIn(getDeviceParametersForPrimaryDeviceTests()),
                        &DeviceParameterToString);
// When the VTS test runs on a device lacking the corresponding HAL version the parameter
// list is empty, this isn't a problem.
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(BoolAccessorHidlTest);
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(BoolAccessorPrimaryHidlTest);

using FloatAccessorHidlTest = AccessorHidlTest<float>;
TEST_P(FloatAccessorHidlTest, MasterVolumeTest) {
    doc::test("Test the master volume if supported");
    testAccessors<OPTIONAL>(
        "master volume", Initial{1}, {0, 0.5}, &IDevice::setMasterVolume, &IDevice::getMasterVolume,
        {-0.1, 1.1, NAN, INFINITY, -INFINITY, 1 + std::numeric_limits<float>::epsilon()});
    // TODO: check that the master volume is really changed
}

INSTANTIATE_TEST_CASE_P(FloatAccessorHidl, FloatAccessorHidlTest,
                        ::testing::ValuesIn(getDeviceParameters()), &DeviceParameterToString);
// When the VTS test runs on a device lacking the corresponding HAL version the parameter
// list is empty, this isn't a problem.
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(FloatAccessorHidlTest);

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////// AudioPatches ////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

class AudioPatchHidlTest : public AudioHidlDeviceTest {
  public:
    void SetUp() override {
        ASSERT_NO_FATAL_FAILURE(AudioHidlDeviceTest::SetUp());  // setup base
        if (!areAudioPatchesSupported()) {
            GTEST_SKIP() << "Audio patches are not supported";
        }
    }
};

TEST_P(AudioPatchHidlTest, AudioPatches) {
    doc::test("Test if audio patches are supported");
    // TODO: test audio patches
}

INSTANTIATE_TEST_CASE_P(AudioPatchHidl, AudioPatchHidlTest,
                        ::testing::ValuesIn(getDeviceParameters()), &DeviceParameterToString);
// When the VTS test runs on a device lacking the corresponding HAL version the parameter
// list is empty, this isn't a problem.
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(AudioPatchHidlTest);

#if MAJOR_VERSION >= 4
static std::string SanitizeStringForGTestName(const std::string& s) {
    std::string result = s;
    for (size_t i = 0; i < result.size(); i++) {
        // gtest test names must only contain alphanumeric characters
        if (!std::isalnum(result[i])) result[i] = '_';
    }
    return result;
}
#endif

/** Generate a test name based on an audio config.
 *
 * As the only parameter changing are channel mask and sample rate,
 * only print those ones in the test name.
 */
static std::string DeviceConfigParameterToString(
        const testing::TestParamInfo<DeviceConfigParameter>& info) {
    const AudioConfig& config = std::get<PARAM_CONFIG>(info.param);
    const auto deviceName = DeviceParameterToString(::testing::TestParamInfo<DeviceParameter>{
            std::get<PARAM_DEVICE>(info.param), info.index});
    const auto devicePart =
            (deviceName.empty() ? "" : deviceName + "_") + std::to_string(info.index);
    // The types had changed a lot between versions 2, 4..6 and 7. Use separate
    // code sections for easier understanding.
#if MAJOR_VERSION == 2
    const auto configPart =
            std::to_string(config.sampleRateHz) + "_" +
            // "MONO" is more clear than "FRONT_LEFT"
            (config.channelMask == AudioChannelMask::OUT_MONO ||
                             config.channelMask == AudioChannelMask::IN_MONO
                     ? "MONO"
                     : ::testing::PrintToString(config.channelMask)) +
            "_" +
            std::visit([](auto&& arg) -> std::string { return ::testing::PrintToString(arg); },
                       std::get<PARAM_FLAGS>(info.param));
#elif MAJOR_VERSION >= 4 && MAJOR_VERSION <= 6
    const auto configPart =
            std::to_string(config.sampleRateHz) + "_" +
            // "MONO" is more clear than "FRONT_LEFT"
            (config.channelMask == mkEnumBitfield(AudioChannelMask::OUT_MONO) ||
                             config.channelMask == mkEnumBitfield(AudioChannelMask::IN_MONO)
                     ? "MONO"
                     // In V4 and above the channel mask is a bitfield.
                     // Printing its value using HIDL's toString for a bitfield emits a lot of extra
                     // text due to overlapping constant values. Instead, we print the bitfield
                     // value as if it was a single value + its hex representation
                     : SanitizeStringForGTestName(
                               ::testing::PrintToString(AudioChannelMask(config.channelMask)) +
                               "_" + toHexString(config.channelMask))) +
            "_" +
            SanitizeStringForGTestName(std::visit(
                    [](auto&& arg) -> std::string {
                        using T = std::decay_t<decltype(arg)>;
                        // Need to use FQN of toString to avoid confusing the compiler
                        return ::android::hardware::audio::common::COMMON_TYPES_CPP_VERSION::
                                toString<T>(hidl_bitfield<T>(arg));
                    },
                    std::get<PARAM_FLAGS>(info.param)));
#elif MAJOR_VERSION >= 7
    const auto configPart =
            ::testing::PrintToString(std::get<PARAM_ATTACHED_DEV_ADDR>(info.param).deviceType) +
            "_" + std::to_string(config.base.sampleRateHz) + "_" +
            // The channel masks and flags are vectors of strings, just need to sanitize them.
            SanitizeStringForGTestName(::testing::PrintToString(config.base.channelMask)) + "_" +
            SanitizeStringForGTestName(::testing::PrintToString(std::get<PARAM_FLAGS>(info.param)));
#endif
    return devicePart + "__" + configPart;
}

class AudioHidlTestWithDeviceConfigParameter
    : public HidlTest,
      public ::testing::WithParamInterface<DeviceConfigParameter> {
  protected:
    void SetUp() override {
        ASSERT_NO_FATAL_FAILURE(HidlTest::SetUp());  // setup base
        ASSERT_TRUE(getDevicesFactory() != nullptr);
        ASSERT_TRUE(getDevice() != nullptr);
    }
    const std::string& getFactoryName() const override {
        return std::get<PARAM_FACTORY_NAME>(std::get<PARAM_DEVICE>(GetParam()));
    }
    const std::string& getDeviceName() const override {
        return std::get<PARAM_DEVICE_NAME>(std::get<PARAM_DEVICE>(GetParam()));
    }
    const AudioConfig& getConfig() const { return std::get<PARAM_CONFIG>(GetParam()); }
#if MAJOR_VERSION == 2
    AudioInputFlag getInputFlags() const {
        return std::get<INDEX_INPUT>(std::get<PARAM_FLAGS>(GetParam()));
    }
    AudioOutputFlag getOutputFlags() const {
        return std::get<INDEX_OUTPUT>(std::get<PARAM_FLAGS>(GetParam()));
    }
#elif MAJOR_VERSION >= 4 && MAJOR_VERSION <= 6
    hidl_bitfield<AudioInputFlag> getInputFlags() const {
        return hidl_bitfield<AudioInputFlag>(
                std::get<INDEX_INPUT>(std::get<PARAM_FLAGS>(GetParam())));
    }
    hidl_bitfield<AudioOutputFlag> getOutputFlags() const {
        return hidl_bitfield<AudioOutputFlag>(
                std::get<INDEX_OUTPUT>(std::get<PARAM_FLAGS>(GetParam())));
    }
#elif MAJOR_VERSION >= 7
    DeviceAddress getAttachedDeviceAddress() const {
        return std::get<PARAM_ATTACHED_DEV_ADDR>(GetParam());
    }
    hidl_vec<AudioInOutFlag> getInputFlags() const { return std::get<PARAM_FLAGS>(GetParam()); }
    hidl_vec<AudioInOutFlag> getOutputFlags() const { return std::get<PARAM_FLAGS>(GetParam()); }
#endif
};

#if MAJOR_VERSION <= 6
#define AUDIO_PRIMARY_HIDL_HAL_TEST
#include "ConfigHelper.h"
#undef AUDIO_PRIMARY_HIDL_HAL_TEST
#endif

//////////////////////////////////////////////////////////////////////////////
///////////////////////////// getInputBufferSize /////////////////////////////
//////////////////////////////////////////////////////////////////////////////

// FIXME: execute input test only if platform declares
// android.hardware.microphone
//        how to get this value ? is it a property ???

class AudioCaptureConfigTest : public AudioHidlTestWithDeviceConfigParameter {
  protected:
    void inputBufferSizeTest(const AudioConfig& audioConfig, bool supportRequired) {
        uint64_t bufferSize;
        ASSERT_OK(getDevice()->getInputBufferSize(audioConfig, returnIn(res, bufferSize)));

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

// Test that the required capture config and those declared in the policy are
// indeed supported
class RequiredInputBufferSizeTest : public AudioCaptureConfigTest {};
TEST_P(RequiredInputBufferSizeTest, RequiredInputBufferSizeTest) {
    doc::test(
        "Input buffer size must be retrievable for a format with required "
        "support.");
    inputBufferSizeTest(getConfig(), true);
}

// Test that the recommended capture config are supported or lead to a
// INVALID_ARGUMENTS return
class OptionalInputBufferSizeTest : public AudioCaptureConfigTest {};
TEST_P(OptionalInputBufferSizeTest, OptionalInputBufferSizeTest) {
    doc::test(
            "Input buffer size should be retrievable for a format with recommended "
            "support.");
    inputBufferSizeTest(getConfig(), false);
}

#if MAJOR_VERSION <= 5
// For V2..5 test the primary device according to CDD requirements.
INSTANTIATE_TEST_CASE_P(
        RequiredInputBufferSize, RequiredInputBufferSizeTest,
        ::testing::Combine(
                ::testing::ValuesIn(getDeviceParametersForPrimaryDeviceTests()),
                ::testing::ValuesIn(ConfigHelper::getRequiredSupportCaptureAudioConfig()),
                ::testing::Values(AudioInputFlag::NONE)),
        &DeviceConfigParameterToString);
INSTANTIATE_TEST_CASE_P(
        RecommendedCaptureAudioConfigSupport, OptionalInputBufferSizeTest,
        ::testing::Combine(
                ::testing::ValuesIn(getDeviceParametersForPrimaryDeviceTests()),
                ::testing::ValuesIn(ConfigHelper::getRecommendedSupportCaptureAudioConfig()),
                ::testing::Values(AudioInputFlag::NONE)),
        &DeviceConfigParameterToString);
#elif MAJOR_VERSION >= 6
INSTANTIATE_TEST_CASE_P(SupportedInputBufferSize, RequiredInputBufferSizeTest,
                        ::testing::ValuesIn(getInputDeviceConfigParameters()),
                        &DeviceConfigParameterToString);
#endif
// When the VTS test runs on a device lacking the corresponding HAL version the parameter
// list is empty, this isn't a problem.
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(OptionalInputBufferSizeTest);
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(RequiredInputBufferSizeTest);

//////////////////////////////////////////////////////////////////////////////
/////////////////////////////// setScreenState ///////////////////////////////
//////////////////////////////////////////////////////////////////////////////

TEST_P(AudioHidlDeviceTest, setScreenState) {
    doc::test("Check that the hal can receive the screen state");
    for (bool turnedOn : {false, true, true, false, false}) {
        ASSERT_RESULT(okOrNotSupported, getDevice()->setScreenState(turnedOn));
    }
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////// {get,set}Parameters /////////////////////////////
//////////////////////////////////////////////////////////////////////////////

TEST_P(AudioHidlDeviceTest, getParameters) {
    doc::test("Check that the hal can set and get parameters");
    hidl_vec<ParameterValue> context;
    hidl_vec<hidl_string> keys;
    hidl_vec<ParameterValue> values;
    ASSERT_OK(Parameters::get(getDevice(), keys, returnIn(res, values)));
    ASSERT_RESULT(okOrNotSupported, res);
    ASSERT_RESULT(okOrNotSupported, Parameters::set(getDevice(), values));
    values.resize(0);
    ASSERT_RESULT(okOrNotSupported, Parameters::set(getDevice(), values));
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////// debugDebug //////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

template <class DebugDump>
static void testDebugDump(DebugDump debugDump) {
    // File descriptors to our pipe. fds[0] corresponds to the read end and
    // fds[1] to the write end.
    int fds[2];
    ASSERT_EQ(0, pipe2(fds, O_NONBLOCK)) << errno;

    // Make sure that the pipe is at least 1 MB in size. The test process runs
    // in su domain, so it should be safe to make this call.
    fcntl(fds[0], F_SETPIPE_SZ, 1 << 20);

    // Wrap the temporary file file descriptor in a native handle
    auto* nativeHandle = native_handle_create(1, 0);
    ASSERT_NE(nullptr, nativeHandle);
    nativeHandle->data[0] = fds[1];

    // Wrap this native handle in a hidl handle
    hidl_handle handle;
    handle.setTo(nativeHandle, false /*take ownership*/);

    ASSERT_OK(debugDump(handle));

    // Check that at least one bit was written by the hal
    // TODO: debugDump does not return a Result.
    // This mean that the hal can not report that it not implementing the
    // function.
    char buff;
    if (read(fds[0], &buff, 1) != 1) {
        doc::note("debugDump does not seem implemented");
    }
    EXPECT_EQ(0, close(fds[0])) << errno;
    EXPECT_EQ(0, close(fds[1])) << errno;
}

TEST_P(AudioHidlDeviceTest, DebugDump) {
    doc::test("Check that the hal can dump its state without error");
    testDebugDump([this](const auto& handle) { return dump(getDevice(), handle); });
}

TEST_P(AudioHidlDeviceTest, DebugDumpInvalidArguments) {
    doc::test("Check that the hal dump doesn't crash on invalid arguments");
    ASSERT_OK(dump(getDevice(), hidl_handle()));
}

//////////////////////////////////////////////////////////////////////////////
////////////////////////// open{Output,Input}Stream //////////////////////////
//////////////////////////////////////////////////////////////////////////////

static inline AudioIoHandle getNextIoHandle() {
    static AudioIoHandle lastHandle{};
    return ++lastHandle;
}

// This class is also used by some device tests.
template <class Stream>
class StreamHelper {
  public:
    // StreamHelper doesn't own the stream, this is for simpler stream lifetime management.
    explicit StreamHelper(sp<Stream>& stream) : mStream(stream) {}
    template <class Open>
    void open(Open openStream, const AudioConfig& config, Result* res,
              AudioConfig* suggestedConfigPtr) {
        AudioConfig suggestedConfig{};
        bool retryWithSuggestedConfig = true;
        if (suggestedConfigPtr == nullptr) {
            suggestedConfigPtr = &suggestedConfig;
            retryWithSuggestedConfig = false;
        }
        ASSERT_OK(openStream(mIoHandle, config, returnIn(*res, mStream, *suggestedConfigPtr)));
        switch (*res) {
            case Result::OK:
                ASSERT_TRUE(mStream != nullptr);
                *suggestedConfigPtr = config;
                break;
            case Result::INVALID_ARGUMENTS:
                ASSERT_TRUE(mStream == nullptr);
                if (retryWithSuggestedConfig) {
                    AudioConfig suggestedConfigRetry;
                    ASSERT_OK(openStream(mIoHandle, *suggestedConfigPtr,
                                         returnIn(*res, mStream, suggestedConfigRetry)));
                    ASSERT_OK(*res);
                    ASSERT_TRUE(mStream != nullptr);
                }
                break;
            default:
                FAIL() << "Invalid return status: " << ::testing::PrintToString(*res);
        }
    }
    void close(bool clear, Result* res) {
        auto ret = mStream->close();
        EXPECT_TRUE(ret.isOk());
        *res = ret;
        if (clear) {
            mStream.clear();
#if MAJOR_VERSION <= 5
            // FIXME: there is no way to know when the remote IStream is being destroyed
            //        Binder does not support testing if an object is alive, thus
            //        wait for 100ms to let the binder destruction propagates and
            //        the remote device has the time to be destroyed.
            //        flushCommand makes sure all local command are sent, thus should reduce
            //        the latency between local and remote destruction.
            IPCThreadState::self()->flushCommands();
            usleep(100 * 1000);
#endif
        }
    }
    AudioIoHandle getIoHandle() const { return mIoHandle; }

  private:
    const AudioIoHandle mIoHandle = getNextIoHandle();
    sp<Stream>& mStream;
};

template <class Stream>
class OpenStreamTest : public AudioHidlTestWithDeviceConfigParameter {
  public:
    // public access to avoid annoyances when using this method in template classes
    // derived from test classes
    sp<Stream> getStream() const { return stream; }

  protected:
    OpenStreamTest() : AudioHidlTestWithDeviceConfigParameter(), helper(stream) {}
    template <class Open>
    void testOpen(Open openStream, const AudioConfig& config) {
        // TODO: only allow failure for RecommendedPlaybackAudioConfig
        ASSERT_NO_FATAL_FAILURE(helper.open(openStream, config, &res, &audioConfig));
        open = true;
    }

    Result closeStream(bool clear = true) {
        open = false;
        helper.close(clear, &res);
        return res;
    }

    void TearDown() override {
        if (open) {
            ASSERT_OK(closeStream());
        }
        AudioHidlTestWithDeviceConfigParameter::TearDown();
    }

  protected:
    AudioConfig audioConfig;
    DeviceAddress address = {};
    sp<Stream> stream;
    StreamHelper<Stream> helper;
    bool open = false;
};

////////////////////////////// openOutputStream //////////////////////////////

class StreamWriter : public StreamWorker<StreamWriter> {
  public:
    using IStreamOut = ::android::hardware::audio::CPP_VERSION::IStreamOut;

    StreamWriter(IStreamOut* stream, size_t bufferSize)
        : mStream(stream), mBufferSize(bufferSize), mData(mBufferSize) {}
    StreamWriter(IStreamOut* stream, size_t bufferSize, std::vector<uint8_t>&& data,
                 std::function<void()> onDataStart, std::function<bool()> onDataWrap)
        : mStream(stream),
          mBufferSize(bufferSize),
          mData(std::move(data)),
          mOnDataStart(onDataStart),
          mOnDataWrap(onDataWrap) {
        ALOGI("StreamWriter data size: %d", (int)mData.size());
    }
    ~StreamWriter() {
        stop();
        if (mEfGroup) {
            EventFlag::deleteEventFlag(&mEfGroup);
        }
    }

    typedef MessageQueue<WriteCommand, ::android::hardware::kSynchronizedReadWrite> CommandMQ;
    typedef MessageQueue<uint8_t, ::android::hardware::kSynchronizedReadWrite> DataMQ;
    typedef MessageQueue<WriteStatus, ::android::hardware::kSynchronizedReadWrite> StatusMQ;

    bool workerInit() {
        std::unique_ptr<CommandMQ> tempCommandMQ;
        std::unique_ptr<DataMQ> tempDataMQ;
        std::unique_ptr<StatusMQ> tempStatusMQ;
        Result retval;
        Return<void> ret = mStream->prepareForWriting(
                1, mBufferSize,
                [&](Result r, const CommandMQ::Descriptor& commandMQ,
                    const DataMQ::Descriptor& dataMQ, const StatusMQ::Descriptor& statusMQ,
                    const auto& /*halThreadInfo*/) {
                    retval = r;
                    if (retval == Result::OK) {
                        tempCommandMQ.reset(new CommandMQ(commandMQ));
                        tempDataMQ.reset(new DataMQ(dataMQ));
                        tempStatusMQ.reset(new StatusMQ(statusMQ));
                        if (tempDataMQ->isValid() && tempDataMQ->getEventFlagWord()) {
                            EventFlag::createEventFlag(tempDataMQ->getEventFlagWord(), &mEfGroup);
                        }
                    }
                });
        if (!ret.isOk()) {
            ALOGE("Transport error while calling prepareForWriting: %s", ret.description().c_str());
            return false;
        }
        if (retval != Result::OK) {
            ALOGE("Error from prepareForWriting: %d", retval);
            return false;
        }
        if (!tempCommandMQ || !tempCommandMQ->isValid() || !tempDataMQ || !tempDataMQ->isValid() ||
            !tempStatusMQ || !tempStatusMQ->isValid() || !mEfGroup) {
            ALOGE_IF(!tempCommandMQ, "Failed to obtain command message queue for writing");
            ALOGE_IF(tempCommandMQ && !tempCommandMQ->isValid(),
                     "Command message queue for writing is invalid");
            ALOGE_IF(!tempDataMQ, "Failed to obtain data message queue for writing");
            ALOGE_IF(tempDataMQ && !tempDataMQ->isValid(),
                     "Data message queue for writing is invalid");
            ALOGE_IF(!tempStatusMQ, "Failed to obtain status message queue for writing");
            ALOGE_IF(tempStatusMQ && !tempStatusMQ->isValid(),
                     "Status message queue for writing is invalid");
            ALOGE_IF(!mEfGroup, "Event flag creation for writing failed");
            return false;
        }
        mCommandMQ = std::move(tempCommandMQ);
        mDataMQ = std::move(tempDataMQ);
        mStatusMQ = std::move(tempStatusMQ);
        return true;
    }

    bool workerCycle() {
        WriteCommand cmd = WriteCommand::WRITE;
        if (!mCommandMQ->write(&cmd)) {
            ALOGE("command message queue write failed");
            return false;
        }
        if (mDataPosition == 0) mOnDataStart();
        const size_t dataSize = std::min(mData.size() - mDataPosition, mDataMQ->availableToWrite());
        bool success = mDataMQ->write(mData.data() + mDataPosition, dataSize);
        bool wrapped = false;
        ALOGE_IF(!success, "data message queue write failed");
        mEfGroup->wake(static_cast<uint32_t>(MessageQueueFlagBits::NOT_EMPTY));

        uint32_t efState = 0;
    retry:
        status_t ret =
                mEfGroup->wait(static_cast<uint32_t>(MessageQueueFlagBits::NOT_FULL), &efState);
        if (efState & static_cast<uint32_t>(MessageQueueFlagBits::NOT_FULL)) {
            WriteStatus writeStatus;
            writeStatus.retval = Result::NOT_INITIALIZED;
            if (!mStatusMQ->read(&writeStatus)) {
                ALOGE("status message read failed");
                success = false;
            }
            if (writeStatus.retval != Result::OK) {
                ALOGE("bad write status: %d", writeStatus.retval);
                success = false;
            }
            mDataPosition += writeStatus.reply.written;
            if (mDataPosition >= mData.size()) {
                mDataPosition = 0;
                wrapped = true;
            }
        }
        if (ret == -EAGAIN || ret == -EINTR) {
            // Spurious wakeup. This normally retries no more than once.
            goto retry;
        } else if (ret) {
            ALOGE("bad wait status: %d", ret);
            success = false;
        }
        if (wrapped) {
            success = mOnDataWrap();
        }
        return success;
    }

  private:
    IStreamOut* const mStream;
    const size_t mBufferSize;
    std::vector<uint8_t> mData;
    std::function<void()> mOnDataStart = []() {};
    std::function<bool()> mOnDataWrap = []() { return true; };
    size_t mDataPosition = 0;
    std::unique_ptr<CommandMQ> mCommandMQ;
    std::unique_ptr<DataMQ> mDataMQ;
    std::unique_ptr<StatusMQ> mStatusMQ;
    EventFlag* mEfGroup = nullptr;
};

class OutputStreamTest
    : public OpenStreamTest<::android::hardware::audio::CPP_VERSION::IStreamOut> {
  protected:
    void SetUp() override {
        ASSERT_NO_FATAL_FAILURE(OpenStreamTest::SetUp());  // setup base
#if MAJOR_VERSION <= 6
        address.device = AudioDevice::OUT_DEFAULT;
#elif MAJOR_VERSION >= 7
        address = getAttachedDeviceAddress();
#endif
        const AudioConfig& config = getConfig();
        auto flags = getOutputFlags();
        testOpen(
                [&](AudioIoHandle handle, AudioConfig config, auto cb) {
#if MAJOR_VERSION == 2
                    return getDevice()->openOutputStream(handle, address, config, flags, cb);
#elif MAJOR_VERSION >= 4 && (MAJOR_VERSION < 7 || (MAJOR_VERSION == 7 && MINOR_VERSION == 0))
                    return getDevice()->openOutputStream(handle, address, config, flags,
                                                         initMetadata, cb);
#elif MAJOR_VERSION == 7 && MINOR_VERSION == 1
                    return getDevice()->openOutputStream_7_1(handle, address, config, flags,
                                                             initMetadata, cb);
#endif
                },
                config);
    }
#if MAJOR_VERSION >= 4 && MAJOR_VERSION <= 6

    const SourceMetadata initMetadata = {
        { { AudioUsage::MEDIA,
            AudioContentType::MUSIC,
            1 /* gain */ } }};
#elif MAJOR_VERSION >= 7
  protected:
    const SourceMetadata initMetadata = {
            { { toString(xsd::AudioUsage::AUDIO_USAGE_MEDIA),
                toString(xsd::AudioContentType::AUDIO_CONTENT_TYPE_MUSIC),
                1 /* gain */,
                toString(xsd::AudioChannelMask::AUDIO_CHANNEL_OUT_STEREO),
                {} } }};
#endif
};
TEST_P(OutputStreamTest, OpenOutputStreamTest) {
    doc::test(
        "Check that output streams can be open with the required and "
        "recommended config");
    // Open done in SetUp
}

#if MAJOR_VERSION <= 5
// For V2..5 test the primary device according to CDD requirements.
INSTANTIATE_TEST_CASE_P(
        RequiredOutputStreamConfigSupport, OutputStreamTest,
        ::testing::Combine(
                ::testing::ValuesIn(getDeviceParametersForPrimaryDeviceTests()),
                ::testing::ValuesIn(ConfigHelper::getRequiredSupportPlaybackAudioConfig()),
                ::testing::Values(AudioOutputFlag::NONE)),
        &DeviceConfigParameterToString);
INSTANTIATE_TEST_CASE_P(
        RecommendedOutputStreamConfigSupport, OutputStreamTest,
        ::testing::Combine(
                ::testing::ValuesIn(getDeviceParametersForPrimaryDeviceTests()),
                ::testing::ValuesIn(ConfigHelper::getRecommendedSupportPlaybackAudioConfig()),
                ::testing::Values(AudioOutputFlag::NONE)),
        &DeviceConfigParameterToString);
#elif MAJOR_VERSION >= 6
// For V6 and above test according to the audio policy manager configuration.
// This is more correct as CDD is written from the apps perspective.
// Audio system provides necessary format conversions for missing configurations.
INSTANTIATE_TEST_CASE_P(DeclaredOutputStreamConfigSupport, OutputStreamTest,
                        ::testing::ValuesIn(getOutputDeviceConfigParameters()),
                        &DeviceConfigParameterToString);
#endif
// When the VTS test runs on a device lacking the corresponding HAL version the parameter
// list is empty, this isn't a problem.
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(OutputStreamTest);

////////////////////////////// openInputStream //////////////////////////////

class StreamReader : public StreamWorker<StreamReader> {
  public:
    using IStreamIn = ::android::hardware::audio::CORE_TYPES_CPP_VERSION::IStreamIn;

    StreamReader(IStreamIn* stream, size_t bufferSize)
        : mStream(stream), mBufferSize(bufferSize), mData(mBufferSize) {}
    ~StreamReader() {
        stop();
        if (mEfGroup) {
            EventFlag::deleteEventFlag(&mEfGroup);
        }
    }

    typedef MessageQueue<ReadParameters, ::android::hardware::kSynchronizedReadWrite> CommandMQ;
    typedef MessageQueue<uint8_t, ::android::hardware::kSynchronizedReadWrite> DataMQ;
    typedef MessageQueue<ReadStatus, ::android::hardware::kSynchronizedReadWrite> StatusMQ;

    bool workerInit() {
        std::unique_ptr<CommandMQ> tempCommandMQ;
        std::unique_ptr<DataMQ> tempDataMQ;
        std::unique_ptr<StatusMQ> tempStatusMQ;
        Result retval;
        Return<void> ret = mStream->prepareForReading(
                1, mBufferSize,
                [&](Result r, const CommandMQ::Descriptor& commandMQ,
                    const DataMQ::Descriptor& dataMQ, const StatusMQ::Descriptor& statusMQ,
                    const auto& /*halThreadInfo*/) {
                    retval = r;
                    if (retval == Result::OK) {
                        tempCommandMQ.reset(new CommandMQ(commandMQ));
                        tempDataMQ.reset(new DataMQ(dataMQ));
                        tempStatusMQ.reset(new StatusMQ(statusMQ));
                        if (tempDataMQ->isValid() && tempDataMQ->getEventFlagWord()) {
                            EventFlag::createEventFlag(tempDataMQ->getEventFlagWord(), &mEfGroup);
                        }
                    }
                });
        if (!ret.isOk()) {
            ALOGE("Transport error while calling prepareForReading: %s", ret.description().c_str());
            return false;
        }
        if (retval != Result::OK) {
            ALOGE("Error from prepareForReading: %d", retval);
            return false;
        }
        if (!tempCommandMQ || !tempCommandMQ->isValid() || !tempDataMQ || !tempDataMQ->isValid() ||
            !tempStatusMQ || !tempStatusMQ->isValid() || !mEfGroup) {
            ALOGE_IF(!tempCommandMQ, "Failed to obtain command message queue for reading");
            ALOGE_IF(tempCommandMQ && !tempCommandMQ->isValid(),
                     "Command message queue for reading is invalid");
            ALOGE_IF(!tempDataMQ, "Failed to obtain data message queue for reading");
            ALOGE_IF(tempDataMQ && !tempDataMQ->isValid(),
                     "Data message queue for reading is invalid");
            ALOGE_IF(!tempStatusMQ, "Failed to obtain status message queue for reading");
            ALOGE_IF(tempStatusMQ && !tempStatusMQ->isValid(),
                     "Status message queue for reading is invalid");
            ALOGE_IF(!mEfGroup, "Event flag creation for reading failed");
            return false;
        }
        mCommandMQ = std::move(tempCommandMQ);
        mDataMQ = std::move(tempDataMQ);
        mStatusMQ = std::move(tempStatusMQ);
        return true;
    }

    bool workerCycle() {
        ReadParameters params;
        params.command = IStreamIn::ReadCommand::READ;
        params.params.read = mBufferSize;
        if (!mCommandMQ->write(&params)) {
            ALOGE("command message queue write failed");
            return false;
        }
        mEfGroup->wake(static_cast<uint32_t>(MessageQueueFlagBits::NOT_FULL));

        uint32_t efState = 0;
        bool success = true;
    retry:
        status_t ret =
                mEfGroup->wait(static_cast<uint32_t>(MessageQueueFlagBits::NOT_EMPTY), &efState);
        if (efState & static_cast<uint32_t>(MessageQueueFlagBits::NOT_EMPTY)) {
            ReadStatus readStatus;
            readStatus.retval = Result::NOT_INITIALIZED;
            if (!mStatusMQ->read(&readStatus)) {
                ALOGE("status message read failed");
                success = false;
            }
            if (readStatus.retval != Result::OK) {
                ALOGE("bad read status: %d", readStatus.retval);
                success = false;
            }
            const size_t dataSize = std::min(mData.size(), mDataMQ->availableToRead());
            if (!mDataMQ->read(mData.data(), dataSize)) {
                ALOGE("data message queue read failed");
                success = false;
            }
        }
        if (ret == -EAGAIN || ret == -EINTR) {
            // Spurious wakeup. This normally retries no more than once.
            goto retry;
        } else if (ret) {
            ALOGE("bad wait status: %d", ret);
            success = false;
        }
        return success;
    }

  private:
    IStreamIn* const mStream;
    const size_t mBufferSize;
    std::vector<uint8_t> mData;
    std::unique_ptr<CommandMQ> mCommandMQ;
    std::unique_ptr<DataMQ> mDataMQ;
    std::unique_ptr<StatusMQ> mStatusMQ;
    EventFlag* mEfGroup = nullptr;
};

class InputStreamTest
    : public OpenStreamTest<::android::hardware::audio::CORE_TYPES_CPP_VERSION::IStreamIn> {
    void SetUp() override {
        ASSERT_NO_FATAL_FAILURE(OpenStreamTest::SetUp());  // setup base
        auto flags = getInputFlags();
#if MAJOR_VERSION <= 6
        address.device = AudioDevice::IN_DEFAULT;
#elif MAJOR_VERSION >= 7
        address = getAttachedDeviceAddress();
        auto& metadata = initMetadata.tracks[0];
        if (!xsd::isTelephonyDevice(address.deviceType)) {
            metadata.source = toString(xsd::AudioSource::AUDIO_SOURCE_UNPROCESSED);
            metadata.channelMask = getConfig().base.channelMask;
        } else {
            address.deviceType = toString(xsd::AudioDevice::AUDIO_DEVICE_IN_DEFAULT);
        }
#if MAJOR_VERSION == 7 && MINOR_VERSION >= 1
        auto flagsIt = std::find(flags.begin(), flags.end(),
                                 toString(xsd::AudioInOutFlag::AUDIO_INPUT_FLAG_ULTRASOUND));
        if (flagsIt != flags.end()) {
            metadata.source = toString(xsd::AudioSource::AUDIO_SOURCE_ULTRASOUND);
        }
#endif  // 7.1
#endif  // MAJOR_VERSION >= 7
        const AudioConfig& config = getConfig();
        testOpen(
                [&](AudioIoHandle handle, AudioConfig config, auto cb) {
                    return getDevice()->openInputStream(handle, address, config, flags,
                                                        initMetadata, cb);
                },
                config);
    }

   protected:
#if MAJOR_VERSION == 2
     const AudioSource initMetadata = AudioSource::DEFAULT;
#elif MAJOR_VERSION >= 4 && MAJOR_VERSION <= 6
     const SinkMetadata initMetadata = {{ {.source = AudioSource::DEFAULT, .gain = 1 } }};
#elif MAJOR_VERSION >= 7
     const std::string& getMixPortName() const { return std::get<PARAM_PORT_NAME>(GetParam()); }
     SinkMetadata initMetadata = {
             {{.source = toString(xsd::AudioSource::AUDIO_SOURCE_DEFAULT),
               .gain = 1,
               .tags = {},
               .channelMask = toString(xsd::AudioChannelMask::AUDIO_CHANNEL_IN_MONO)}}};
#endif
};

TEST_P(InputStreamTest, OpenInputStreamTest) {
    doc::test(
        "Check that input streams can be open with the required and "
        "recommended config");
    // Open done in setup
}
#if MAJOR_VERSION <= 5
// For V2..5 test the primary device according to CDD requirements.
INSTANTIATE_TEST_CASE_P(
        RequiredInputStreamConfigSupport, InputStreamTest,
        ::testing::Combine(
                ::testing::ValuesIn(getDeviceParametersForPrimaryDeviceTests()),
                ::testing::ValuesIn(ConfigHelper::getRequiredSupportCaptureAudioConfig()),
                ::testing::Values(AudioInputFlag::NONE)),
        &DeviceConfigParameterToString);
INSTANTIATE_TEST_CASE_P(
        RecommendedInputStreamConfigSupport, InputStreamTest,
        ::testing::Combine(
                ::testing::ValuesIn(getDeviceParametersForPrimaryDeviceTests()),
                ::testing::ValuesIn(ConfigHelper::getRecommendedSupportCaptureAudioConfig()),
                ::testing::Values(AudioInputFlag::NONE)),
        &DeviceConfigParameterToString);
#elif MAJOR_VERSION >= 6
// For V6 and above test according to the audio policy manager configuration.
// This is more correct as CDD is written from the apps perspective.
// Audio system provides necessary format conversions for missing configurations.
INSTANTIATE_TEST_CASE_P(DeclaredInputStreamConfigSupport, InputStreamTest,
                        ::testing::ValuesIn(getInputDeviceConfigParameters()),
                        &DeviceConfigParameterToString);
#endif
// When the VTS test runs on a device lacking the corresponding HAL version the parameter
// list is empty, this isn't a problem.
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(InputStreamTest);

//////////////////////////////////////////////////////////////////////////////
////////////////////////////// IStream getters ///////////////////////////////
//////////////////////////////////////////////////////////////////////////////

/* Could not find a way to write a test for two parametrized class fixure
 * thus use this macro do duplicate tests for Input and Output stream */
#define TEST_IO_STREAM(test_name, documentation, code) \
    TEST_P(InputStreamTest, test_name) {               \
        doc::test(documentation);                      \
        code;                                          \
    }                                                  \
    TEST_P(OutputStreamTest, test_name) {              \
        doc::test(documentation);                      \
        code;                                          \
    }

TEST_IO_STREAM(GetFrameCount, "Check that getting stream frame count does not crash the HAL.",
               ASSERT_TRUE(stream->getFrameCount().isOk()))

#if MAJOR_VERSION <= 6
TEST_IO_STREAM(GetSampleRate, "Check that the stream sample rate == the one it was opened with",
               ASSERT_EQ(audioConfig.sampleRateHz, extract(stream->getSampleRate())))

TEST_IO_STREAM(GetChannelMask, "Check that the stream channel mask == the one it was opened with",
               ASSERT_EQ(audioConfig.channelMask, extract(stream->getChannelMask())))

TEST_IO_STREAM(GetFormat, "Check that the stream format == the one it was opened with",
               ASSERT_EQ(audioConfig.format, extract(stream->getFormat())))
#endif

// TODO: for now only check that the framesize is not incoherent
TEST_IO_STREAM(GetFrameSize, "Check that the stream frame size == the one it was opened with",
               ASSERT_GT(extract(stream->getFrameSize()), 0U))

TEST_IO_STREAM(GetBufferSize, "Check that the stream buffer size== the one it was opened with",
               ASSERT_GE(extract(stream->getBufferSize()), extract(stream->getFrameSize())));

template <class Property, class CapabilityGetter>
static void testCapabilityGetter(const std::string& name, IStream* stream,
                                 CapabilityGetter capabilityGetter,
                                 Return<Property> (IStream::*getter)(),
                                 Return<Result> (IStream::*setter)(Property),
                                 bool currentMustBeSupported = true) {
    hidl_vec<Property> capabilities;
    auto ret = capabilityGetter(stream, capabilities);
    ASSERT_RESULT(okOrNotSupported, ret);
    bool notSupported = ret == Result::NOT_SUPPORTED;
    if (notSupported) {
        doc::partialTest(name + " is not supported");
        return;
    };

    if (currentMustBeSupported) {
        ASSERT_NE(0U, capabilities.size()) << name << " must not return an empty list";
        Property currentValue = extract((stream->*getter)());
        EXPECT_TRUE(std::find(capabilities.begin(), capabilities.end(), currentValue) !=
                    capabilities.end())
            << "value returned by " << name << "() = " << testing::PrintToString(currentValue)
            << " is not in the list of the supported ones " << toString(capabilities);
    }

    // Check that all declared supported values are indeed supported
    for (auto capability : capabilities) {
        auto ret = (stream->*setter)(capability);
        ASSERT_TRUE(ret.isOk());
        if (ret == Result::NOT_SUPPORTED) {
            doc::partialTest("Setter is not supported");
            return;
        }
        ASSERT_OK(ret);
        ASSERT_EQ(capability, extract((stream->*getter)()));
    }
}

#if MAJOR_VERSION <= 6
TEST_IO_STREAM(SupportedSampleRate, "Check that the stream sample rate is declared as supported",
               testCapabilityGetter("getSupportedSampleRate", stream.get(),
                                    &GetSupported::sampleRates, &IStream::getSampleRate,
                                    &IStream::setSampleRate,
                                    // getSupportedSampleRate returns the native sampling rates,
                                    // (the sampling rates that can be played without resampling)
                                    // but other sampling rates can be supported by the HAL.
                                    false))

TEST_IO_STREAM(SupportedChannelMask, "Check that the stream channel mask is declared as supported",
               testCapabilityGetter("getSupportedChannelMask", stream.get(),
                                    &GetSupported::channelMasks, &IStream::getChannelMask,
                                    &IStream::setChannelMask))

TEST_IO_STREAM(SupportedFormat, "Check that the stream format is declared as supported",
               testCapabilityGetter("getSupportedFormat", stream.get(), &GetSupported::formats,
                                    &IStream::getFormat, &IStream::setFormat))
#else
static void testGetSupportedProfiles(IStream* stream) {
    Result res;
    hidl_vec<AudioProfile> profiles;
    auto ret = stream->getSupportedProfiles(returnIn(res, profiles));
    EXPECT_TRUE(ret.isOk());
    if (res == Result::OK) {
        EXPECT_GT(profiles.size(), 0);
    } else {
        EXPECT_EQ(Result::NOT_SUPPORTED, res);
    }
}

TEST_IO_STREAM(GetSupportedProfiles, "Try to call optional method GetSupportedProfiles",
               testGetSupportedProfiles(stream.get()))

static void testSetAudioProperties(IStream* stream) {
    Result res;
    hidl_vec<AudioProfile> profiles;
    auto ret = stream->getSupportedProfiles(returnIn(res, profiles));
    EXPECT_TRUE(ret.isOk());
    if (res == Result::NOT_SUPPORTED) {
        GTEST_SKIP() << "Retrieving supported profiles is not implemented";
    }
    for (const auto& profile : profiles) {
        for (const auto& sampleRate : profile.sampleRates) {
            for (const auto& channelMask : profile.channelMasks) {
                AudioConfigBaseOptional config;
                config.format.value(profile.format);
                config.sampleRateHz.value(sampleRate);
                config.channelMask.value(channelMask);
                auto ret = stream->setAudioProperties(config);
                EXPECT_TRUE(ret.isOk());
                if (ret == Result::NOT_SUPPORTED) {
                    GTEST_SKIP() << "setAudioProperties is not supported";
                }
                EXPECT_EQ(Result::OK, ret)
                        << profile.format << "; " << sampleRate << "; " << channelMask;
            }
        }
    }
}

TEST_IO_STREAM(SetAudioProperties, "Call setAudioProperties for all supported profiles",
               testSetAudioProperties(stream.get()))
#endif  // MAJOR_VERSION <= 6

static void testGetAudioProperties(IStream* stream, AudioConfig expectedConfig) {
#if MAJOR_VERSION <= 6
    uint32_t sampleRateHz;
    auto mask = mkEnumBitfield<AudioChannelMask>({});
    AudioFormat format;

    auto ret = stream->getAudioProperties(returnIn(sampleRateHz, mask, format));
    EXPECT_TRUE(ret.isOk());

    // FIXME: the qcom hal it does not currently negotiate the sampleRate &
    // channel mask
    EXPECT_EQ(expectedConfig.sampleRateHz, sampleRateHz);
    EXPECT_EQ(expectedConfig.channelMask, mask);
    EXPECT_EQ(expectedConfig.format, format);
#elif MAJOR_VERSION >= 7
    Result res;
    AudioConfigBase actualConfig{};
    auto ret = stream->getAudioProperties(returnIn(res, actualConfig));
    EXPECT_TRUE(ret.isOk());
    EXPECT_EQ(Result::OK, res);
    EXPECT_EQ(expectedConfig.base.sampleRateHz, actualConfig.sampleRateHz);
    EXPECT_EQ(expectedConfig.base.channelMask, actualConfig.channelMask);
    EXPECT_EQ(expectedConfig.base.format, actualConfig.format);
#endif
}

TEST_IO_STREAM(GetAudioProperties,
               "Check that the stream audio properties == the ones it was opened with",
               testGetAudioProperties(stream.get(), audioConfig))

TEST_IO_STREAM(SetHwAvSync, "Try to set hardware sync to an invalid value",
               ASSERT_RESULT(okOrNotSupportedOrInvalidArgs, stream->setHwAvSync(666)))

static void checkGetNoParameter(IStream* stream, hidl_vec<hidl_string> keys,
                                std::initializer_list<Result> expectedResults) {
    hidl_vec<ParameterValue> parameters;
    Result res;
    ASSERT_OK(Parameters::get(stream, keys, returnIn(res, parameters)));
    ASSERT_RESULT(expectedResults, res);
    if (res == Result::OK) {
        for (auto& parameter : parameters) {
            ASSERT_EQ(0U, parameter.value.size()) << toString(parameter);
        }
    }
}

/* Get/Set parameter is intended to be an opaque channel between vendors app and
 * their HALs.
 * Thus can not be meaningfully tested.
 */
TEST_IO_STREAM(getEmptySetParameter, "Retrieve the values of an empty set",
               checkGetNoParameter(stream.get(), {} /* keys */, {Result::OK}))

TEST_IO_STREAM(getNonExistingParameter, "Retrieve the values of an non existing parameter",
               checkGetNoParameter(stream.get(), {"Non existing key"} /* keys */,
                                   {Result::NOT_SUPPORTED}))

TEST_IO_STREAM(setEmptySetParameter, "Set the values of an empty set of parameters",
               ASSERT_RESULT(Result::OK, Parameters::set(stream, {})))

TEST_IO_STREAM(setNonExistingParameter, "Set the values of an non existing parameter",
               // Unfortunately, the set_parameter legacy interface did not return any
               // error code when a key is not supported.
               // To allow implementation to just wrapped the legacy one, consider OK as a
               // valid result for setting a non existing parameter.
               ASSERT_RESULT(okOrNotSupportedOrInvalidArgs,
                             Parameters::set(stream, {{"non existing key", "0"}})))

TEST_IO_STREAM(DebugDump, "Check that a stream can dump its state without error",
               testDebugDump([this](const auto& handle) { return dump(stream, handle); }))

TEST_IO_STREAM(DebugDumpInvalidArguments,
               "Check that the stream dump doesn't crash on invalid arguments",
               ASSERT_OK(dump(stream, hidl_handle())))

//////////////////////////////////////////////////////////////////////////////
////////////////////////////// addRemoveEffect ///////////////////////////////
//////////////////////////////////////////////////////////////////////////////

TEST_IO_STREAM(AddNonExistingEffect, "Adding a non existing effect should fail",
               ASSERT_RESULT(Result::INVALID_ARGUMENTS, stream->addEffect(666)))
TEST_IO_STREAM(RemoveNonExistingEffect, "Removing a non existing effect should fail",
               ASSERT_RESULT(Result::INVALID_ARGUMENTS, stream->removeEffect(666)))

// TODO: positive tests

//////////////////////////////////////////////////////////////////////////////
/////////////////////////////// Control ////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

TEST_IO_STREAM(standby, "Make sure the stream can be put in stanby",
               ASSERT_OK(stream->standby()))  // can not fail

TEST_IO_STREAM(startNoMmap, "Starting a mmaped stream before mapping it should fail",
               ASSERT_RESULT(invalidStateOrNotSupported, stream->start()))

TEST_IO_STREAM(stopNoMmap, "Stopping a mmaped stream before mapping it should fail",
               ASSERT_RESULT(invalidStateOrNotSupported, stream->stop()))

TEST_IO_STREAM(getMmapPositionNoMmap, "Get a stream Mmap position before mapping it should fail",
               ASSERT_RESULT(invalidStateOrNotSupported, stream->stop()))

TEST_IO_STREAM(close, "Make sure a stream can be closed", ASSERT_OK(closeStream()))
// clang-format off
TEST_IO_STREAM(closeTwice, "Make sure a stream can not be closed twice",
        ASSERT_OK(closeStream(false /*clear*/));
        ASSERT_EQ(Result::INVALID_STATE, closeStream()))
// clang-format on

static void testMmapBufferOfInvalidSize(IStream* stream) {
    for (int32_t value : {-1, 0, std::numeric_limits<int32_t>::max()}) {
        MmapBufferInfo info;
        Result res;
        EXPECT_OK(stream->createMmapBuffer(value, returnIn(res, info)));
        EXPECT_RESULT(invalidArgsOrNotSupported, res) << "value=" << value;
    }
}

TEST_IO_STREAM(CreateTooBigMmapBuffer, "Create mmap buffer of invalid size must fail",
               testMmapBufferOfInvalidSize(stream.get()))

static void testGetMmapPositionOfNonMmapedStream(IStream* stream) {
    Result res;
    MmapPosition position;
    ASSERT_OK(stream->getMmapPosition(returnIn(res, position)));
    ASSERT_RESULT(invalidArgsOrNotSupported, res);
}

TEST_IO_STREAM(GetMmapPositionOfNonMmapedStream,
               "Retrieving the mmap position of a non mmaped stream should fail",
               testGetMmapPositionOfNonMmapedStream(stream.get()))

//////////////////////////////////////////////////////////////////////////////
///////////////////////////////// StreamIn ///////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

TEST_P(InputStreamTest, GetAudioSource) {
    doc::test("Retrieving the audio source of an input stream should always succeed");
    AudioSource source;
    ASSERT_OK(stream->getAudioSource(returnIn(res, source)));
    if (res == Result::NOT_SUPPORTED) {
        doc::partialTest("getAudioSource is not supported");
        return;
    }
    ASSERT_OK(res);
#if MAJOR_VERSION <= 6
    ASSERT_EQ(AudioSource::DEFAULT, source);
#elif MAJOR_VERSION >= 7
    ASSERT_EQ(xsd::AudioSource::AUDIO_SOURCE_DEFAULT, xsd::stringToAudioSource(source));
#endif
}

static void testUnitaryGain(std::function<Return<Result>(float)> setGain) {
    for (float value : (float[]){-INFINITY, -1.0, 1.0 + std::numeric_limits<float>::epsilon(), 2.0,
                                 INFINITY, NAN}) {
        EXPECT_RESULT(Result::INVALID_ARGUMENTS, setGain(value)) << "value=" << value;
    }
    // Do not consider -0.0 as an invalid value as it is == with 0.0
    for (float value : {-0.0, 0.0, 0.01, 0.5, 0.09, 1.0 /* Restore volume*/}) {
        EXPECT_OK(setGain(value)) << "value=" << value;
    }
}

static void testOptionalUnitaryGain(std::function<Return<Result>(float)> setGain,
                                    std::string debugName) {
    auto result = setGain(1);
    ASSERT_IS_OK(result);
    if (result == Result::NOT_SUPPORTED) {
        doc::partialTest(debugName + " is not supported");
        return;
    }
    testUnitaryGain(setGain);
}

TEST_P(InputStreamTest, SetGain) {
    doc::test("The gain of an input stream should only be set between [0,1]");
    testOptionalUnitaryGain([this](float volume) { return stream->setGain(volume); },
                            "InputStream::setGain");
}

static void testPrepareForReading(
        ::android::hardware::audio::CORE_TYPES_CPP_VERSION::IStreamIn* stream, uint32_t frameSize,
        uint32_t framesCount) {
    Result res;
    // Ignore output parameters as the call should fail
    ASSERT_OK(stream->prepareForReading(frameSize, framesCount,
                                        [&res](auto r, auto&, auto&, auto&, auto) { res = r; }));
    EXPECT_RESULT(Result::INVALID_ARGUMENTS, res);
}

TEST_P(InputStreamTest, PrepareForReadingWithZeroBuffer) {
    doc::test("Preparing a stream for reading with a 0 sized buffer should fail");
    testPrepareForReading(stream.get(), 0, 0);
}

TEST_P(InputStreamTest, PrepareForReadingWithHugeBuffer) {
    doc::test("Preparing a stream for reading with a 2^32 sized buffer should fail");
    testPrepareForReading(stream.get(), 1, std::numeric_limits<uint32_t>::max());
}

TEST_P(InputStreamTest, PrepareForReadingCheckOverflow) {
    doc::test(
        "Preparing a stream for reading with a overflowing sized buffer should "
        "fail");
    auto uintMax = std::numeric_limits<uint32_t>::max();
    testPrepareForReading(stream.get(), uintMax, uintMax);
}

TEST_P(InputStreamTest, GetInputFramesLost) {
    doc::test("The number of frames lost on a never started stream should be 0");
    auto ret = stream->getInputFramesLost();
    ASSERT_IS_OK(ret);
    uint32_t framesLost{ret};
    ASSERT_EQ(0U, framesLost);
}

//////////////////////////////////////////////////////////////////////////////
///////////////////////////////// StreamOut //////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

TEST_P(OutputStreamTest, getLatency) {
    doc::test("Make sure latency is over 0");
    auto result = stream->getLatency();
    ASSERT_IS_OK(result);
    ASSERT_GT(result, 0U);
}

TEST_P(OutputStreamTest, setVolume) {
    doc::test("Try to set the output volume");
    testOptionalUnitaryGain([this](float volume) { return stream->setVolume(volume, volume); },
                            "setVolume");
}

static void testPrepareForWriting(::android::hardware::audio::CPP_VERSION::IStreamOut* stream,
                                  uint32_t frameSize, uint32_t framesCount) {
    Result res;
    // Ignore output parameters as the call should fail
    ASSERT_OK(stream->prepareForWriting(frameSize, framesCount,
                                        [&res](auto r, auto&, auto&, auto&, auto) { res = r; }));
    EXPECT_RESULT(Result::INVALID_ARGUMENTS, res);
}

TEST_P(OutputStreamTest, PrepareForWriteWithZeroBuffer) {
    doc::test("Preparing a stream for writing with a 0 sized buffer should fail");
    testPrepareForWriting(stream.get(), 0, 0);
}

TEST_P(OutputStreamTest, PrepareForWriteWithHugeBuffer) {
    doc::test("Preparing a stream for writing with a 2^32 sized buffer should fail");
    testPrepareForWriting(stream.get(), 1, std::numeric_limits<uint32_t>::max());
}

TEST_P(OutputStreamTest, PrepareForWritingCheckOverflow) {
    doc::test(
        "Preparing a stream for writing with a overflowing sized buffer should "
        "fail");
    auto uintMax = std::numeric_limits<uint32_t>::max();
    testPrepareForWriting(stream.get(), uintMax, uintMax);
}

struct Capability {
    using IStreamOut = ::android::hardware::audio::CPP_VERSION::IStreamOut;

    Capability(IStreamOut* stream) {
        EXPECT_OK(stream->supportsPauseAndResume(returnIn(pause, resume)));
        drain = extract(stream->supportsDrain());
    }
    bool pause = false;
    bool resume = false;
    bool drain = false;
};

TEST_P(OutputStreamTest, SupportsPauseAndResumeAndDrain) {
    doc::test("Implementation must expose pause, resume and drain capabilities");
    Capability(stream.get());
}

TEST_P(OutputStreamTest, GetRenderPosition) {
    doc::test("A new stream render position should be 0 or INVALID_STATE");
    uint32_t dspFrames;
    ASSERT_OK(stream->getRenderPosition(returnIn(res, dspFrames)));
    if (res == Result::NOT_SUPPORTED) {
        doc::partialTest("getRenderPosition is not supported");
        return;
    }
    expectValueOrFailure(res, 0U, dspFrames, Result::INVALID_STATE);
}

TEST_P(OutputStreamTest, GetNextWriteTimestamp) {
    doc::test("A new stream next write timestamp should be 0 or INVALID_STATE");
    uint64_t timestampUs;
    ASSERT_OK(stream->getNextWriteTimestamp(returnIn(res, timestampUs)));
    if (res == Result::NOT_SUPPORTED) {
        doc::partialTest("getNextWriteTimestamp is not supported");
        return;
    }
    expectValueOrFailure(res, uint64_t{0}, timestampUs, Result::INVALID_STATE);
}

/** Stub implementation of out stream callback. */
class MockOutCallbacks : public IStreamOutCallback {
    Return<void> onWriteReady() override { return {}; }
    Return<void> onDrainReady() override { return {}; }
    Return<void> onError() override { return {}; }
};

static bool isAsyncModeSupported(::android::hardware::audio::CPP_VERSION::IStreamOut* stream) {
    auto res = stream->setCallback(new MockOutCallbacks);
    stream->clearCallback();  // try to restore the no callback state, ignore
                              // any error
    EXPECT_RESULT(okOrNotSupported, res);
    return res.isOk() ? res == Result::OK : false;
}

TEST_P(OutputStreamTest, SetCallback) {
    doc::test(
        "If supported, registering callback for async operation should never "
        "fail");
    if (!isAsyncModeSupported(stream.get())) {
        doc::partialTest("The stream does not support async operations");
        return;
    }
    ASSERT_OK(stream->setCallback(new MockOutCallbacks));
    ASSERT_OK(stream->setCallback(new MockOutCallbacks));
}

TEST_P(OutputStreamTest, clearCallback) {
    doc::test(
        "If supported, clearing a callback to go back to sync operation should "
        "not fail");
    if (!isAsyncModeSupported(stream.get())) {
        doc::partialTest("The stream does not support async operations");
        return;
    }
    // TODO: Clarify if clearing a non existing callback should fail
    ASSERT_OK(stream->setCallback(new MockOutCallbacks));
    ASSERT_OK(stream->clearCallback());
}

TEST_P(OutputStreamTest, Resume) {
    doc::test(
        "If supported, a stream should fail to resume if not previously "
        "paused");
    if (!Capability(stream.get()).resume) {
        doc::partialTest("The output stream does not support resume");
        return;
    }
    ASSERT_RESULT(Result::INVALID_STATE, stream->resume());
}

TEST_P(OutputStreamTest, Pause) {
    doc::test(
        "If supported, a stream should fail to pause if not previously "
        "started");
    if (!Capability(stream.get()).pause) {
        doc::partialTest("The output stream does not support pause");
        return;
    }
    ASSERT_RESULT(Result::INVALID_STATE, stream->pause());
}

static void testDrain(::android::hardware::audio::CPP_VERSION::IStreamOut* stream,
                      AudioDrain type) {
    if (!Capability(stream).drain) {
        doc::partialTest("The output stream does not support drain");
        return;
    }
    ASSERT_RESULT(Result::OK, stream->drain(type));
}

TEST_P(OutputStreamTest, DrainAll) {
    doc::test("If supported, a stream should always succeed to drain");
    testDrain(stream.get(), AudioDrain::ALL);
}

TEST_P(OutputStreamTest, DrainEarlyNotify) {
    doc::test("If supported, a stream should always succeed to drain");
    testDrain(stream.get(), AudioDrain::EARLY_NOTIFY);
}

TEST_P(OutputStreamTest, FlushStop) {
    doc::test("If supported, a stream should always succeed to flush");
    auto ret = stream->flush();
    ASSERT_IS_OK(ret);
    if (ret == Result::NOT_SUPPORTED) {
        doc::partialTest("Flush is not supported");
        return;
    }
    ASSERT_OK(ret);
}

TEST_P(OutputStreamTest, GetPresentationPositionStop) {
    doc::test(
        "If supported, a stream should always succeed to retrieve the "
        "presentation position");
    uint64_t frames;
    TimeSpec measureTS;
    ASSERT_OK(stream->getPresentationPosition(returnIn(res, frames, measureTS)));
#if MAJOR_VERSION <= 6
    if (res == Result::NOT_SUPPORTED) {
        doc::partialTest("getPresentationPosition is not supported");
        return;
    }
#else
    ASSERT_NE(Result::NOT_SUPPORTED, res) << "getPresentationPosition is mandatory in V7";
#endif
    ASSERT_EQ(0U, frames);

    if (measureTS.tvNSec == 0 && measureTS.tvSec == 0) {
        // As the stream has never written a frame yet,
        // the timestamp does not really have a meaning, allow to return 0
        return;
    }

    // Make sure the return measure is not more than 1s old.
    struct timespec currentTS;
    ASSERT_EQ(0, clock_gettime(CLOCK_MONOTONIC, &currentTS)) << errno;

    auto toMicroSec = [](uint64_t sec, auto nsec) { return sec * 1e+6 + nsec / 1e+3; };
    auto currentTime = toMicroSec(currentTS.tv_sec, currentTS.tv_nsec);
    auto measureTime = toMicroSec(measureTS.tvSec, measureTS.tvNSec);
    ASSERT_PRED2([](auto c, auto m) { return c - m < 1e+6; }, currentTime, measureTime);
}

//////////////////////////////////////////////////////////////////////////////
/////////////////////////////// PrimaryDevice ////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

TEST_P(AudioPrimaryHidlTest, setVoiceVolume) {
    doc::test("Make sure setVoiceVolume only succeed if volume is in [0,1]");
    testUnitaryGain([this](float volume) { return getDevice()->setVoiceVolume(volume); });
}

TEST_P(BoolAccessorPrimaryHidlTest, BtScoNrecEnabled) {
    doc::test("Query and set the BT SCO NR&EC state");
    testAccessors<OPTIONAL>("BtScoNrecEnabled", Initial{false, OPTIONAL}, {true},
                            &IPrimaryDevice::setBtScoNrecEnabled,
                            &IPrimaryDevice::getBtScoNrecEnabled);
}

TEST_P(BoolAccessorPrimaryHidlTest, setGetBtScoWidebandEnabled) {
    doc::test("Query and set the SCO whideband state");
    testAccessors<OPTIONAL>("BtScoWideband", Initial{false, OPTIONAL}, {true},
                            &IPrimaryDevice::setBtScoWidebandEnabled,
                            &IPrimaryDevice::getBtScoWidebandEnabled);
}

using TtyModeAccessorPrimaryHidlTest =
        AccessorHidlTest<::android::hardware::audio::CPP_VERSION::IPrimaryDevice::TtyMode,
                         AudioPrimaryHidlTest>;
TEST_P(TtyModeAccessorPrimaryHidlTest, setGetTtyMode) {
    doc::test("Query and set the TTY mode state");
    testAccessors<OPTIONAL>(
        "TTY mode", Initial{IPrimaryDevice::TtyMode::OFF},
        {IPrimaryDevice::TtyMode::HCO, IPrimaryDevice::TtyMode::VCO, IPrimaryDevice::TtyMode::FULL},
        &IPrimaryDevice::setTtyMode, &IPrimaryDevice::getTtyMode);
}
INSTANTIATE_TEST_CASE_P(TtyModeAccessorPrimaryHidl, TtyModeAccessorPrimaryHidlTest,
                        ::testing::ValuesIn(getDeviceParametersForPrimaryDeviceTests()),
                        &DeviceParameterToString);
// When the VTS test runs on a device lacking the corresponding HAL version the parameter
// list is empty, this isn't a problem.
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(TtyModeAccessorPrimaryHidlTest);

TEST_P(BoolAccessorPrimaryHidlTest, setGetHac) {
    doc::test("Query and set the HAC state");
    testAccessors<OPTIONAL>("HAC", Initial{false}, {true}, &IPrimaryDevice::setHacEnabled,
                            &IPrimaryDevice::getHacEnabled);
}
