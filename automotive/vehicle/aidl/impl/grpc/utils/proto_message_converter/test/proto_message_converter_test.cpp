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

#include <vector>

#include <JsonConfigLoader.h>
#include <ProtoMessageConverter.h>
#include <VehicleHalTypes.h>

#include <android-base/file.h>
#include <android-base/format.h>
#include <android/hardware/automotive/vehicle/VehiclePropConfig.pb.h>
#include <android/hardware/automotive/vehicle/VehiclePropValue.pb.h>
#include <gtest/gtest.h>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace proto_msg_converter {

namespace {

namespace proto = ::android::hardware::automotive::vehicle::proto;
namespace aidl_vehicle = ::aidl::android::hardware::automotive::vehicle;

constexpr char DEFAULT_PROPERTIES_CONFIG[] = "DefaultProperties.json";

inline std::string getConfigPath(const std::string& name) {
    return android::base::GetExecutableDirectory() + "/" + name;
}

std::vector<aidl_vehicle::VehiclePropConfig> prepareTestConfigs() {
    JsonConfigLoader loader;
    auto result = loader.loadPropConfig(getConfigPath(DEFAULT_PROPERTIES_CONFIG));
    if (!result.ok()) {
        return {};
    }
    std::vector<aidl_vehicle::VehiclePropConfig> configs;
    for (auto& [_, configDeclaration] : result.value()) {
        configs.push_back(configDeclaration.config);
    }
    return configs;
}

std::vector<aidl_vehicle::VehiclePropValue> prepareTestValues() {
    JsonConfigLoader loader;
    auto result = loader.loadPropConfig(getConfigPath(DEFAULT_PROPERTIES_CONFIG));
    if (!result.ok()) {
        return {};
    }
    std::vector<aidl_vehicle::VehiclePropValue> values;
    int64_t timestamp = 1;
    for (auto& [_, configDeclaration] : result.value()) {
        values.push_back({
                .timestamp = timestamp,
                .areaId = 123,
                .prop = configDeclaration.config.prop,
                .value = configDeclaration.initialValue,
                .status = aidl_vehicle::VehiclePropertyStatus::ERROR,
        });
    }
    return values;
}

class PropConfigConversionTest : public testing::TestWithParam<aidl_vehicle::VehiclePropConfig> {};

class PropValueConversionTest : public testing::TestWithParam<aidl_vehicle::VehiclePropValue> {};

}  // namespace

TEST_P(PropConfigConversionTest, testConversion) {
    proto::VehiclePropConfig protoCfg;
    aidl_vehicle::VehiclePropConfig aidlCfg;

    aidlToProto(GetParam(), &protoCfg);
    protoToAidl(protoCfg, &aidlCfg);

    EXPECT_EQ(aidlCfg, GetParam());
}

TEST_P(PropValueConversionTest, testConversion) {
    proto::VehiclePropValue protoVal;
    aidl_vehicle::VehiclePropValue aidlVal;

    aidlToProto(GetParam(), &protoVal);
    protoToAidl(protoVal, &aidlVal);

    EXPECT_EQ(aidlVal, GetParam());
}

INSTANTIATE_TEST_SUITE_P(DefaultConfigs, PropConfigConversionTest,
                         ::testing::ValuesIn(prepareTestConfigs()),
                         [](const ::testing::TestParamInfo<aidl_vehicle::VehiclePropConfig>& info) {
                             return ::fmt::format("property_{:d}", info.param.prop);
                         });

INSTANTIATE_TEST_SUITE_P(TestValues, PropValueConversionTest,
                         ::testing::ValuesIn(prepareTestValues()),
                         [](const ::testing::TestParamInfo<aidl_vehicle::VehiclePropValue>& info) {
                             return ::fmt::format("property_{:d}", info.param.prop);
                         });

}  // namespace proto_msg_converter
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
