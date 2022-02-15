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
 * An app determines the type of a key request returned from getKeyRequest.
 */
@VintfStability
@Backing(type="int")
enum KeyRequestType {
    /**
     * Key request type is for an initial license request
     */
    INITIAL,
    /**
     * Key request type is for license renewal. Renewal requests are used
     * to extend the validity period for streaming keys.
     */
    RENEWAL,
    /**
     * Key request type is a release. A key release causes offline keys
     * to become available for streaming.
     */
    RELEASE,
    /**
     * Key request type is unknown due to some error condition.
     */
    UNKNOWN,
    /**
     * Keys are already loaded. No key request is needed.
     */
    NONE,
    /**
     * Keys have previously been loaded. An additional (non-renewal) license
     * request is needed.
     */
    UPDATE,
}
