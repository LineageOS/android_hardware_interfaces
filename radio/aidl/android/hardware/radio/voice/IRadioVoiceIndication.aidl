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

package android.hardware.radio.voice;

import android.hardware.radio.RadioIndicationType;
import android.hardware.radio.voice.CdmaCallWaiting;
import android.hardware.radio.voice.CdmaInformationRecord;
import android.hardware.radio.voice.CdmaOtaProvisionStatus;
import android.hardware.radio.voice.CdmaSignalInfoRecord;
import android.hardware.radio.voice.EmergencyNumber;
import android.hardware.radio.voice.SrvccState;
import android.hardware.radio.voice.StkCcUnsolSsResult;
import android.hardware.radio.voice.UssdModeType;

/**
 * Interface declaring unsolicited radio indications for voice APIs.
 * @hide
 */
@VintfStability
oneway interface IRadioVoiceIndication {
    /**
     * Ring indication for an incoming call (eg, RING or CRING event). There must be at least one
     * callRing() at the beginning of a call and sending multiple is optional. If the system
     * property ro.telephony.call_ring.multiple is false then the upper layers must generate the
     * multiple events internally. Otherwise the vendor code must generate multiple callRing() if
     * ro.telephony.call_ring.multiple is true or if it is absent.
     * The rate of these events is controlled by ro.telephony.call_ring.delay and has a default
     * value of 3000 (3 seconds) if absent.
     *
     * @param type Type of radio indication
     * @param isGsm true for GSM & false for CDMA
     * @param record Cdma Signal Information
     */
    void callRing(in RadioIndicationType type, in boolean isGsm, in CdmaSignalInfoRecord record);

    /**
     * Indicates when call state has changed. Callee must invoke IRadioVoice.getCurrentCalls().
     * Must be invoked on, for example, "RING", "BUSY", "NO CARRIER", and also call state
     * transitions (DIALING->ALERTING ALERTING->ACTIVE). Redundent or extraneous invocations are
     * tolerated.
     *
     * @param type Type of radio indication
     */
    void callStateChanged(in RadioIndicationType type);

    /**
     * Indicates when CDMA radio receives a call waiting indication.
     *
     * @param type Type of radio indication
     * @param callWaitingRecord Cdma CallWaiting information
     */
    void cdmaCallWaiting(in RadioIndicationType type, in CdmaCallWaiting callWaitingRecord);

    /**
     * Indicates when CDMA radio receives one or more info recs.
     *
     * @param type Type of radio indication
     * @param records New CDMA information records.
     *        Max length is RadioConst:CDMA_MAX_NUMBER_OF_INFO_RECS
     */
    void cdmaInfoRec(in RadioIndicationType type, in CdmaInformationRecord[] records);

    /**
     * Indicates when CDMA radio receives an update of the progress of an OTASP/OTAPA call.
     *
     * @param type Type of radio indication
     * @param status Cdma OTA provision status
     */
    void cdmaOtaProvisionStatus(in RadioIndicationType type, in CdmaOtaProvisionStatus status);

    /**
     * Report the current list of emergency numbers. Each emergency number in the emergency number
     * list contains a dialing number, zero or more service category(s), zero or more emergency
     * uniform resource names, mobile country code, mobile network code, and source(s) that indicate
     * where it comes from.
     * Radio must report all the valid emergency numbers with known mobile country code, mobile
     * network code, emergency service categories, and emergency uniform resource names from all
     * available sources including network signaling, sim, modem/oem configuration, and default
     * configuration (112 and 911 must be always available; additionally, 000, 08, 110, 999, 118
     * and 119 must be available when sim is not present). Radio shall not report emergency numbers
     * that are invalid in the current locale. The reported emergency number list must not have
     * duplicate EmergencyNumber entries. Please refer the documentation of EmergencyNumber to
     * construct each emergency number to report.
     * Radio must report the complete list of emergency numbers whenever the emergency numbers in
     * the list are changed or whenever the client and the radio server are connected.
     *
     * Reference: 3gpp 22.101, Section 10 - Emergency Calls;
     *            3gpp 24.008, Section 9.2.13.4 - Emergency Number List
     *
     * @param type Type of radio indication
     * @param emergencyNumberList Current list of emergency numbers known to radio.
     */
    void currentEmergencyNumberList(
            in RadioIndicationType type, in EmergencyNumber[] emergencyNumberList);

    /**
     * Indicates that the radio system selection module has autonomously entered emergency
     * callback mode.
     *
     * @param type Type of radio indication
     */
    void enterEmergencyCallbackMode(in RadioIndicationType type);

    /**
     * Indicates when Emergency Callback Mode ends. Indicates that the radio system selection module
     * has proactively exited emergency callback mode.
     *
     * @param type Type of radio indication
     */
    void exitEmergencyCallbackMode(in RadioIndicationType type);

    /**
     * Indicates that nework doesn't have in-band information, need to play out-band tone.
     *
     * @param type Type of radio indication
     * @param start true = start play ringback tone, false = stop playing ringback tone
     */
    void indicateRingbackTone(in RadioIndicationType type, in boolean start);

    /**
     * Indicates when Supplementary service(SS) response is received when DIAL/USSD/SS is changed to
     * SS by call control.
     *
     * @param type Type of radio indication
     */
    void onSupplementaryServiceIndication(in RadioIndicationType type, in StkCcUnsolSsResult ss);

    /**
     * Indicates when a new USSD message is received. The USSD session is assumed to persist if the
     * type code is REQUEST, otherwise the current session (if any) is assumed to have terminated.
     *
     * @param type Type of radio indication
     * @param modeType USSD type code
     * @param msg Message string in UTF-8, if applicable
     */
    void onUssd(in RadioIndicationType type, in UssdModeType modeType, in String msg);

    /**
     * Indicates that framework/application must reset the uplink mute state.
     *
     * @param type Type of radio indication
     */
    void resendIncallMute(in RadioIndicationType type);

    /**
     * Indicates when Single Radio Voice Call Continuity (SRVCC) progress state has changed.
     *
     * @param type Type of radio indication
     * @param state New Srvcc State
     */
    void srvccStateNotify(in RadioIndicationType type, in SrvccState state);

    /**
     * Indicates when there is an ALPHA from UICC during Call Control.
     *
     * @param type Type of radio indication
     * @param alpha ALPHA string from UICC in UTF-8 format
     */
    void stkCallControlAlphaNotify(in RadioIndicationType type, in String alpha);

    /**
     * Indicates when SIM wants application to setup a voice call.
     *
     * @param type Type of radio indication
     * @param timeout Timeout value in millisec for setting up voice call
     */
    void stkCallSetup(in RadioIndicationType type, in long timeout);
}
