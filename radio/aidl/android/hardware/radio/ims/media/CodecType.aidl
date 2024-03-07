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

/** @hide */
@VintfStability
@Backing(type="int")
@JavaDerive(toString=true)
enum CodecType {
    /** Adaptive Multi-Rate */
    AMR = 1 << 0,
    /** Adaptive Multi-Rate Wide Band */
    AMR_WB = 1 << 1,
    /** Enhanced Voice Services */
    EVS = 1 << 2,
    /** G.711 A-law i.e. Pulse Code Modulation using A-law */
    PCMA = 1 << 3,
    /** G.711 μ-law i.e. Pulse Code Modulation using μ-law */
    PCMU = 1 << 4,
}
