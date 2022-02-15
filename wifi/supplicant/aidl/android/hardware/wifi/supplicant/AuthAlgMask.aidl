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

package android.hardware.wifi.supplicant;

/**
 * Possible mask of values for AuthAlg param.
 * See /external/wpa_supplicant_8/src/common/defs.h for
 * the historical values (starting at WPA_AUTH_ALG_OPEN).
 */
@VintfStability
@Backing(type="int")
enum AuthAlgMask {
    OPEN = 1 << 0,
    SHARED = 1 << 1,
    LEAP = 1 << 2,
    SAE = 1 << 4,
}
