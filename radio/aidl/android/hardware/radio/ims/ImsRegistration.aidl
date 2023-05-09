/*
 * Copyright (C) 2022 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License"),
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

package android.hardware.radio.ims;

import android.hardware.radio.AccessNetwork;
import android.hardware.radio.ims.ImsRegistrationState;
import android.hardware.radio.ims.SuggestedAction;

/** @hide */
@VintfStability
@JavaDerive(toString=true)
parcelable ImsRegistration {
    /** Default value */
    const int IMS_MMTEL_CAPABILITY_NONE = 0;
    /** IMS voice */
    const int IMS_MMTEL_CAPABILITY_VOICE = 1 << 0;
    /** IMS video */
    const int IMS_MMTEL_CAPABILITY_VIDEO = 1 << 1;
    /** IMS SMS */
    const int IMS_MMTEL_CAPABILITY_SMS = 1 << 2;
    /** IMS RCS */
    const int IMS_RCS_CAPABILITIES = 1 << 3;

    /** Indicates the current IMS registration state. */
    ImsRegistrationState regState;

    /**
     * Indicates the type of the radio access network where IMS is registered.
     */
    AccessNetwork accessNetworkType;

    /** Indicates the expected action for the radio to do. */
    SuggestedAction suggestedAction;

    /**
     * Values are bitwise ORs of IMS_MMTEL_CAPABILITY_* constants and IMS_RCS_CAPABILITIES.
     * IMS capability such as VOICE, VIDEO, SMS and RCS.
     */
    int capabilities;
}
