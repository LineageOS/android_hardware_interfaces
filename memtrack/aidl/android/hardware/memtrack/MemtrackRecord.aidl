/*
 * Copyright (C) 2020 The Android Open Source Project
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

package android.hardware.memtrack;

/*
 * Each record consists of the size of the memory used by the process and
 * flags indicate all the MemtrackFlags that are valid for this record.
 */
@VintfStability
parcelable MemtrackRecord {
    /* Memtrack Flags */
    const int FLAG_SMAPS_ACCOUNTED = 1 << 1;
    const int FLAG_SMAPS_UNACCOUNTED = 1 << 2;
    const int FLAG_SHARED = 1 << 3;
    const int FLAG_SHARED_PSS = 1 << 4;
    const int FLAG_PRIVATE = 1 << 5;
    const int FLAG_SYSTEM = 1 << 6;
    const int FLAG_DEDICATED = 1 << 7;
    const int FLAG_NONSECURE = 1 << 8;
    const int FLAG_SECURE = 1 << 9;

    /* Bitfield indicating all flags that are valid for this record */
    int flags;

    long sizeInBytes;
}

