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

package android.hardware.tv.tuner;

import android.hardware.tv.tuner.PlaybackStatus;
import android.hardware.tv.tuner.RecordStatus;

/**
 * This interface is used by the HAL to notify the DVR playback and record status
 * back to the client, the cient implements the interfaces and passes a handle to
 * the HAL.
 * @hide
 */
@VintfStability
oneway interface IDvrCallback {
    /**
     * Notify the client a new status of the demux's playback.
     *
     * @param status a new status of the demux's playback.
     */
    void onPlaybackStatus(in PlaybackStatus status);

    /**
     * Notify the client a new status of the demux's record.
     *
     * @param status a new status of the demux's record.
     */
    void onRecordStatus(in RecordStatus status);
}
