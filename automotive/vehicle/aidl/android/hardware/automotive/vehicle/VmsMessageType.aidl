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

package android.hardware.automotive.vehicle;

/**
 * This enum lists the types of supported VMS messages. It is used as the first
 * integer in the vehicle property integers array and determines how the rest of
 * the message is decoded.
 */
@VintfStability
@Backing(type="int")
enum VmsMessageType {
    /**
     * A request from the subscribers to the VMS service to subscribe to a layer.
     *
     * This message type uses enum VmsMessageWithLayerIntegerValuesIndex.
     */
    SUBSCRIBE = 1,
    /**
     * A request from the subscribers to the VMS service to subscribe to a layer from a specific
     * publisher.
     *
     * This message type uses enum VmsMessageWithLayerAndPublisherIdIntegerValuesIndex.
     */
    SUBSCRIBE_TO_PUBLISHER = 2,
    /**
     * A request from the subscribers to the VMS service to unsubscribes from a layer.
     *
     * This message type uses enum VmsMessageWithLayerIntegerValuesIndex.
     */
    UNSUBSCRIBE = 3,
    /**
     * A request from the subscribers to the VMS service to unsubscribes from a layer from a
     * specific publisher.
     *
     * This message type uses enum VmsMessageWithLayerAndPublisherIdIntegerValuesIndex.
     */
    UNSUBSCRIBE_TO_PUBLISHER = 4,
    /**
     * Information from the publishers to the VMS service about the layers which the client can
     * publish.
     *
     * This message type uses enum VmsOfferingMessageIntegerValuesIndex.
     */
    OFFERING = 5,
    /**
     * A request from the subscribers to the VMS service to get the available layers.
     *
     * This message type uses enum VmsBaseMessageIntegerValuesIndex.
     */
    AVAILABILITY_REQUEST = 6,
    /**
     * A request from the publishers to the VMS service to get the layers with subscribers.
     *
     * This message type uses enum VmsBaseMessageIntegerValuesIndex.
     */
    SUBSCRIPTIONS_REQUEST = 7,
    /**
     * A response from the VMS service to the subscribers to a VmsMessageType.AVAILABILITY_REQUEST
     *
     * This message type uses enum VmsAvailabilityStateIntegerValuesIndex.
     */
    AVAILABILITY_RESPONSE = 8,
    /**
     * A notification from the VMS service to the subscribers on a change in the available layers.
     *
     * This message type uses enum VmsAvailabilityStateIntegerValuesIndex.
     */
    AVAILABILITY_CHANGE = 9,
    /**
     * A response from the VMS service to the publishers to a VmsMessageType.SUBSCRIPTIONS_REQUEST
     *
     * This message type uses enum VmsSubscriptionsStateIntegerValuesIndex.
     */
    SUBSCRIPTIONS_RESPONSE = 10,
    /**
     * A notification from the VMS service to the publishers on a change in the layers with
     * subscribers.
     *
     * This message type uses enum VmsSubscriptionsStateIntegerValuesIndex.
     */
    SUBSCRIPTIONS_CHANGE = 11,
    /**
     * A message from the VMS service to the subscribers or from the publishers to the VMS service
     * with a serialized VMS data packet as defined in the VMS protocol.
     *
     * This message type uses enum VmsMessageWithLayerAndPublisherIdIntegerValuesIndex.
     */
    DATA = 12,
    /**
     * A request from the publishers to the VMS service to get a Publisher ID for a serialized VMS
     * provider description packet as defined in the VMS protocol.
     *
     * This message type uses enum VmsBaseMessageIntegerValuesIndex.
     */
    PUBLISHER_ID_REQUEST = 13,
    /**
     * A response from the VMS service to the publisher that contains a provider description packet
     * and the publisher ID assigned to it.
     *
     * This message type uses enum VmsPublisherInformationIntegerValuesIndex.
     */
    PUBLISHER_ID_RESPONSE = 14,
    /**
     * A request from the subscribers to the VMS service to get information for a Publisher ID.
     *
     * This message type uses enum VmsPublisherInformationIntegerValuesIndex.
     */
    PUBLISHER_INFORMATION_REQUEST = 15,
    /**
     * A response from the VMS service to the subscribers that contains a provider description
     * packet and the publisher ID assigned to it.
     *
     * This message type uses enum VmsPublisherInformationIntegerValuesIndex.
     */
    PUBLISHER_INFORMATION_RESPONSE = 16,
    /**
     * A notification indicating that the sender has been reset.
     *
     * The receiving party must reset its internal state and respond to the
     * sender with a START_SESSION message as acknowledgement.
     *
     * This message type uses enum VmsStartSessionMessageIntegerValuesIndex.
     */
    START_SESSION = 17,
    // LAST_VMS_MESSAGE_TYPE = START_SESSION,
}
