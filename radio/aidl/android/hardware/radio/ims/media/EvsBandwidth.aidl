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

/**
 * EVS Speech codec bandwidths, See 3gpp spec 26.441 Table 1
 * @hide
 */
@VintfStability
@Backing(type="int")
@JavaDerive(toString=true)
enum EvsBandwidth {
    NONE = 0,
    NARROW_BAND = 1 << 0,
    WIDE_BAND = 1 << 1,
    SUPER_WIDE_BAND = 1 << 2,
    FULL_BAND = 1 << 3,
}
