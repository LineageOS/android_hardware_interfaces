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

package android.hardware.radio.ims;

import android.hardware.radio.RadioIndicationType;
import android.hardware.radio.ims.ConnectionFailureInfo;
import android.hardware.radio.ims.ImsDeregistrationReason;
import android.hardware.radio.ims.ImsStreamDirection;
import android.hardware.radio.ims.ImsStreamType;

/**
 * Interface declaring unsolicited radio indications for ims APIs.
 * @hide
 */
@VintfStability
oneway interface IRadioImsIndication {
    /**
     * Fired by radio when any IMS traffic is not sent to network due to any failure
     * on cellular networks. IMS service shall call stopImsTraffic when receiving
     * this indication.
     *
     * @param type Type of radio indication
     * @param token The token of startImsTraffic() associated with this indication
     * @param info Connection failure information
     */
    void onConnectionSetupFailure(
            in RadioIndicationType type, int token, in ConnectionFailureInfo info);

    /**
     * Access Network Bitrate Recommendation (ANBR), see 3GPP TS 26.114.
     * Notifies the bit rate received from the network via ANBR message
     *
     * @param type Type of radio indication
     * @param mediaType Media type is used to identify media stream such as audio or video
     * @param direction Direction of this packet stream (e.g. uplink or downlink)
     * @param bitsPerSecond The recommended bit rate for the UE
     * for a specific logical channel and a specific direction by NW
     */
    void notifyAnbr(in RadioIndicationType type, in ImsStreamType mediaType,
            in ImsStreamDirection direction, int bitsPerSecond);

    /**
     * Requests IMS stack to perform graceful IMS deregistration before radio performing
     * network detach in the events of SIM remove, refresh or and so on. The radio waits for
     * the IMS deregistration, which will be notified by telephony via
     * {@link IRadioIms#updateImsRegistrationInfo()}, or a certain timeout interval to start
     * the network detach procedure.
     *
     * @param type Type of radio indication
     * @param reason the reason why the deregistration is triggered
     */
    void triggerImsDeregistration(in RadioIndicationType type, in ImsDeregistrationReason reason);
}
