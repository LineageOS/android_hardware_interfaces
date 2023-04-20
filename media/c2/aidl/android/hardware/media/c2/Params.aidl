/*
 * Copyright (C) 2022 The Android Open Source Project
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

package android.hardware.media.c2;

/**
 * Flattened representation of std::vector<C2Param> object.
 *
 * The `Params` type is an array of bytes made up by concatenating a list of
 * C2Param objects. The start index (offset into @ref Params) of each C2Param
 * object in the list is divisible by 8. Up to 7 padding bytes may be added
 * after each C2Param object to achieve this 64-bit alignment.
 *
 * Each C2Param object has the following layout:
 * - 4 bytes: C2Param structure index (of type @ref ParamIndex) identifying the
 *   type of the C2Param object.
 * - 4 bytes: size of the C2Param object (unsigned 4-byte integer).
 * - (size - 8) bytes: data of the C2Param object.
 */
@VintfStability
parcelable Params {
    byte[] params;
}
