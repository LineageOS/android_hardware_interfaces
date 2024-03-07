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

package android.hardware.camera.device;

import android.hardware.camera.device.HalStream;

/**
 * ConfigureStreamsRet.
 *
 * Parcelable returned by the 'configureStreamsV2' call.
 * This contains information which informs the camera framework
 * about properties of each stream configured and also whether the
 * the hal buffer manager must be used for the session configured.
 */
@VintfStability
parcelable ConfigureStreamsRet {
    /**
     * A vector of HalStream Parcelables, which contain information
     * about the stream parameters desired by the HAL such as usage flags,
     * overridden format, maximum buffers etc.
     */
    HalStream[] halStreams;
    /**
     * A boolean informing the camera framework whether the HAL buffer manager
     * must be used for the session configured.
     */
    boolean enableHalBufferManager = false;
}
