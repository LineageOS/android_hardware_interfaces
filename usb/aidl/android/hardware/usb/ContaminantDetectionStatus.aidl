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

package android.hardware.usb;

@VintfStability
enum ContaminantDetectionStatus {
    /**
     * Contaminant presence detection is not supported.
     */
    NOT_SUPPORTED = 0,
    /**
     * Contaminant presence detection is supported but disabled.
     */
    DISABLED = 1,
    /**
     * Contaminant presence detection is enabled and contaminant not detected.
     */
    NOT_DETECTED = 2,
    /**
     * Contaminant presence detection is enabled and contaminant detected.
     */
    DETECTED = 3,
}
