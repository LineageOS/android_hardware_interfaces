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
 * Used by EVS_SERVICE_REQUEST to enumerate the service's type.
 */
@VintfStability
@Backing(type="int")
enum EvsServiceType {
    REARVIEW = 0,
    SURROUNDVIEW = 1,
    FRONTVIEW = 2,
    LEFTVIEW = 3,
    RIGHTVIEW = 4,
    DRIVERVIEW = 5,
    FRONTPASSENGERSVIEW = 6,
    REARPASSENGERSVIEW = 7,
    USER_DEFINED = 1000,
}
