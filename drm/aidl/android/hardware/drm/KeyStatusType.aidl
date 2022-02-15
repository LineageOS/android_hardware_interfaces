/*
 * Copyright (C) 2021 The Android Open Source Project
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

package android.hardware.drm;

@VintfStability
@Backing(type="int")
enum KeyStatusType {
    /**
     * The key is currently usable to decrypt media data.
     */
    USABLE,
    /**
     * The key is no longer usable to decrypt media data because its expiration
     * time has passed.
     */
    EXPIRED,
    /**
     * The key is not currently usable to decrypt media data because its output
     * requirements cannot currently be met.
     */
    OUTPUT_NOT_ALLOWED,
    /**
     * The status of the key is not yet known and is being determined.
     */
    STATUS_PENDING,
    /**
     * The key is not currently usable to decrypt media data because of an
     * internal error in processing unrelated to input parameters.
     */
    INTERNAL_ERROR,
    /**
     * The key is not yet usable to decrypt media because the start
     * time is in the future. The key must become usable when
     * its start time is reached.
     */
    USABLE_IN_FUTURE,
}
