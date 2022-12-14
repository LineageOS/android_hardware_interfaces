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

import android.hardware.media.c2.FieldId;

/**
 * Description of a field inside a C2Param structure.
 */
@VintfStability
parcelable FieldDescriptor {
    /**
     * Possible types of the field.
     */
    @VintfStability
    @Backing(type="int")
    enum Type {
        NO_INIT = 0,
        INT32,
        UINT32,
        CNTR32,
        INT64,
        UINT64,
        CNTR64,
        FLOAT,
        /**
         * Fixed-size string (POD).
         */
        STRING = 0x100,
        /**
         * A blob has no sub-elements and can be thought of as an array of
         * bytes. However, bytes cannot be individually addressed by clients.
         */
        BLOB,
        /**
         * The field is a structure that may contain other fields.
         */
        STRUCT = 0x20000,
    }
    /**
     * Named value type. This is used for defining an enum value for a numeric
     * type.
     */
    @VintfStability
    parcelable NamedValue {
        /**
         * Name of the enum value. This must be unique for each enum value in
         * the same field.
         */
        String name;
        /**
         * Underlying value of the enum value. Multiple enum names may have the
         * same underlying value.
         */
        long value;
    }
    /**
     * Location of the field in the C2Param structure
     */
    FieldId fieldId;
    /**
     * Type of the field.
     */
    Type type;
    /**
     * If #type is #Type.STRUCT, #structIndex is the C2Param structure index;
     * otherwise, #structIndex is not used.
     */
    int structIndex;
    /**
     * Extent of the field.
     * - For a non-array field, #extent is 1.
     * - For a fixed-length array field, #extent is the length. An array field
     *   of length 1 is indistinguishable from a non-array field.
     * - For a variable-length array field, #extent is 0. This can only occur as
     *   the last member of a C2Param structure.
     */
    int extent;
    /**
     * Name of the field. This must be unique for each field in the same
     * structure.
     */
    String name;
    /**
     * List of enum values. This is not used when #type is not one of the
     * numeric types.
     */
    NamedValue[] namedValues;
}
