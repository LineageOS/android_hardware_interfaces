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

@VintfStability
@Backing(type="int")
enum CsSecurityLevel {
    /**
     * Ranging algorithm is not implemented.
     */
    NOT_SUPPORTED = 0x00,
    /**
     * Either CS tone or CS RTT.
     */
    ONE = 0x01,
    /**
     * 150 ns CS RTT accuracy and CS tones.
     */
    TWO = 0x02,
    /**
     * 10 ns CS RTT accuracy and CS tones.
     */
    THREE = 0x03,
    /**
     * Level 3 with the addition of CS RTT sounding sequence or random sequence
     * payloads, and support of the Normalized Attack Detector Metric requirements.
     */
    FOUR = 0x04,
}
