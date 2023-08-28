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

#include <JsonConfigLoader.h>

#include <PropertyUtils.h>

#include <gtest/gtest.h>
#include <sstream>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {

using ::aidl::android::hardware::automotive::vehicle::RawPropValues;
using ::aidl::android::hardware::automotive::vehicle::VehicleAreaConfig;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropConfig;
using ::aidl::android::hardware::automotive::vehicle::VehicleProperty;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropertyAccess;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropertyChangeMode;

class JsonConfigLoaderUnitTest : public ::testing::Test {
  protected:
    JsonConfigLoader mLoader;
};

TEST_F(JsonConfigLoaderUnitTest, testBasic) {
    std::istringstream iss(R"(
    {
        "properties": [{
            "property": 291504388
        }]
    }
    )");

    auto result = mLoader.loadPropConfig(iss);

    ASSERT_TRUE(result.ok()) << result.error().message();

    auto configs = result.value();
    ASSERT_EQ(configs.size(), 1u);
    ASSERT_EQ(configs.begin()->second.config.prop, 291504388);
}

TEST_F(JsonConfigLoaderUnitTest, testRootNotObject) {
    std::istringstream iss(R"(
    []
    )");

    ASSERT_FALSE(mLoader.loadPropConfig(iss).ok()) << "root is not an object must cause error";
}

TEST_F(JsonConfigLoaderUnitTest, testMissingPropertiesField) {
    std::istringstream iss(R"(
    {
        "abcd": 1234
    }
    )");

    ASSERT_FALSE(mLoader.loadPropConfig(iss).ok()) << "Missing 'properties' field must cause error";
}

TEST_F(JsonConfigLoaderUnitTest, testPropertiesFieldNotArray) {
    std::istringstream iss(R"(
    {
        "properties': {'a': 'b'}
    }
    )");

    ASSERT_FALSE(mLoader.loadPropConfig(iss).ok())
            << "'properties' field is not an array must cause error";
}

TEST_F(JsonConfigLoaderUnitTest, testPropertyIsEnum) {
    std::istringstream iss(R"(
    {
        "properties": [{
            "property": "VehicleProperty::INFO_FUEL_CAPACITY"
        }]
    }
    )");

    auto result = mLoader.loadPropConfig(iss);

    ASSERT_TRUE(result.ok()) << result.error().message();

    auto configs = result.value();
    ASSERT_EQ(configs.size(), 1u);
    ASSERT_EQ(configs.begin()->second.config.prop, toInt(VehicleProperty::INFO_FUEL_CAPACITY));
}

TEST_F(JsonConfigLoaderUnitTest, testPropertyEnum_FailInvalidEnum) {
    std::istringstream iss(R"(
    {
        "properties": [{
            "property": "VehicleProperty::BLAH"
        }]
    }
    )");

    ASSERT_FALSE(mLoader.loadPropConfig(iss).ok())
            << "Invalid VehicleProperty enum must cause error";
}

TEST_F(JsonConfigLoaderUnitTest, testPropertyEnum_FailInvalidType) {
    std::istringstream iss(R"(
    {
        "properties": [{
            "property": "test"
        }]
    }
    )");

    ASSERT_FALSE(mLoader.loadPropConfig(iss).ok())
            << "Invalid VehicleProperty type must cause error";
}

TEST_F(JsonConfigLoaderUnitTest, testProperty_FailInvalidJson) {
    std::istringstream iss(R"(
    {
        "properties": [{
    }
    )");

    ASSERT_FALSE(mLoader.loadPropConfig(iss).ok()) << "Invalid JSON format must cause error";
}

TEST_F(JsonConfigLoaderUnitTest, testConfigArray) {
    std::istringstream iss(R"(
    {
        "properties": [{
            "property": "VehicleProperty::INFO_FUEL_CAPACITY",
            "configArray": [1, 2, 3]
        }]
    }
    )");

    auto result = mLoader.loadPropConfig(iss);

    ASSERT_TRUE(result.ok()) << result.error().message();

    auto configs = result.value();
    ASSERT_EQ(configs.size(), 1u);
    ASSERT_EQ(configs.begin()->second.config.configArray, std::vector<int>({1, 2, 3}));
}

TEST_F(JsonConfigLoaderUnitTest, testConfigArrayConstants) {
    std::istringstream iss(R"(
    {
        "properties": [{
            "property": "VehicleProperty::INFO_FUEL_CAPACITY",
            "configArray": [1, 2, "Constants::FUEL_DOOR_REAR_LEFT"]
        }]
    }
    )");

    auto result = mLoader.loadPropConfig(iss);

    ASSERT_TRUE(result.ok()) << result.error().message();

    auto configs = result.value();
    ASSERT_EQ(configs.size(), 1u);
    ASSERT_EQ(configs.begin()->second.config.configArray,
              std::vector<int>({1, 2, FUEL_DOOR_REAR_LEFT}));
}

// We have special logic to deal with GALLON and US_GALLON since they share the same value.
TEST_F(JsonConfigLoaderUnitTest, testConfigArrayUnitGallon) {
    std::istringstream iss(R"(
    {
        "properties": [{
            "property": "VehicleProperty::INFO_FUEL_CAPACITY",
            "configArray": [1, 2, "VehicleUnit::GALLON"]
        }]
    }
    )");

    auto result = mLoader.loadPropConfig(iss);

    ASSERT_TRUE(result.ok()) << result.error().message();
}

TEST_F(JsonConfigLoaderUnitTest, testConfigArrayUnitUsGallon) {
    std::istringstream iss(R"(
    {
        "properties": [{
            "property": "VehicleProperty::INFO_FUEL_CAPACITY",
            "configArray": [1, 2, "VehicleUnit::US_GALLON"]
        }]
    }
    )");

    auto result = mLoader.loadPropConfig(iss);

    ASSERT_TRUE(result.ok()) << result.error().message();
}

TEST_F(JsonConfigLoaderUnitTest, testConfigArray_FailInvalidEnum) {
    std::istringstream iss(R"(
    {
        "properties": [{
            "property": "VehicleProperty::INFO_FUEL_CAPACITY",
            "configArray": [1, 2, "VehicleUnits::BLAH"]
        }]
    }
    )");

    ASSERT_FALSE(mLoader.loadPropConfig(iss).ok())
            << "Invalid enum in ConfigArray must cause error";
}

TEST_F(JsonConfigLoaderUnitTest, testConfigArray_FailNotArray) {
    std::istringstream iss(R"(
    {
        "properties": [{
            "property": "VehicleProperty::INFO_FUEL_CAPACITY",
            "configArray": "123"
        }]
    }
    )");

    ASSERT_FALSE(mLoader.loadPropConfig(iss).ok())
            << "ConfigArray is not an array must cause error";
}

TEST_F(JsonConfigLoaderUnitTest, testConfigString) {
    std::istringstream iss(R"(
    {
        "properties": [{
            "property": "VehicleProperty::INFO_FUEL_CAPACITY",
            "configString": "test"
        }]
    }
    )");

    auto result = mLoader.loadPropConfig(iss);

    ASSERT_TRUE(result.ok()) << result.error().message();

    auto configs = result.value();
    ASSERT_EQ(configs.size(), 1u);
    ASSERT_EQ(configs.begin()->second.config.configString, "test");
}

TEST_F(JsonConfigLoaderUnitTest, testConfigString_FailNotString) {
    std::istringstream iss(R"(
    {
        "properties": [{
            "property": "VehicleProperty::INFO_FUEL_CAPACITY",
            "configString": 1234
        }]
    }
    )");

    ASSERT_FALSE(mLoader.loadPropConfig(iss).ok())
            << "ConfigString is not a String must cause error";
}

TEST_F(JsonConfigLoaderUnitTest, testCheckDefaultAccessChangeMode) {
    std::istringstream iss(R"(
    {
        "properties": [{
            "property": "VehicleProperty::INFO_FUEL_CAPACITY"
        }]
    }
    )");

    auto result = mLoader.loadPropConfig(iss);

    ASSERT_TRUE(result.ok()) << result.error().message();
    auto configs = result.value();
    ASSERT_EQ(configs.size(), 1u);

    const VehiclePropConfig& propConfig = configs.begin()->second.config;
    ASSERT_EQ(propConfig.access, VehiclePropertyAccess::READ);
    ASSERT_EQ(propConfig.changeMode, VehiclePropertyChangeMode::STATIC);
}

TEST_F(JsonConfigLoaderUnitTest, testAccessOverride) {
    std::istringstream iss(R"(
    {
        "properties": [{
            "property": "VehicleProperty::INFO_FUEL_CAPACITY",
            "access": "VehiclePropertyAccess::WRITE"
        }]
    }
    )");

    auto result = mLoader.loadPropConfig(iss);

    ASSERT_TRUE(result.ok()) << result.error().message();
    auto configs = result.value();
    ASSERT_EQ(configs.size(), 1u);

    const VehiclePropConfig& propConfig = configs.begin()->second.config;
    ASSERT_EQ(propConfig.access, VehiclePropertyAccess::WRITE);
    ASSERT_EQ(propConfig.changeMode, VehiclePropertyChangeMode::STATIC);
}

TEST_F(JsonConfigLoaderUnitTest, testChangeModeOverride) {
    std::istringstream iss(R"(
    {
        "properties": [{
            "property": "VehicleProperty::INFO_FUEL_CAPACITY",
            "changeMode": "VehiclePropertyChangeMode::ON_CHANGE"
        }]
    }
    )");

    auto result = mLoader.loadPropConfig(iss);

    ASSERT_TRUE(result.ok()) << result.error().message();
    auto configs = result.value();
    ASSERT_EQ(configs.size(), 1u);

    const VehiclePropConfig& propConfig = configs.begin()->second.config;
    ASSERT_EQ(propConfig.access, VehiclePropertyAccess::READ);
    ASSERT_EQ(propConfig.changeMode, VehiclePropertyChangeMode::ON_CHANGE);
}

TEST_F(JsonConfigLoaderUnitTest, testCustomProp) {
    std::istringstream iss(R"(
    {
        "properties": [{
            "property": 1234,
            "access": "VehiclePropertyAccess::WRITE",
            "changeMode": "VehiclePropertyChangeMode::ON_CHANGE"
        }]
    }
    )");

    auto result = mLoader.loadPropConfig(iss);

    ASSERT_TRUE(result.ok()) << result.error().message();
    auto configs = result.value();
    ASSERT_EQ(configs.size(), 1u);

    const VehiclePropConfig& propConfig = configs.begin()->second.config;
    ASSERT_EQ(propConfig.access, VehiclePropertyAccess::WRITE);
    ASSERT_EQ(propConfig.changeMode, VehiclePropertyChangeMode::ON_CHANGE);
}

TEST_F(JsonConfigLoaderUnitTest, testCustomProp_FailMissingAccess) {
    std::istringstream iss(R"(
    {
        "properties": [{
            "property": 1234,
            "changeMode": "VehiclePropertyChangeMode::ON_CHANGE"
        }]
    }
    )");

    ASSERT_FALSE(mLoader.loadPropConfig(iss).ok())
            << "Missing access for custom property must cause error";
}

TEST_F(JsonConfigLoaderUnitTest, testCustomProp_FailMissingChangeMode) {
    std::istringstream iss(R"(
    {
        "properties": [{
            "property": 1234,
            "access": "VehiclePropertyAccess::WRITE"
        }]
    }
    )");

    ASSERT_FALSE(mLoader.loadPropConfig(iss).ok())
            << "Missing change mode for custom property must cause error";
}

TEST_F(JsonConfigLoaderUnitTest, testMinSampleRate) {
    std::istringstream iss(R"(
    {
        "properties": [{
            "property": "VehicleProperty::INFO_FUEL_CAPACITY",
            "minSampleRate": 1,
        }]
    }
    )");

    auto result = mLoader.loadPropConfig(iss);

    ASSERT_TRUE(result.ok()) << result.error().message();

    auto configs = result.value();
    ASSERT_EQ(configs.size(), 1u);
    ASSERT_EQ(configs.begin()->second.config.minSampleRate, 1);
}

TEST_F(JsonConfigLoaderUnitTest, testMinSampleRate_FailInvalidType) {
    std::istringstream iss(R"(
    {
        "properties": [{
            "property": "VehicleProperty::INFO_FUEL_CAPACITY",
            "minSampleRate": "abcd",
        }]
    }
    )");

    ASSERT_FALSE(mLoader.loadPropConfig(iss).ok())
            << "Wrong type for MinSampleRate must cause error";
}

TEST_F(JsonConfigLoaderUnitTest, testMaxSampleRate) {
    std::istringstream iss(R"(
    {
        "properties": [{
            "property": "VehicleProperty::INFO_FUEL_CAPACITY",
            "maxSampleRate": 1,
        }]
    }
    )");

    auto result = mLoader.loadPropConfig(iss);

    ASSERT_TRUE(result.ok()) << result.error().message();

    auto configs = result.value();
    ASSERT_EQ(configs.size(), 1u);
    ASSERT_EQ(configs.begin()->second.config.maxSampleRate, 1);
}

TEST_F(JsonConfigLoaderUnitTest, testMaxSampleRate_FailInvalidType) {
    std::istringstream iss(R"(
    {
        "properties": [{
            "property": "VehicleProperty::INFO_FUEL_CAPACITY",
            "maxSampleRate": "abcd",
        }]
    }
    )");

    ASSERT_FALSE(mLoader.loadPropConfig(iss).ok())
            << "Wrong type for MaxSampleRate must cause error";
}

TEST_F(JsonConfigLoaderUnitTest, testDefaultValue_Simple) {
    std::istringstream iss(R"(
    {
        "properties": [{
            "property": "VehicleProperty::INFO_FUEL_CAPACITY",
            "defaultValue": {
                "int32Values": [1, 2]
            }
        }]
    }
    )");

    auto result = mLoader.loadPropConfig(iss);

    ASSERT_TRUE(result.ok()) << result.error().message();

    auto configs = result.value();
    ASSERT_EQ(configs.size(), 1u);
    ASSERT_EQ(configs.begin()->second.initialValue.int32Values, std::vector<int32_t>({1, 2}));
}

TEST_F(JsonConfigLoaderUnitTest, testDefaultValue_Mixed) {
    std::istringstream iss(R"(
    {
        "properties": [{
            "property": "VehicleProperty::INFO_FUEL_CAPACITY",
            "defaultValue": {
                "int32Values": [1, "Constants::FUEL_DOOR_REAR_LEFT"],
                "int64Values": [2, "Constants::FUEL_DOOR_REAR_LEFT"],
                "floatValues": [3.0, "Constants::FUEL_DOOR_REAR_LEFT"],
                "stringValue": "abcd"
            }
        }]
    }
    )");

    auto result = mLoader.loadPropConfig(iss);

    ASSERT_TRUE(result.ok()) << result.error().message();

    auto configs = result.value();
    ASSERT_EQ(configs.size(), 1u);

    const RawPropValues& initialValue = configs.begin()->second.initialValue;
    ASSERT_EQ(initialValue.int32Values, std::vector<int32_t>({1, FUEL_DOOR_REAR_LEFT}));
    ASSERT_EQ(initialValue.int64Values,
              std::vector<int64_t>({2, static_cast<int64_t>(FUEL_DOOR_REAR_LEFT)}));
    ASSERT_EQ(initialValue.floatValues,
              std::vector<float>({3.0, static_cast<float>(FUEL_DOOR_REAR_LEFT)}));
    ASSERT_EQ(initialValue.stringValue, "abcd");
}

TEST_F(JsonConfigLoaderUnitTest, testDefaultValue_FailNotObject) {
    std::istringstream iss(R"(
    {
        "properties": [{
            "property": "VehicleProperty::INFO_FUEL_CAPACITY",
            "defaultValue": []
        }]
    }
    )");

    ASSERT_FALSE(mLoader.loadPropConfig(iss).ok())
            << "DefaultValue is not an object must cause error";
}

TEST_F(JsonConfigLoaderUnitTest, testDefaultValue_FailInvalidType) {
    std::istringstream iss(R"(
    {
        "properties": [{
            "property": "VehicleProperty::INFO_FUEL_CAPACITY",
            "defaultValue": {
                "int32Values": [1.1]
            }
        }]
    }
    )");

    ASSERT_FALSE(mLoader.loadPropConfig(iss).ok())
            << "Wrong type for DefaultValue must cause error";
}

TEST_F(JsonConfigLoaderUnitTest, testAreas_Simple) {
    std::istringstream iss(R"(
    {
        "properties": [{
            "property": "VehicleProperty::INFO_FUEL_CAPACITY",
            "areas": [{
                "areaId": "Constants::HVAC_ALL",
                "minInt32Value": 1,
                "maxInt32Value": 7
            }]
        }]
    }
    )");

    auto result = mLoader.loadPropConfig(iss);

    auto configs = result.value();
    ASSERT_EQ(configs.size(), 1u);

    const VehiclePropConfig& config = configs.begin()->second.config;
    ASSERT_EQ(config.areaConfigs.size(), 1u);
    const VehicleAreaConfig& areaConfig = config.areaConfigs[0];
    ASSERT_EQ(areaConfig.minInt32Value, 1);
    ASSERT_EQ(areaConfig.maxInt32Value, 7);
    ASSERT_EQ(areaConfig.areaId, HVAC_ALL);
}

TEST_F(JsonConfigLoaderUnitTest, testAreas_DefaultValueForEachArea) {
    std::istringstream iss(R"(
    {
        "properties": [{
            "property": "VehicleProperty::INFO_FUEL_CAPACITY",
            "areas": [{
                "areaId": "Constants::HVAC_LEFT",
                "defaultValue": {
                    "int32Values": [1]
                }
            }, {
                "areaId": "Constants::HVAC_RIGHT",
                "defaultValue": {
                    "int32Values": [2]
                }
            }]
        }]
    }
    )");

    auto result = mLoader.loadPropConfig(iss);

    auto configs = result.value();
    ASSERT_EQ(configs.size(), 1u);

    const VehiclePropConfig& config = configs.begin()->second.config;
    ASSERT_EQ(config.areaConfigs.size(), 2u);
    ASSERT_EQ(config.areaConfigs[0].areaId, HVAC_LEFT);
    ASSERT_EQ(config.areaConfigs[1].areaId, HVAC_RIGHT);
    ASSERT_EQ(configs.begin()->second.initialAreaValues[HVAC_LEFT],
              RawPropValues{.int32Values = {1}});
    ASSERT_EQ(configs.begin()->second.initialAreaValues[HVAC_RIGHT],
              RawPropValues{.int32Values = {2}});
}

TEST_F(JsonConfigLoaderUnitTest, testAreas_FailInvalidTypeForOneAreaValue) {
    std::istringstream iss(R"(
    {
        "properties": [{
            "property": "VehicleProperty::INFO_FUEL_CAPACITY",
            "areas": [{
                "areaId": "Constants::HVAC_LEFT",
                "defaultValue": {
                    "int32Values": [1]
                }
            }, {
                "areaId": "Constants::HVAC_RIGHT",
                "defaultValue": {
                    "int32Values": [2.1]
                }
            }]
        }]
    }
    )");

    ASSERT_FALSE(mLoader.loadPropConfig(iss).ok())
            << "Wrong type for DefaultValue for one area must cause error";
}

TEST_F(JsonConfigLoaderUnitTest, testAreas_HandlesNoSupportedEnumValuesDeclared) {
    std::istringstream iss(R"(
    {
        "properties": [{
            "property": "VehicleProperty::CABIN_LIGHTS_STATE",
            "areas": [{
                "areaId": 0,
            }]
        }]
    }
    )");

    auto result = mLoader.loadPropConfig(iss);
    ASSERT_TRUE(result.ok());

    auto configs = result.value();
    ASSERT_EQ(configs.size(), 1u);

    const VehiclePropConfig& config = configs.begin()->second.config;
    ASSERT_EQ(config.areaConfigs.size(), 1u);

    const VehicleAreaConfig& areaConfig = config.areaConfigs[0];
    ASSERT_EQ(areaConfig.areaId, 0);
    ASSERT_FALSE(areaConfig.supportedEnumValues);
}

TEST_F(JsonConfigLoaderUnitTest, testAreas_HandlesSupportedEnumValues) {
    std::istringstream iss(R"(
    {
        "properties": [{
            "property": "VehicleProperty::CABIN_LIGHTS_STATE",
            "areas": [{
                "areaId": 0,
                "supportedEnumValues": ["Constants::LIGHT_STATE_ON", "Constants::LIGHT_STATE_OFF"]
            }]
        }]
    }
    )");

    auto result = mLoader.loadPropConfig(iss);
    ASSERT_TRUE(result.ok());

    auto configs = result.value();
    ASSERT_EQ(configs.size(), 1u);

    const VehiclePropConfig& config = configs.begin()->second.config;
    ASSERT_EQ(config.areaConfigs.size(), 1u);

    const VehicleAreaConfig& areaConfig = config.areaConfigs[0];
    ASSERT_EQ(areaConfig.areaId, 0);
    ASSERT_TRUE(areaConfig.supportedEnumValues);
    ASSERT_EQ(areaConfig.supportedEnumValues.value().size(), 2u);
    ASSERT_EQ(areaConfig.supportedEnumValues.value(),
              std::vector<int64_t>({LIGHT_STATE_ON, LIGHT_STATE_OFF}));
}

TEST_F(JsonConfigLoaderUnitTest, testAreas_HandlesEmptySupportedEnumValues) {
    std::istringstream iss(R"(
    {
        "properties": [{
            "property": "VehicleProperty::CABIN_LIGHTS_STATE",
            "areas": [{
                "areaId": 0,
                "supportedEnumValues": []
            }]
        }]
    }
    )");

    auto result = mLoader.loadPropConfig(iss);
    ASSERT_TRUE(result.ok());

    auto configs = result.value();
    ASSERT_EQ(configs.size(), 1u);

    const VehiclePropConfig& config = configs.begin()->second.config;
    ASSERT_EQ(config.areaConfigs.size(), 1u);

    const VehicleAreaConfig& areaConfig = config.areaConfigs[0];
    ASSERT_EQ(areaConfig.areaId, 0);
    ASSERT_FALSE(areaConfig.supportedEnumValues);
}

}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
