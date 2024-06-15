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

import android.hardware.security.see.storage.CreationMode;

/** The interface for an open file */
interface IFile {
    /**
     * Read bytes from this file.
     *
     * @size:
     *     the size (in bytes) of the segment to read. If @size is larger than the service's maximum
     *       read size, the call will return an error (EX_ILLEGAL_ARGUMENT).
     * @offset:
     *     the offset (in bytes) at which to start reading
     *
     * Return:
     *     the sequence of bytes at [offset, offset + size) in the file
     *
     * May return service-specific errors:
     *   - ERR_FS_* if the filesystem has been tampered with in a way that the `readIntegrity` the
     *       file was opened with does not acknowledge
     */
    byte[] read(long size, long offset);

    /**
     * Write the bytes in `buffer` to this file.
     *
     * @offset:
     *     the offset (in bytes) at which to start writing
     *
     * Return:
     *     the number of bytes written successfully
     *
     * May return service-specific errors:
     *   - ERR_FS_* if the filesystem has been tampered with in a way that the `readIntegrity` the
     *       file was opened with does not acknowledge
     */
    long write(long offset, in byte[] buffer);

    /**
     * Reads this file's size.
     *
     * May return service-specific errors:
     *   - ERR_FS_* if the filesystem has been tampered with in a way that the `readIntegrity` the
     *       file was opened with does not acknowledge
     */
    long getSize();

    /**
     * Sets this file's size.
     *
     * Truncates the file if `new_size` is less than the current size. If `new_size` is greater than
     * the current size, the file will be extended with zeroed data.
     *
     * @newSize:
     *     the file's new size
     *
     * May return service-specific errors:
     *   - ERR_FS_* if the filesystem has been tampered with in a way that the `readIntegrity` the
     *       file was opened with does not acknowledge
     */
    void setSize(long newSize);

    /**
     * Renames this file.
     *
     * @destPath:
     *     the file's new path, relative to filesystem root
     * @destCreateMode:
     *     controls creation behavior of the dest file
     *
     * May return service-specific errors:
     *   - ERR_NOT_FOUND if no file exists at @destPath and @destCreateMode is `NO_CREATE`
     *   - ERR_ALREADY_EXISTS if a file already exists at @destPath and @destCreateMode is
     *       `CREATE_EXCLUSIVE`
     *   - ERR_FS_* if the filesystem has been tampered with in a way that the `readIntegrity` the
     *       file was opened with does not acknowledge
     */
    void rename(in @utf8InCpp String destPath, in CreationMode destCreateMode);
}
