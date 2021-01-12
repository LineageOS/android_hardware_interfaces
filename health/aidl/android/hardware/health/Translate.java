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

public class Translate {
    static public android.hardware.health.StorageInfo h2aTranslate(
            android.hardware.health.V2_0.StorageInfo in) {
        android.hardware.health.StorageInfo out = new android.hardware.health.StorageInfo();
        out.eol = in.eol;
        out.lifetimeA = in.lifetimeA;
        out.lifetimeB = in.lifetimeB;
        out.version = in.version;
        return out;
    }

    static public android.hardware.health.DiskStats h2aTranslate(
            android.hardware.health.V2_0.DiskStats in) {
        android.hardware.health.DiskStats out = new android.hardware.health.DiskStats();
        out.reads = in.reads;
        out.readMerges = in.readMerges;
        out.readSectors = in.readSectors;
        out.readTicks = in.readTicks;
        out.writes = in.writes;
        out.writeMerges = in.writeMerges;
        out.writeSectors = in.writeSectors;
        out.writeTicks = in.writeTicks;
        out.ioInFlight = in.ioInFlight;
        out.ioTicks = in.ioTicks;
        out.ioInQueue = in.ioInQueue;
        return out;
    }

    static public android.hardware.health.HealthInfo h2aTranslate(
            android.hardware.health.V2_1.HealthInfo in) {
        android.hardware.health.HealthInfo out = new android.hardware.health.HealthInfo();
        out.chargerAcOnline = in.legacy.legacy.chargerAcOnline;
        out.chargerUsbOnline = in.legacy.legacy.chargerUsbOnline;
        out.chargerWirelessOnline = in.legacy.legacy.chargerWirelessOnline;
        out.maxChargingCurrentMicroamps = in.legacy.legacy.maxChargingCurrent;
        out.maxChargingVoltageMicrovolts = in.legacy.legacy.maxChargingVoltage;
        out.batteryStatus = in.legacy.legacy.batteryStatus;
        out.batteryHealth = in.legacy.legacy.batteryHealth;
        out.batteryPresent = in.legacy.legacy.batteryPresent;
        out.batteryLevel = in.legacy.legacy.batteryLevel;
        out.batteryVoltageMillivolts = in.legacy.legacy.batteryVoltage;
        out.batteryTemperatureTenthsCelsius = in.legacy.legacy.batteryTemperature;
        out.batteryCurrentMicroamps = in.legacy.legacy.batteryCurrent;
        out.batteryCycleCount = in.legacy.legacy.batteryCycleCount;
        out.batteryFullChargeUah = in.legacy.legacy.batteryFullCharge;
        out.batteryChargeCounterUah = in.legacy.legacy.batteryChargeCounter;
        out.batteryTechnology = in.legacy.legacy.batteryTechnology;
        out.batteryCurrentAverageMicroamps = in.legacy.batteryCurrentAverage;
        out.diskStats = new android.hardware.health.DiskStats[in.legacy.diskStats.size()];
        for (int i = 0; i < in.legacy.diskStats.size(); i++) {
            out.diskStats[i] = h2aTranslate(in.legacy.diskStats.get(i));
        }
        out.storageInfos = new android.hardware.health.StorageInfo[in.legacy.storageInfos.size()];
        for (int i = 0; i < in.legacy.storageInfos.size(); i++) {
            out.storageInfos[i] = h2aTranslate(in.legacy.storageInfos.get(i));
        }
        out.batteryCapacityLevel = in.batteryCapacityLevel;
        out.batteryChargeTimeToFullNowSeconds = in.batteryChargeTimeToFullNowSeconds;
        out.batteryFullChargeDesignCapacityUah = in.batteryFullChargeDesignCapacityUah;
        return out;
    }
}
