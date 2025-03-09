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

/**
 * Audio activiation type which can be used to activate the min/max
 * volume changes.
 */
@VintfStability
@Backing(type="int")
@JavaDerive(toString=true)
enum VolumeInvocationType {
    /**
     * Invocation of volume group activation performed at every playback change.
     */
    ON_PLAYBACK_CHANGED,
    /**
     * Invocation of volume group activation performed only once at playback after first playback
     * for a client (app/service UID).
     */
    ON_SOURCE_CHANGED,
    /**
     * Invocation of volume group activation in perform only at playback once after boot up.
     */
    ON_BOOT,
}
