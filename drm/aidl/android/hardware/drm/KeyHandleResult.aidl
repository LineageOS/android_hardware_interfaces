/*
 * Copyright (C) 2025 The Android Open Source Project
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

/**
 * The result of a getKeyHandle call from ICryptoPlugin.
 *
 * This structure holds the resulting opaque key handle.
 * The key handle is used by components that perform decryption and decoding
 * in the same step.
 */
@VintfStability
parcelable KeyHandleResult {
    /**
     * An opaque handle to the selected key.
     */
    byte[] keyHandle;
}
