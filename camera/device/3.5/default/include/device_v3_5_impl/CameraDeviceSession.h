/*
 * Copyright (C) 2018 The Android Open Source Project
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

#ifndef ANDROID_HARDWARE_CAMERA_DEVICE_V3_5_CAMERADEVICE3SESSION_H
#define ANDROID_HARDWARE_CAMERA_DEVICE_V3_5_CAMERADEVICE3SESSION_H

#include <android/hardware/camera/device/3.5/ICameraDevice.h>
#include <android/hardware/camera/device/3.5/ICameraDeviceSession.h>
#include <android/hardware/camera/device/3.5/ICameraDeviceCallback.h>
#include <../../3.4/default/include/device_v3_4_impl/CameraDeviceSession.h>
#include <unordered_map>

namespace android {
namespace hardware {
namespace camera {
namespace device {
namespace V3_5 {
namespace implementation {

using namespace ::android::hardware::camera::device;
using ::android::hardware::camera::device::V3_2::BufferStatus;
using ::android::hardware::camera::device::V3_2::CaptureRequest;
using ::android::hardware::camera::device::V3_2::StreamBuffer;
using ::android::hardware::camera::device::V3_5::StreamConfiguration;
using ::android::hardware::camera::device::V3_4::HalStreamConfiguration;
using ::android::hardware::camera::device::V3_5::ICameraDeviceSession;
using ::android::hardware::camera::device::V3_5::ICameraDeviceCallback;
using ::android::hardware::camera::device::V3_2::implementation::Camera3Stream;
using ::android::hardware::camera::common::V1_0::Status;
using ::android::hardware::camera::common::V1_0::helper::HandleImporter;
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
    typedef camera3_buffer_request_status_t (callbacks_request_stream_buffer_t)(
            const struct camera3_callback_ops *,
            uint32_t num_buffer_reqs,
            const camera3_buffer_request_t *buffer_reqs,
            /*out*/uint32_t *num_returned_buf_reqs,
            /*out*/camera3_stream_buffer_ret_t *returned_buf_reqs);

    typedef void (callbacks_return_stream_buffer_t)(
            const struct camera3_callback_ops *,
            uint32_t num_buffers,
            const camera3_stream_buffer_t* const* buffers);
}

struct CameraDeviceSession : public V3_4::implementation::CameraDeviceSession {

    CameraDeviceSession(camera3_device_t*,
            const camera_metadata_t* deviceInfo,
            const sp<V3_2::ICameraDeviceCallback>&);
    virtual ~CameraDeviceSession();

    virtual sp<V3_2::ICameraDeviceSession> getInterface() override {
        return new TrampolineSessionInterface_3_5(this);
    }

protected:
    // Methods from v3.4 and earlier will trampoline to inherited implementation
    Return<void> configureStreams_3_5(
            const StreamConfiguration& requestedConfiguration,
            ICameraDeviceSession::configureStreams_3_5_cb _hidl_cb);

    Return<void> signalStreamFlush(
            const hidl_vec<int32_t>& streamIds,
            uint32_t streamConfigCounter);

    virtual Status importRequest(
            const CaptureRequest& request,
            hidl_vec<buffer_handle_t*>& allBufPtrs,
            hidl_vec<int>& allFences) override;

    Return<void> isReconfigurationRequired(const V3_2::CameraMetadata& oldSessionParams,
            const V3_2::CameraMetadata& newSessionParams,
            ICameraDeviceSession::isReconfigurationRequired_cb _hidl_cb);
    /**
     * Static callback forwarding methods from HAL to instance
     */
    static callbacks_request_stream_buffer_t sRequestStreamBuffers;
    static callbacks_return_stream_buffer_t sReturnStreamBuffers;

    camera3_buffer_request_status_t requestStreamBuffers(
            uint32_t num_buffer_reqs,
            const camera3_buffer_request_t *buffer_reqs,
            /*out*/uint32_t *num_returned_buf_reqs,
            /*out*/camera3_stream_buffer_ret_t *returned_buf_reqs);

    void returnStreamBuffers(
            uint32_t num_buffers,
            const camera3_stream_buffer_t* const* buffers);

    struct BufferHasher {
        size_t operator()(const buffer_handle_t& buf) const {
            if (buf == nullptr)
                return 0;

            size_t result = 1;
            result = 31 * result + buf->numFds;
            for (int i = 0; i < buf->numFds; i++) {
                result = 31 * result + buf->data[i];
            }
            return result;
        }
    };

    struct BufferComparator {
        bool operator()(const buffer_handle_t& buf1, const buffer_handle_t& buf2) const {
            if (buf1->numFds == buf2->numFds) {
                for (int i = 0; i < buf1->numFds; i++) {
                    if (buf1->data[i] != buf2->data[i]) {
                        return false;
                    }
                }
                return true;
            }
            return false;
        }
    };

    Camera3Stream* getStreamPointer(int32_t streamId);

    // Register buffer to mBufferIdMaps so we can find corresponding bufferId
    // when the buffer is returned to camera service
    void pushBufferId(const buffer_handle_t& buf, uint64_t bufferId, int streamId);

    // Method to pop buffer's bufferId from mBufferIdMaps
    // BUFFER_ID_NO_BUFFER is returned if no matching buffer is found
    uint64_t popBufferId(const buffer_handle_t& buf, int streamId);

    // Method to cleanup imported buffer/fences if requestStreamBuffers fails half way
    void cleanupInflightBufferFences(
            std::vector<int>& fences, std::vector<std::pair<buffer_handle_t, int>>& bufs);

    // Overrides the default constructCaptureResult behavior for buffer management APIs
    virtual uint64_t getCapResultBufferId(const buffer_handle_t& buf, int streamId) override;

    std::mutex mBufferIdMapLock; // protecting mBufferIdMaps and mNextBufferId
    typedef std::unordered_map<const buffer_handle_t, uint64_t,
            BufferHasher, BufferComparator> BufferIdMap;
    // stream ID -> per stream buffer ID map for buffers coming from requestStreamBuffers API
    // Entries are created during requestStreamBuffers when a stream first request a buffer, and
    // deleted in returnStreamBuffers/processCaptureResult* when all buffers are returned
    std::unordered_map<int, BufferIdMap> mBufferIdMaps;

    sp<ICameraDeviceCallback> mCallback_3_5;
    bool mSupportBufMgr;

private:

    struct TrampolineSessionInterface_3_5 : public ICameraDeviceSession {
        TrampolineSessionInterface_3_5(sp<CameraDeviceSession> parent) :
                mParent(parent) {}

        virtual Return<void> constructDefaultRequestSettings(
                V3_2::RequestTemplate type,
                V3_3::ICameraDeviceSession::constructDefaultRequestSettings_cb _hidl_cb) override {
            return mParent->constructDefaultRequestSettings(type, _hidl_cb);
        }

        virtual Return<void> configureStreams(
                const V3_2::StreamConfiguration& requestedConfiguration,
                V3_3::ICameraDeviceSession::configureStreams_cb _hidl_cb) override {
            return mParent->configureStreams(requestedConfiguration, _hidl_cb);
        }

        virtual Return<void> processCaptureRequest_3_4(
                const hidl_vec<V3_4::CaptureRequest>& requests,
                const hidl_vec<V3_2::BufferCache>& cachesToRemove,
                ICameraDeviceSession::processCaptureRequest_3_4_cb _hidl_cb) override {
            return mParent->processCaptureRequest_3_4(requests, cachesToRemove, _hidl_cb);
        }

        virtual Return<void> processCaptureRequest(const hidl_vec<V3_2::CaptureRequest>& requests,
                const hidl_vec<V3_2::BufferCache>& cachesToRemove,
                V3_3::ICameraDeviceSession::processCaptureRequest_cb _hidl_cb) override {
            return mParent->processCaptureRequest(requests, cachesToRemove, _hidl_cb);
        }

        virtual Return<void> getCaptureRequestMetadataQueue(
                V3_3::ICameraDeviceSession::getCaptureRequestMetadataQueue_cb _hidl_cb) override  {
            return mParent->getCaptureRequestMetadataQueue(_hidl_cb);
        }

        virtual Return<void> getCaptureResultMetadataQueue(
                V3_3::ICameraDeviceSession::getCaptureResultMetadataQueue_cb _hidl_cb) override  {
            return mParent->getCaptureResultMetadataQueue(_hidl_cb);
        }

        virtual Return<Status> flush() override {
            return mParent->flush();
        }

        virtual Return<void> close() override {
            return mParent->close();
        }

        virtual Return<void> configureStreams_3_3(
                const V3_2::StreamConfiguration& requestedConfiguration,
                configureStreams_3_3_cb _hidl_cb) override {
            return mParent->configureStreams_3_3(requestedConfiguration, _hidl_cb);
        }

        virtual Return<void> configureStreams_3_4(
                const V3_4::StreamConfiguration& requestedConfiguration,
                configureStreams_3_4_cb _hidl_cb) override {
            return mParent->configureStreams_3_4(requestedConfiguration, _hidl_cb);
        }

        virtual Return<void> configureStreams_3_5(
                const StreamConfiguration& requestedConfiguration,
                configureStreams_3_5_cb _hidl_cb) override {
            return mParent->configureStreams_3_5(requestedConfiguration, _hidl_cb);
        }

        virtual Return<void> signalStreamFlush(
                const hidl_vec<int32_t>& requests,
                uint32_t streamConfigCounter) override {
            return mParent->signalStreamFlush(requests, streamConfigCounter);
        }

        virtual Return<void> isReconfigurationRequired(const V3_2::CameraMetadata& oldSessionParams,
                const V3_2::CameraMetadata& newSessionParams,
                ICameraDeviceSession::isReconfigurationRequired_cb _hidl_cb) override {
            return mParent->isReconfigurationRequired(oldSessionParams, newSessionParams, _hidl_cb);
        }
    private:
        sp<CameraDeviceSession> mParent;
    };
};

}  // namespace implementation
}  // namespace V3_5
}  // namespace device
}  // namespace camera
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_CAMERA_DEVICE_V3_5_CAMERADEVICE3SESSION_H
