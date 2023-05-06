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

package android.hardware.radio.data;

/**
 * This struct represents the OsId + OsAppId as defined in TS 24.526 Section 5.2
 * @hide
 */
@VintfStability
@JavaDerive(toString=true)
parcelable OsAppId {
    /**
     * Byte array representing OsId + OsAppId. The minimum length of the array is 18 and maximum
     * length is 272 (16 bytes for OsId + 1 byte for OsAppId length + up to 255 bytes for OsAppId).
     */
    byte[] osAppId;
}
