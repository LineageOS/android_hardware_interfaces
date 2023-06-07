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
import android.hardware.media.c2.Status;

/**
 * This structure is used to hold the result from
 * IConfigurable::querySupportedValues().
 */
@VintfStability
parcelable FieldSupportedValuesQueryResult {
    /**
     * Result of the query. Possible values are
     * - `Status::OK`: The query was successful.
     * - `Status::BAD_STATE`: The query was requested when the `IConfigurable` instance
     *   was in a bad state.
     * - `Status::BAD_INDEX`: The requested field was not recognized.
     * - `Status::TIMED_OUT`: The query could not be completed in a timely manner.
     * - `Status::BLOCKING`: The query must block, but the parameter `mayBlock` in the
     *   call to `querySupportedValues()` was `false`.
     * - `Status::CORRUPTED`: Some unknown error occurred.
     */
    Status status;
    /**
     * Supported values. This is meaningful only when #status is `OK`.
     */
    FieldSupportedValues values;
}
