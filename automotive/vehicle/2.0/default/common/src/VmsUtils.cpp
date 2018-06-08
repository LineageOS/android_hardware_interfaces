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

#include "VmsUtils.h"

#include <common/include/vhal_v2_0/VehicleUtils.h>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace V2_0 {
namespace vms {

static constexpr int kMessageIndex = toInt(VmsBaseMessageIntegerValuesIndex::MESSAGE_TYPE);
static constexpr int kMessageTypeSize = 1;
static constexpr int kLayerNumberSize = 1;
static constexpr int kLayerSize = 3;
static constexpr int kLayerAndPublisherSize = 4;

// TODO(aditin): We should extend the VmsMessageType enum to include a first and
// last, which would prevent breakages in this API. However, for all of the
// functions in this module, we only need to guarantee that the message type is
// between SUBSCRIBE and DATA.
static constexpr int kFirstMessageType = toInt(VmsMessageType::SUBSCRIBE);
static constexpr int kLastMessageType = toInt(VmsMessageType::DATA);

std::unique_ptr<VehiclePropValue> createBaseVmsMessage(size_t message_size) {
    auto result = createVehiclePropValue(VehiclePropertyType::INT32, message_size);
    result->prop = toInt(VehicleProperty::VEHICLE_MAP_SERVICE);
    result->areaId = toInt(VehicleArea::GLOBAL);
    return result;
}

std::unique_ptr<VehiclePropValue> createSubscribeMessage(const VmsLayer& layer) {
    auto result = createBaseVmsMessage(kMessageTypeSize + kLayerSize);
    result->value.int32Values = hidl_vec<int32_t>{toInt(VmsMessageType::SUBSCRIBE), layer.type,
                                                  layer.subtype, layer.version};
    return result;
}

std::unique_ptr<VehiclePropValue> createSubscribeToPublisherMessage(
    const VmsLayerAndPublisher& layer_publisher) {
    auto result = createBaseVmsMessage(kMessageTypeSize + kLayerAndPublisherSize);
    result->value.int32Values = hidl_vec<int32_t>{
        toInt(VmsMessageType::SUBSCRIBE_TO_PUBLISHER), layer_publisher.layer.type,
        layer_publisher.layer.subtype, layer_publisher.layer.version, layer_publisher.publisher_id};
    return result;
}

std::unique_ptr<VehiclePropValue> createUnsubscribeMessage(const VmsLayer& layer) {
    auto result = createBaseVmsMessage(kMessageTypeSize + kLayerSize);
    result->value.int32Values = hidl_vec<int32_t>{toInt(VmsMessageType::UNSUBSCRIBE), layer.type,
                                                  layer.subtype, layer.version};
    return result;
}

std::unique_ptr<VehiclePropValue> createUnsubscribeToPublisherMessage(
    const VmsLayerAndPublisher& layer_publisher) {
    auto result = createBaseVmsMessage(kMessageTypeSize + kLayerAndPublisherSize);
    result->value.int32Values = hidl_vec<int32_t>{
        toInt(VmsMessageType::UNSUBSCRIBE_TO_PUBLISHER), layer_publisher.layer.type,
        layer_publisher.layer.subtype, layer_publisher.layer.version, layer_publisher.publisher_id};
    return result;
}

std::unique_ptr<VehiclePropValue> createOfferingMessage(
    const std::vector<VmsLayerOffering>& offering) {
    int message_size = kMessageTypeSize + kLayerNumberSize;
    for (const auto& offer : offering) {
        message_size += kLayerNumberSize + (1 + offer.dependencies.size()) * kLayerSize;
    }
    auto result = createBaseVmsMessage(message_size);

    std::vector<int32_t> offers = {toInt(VmsMessageType::OFFERING),
                                   static_cast<int>(offering.size())};
    for (const auto& offer : offering) {
        std::vector<int32_t> layer_vector = {offer.layer.type, offer.layer.subtype,
                                             offer.layer.version,
                                             static_cast<int32_t>(offer.dependencies.size())};
        for (const auto& dependency : offer.dependencies) {
            std::vector<int32_t> dependency_layer = {dependency.type, dependency.subtype,
                                                     dependency.version};
            layer_vector.insert(layer_vector.end(), dependency_layer.begin(),
                                dependency_layer.end());
        }
        offers.insert(offers.end(), layer_vector.begin(), layer_vector.end());
    }
    result->value.int32Values = offers;
    return result;
}

std::unique_ptr<VehiclePropValue> createAvailabilityRequest() {
    auto result = createBaseVmsMessage(kMessageTypeSize);
    result->value.int32Values = hidl_vec<int32_t>{
        toInt(VmsMessageType::AVAILABILITY_REQUEST),
    };
    return result;
}

std::unique_ptr<VehiclePropValue> createSubscriptionsRequest() {
    auto result = createBaseVmsMessage(kMessageTypeSize);
    result->value.int32Values = hidl_vec<int32_t>{
        toInt(VmsMessageType::SUBSCRIPTIONS_REQUEST),
    };
    return result;
}

std::unique_ptr<VehiclePropValue> createDataMessage(const std::string& bytes) {
    auto result = createBaseVmsMessage(kMessageTypeSize);
    result->value.int32Values = hidl_vec<int32_t>{toInt(VmsMessageType::DATA)};
    result->value.bytes = std::vector<uint8_t>(bytes.begin(), bytes.end());
    return result;
}

bool isValidVmsProperty(const VehiclePropValue& value) {
    return (value.prop == toInt(VehicleProperty::VEHICLE_MAP_SERVICE));
}

bool isValidVmsMessageType(const VehiclePropValue& value) {
    return (value.value.int32Values.size() > 0 &&
            value.value.int32Values[kMessageIndex] >= kFirstMessageType &&
            value.value.int32Values[kMessageIndex] <= kLastMessageType);
}

bool isValidVmsMessage(const VehiclePropValue& value) {
    return (isValidVmsProperty(value) && isValidVmsMessageType(value));
}

VmsMessageType parseMessageType(const VehiclePropValue& value) {
    return static_cast<VmsMessageType>(value.value.int32Values[kMessageIndex]);
}

std::string parseData(const VehiclePropValue& value) {
    if (isValidVmsMessage(value) && parseMessageType(value) == VmsMessageType::DATA &&
        value.value.bytes.size() > 0) {
        return std::string(value.value.bytes.begin(), value.value.bytes.end());
    } else {
        return std::string();
    }
}

}  // namespace vms
}  // namespace V2_0
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
