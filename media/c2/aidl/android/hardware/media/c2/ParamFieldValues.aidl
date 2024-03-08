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

import android.hardware.media.c2.FieldSupportedValues;
import android.hardware.media.c2.ParamField;

/**
 * Supported values for a field.
 *
 * This is a pair of the field specifier together with an optional supported
 * values object. This structure is used when reporting parameter configuration
 * failures and conflicts.
 */
@VintfStability
parcelable ParamFieldValues {
    /**
     * Reference to a field or a C2Param structure.
     */
    ParamField paramOrField;
    /**
     * Optional supported values for the field if #paramOrField specifies an
     * actual field that is numeric (non struct, blob or string). Supported
     * values for arrays (including string and blobs) describe the supported
     * values for each element (character for string, and bytes for blobs). It
     * is optional for read-only strings and blobs.
     */
    FieldSupportedValues[] values;
}
