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

package android.hardware.radio;

@VintfStability
@Backing(type="int")
enum RadioCdmaSmsConst {
    ADDRESS_MAX = 36,
    SUBADDRESS_MAX = 36,
    BEARER_DATA_MAX = 255,
    UDH_MAX_SND_SIZE = 128,
    UDH_EO_DATA_SEGMENT_MAX = 131,
    MAX_UD_HEADERS = 7,
    USER_DATA_MAX = 229,
    UDH_LARGE_PIC_SIZE = 128,
    UDH_SMALL_PIC_SIZE = 32,
    UDH_VAR_PIC_SIZE = 134,
    UDH_ANIM_NUM_BITMAPS = 4,
    UDH_LARGE_BITMAP_SIZE = 32,
    UDH_SMALL_BITMAP_SIZE = 8,
    UDH_OTHER_SIZE = 226,
    IP_ADDRESS_SIZE = 4,
}
