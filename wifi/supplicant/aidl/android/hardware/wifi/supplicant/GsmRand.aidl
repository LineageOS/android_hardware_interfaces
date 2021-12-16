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
 * Byte array with expected length 16. Used by NetworkRequestEapSimGsmAuthParams
 * to pass an array of byte arrays, as 2D arrays are not supported in AIDL.
 *
 * TODO (b/210705533): Replace this type with a 2D byte array.
 */
@VintfStability
parcelable GsmRand {
    byte[/* 16 */] data;
}
