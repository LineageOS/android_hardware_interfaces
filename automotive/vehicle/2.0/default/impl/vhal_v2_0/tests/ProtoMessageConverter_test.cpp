/*
 * Copyright (C) 2019 The Android Open Source Project
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

#include <gtest/gtest.h>

#include <utils/SystemClock.h>

#include "vhal_v2_0/DefaultConfig.h"
#include "vhal_v2_0/ProtoMessageConverter.h"
#include "vhal_v2_0/VehicleUtils.h"

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace V2_0 {
namespace impl {
namespace proto_msg_converter {

namespace {

void CheckPropConfigConversion(const VehiclePropConfig& config) {
    vhal_proto::VehiclePropConfig protoCfg;
    VehiclePropConfig tmpConfig;

    toProto(&protoCfg, config);
    fromProto(&tmpConfig, protoCfg);

    EXPECT_EQ(config.prop, tmpConfig.prop);
    EXPECT_EQ(config.access, tmpConfig.access);
    EXPECT_EQ(config.changeMode, tmpConfig.changeMode);
    EXPECT_EQ(config.configString, tmpConfig.configString);
    EXPECT_EQ(config.minSampleRate, tmpConfig.minSampleRate);
    EXPECT_EQ(config.maxSampleRate, tmpConfig.maxSampleRate);
    EXPECT_EQ(config.configArray, tmpConfig.configArray);

    EXPECT_EQ(config.areaConfigs.size(), tmpConfig.areaConfigs.size());

    auto cfgType = getPropType(config.prop);
    for (size_t idx = 0; idx < std::min(config.areaConfigs.size(), tmpConfig.areaConfigs.size());
         ++idx) {
        auto& lhs = config.areaConfigs[idx];
        auto& rhs = tmpConfig.areaConfigs[idx];
        EXPECT_EQ(lhs.areaId, rhs.areaId);
        switch (cfgType) {
            case VehiclePropertyType::INT64:
                EXPECT_EQ(lhs.minInt64Value, rhs.minInt64Value);
                EXPECT_EQ(lhs.maxInt64Value, rhs.maxInt64Value);
                break;
            case VehiclePropertyType::FLOAT:
                EXPECT_EQ(lhs.minFloatValue, rhs.minFloatValue);
                EXPECT_EQ(lhs.maxFloatValue, rhs.maxFloatValue);
                break;
            case VehiclePropertyType::INT32:
                EXPECT_EQ(lhs.minInt32Value, rhs.minInt32Value);
                EXPECT_EQ(lhs.maxInt32Value, rhs.maxInt32Value);
                break;
            default:
                // ignore min/max values
                break;
        }
    }
}

void CheckPropValueConversion(const VehiclePropValue& val) {
    vhal_proto::VehiclePropValue protoVal;
    VehiclePropValue tmpVal;

    toProto(&protoVal, val);
    fromProto(&tmpVal, protoVal);

    EXPECT_EQ(val, tmpVal);
}

TEST(ProtoMessageConverterTest, basic) {
    for (auto& property : impl::kVehicleProperties) {
        CheckPropConfigConversion(property.config);

        VehiclePropValue prop;
        prop.timestamp = elapsedRealtimeNano();
        prop.areaId = 123;
        prop.prop = property.config.prop;
        prop.value = property.initialValue;
        prop.status = VehiclePropertyStatus::ERROR;
        CheckPropValueConversion(prop);
    }
}

}  // namespace

}  // namespace proto_msg_converter
}  // namespace impl
}  // namespace V2_0
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
