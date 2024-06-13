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

package android.hardware.automotive.vehicle;

/**
 * Used to enumerate the different states the ignition of the vehicle can be in.
 *
 * Through the use of VehicleAreaConfig#supportedEnumValues, OEMs may specify they only support a
 * subset of the enums that are defined here.
 */
@VintfStability
@Backing(type="int")
enum VehicleIgnitionState {
    UNDEFINED = 0,
    /**
     * Steering wheel is locked
     */
    LOCK = 1,
    /**
     * Steering wheel is not locked, engine and all accessories are OFF. If
     * car can be in LOCK and OFF state at the same time than HAL must report
     * LOCK state.
     *
     * If IGNITION_STATE is implemented on a BEV, then this state must
     * communicate that the BEV's High Voltage battery is disconnected and thus
     * the vehicle is OFF.
     */
    OFF,
    /**
     * Typically in this state accessories become available (e.g. radio).
     * Instrument cluster and engine are turned off
     */
    ACC,
    /**
     * Ignition is in state ON. Accessories and instrument cluster available,
     * engine might be running or ready to be started.
     *
     * If IGNITION_STATE is implemented on a BEV, then this state must
     * communicate that the BEV's High Voltage battery is connected and thus the
     * vehicle is ON.
     */
    ON,
    /**
     * Typically in this state engine is starting (cranking).
     */
    START,
}
