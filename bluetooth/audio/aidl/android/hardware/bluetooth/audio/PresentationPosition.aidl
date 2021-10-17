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

package android.hardware.bluetooth.audio;

@VintfStability
parcelable PresentationPosition {
    @VintfStability
    parcelable TimeSpec {
        /**
         * seconds
         */
        long tvSec;
        /**
         *  nanoseconds
         */
        long tvNSec;
    }
    /*
     * remoteDeviceAudioDelayNanos the audio delay from when the remote
     * device (e.g. headset) receives audio data to when the device plays the
     * sound. If the delay is unknown, the value is set to zero.
     */
    long remoteDeviceAudioDelayNanos;
    /*
     * transmittedOctets the number of audio data octets that were sent
     * to a remote device. This excludes octets that have been written to the
     * data path but have not been sent to the remote device. The count is
     * not reset until stopStream() is called. If the software data path is
     * unused (e.g. Hardware Offload), the value is set to 0.
     */
    long transmittedOctets;
    /*
     * transmittedOctetsTimestamp the value of CLOCK_MONOTONIC
     * corresponding to transmittedOctets. If the software data path is
     * unused (e.g., for Hardware Offload), the value is set to zero.
     */
    TimeSpec transmittedOctetsTimestamp;
}
