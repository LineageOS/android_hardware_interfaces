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

package android.hardware.automotive.audiocontrol;

import android.media.audio.common.AudioPort;

/**
 * Interface definition for asynchronous changes to audio configs defined
 * for a hardware {@link android.hardware.audio.core.IModule}.
 */
@VintfStability
oneway interface IModuleChangeCallback {
    /**
     * Used to indicate that one or more {@link android.media.audio.common.AudioPort}
     * configs have changed. Implementations MUST return at least one AudioPort.
     *
     * Notes for AudioPort:
     * 1. For V3, the support will be limited to configurable AudioGain stages - per
     *    car audio framework support.
     * 2. For automotive 'bus' devices, the expected settings are
     *     AudioDevice {
     *        AudioDeviceDescription {type: IN/OUT_DEVICE, connection: CONNECTION_BUS}
     *        AudioDeviceAddress {id: string}}
     *
     * Notes for AudioGain:
     * 1. Car audio framework only supports AudioGainMode::JOINT. Any other mode
     *    selection will be ignored.
     *    See {@link android.media.audio.common.AudioGainMode}
     * 2. Implementations MUST ensure that the gain stages are identical for buses
     *    that map to the same volume group. Any inconsistencies will result in
     *    inferior volume-change experience to the users.
     * 3. Implementations MUST ensure that the new gain stages are subset (do not
     *    exceed) of the gain stage definitions provided to audio policy. If they
     *    exceed, it can result in failure when setting value over the range
     *    allowed by the audio policy.
     *
     * Other notes:
     * 1. In case of AudioControl  service restart or resume, the implementations MUST
     *    issue an immediate callback following registration.
     * 2. In case of client restart, the AudioControl service MUST clear all stale
     *    callbacks.
     *
     * @param audioPorts list of {@link android.media.audio.common.AudioPort} that
     *                   are updated
     */
    void onAudioPortsChanged(in AudioPort[] audioPorts);
}
