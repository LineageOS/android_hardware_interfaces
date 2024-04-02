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
package android.hardware.security.see.hwcrypto.types;

/*
 * Service error codes. Will be returned as service specific errors.
 */
parcelable HalErrorCode {
    /* Success */
    const int NO_ERROR = 0;

    /* Generic error */
    const int GENERIC_ERROR = -1;

    /* Desired operation cannot be performed because of the server current state */
    const int BAD_STATE = -2;

    /* Operation or parameters are not supported by the server */
    const int UNSUPPORTED = -3;

    /* Error encountered when parsing parameters */
    const int SERIALIZATION_ERROR = -4;

    /* Server ran out of memory when performing operation */
    const int ALLOCATION_ERROR = -5;

    /* Provided key is not compatible with the operation */
    const int INVALID_KEY = -6;

    /* Bad parameter supplied for the desired operation */
    const int BAD_PARAMETER = -7;
}
