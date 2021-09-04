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

package android.hardware.radio;

@VintfStability
@Backing(type="int")
enum RadioCapabilityPhase {
    /**
     * Logical Modem's (LM) initial value and value after FINISH completes
     */
    CONFIGURED,
    /**
     * START is sent before APPLY and indicates that an APPLY is forthcoming with these same
     * parameters.
     */
    START,
    /**
     * APPLY is sent after all LM's receive START and returned RadioCapability.status = 0.
     * If any START's fail, hal implementation must not send APPLY.
     */
    APPLY,
    /**
     * UNSOL_RSP is sent with unsolicited radioCapability()
     */
    UNSOL_RSP,
    /**
     * FINISH is sent after all commands have completed. If an error occurs in any previous command,
     * the RadioAccessFamily and logicalModemUuid fields must be the prior configuration thus
     * restoring the configuration to the previous value. An error returned by FINISH will generally
     * be ignored or may cause that LM to be removed from service.
     */
    FINISH,
}
