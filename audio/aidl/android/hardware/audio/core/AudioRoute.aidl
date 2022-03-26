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
 * Audio route specifies a path from multiple audio source ports to one audio
 * sink port. As an example, when emitting audio output, source ports typically
 * are mix ports (audio data from the framework), the sink is a device
 * port. When acquiring audio, source ports are device ports, the sink is a mix
 * port.
 */
@JavaDerive(equals=true, toString=true)
@VintfStability
parcelable AudioRoute {
    /**
     * The list of IDs of source audio ports ('AudioPort.id').
     * There must be at least one source in a valid route and all IDs must be
     * unique.
     */
    int[] sourcePortIds;
    /** The ID of the sink audio port ('AudioPort.id'). */
    int sinkPortId;
    /** If set, only one source can be active, mixing is not supported. */
    boolean isExclusive;
}
