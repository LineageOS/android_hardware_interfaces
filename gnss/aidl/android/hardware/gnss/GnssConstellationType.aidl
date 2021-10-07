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

/**
 * GNSS constellation type
 *
 * This is to specify the navigation satellite system, for example, as listed in Section 3.5 in
 * RINEX Version 3.04.
 */
@VintfStability
@Backing(type="int")
enum GnssConstellationType {
    UNKNOWN = 0,
    /** Global Positioning System. */
    GPS     = 1,
    /** Satellite-Based Augmentation System. */
    SBAS    = 2,
    /** Global Navigation Satellite System. */
    GLONASS = 3,
    /** Quasi-Zenith Satellite System. */
    QZSS    = 4,
    /** BeiDou Navigation Satellite System. */
    BEIDOU  = 5,
    /** Galileo Navigation Satellite System. */
    GALILEO = 6,
    /** Indian Regional Navigation Satellite System. */
    IRNSS   = 7,
}