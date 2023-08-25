/*
 * Copyright (C) 2023 The Android Open Source Project
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

package android.hardware.wifi;

/**
 * Defines the maximum permissible power spectral density on a range of
 * frequencies to support 6Ghz with standard power for AFC.
 * The format of the data follows spec from the Wi-Fi Alliance AFC System to
 * AFC Device Interface Specification: AvailableFrequencyInfo object.
 */
@VintfStability
parcelable AvailableAfcFrequencyInfo {
    /**
     * Defines the lowest frequency included in this 6Ghz frequency range.
     */
    int startFrequencyMhz;
    /**
     * Defines the highest frequency included in this 6Ghz frequency range.
     */
    int endFrequencyMhz;
    /**
     * The maximum permissible EIRP available in any one MHz bin within the
     * frequency range specified. The limit is expressed as a power spectral
     * density with units of dBm per MHz.
     */
    int maxPsd;
}
