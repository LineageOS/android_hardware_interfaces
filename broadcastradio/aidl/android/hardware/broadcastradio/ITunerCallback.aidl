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

package android.hardware.broadcastradio;

import android.hardware.broadcastradio.ConfigFlag;
import android.hardware.broadcastradio.ProgramInfo;
import android.hardware.broadcastradio.ProgramListChunk;
import android.hardware.broadcastradio.ProgramSelector;
import android.hardware.broadcastradio.Result;
import android.hardware.broadcastradio.VendorKeyValue;

@VintfStability
oneway interface ITunerCallback {
    /**
     * Method called by the HAL when a tuning operation fails asynchronously
     * following {@link IBroadcastRadio#tune}, {@link IBroadcastRadio#seek}
     * or {@link IBroadcastRadio#step}.
     *
     * This callback is only called when the tune(), seek() or step() command
     * succeeds without returning any error at first.
     *
     * @param result {@link Result#TIMEOUT} in case that tune(), seek() or
     *               step() is not completed within
     *               @link IBroadcastRadio#TUNER_TIMEOUT_MS}
     *               or {@link Result#CANCELED} if the command was canceled.
     * @param selector A ProgramSelector structure passed from tune() call;
     *                 empty for step() and seek().
     */
    void onTuneFailed(in Result result, in ProgramSelector selector);

    /**
     * Method called by the HAL when current program information (including
     * metadata) is updated. It must be called when {@link IBroadcastRadio#tune}
     * {@link IBroadcastRadio#seek} or {@link IBroadcastRadio#step} command
     * succeeds.
     *
     * This is also called when the radio tuned to the static (not a valid
     * station), see {@link ProgramInfo#FLAG_TUNABLE} flag.
     *
     * @param info Current program information.
     */
    void onCurrentProgramInfoChanged(in ProgramInfo info);

    /**
     * A delta update of the program list, called whenever there's a change in
     * the list.
     *
     * If there are frequent changes, HAL implementation must throttle the rate
     * of the updates.
     *
     * There is a hard limit on binder transaction buffer, and the list must
     * not exceed it. For large lists, HAL implementation must split them to
     * multiple chunks, no larger than 500kiB each, and call this program list
     * update callback method separately.
     *
     * @param chunk A chunk of the program list update.
     */
    void onProgramListUpdated(in ProgramListChunk chunk);

    /**
     * Method called by the HAL when the antenna gets connected or disconnected.
     *
     * For broadcast radio service, client must assume the antenna is connected.
     * If it's not, then antennaStateChange must be called within
     * {@link IBroadcastRadio#ANTENNA_STATE_CHANGE_TIMEOUT_MS} to indicate that.
     *
     * @param connected {@code true} if the antenna is now connected, {@code false}
     * otherwise.
     */
    void onAntennaStateChange(in boolean connected);

    /**
     * Generic callback for passing updates to config flags.
     *
     * It's up to the HAL implementation if and how to implement this callback,
     * as long as it obeys the prefix rule. However, setConfigFlag must not
     * trigger this callback, while an internal event can change config flag
     * asynchronously at the HAL layer.
     *
     * @param flag Flag that has changed.
     * @param value The new value of the given flag.
     */
    void onConfigFlagUpdated(in ConfigFlag flag, in boolean value);

    /**
     * Generic callback for passing updates to vendor-specific parameter values.
     * The framework does not interpret the parameters, they are passed
     * in an opaque manner between a vendor application and HAL.
     *
     * It's up to the HAL implementation if and how to implement this callback,
     * as long as it obeys the prefix rule. In particular, only selected keys
     * may be notified this way. However, setParameters must not trigger
     * this callback, while an internal event can change parameters
     * asynchronously at the HAL layer.
     *
     * @param parameters Vendor-specific key-value pairs,
     *                   opaque to Android framework.
     */
    void onParametersUpdated(in VendorKeyValue[] parameters);
}
