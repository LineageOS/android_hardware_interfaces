/*
 * Copyright (C) 2022 The Android Open Source Project
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

import android.hardware.radio.ims.media.AmrMode;
import android.hardware.radio.ims.media.EvsMode;

/** @hide */
@VintfStability
@JavaDerive(toString=true)
union CodecMode {
    /** Default value */
    boolean noinit;
    /** AMR codec mode to represent the bit rate. See 3ggp Specs 26.976 & 26.071 */
    AmrMode amr;
    /** EVS codec mode to represent the bit rate. See 3ggp Spec 26.952 Table 5.1 */
    EvsMode evs;
}
