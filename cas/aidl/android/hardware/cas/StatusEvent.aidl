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

package android.hardware.cas;

/**
 * The Event Type for status change.
 * @hide
 */
@VintfStability
@Backing(type="byte")
enum StatusEvent {
    /**
     * The status of CAS plugin was changed due to physical module insertion or
     * removal. Client must call enumeratePlugins to update plugins' status.
     */
    PLUGIN_PHYSICAL_MODULE_CHANGED,

    /**
     * The status of supported session number was changed due to physical module
     * insertion or removal. Client must update session resource according to
     * latest StatusMessage from the StatusEvent. The plugin supports unlimited
     * session by default.
     */
    PLUGIN_SESSION_NUMBER_CHANGED,
}
