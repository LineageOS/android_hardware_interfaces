/*
 * Copyright (C) 2021 The Android Open Source Project
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
 * Information on storage device including life time estimates, end of life
 * information and other attributes.
 *
 * All integers in this struct must be interpreted as non-negative.
 */
@VintfStability
parcelable StorageInfo {
    /**
     * pre-eol (end of life) information. Follows JEDEC standard No.84-B50.
     *
     * Value must be interpreted as non-negative.
     */
    int eol;
    /**
     * device life time estimation (type A). Follows JEDEC standard No.84-B50.
     *
     * Value must be interpreted as non-negative.
     */
    int lifetimeA;
    /**
     * device life time estimation (type B). Follows JEDEC standard No.84-B50.
     *
     * Value must be interpreted as non-negative.
     */
    int lifetimeB;
    /**
     * version string
     */
    String version;
}
