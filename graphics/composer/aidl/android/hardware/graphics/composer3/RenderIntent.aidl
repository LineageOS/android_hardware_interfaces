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
 * RenderIntent defines the mapping from color mode colors to display colors.
 *
 * A render intent must not change how it maps colors when the color mode
 * changes. That is to say that when a render intent maps color C to color C',
 * the fact that color C can have different pixel values in different color
 * modes should not affect the mapping.
 *
 * RenderIntent overrides the render intents defined for individual color
 * modes. It is ignored when the color mode is ColorMode.NATIVE, because
 * ColorMode.NATIVE colors are already display colors.
 */
@VintfStability
@Backing(type="int")
enum RenderIntent {
    /**
     * Colors in the display gamut are unchanged. Colors out of the display
     * gamut are hard-clipped.
     *
     * This implies that the display must have been calibrated unless
     * ColorMode.NATIVE is the only supported color mode.
     */
    COLORIMETRIC = 0,
    /**
     * Enhance colors that are in the display gamut. Colors out of the display
     * gamut are hard-clipped.
     *
     * The enhancement typically picks the biggest standard color space (e.g.
     * DCI-P3) that is narrower than the display gamut and stretches it to the
     * display gamut. The stretching is recommended to preserve skin tones.
     */
    ENHANCE = 1,
    /**
     * Tone map high-dynamic-range colors to the display's dynamic range. The
     * dynamic range of the colors are communicated separately. After tone
     * mapping, the mapping to the display gamut is as defined in
     * COLORIMETRIC.
     */
    TONE_MAP_COLORIMETRIC = 2,
    /**
     * Tone map high-dynamic-range colors to the display's dynamic range. The
     * dynamic range of the colors are communicated separately. After tone
     * mapping, the mapping to the display gamut is as defined in ENHANCE.
     *
     * The tone mapping step and the enhancing step must match
     * TONE_MAP_COLORIMETRIC and ENHANCE respectively when they are also
     * supported.
     */
    TONE_MAP_ENHANCE = 3,
}
