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

import android.hardware.radio.ims.media.CodecSpecificParams;
import android.hardware.radio.ims.media.CodecType;

/** @hide */
@VintfStability
@JavaDerive(toString=true)
parcelable CodecParams {
    /** Negotiated codec type */
    CodecType codecType;
    /**
     * Static or dynamic payload type number negotiated through the SDP for
     * the incoming RTP packets. This value shall be matched with the PT value
     * of the incoming RTP header. Values 0 to 127, see RFC 3551 section 6
     */
    byte rxPayloadTypeNumber;
    /**
     * Static or dynamic payload type number negotiated through the SDP for
     * the outgoing RTP packets. This value shall be set to the PT value
     * of the outgoing RTP header. Values 0 to 127, see RFC 3551 section 6
     */
    byte txPayloadTypeNumber;
    /** Sampling rate in kHz*/
    byte samplingRateKHz;
    /** dtx: Whether discontinuous transmission is enabled or not */
    boolean dtxEnabled;
    /** Codec specific parameters */
    CodecSpecificParams codecSpecificParams;
}
