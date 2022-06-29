/*
 * Copyright 2021 The Android Open Source Project
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

package android.hardware.tv.tuner;

/**
 * Bits Settings for Section Filter.
 * @hide
 */
@VintfStability
parcelable DemuxFilterSectionBits {
    /**
     * The bytes are configured for Section Filter
     */
    byte[] filter;

    /**
     * Active bits in the configured bytes to be used for filtering
     */
    byte[] mask;

    /**
     * Do positive match at the bit position of the configured bytes when the
     * bit at same position of the mode is 0.
     * Do negative match at the bit position of the configured bytes when the
     * bit at same position of the mode is 1.
     */
    byte[] mode;
}
