/*
 * Copyright (C) 2022 The Android Open Source Project
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

package android.hardware.wifi;

/**
 * Struct describing each debug ring buffer supported by
 * the device.
 */
@VintfStability
parcelable WifiDebugRingBufferStatus {
    /**
     * Name of this debug ring buffer.
     */
    String ringName;
    /**
     * Combination of |WifiDebugRingBufferFlags| values.
     */
    int flags;
    /**
     * Unique integer representing the ring.
     */
    int ringId;
    /**
     * Total memory size allocated for the buffer.
     */
    int sizeInBytes;
    /**
     * Amount of free space in the buffer.
     */
    int freeSizeInBytes;
    /**
     * Verbose level for ring buffer.
     */
    int verboseLevel;
}
