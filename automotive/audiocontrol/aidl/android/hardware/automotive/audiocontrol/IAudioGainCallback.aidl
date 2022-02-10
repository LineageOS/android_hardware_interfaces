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

package android.hardware.automotive.audiocontrol;

import android.hardware.automotive.audiocontrol.AudioGainConfigInfo;
import android.hardware.automotive.audiocontrol.Reasons;

/**
 * Interface definition for a callback to be invoked when the gain(s) of the device port(s) is(are)
 * updated at HAL layer.
 *
 * <p>This defines counter part API of
 * {@link android.hardware.automotive.audiocontrol.IAudioControl#onDevicesToDuckChange},
 * {@link android.hardware.automotive.audiocontrol.IAudioControl#onDevicesToMuteChange} and
 * {@link android.hardware.automotive.audiocontrol.IAudioControl#setAudioDeviceGainsChanged} APIs.
 *
 * The previous API defines Mute/Duck order decided by the client (e.g. CarAudioService)
 * and delegated to AudioControl for application.
 *
 * This callback interface defines Mute/Duck notification decided by AudioControl HAL (due to
 * e.g. - external conditions from Android IVI subsystem
 *      - regulation / need faster decision rather than using
 *        {@link android.hardware.automotive.audiocontrol.IAudioControl#onAudioFocusChange} to
 *        report the use case and then waiting for CarAudioService decision to Mute/Duck.
 */
@VintfStability
oneway interface IAudioGainCallback {
    /**
     * Used to indicated the one or more audio device port gains have changed unexpectidely, i.e.
     * initiated by HAL, not by CarAudioService.
     * This is the counter part of the
     *      {@link android.hardware.automotive.audiocontrol.onDevicesToDuckChange},
     *      {@link android.hardware.automotive.audiocontrol.onDevicesToMuteChange} and
     *      {@link android.hardware.automotive.audiocontrol.setAudioDeviceGainsChanged} APIs.
     *
     * Flexibility is given to OEM to mute/duck in HAL or in CarAudioService.
     * For critical use cases (i.e. when regulation is required), better to handle mute/duck in
     * HAL layer and informs upper layer.
     * Non critical use case may report gain and focus and CarAudioService to decide of duck/mute.
     *
     * @param reasons List of reasons that triggered the given gains changed.
     *                This must be one or more of the
     *                {@link android.hardware.automotive.audiocontrol.Reasons} constants.
     *                It will define if the port has been muted/ducked or must now affected
     *                by gain limitation that shall be notified/enforced at CarAudioService
     *                layer.
     *
     * @param gains List of gains affected by the change.
     */
    void onAudioDeviceGainsChanged(in Reasons[] reasons, in AudioGainConfigInfo[] gains);
}
