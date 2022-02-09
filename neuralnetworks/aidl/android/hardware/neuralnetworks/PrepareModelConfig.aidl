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

package android.hardware.neuralnetworks;

import android.hardware.neuralnetworks.ExecutionPreference;
import android.hardware.neuralnetworks.ExtensionNameAndPrefix;
import android.hardware.neuralnetworks.Priority;
import android.hardware.neuralnetworks.TokenValuePair;

/**
 * A type that is used to represent all configuration needed to
 * prepare a model.
 */
@VintfStability
parcelable PrepareModelConfig {
    /**
     * The byte size of the cache token.
     */
    const int BYTE_SIZE_OF_CACHE_TOKEN = 32;

    /**
     * Indicates the intended execution behavior of a prepared model.
     */
    ExecutionPreference preference;
    /**
     * The priority of the prepared model relative to other prepared
     * models owned by the client.
     */
    Priority priority;
    /**
     * The time by which the model is expected to be prepared. The
     * time is measured in nanoseconds since boot (as from
     * clock_gettime(CLOCK_BOOTTIME, &ts) or
     * ::android::base::boot_clock). If the model cannot be prepared
     * by the deadline, the preparation may be aborted. Passing -1
     * means the deadline is omitted. Other negative values are
     * invalid.
     */
    long deadlineNs;
    /**
     * A vector of file descriptors for the security-sensitive cache.
     * The length of the vector must either be 0 indicating that
     * caching information is not provided, or match the
     * numModelCache returned from IDevice::getNumberOfCacheFilesNeeded. The
     * cache file descriptors will be provided in the same order when
     * retrieving the preparedModel from cache files with
     * IDevice::prepareModelFromCache.
     */
    ParcelFileDescriptor[] modelCache;
    /**
     * A vector of file descriptors for the constants' cache. The
     * length of the vector must either be 0 indicating that caching
     * information is not provided, or match the numDataCache
     * returned from IDevice::getNumberOfCacheFilesNeeded. The cache file
     * descriptors will be provided in the same order when retrieving
     * the preparedModel from cache files with IDevice::prepareModelFromCache.
     */
    ParcelFileDescriptor[] dataCache;
    /**
     * A caching token of length BYTE_SIZE_OF_CACHE_TOKEN identifying
     * the prepared model. The same token will be provided when
     * retrieving the prepared model from the cache files with
     * IDevice::prepareModelFromCache.  Tokens should be chosen to have a low
     * rate of collision for a particular application. The driver
     * cannot detect a collision; a collision will result in a failed
     * execution or in a successful execution that produces incorrect
     * output values. If both modelCache and dataCache are empty
     * indicating that caching information is not provided, this
     * token must be ignored.
     */
    byte[BYTE_SIZE_OF_CACHE_TOKEN] cacheToken;
    /**
     * A vector of token / value pairs represent vendor specific
     * compilation hints or metadata. The provided TokenValuePairs must not
     * contain the same token twice. The driver must validate the
     * data and ignore invalid hints. It is up to the driver to
     * decide whether to respect the provided hints or not.
     */
    TokenValuePair[] compilationHints;
    /**
     * The mapping between extension names and prefixes of token values.
     * The driver must ignore the corresponding compilation hint, if
     * the extension is not supported.
     */
    ExtensionNameAndPrefix[] extensionNameToPrefix;
}
