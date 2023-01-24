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

/**
 * Possible values for Battery Health.
 * Note: These are currently in sync with BatteryManager and must not
 * be extended / altered.
 */
@VintfStability
@Backing(type="int")
enum BatteryChargingState {
    INVALID = 0,
    /**
     * Default state.
     */
    NORMAL = 1,
    /**
     * Reported when the battery is too cold to charge at a normal
     * rate or stopped charging due to low temperature.
     */
    TOO_COLD = 2,
    /**
     * Reported when the battery is too hot to charge at a normal
     * rate or stopped charging due to hot temperature.
     */
    TOO_HOT = 3,
    /**
     * The device is using a special charging profile that designed
     * to prevent accelerated aging.
     */
    LONG_LIFE = 4,
    /**
     * The device is using a special charging profile designed to
     * improve battery cycle life, performances or both.
     */
    ADAPTIVE = 5,
}
