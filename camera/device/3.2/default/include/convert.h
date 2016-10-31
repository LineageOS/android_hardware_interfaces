/*
 * Copyright (C) 2016 The Android Open Source Project
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

#ifndef HARDWARE_INTERFACES_CAMERA_DEVICE_V3_2_DEFAULT_INCLUDE_CONVERT_H_

#define HARDWARE_INTERFACES_CAMERA_DEVICE_V3_2_DEFAULT_INCLUDE_CONVERT_H_

#include <set>


#include <android/hardware/graphics/common/1.0/types.h>
#include <android/hardware/camera/device/3.2/types.h>
#include "hardware/camera3.h"

namespace android {
namespace hardware {
namespace camera {
namespace device {
namespace V3_2 {
namespace implementation {

// Cacheing the buffer/fence from camera service so HAL can reference the pointer after the
// processCaptureRequest call has returned.
// Remove the cache when:
//     1. HAL API call failed, or
//     2. HAL returns the buffer and the callback to camera service has returned
struct StreamBufferCache {
    buffer_handle_t mBuffer;
    camera3_stream_buffer_t mStreamBuffer;
};

// The camera3_stream_t sent to conventional HAL. Added mId fields to enable stream ID lookup
// fromt a downcasted camera3_stream
struct Camera3Stream : public camera3_stream {
    int mId;
};

// *dst will point to the data owned by src, but src still owns the data after this call returns.
bool convertFromHidl(const CameraMetadata &src, const camera_metadata_t** dst);
void convertToHidl(const camera_metadata_t* src, CameraMetadata* dst);

void convertFromHidl(const Stream &src, Camera3Stream* dst);
void convertToHidl(const Camera3Stream* src, HalStream* dst);

// dst->mStreamBuffer.buffer will be pointing to dst->mBuffer.
// Most likely dst will be passed to HAL and HAL will try to access mStreamBuffer.buffer
// after the API call returns. In that case caller must not use a local variable
// within the scope of the API call to hold dst, because then dst->mStreamBuffer.buffer will be
// invalid after the API call returns.
void convertFromHidl(
        buffer_handle_t, BufferStatus, camera3_stream_t*, int acquireFence, // inputs
        StreamBufferCache* dst);

void convertToHidl(const camera3_stream_configuration_t& src, HalStreamConfiguration* dst);

// The camera3_stream_t* in src must be the same as what wrapper HAL passed to conventional
// HAL, or the ID lookup will return garbage. Caller should validate the ID in ErrorMsg is
// indeed one of active stream IDs
void convertToHidl(const camera3_notify_msg* src, NotifyMsg* dst);

}  // namespace implementation
}  // namespace V3_2
}  // namespace device
}  // namespace camera
}  // namespace hardware
}  // namespace android

#endif  // HARDWARE_INTERFACES_CAMERA_DEVICE_V3_2_DEFAULT_INCLUDE_CONVERT_H_
