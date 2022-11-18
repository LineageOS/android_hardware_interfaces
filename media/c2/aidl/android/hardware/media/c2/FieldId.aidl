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
 * Identifying information of a field relative to a known C2Param structure.
 *
 * Within a given C2Param structure, each field is uniquely identified by @ref
 * FieldId.
 */
@VintfStability
parcelable FieldId {
    /**
     * Offset of the field in bytes.
     */
    int offset;
    /**
     * Size of the field in bytes.
     */
    int sizeBytes;
}
