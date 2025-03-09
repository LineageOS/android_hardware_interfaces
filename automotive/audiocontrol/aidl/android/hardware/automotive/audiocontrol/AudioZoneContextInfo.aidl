/*
 * Copyright (C) 2024 The Android Open Source Project
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

package android.hardware.automotive.audiocontrol;

import android.media.audio.common.AudioAttributes;

/**
 * Encapsulates groups of audio attributes which should be managed together.
 */
@JavaDerive(equals=true, toString=true)
@VintfStability
parcelable AudioZoneContextInfo {
    /**
     * Value indicating the context info id is not assigned.
     */
    const int UNASSIGNED_CONTEXT_ID = -1;

    /**
     * Context name which can be used to map the info to an audio route
     * management as described in each audio configuration.
     *
     * <P>Name must be non-empty and unique among all audio context info within the same
     * {@link android.hardware.automotive.audiocontrol.AudioZoneContext} container.
     */
    String name;

    /**
     * Used in car audio service to manage the info
     *
     * <P>Must be non-negative integer if assigned, or UNASSIGNED_CONTEXT_ID otherwise. If using
     * configurable audio policy engine audio routing with multi-zone configurations the value must
     * be assigned.
     */
    int id = UNASSIGNED_CONTEXT_ID;

    /**
     * List of audio attributes that belong to the context
     */
    List<AudioAttributes> audioAttributes;
}
