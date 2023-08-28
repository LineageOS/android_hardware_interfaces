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

package android.hardware.usb;

@VintfStability
@Backing(type="int")
/**
 * Indicates the current status of DisplayPort link training over USB-C for the
 * attached DisplayPort Alt Mode partner sink as described in DisplayPort
 * v1.4/2.1.
 */
enum LinkTrainingStatus {
    /*
     * Indicates the link training result is not available because link training
     * has not completed or initiated yet.
     */
    UNKNOWN = 0,
    /*
     * Indicates that link training has completed and optimal settings for data
     * transmission between the sink and source device have successfully been
     * negotiated.
     */
    SUCCESS = 1,
    /*
     * Indicates that link training has failed and the link initialization has
     * terminated.
     */
    FAILURE = 2,
}
