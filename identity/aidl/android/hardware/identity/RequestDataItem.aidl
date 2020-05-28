/*
 * Copyright 2020 The Android Open Source Project
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

package android.hardware.identity;

@VintfStability
parcelable RequestDataItem {
    /**
     * The data item name being requested, for example "driving_privileges".
     */
    @utf8InCpp String name;

    /**
     * The size of the data item value.
     *
     * Data item values are always encoded as CBOR so this is the length of
     * the CBOR encoding of the value.
     */
    long size;

    /**
     * The access control profile ids this data item is configured with.
     */
    int[] accessControlProfileIds;
}
