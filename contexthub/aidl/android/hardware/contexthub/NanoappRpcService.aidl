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

package android.hardware.contexthub;

/**
 * An RPC service exposed by a nanoapp.
 *
 * The implementation of the RPC interface is not defined by the HAL, and is written
 * at the messaging endpoint layers (Android app and/or CHRE nanoapp). NanoappRpcService
 * contains the informational metadata to be consumed by the RPC interface layer.
 */
@VintfStability
parcelable NanoappRpcService {
    /**
     * The unique 64-bit ID of an RPC service exposed by a nanoapp. Note that
     * the uniqueness is only required within the nanoapp's domain (i.e. the
     * combination of the nanoapp ID and service id must be unique).
     */
    long id;

    /**
     * The software version of this service, which follows the semantic
     * versioning scheme (see semver.org). It follows the format
     * major.minor.patch, where major and minor versions take up one byte
     * each, and the patch version takes up the final 2 bytes.
     */
    int version;
}
