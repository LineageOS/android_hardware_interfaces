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
 * Video stream coding format.
 * @hide
 */
@VintfStability
@Backing(type="int")
enum VideoStreamType {
    UNDEFINED,

    /*
     * ITU-T | ISO/IEC Reserved
     */
    RESERVED,

    /*
     * ISO/IEC 11172
     */
    MPEG1,

    /*
     * ITU-T Rec.H.262 and ISO/IEC 13818-2
     */
    MPEG2,

    /*
     * ISO/IEC 14496-2 (MPEG-4 H.263 based video)
     */
    MPEG4P2,

    /*
     * ITU-T Rec.H.264 and ISO/IEC 14496-10
     */
    AVC,

    /*
     * ITU-T Rec. H.265 and ISO/IEC 23008-2
     */
    HEVC,

    /*
     * Microsoft VC.1
     */
    VC1,

    /*
     * Google VP8
     */
    VP8,

    /*
     * Google VP9
     */
    VP9,

    /*
     * AOMedia Video 1
     */
    AV1,

    /*
     * Chinese Standard
     */
    AVS,

    /*
     * New Chinese Standard
     */
    AVS2,

    /*
     * ITU-T Rec. H.266 and ISO/IEC 23090-3
     */
    VVC,
}
