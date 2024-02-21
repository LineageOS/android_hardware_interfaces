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

enum ReadIntegrity {
    /**
     * Return an error on reads if any REE alteration of the written data
     * has been detected.
     */
    NO_TAMPER,

    /**
     * Return an error on reads if any REE alteration other than a reset
     * has been detected.
     */
    IGNORE_RESET,

    /**
     * Return an error if any REE alteration other than a rollback to a
     * valid checkpoint has been detected. (What makes a checkpoint valid is
     * implementation defined; an implementation might take a checkpoint on its
     * first post-factory boot. A reset is a rollback to the initial state.)
     */
    IGNORE_ROLLBACK,

    // There's no `IGNORE_ALL` because if REE has done any alteration other
    // than a rollback, the file contents will be known-bad data.
}
