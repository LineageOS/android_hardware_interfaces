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

@VintfStability
parcelable ProvisionRequest {
    /** The opaque certificate request blob. */
    byte[] request;

    /**
     * The URL that the provisioning request may be sent to,
     * if known by the HAL implementation. An app can choose to
     * override this URL. If the HAL implementation does not provide
     * a defaultUrl, the returned string must be empty.
     */
    String defaultUrl;
}
