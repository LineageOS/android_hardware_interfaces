/*
 * Copyright 2021 The Android Open Source Project
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

package android.hardware.tv.tuner;

/**
 * Lnb Event Type.
 * @hide
 */
@VintfStability
@Backing(type="int")
enum LnbEventType {
    DISEQC_RX_OVERFLOW,

    /**
     * If LNB detect that outgoing Diseqc message isn't delivered on time.
     */
    DISEQC_RX_TIMEOUT,

    /**
     * If LNB detect that the incoming Diseqc message has parity error.
     */
    DISEQC_RX_PARITY_ERROR,

    /**
     * If LNB detect that the LNB is overload.
     */
    LNB_OVERLOAD,
}
