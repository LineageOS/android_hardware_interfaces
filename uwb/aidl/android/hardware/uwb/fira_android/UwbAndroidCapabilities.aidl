/*
 * Copyright (C) 2021 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.hardware.uwb.fira_android;

/**
 * Android specific capabilities should be defined here.
 *
 * For any features enabled via the FIRA vendor commands for Android, use this bitmask
 * to allow devices to expose the features supported by the HAL implementation.
 *
 */
@VintfStability
@Backing(type="long")
enum UwbAndroidCapabilities {
    /** TODO: Change the name if necessary when the corresponding vendor commands are added */
    POWER_STATS_QUERY = 0x1,
}
