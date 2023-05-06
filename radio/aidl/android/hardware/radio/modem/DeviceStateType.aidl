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

package android.hardware.radio.modem;

/** @hide */
@VintfStability
@Backing(type="int")
@JavaDerive(toString=true)
enum DeviceStateType {
    /**
     * Device power save mode (provided by PowerManager). True indicates the device is in
     * power save mode.
     */
    POWER_SAVE_MODE,
    /**
     * Device charging state (provided by BatteryManager). True indicates the device is charging.
     */
    CHARGING_STATE,
    /**
     * Low data expected mode. True indicates low data traffic is expected, for example, when the
     * device is idle (e.g. not doing tethering in the background). Note this doesn't mean no data
     * is expected.
     */
    LOW_DATA_EXPECTED,
}
