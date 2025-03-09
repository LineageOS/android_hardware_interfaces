/*
 * Copyright (C) 2024 The Android Open Source Project
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

package android.hardware.radio.config;

import android.hardware.radio.config.SimType;

/** @hide */
@VintfStability
@JavaDerive(toString=true)
parcelable SimTypeInfo {
    /**
     * Current SimType on the physical slot id.
     **/
    SimType currentSimType = SimType.UNKNOWN;
    /**
     * Bitmask of the sim types supported by the physical slot id. Physical slot can support more
     * than one SimType.
     * Example:
     * if the physical slot id supports either pSIM/eSIM and currently pSIM is active,
     * currentSimType will be SimType::PHYSICAL and supportedSimTypes will be
     * SimType::PHYSICAL | SimType::ESIM.
     **/
    int supportedSimTypes;
}
