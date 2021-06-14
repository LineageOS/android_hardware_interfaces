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
enum CdmaSmsNumberPlan {
    UNKNOWN,
    /**
     * CCITT E.164 and E.163, including ISDN plan
     */
    TELEPHONY,
    RESERVED_2,
    /**
     * CCITT X.121
     */
    DATA,
    /**
     * CCITT F.69
     */
    TELEX,
    RESERVED_5,
    RESERVED_6,
    RESERVED_7,
    RESERVED_8,
    PRIVATE,
    RESERVED_10,
    RESERVED_11,
    RESERVED_12,
    RESERVED_13,
    RESERVED_14,
    RESERVED_15,
}
