/*
 * Copyright 2021 The Android Open Source Project
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

package android.hardware.tv.tuner;

import android.hardware.tv.tuner.ILnbCallback;
import android.hardware.tv.tuner.LnbPosition;
import android.hardware.tv.tuner.LnbTone;
import android.hardware.tv.tuner.LnbVoltage;

/**
 * A Tuner LNB (low-noise block downconverter) is used by satellite frontend
 * to receive the microwave signal from the satellite, amplify it, and
 * downconvert the frequency to a lower frequency.
 * @hide
 */
@VintfStability
interface ILnb {
    /**
     * Set the lnb callback.
     *
     * ILnbCallback is used by the client to receive events from the Lnb.
     * Only one callback per ILnb instance is supported. The callback
     * will be replaced if it's set again.
     *
     * @param callback Callback object to pass Lnb events to the system.
     *        The previously registered callback must be replaced with this one.
     *        It can be null.
     */
    void setCallback(in ILnbCallback callback);

    /**
     * Set the lnb's power voltage.
     *
     * @param voltage the power's voltage the Lnb to use.
     */
    void setVoltage(in LnbVoltage voltage);

    /**
     * Set the lnb's tone mode.
     *
     * @param tone the tone mode the Lnb to use.
     */
    void setTone(in LnbTone tone);

    /**
     * Select the lnb's position.
     *
     * @param position the position the Lnb to use.
     */
    void setSatellitePosition(in LnbPosition position);

    /**
     *  Sends DiSEqC (Digital Satellite Equipment Control) message.
     *
     * Client sends DiSeqc message to DiSEqc to LNB. The response message from
     * the device comes back to the client through frontend's callback
     * onDiseqcMessage.
     *
     * @param diseqcMessage a byte array of data for DiSEqC message which is
     *        specified by EUTELSAT Bus Functional Specification Version 4.2.
     */
    void sendDiseqcMessage(in byte[] diseqcMessage);

    /**
     * Releases the LNB instance
     *
     * Associated resources are released.  close may be called more than once.
     * Calls to any other method after this will return an error
     */
    void close();
}
