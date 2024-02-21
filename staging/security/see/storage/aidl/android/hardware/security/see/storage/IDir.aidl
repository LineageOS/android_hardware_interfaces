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

/** The interface for an open directory */
interface IDir {
    /**
     * Gets the next batch of filenames in this directory.
     *
     * Calling multiple times will return different results as the IDir iterates through all the
     * files it contains. When all filenames have been returned, all successive calls will return an
     * empty list.
     *
     * @maxCount:
     *     the maximum number of filenames to return. A @maxCount of 0 signifies no limit on the
     * number of filenames returned.
     *
     * Returns:
     *     An ordered list of filenames. If @maxCount > 0, the length of the returned list will be
     * less than or equal to @maxCount.
     *
     * May return service-specific errors:
     *   - ERR_FS_* if the filesystem has been tampered with in a way that the `readIntegrity` the
     *       dir was opened with does not acknowledge
     */
    @utf8InCpp String[] readNextFilenames(int maxCount);
}
