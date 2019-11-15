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
static constexpr int kPublisherIdSize = 1;
static constexpr int kLayerNumberSize = 1;
static constexpr int kLayerSize = 3;
static constexpr int kLayerAndPublisherSize = 4;
static constexpr int kSessionIdsSize = 2;
static constexpr int kPublisherIdIndex =
        toInt(VmsPublisherInformationIntegerValuesIndex::PUBLISHER_ID);
static constexpr int kSubscriptionStateSequenceNumberIndex =
        toInt(VmsSubscriptionsStateIntegerValuesIndex::SEQUENCE_NUMBER);
static constexpr int kAvailabilitySequenceNumberIndex =
        toInt(VmsAvailabilityStateIntegerValuesIndex::SEQUENCE_NUMBER);

// TODO(aditin): We should extend the VmsMessageType enum to include a first and
// last, which would prevent breakages in this API. However, for all of the
// functions in this module, we only need to guarantee that the message type is
// between SUBSCRIBE and START_SESSION.
static constexpr int kFirstMessageType = toInt(VmsMessageType::SUBSCRIBE);
static constexpr int kLastMessageType = toInt(VmsMessageType::START_SESSION);

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

std::unique_ptr<VehiclePropValue> createOfferingMessage(const VmsOffers& offers) {
    int message_size = kMessageTypeSize + kPublisherIdSize + kLayerNumberSize;
    for (const auto& offer : offers.offerings) {
        message_size += kLayerSize + kLayerNumberSize + (offer.dependencies.size() * kLayerSize);
    }
    auto result = createBaseVmsMessage(message_size);

    std::vector<int32_t> offerings = {toInt(VmsMessageType::OFFERING), offers.publisher_id,
                                      static_cast<int>(offers.offerings.size())};
    for (const auto& offer : offers.offerings) {
        std::vector<int32_t> layer_vector = {offer.layer.type, offer.layer.subtype,
                                             offer.layer.version,
                                             static_cast<int32_t>(offer.dependencies.size())};
        for (const auto& dependency : offer.dependencies) {
            std::vector<int32_t> dependency_layer = {dependency.type, dependency.subtype,
                                                     dependency.version};
            layer_vector.insert(layer_vector.end(), dependency_layer.begin(),
                                dependency_layer.end());
        }
        offerings.insert(offerings.end(), layer_vector.begin(), layer_vector.end());
    }
    result->value.int32Values = offerings;
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

std::unique_ptr<VehiclePropValue> createDataMessageWithLayerPublisherInfo(
        const VmsLayerAndPublisher& layer_publisher, const std::string& vms_packet) {
    auto result = createBaseVmsMessage(kMessageTypeSize + kLayerAndPublisherSize);
    result->value.int32Values = hidl_vec<int32_t>{
            toInt(VmsMessageType::DATA), layer_publisher.layer.type, layer_publisher.layer.subtype,
            layer_publisher.layer.version, layer_publisher.publisher_id};
    result->value.bytes = std::vector<uint8_t>(vms_packet.begin(), vms_packet.end());
    return result;
}

std::unique_ptr<VehiclePropValue> createPublisherIdRequest(
        const std::string& vms_provider_description) {
    auto result = createBaseVmsMessage(kMessageTypeSize);
    result->value.int32Values = hidl_vec<int32_t>{
            toInt(VmsMessageType::PUBLISHER_ID_REQUEST),
    };
    result->value.bytes =
            std::vector<uint8_t>(vms_provider_description.begin(), vms_provider_description.end());
    return result;
}

std::unique_ptr<VehiclePropValue> createStartSessionMessage(const int service_id,
                                                            const int client_id) {
    auto result = createBaseVmsMessage(kMessageTypeSize + kSessionIdsSize);
    result->value.int32Values = hidl_vec<int32_t>{
            toInt(VmsMessageType::START_SESSION),
            service_id,
            client_id,
    };
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

int32_t parsePublisherIdResponse(const VehiclePropValue& publisher_id_response) {
    if (isValidVmsMessage(publisher_id_response) &&
        parseMessageType(publisher_id_response) == VmsMessageType::PUBLISHER_ID_RESPONSE &&
        publisher_id_response.value.int32Values.size() > kPublisherIdIndex) {
        return publisher_id_response.value.int32Values[kPublisherIdIndex];
    }
    return -1;
}

bool isSequenceNumberNewer(const VehiclePropValue& subscriptions_state,
                           const int last_seen_sequence_number) {
    return (isValidVmsMessage(subscriptions_state) &&
            (parseMessageType(subscriptions_state) == VmsMessageType::SUBSCRIPTIONS_CHANGE ||
             parseMessageType(subscriptions_state) == VmsMessageType::SUBSCRIPTIONS_RESPONSE) &&
            subscriptions_state.value.int32Values.size() > kSubscriptionStateSequenceNumberIndex &&
            subscriptions_state.value.int32Values[kSubscriptionStateSequenceNumberIndex] >
                    last_seen_sequence_number);
}

int32_t getSequenceNumberForSubscriptionsState(const VehiclePropValue& subscriptions_state) {
    if (isValidVmsMessage(subscriptions_state) &&
        (parseMessageType(subscriptions_state) == VmsMessageType::SUBSCRIPTIONS_CHANGE ||
         parseMessageType(subscriptions_state) == VmsMessageType::SUBSCRIPTIONS_RESPONSE) &&
        subscriptions_state.value.int32Values.size() > kSubscriptionStateSequenceNumberIndex) {
        return subscriptions_state.value.int32Values[kSubscriptionStateSequenceNumberIndex];
    }
    return -1;
}

std::vector<VmsLayer> getSubscribedLayers(const VehiclePropValue& subscriptions_state,
                                          const VmsOffers& offers) {
    if (isValidVmsMessage(subscriptions_state) &&
        (parseMessageType(subscriptions_state) == VmsMessageType::SUBSCRIPTIONS_CHANGE ||
         parseMessageType(subscriptions_state) == VmsMessageType::SUBSCRIPTIONS_RESPONSE) &&
        subscriptions_state.value.int32Values.size() >
                toInt(VmsSubscriptionsStateIntegerValuesIndex::NUMBER_OF_LAYERS)) {
        int subscriptions_state_int_size = subscriptions_state.value.int32Values.size();
        std::unordered_set<VmsLayer, VmsLayer::VmsLayerHashFunction> offered_layers;
        for (const auto& offer : offers.offerings) {
            offered_layers.insert(offer.layer);
        }
        std::vector<VmsLayer> subscribed_layers;

        int current_index = toInt(VmsSubscriptionsStateIntegerValuesIndex::SUBSCRIPTIONS_START);

        // Add all subscribed layers which are offered by the current publisher.
        const int32_t num_of_layers = subscriptions_state.value.int32Values[toInt(
                VmsSubscriptionsStateIntegerValuesIndex::NUMBER_OF_LAYERS)];
        for (int i = 0; i < num_of_layers; i++) {
            if (subscriptions_state_int_size < current_index + kLayerSize) {
                return {};
            }
            VmsLayer layer = VmsLayer(subscriptions_state.value.int32Values[current_index],
                                      subscriptions_state.value.int32Values[current_index + 1],
                                      subscriptions_state.value.int32Values[current_index + 2]);
            if (offered_layers.find(layer) != offered_layers.end()) {
                subscribed_layers.push_back(std::move(layer));
            }
            current_index += kLayerSize;
        }

        // Add all subscribed associated layers which are offered by the current publisher.
        // For this, we need to check if the associated layer has a publisher ID which is
        // same as that of the current publisher.
        if (subscriptions_state_int_size >
            toInt(VmsSubscriptionsStateIntegerValuesIndex::NUMBER_OF_ASSOCIATED_LAYERS)) {
            const int32_t num_of_associated_layers = subscriptions_state.value.int32Values[toInt(
                    VmsSubscriptionsStateIntegerValuesIndex::NUMBER_OF_ASSOCIATED_LAYERS)];

            for (int i = 0; i < num_of_associated_layers; i++) {
                if (subscriptions_state_int_size < current_index + kLayerSize) {
                    return {};
                }
                VmsLayer layer = VmsLayer(subscriptions_state.value.int32Values[current_index],
                                          subscriptions_state.value.int32Values[current_index + 1],
                                          subscriptions_state.value.int32Values[current_index + 2]);
                current_index += kLayerSize;
                if (offered_layers.find(layer) != offered_layers.end() &&
                    subscriptions_state_int_size > current_index) {
                    int32_t num_of_publisher_ids =
                            subscriptions_state.value.int32Values[current_index];
                    current_index++;
                    for (int j = 0; j < num_of_publisher_ids; j++) {
                        if (subscriptions_state_int_size > current_index &&
                            subscriptions_state.value.int32Values[current_index] ==
                                    offers.publisher_id) {
                            subscribed_layers.push_back(std::move(layer));
                        }
                        current_index++;
                    }
                }
            }
        }
        return subscribed_layers;
    }
    return {};
}

bool hasServiceNewlyStarted(const VehiclePropValue& availability_change) {
    return (isValidVmsMessage(availability_change) &&
            parseMessageType(availability_change) == VmsMessageType::AVAILABILITY_CHANGE &&
            availability_change.value.int32Values.size() > kAvailabilitySequenceNumberIndex &&
            availability_change.value.int32Values[kAvailabilitySequenceNumberIndex] == 0);
}

VmsSessionStatus parseStartSessionMessage(const VehiclePropValue& start_session,
                                          const int current_service_id, const int current_client_id,
                                          int* new_service_id) {
    if (isValidVmsMessage(start_session) &&
        parseMessageType(start_session) == VmsMessageType::START_SESSION &&
        start_session.value.int32Values.size() == kSessionIdsSize + 1) {
        *new_service_id = start_session.value.int32Values[1];
        const int new_client_id = start_session.value.int32Values[2];
        if (new_client_id != current_client_id) {
            // If the new_client_id = -1, it means the service has newly started.
            // But if it is not -1 and is different than the current client ID, then
            // it means that the service did not have the correct client ID. In
            // both these cases, the client should acknowledge with a START_SESSION
            // message containing the correct client ID. So here, the status is returned as
            // kNewServerSession.
            return VmsSessionStatus::kNewServerSession;
        } else {
            // kAckToCurrentSession is returned if the new client ID is same as the current one.
            return VmsSessionStatus::kAckToCurrentSession;
        }
    }
    // If the message is invalid then persist the old service ID.
    *new_service_id = current_service_id;
    return VmsSessionStatus::kInvalidMessage;
}

bool isAvailabilitySequenceNumberNewer(const VehiclePropValue& availability_state,
                                       const int last_seen_availability_sequence_number) {
    return (isValidVmsMessage(availability_state) &&
            (parseMessageType(availability_state) == VmsMessageType::AVAILABILITY_CHANGE ||
             parseMessageType(availability_state) == VmsMessageType::AVAILABILITY_RESPONSE) &&
            availability_state.value.int32Values.size() > kAvailabilitySequenceNumberIndex &&
            availability_state.value.int32Values[kAvailabilitySequenceNumberIndex] >
                    last_seen_availability_sequence_number);
}

int32_t getSequenceNumberForAvailabilityState(const VehiclePropValue& availability_state) {
    if (isValidVmsMessage(availability_state) &&
        (parseMessageType(availability_state) == VmsMessageType::AVAILABILITY_CHANGE ||
         parseMessageType(availability_state) == VmsMessageType::AVAILABILITY_RESPONSE) &&
        availability_state.value.int32Values.size() > kAvailabilitySequenceNumberIndex) {
        return availability_state.value.int32Values[kAvailabilitySequenceNumberIndex];
    }
    return -1;
}

std::vector<VmsAssociatedLayer> getAvailableLayers(const VehiclePropValue& availability_state) {
    if (isValidVmsMessage(availability_state) &&
        (parseMessageType(availability_state) == VmsMessageType::AVAILABILITY_CHANGE ||
         parseMessageType(availability_state) == VmsMessageType::AVAILABILITY_RESPONSE) &&
        availability_state.value.int32Values.size() >
                toInt(VmsAvailabilityStateIntegerValuesIndex::NUMBER_OF_ASSOCIATED_LAYERS)) {
        int availability_state_int_size = availability_state.value.int32Values.size();
        const int32_t num_of_associated_layers = availability_state.value.int32Values[toInt(
                VmsAvailabilityStateIntegerValuesIndex::NUMBER_OF_ASSOCIATED_LAYERS)];
        int current_index = toInt(VmsAvailabilityStateIntegerValuesIndex::LAYERS_START);
        std::vector<VmsAssociatedLayer> available_layers;
        for (int i = 0; i < num_of_associated_layers; i++) {
            if (availability_state_int_size < current_index + kLayerSize) {
                return {};
            }
            VmsLayer layer = VmsLayer(availability_state.value.int32Values[current_index],
                                      availability_state.value.int32Values[current_index + 1],
                                      availability_state.value.int32Values[current_index + 2]);
            current_index += kLayerSize;
            std::vector<int> publisher_ids;
            if (availability_state_int_size > current_index) {
                int32_t num_of_publisher_ids = availability_state.value.int32Values[current_index];
                current_index++;
                for (int j = 0; j < num_of_publisher_ids; j++) {
                    if (availability_state_int_size > current_index) {
                        publisher_ids.push_back(
                                availability_state.value.int32Values[current_index]);
                        current_index++;
                    }
                }
            }
            available_layers.emplace_back(layer, std::move(publisher_ids));
        }
        return available_layers;
    }
    return {};
}

}  // namespace vms
}  // namespace V2_0
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
