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

package android.hardware.radio.satellite;

import android.hardware.radio.satellite.IRadioSatelliteIndication;
import android.hardware.radio.satellite.IRadioSatelliteResponse;
import android.hardware.radio.satellite.IndicationFilter;
import android.hardware.radio.satellite.SatelliteFeature;

/**
 * This interface is used by telephony to send commands to and query info from satellite modem.
 * All the functions have minimum one parameter:
 * serial: which corresponds to serial no. of request. Serial numbers must only be memorized for the
 * duration of a method call. If clients provide colliding serials (including passing the same
 * serial to different methods), multiple responses (one for each method call) must still be served.
 */
@VintfStability
oneway interface IRadioSatellite {
    /**
     * Add contacts that are allowed to be used for satellite communication. This is applicable for
     * incoming messages as well.
     *
     * @param serial Serial number of request.
     * @param contacts List of allowed contacts to be added.
     */
    void addAllowedSatelliteContacts(in int serial, in String[] contacts);

    /**
     * Get feature capabilities supported by satellite.
     *
     * @param serial Serial number of request.
     */
    void getCapabilities(in int serial);

    /**
     * Get max number of characters per text message.
     *
     * @param serial Serial number of request.
     */
    void getMaxCharactersPerTextMessage(in int serial);

    /**
     * Get pending messages.
     *
     * @param serial Serial number of request.
     */
    void getPendingMessages(in int serial);

    /**
     * Get satellite modem state.
     *
     * @param serial Serial number of request.
     */
    void getPowerState(in int serial);

    /**
     * Get current satellite registration mode, which is defined in {@link #SatelliteMode}.
     *
     * @param serial Serial number of request.
     */
    void getSatelliteMode(in int serial);

    /**
     * Get time for next visibility of satellite.
     *
     * @param serial Serial number of request.
     */
    void getTimeForNextSatelliteVisibility(in int serial);

    /**
     * Provision the subscription with a satellite provider. This is needed to register the
     * subscription if the provider allows dynamic registration.
     *
     * @param serial Serial number of request.
     * @param imei IMEI of the SIM associated with the satellite modem.
     * @param msisdn MSISDN of the SIM associated with the satellite modem.
     * @param imsi IMSI of the SIM associated with the satellite modem.
     * @param features List of features to be provisioned.
     */
    void provisionService(in int serial, in String imei, in String msisdn, in String imsi,
            in SatelliteFeature[] features);

    /**
     * Remove contacts that are allowed to be used for satellite communication. This is applicable
     * for incoming messages as well.
     *
     * @param serial Serial number of request.
     * @param contacts List of allowed contacts to be removed.
     */
    void removeAllowedSatelliteContacts(in int serial, in String[] contacts);

    /**
     * When response type received from a radio indication or radio response is
     * RadioIndicationType:UNSOLICITED_ACK_EXP or RadioResponseType:SOLICITED_ACK_EXP respectively,
     * acknowledge the receipt of those messages by sending responseAcknowledgement().
     */
    void responseAcknowledgement();

    /**
     * Send text messages.
     *
     * @param serial Serial number of request.
     * @param messages List of messages in text format to be sent.
     * @param destination The recipient of the message.
     * @param latitude The current latitude of the device.
     * @param longitude The current longitude of the device. The location (i.e., latitude and
     *        longitude) of the device will be filled for emergency messages.
     */
    void sendMessages(in int serial, in String[] messages, in String destination,
            in double latitude, in double longitude);

    /**
     * Set the filter for what type of indication framework want to receive from modem.
     *
     * @param serial Serial number of request.
     * @param filterBitmask The filter bitmask identifying what type of indication Telephony
     *                      framework wants to receive from modem. This bitmask is the 'or'
     *                      combination of the enum values defined in {@link #IndicationFilter}.
     */
    void setIndicationFilter(in int serial, in int filterBitmask);

    /**
     * Turn satellite modem on/off.
     *
     * @param serial Serial number of request.
     * @param on True for turning on.
     *           False for turning off.
     */
    void setPower(in int serial, in boolean on);

    /**
     * Set response functions for Satellite requests and indications.
     *
     * @param satelliteResponse Object containing response functions
     * @param satelliteIndication Object containing radio indications
     */
    void setResponseFunctions(in IRadioSatelliteResponse satelliteResponse,
            in IRadioSatelliteIndication satelliteIndication);

    /**
     * User started pointing to the satellite. Modem should continue to update the pointing input
     * as user device/satellite moves.
     *
     * @param serial Serial number of request.
     */
    void startSendingSatellitePointingInfo(in int serial);

    /**
     * Stop sending satellite pointing info to the framework.
     *
     * @param serial Serial number of request.
     */
    void stopSendingSatellitePointingInfo(in int serial);
}
