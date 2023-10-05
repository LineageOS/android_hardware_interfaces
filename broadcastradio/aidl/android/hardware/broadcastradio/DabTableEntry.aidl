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

package android.hardware.broadcastradio;

/**
 * An entry in regional configuration for DAB.
 *
 * <p>This defines a frequency table row for ensembles.
 */
@VintfStability
@JavaDerive(equals=true, toString=true)
parcelable DabTableEntry {
    /**
     * Channel name, i.e. 5A, 7B.
     *
     * It must match the following regular expression:
     * /^[A-Z0-9][A-Z0-9 ]{0,5}[A-Z0-9]$/ (2-7 uppercase alphanumeric characters
     * without spaces allowed at the beginning nor end).
     */
    String label;

    /**
     * Frequency, in kHz.
     */
    int frequencyKhz;
}
