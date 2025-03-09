/*
 * Copyright 2024 The Android Open Source Project
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

package android.hardware.bluetooth.ranging;

/**
 * The subevent abort reason of Channel Sounding.
 * See BLUETOOTH CORE SPECIFICATION Version 6.0 | Vol 4, Part E 7.7.65.42 for details.
 *
 * Specification: https://www.bluetooth.com/specifications/specs/core60-html/
 */
@VintfStability
@Backing(type="byte")
enum SubeventAbortReason {
    SUCCESS = 0x0,
    LOCAL_OR_REMOTE_REQUEST = 0x01,
    NO_CS_SYNC_RECEIVED = 0x02,
    SCHEDULING_CONFLICTS_OR_LIMITED_RESOURCES = 0x03,
    UNSPECIFIED = 0x0F,
}
