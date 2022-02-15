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

package android.hardware.nfc;

/**
 * Different closing types for NFC HAL.
 */
@VintfStability
@Backing(type="int")
enum NfcCloseType {
    /**
     * Close the NFC controller and free NFC HAL resources.
     */
    DISABLE = 0,

    /**
     * Switch the NFC controller operation mode and free NFC HAL resources.
     * Enable NFC functionality for off host card emulation usecases in
     * device(host) power off(switched off) state, if the device
     * supports power off use cases. If the device doesn't support power
     * off use cases, this call should be same as DISABLE.
     */
    HOST_SWITCHED_OFF = 1,
}
