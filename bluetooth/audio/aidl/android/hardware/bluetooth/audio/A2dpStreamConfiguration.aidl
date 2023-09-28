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

package android.hardware.bluetooth.audio;

import android.hardware.bluetooth.audio.CodecId;

@VintfStability
parcelable A2dpStreamConfiguration {
    /**
     * Peer MTU (16 bits)
     */
    int peerMtu;

    /**
     * Optional SCMS-T Content Protection header
     * that precedes audio content when enabled [A2DP - 3.2.1-2].
     * The content protection byte is defined by [Assigned Number - 6.3.2].
     */
    @nullable byte[1] cpHeaderScmst;

    /**
     * Codec Identifier and `configuration` as defined by A2DP for codec
     * interoperability requirements. Using `codecId.a2dp`, the format is given
     * by the `Codec Specific Information Elements` [A2DP - 4.3-6.2], and
     * using `codecId.vendor`, by `Vendor Specific Value` [A2DP - 4.7.2].
     */
    CodecId codecId;
    byte[] configuration;
}
