/**
 * Copyright (c) 2021, The Android Open Source Project
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

package android.hardware.graphics.composer3;

/**
 * Display requests returned by getDisplayRequests.
 */
@VintfStability
@Backing(type="int")
enum DisplayRequest {
    /**
     * Instructs the client to provide a new client target buffer, even if
     * no layers are marked for client composition.
     */
    FLIP_CLIENT_TARGET = 1 << 0,
    /**
     * Instructs the client to write the result of client composition
     * directly into the virtual display output buffer. If any of the
     * layers are not marked as Composition::CLIENT or the given display
     * is not a virtual display, this request has no effect.
     */
    WRITE_CLIENT_TARGET_TO_OUTPUT = 1 << 1,
}
