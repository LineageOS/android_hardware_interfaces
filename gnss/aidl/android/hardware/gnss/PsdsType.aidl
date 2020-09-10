/*
 * Copyright (C) 2020 The Android Open Source Project
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

package android.hardware.gnss;

/** The type of PSDS data. */
@VintfStability
@Backing(type="int")
enum PsdsType {

    /**
     * Long-Term type PSDS data, which lasts for many hours to several days and often provides
     * satellite orbit and clock accuracy of 2 - 20 meters.
     */
    LONG_TERM = 1,

    /**
     * Normal type PSDS data, which is similar to broadcast ephemeris in longevity - lasting for
     * hours and providings satellite orbit and clock accuracy of 1 - 2 meters.
     */
    NORMAL = 2,

    /**
     * Real-Time type PSDS data, which lasts for minutes and provides brief satellite status
     * information such as temporary malfunction, but does not include satellite orbit or clock
     * information.
     */
    REALTIME = 3,
}
