/*
 * Copyright (C) 2021 The Android Open Source Project
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

package android.hardware.contexthub;

import android.hardware.contexthub.NanoappRpcService;

@VintfStability
parcelable NanoappInfo {
    /** The unique identifier of the nanoapp. */
    long nanoappId;

    /** The version of the nanoapp */
    int nanoappVersion;

    /** True if this nanoapp is in a running state, false otherwise */
    boolean enabled;

    /**
     * The list of Android permissions used by this nanoapp. This list MUST
     * correspond to the permissions required for an equivalent Android app to
     * sample similar signals through the Android framework.
     *
     * For example, if a nanoapp used location-based signals, the permissions
     * list MUST contains android.permission.ACCESS_FINE_LOCATION and
     * android.permission.ACCESS_BACKGROUND_LOCATION. If it were to also use
     * audio data, it would require adding android.permission.RECORD_AUDIO to
     * this list.
     */
    String[] permissions;

    /**
     * The list of RPC services supported by this nanoapp.
     */
    NanoappRpcService[] rpcServices;
}
