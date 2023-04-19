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
 * Description of a set of values.
 *
 * If the `step` member is 0, and `num` and `denom` are both 1, the `Range`
 * structure represents a closed interval bounded by `min` and `max`.
 *
 * Otherwise, the #ValueRange structure represents a finite sequence of numbers
 * produced from the following recurrence relation:
 *
 * @code
 * v[0] = min
 * v[i] = v[i - 1] * num / denom + step ; i >= 1
 * @endcode
 *
 * Both the ratio `num / denom` and the value `step` must be positive. The
 * last number in the sequence described by this #Range structure is the
 * largest number in the sequence that is smaller than or equal to `max`.
 *
 * @note
 * The division in the formula may truncate the result if the data type of
 * these values is an integral type.
 */
@VintfStability
parcelable ValueRange {
    /**
     * Lower end of the range (inclusive).
     */
    long min;
    /**
     * Upper end of the range (inclusive).
     */
    long max;
    /**
     * The non-homogeneous term in the recurrence relation.
     */
    long step;
    /**
     * The numerator of the scale coefficient in the recurrence relation.
     */
    long num;
    /**
     * The denominator of the scale coefficient in the recurrence relation.
     */
    long denom;
}
