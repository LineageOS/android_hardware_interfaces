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

package android.hardware.automotive.vehicle;

/**
 * Used to enumerate the current warning state of the driver drowsiness and attention monitoring
 * system.
 */
@VintfStability
@Backing(type="int")
enum DriverDrowsinessAttentionWarning {
    /**
     * This state is used as an alternative for any DriverDrowsinessAttentionWarning value that is
     * defined in the platform. Ideally, implementations of
     * VehicleProperty#DRIVER_DROWSINESS_ATTENTION_WARNING should not use this state. The framework
     * can use this field to remain backwards compatible if DriverDrowsinessAttentionWarning is
     * extended to include additional states.
     */
    OTHER = 0,
    /**
     * When the driver drowsiness and attention warning is enabled, and the driver's current
     * drowsiness and attention level does not warrant the system to send a warning.
     */
    NO_WARNING = 1,
    /**
     * When the driver drowsiness and attention warning is enabled, and the system is warning the
     * driver based on its assessment of the driver's current drowsiness and attention level.
     */
    WARNING = 2,
}
