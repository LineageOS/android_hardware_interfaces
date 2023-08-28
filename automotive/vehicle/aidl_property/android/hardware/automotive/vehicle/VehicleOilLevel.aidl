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
 * Used to enumerate the current level of VehicleProperty#ENGINE_OIL_LEVEL.
 */
@VintfStability
@Backing(type="int")
enum VehicleOilLevel {
    /**
     * The oil level of the engine is critically low, so the vehicle may be unsafe to drive.
     */
    CRITICALLY_LOW = 0,
    /**
     * The oil level of the engine is low and needs to be replaced.
     */
    LOW = 1,
    /**
     * The oil level of the engine is normal for the vehicle.
     */
    NORMAL = 2,
    /**
     * The oil level of the engine is high, so the vehicle may be unsafe to drive.
     */
    HIGH = 3,
    /**
     * This value represents an error when retrieving the oil level of the engine.
     */
    ERROR = 4,
}
