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

package android.hardware.gnss;
/* @hide */
@VintfStability
interface IGnssCallback {
  void gnssSetCapabilitiesCb(in int capabilities);
  void gnssStatusCb(in android.hardware.gnss.IGnssCallback.GnssStatusValue status);
  void gnssSvStatusCb(in android.hardware.gnss.IGnssCallback.GnssSvInfo[] svInfoList);
  void gnssLocationCb(in android.hardware.gnss.GnssLocation location);
  void gnssNmeaCb(in long timestamp, in @utf8InCpp String nmea);
  void gnssAcquireWakelockCb();
  void gnssReleaseWakelockCb();
  void gnssSetSystemInfoCb(in android.hardware.gnss.IGnssCallback.GnssSystemInfo info);
  void gnssRequestTimeCb();
  void gnssRequestLocationCb(in boolean independentFromGnss, in boolean isUserEmergency);
  void gnssSetSignalTypeCapabilitiesCb(in android.hardware.gnss.GnssSignalType[] gnssSignalTypes);
  const int CAPABILITY_SCHEDULING = (1 << 0) /* 1 */;
  const int CAPABILITY_MSB = (1 << 1) /* 2 */;
  const int CAPABILITY_MSA = (1 << 2) /* 4 */;
  const int CAPABILITY_SINGLE_SHOT = (1 << 3) /* 8 */;
  const int CAPABILITY_ON_DEMAND_TIME = (1 << 4) /* 16 */;
  const int CAPABILITY_GEOFENCING = (1 << 5) /* 32 */;
  const int CAPABILITY_MEASUREMENTS = (1 << 6) /* 64 */;
  const int CAPABILITY_NAV_MESSAGES = (1 << 7) /* 128 */;
  const int CAPABILITY_LOW_POWER_MODE = (1 << 8) /* 256 */;
  const int CAPABILITY_SATELLITE_BLOCKLIST = (1 << 9) /* 512 */;
  const int CAPABILITY_MEASUREMENT_CORRECTIONS = (1 << 10) /* 1024 */;
  const int CAPABILITY_ANTENNA_INFO = (1 << 11) /* 2048 */;
  const int CAPABILITY_CORRELATION_VECTOR = (1 << 12) /* 4096 */;
  const int CAPABILITY_SATELLITE_PVT = (1 << 13) /* 8192 */;
  const int CAPABILITY_MEASUREMENT_CORRECTIONS_FOR_DRIVING = (1 << 14) /* 16384 */;
  const int CAPABILITY_ACCUMULATED_DELTA_RANGE = (1 << 15) /* 32768 */;
  @Backing(type="int") @VintfStability
  enum GnssStatusValue {
    NONE = 0,
    SESSION_BEGIN = 1,
    SESSION_END = 2,
    ENGINE_ON = 3,
    ENGINE_OFF = 4,
  }
  @Backing(type="int") @VintfStability
  enum GnssSvFlags {
    NONE = 0,
    HAS_EPHEMERIS_DATA = (1 << 0) /* 1 */,
    HAS_ALMANAC_DATA = (1 << 1) /* 2 */,
    USED_IN_FIX = (1 << 2) /* 4 */,
    HAS_CARRIER_FREQUENCY = (1 << 3) /* 8 */,
  }
  @VintfStability
  parcelable GnssSvInfo {
    int svid;
    android.hardware.gnss.GnssConstellationType constellation;
    float cN0Dbhz;
    float basebandCN0DbHz;
    float elevationDegrees;
    float azimuthDegrees;
    long carrierFrequencyHz;
    int svFlag;
  }
  @VintfStability
  parcelable GnssSystemInfo {
    int yearOfHw;
    @utf8InCpp String name;
  }
}
