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

import android.hardware.media.c2.FieldDescriptor;

/**
 * Description of a C2Param structure. It consists of an index and a list of
 * `FieldDescriptor`s.
 */
@VintfStability
parcelable StructDescriptor {
    /**
     * Index of the structure.
     *
     * Actually C2Param::CoreIndex
     * Core index is the underlying parameter type for a parameter. It is used to describe the
     * layout of the parameter structure regardless of the component or parameter kind/scope.
     *
     * It is used to identify and distinguish global parameters, and also parameters on a given
     * port or stream. They must be unique for the set of global parameters, as well as for the
     * set of parameters on each port or each stream, but the same core index can be used for
     * parameters on different streams or ports, as well as for global parameters and port/stream
     * parameters.
     */
    int type;
    /**
     * List of fields in the structure.
     *
     * Fields are ordered by their offsets. A field that is a structure is
     * ordered before its members.
     */
    FieldDescriptor[] fields;
}
