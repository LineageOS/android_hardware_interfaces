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
 * Audio stream coding format.
 * @hide
 */
@VintfStability
@Backing(type="int")
enum AudioStreamType {
    UNDEFINED,

    /*
     * Uncompressed Audio
     */
    PCM,

    /*
     * MPEG Audio Layer III versions
     */
    MP3,

    /*
     * ISO/IEC 11172 Audio
     */
    MPEG1,

    /*
     * ISO/IEC 13818-3
     */
    MPEG2,

    /*
     * ISO/IEC 23008-3 (MPEG-H Part 3)
     */
    MPEGH,

    /*
     * ISO/IEC 14496-3
     */
    AAC,

    /*
     * Dolby Digital
     */
    AC3,

    /*
     * Dolby Digital Plus
     */
    EAC3,

    /*
     * Dolby AC-4
     */
    AC4,

    /*
     * Basic DTS
     */
    DTS,

    /*
     * High Resolution DTS
     */
    DTS_HD,

    /*
     * Windows Media Audio
     */
    WMA,

    /*
     * Opus Interactive Audio Codec
     */
    OPUS,

    /*
     * VORBIS Interactive Audio Codec
     */
    VORBIS,

    /*
     * SJ/T 11368-2006
     */
    DRA,

    /*
     * AAC with ADTS (Audio Data Transport Format).
     */
    AAC_ADTS,

    /*
     * AAC with ADTS with LATM (Low-overhead MPEG-4 Audio Transport Multiplex).
     */
    AAC_LATM,

    /*
     * High-Efficiency AAC (HE-AAC) with ADTS (Audio Data Transport Format).
     */
    AAC_HE_ADTS,

    /*
     * High-Efficiency AAC (HE-AAC) with LATM (Low-overhead MPEG-4 Audio Transport Multiplex).
     */
    AAC_HE_LATM,
}
