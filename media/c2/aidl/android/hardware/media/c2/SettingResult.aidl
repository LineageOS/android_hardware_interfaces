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

import android.hardware.media.c2.ParamFieldValues;

/**
 * Information describing the reason the parameter settings may fail, or may be
 * overridden.
 */
@VintfStability
parcelable SettingResult {
    /**
     * Failure code
     */
    @VintfStability
    @Backing(type="int")
    enum Failure {
        /**
         * Parameter is not supported.
         */
        BAD_TYPE,
        /**
         * Parameter is not supported on the specific port.
         */
        BAD_PORT,
        /**
         * Parameter is not supported on the specific stream.
         */
        BAD_INDEX,
        /**
         * Parameter is read-only and cannot be set.
         */
        READ_ONLY,
        /**
         * Parameter mismatches input data.
         */
        MISMATCH,
        /**
         * Strict parameter does not accept value for the field at all.
         */
        BAD_VALUE,
        /**
         * Strict parameter field value is in conflict with an/other
         * setting(s).
         */
        CONFLICT,
        /**
         * Parameter field is out of range due to other settings. (This failure
         * mode can only be used for strict calculated parameters.)
         */
        UNSUPPORTED,
        /**
         * Field does not access the requested parameter value at all. It has
         * been corrected to the closest supported value. This failure mode is
         * provided to give guidance as to what are the currently supported
         * values for this field (which may be a subset of the at-all-potential
         * values).
         */
        INFO_BAD_VALUE,
        /**
         * Requested parameter value is in conflict with an/other setting(s)
         * and has been corrected to the closest supported value. This failure
         * mode is given to provide guidance as to what are the currently
         * supported values as well as to optionally provide suggestion to the
         * client as to how to enable the requested parameter value.
         */
        INFO_CONFLICT,
    }
    Failure failure;
    /**
     * Failing (or corrected) field or parameter and optionally, currently
     * supported values for the field. Values must only be set for field
     * failures other than `BAD_VALUE`, and only if they are different from the
     * globally supported values (e.g. due to restrictions by another parameter
     * or input data).
     */
    ParamFieldValues field;
    /**
     * Conflicting parameters or fields with (optional) suggested values for any
     * conflicting fields to avoid the conflict. Values must only be set for
     * `CONFLICT`, `UNSUPPORTED` or `INFO_CONFLICT` failure code.
     */
    ParamFieldValues[] conflicts;
}
