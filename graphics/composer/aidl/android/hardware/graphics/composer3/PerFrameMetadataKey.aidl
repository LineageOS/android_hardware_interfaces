/**
 * Copyright (c) 2021, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.hardware.graphics.composer3;

/**
 * PerFrameMetadataKey
 *
 * A set of PerFrameMetadataKey pertains specifically to blob-formatted
 * metadata (as opposed to float-valued metadata).
 * The list of keys that represent blobs are:
 * 1. HDR10_PLUS_SEI
 */
@VintfStability
@Backing(type="int")
enum PerFrameMetadataKey {
    /**
     * SMPTE ST 2084:2014.
     * Coordinates defined in CIE 1931 xy chromaticity space
     *
     *
     * SMPTE ST 2084:2014
     */
    DISPLAY_RED_PRIMARY_X,
    /**
     * SMPTE ST 2084:2014
     */
    DISPLAY_RED_PRIMARY_Y,
    /**
     * SMPTE ST 2084:2014
     */
    DISPLAY_GREEN_PRIMARY_X,
    /**
     * SMPTE ST 2084:2014
     */
    DISPLAY_GREEN_PRIMARY_Y,
    /**
     * SMPTE ST 2084:2014
     */
    DISPLAY_BLUE_PRIMARY_X,
    /**
     * SMPTE ST 2084:2014
     */
    DISPLAY_BLUE_PRIMARY_Y,
    /**
     * SMPTE ST 2084:2014
     */
    WHITE_POINT_X,
    /**
     * SMPTE ST 2084:2014
     */
    WHITE_POINT_Y,
    /**
     * SMPTE ST 2084:2014.
     * Units: nits
     * max as defined by ST 2048: 10,000 nits
     */
    MAX_LUMINANCE,
    /**
     * SMPTE ST 2084:2014
     */
    MIN_LUMINANCE,
    /**
     * CTA 861.3
     */
    MAX_CONTENT_LIGHT_LEVEL,
    /**
     * CTA 861.3
     */
    MAX_FRAME_AVERAGE_LIGHT_LEVEL,
    /**
     * HDR10+ metadata
     * Specifies a metadata blob adhering to
     * the ST2094-40 SEI message spec, Version 1.0
     */
    HDR10_PLUS_SEI,
}
