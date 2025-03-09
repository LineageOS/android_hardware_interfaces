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

package android.hardware.radio.voice;

/** @hide */
@VintfStability
@Backing(type="int")
@JavaDerive(toString=true)
enum CdmaOtaProvisionStatus {
    /** @deprecated Legacy CDMA is unsupported. */
    SPL_UNLOCKED,
    /** @deprecated Legacy CDMA is unsupported. */
    SPC_RETRIES_EXCEEDED,
    /** @deprecated Legacy CDMA is unsupported. */
    A_KEY_EXCHANGED,
    /** @deprecated Legacy CDMA is unsupported. */
    SSD_UPDATED,
    /** @deprecated Legacy CDMA is unsupported. */
    NAM_DOWNLOADED,
    /** @deprecated Legacy CDMA is unsupported. */
    MDN_DOWNLOADED,
    /** @deprecated Legacy CDMA is unsupported. */
    IMSI_DOWNLOADED,
    /** @deprecated Legacy CDMA is unsupported. */
    PRL_DOWNLOADED,
    /** @deprecated Legacy CDMA is unsupported. */
    COMMITTED,
    /** @deprecated Legacy CDMA is unsupported. */
    OTAPA_STARTED,
    /** @deprecated Legacy CDMA is unsupported. */
    OTAPA_STOPPED,
    /** @deprecated Legacy CDMA is unsupported. */
    OTAPA_ABORTED,
}
