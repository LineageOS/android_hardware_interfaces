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

#ifndef android_hardware_automotive_vehicle_V2_0_VmsUtils_H_
#define android_hardware_automotive_vehicle_V2_0_VmsUtils_H_

#include <memory>
#include <string>
#include <unordered_set>

#include <android/hardware/automotive/vehicle/2.0/types.h>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace V2_0 {
namespace vms {

// VmsUtils are a set of abstractions for creating and parsing Vehicle Property
// updates to VehicleProperty::VEHICLE_MAP_SERVICE. The format for parsing a
// VehiclePropValue update with a VMS message is specified in the Vehicle HIDL.
//
// This interface is meant for use by HAL clients of VMS; corresponding
// functionality is also provided by VMS in the embedded car service.

// A VmsLayer is comprised of a type, subtype, and version.
struct VmsLayer {
    VmsLayer(int type, int subtype, int version) : type(type), subtype(subtype), version(version) {}
    int type;
    int subtype;
    int version;
    bool operator==(const VmsLayer& layer) const {
        return this->type == layer.type && this->subtype == layer.subtype &&
               this->version == layer.version;
    }

    // Class for hash function
    class VmsLayerHashFunction {
      public:
        // Hash of the variables is returned.
        size_t operator()(const VmsLayer& layer) const {
            return std::hash<int>()(layer.type) ^ std::hash<int>()(layer.type) ^
                   std::hash<int>()(layer.type);
        }
    };
};

struct VmsLayerAndPublisher {
    VmsLayerAndPublisher(VmsLayer layer, int publisher_id)
        : layer(std::move(layer)), publisher_id(publisher_id) {}
    VmsLayer layer;
    int publisher_id;
};

// A VmsAssociatedLayer is used by subscribers to specify which publisher IDs
// are acceptable for a given layer.
struct VmsAssociatedLayer {
    VmsAssociatedLayer(VmsLayer layer, std::vector<int> publisher_ids)
        : layer(std::move(layer)), publisher_ids(std::move(publisher_ids)) {}
    VmsLayer layer;
    std::vector<int> publisher_ids;
};

// A VmsLayerOffering refers to a single layer that can be published, along with
// its dependencies. Dependencies can be empty.
struct VmsLayerOffering {
    VmsLayerOffering(VmsLayer layer, std::vector<VmsLayer> dependencies)
        : layer(std::move(layer)), dependencies(std::move(dependencies)) {}
    VmsLayerOffering(VmsLayer layer) : layer(layer), dependencies() {}
    VmsLayer layer;
    std::vector<VmsLayer> dependencies;
};

// A VmsOffers refers to a list of layers that can be published by the publisher
// with the specified publisher ID.
struct VmsOffers {
    VmsOffers(int publisher_id, std::vector<VmsLayerOffering> offerings)
        : publisher_id(publisher_id), offerings(std::move(offerings)) {}
    int publisher_id;
    std::vector<VmsLayerOffering> offerings;
};

// A VmsSubscriptionsState is delivered in response to a
// VmsMessageType.SUBSCRIPTIONS_REQUEST or on the first SUBSCRIBE or last
// UNSUBSCRIBE for a layer. It indicates which layers or associated_layers are
// currently being subscribed to in the system.
struct VmsSubscriptionsState {
    int sequence_number;
    std::vector<VmsLayer> layers;
    std::vector<VmsAssociatedLayer> associated_layers;
};

struct VmsAvailabilityState {
    int sequence_number;
    std::vector<VmsAssociatedLayer> associated_layers;
};

// An enum to represent the result of parsing START_SESSION message from the VMS service.
enum VmsSessionStatus {
    // When a new session is received, the client should acknowledge it with the correct
    // IDs in the START_SESSION message.
    kNewServerSession,
    // When an acknowledgement it received, the client can start using the connection.
    kAckToCurrentSession,
    // Invalid message with either invalid format or unexpected data.
    kInvalidMessage
};

// Creates an empty base VMS message with some pre-populated default fields.
std::unique_ptr<VehiclePropValue> createBaseVmsMessage(size_t message_size);

// Creates a VehiclePropValue containing a message of type
// VmsMessageType.SUBSCRIBE, specifying to the VMS service
// which layer to subscribe to.
std::unique_ptr<VehiclePropValue> createSubscribeMessage(const VmsLayer& layer);

// Creates a VehiclePropValue containing a message of type
// VmsMessageType.SUBSCRIBE_TO_PUBLISHER, specifying to the VMS service
// which layer and publisher_id to subscribe to.
std::unique_ptr<VehiclePropValue> createSubscribeToPublisherMessage(
    const VmsLayerAndPublisher& layer);

// Creates a VehiclePropValue containing a message of type
// VmsMessageType.UNSUBSCRIBE, specifying to the VMS service
// which layer to unsubscribe from.
std::unique_ptr<VehiclePropValue> createUnsubscribeMessage(const VmsLayer& layer);

// Creates a VehiclePropValue containing a message of type
// VmsMessageType.UNSUBSCRIBE_TO_PUBLISHER, specifying to the VMS service
// which layer and publisher_id to unsubscribe from.
std::unique_ptr<VehiclePropValue> createUnsubscribeToPublisherMessage(
    const VmsLayerAndPublisher& layer);

// Creates a VehiclePropValue containing a message of type
// VmsMessageType.OFFERING, specifying to the VMS service which layers are being
// offered and their dependencies, if any.
std::unique_ptr<VehiclePropValue> createOfferingMessage(const VmsOffers& offers);

// Creates a VehiclePropValue containing a message of type
// VmsMessageType.AVAILABILITY_REQUEST.
std::unique_ptr<VehiclePropValue> createAvailabilityRequest();

// Creates a VehiclePropValue containing a message of type
// VmsMessageType.SUBSCRIPTIONS_REQUEST.
std::unique_ptr<VehiclePropValue> createSubscriptionsRequest();

// Creates a VehiclePropValue containing a message of type VmsMessageType.DATA.
// Returns a nullptr if the vms_packet string in bytes is empty or if the layer_publisher
// information in VmsLayerAndPublisher format is missing the later or publisher
// information.
//
// For example, to build a VehiclePropValue message containing a proto, the caller
// should first convert the proto to a byte string (vms_packet) using the
// SerializeToString proto API. Then, it use this interface to build the VehicleProperty
// by passing publisher and layer information (layer_publisher) and the vms_packet.
std::unique_ptr<VehiclePropValue> createDataMessageWithLayerPublisherInfo(
        const VmsLayerAndPublisher& layer_publisher, const std::string& vms_packet);

// Creates a VehiclePropValue containing a message of type
// VmsMessageType.PUBLISHER_ID_REQUEST with the given publisher information.
// Returns a nullptr if the input is empty.
std::unique_ptr<VehiclePropValue> createPublisherIdRequest(
        const std::string& vms_provider_description);

// Creates a VehiclePropValue message of type VmsMessageType.START_SESSION.
std::unique_ptr<VehiclePropValue> createStartSessionMessage(const int service_id,
                                                            const int client_id);

// Returns true if the VehiclePropValue pointed to by value contains a valid Vms
// message, i.e. the VehicleProperty, VehicleArea, and VmsMessageType are all
// valid. Note: If the VmsMessageType enum is extended, this function will
// return false for any new message types added.
bool isValidVmsMessage(const VehiclePropValue& value);

// Returns the message type. Expects that the VehiclePropValue contains a valid
// Vms message, as verified by isValidVmsMessage.
VmsMessageType parseMessageType(const VehiclePropValue& value);

// Constructs a string byte array from a message of type VmsMessageType.DATA.
// Returns an empty string if the message type doesn't match or if the
// VehiclePropValue does not contain a byte array.
//
// A proto message can then be constructed by passing the result of this
// function to ParseFromString.
std::string parseData(const VehiclePropValue& value);

// Returns the publisher ID by parsing the VehiclePropValue containing the ID.
// Returns null if the message is invalid.
int32_t parsePublisherIdResponse(const VehiclePropValue& publisher_id_response);

// Returns true if the new sequence number is greater than the last seen
// sequence number.
bool isSequenceNumberNewer(const VehiclePropValue& subscriptions_state,
                           const int last_seen_sequence_number);

// Returns sequence number of the message.
int32_t getSequenceNumberForSubscriptionsState(const VehiclePropValue& subscriptions_state);

// Takes a subscriptions state message and returns the layers that have active
// subscriptions of the layers that are offered by your HAL client/publisher.
//
// A publisher can use this function when receiving a subscriptions response or subscriptions
// change message to determine which layers to publish data on.
// The caller of this function can optionally decide to not consume these layers
// if the subscription change has the sequence number less than the last seen
// sequence number.
std::vector<VmsLayer> getSubscribedLayers(const VehiclePropValue& subscriptions_state,
                                          const VmsOffers& offers);

// Takes an availability change message and returns true if the parsed message implies that
// the service has newly started or restarted.
// If the message has a sequence number 0, it means that the service
// has newly started or restarted.
bool hasServiceNewlyStarted(const VehiclePropValue& availability_change);

// Takes a start session message, current service ID, current client ID; and returns the type/status
// of the message. It also populates the new service ID with the correct value.
VmsSessionStatus parseStartSessionMessage(const VehiclePropValue& start_session,
                                          const int current_service_id, const int current_client_id,
                                          int* new_service_id);

// Returns true if the new sequence number of the availability state message is greater than
// the last seen availability sequence number.
bool isAvailabilitySequenceNumberNewer(const VehiclePropValue& availability_state,
                                       const int last_seen_availability_sequence_number);

// Returns sequence number of the availability state message.
int32_t getSequenceNumberForAvailabilityState(const VehiclePropValue& availability_state);

// Takes a availability state message and returns the associated layers that are
// available to publish data.
//
// A subscriber can use this function when receiving an availability response or availability
// change message to determine which associated layers are ready to publish data.
// The caller of this function can optionally decide to not consume these layers
// if the availability change has the sequence number less than the last seen
// sequence number.
std::vector<VmsAssociatedLayer> getAvailableLayers(const VehiclePropValue& availability_state);

}  // namespace vms
}  // namespace V2_0
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android

#endif  // android_hardware_automotive_vehicle_V2_0_VmsUtils_H_
