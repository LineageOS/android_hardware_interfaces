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

#ifndef ANDROID_HARDWARE_CAMERA_DEVICE_V3_2_CAMERADEVICE3SESSION_H
#define ANDROID_HARDWARE_CAMERA_DEVICE_V3_2_CAMERADEVICE3SESSION_H

#include <unordered_map>
#include "hardware/camera_common.h"
#include "hardware/camera3.h"
#include "utils/Mutex.h"
#include <android/hardware/camera/device/3.2/ICameraDevice.h>
#include <android/hardware/camera/device/3.2/ICameraDeviceSession.h>
#include <hidl/Status.h>
#include <hidl/MQDescriptor.h>
#include <include/convert.h>

namespace android {
namespace hardware {
namespace camera {
namespace device {
namespace V3_2 {
namespace implementation {

using ::android::hardware::camera::device::V3_2::CaptureRequest;
using ::android::hardware::camera::device::V3_2::HalStreamConfiguration;
using ::android::hardware::camera::device::V3_2::StreamConfiguration;
using ::android::hardware::camera::device::V3_2::ICameraDeviceSession;
using ::android::hardware::camera::common::V1_0::Status;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::hidl_vec;
using ::android::hardware::hidl_string;
using ::android::sp;
using ::android::Mutex;

/**
 * Function pointer types with C calling convention to
 * use for HAL callback functions.
 */
extern "C" {
    typedef void (callbacks_process_capture_result_t)(
        const struct camera3_callback_ops *,
        const camera3_capture_result_t *);

    typedef void (callbacks_notify_t)(
        const struct camera3_callback_ops *,
        const camera3_notify_msg_t *);
}

struct CameraDeviceSession : public ICameraDeviceSession, private camera3_callback_ops  {

    CameraDeviceSession(camera3_device_t*, const sp<ICameraDeviceCallback>&);
    ~CameraDeviceSession();
    // Call by CameraDevice to dump active device states
    void dumpState(const native_handle_t* fd);
    // Caller must use this method to check if CameraDeviceSession ctor failed
    bool isInitFailed() { return mInitFail; }
    // Used by CameraDevice to signal external camera disconnected
    void disconnect();
    bool isClosed();

    // Methods from ::android::hardware::camera::device::V3_2::ICameraDeviceSession follow.
    Return<void> constructDefaultRequestSettings(RequestTemplate type, constructDefaultRequestSettings_cb _hidl_cb) override;
    Return<void> configureStreams(const StreamConfiguration& requestedConfiguration, configureStreams_cb _hidl_cb) override;
    Return<Status> processCaptureRequest(const CaptureRequest& request) override;
    Return<Status> flush() override;
    Return<void> close() override;

private:
    // protecting mClosed/mDisconnected/mInitFail
    mutable Mutex mStateLock;
    // device is closed either
    //    - closed by user
    //    - init failed
    //    - camera disconnected
    bool mClosed = false;

    // Set by CameraDevice (when external camera is disconnected)
    bool mDisconnected = false;

    camera3_device_t* mDevice;
    const sp<ICameraDeviceCallback> mCallback;
    // Stream ID -> Camera3Stream cache
    std::map<int, Camera3Stream> mStreamMap;

    mutable Mutex mInflightLock; // protecting mInflightBuffers and mCirculatingBuffers
    // (streamID, frameNumber) -> inflight buffer cache
    std::map<std::pair<int, uint32_t>, camera3_stream_buffer_t>  mInflightBuffers;

    struct BufferHasher {
        size_t operator()(const buffer_handle_t& buf) const {
            if (buf == nullptr)
                return 0;

            size_t result = 1;
            result = 31 * result + buf->numFds;
            result = 31 * result + buf->numInts;
            int length = buf->numFds + buf->numInts;
            for (int i = 0; i < length; i++) {
                result = 31 * result + buf->data[i];
            }
            return result;
        }
    };

    struct BufferComparator {
        bool operator()(const buffer_handle_t& buf1, const buffer_handle_t& buf2) const {
            if (buf1->numFds == buf2->numFds && buf1->numInts == buf2->numInts) {
                int length = buf1->numFds + buf1->numInts;
                for (int i = 0; i < length; i++) {
                    if (buf1->data[i] != buf2->data[i]) {
                        return false;
                    }
                }
                return true;
            }
            return false;
        }
    };

    // buffers currently ciculating between HAL and camera service
    // key: buffer_handle_t sent via HIDL interface
    // value: imported buffer_handle_t
    // Buffer will be imported during process_capture_request and will be freed
    // when the its stream is deleted or camera device session is closed
    typedef std::unordered_map<buffer_handle_t, buffer_handle_t,
            BufferHasher, BufferComparator> CirculatingBuffers;
    // Stream ID -> circulating buffers map
    std::map<int, CirculatingBuffers> mCirculatingBuffers;

    bool mInitFail;
    bool initialize();

    Status initStatus() const;

    // Validate and import request's input buffer and acquire fence
    Status importRequest(
            const CaptureRequest& request,
            hidl_vec<buffer_handle_t*>& allBufPtrs,
            hidl_vec<int>& allFences);

    static void cleanupInflightFences(
            hidl_vec<int>& allFences, size_t numFences);

    /**
     * Static callback forwarding methods from HAL to instance
     */
    static callbacks_process_capture_result_t sProcessCaptureResult;
    static callbacks_notify_t sNotify;
};

}  // namespace implementation
}  // namespace V3_2
}  // namespace device
}  // namespace camera
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_CAMERA_DEVICE_V3_2_CAMERADEVICE3SESSION_H
