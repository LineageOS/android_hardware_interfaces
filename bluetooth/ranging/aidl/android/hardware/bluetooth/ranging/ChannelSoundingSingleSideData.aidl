/*
 * Copyright 2023 The Android Open Source Project
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

package android.hardware.bluetooth.ranging;

import android.hardware.bluetooth.ranging.ComplexNumber;
import android.hardware.bluetooth.ranging.Nadm;
import android.hardware.bluetooth.ranging.StepTonePct;

/**
 * Raw ranging data of Channel Sounding from either Initator or Reflector
 */
@VintfStability
parcelable ChannelSoundingSingleSideData {
    /**
     * PCT (complex value) measured from mode-2 or mode-3 steps in a CS procedure (in time order).
     */
    @nullable List<StepTonePct> stepTonePcts;
    /**
     * Packet Quality from mode-1 or mode-3 steps in a CS procedures (in time order).
     */
    @nullable byte[] packetQuality;
    /**
     * Packet RSSI (-127 to 20) of mode-0, mode-1, or mode-3 step data, in dBm.
     */
    @nullable byte[] packetRssiDbm;
    /**
     * Packet NADM of mode-1 or mode-3 step data for attack detection.
     */
    @nullable Nadm[] packetNadm;
    /**
     * Packet_PCT1 or packet_PCT2 of mode-1 or mode-3, if sounding sequence is used and sounding
     * phase-based ranging is supported.
     */
    @nullable List<ComplexNumber> packetPct1;
    @nullable List<ComplexNumber> packetPct2;
    /**
     * Reference power level (-127 to 20) of the signal in the procedure, in dBm.
     */
    byte referencePowerDbm;
    /**
     * Parameter for vendors to place vendor-specific raw ranging data.
     */
    @nullable byte[] vendorSpecificCsSingleSidedata;
}
