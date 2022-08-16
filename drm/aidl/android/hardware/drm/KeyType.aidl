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
@Backing(type="int")
enum KeyType {
    /**
     * Drm keys can be for offline content or for online streaming.
     * Offline keys are persisted on the device and may be used when the device
     * is disconnected from the network.
     */
    OFFLINE,
    /**
     * Keys for streaming are not persisted and require the device to be
     * connected to the network for periodic renewal.
     */
    STREAMING,
    /**
     * The Release type is used to request that offline keys be no longer
     * restricted to offline use.
     */
    RELEASE,
}
