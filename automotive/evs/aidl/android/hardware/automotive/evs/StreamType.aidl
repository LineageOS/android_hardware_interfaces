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

package android.hardware.automotive.evs;

/**
 * StreamType:
 *
 * The type of the camera stream, which defines whether the EVS client device is
 * the producer or the consumer for that stream, and how the buffers of the
 * stream relate to the other streams.
 */
@VintfStability
@Backing(type="int")
enum StreamType {
    /**
     * This stream is an output stream; the EVS HAL device must fill buffers
     * from this stream with newly captured or reprocessed image data.
     */
    OUTPUT = 0,

    /**
     * This stream is an input stream; the EVS HAL device must read buffers
     * from this stream and send them through the camera processing pipeline,
     * as if the buffer was a newly captured image from the imager.
     */
    INPUT = 1
}
