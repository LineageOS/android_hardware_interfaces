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

@VintfStability
@Backing(type="byte")
enum FormatColorComponent {
    /*
     * The first component  (eg, for RGBA_8888, this is R)
     */
    FORMAT_COMPONENT_0 = 1 << 0,
    /*
     * The second component (eg, for RGBA_8888, this is G)
     */
    FORMAT_COMPONENT_1 = 1 << 1,
    /*
     * The third component  (eg, for RGBA_8888, this is B)
     */
    FORMAT_COMPONENT_2 = 1 << 2,
    /*
     * The fourth component (eg, for RGBA_8888, this is A)
     */
    FORMAT_COMPONENT_3 = 1 << 3,
}
