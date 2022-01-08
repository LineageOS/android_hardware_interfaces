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

package android.hardware.drm;

import android.hardware.drm.KeyRequestType;

@VintfStability
parcelable KeyRequest {
    /** The opaque key request blob. */
    byte[] request;

    /**
     * Enumerated type:
     *     INITIAL - the first key request for a license
     *     NONE - indicates that no request is needed because the keys
     *         are already loaded
     *     RENEWAL - is a subsequent key request used to refresh the
     *         keys in a license
     *     RELEASE - indicates keys are being released
     *     UPDATE - indicates that the keys need to be refetched after
     *         the initial license request
     */
    KeyRequestType requestType;

    /**
     * The URL that the request may be sent to,
     * if provided by the drm HAL. The app can choose to
     * override this URL. If the HAL implementation does not provide
     * a defaultUrl, the returned string must be empty.
     */
    String defaultUrl;
}
