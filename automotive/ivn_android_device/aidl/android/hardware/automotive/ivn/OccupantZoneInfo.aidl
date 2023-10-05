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

package android.hardware.automotive.ivn;

import android.hardware.automotive.ivn.OccupantType;

/**
 * Represents an occupant zone in a car.
 *
 * <p>Each occupant does not necessarily represent single person but it is for mapping to one
 * set of displays. For example, for display located in center rear seat, both left and right
 * side passengers may use it but it is abstracted as a single occupant zone.</p>
 */
@JavaDerive(equals=true, toString=true)
@VintfStability
parcelable OccupantZoneInfo {
    /**
     * This is an unique id to distinguish each occupant zone.
     *
     * <p>This can be helpful to distinguish different zones when {@link #occupantType} and
     * {@link #seat} are the same for multiple occupant / passenger zones.</p>
     *
     * <p>This id will remain the same for the same zone across configuration changes like
     * user switching or display changes</p>
     */
    int zoneId;
    /** Represents type of passenger */
    OccupantType occupantType;
    /**
     * Represents seat assigned for the occupant. In some system, this can have value of
     * {@code VehicleAreaSeat#SEAT_UNKNOWN}.
     *
     * <p>This might be one of {@code VehicleAreaSeat} or a combination of {@code VehicleAreaSeat}.
     */
    int seat;
}
