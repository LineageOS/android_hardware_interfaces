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

package android.hardware.graphics.composer3.command;

@VintfStability
parcelable WhitePointNits {
    /**
     * The desired white point for the layer. This is intended to be used when presenting
     * an SDR layer alongside HDR content. The HDR content will be presented at the display
     * brightness in nits, and accordingly SDR content shall be dimmed to the desired white point
     * provided.
     * @see LayerCommand.whitePointNits.
     */
    float nits;
}
