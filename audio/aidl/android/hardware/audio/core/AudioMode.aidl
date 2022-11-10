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
 * The audio mode describes states of the audio system of the device that
 * can significantly affect the rules of audio routing, volume control, etc.
 * The audio mode is controlled by the framework, however the HAL has some
 * flexibility in the choice of modes to support, see 'IModule.updateAudioMode'.
 */
@VintfStability
@Backing(type="int")
enum AudioMode {
    /** No active calls. */
    NORMAL = 0,
    /** The device is playing the ringtone. */
    RINGTONE = 1,
    /** The call is handled by the telephony stack ("voice call"). */
    IN_CALL = 2,
    /** The call is handled by an application ("VoIP call"). */
    IN_COMMUNICATION = 3,
    /** Call screening is in progress. */
    CALL_SCREEN = 4,
}
