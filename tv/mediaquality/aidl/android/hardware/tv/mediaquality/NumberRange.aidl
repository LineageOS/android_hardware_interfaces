/*
 * Copyright (C) 2024 The Android Open Source Project
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

package android.hardware.tv.mediaquality;

/** Parameter supported number range **/
@VintfStability
union NumberRange {
    /** Min value and max value of an int parameter. Inclusive. */
    @nullable int[2] intMinMax;
    /** Min value and max value of a long parameter. Inclusive. */
    @nullable long[2] longMinMax;
    /** Min value and max value of a double parameter. Inclusive. */
    @nullable double[2] doubleMinMax;

    /** An array of supported int values. */
    @nullable int[] intValuesSupported;
    /** An array of supported long values. */
    @nullable long[] longValuesSupported;
    /** An array of supported double values. */
    @nullable double[] doubleValuesSupported;
}
