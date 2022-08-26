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

package android.hardware.audio.core;

/**
 * Audio patch specifies a connection between multiple audio port
 * configurations.
 */
@JavaDerive(equals=true, toString=true)
@VintfStability
parcelable AudioPatch {
    /** The ID of the patch, unique within the HAL module. */
    int id;
    /**
     * The list of IDs of source audio port configs ('AudioPortConfig.id').
     * There must be at least one source in a valid patch and all IDs must be
     * unique.
     */
    int[] sourcePortConfigIds;
    /**
     * The list of IDs of sink audio port configs ('AudioPortConfig.id').
     * There must be at least one sink in a valid patch and all IDs must be
     * unique.
     */
    int[] sinkPortConfigIds;
    /**
     * The minimum buffer size, in frames, which streams must use for
     * this connection configuration. This field is filled out by the
     * HAL module on creation of the patch and must be a positive number.
     */
    int minimumStreamBufferSizeFrames;
    /**
     * Latencies, in milliseconds, associated with each sink port config from
     * the 'sinkPortConfigIds' field. This field is filled out by the HAL module
     * on creation or updating of the patch and must be a positive number. This
     * is a nominal value. The current value of latency is provided via
     * 'StreamDescriptor' command exchange on each audio I/O operation.
     */
    int[] latenciesMs;
}
