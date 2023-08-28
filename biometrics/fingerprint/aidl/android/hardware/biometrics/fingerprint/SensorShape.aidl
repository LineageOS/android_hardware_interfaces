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

package android.hardware.biometrics.fingerprint;
/**
 * This is most useful for sensors configured as FingerprintSensorType::UNDER_DISPLAY_OPTICAL,
 * as it's used to compute the on-screen sensor boundaries for the touch detection algorithm.
 *
 * @hide
 */
@VintfStability
@Backing(type="byte")
enum SensorShape {
    SQUARE,
    CIRCLE,
}
