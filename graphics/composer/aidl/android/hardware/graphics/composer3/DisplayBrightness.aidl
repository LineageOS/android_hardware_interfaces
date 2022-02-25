/**
 * Copyright 2021, The Android Open Source Project
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

@VintfStability
parcelable DisplayBrightness {
    /**
     * A number between 0.0f (minimum brightness) and 1.0f (maximum brightness), a negative value to
     * turn the backlight off
     */
    float brightness;

    /**
     * The absolute brightness in nits of the backlight, if it is available. This will be a negative
     * value if it is not known.
     *
     * An implementation may choose to use this value to assist with tone-mapping, as a mapping
     * between the brightness float and the nits may not otherwise be known.
     */
    float brightnessNits;
}
