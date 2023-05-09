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

import android.hardware.radio.ims.media.EvsBandwidth;
import android.hardware.radio.ims.media.EvsMode;

/** @hide */
@VintfStability
@JavaDerive(toString=true)
parcelable EvsParams {
    /** EVS codec bandwidth */
    EvsBandwidth bandwidth;

    /** mode-set: EVS codec mode to represent the bit rate */
    EvsMode evsMode;
    /**
     * ch-aw-recv: Channel aware mode for the receive direction. Permissible values
     * are -1, 0, 2, 3, 5, and 7. If -1, channel-aware mode is disabled in the
     * session for the receive direction. If 0 or not present, partial redundancy
     * (channel-aware mode) is not used at the start of the session for the receive
     * direction. If positive (2, 3, 5, or 7), partial redundancy (channel-aware
     * mode) is used at the start of the session for the receive direction using the
     * value as the offset, See 3GPP TS 26.445 section 4.4.5
     */
    byte channelAwareMode;
    /**
     * hf-only: Header full only is used for the outgoing and incoming packets.
     * If it's true then the session shall support header full format only else the
     * session could support both header full format and compact format.
     */
    boolean useHeaderFullOnly;
    /**
     * evs-mode-switch: Used for switching between EVS Primary mode and EVS AMR-WB IO mode,
     * If this value is true, the codec operates in AMR-WB IO mode
     */
    boolean useEvsModeSwitch;
    /**
     * cmr: Codec mode request is used to request the speech codec encoder of the
     * other party to set the frame type index of speech mode via RTP header, See
     * 3GPP TS 26.445 section A.3. Allowed values are -1, 0 and 1.
     */
    byte codecModeRequest;
}
