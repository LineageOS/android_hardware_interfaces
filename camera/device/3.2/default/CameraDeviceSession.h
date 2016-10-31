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
    const sp<ICameraDeviceCallback>& mCallback;
    // Stream ID -> Camera3Stream cache
    std::map<int, Camera3Stream> mStreamMap;
    // (streamID, frameNumber) -> inflight buffer cache
    std::map<std::pair<int, uint32_t>, StreamBufferCache>  mInflightBuffers;

    bool mInitFail;
    bool initialize();

    Status initStatus() const;

    // Validate and import request's input buffer and acquire fence
    static Status importRequest(
            const CaptureRequest& request,
            hidl_vec<buffer_handle_t>& allBufs,
            hidl_vec<int>& allFences);

    static void cleanupInflightBufferFences(
            hidl_vec<buffer_handle_t>& allBufs, size_t numBufs,
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
