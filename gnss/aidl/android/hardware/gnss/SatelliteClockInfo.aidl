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
 * Contains estimates of the satellite clock info.
 */
@VintfStability
parcelable SatelliteClockInfo {
    /**
     * Satellite hardware code bias of the reported code type w.r.t
     * ionosphere-free measurement in meters.
     *
     * When broadcast ephemeris is used, this is the offset caused
     * by the satellite hardware delays at different frequencies;
     * e.g. in IS-GPS-705D, this term is described in Section
     * 20.3.3.3.1.2.1.
     *
     * For GPS this term is ~10ns, and affects the satellite position
     * computation by less than a millimeter.
     */
    double satHardwareCodeBiasMeters;

    /**
     * Satellite time correction for ionospheric-free signal measurement
     * (meters). The satellite clock correction for the given signal type
     * = satTimeCorrectionMeters - satHardwareCodeBiasMeters.
     *
     * When broadcast ephemeris is used, this is the offset modeled in the
     * clock terms broadcast over the air by the satellites;
     * e.g. in IS-GPS-200H, Section 20.3.3.3.3.1, this term is
     * ∆tsv = af0 + af1(t - toc) + af2(t - toc)^2 + ∆tr.
     *
     * If another source of ephemeris is used for SatellitePvt, then the
     * equivalent value of satTimeCorrection must be provided.
     *
     * For GPS this term is ~1ms, and affects the satellite position
     * computation by ~1m.
     */
    double satTimeCorrectionMeters;

    /** Satellite clock drift (meters per second). */
    double satClkDriftMps;
}
