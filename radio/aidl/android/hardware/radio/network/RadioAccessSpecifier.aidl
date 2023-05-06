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

package android.hardware.radio.network;

import android.hardware.radio.AccessNetwork;
import android.hardware.radio.network.RadioAccessSpecifierBands;

/** @hide */
@VintfStability
@JavaDerive(toString=true)
parcelable RadioAccessSpecifier {
    /**
     * The type of network to scan.
     */
    AccessNetwork accessNetwork;
    /**
     * The frequency bands to scan. Maximum length of the vector is 8.
     */
    RadioAccessSpecifierBands bands;
    /**
     * The radio channels to scan as defined in 3GPP TS 25.101 and 36.101.
     * Maximum length of the vector is 32.
     */
    int[] channels;
}
