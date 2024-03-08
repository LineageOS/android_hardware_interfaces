/*
 * Copyright 2023 The Android Open Source Project
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

package android.hardware.bluetooth.ranging;

@VintfStability
@Backing(type="int")
enum ResultType {
    RESULT_METERS = 0x00,
    ERROR_METERS = 0x01,
    AZIMUTH_DEGREES = 0x02,
    ERROR_AZIMUTH_DEGREES = 0x03,
    ALTITUDE_DEGREES = 0x04,
    ERROR_ALTITUDE_DEGREES = 0x05,
    DELAY_SPREAD_METERS = 0x06,
    CONFIDENCE_LEVEL = 0x07,
    SECURITY_LEVEL = 0x08,
    VELOCITY = 0x09,
}
