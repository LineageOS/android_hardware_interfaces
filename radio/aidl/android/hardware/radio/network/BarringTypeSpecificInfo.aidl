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

package android.hardware.radio.network;

/** @hide */
@VintfStability
@JavaDerive(toString=true)
parcelable BarringTypeSpecificInfo {
    /**
     * The barring factor as a percentage 0-100
     */
    int factor;
    /**
     * The number of seconds between re-evaluations of barring
     */
    int timeSeconds;
    /**
     * Indicates whether barring is currently being applied.
     *
     * <p>True if the UE applies barring to a conditionally barred service based on the conditional
     * barring parameters.
     *
     * <p>False if the service is conditionally barred but barring is not currently applied, which
     * could be due to either the barring criteria not having been evaluated (if the UE has not
     * attempted to use the service) or due to the criteria being evaluated and the UE being
     * permitted to use the service despite conditional barring.
     */
    boolean isBarred;
}
