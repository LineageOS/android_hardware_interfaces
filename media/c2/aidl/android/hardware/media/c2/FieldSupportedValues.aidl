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

import android.hardware.media.c2.ValueRange;

/*
 * Description of supported values for a field of C2Param.
 *
 * This can be a continuous range or a discrete set of values.
 *
 * The intended type of values must be made clear in the context where
 * `FieldSupportedValues` is used.
 */
@VintfStability
union FieldSupportedValues {
    /**
     * No supported values
     */
    boolean empty;
    /**
     * Numeric range, described in a #ValueRange structure
     */
    ValueRange range;
    /**
     * List of values
     */
    long[] values;
    /**
     * List of flags that can be OR-ed.
     *
     * The list contains { min-mask, flag1, flag2... }. Basically, the first
     * value is the required set of flags to be set, and the rest of the values are flags that can
     * be set independently. FLAGS is only supported for integral types. Supported flags should
     * not overlap, as it can make validation non-deterministic. The standard validation method
     * is that starting from the original value, if each flag is removed when fully present (the
     * min-mask must be fully present), we shall arrive at 0.
     */
    long[] flags;
}
