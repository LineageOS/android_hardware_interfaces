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
///////////////////////////////////////////////////////////////////////////////
// THIS FILE IS IMMUTABLE. DO NOT EDIT IN ANY CASE.                          //
///////////////////////////////////////////////////////////////////////////////

// This file is a snapshot of an AIDL file. Do not edit it manually. There are
// two cases:
// 1). this is a frozen version file - do not edit this in any case.
// 2). this is a 'current' file. If you make a backwards compatible change to
//     the interface (from the latest frozen version), the build system will
//     prompt you to update this file with `m <name>-update-api`.
//
// You must not make a backward incompatible change to any AIDL file built
// with the aidl_interface module type with versions property set. The module
// type is used to build AIDL files in a way that they can be used across
// independently updatable components of the system. If a device is shipped
// with such a backward incompatible change, it has a high risk of breaking
// later when a module using the interface is updated, e.g., Mainline modules.

package android.hardware.gnss.gnss_assistance;
/* @hide */
@VintfStability
parcelable GnssAssistance {
  android.hardware.gnss.gnss_assistance.GnssAssistance.GpsAssistance gpsAssistance;
  android.hardware.gnss.gnss_assistance.GnssAssistance.GlonassAssistance glonassAssistance;
  android.hardware.gnss.gnss_assistance.GnssAssistance.GalileoAssistance galileoAssistance;
  android.hardware.gnss.gnss_assistance.GnssAssistance.BeidouAssistance beidouAssistance;
  android.hardware.gnss.gnss_assistance.GnssAssistance.QzssAssistance qzssAssistance;
  @VintfStability
  parcelable GnssSatelliteCorrections {
    int svid;
    android.hardware.gnss.gnss_assistance.IonosphericCorrection[] inonosphericCorrections;
  }
  @VintfStability
  parcelable GpsAssistance {
    android.hardware.gnss.gnss_assistance.GnssAlmanac almanac;
    android.hardware.gnss.gnss_assistance.KlobucharIonosphericModel ionosphericModel;
    android.hardware.gnss.gnss_assistance.UtcModel utcModel;
    android.hardware.gnss.gnss_assistance.LeapSecondsModel leapSecondsModel;
    android.hardware.gnss.gnss_assistance.TimeModel[] timeModels;
    android.hardware.gnss.gnss_assistance.GpsSatelliteEphemeris[] satelliteEphemeris;
    android.hardware.gnss.gnss_assistance.RealTimeIntegrityModel[] realTimeIntegrityModels;
    android.hardware.gnss.gnss_assistance.GnssAssistance.GnssSatelliteCorrections[] satelliteCorrections;
  }
  @VintfStability
  parcelable GalileoAssistance {
    android.hardware.gnss.gnss_assistance.GnssAlmanac almanac;
    android.hardware.gnss.gnss_assistance.GalileoIonosphericModel ionosphericModel;
    android.hardware.gnss.gnss_assistance.UtcModel utcModel;
    android.hardware.gnss.gnss_assistance.LeapSecondsModel leapSecondsModel;
    android.hardware.gnss.gnss_assistance.TimeModel[] timeModels;
    android.hardware.gnss.gnss_assistance.GalileoSatelliteEphemeris[] satelliteEphemeris;
    android.hardware.gnss.gnss_assistance.RealTimeIntegrityModel[] realTimeIntegrityModels;
    android.hardware.gnss.gnss_assistance.GnssAssistance.GnssSatelliteCorrections[] satelliteCorrections;
  }
  @VintfStability
  parcelable GlonassAssistance {
    android.hardware.gnss.gnss_assistance.GlonassAlmanac almanac;
    android.hardware.gnss.gnss_assistance.UtcModel utcModel;
    android.hardware.gnss.gnss_assistance.TimeModel[] timeModels;
    android.hardware.gnss.gnss_assistance.GlonassSatelliteEphemeris[] satelliteEphemeris;
    android.hardware.gnss.gnss_assistance.GnssAssistance.GnssSatelliteCorrections[] satelliteCorrections;
  }
  @VintfStability
  parcelable QzssAssistance {
    android.hardware.gnss.gnss_assistance.GnssAlmanac almanac;
    android.hardware.gnss.gnss_assistance.KlobucharIonosphericModel ionosphericModel;
    android.hardware.gnss.gnss_assistance.UtcModel utcModel;
    android.hardware.gnss.gnss_assistance.LeapSecondsModel leapSecondsModel;
    android.hardware.gnss.gnss_assistance.TimeModel[] timeModels;
    android.hardware.gnss.gnss_assistance.QzssSatelliteEphemeris[] satelliteEphemeris;
    android.hardware.gnss.gnss_assistance.RealTimeIntegrityModel[] realTimeIntegrityModels;
    android.hardware.gnss.gnss_assistance.GnssAssistance.GnssSatelliteCorrections[] satelliteCorrections;
  }
  @VintfStability
  parcelable BeidouAssistance {
    android.hardware.gnss.gnss_assistance.GnssAlmanac almanac;
    android.hardware.gnss.gnss_assistance.KlobucharIonosphericModel ionosphericModel;
    android.hardware.gnss.gnss_assistance.UtcModel utcModel;
    android.hardware.gnss.gnss_assistance.LeapSecondsModel leapSecondsModel;
    android.hardware.gnss.gnss_assistance.TimeModel[] timeModels;
    android.hardware.gnss.gnss_assistance.BeidouSatelliteEphemeris[] satelliteEphemeris;
    android.hardware.gnss.gnss_assistance.RealTimeIntegrityModel[] realTimeIntegrityModels;
    android.hardware.gnss.gnss_assistance.GnssAssistance.GnssSatelliteCorrections[] satelliteCorrections;
  }
}
