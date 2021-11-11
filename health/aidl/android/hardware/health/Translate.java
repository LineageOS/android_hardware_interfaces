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

    private static void h2aTranslateInternal(
            android.hardware.health.HealthInfo out, android.hardware.health.V1_0.HealthInfo in) {
        out.chargerAcOnline = in.chargerAcOnline;
        out.chargerUsbOnline = in.chargerUsbOnline;
        out.chargerWirelessOnline = in.chargerWirelessOnline;
        out.maxChargingCurrentMicroamps = in.maxChargingCurrent;
        out.maxChargingVoltageMicrovolts = in.maxChargingVoltage;
        out.batteryStatus = in.batteryStatus;
        out.batteryHealth = in.batteryHealth;
        out.batteryPresent = in.batteryPresent;
        out.batteryLevel = in.batteryLevel;
        out.batteryVoltageMillivolts = in.batteryVoltage;
        out.batteryTemperatureTenthsCelsius = in.batteryTemperature;
        out.batteryCurrentMicroamps = in.batteryCurrent;
        out.batteryCycleCount = in.batteryCycleCount;
        out.batteryFullChargeUah = in.batteryFullCharge;
        out.batteryChargeCounterUah = in.batteryChargeCounter;
        out.batteryTechnology = in.batteryTechnology;
    }

    public static android.hardware.health.HealthInfo h2aTranslate(
            android.hardware.health.V1_0.HealthInfo in) {
        android.hardware.health.HealthInfo out = new android.hardware.health.HealthInfo();
        h2aTranslateInternal(out, in);
        return out;
    }

    static public android.hardware.health.HealthInfo h2aTranslate(
            android.hardware.health.V2_1.HealthInfo in) {
        android.hardware.health.HealthInfo out = new android.hardware.health.HealthInfo();
        h2aTranslateInternal(out, in.legacy.legacy);
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
