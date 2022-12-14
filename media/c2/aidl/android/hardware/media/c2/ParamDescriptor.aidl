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
 * Usage description of a C2Param structure.
 *
 * @ref ParamDescriptor is returned by IConfigurable::querySupportedParams().
 */
@VintfStability
parcelable ParamDescriptor {
    /** The list of bit flags for attrib */
    /**
     * The parameter is required to be specified.
     */
    const int ATTRIBUTE_REQUIRED = 1 << 0;
    /**
     * The parameter retains its value.
     */
    const int ATTRIBUTE_PERSISTENT = 1 << 1;
    /**
     * The parameter is strict.
     */
    const int ATTRIBUTE_STRICT = 1 << 2;
    /**
     * The parameter is publicly read-only.
     */
    const int ATTRIBUTE_READ_ONLY = 1 << 3;
    /**
     * The parameter must not be visible to clients.
     */
    const int ATTRIBUTE_HIDDEN = 1 << 4;
    /**
     * The parameter must not be used by framework (other than testing).
     */
    const int ATTRIBUTE_INTERNAL = 1 << 5;
    /**
     * The parameter is publicly constant (hence read-only).
     */
    const int ATTRIBUTE_CONST = 1 << 6;

    /**
     * Index of the C2Param structure being described.
     */
    int index;
    /**
     * bit flag for attribute defined in the above.
     */
    int attrib;
    /**
     * Name of the structure. This must be unique for each structure.
     */
    String name;
    /**
     * Indices of other C2Param structures that this C2Param structure depends
     * on.
     */
    int[] dependencies;
}
