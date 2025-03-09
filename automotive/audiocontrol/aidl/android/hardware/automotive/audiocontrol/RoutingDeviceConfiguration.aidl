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
 * Use to configure audio device routing mechanism
 */
@JavaDerive(equals=true, toString=true)
@VintfStability
enum RoutingDeviceConfiguration {
    /**
     * Use to indicate that audio should be managed based on previously supportec mechamisms in
     * car audio service.
     *
     * <p>If this used then the API to setup the audio zones can just throw
     * {@code EX_UNSUPPORTED_OPERATION} if called.
     */
    DEFAULT_AUDIO_ROUTING,
    /**
     * Use to indicate that audio should be managed using the dynamic audio
     * policy as setup by car audio service using the setup configuration from
     * the {@Link android.hardware.automotive.audiocontrol.AudioZone}'s info from audio control HAL.
     *
     * <p>If this used then the APIs to setup the audio zones must return a valid audio zone
     * configuration for the device.
     */
    DYNAMIC_AUDIO_ROUTING,
    /**
     * Use to indicate that audio should be managed using the core audio
     * routing as setup by car audio service using the setup configuration from
     * the {@Link android.hardware.automotive.audiocontrol.AudioZone}'s info from audio control HAL
     * and the information contained within the configurable audio policy engine.
     *
     * <p>If this used then the APIs to setup the audio zone(s) must return valid audio zone
     * configuration(s) for the device.
     */
    CONFIGURABLE_AUDIO_ENGINE_ROUTING,
}
