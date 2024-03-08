/*
 * Copyright (C) 2023 The Android Open Source Project
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

/**
 * Possible values for BatteryPartStatus.
 * Note: These are currently in sync with BatteryManager and must not
 * be extended / altered.
 */
@VintfStability
@Backing(type="int")
enum BatteryPartStatus {
    /**
     * Device cannot differentiate an original battery from a replaced battery.
     */
    UNSUPPORTED = 0,
    /**
     * Device has the original battery it was manufactured with.
     */
    ORIGINAL = 1,
    /**
     * Device has a replaced battery.
     */
    REPLACED = 2,
}
