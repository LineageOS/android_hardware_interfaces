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

package android.hardware.radio.network;

/** @hide */
@VintfStability
@JavaDerive(toString=true)
parcelable GsmSignalStrength {
    /**
     * Valid values are (0-61, 99) as defined in TS 27.007 8.69; INT_MAX means invalid/unreported.
     */
    int signalStrength;
    /**
     * Bit error rate (0-7, 99) as defined in TS 27.007 8.5; INT_MAX means invalid/unreported.
     */
    int bitErrorRate;
    /**
     * Timing advance in bit periods. 1 bit period = 48/13 us. INT_MAX means invalid/unreported.
     */
    int timingAdvance;
}
