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

package android.hardware.automotive.evs;

import android.hardware.graphics.common.HardwareBuffer;

/**
 * Structure representing an image buffer through our APIs
 *
 * In addition to the handle to the graphics memory, we need to retain
 * the properties of the buffer for easy reference and reconstruction of
 * an ANativeWindowBuffer object on the remote side of API calls.
 * (Not least because OpenGL expect an ANativeWindowBuffer* for us as a
 * texture via eglCreateImageKHR()).
 */
@VintfStability
parcelable BufferDesc {
    /**
     * Stable AIDL counter part of AHardwareBuffer.  Please see
     * hardware/interfaces/graphics/common/aidl/android/hardware/graphics/common/HardwareBuffer.aidl
     * for more details.
     */
    HardwareBuffer buffer;
    /**
     * The size of a pixel in the units of bytes.
     */
    int pixelSizeBytes;
    /**
     * Opaque value from driver
     */
    int bufferId;
    /**
     * Unique identifier of the physical camera device that produces this buffer.
     */
    @utf8InCpp
    String deviceId;
    /**
     * Time that this buffer is being filled in the units of microseconds and must be
     * obtained from android::elapsedRealtimeNanos() or its equivalents.
     */
    long timestamp;
    /**
     * Frame metadata.  This is opaque to EvsManager.
     */
    byte[] metadata;
}
