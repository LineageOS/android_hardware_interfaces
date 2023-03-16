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

package android.hardware.wifi.supplicant;

/**
 * Details of the PMKSA cache entry that was added in supplicant.
 */
@VintfStability
parcelable PmkSaCacheData {
    /**
     * BSSID of the access point to which the station is associated.
     */
    byte[6] bssid;
    /**
     * PMK expiration time in seconds.
     */
    long expirationTimeInSec;
    /**
     * Serialized PMK cache entry.
     * The content is opaque for the framework and depends on the native implementation.
     */
    byte[] serializedEntry;
}
