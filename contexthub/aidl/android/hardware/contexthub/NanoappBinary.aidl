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

@VintfStability
parcelable NanoappBinary {
    /** Indicates that the nanoapp is securely signed (e.g. for production) */
    const int FLAG_SIGNED = 1 << 0;
    const int FLAG_ENCRYPTED = 1 << 1;
    /** Indicates that the nanoapp can run on a Context Hub's TCM memory region */
    const int FLAG_TCM_CAPABLE = 1 << 2;

    /**
     * The unique identifier of the nanoapp for the entire system. See chreNanoappInfo in
     * system/chre/chre_api/include/chre_api/chre/event.h for the convention for choosing
     * this ID.
     */
    long nanoappId;

    /** The version of the nanoapp. */
    int nanoappVersion;

    /** The nanoapp flags, comprised of the bitmasks defined in FLAG_* constants above. */
    int flags;

    /**
     * The version of the CHRE API that this nanoapp was compiled against. See
     * the CHRE API header file chre/version.h for more information. The hub
     * implementation must use this to confirm compatibility before loading
     * this nanoapp.
     */
    byte targetChreApiMajorVersion;
    byte targetChreApiMinorVersion;

    /**
     * Implementation-specific binary nanoapp data. This does not include the
     * common nanoapp header that contains the app ID, etc., as this data is
     * explicitly passed through the other fields in this struct.
     */
    byte[] customBinary;
}
