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

/**
 * EventType enumerates the events that can be delivered by sendEvent
 */
@VintfStability
@Backing(type="int")
enum EventType {
    /**
     * This event type indicates that the app needs to request a certificate
     * from the provisioning server. The request message data is obtained using
     * getProvisionRequest().
     */
    PROVISION_REQUIRED,
    /**
     * This event type indicates that the app needs to request keys from a
     * license server. The request message data is obtained using getKeyRequest.
     */
    KEY_NEEDED,
    /**
     * This event type indicates that the licensed usage duration for keys in a
     * session has expired. The keys are no longer valid.
     */
    KEY_EXPIRED,
    /**
     * This event may indicate some specific vendor-defined condition, see your
     * DRM provider documentation for details.
     */
    VENDOR_DEFINED,
    /**
     * This event indicates that a session opened by the app has been reclaimed
     * by the resource manager.
     */
    SESSION_RECLAIMED,
}
