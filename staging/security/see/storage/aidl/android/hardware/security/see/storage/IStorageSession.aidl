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

import android.hardware.security.see.storage.DeleteOptions;
import android.hardware.security.see.storage.IDir;
import android.hardware.security.see.storage.IFile;
import android.hardware.security.see.storage.OpenOptions;
import android.hardware.security.see.storage.ReadIntegrity;
import android.hardware.security.see.storage.RenameOptions;

/**
 * Interface for a Secure Storage session
 *
 * When the connection is opened, it will start a transaction and any changes made through this
 * session or the interfaces this session returns will be added to this transaction's pending
 * changes. Calling `CommitChanges`/`AbandonChanges` will commit/abandon these pending changes, and
 * start a new, empty transaction. The interfaces this session returns _remain_ valid across
 * transactions; it is not necessary, for example, to reopen a file after a commit.
 *
 * Any changes still pending when the session is dropped will be abandoned.
 */
interface IStorageSession {
    /**
     * Commits any pending changes made through this session to storage.
     *
     * The session will no longer have pending changes after this call returns. Files may then still
     * be modified through this session to create another commit.
     *
     * May return service-specific errors:
     *   - ERR_BAD_TRANSACTION
     */
    void commitChanges();

    /**
     * Abandons any pending changes made through this session.
     *
     * The session can then be reused to make new changes.
     */
    void abandonChanges();

    /**
     * Opens a secure file for writing and/or reading.
     *
     * Changes made to the file are part of the current transaction. Dropping this session
     * invalidates the returned `IFile` interface
     *
     * @filePath:
     *     path to the file, relative to filesystem root
     * @options:
     *     options controlling opening behavior
     *
     * May return service-specific errors:
     *   - ERR_NOT_FOUND
     *   - ERR_ALREADY_EXISTS
     *   - ERR_FS_* if the filesystem has been tampered with in a way that @options.readIntegrity
     *       does not acknowledge
     */
    IFile openFile(in @utf8InCpp String filePath, in OpenOptions options);

    /**
     * Delete a file.
     *
     * @filePath:
     *     path to the file, relative to filesystem root
     * @options:
     *     options controlling deletion behavior
     *
     * May return service-specific errors:
     *   - ERR_NOT_FOUND
     *   - ERR_FS_* if the filesystem has been tampered with in a way that @options.readIntegrity
     *       does not acknowledge
     */
    void deleteFile(in @utf8InCpp String filePath, in DeleteOptions options);

    /**
     * Renames an existing file.
     *
     * The file must not already be opened. (If it is, use `IFile::rename`.)
     *
     * @currentPath:
     *     path to the file, relative to filesystem root
     * @destPath:
     *     the file's new path, relative to filesystem root
     * @options:
     *     options controlling rename behavior
     *
     * May return service-specific errors:
     *   - ERR_NOT_FOUND if no file exists at @currentPath, or if @options.destCreateMode is
     *       `NO_CREATE` and no file exists at @destPath
     *   - ERR_ALREADY_EXISTS if @options.destCreateMode is `CREATE_EXCLUSIVE` and a file exists at
     *       @destPath
     *   - ERR_FS_* if the filesystem has been tampered with in a way that @options.readIntegrity
     *       does not acknowledge
     */
    void renameFile(in @utf8InCpp String currentPath, in @utf8InCpp String destPath,
            in RenameOptions options);

    /**
     * Opens a directory from a filesystem with the given properties.
     *
     * Dropping this session invalidates the returned `IDir` interface.
     *
     * @path:
     *     path to the directory, relative to filesystem root
     * @readIntegrity:
     *     allow opening (and subsequent read/write operations) despite possible tampering for the
     * directory
     *
     * May return service-specific errors:
     *   - ERR_NOT_FOUND
     *   - ERR_FS_* if the filesystem has been tampered with in a way that @readIntegrity does not
     *       acknowledge
     */
    IDir openDir(in @utf8InCpp String path, in ReadIntegrity readIntegrity);
}
