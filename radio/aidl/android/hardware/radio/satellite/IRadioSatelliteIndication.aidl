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

import android.hardware.radio.RadioIndicationType;
import android.hardware.radio.satellite.NTRadioTechnology;
import android.hardware.radio.satellite.PointingInfo;
import android.hardware.radio.satellite.SatelliteFeature;
import android.hardware.radio.satellite.SatelliteMode;

/**
 * Interface declaring unsolicited radio indications for satellite APIs.
 */
@VintfStability
oneway interface IRadioSatelliteIndication {
    /**
     * Confirms that ongoing message transfer is complete.
     *
     * @param type Type of radio indication
     * @param complete True mean the transfer is complete.
     *                 False means the transfer is not complete.
     */
    void onMessagesTransferComplete(in RadioIndicationType type, in boolean complete);

    /**
     * Indicates new message received on device.
     *
     * @param type Type of radio indication
     * @param messages List of new messages received.
     */
    void onNewMessages(in RadioIndicationType type, in String[] messages);

    /**
     * Indicates that satellite has pending messages for the device to be pulled.
     *
     * @param type Type of radio indication
     * @param count Number of pending messages.
     */
    void onPendingMessageCount(in RadioIndicationType type, in int count);

    /**
     * Indicate that satellite provision state has changed.
     *
     * @param type Type of radio indication
     * @param provisioned True means the service is provisioned.
     *                    False means the service is not provisioned.
     * @param features List of Feature whose provision state has changed.
     */
    void onProvisionStateChanged(
            in RadioIndicationType type, boolean provisioned, in SatelliteFeature[] features);

    /**
     * Indicate that satellite mode has changed.
     *
     * @param type Type of radio indication
     * @param mode The current mode of the satellite modem.
     */
    void onSatelliteModeChanged(in RadioIndicationType type, in SatelliteMode mode);

    /**
     * Indicate that satellite Pointing input has changed.
     *
     * @param type Type of radio indication
     * @param pointingInfo The current pointing info.
     */
    void onSatellitePointingInfoChanged(in RadioIndicationType type, in PointingInfo pointingInfo);

    /**
     * Indicate that satellite radio technology has changed.
     *
     * @param type Type of radio indication
     * @param technology The current technology of the satellite modem.
     */
    void onSatelliteRadioTechnologyChanged(
            in RadioIndicationType type, in NTRadioTechnology technology);
}
