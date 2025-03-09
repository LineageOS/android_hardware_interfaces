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

package android.hardware.health;

/*
 * Information on foldable hinge health life time estimates, end of life
 * information and other attributes.
 *
 * All integers in this struct must be interpreted as non-negative.
 */
@VintfStability
parcelable HingeInfo {
    /**
     * returns count of times a given hinge has been folded.
     *
     * opening fully counts as 1 fold and closing fully counts as another.
     * The hinge has to engage in its full range of motion to be considered
     * a fold. Partial folds must not be counted.
     */
    int numTimesFolded;
    /**
     * returns the expected lifespan of hinge in units of number of folds
     */
    int expectedHingeLifespan;
}
