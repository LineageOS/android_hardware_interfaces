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

package android.hardware.radio.modem;

import android.hardware.radio.RadioIndicationType;
import android.hardware.radio.modem.HardwareConfig;
import android.hardware.radio.modem.RadioCapability;
import android.hardware.radio.modem.RadioState;
import android.hardware.radio.modem.ImeiInfo;

/**
 * Interface declaring unsolicited radio indications for modem APIs.
 * @hide
 */
@VintfStability
oneway interface IRadioModemIndication {
    /**
     * Indicates when the hardware configuration associated with the RILd changes.
     *
     * @param type Type of radio indication
     * @param configs Array of hardware configs
     */
    void hardwareConfigChanged(in RadioIndicationType type, in HardwareConfig[] configs);

    /**
     * Indicates when there is a modem reset.
     * When modem restarts, one of the following radio state transitions must happen
     * 1) RadioState:ON->RadioState:UNAVAILABLE->RadioState:ON or
     * 2) RadioState:OFF->RadioState:UNAVAILABLE->RadioState:OFF
     * This message must be sent either just before the Radio State changes to
     * RadioState:UNAVAILABLE or just after but must never be sent after the Radio State changes
     * from RadioState:UNAVAILABLE to RadioState:ON/RadioState:OFF again. It must NOT be sent after
     * the Radio state changes to RadioState:ON/RadioState:OFF after the modem restart as that may
     * be interpreted as a second modem reset by the framework.
     *
     * @param type Type of radio indication
     * @param reason the reason for the reset. It may be a crash signature if the restart was due to
     *        a crash or some string such as "user-initiated restart" or "AT command initiated
     *        restart" that explains the cause of the modem restart
     */
    void modemReset(in RadioIndicationType type, in String reason);

    /**
     * Sent when setRadioCapability() completes. Returns the phone radio capability exactly as
     * getRadioCapability() and must be the same set as sent by setRadioCapability().
     *
     * @param type Type of radio indication
     * @param rc Current radio capability
     */
    void radioCapabilityIndication(in RadioIndicationType type, in RadioCapability rc);

    /**
     * Indicates when radio state changes.
     *
     * @param type Type of radio indication
     * @param radioState Current radio state
     */
    void radioStateChanged(in RadioIndicationType type, in RadioState radioState);

    /**
     * Indicates the ril connects and returns the version
     *
     * @param type Type of radio indication
     */
    void rilConnected(in RadioIndicationType type);

    /**
     * Indicates when there is a change in the IMEI mapping.
     *
     * @param type Type of radio indication
     * @param imeiInfo IMEI information
     */
     void onImeiMappingChanged(in RadioIndicationType type, in ImeiInfo imeiInfo);
}
