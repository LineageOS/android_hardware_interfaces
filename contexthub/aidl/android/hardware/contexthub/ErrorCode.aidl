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

package android.hardware.contexthub;

@VintfStability
@Backing(type="byte")
enum ErrorCode {
    /**
     * No Error.
     */
    OK = 0,

    /**
     * A generic transient error. The sender may retry the
     * operation, but there is no guarantee of success.
     */
    TRANSIENT_ERROR,

    /**
     * A generic permanent error. The sender should not retry the operation.
     */
    PERMANENT_ERROR,

    /**
     * The request failed because the sender does not have necessary permissions.
     * The sender should not retry the operation.
     */
    PERMISSION_DENIED,

    /**
     * The request failed because the destination was not found.
     * The sender should not retry the operation.
     */
    DESTINATION_NOT_FOUND,
}
