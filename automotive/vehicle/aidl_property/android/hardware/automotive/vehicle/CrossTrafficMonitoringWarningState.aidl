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
 * Used to enumerate the state of Cross Traffic Monitoring Warning system.
 */
@VintfStability
@Backing(type="int")
enum CrossTrafficMonitoringWarningState {

    /**
     * This state is used as an alternative to any CrossTrafficMonitoringWarningState value that is
     * not defined in the platform. Ideally, implementations of
     * VehicleProperty#CROSS_TRAFFIC_MONITORING_WARNING_STATE should not use this state. The
     * framework can use this field to remain backwards compatible if
     * CrossTrafficMonitoringWarningState is extended to include additional states.
     */
    OTHER = 0,
    /**
     * Cross Traffic Monitoring Warning is enabled and monitoring safety, but no potential collision
     * is detected.
     */
    NO_WARNING = 1,
    /**
     * Cross Traffic Monitoring Warning is enabled and is actively warning the user of incoming
     * moving objects coming from the driver's left side in front of the vehicle.
     */
    WARNING_FRONT_LEFT = 2,
    /**
     * Cross Traffic Monitoring Warning is enabled and is actively warning the user of incoming
     * moving objects coming from the driver's right side in front of the vehicle.
     */
    WARNING_FRONT_RIGHT = 3,
    /**
     * Cross Traffic Monitoring Warning is enabled and is actively warning the user of incoming
     * moving objects coming from both the driver's left side and the driver's right side in front
     * of the vehicle.
     */
    WARNING_FRONT_BOTH = 4,
    /**
     * Cross Traffic Monitoring Warning is enabled and is actively warning the user of incoming
     * moving objects coming from the driver's left side behind the vehicle.
     */
    WARNING_REAR_LEFT = 5,
    /**
     * Cross Traffic Monitoring Warning is enabled and is actively warning the user of incoming
     * moving objects coming from the driver's right side behind the vehicle.
     */
    WARNING_REAR_RIGHT = 6,
    /**
     * Cross Traffic Monitoring Warning is enabled and is actively warning the user of incoming
     * moving objects coming from the driver's left side and the driver's right side behind the
     * vehicle.
     */
    WARNING_REAR_BOTH = 7,
}
