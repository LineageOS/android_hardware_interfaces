/**
 * Copyright (c) 2021, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.hardware.graphics.composer3.command;

import android.hardware.graphics.composer3.LayerGenericMetadataKey;

@VintfStability
parcelable GenericMetadata {
    /**
     * Indicates which metadata value should be set.
     */
    LayerGenericMetadataKey key;
    /**
     * The binary representation of a AIDL struct corresponding to
     * the key as described above.
     * TODO(b/209691612): revisit the use of byte[]
     */
    byte[] value;
}
