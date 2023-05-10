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

import android.hardware.security.see.storage.FileProperties;
import android.hardware.security.see.storage.IStorageSession;

/**
 * Interface for the Secure Storage HAL
 *
 * Creates sessions which can be used to access storage.
 */
interface ISecureStorage {
    const int ERR_UNSUPPORTED_PROPERTIES = 1;
    const int ERR_NOT_FOUND = 2;
    const int ERR_ALREADY_EXISTS = 3;
    const int ERR_BAD_TRANSACTION = 4;

    const int ERR_FS_RESET = 5;
    const int ERR_FS_ROLLED_BACK = 6;
    const int ERR_FS_TAMPERED = 7;

    /**
     * Starts a storage session for a filesystem.
     *
     * @properties:
     *     the minimum filesystem properties requested for the session.
     *
     * May return service-specific errors:
     *   - ERR_UNSUPPORTED_PROPERTIES if no filesystems exist which meet the minimum requested
     * requirements
     */
    IStorageSession startSession(in FileProperties properties);
}
