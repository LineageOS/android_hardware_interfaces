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
    VmsOffers offers = {123, {VmsLayerOffering(VmsLayer(1, 0, 2))}};
    auto message = createOfferingMessage(offers);
    ASSERT_NE(message, nullptr);
    EXPECT_TRUE(isValidVmsMessage(*message));
    EXPECT_EQ(message->prop, toInt(VehicleProperty::VEHICLE_MAP_SERVICE));
    EXPECT_EQ(message->value.int32Values.size(), 0x7ul);
    EXPECT_EQ(parseMessageType(*message), VmsMessageType::OFFERING);

    // Publisher ID
    EXPECT_EQ(message->value.int32Values[1], 123);

    // Number of layer offerings
    EXPECT_EQ(message->value.int32Values[2], 1);

    // Layer
    EXPECT_EQ(message->value.int32Values[3], 1);
    EXPECT_EQ(message->value.int32Values[4], 0);
    EXPECT_EQ(message->value.int32Values[5], 2);

    // Number of dependencies
    EXPECT_EQ(message->value.int32Values[6], 0);
}

TEST(VmsUtilsTest, offeringWithDependencies) {
    VmsLayer layer(1, 0, 2);
    std::vector<VmsLayer> dependencies = {VmsLayer(2, 0, 2), VmsLayer(3, 0, 3)};
    std::vector<VmsLayerOffering> offering = {VmsLayerOffering(layer, dependencies)};
    VmsOffers offers = {123, offering};
    auto message = createOfferingMessage(offers);
    ASSERT_NE(message, nullptr);
    EXPECT_TRUE(isValidVmsMessage(*message));
    EXPECT_EQ(message->prop, toInt(VehicleProperty::VEHICLE_MAP_SERVICE));
    EXPECT_EQ(message->value.int32Values.size(), 0xdul);
    EXPECT_EQ(parseMessageType(*message), VmsMessageType::OFFERING);

    // Publisher ID
    EXPECT_EQ(message->value.int32Values[1], 123);

    // Number of layer offerings
    EXPECT_EQ(message->value.int32Values[2], 1);

    // Layer
    EXPECT_EQ(message->value.int32Values[3], 1);
    EXPECT_EQ(message->value.int32Values[4], 0);
    EXPECT_EQ(message->value.int32Values[5], 2);

    // Number of dependencies
    EXPECT_EQ(message->value.int32Values[6], 2);

    // Dependency 1
    EXPECT_EQ(message->value.int32Values[7], 2);
    EXPECT_EQ(message->value.int32Values[8], 0);
    EXPECT_EQ(message->value.int32Values[9], 2);

    // Dependency 2
    EXPECT_EQ(message->value.int32Values[10], 3);
    EXPECT_EQ(message->value.int32Values[11], 0);
    EXPECT_EQ(message->value.int32Values[12], 3);
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
    const std::string bytes = "aaa";
    const VmsLayerAndPublisher layer_and_publisher(VmsLayer(2, 0, 1), 123);
    auto message = createDataMessageWithLayerPublisherInfo(layer_and_publisher, bytes);
    ASSERT_NE(message, nullptr);
    EXPECT_TRUE(isValidVmsMessage(*message));
    EXPECT_EQ(message->prop, toInt(VehicleProperty::VEHICLE_MAP_SERVICE));
    EXPECT_EQ(message->value.int32Values.size(), 0x5ul);
    EXPECT_EQ(message->value.int32Values[0], toInt(VmsMessageType::DATA));

    // Layer
    EXPECT_EQ(message->value.int32Values[1], 2);
    EXPECT_EQ(message->value.int32Values[2], 0);
    EXPECT_EQ(message->value.int32Values[3], 1);

    // Publisher ID
    EXPECT_EQ(message->value.int32Values[4], 123);

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
    message->value.int32Values[0] = -1;

    EXPECT_FALSE(isValidVmsMessage(*message));
}

TEST(VmsUtilsTest, parseDataMessage) {
    const std::string bytes = "aaa";
    const VmsLayerAndPublisher layer_and_publisher(VmsLayer(1, 0, 1), 123);
    auto message = createDataMessageWithLayerPublisherInfo(layer_and_publisher, bytes);
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

TEST(VmsUtilsTest, publisherIdRequest) {
    std::string bytes = "pub_id";
    auto message = createPublisherIdRequest(bytes);
    ASSERT_NE(message, nullptr);
    EXPECT_TRUE(isValidVmsMessage(*message));
    EXPECT_EQ(message->prop, toInt(VehicleProperty::VEHICLE_MAP_SERVICE));
    EXPECT_EQ(message->value.int32Values.size(), 0x1ul);
    EXPECT_EQ(parseMessageType(*message), VmsMessageType::PUBLISHER_ID_REQUEST);
    EXPECT_EQ(message->value.bytes.size(), bytes.size());
    EXPECT_EQ(memcmp(message->value.bytes.data(), bytes.data(), bytes.size()), 0);
}

TEST(VmsUtilsTest, validPublisherIdResponse) {
    auto message = createBaseVmsMessage(2);
    message->value.int32Values =
            hidl_vec<int32_t>{toInt(VmsMessageType::PUBLISHER_ID_RESPONSE), 1234};
    EXPECT_EQ(parsePublisherIdResponse(*message), 1234);
}

TEST(VmsUtilsTest, invalidPublisherIdResponse) {
    auto message = createBaseVmsMessage(1);
    EXPECT_EQ(parsePublisherIdResponse(*message), -1);
}

TEST(VmsUtilsTest, validSequenceNumberForSubscriptionsChange) {
    auto message = createBaseVmsMessage(2);
    message->value.int32Values =
            hidl_vec<int32_t>{toInt(VmsMessageType::SUBSCRIPTIONS_CHANGE), 1234};
    EXPECT_EQ(getSequenceNumberForSubscriptionsState(*message), 1234);
}

TEST(VmsUtilsTest, validSequenceNumberForSubscriptionsResponse) {
    auto message = createBaseVmsMessage(2);
    message->value.int32Values =
            hidl_vec<int32_t>{toInt(VmsMessageType::SUBSCRIPTIONS_RESPONSE), 1234};
    EXPECT_EQ(getSequenceNumberForSubscriptionsState(*message), 1234);
}

TEST(VmsUtilsTest, invalidSubscriptionsState) {
    auto message = createBaseVmsMessage(1);
    EXPECT_EQ(getSequenceNumberForSubscriptionsState(*message), -1);
}

TEST(VmsUtilsTest, newSequenceNumberForExistingSmallerNumberForChange) {
    auto message = createBaseVmsMessage(2);
    message->value.int32Values =
            hidl_vec<int32_t>{toInt(VmsMessageType::SUBSCRIPTIONS_CHANGE), 1234};
    EXPECT_TRUE(isSequenceNumberNewer(*message, 1233));
}

TEST(VmsUtilsTest, newSequenceNumberForExistingSmallerNumberForResponse) {
    auto message = createBaseVmsMessage(2);
    message->value.int32Values =
            hidl_vec<int32_t>{toInt(VmsMessageType::SUBSCRIPTIONS_RESPONSE), 1234};
    EXPECT_TRUE(isSequenceNumberNewer(*message, 1233));
}

TEST(VmsUtilsTest, newSequenceNumberForExistingGreaterNumberForChange) {
    auto message = createBaseVmsMessage(2);
    message->value.int32Values =
            hidl_vec<int32_t>{toInt(VmsMessageType::SUBSCRIPTIONS_CHANGE), 1234};
    EXPECT_FALSE(isSequenceNumberNewer(*message, 1235));
}

TEST(VmsUtilsTest, newSequenceNumberForExistingGreaterNumberForResponse) {
    auto message = createBaseVmsMessage(2);
    message->value.int32Values =
            hidl_vec<int32_t>{toInt(VmsMessageType::SUBSCRIPTIONS_RESPONSE), 1234};
    EXPECT_FALSE(isSequenceNumberNewer(*message, 1235));
}

TEST(VmsUtilsTest, newSequenceNumberForSameNumberForChange) {
    auto message = createBaseVmsMessage(2);
    message->value.int32Values =
            hidl_vec<int32_t>{toInt(VmsMessageType::SUBSCRIPTIONS_CHANGE), 1234};
    EXPECT_FALSE(isSequenceNumberNewer(*message, 1234));
}

TEST(VmsUtilsTest, newSequenceNumberForSameNumberForResponse) {
    auto message = createBaseVmsMessage(2);
    message->value.int32Values =
            hidl_vec<int32_t>{toInt(VmsMessageType::SUBSCRIPTIONS_RESPONSE), 1234};
    EXPECT_FALSE(isSequenceNumberNewer(*message, 1234));
}

void testSubscribedLayers(VmsMessageType type) {
    VmsOffers offers = {123,
                        {VmsLayerOffering(VmsLayer(1, 0, 1), {VmsLayer(4, 1, 1)}),
                         VmsLayerOffering(VmsLayer(2, 0, 1))}};
    auto message = createBaseVmsMessage(2);
    message->value.int32Values = hidl_vec<int32_t>{toInt(type),
                                                   1234,  // sequence number
                                                   2,     // number of layers
                                                   1,     // number of associated layers
                                                   1,     // layer 1
                                                   0,           1,
                                                   4,  // layer 2
                                                   1,           1,
                                                   2,  // associated layer
                                                   0,           1,
                                                   2,    // number of publisher IDs
                                                   111,  // publisher IDs
                                                   123};
    EXPECT_TRUE(isValidVmsMessage(*message));
    auto result = getSubscribedLayers(*message, offers);
    EXPECT_EQ(static_cast<int>(result.size()), 2);
    EXPECT_EQ(result.at(0), VmsLayer(1, 0, 1));
    EXPECT_EQ(result.at(1), VmsLayer(2, 0, 1));
}

TEST(VmsUtilsTest, subscribedLayersForChange) {
    testSubscribedLayers(VmsMessageType::SUBSCRIPTIONS_CHANGE);
}

TEST(VmsUtilsTest, subscribedLayersForResponse) {
    testSubscribedLayers(VmsMessageType::SUBSCRIPTIONS_RESPONSE);
}

void testSubscribedLayersWithDifferentSubtype(VmsMessageType type) {
    VmsOffers offers = {123, {VmsLayerOffering(VmsLayer(1, 0, 1))}};
    auto message = createBaseVmsMessage(2);
    message->value.int32Values = hidl_vec<int32_t>{toInt(type),
                                                   1234,  // sequence number
                                                   1,     // number of layers
                                                   0,     // number of associated layers
                                                   1,     // layer 1
                                                   1,     // different subtype
                                                   1};
    EXPECT_TRUE(isValidVmsMessage(*message));
    EXPECT_TRUE(getSubscribedLayers(*message, offers).empty());
}

TEST(VmsUtilsTest, subscribedLayersWithDifferentSubtypeForChange) {
    testSubscribedLayersWithDifferentSubtype(VmsMessageType::SUBSCRIPTIONS_CHANGE);
}

TEST(VmsUtilsTest, subscribedLayersWithDifferentSubtypeForResponse) {
    testSubscribedLayersWithDifferentSubtype(VmsMessageType::SUBSCRIPTIONS_RESPONSE);
}

void subscribedLayersWithDifferentVersion(VmsMessageType type) {
    VmsOffers offers = {123, {VmsLayerOffering(VmsLayer(1, 0, 1))}};
    auto message = createBaseVmsMessage(2);
    message->value.int32Values = hidl_vec<int32_t>{toInt(type),
                                                   1234,             // sequence number
                                                   1,                // number of layers
                                                   0,                // number of associated layers
                                                   1,                // layer 1
                                                   0,           2};  // different version
    EXPECT_TRUE(isValidVmsMessage(*message));
    EXPECT_TRUE(getSubscribedLayers(*message, offers).empty());
}

TEST(VmsUtilsTest, subscribedLayersWithDifferentVersionForChange) {
    subscribedLayersWithDifferentVersion(VmsMessageType::SUBSCRIPTIONS_CHANGE);
}

TEST(VmsUtilsTest, subscribedLayersWithDifferentVersionForResponse) {
    subscribedLayersWithDifferentVersion(VmsMessageType::SUBSCRIPTIONS_RESPONSE);
}

void subscribedLayersWithDifferentPublisherId(VmsMessageType type) {
    VmsOffers offers = {123, {VmsLayerOffering(VmsLayer(1, 0, 1))}};
    auto message = createBaseVmsMessage(2);
    message->value.int32Values = hidl_vec<int32_t>{toInt(type),
                                                   1234,  // sequence number
                                                   0,     // number of layers
                                                   1,     // number of associated layers
                                                   1,     // associated layer 1
                                                   0,           1,
                                                   1,     // number of publisher IDs
                                                   234};  // publisher ID 1
    EXPECT_TRUE(isValidVmsMessage(*message));
    EXPECT_TRUE(getSubscribedLayers(*message, offers).empty());
}

TEST(VmsUtilsTest, subscribedLayersWithDifferentPublisherIdForChange) {
    subscribedLayersWithDifferentPublisherId(VmsMessageType::SUBSCRIPTIONS_CHANGE);
}

TEST(VmsUtilsTest, subscribedLayersWithDifferentPublisherIdForResponse) {
    subscribedLayersWithDifferentPublisherId(VmsMessageType::SUBSCRIPTIONS_RESPONSE);
}

TEST(VmsUtilsTest, serviceNewlyStarted) {
    auto message = createBaseVmsMessage(2);
    message->value.int32Values = hidl_vec<int32_t>{toInt(VmsMessageType::AVAILABILITY_CHANGE), 0};
    EXPECT_TRUE(hasServiceNewlyStarted(*message));
}

TEST(VmsUtilsTest, serviceNotNewlyStarted) {
    auto message = createBaseVmsMessage(2);
    message->value.int32Values =
            hidl_vec<int32_t>{toInt(VmsMessageType::AVAILABILITY_CHANGE), 1234};
    EXPECT_FALSE(hasServiceNewlyStarted(*message));
}

TEST(VmsUtilsTest, invalidAvailabilityChange) {
    auto message = createBaseVmsMessage(1);
    EXPECT_FALSE(hasServiceNewlyStarted(*message));
}

TEST(VmsUtilsTest, startSessionRequest) {
    auto message = createStartSessionMessage(123, 456);
    ASSERT_NE(message, nullptr);
    EXPECT_TRUE(isValidVmsMessage(*message));
    EXPECT_EQ(message->prop, toInt(VehicleProperty::VEHICLE_MAP_SERVICE));
    EXPECT_EQ(message->value.int32Values.size(), 0x3ul);
    EXPECT_EQ(parseMessageType(*message), VmsMessageType::START_SESSION);
    EXPECT_EQ(message->value.int32Values[1], 123);
    EXPECT_EQ(message->value.int32Values[2], 456);
}

TEST(VmsUtilsTest, startSessionServiceNewlyStarted) {
    auto message = createBaseVmsMessage(3);
    int new_service_id;
    message->value.int32Values = hidl_vec<int32_t>{toInt(VmsMessageType::START_SESSION), 123, -1};
    EXPECT_EQ(parseStartSessionMessage(*message, 122, 456, &new_service_id),
              VmsSessionStatus::kNewServerSession);
    EXPECT_EQ(new_service_id, 123);
}

TEST(VmsUtilsTest, startSessionServiceNewlyStartedEdgeCase) {
    auto message = createBaseVmsMessage(3);
    int new_service_id;
    message->value.int32Values = hidl_vec<int32_t>{toInt(VmsMessageType::START_SESSION), 0, -1};
    EXPECT_EQ(parseStartSessionMessage(*message, -1, 0, &new_service_id),
              VmsSessionStatus::kNewServerSession);
    EXPECT_EQ(new_service_id, 0);
}

TEST(VmsUtilsTest, startSessionClientNewlyStarted) {
    auto message = createBaseVmsMessage(3);
    int new_service_id;
    message->value.int32Values = hidl_vec<int32_t>{toInt(VmsMessageType::START_SESSION), 123, 456};
    EXPECT_EQ(parseStartSessionMessage(*message, -1, 456, &new_service_id),
              VmsSessionStatus::kAckToCurrentSession);
    EXPECT_EQ(new_service_id, 123);
}

TEST(VmsUtilsTest, startSessionClientNewlyStartedWithSameServerAndClientId) {
    auto message = createBaseVmsMessage(3);
    int new_service_id;
    message->value.int32Values = hidl_vec<int32_t>{toInt(VmsMessageType::START_SESSION), 123, 456};
    EXPECT_EQ(parseStartSessionMessage(*message, 123, 456, &new_service_id),
              VmsSessionStatus::kAckToCurrentSession);
    EXPECT_EQ(new_service_id, 123);
}

TEST(VmsUtilsTest, startSessionWithZeroAsIds) {
    auto message = createBaseVmsMessage(3);
    int new_service_id;
    message->value.int32Values = hidl_vec<int32_t>{toInt(VmsMessageType::START_SESSION), 0, 0};
    EXPECT_EQ(parseStartSessionMessage(*message, 0, 0, &new_service_id),
              VmsSessionStatus::kAckToCurrentSession);
    EXPECT_EQ(new_service_id, 0);
}

TEST(VmsUtilsTest, startSessionOldServiceId) {
    auto message = createBaseVmsMessage(3);
    int new_service_id;
    message->value.int32Values = hidl_vec<int32_t>{toInt(VmsMessageType::START_SESSION), 120, 456};
    EXPECT_EQ(parseStartSessionMessage(*message, 123, 456, &new_service_id),
              VmsSessionStatus::kAckToCurrentSession);
    EXPECT_EQ(new_service_id, 120);
}

TEST(VmsUtilsTest, startSessionNegativeServerId) {
    auto message = createBaseVmsMessage(3);
    int new_service_id;
    message->value.int32Values = hidl_vec<int32_t>{toInt(VmsMessageType::START_SESSION), -1, 456};
    EXPECT_EQ(parseStartSessionMessage(*message, -1, 456, &new_service_id),
              VmsSessionStatus::kAckToCurrentSession);
    EXPECT_EQ(new_service_id, -1);
}

TEST(VmsUtilsTest, startSessionInvalidMessageFormat) {
    auto message = createBaseVmsMessage(2);
    int new_service_id;
    message->value.int32Values = hidl_vec<int32_t>{toInt(VmsMessageType::START_SESSION), 123};
    EXPECT_EQ(parseStartSessionMessage(*message, 123, 456, &new_service_id),
              VmsSessionStatus::kInvalidMessage);
    EXPECT_EQ(new_service_id, 123);
}

}  // namespace

}  // namespace vms
}  // namespace V2_0
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
