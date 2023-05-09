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
parcelable CdmaSignalStrength {
    /**
     * This value is the actual RSSI value multiplied by -1. Example: If the actual RSSI is -75,
     * then this response value will be 75. INT_MAX means invalid/unreported.
     */
    int dbm;
    /**
     * This value is the actual Ec/Io multiplied by -10. Example: If the actual Ec/Io is -12.5 dB,
     * then this response value will be 125. INT_MAX means invalid/unreported.
     */
    int ecio;
}
