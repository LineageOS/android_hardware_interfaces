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

/**
 * A crypto Pattern is a repeating sequence of encrypted and clear blocks
 * occurring within the bytes indicated by mNumBytesOfEncryptedDatad bytes
 * of a subsample. Patterns are used to reduce the CPU overhead of
 * decrypting samples. As an example, HLS uses 1:9 patterns where every
 * 10th block is encrypted.
 */
@VintfStability
parcelable Pattern {
    /**
     * The number of blocks to be encrypted in the pattern. If zero,
     * pattern encryption is inoperative.
     */
    int encryptBlocks;

    /**
     * The number of blocks to be skipped (left clear) in the pattern. If
     * zero, pattern encryption is inoperative.
     */
    int skipBlocks;
}
