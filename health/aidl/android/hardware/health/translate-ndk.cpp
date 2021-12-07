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

#include "android/hardware/health/translate-ndk.h"

namespace android::h2a {

static_assert(aidl::android::hardware::health::BatteryStatus::UNKNOWN ==
              static_cast<aidl::android::hardware::health::BatteryStatus>(
                      ::android::hardware::health::V1_0::BatteryStatus::UNKNOWN));
static_assert(aidl::android::hardware::health::BatteryStatus::CHARGING ==
              static_cast<aidl::android::hardware::health::BatteryStatus>(
                      ::android::hardware::health::V1_0::BatteryStatus::CHARGING));
static_assert(aidl::android::hardware::health::BatteryStatus::DISCHARGING ==
              static_cast<aidl::android::hardware::health::BatteryStatus>(
                      ::android::hardware::health::V1_0::BatteryStatus::DISCHARGING));
static_assert(aidl::android::hardware::health::BatteryStatus::NOT_CHARGING ==
              static_cast<aidl::android::hardware::health::BatteryStatus>(
                      ::android::hardware::health::V1_0::BatteryStatus::NOT_CHARGING));
static_assert(aidl::android::hardware::health::BatteryStatus::FULL ==
              static_cast<aidl::android::hardware::health::BatteryStatus>(
                      ::android::hardware::health::V1_0::BatteryStatus::FULL));

static_assert(aidl::android::hardware::health::BatteryHealth::UNKNOWN ==
              static_cast<aidl::android::hardware::health::BatteryHealth>(
                      ::android::hardware::health::V1_0::BatteryHealth::UNKNOWN));
static_assert(aidl::android::hardware::health::BatteryHealth::GOOD ==
              static_cast<aidl::android::hardware::health::BatteryHealth>(
                      ::android::hardware::health::V1_0::BatteryHealth::GOOD));
static_assert(aidl::android::hardware::health::BatteryHealth::OVERHEAT ==
              static_cast<aidl::android::hardware::health::BatteryHealth>(
                      ::android::hardware::health::V1_0::BatteryHealth::OVERHEAT));
static_assert(aidl::android::hardware::health::BatteryHealth::DEAD ==
              static_cast<aidl::android::hardware::health::BatteryHealth>(
                      ::android::hardware::health::V1_0::BatteryHealth::DEAD));
static_assert(aidl::android::hardware::health::BatteryHealth::OVER_VOLTAGE ==
              static_cast<aidl::android::hardware::health::BatteryHealth>(
                      ::android::hardware::health::V1_0::BatteryHealth::OVER_VOLTAGE));
static_assert(aidl::android::hardware::health::BatteryHealth::UNSPECIFIED_FAILURE ==
              static_cast<aidl::android::hardware::health::BatteryHealth>(
                      ::android::hardware::health::V1_0::BatteryHealth::UNSPECIFIED_FAILURE));
static_assert(aidl::android::hardware::health::BatteryHealth::COLD ==
              static_cast<aidl::android::hardware::health::BatteryHealth>(
                      ::android::hardware::health::V1_0::BatteryHealth::COLD));

static_assert(aidl::android::hardware::health::BatteryCapacityLevel::UNSUPPORTED ==
              static_cast<aidl::android::hardware::health::BatteryCapacityLevel>(
                      ::android::hardware::health::V2_1::BatteryCapacityLevel::UNSUPPORTED));
static_assert(aidl::android::hardware::health::BatteryCapacityLevel::UNKNOWN ==
              static_cast<aidl::android::hardware::health::BatteryCapacityLevel>(
                      ::android::hardware::health::V2_1::BatteryCapacityLevel::UNKNOWN));
static_assert(aidl::android::hardware::health::BatteryCapacityLevel::CRITICAL ==
              static_cast<aidl::android::hardware::health::BatteryCapacityLevel>(
                      ::android::hardware::health::V2_1::BatteryCapacityLevel::CRITICAL));
static_assert(aidl::android::hardware::health::BatteryCapacityLevel::LOW ==
              static_cast<aidl::android::hardware::health::BatteryCapacityLevel>(
                      ::android::hardware::health::V2_1::BatteryCapacityLevel::LOW));
static_assert(aidl::android::hardware::health::BatteryCapacityLevel::NORMAL ==
              static_cast<aidl::android::hardware::health::BatteryCapacityLevel>(
                      ::android::hardware::health::V2_1::BatteryCapacityLevel::NORMAL));
static_assert(aidl::android::hardware::health::BatteryCapacityLevel::HIGH ==
              static_cast<aidl::android::hardware::health::BatteryCapacityLevel>(
                      ::android::hardware::health::V2_1::BatteryCapacityLevel::HIGH));
static_assert(aidl::android::hardware::health::BatteryCapacityLevel::FULL ==
              static_cast<aidl::android::hardware::health::BatteryCapacityLevel>(
                      ::android::hardware::health::V2_1::BatteryCapacityLevel::FULL));

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::health::V2_0::StorageInfo& in,
        aidl::android::hardware::health::StorageInfo* out) {
    out->eol = in.eol;
    out->lifetimeA = in.lifetimeA;
    out->lifetimeB = in.lifetimeB;
    out->version = in.version;
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::health::V2_0::DiskStats& in,
        aidl::android::hardware::health::DiskStats* out) {
    out->reads = static_cast<int64_t>(in.reads);
    out->readMerges = static_cast<int64_t>(in.readMerges);
    out->readSectors = static_cast<int64_t>(in.readSectors);
    out->readTicks = static_cast<int64_t>(in.readTicks);
    out->writes = static_cast<int64_t>(in.writes);
    out->writeMerges = static_cast<int64_t>(in.writeMerges);
    out->writeSectors = static_cast<int64_t>(in.writeSectors);
    out->writeTicks = static_cast<int64_t>(in.writeTicks);
    out->ioInFlight = static_cast<int64_t>(in.ioInFlight);
    out->ioTicks = static_cast<int64_t>(in.ioTicks);
    out->ioInQueue = static_cast<int64_t>(in.ioInQueue);
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::health::V2_0::HealthInfo& in,
        aidl::android::hardware::health::HealthInfo* out) {
    out->chargerAcOnline = static_cast<bool>(in.legacy.chargerAcOnline);
    out->chargerUsbOnline = static_cast<bool>(in.legacy.chargerUsbOnline);
    out->chargerWirelessOnline = static_cast<bool>(in.legacy.chargerWirelessOnline);
    out->maxChargingCurrentMicroamps = static_cast<int32_t>(in.legacy.maxChargingCurrent);
    out->maxChargingVoltageMicrovolts = static_cast<int32_t>(in.legacy.maxChargingVoltage);
    out->batteryStatus =
            static_cast<aidl::android::hardware::health::BatteryStatus>(in.legacy.batteryStatus);
    out->batteryHealth =
            static_cast<aidl::android::hardware::health::BatteryHealth>(in.legacy.batteryHealth);
    out->batteryPresent = static_cast<bool>(in.legacy.batteryPresent);
    out->batteryLevel = static_cast<int32_t>(in.legacy.batteryLevel);
    out->batteryVoltageMillivolts = static_cast<int32_t>(in.legacy.batteryVoltage);
    out->batteryTemperatureTenthsCelsius = static_cast<int32_t>(in.legacy.batteryTemperature);
    out->batteryCurrentMicroamps = static_cast<int32_t>(in.legacy.batteryCurrent);
    out->batteryCycleCount = static_cast<int32_t>(in.legacy.batteryCycleCount);
    out->batteryFullChargeUah = static_cast<int32_t>(in.legacy.batteryFullCharge);
    out->batteryChargeCounterUah = static_cast<int32_t>(in.legacy.batteryChargeCounter);
    out->batteryTechnology = in.legacy.batteryTechnology;
    out->batteryCurrentAverageMicroamps = static_cast<int32_t>(in.batteryCurrentAverage);
    out->diskStats.clear();
    out->diskStats.resize(in.diskStats.size());
    for (size_t i = 0; i < in.diskStats.size(); ++i)
        if (!translate(in.diskStats[i], &out->diskStats[i])) return false;
    out->storageInfos.clear();
    out->storageInfos.resize(in.storageInfos.size());
    for (size_t i = 0; i < in.storageInfos.size(); ++i)
        if (!translate(in.storageInfos[i], &out->storageInfos[i])) return false;
    return true;
}
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::health::V2_1::HealthInfo& in,
        aidl::android::hardware::health::HealthInfo* out) {
    if (!translate(in.legacy, out)) return false;
    out->batteryCapacityLevel = static_cast<aidl::android::hardware::health::BatteryCapacityLevel>(
            in.batteryCapacityLevel);
    out->batteryChargeTimeToFullNowSeconds =
            static_cast<int64_t>(in.batteryChargeTimeToFullNowSeconds);
    out->batteryFullChargeDesignCapacityUah =
            static_cast<int32_t>(in.batteryFullChargeDesignCapacityUah);
    return true;
}

}  // namespace android::h2a
