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

package android.hardware.radio.modem;

/** @hide */
@VintfStability
@JavaDerive(toString=true)
parcelable RadioCapability {
    /**
     * Logical Modem's (LM) initial value and value after PHASE_FINISH completes.
     */
    const int PHASE_CONFIGURED = 0;
    /**
     * PHASE_START is sent before PHASE_APPLY and indicates that an APPLY is forthcoming with these
     * same parameters.
     */
    const int PHASE_START = 1;
    /**
     * PHASE_APPLY is sent after all LM's receive PHASE_START and returned
     * RadioCapability.status = 0.
     * If any PHASE_START's fail, hal implementation must not send PHASE_APPLY.
     */
    const int PHASE_APPLY = 2;
    /**
     * PHASE_UNSOL_RSP is sent with unsolicited radioCapability().
     */
    const int PHASE_UNSOL_RSP = 3;
    /**
     * PHASE_FINISH is sent after all commands have completed. If an error occurs in any previous
     * command, the RadioAccessFamily and logicalModemUuid fields must be the prior configuration
     * thus restoring the configuration to the previous value. An error returned by PHASE_FINISH
     * will generally be ignored or may cause that LM to be removed from service.
     */
    const int PHASE_FINISH = 4;

    /**
     * This parameter has no meaning with PHASE_START, PHASE_APPLY.
     */
    const int STATUS_NONE = 0;
    /**
     * Tell modem the action transaction of set radio capability was successful with PHASE_FINISH.
     */
    const int STATUS_SUCCESS = 1;
    /**
     * Tell modem the action transaction of set radio capability failed with PHASE_FINISH.
     */
    const int STATUS_FAIL = 2;

    /**
     * Unique session value defined by framework returned in all "responses/unslo".
     */
    int session;
    /**
     * Values are PHASE_
     */
    int phase;
    /**
     * 32-bit bitmap of RadioAccessFamily.
     */
    int raf;
    /**
     * A UUID typically "com.xxxx.lmX" where X is the logical modem.
     * RadioConst:MAX_UUID_LENGTH is the max length.
     */
    String logicalModemUuid;
    /**
     * Values are STATUS_
     */
    int status;
}
