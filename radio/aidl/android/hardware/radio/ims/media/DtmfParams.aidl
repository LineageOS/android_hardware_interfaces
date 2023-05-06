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

package android.hardware.radio.ims.media;

/** @hide */
@VintfStability
@JavaDerive(toString=true)
parcelable DtmfParams {
    /**
     * Dynamic payload type number to be used for DTMF RTP packets received.
     * The values is in the range from 96 to 127 chosen during the session
     * establishment. The PT  value of the RTP header of all DTMF packets shall be
     * set with this value.
     */
    byte rxPayloadTypeNumber;

    /**
     * Dynamic payload type number to be used for DTMF RTP packets sent.
     * The values is in the range from 96 to 127 chosen during the session
     * establishment. The PT value of the RTP header of all DTMF packets shall be set
     * with this value.
     */
    byte txPayloadTypeNumber;

    /** Sampling rate in kHz */
    byte samplingRateKHz;
}
