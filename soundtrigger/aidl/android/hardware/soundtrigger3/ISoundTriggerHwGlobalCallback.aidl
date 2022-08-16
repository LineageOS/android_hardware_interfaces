/*
 * Copyright 2021 The Android Open Source Project
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

package android.hardware.soundtrigger3;

/**
 * SoundTrigger HAL callback interface for events not associated with a particular model.
 */
@VintfStability
interface ISoundTriggerHwGlobalCallback {
    /**
     * Callback method called by the HAL whenever internal conditions have been made available, such
     * that a call that would previously have failed with an -EBUSY status may now succeed.
     * There is no guarantee that any call would succeed following this event. It is merely a hint
     * to the client that it may retry.
     * Conversely, any call that have failed previously with a
     * ServiceSpecificException(RESOURCE_CONTENTION) is guaranteed to fail again if retried, until
     * this callback is delivered.
     */
    void onResourcesAvailable();
}
