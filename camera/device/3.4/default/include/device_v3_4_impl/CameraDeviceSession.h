/*
 * Copyright (C) 2017-2018 The Android Open Source Project
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

#ifndef ANDROID_HARDWARE_CAMERA_DEVICE_V3_4_CAMERADEVICE3SESSION_H
#define ANDROID_HARDWARE_CAMERA_DEVICE_V3_4_CAMERADEVICE3SESSION_H

#include <android/hardware/camera/device/3.2/ICameraDevice.h>
#include <android/hardware/camera/device/3.4/ICameraDeviceSession.h>
#include <android/hardware/camera/device/3.4/ICameraDeviceCallback.h>
#include <../../3.3/default/CameraDeviceSession.h>
#include <../../3.3/default/include/convert.h>
#include <fmq/MessageQueue.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>
#include <deque>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include "CameraMetadata.h"
#include "HandleImporter.h"
#include "hardware/camera3.h"
#include "hardware/camera_common.h"
#include "utils/Mutex.h"

namespace android {
namespace hardware {
namespace camera {
namespace device {
namespace V3_4 {
namespace implementation {

using namespace ::android::hardware::camera::device;
using ::android::hardware::camera::device::V3_2::CaptureRequest;
using ::android::hardware::camera::device::V3_2::StreamType;
using ::android::hardware::camera::device::V3_4::StreamConfiguration;
using ::android::hardware::camera::device::V3_4::HalStreamConfiguration;
using ::android::hardware::camera::device::V3_4::ICameraDeviceSession;
using ::android::hardware::camera::device::V3_4::ICameraDeviceCallback;
using ::android::hardware::camera::common::V1_0::Status;
using ::android::hardware::camera::common::V1_0::helper::HandleImporter;
using ::android::hardware::kSynchronizedReadWrite;
using ::android::hardware::MessageQueue;
using ::android::hardware::MQDescriptorSync;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::hidl_vec;
using ::android::hardware::hidl_string;
using ::android::sp;
using ::android::Mutex;

struct CameraDeviceSession : public V3_3::implementation::CameraDeviceSession {

    CameraDeviceSession(camera3_device_t*,
            const camera_metadata_t* deviceInfo,
            const sp<V3_2::ICameraDeviceCallback>&);
    virtual ~CameraDeviceSession();

    virtual sp<V3_2::ICameraDeviceSession> getInterface() override {
        return new TrampolineSessionInterface_3_4(this);
    }

protected:
    // Methods from v3.3 and earlier will trampoline to inherited implementation

    Return<void> configureStreams_3_4(
            const StreamConfiguration& requestedConfiguration,
            ICameraDeviceSession::configureStreams_3_4_cb _hidl_cb);

    bool preProcessConfigurationLocked_3_4(
            const StreamConfiguration& requestedConfiguration,
            camera3_stream_configuration_t *stream_list /*out*/,
            hidl_vec<camera3_stream_t*> *streams /*out*/);
    void postProcessConfigurationLocked_3_4(const StreamConfiguration& requestedConfiguration);
    void postProcessConfigurationFailureLocked_3_4(
            const StreamConfiguration& requestedConfiguration);

    Return<void> processCaptureRequest_3_4(
            const hidl_vec<V3_4::CaptureRequest>& requests,
            const hidl_vec<V3_2::BufferCache>& cachesToRemove,
            ICameraDeviceSession::processCaptureRequest_3_4_cb _hidl_cb);
    Status processOneCaptureRequest_3_4(const V3_4::CaptureRequest& request);

    std::map<int, std::string> mPhysicalCameraIdMap;

    static V3_2::implementation::callbacks_process_capture_result_t sProcessCaptureResult_3_4;
    static V3_2::implementation::callbacks_notify_t sNotify_3_4;

    class ResultBatcher_3_4 : public V3_3::implementation::CameraDeviceSession::ResultBatcher {
    public:
        ResultBatcher_3_4(const sp<V3_2::ICameraDeviceCallback>& callback);
        void processCaptureResult_3_4(CaptureResult& result);
    private:
        void freeReleaseFences_3_4(hidl_vec<CaptureResult>&);
        void processOneCaptureResult_3_4(CaptureResult& result);
        void invokeProcessCaptureResultCallback_3_4(hidl_vec<CaptureResult> &results,
                bool tryWriteFmq);

        sp<ICameraDeviceCallback> mCallback_3_4;
    } mResultBatcher_3_4;

    // Whether this camera device session is created with version 3.4 callback.
    bool mHasCallback_3_4;

    // Physical camera ids for the logical multi-camera. Empty if this
    // is not a logical multi-camera.
    std::unordered_set<std::string> mPhysicalCameraIds;
private:

    struct TrampolineSessionInterface_3_4 : public ICameraDeviceSession {
        TrampolineSessionInterface_3_4(sp<CameraDeviceSession> parent) :
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

        virtual Return<void> processCaptureRequest_3_4(const hidl_vec<V3_4::CaptureRequest>& requests,
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
                const StreamConfiguration& requestedConfiguration,
                configureStreams_3_4_cb _hidl_cb) override {
            return mParent->configureStreams_3_4(requestedConfiguration, _hidl_cb);
        }

    private:
        sp<CameraDeviceSession> mParent;
    };
};

}  // namespace implementation
}  // namespace V3_4
}  // namespace device
}  // namespace camera
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_CAMERA_DEVICE_V3_4_CAMERADEVICE3SESSION_H
