/*
 * Copyright 2020, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.hardware.security.dice;

@Backing(type="int")
/**
 * These response codes are used as service specific exception codes by
 * IDiceDevice.
 * @hide
 */
@VintfStability
enum ResponseCode {
    /**
     * The caller has insufficient privilege to access the DICE API.
     */
    PERMISSION_DENIED = 1,
    /**
     * An unexpected error occurred, likely with IO or IPC.
     */
    SYSTEM_ERROR = 2,
    /**
     * Returned if the called function is not implemented.
     */
    NOT_IMPLEMENTED = 3,
    /**
     * An attempt to demote the implementation failed.
     */
    DEMOTION_FAILED = 4,
}
