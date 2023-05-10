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
package android.hardware.security.see.storage;

enum FileIntegrity {
    /** REE may prevent operations, but cannot alter data once written. */
    TAMPER_PROOF_AT_REST,

    /**
     * REE may alter written data, but changes will be detected and reported as
     * an error on read.
     */
    TAMPER_DETECT,

    /**
     * REE may alter written data. Changes other than full filesystem resets will be detected and
     * reported.
     */
    TAMPER_DETECT_IGNORE_RESET,
}
