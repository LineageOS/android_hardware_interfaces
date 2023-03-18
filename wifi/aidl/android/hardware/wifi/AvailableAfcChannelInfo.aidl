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
 * Defines the maximum EIRP per channel for supporting 6Ghz standard power for AFC. The format of
 * the data structure is derived from the Wi-Fi Alliance AFC System to AFC Device Interface
 * Specification: AvailableChannelInfo object.
 */
@VintfStability
parcelable AvailableAfcChannelInfo {
    /**
     * The global operating class used to define the channel center frequency indices
     * and operating bandwidth.
     */
    int globalOperatingClass;

    /**
     * The channel center frequency index.
     */
    int channelCfi;

    /**
     * The maximum permissible EIRP in units of dBm available for the channel
     * specified by channelCfi. In addition, in any portion of the channel, the conducted PSD plus
     * the maximum antenna gain cannot exceed the maxEirp divided by the channel width defined by
     * the globalOperatingClass.
     */
    int maxEirpDbm;
}
