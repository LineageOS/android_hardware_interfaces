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

package android.hardware.gnss.gnss_assistance;

/**
 * Contains Galileo ionospheric model.
 * This is Defined in Galileo-OS-SIS-ICD-v2.1, 5.1.6.
 *
 * @hide
 */
@VintfStability
parcelable GalileoIonosphericModel {
    /** Effective ionisation level 1st order parameter in sfu. */
    double ai0;

    /** Effective ionisation level 2nd order parameter in sfu per degree. */
    double ai1;

    /** Effective ionisation level 3nd order parameter in sfu per degree squared. */
    double ai2;
}
