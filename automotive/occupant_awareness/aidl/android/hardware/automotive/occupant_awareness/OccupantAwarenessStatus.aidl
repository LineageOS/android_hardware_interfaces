/*
 * Copyright (C) 2019 The Android Open Source Project
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

package android.hardware.automotive.occupant_awareness;

@VintfStability
@Backing(type="byte")
enum OccupantAwarenessStatus {
    /*
     * System is online and ready to serve requests.
     */
    READY = 0,
    /**
     * Detection is not supported in this vehicle due to a permanent lack of capabilities. Clients
     * need not retry.
     */
    NOT_SUPPORTED = 1,
    /*
     * The system has not yet been initialized. No requests can be served until the
     * initialization process completes. This state does not indicate any error and
     * clients should retry later.
     */
    NOT_INITIALIZED = 2,
    /*
     * A permanent failure has occurred. No detections will be provided.
     */
    FAILURE = 3,
}
