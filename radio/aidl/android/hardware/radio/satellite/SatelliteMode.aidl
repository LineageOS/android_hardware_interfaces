/*
 * Copyright (C) 2022 The Android Open Source Project
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

package android.hardware.radio.satellite;

@VintfStability
@Backing(type="int")
@JavaDerive(toString=true)
enum SatelliteMode {
    /* Satellite modem is powered off */
    POWERED_OFF = 0,
    /* Satellite modem is in out of service state and not searching for satellite signal */
    OUT_OF_SERVICE_NOT_SEARCHING = 1,
    /* Satellite modem is in out of service state and searching for satellite signal */
    OUT_OF_SERVICE_SEARCHING = 2,
    /* Satellite modem has found satellite signal and gets connected to the satellite network */
    ACQUIRED = 3,
    /* Satellite modem is sending and/or receiving messages */
    MESSAGE_TRANSFERRING = 4
}
