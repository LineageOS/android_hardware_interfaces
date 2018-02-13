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

#include <android/hardware/automotive/vehicle/2.0/IVehicle.h>
#include <gtest/gtest.h>

#include "VehicleHalTestUtils.h"
#include "vhal_v2_0/VmsUtils.h"

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace V2_0 {
namespace vms {

namespace {

TEST(VmsUtilsTest, subscribeMessage) {
    VmsLayer layer(1, 0, 2);
    auto message = createSubscribeMessage(layer);
    ASSERT_NE(message, nullptr);
    EXPECT_TRUE(isValidVmsMessage(*message));
    EXPECT_EQ(message->prop, toInt(VehicleProperty::VEHICLE_MAP_SERVICE));
    EXPECT_EQ(message->value.int32Values.size(), 0x4ul);
    EXPECT_EQ(parseMessageType(*message), VmsMessageType::SUBSCRIBE);

    // Layer
    EXPECT_EQ(message->value.int32Values[1], 1);
    EXPECT_EQ(message->value.int32Values[2], 0);
    EXPECT_EQ(message->value.int32Values[3], 2);
}

TEST(VmsUtilsTest, unsubscribeMessage) {
    VmsLayer layer(1, 0, 2);
    auto message = createUnsubscribeMessage(layer);
    ASSERT_NE(message, nullptr);
    EXPECT_TRUE(isValidVmsMessage(*message));
    EXPECT_EQ(message->prop, toInt(VehicleProperty::VEHICLE_MAP_SERVICE));
    EXPECT_EQ(message->value.int32Values.size(), 0x4ul);
    EXPECT_EQ(parseMessageType(*message), VmsMessageType::UNSUBSCRIBE);

    // Layer
    EXPECT_EQ(message->value.int32Values[1], 1);
    EXPECT_EQ(message->value.int32Values[2], 0);
    EXPECT_EQ(message->value.int32Values[3], 2);
}

TEST(VmsUtilsTest, singleOfferingMessage) {
    std::vector<VmsLayerOffering> offering = {VmsLayerOffering(VmsLayer(1, 0, 2))};
    auto message = createOfferingMessage(offering);
    ASSERT_NE(message, nullptr);
    EXPECT_TRUE(isValidVmsMessage(*message));
    EXPECT_EQ(message->prop, toInt(VehicleProperty::VEHICLE_MAP_SERVICE));
    EXPECT_EQ(message->value.int32Values.size(), 0x6ul);
    EXPECT_EQ(parseMessageType(*message), VmsMessageType::OFFERING);

    // Number of layer offerings
    EXPECT_EQ(message->value.int32Values[1], 1);

    // Layer
    EXPECT_EQ(message->value.int32Values[2], 1);
    EXPECT_EQ(message->value.int32Values[3], 0);
    EXPECT_EQ(message->value.int32Values[4], 2);

    // Number of dependencies
    EXPECT_EQ(message->value.int32Values[5], 0);
}

TEST(VmsUtilsTest, offeringWithDependencies) {
    VmsLayer layer(1, 0, 2);
    std::vector<VmsLayer> dependencies = {VmsLayer(2, 0, 2)};
    std::vector<VmsLayerOffering> offering = {VmsLayerOffering(layer, dependencies)};
    auto message = createOfferingMessage(offering);
    ASSERT_NE(message, nullptr);
    EXPECT_TRUE(isValidVmsMessage(*message));
    EXPECT_EQ(message->prop, toInt(VehicleProperty::VEHICLE_MAP_SERVICE));
    EXPECT_EQ(message->value.int32Values.size(), 0x9ul);
    EXPECT_EQ(parseMessageType(*message), VmsMessageType::OFFERING);

    // Number of layer offerings
    EXPECT_EQ(message->value.int32Values[1], 1);

    // Layer
    EXPECT_EQ(message->value.int32Values[2], 1);
    EXPECT_EQ(message->value.int32Values[3], 0);
    EXPECT_EQ(message->value.int32Values[4], 2);

    // Number of dependencies
    EXPECT_EQ(message->value.int32Values[5], 1);

    // Dependency 1
    EXPECT_EQ(message->value.int32Values[6], 2);
    EXPECT_EQ(message->value.int32Values[7], 0);
    EXPECT_EQ(message->value.int32Values[8], 2);
}

TEST(VmsUtilsTest, availabilityMessage) {
    auto message = createAvailabilityRequest();
    ASSERT_NE(message, nullptr);
    EXPECT_TRUE(isValidVmsMessage(*message));
    EXPECT_EQ(message->prop, toInt(VehicleProperty::VEHICLE_MAP_SERVICE));
    EXPECT_EQ(message->value.int32Values.size(), 0x1ul);
    EXPECT_EQ(parseMessageType(*message), VmsMessageType::AVAILABILITY_REQUEST);
}

TEST(VmsUtilsTest, subscriptionsMessage) {
    auto message = createSubscriptionsRequest();
    ASSERT_NE(message, nullptr);
    EXPECT_TRUE(isValidVmsMessage(*message));
    EXPECT_EQ(message->prop, toInt(VehicleProperty::VEHICLE_MAP_SERVICE));
    EXPECT_EQ(message->value.int32Values.size(), 0x1ul);
    EXPECT_EQ(parseMessageType(*message), VmsMessageType::SUBSCRIPTIONS_REQUEST);
}

TEST(VmsUtilsTest, dataMessage) {
    std::string bytes = "aaa";
    auto message = createDataMessage(bytes);
    ASSERT_NE(message, nullptr);
    EXPECT_TRUE(isValidVmsMessage(*message));
    EXPECT_EQ(message->prop, toInt(VehicleProperty::VEHICLE_MAP_SERVICE));
    EXPECT_EQ(message->value.int32Values.size(), 0x1ul);
    EXPECT_EQ(parseMessageType(*message), VmsMessageType::DATA);
    EXPECT_EQ(message->value.bytes.size(), bytes.size());
    EXPECT_EQ(memcmp(message->value.bytes.data(), bytes.data(), bytes.size()), 0);
}

TEST(VmsUtilsTest, emptyMessageInvalid) {
    VehiclePropValue empty_prop;
    EXPECT_FALSE(isValidVmsMessage(empty_prop));
}

TEST(VmsUtilsTest, invalidMessageType) {
    VmsLayer layer(1, 0, 2);
    auto message = createSubscribeMessage(layer);
    message->value.int32Values[0] = 0;

    EXPECT_FALSE(isValidVmsMessage(*message));
}

TEST(VmsUtilsTest, parseDataMessage) {
    std::string bytes = "aaa";
    auto message = createDataMessage(bytes);
    auto data_str = parseData(*message);
    ASSERT_FALSE(data_str.empty());
    EXPECT_EQ(data_str, bytes);
}

TEST(VmsUtilsTest, parseInvalidDataMessage) {
    VmsLayer layer(1, 0, 2);
    auto message = createSubscribeMessage(layer);
    auto data_str = parseData(*message);
    EXPECT_TRUE(data_str.empty());
}

}  // namespace

}  // namespace vms
}  // namespace V2_0
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
