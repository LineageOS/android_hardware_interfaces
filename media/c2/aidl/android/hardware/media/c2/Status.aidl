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

package android.hardware.media.c2;

/**
 * Common return values for Codec2 operations.
 */
@VintfStability
parcelable Status {
    /**
     * Operation completed successfully.
     */
    const int OK = 0;
    /**
     * Argument has invalid value (user error).
     */
    const int BAD_VALUE = -22;
    /**
     * Argument uses invalid index (user error).
     */
    const int BAD_INDEX = -75;
    /**
     * Argument/Index is valid but not possible.
     */
    const int CANNOT_DO = -2147483646;
    /**
     * Object already exists.
     */
    const int DUPLICATE = -17;
    /**
     * Object not found.
     */
    const int NOT_FOUND = -2;
    /**
     * Operation is not permitted in the current state.
     */
    const int BAD_STATE = -38;
    /**
     * Operation would block but blocking is not permitted.
     */
    const int BLOCKING = -9930;
    /**
     * Not enough memory to complete operation.
     */
    const int NO_MEMORY = -12;
    /**
     * Missing permission to complete operation.
     */
    const int REFUSED = -1;
    /**
     * Operation did not complete within timeout.
     */
    const int TIMED_OUT = -110;
    /**
     * Operation is not implemented/supported (optional only).
     */
    const int OMITTED = -74;
    /**
     * Some unexpected error prevented the operation.
     */
    const int CORRUPTED = -2147483648;
    /**
     * Status has not been initialized.
     */
    const int NO_INIT = -19;

    int status;
}
