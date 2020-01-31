/*
 * Copyright (C) 2020 The Android Open Source Project
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

#ifndef ANDROID_HARDWARE_CAMERA_DEVICE_V3_6_EXTCAMERAOFFLINESESSION_H
#define ANDROID_HARDWARE_CAMERA_DEVICE_V3_6_EXTCAMERAOFFLINESESSION_H

#include <android/hardware/camera/device/3.5/ICameraDeviceCallback.h>
#include <android/hardware/camera/device/3.6/ICameraDeviceSession.h>
#include <android/hardware/camera/device/3.6/ICameraOfflineSession.h>
#include <android/hardware/camera/common/1.0/types.h>
#include <fmq/MessageQueue.h>
#include <hidl/MQDescriptor.h>
#include <deque>
#include <../../3.4/default/include/ext_device_v3_4_impl/ExternalCameraUtils.h>
#include <../../3.5/default/include/ext_device_v3_5_impl/ExternalCameraDeviceSession.h>
#include <HandleImporter.h>
#include <Exif.h>
#include <android-base/unique_fd.h>

namespace android {
namespace hardware {
namespace camera {
namespace device {
namespace V3_6 {
namespace implementation {

using ::android::hardware::camera::device::V3_2::BufferCache;
using ::android::hardware::camera::device::V3_5::BufferRequest;
using ::android::hardware::camera::device::V3_5::BufferRequestStatus;
using ::android::hardware::camera::device::V3_2::BufferStatus;
using ::android::hardware::camera::device::V3_2::CameraMetadata;
using ::android::hardware::camera::device::V3_2::CaptureRequest;
using ::android::hardware::camera::device::V3_2::CaptureResult;
using ::android::hardware::camera::device::V3_2::ErrorCode;
using ::android::hardware::camera::device::V3_5::ICameraDeviceCallback;
using ::android::hardware::camera::device::V3_2::MsgType;
using ::android::hardware::camera::device::V3_2::NotifyMsg;
using ::android::hardware::camera::device::V3_2::RequestTemplate;
using ::android::hardware::camera::device::V3_2::Stream;
using ::android::hardware::camera::device::V3_5::StreamConfiguration;
using ::android::hardware::camera::device::V3_2::StreamConfigurationMode;
using ::android::hardware::camera::device::V3_2::StreamRotation;
using ::android::hardware::camera::device::V3_2::StreamType;
using ::android::hardware::camera::device::V3_2::DataspaceFlags;
using ::android::hardware::camera::device::V3_2::CameraBlob;
using ::android::hardware::camera::device::V3_2::CameraBlobId;
using ::android::hardware::camera::device::V3_4::HalStreamConfiguration;
using ::android::hardware::camera::device::V3_6::ICameraOfflineSession;
using ::android::hardware::camera::common::V1_0::Status;
using ::android::hardware::camera::common::V1_0::helper::HandleImporter;
using ::android::hardware::camera::common::V1_0::helper::ExifUtils;
using ::android::hardware::camera::external::common::ExternalCameraConfig;
using ::android::hardware::camera::external::common::Size;
using ::android::hardware::camera::external::common::SizeHasher;
using ::android::hardware::graphics::common::V1_0::BufferUsage;
using ::android::hardware::graphics::common::V1_0::Dataspace;
using ::android::hardware::graphics::common::V1_0::PixelFormat;
using ::android::hardware::kSynchronizedReadWrite;
using ::android::hardware::MessageQueue;
using ::android::hardware::MQDescriptorSync;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::hidl_vec;
using ::android::hardware::hidl_string;
using ::android::sp;
using ::android::Mutex;
using ::android::base::unique_fd;

using ::android::hardware::camera::device::V3_4::implementation::SupportedV4L2Format;
using ::android::hardware::camera::device::V3_4::implementation::CroppingType;
using ::android::hardware::camera::device::V3_4::implementation::CirculatingBuffers;
using ::android::hardware::camera::device::V3_4::implementation::HalRequest;
using ::android::hardware::camera::device::V3_4::implementation::OutputThreadInterface;

struct ExternalCameraOfflineSession : public virtual RefBase,
        public virtual OutputThreadInterface {

    ExternalCameraOfflineSession(
            const CroppingType& croppingType,
            const common::V1_0::helper::CameraMetadata& chars,
            const std::string& cameraId,
            const std::string& exifMake,
            const std::string& exifModel,
            uint32_t blobBufferSize,
            bool afTrigger,
            const hidl_vec<Stream>& offlineStreams,
            std::deque<std::shared_ptr<HalRequest>>& offlineReqs,
            const std::map<int, CirculatingBuffers>& circulatingBuffers);

    bool initialize();

    virtual ~ExternalCameraOfflineSession();

    // Retrieve the HIDL interface, split into its own class to avoid inheritance issues when
    // dealing with minor version revs and simultaneous implementation and interface inheritance
    virtual sp<V3_6::ICameraOfflineSession> getInterface() {
        return new TrampolineSessionInterface_3_6(this);
    }

protected:

    // Methods from OutputThreadInterface
    virtual Status importBuffer(int32_t streamId,
            uint64_t bufId, buffer_handle_t buf,
            /*out*/buffer_handle_t** outBufPtr,
            bool allowEmptyBuf) override;

    virtual Status processCaptureResult(std::shared_ptr<HalRequest>&) override;

    virtual Status processCaptureRequestError(const std::shared_ptr<HalRequest>&,
            /*out*/std::vector<NotifyMsg>* msgs = nullptr,
            /*out*/std::vector<CaptureResult>* results = nullptr) override;

    virtual ssize_t getJpegBufferSize(uint32_t width, uint32_t height) const override;

    virtual void notifyError(uint32_t frameNumber, int32_t streamId, ErrorCode ec) override;
    // End of OutputThreadInterface methods

    class OutputThread : public V3_5::implementation::ExternalCameraDeviceSession::OutputThread {
    public:
        OutputThread(
                wp<OutputThreadInterface> parent, CroppingType ct,
                const common::V1_0::helper::CameraMetadata& chars,
                sp<V3_5::implementation::ExternalCameraDeviceSession::BufferRequestThread> bufReqThread,
                std::deque<std::shared_ptr<HalRequest>>& offlineReqs) :
                V3_5::implementation::ExternalCameraDeviceSession::OutputThread(
                        parent, ct, chars, bufReqThread),
                mOfflineReqs(offlineReqs) {}

        virtual bool threadLoop() override;

    protected:
        std::deque<std::shared_ptr<HalRequest>> mOfflineReqs;
    }; // OutputThread


    Return<void> setCallback(const sp<ICameraDeviceCallback>& cb);

    Return<void> getCaptureResultMetadataQueue(
            V3_3::ICameraDeviceSession::getCaptureResultMetadataQueue_cb _hidl_cb);

    Return<void> close();

    void initOutputThread();

    void invokeProcessCaptureResultCallback(
            hidl_vec<CaptureResult> &results, bool tryWriteFmq);

    status_t fillCaptureResult(common::V1_0::helper::CameraMetadata& md, nsecs_t timestamp);

    void cleanupBuffersLocked(int id);

    // Protect (most of) HIDL interface methods from synchronized-entering
    mutable Mutex mInterfaceLock;

    mutable Mutex mLock; // Protect all data members except otherwise noted

    bool mClosed = false;
    const CroppingType mCroppingType;
    const common::V1_0::helper::CameraMetadata mChars;
    const std::string mCameraId;
    const std::string mExifMake;
    const std::string mExifModel;
    const uint32_t mBlobBufferSize;

    std::mutex mAfTriggerLock; // protect mAfTrigger
    bool mAfTrigger;

    const hidl_vec<Stream> mOfflineStreams;
    std::deque<std::shared_ptr<HalRequest>> mOfflineReqs;

    // Protect mCirculatingBuffers, must not lock mLock after acquiring this lock
    mutable Mutex mCbsLock;
    std::map<int, CirculatingBuffers> mCirculatingBuffers;

    static HandleImporter sHandleImporter;

    using ResultMetadataQueue = MessageQueue<uint8_t, kSynchronizedReadWrite>;
    std::shared_ptr<ResultMetadataQueue> mResultMetadataQueue;

    // Protect against invokeProcessCaptureResultCallback()
    Mutex mProcessCaptureResultLock;

    sp<ICameraDeviceCallback> mCallback;

    sp<V3_5::implementation::ExternalCameraDeviceSession::BufferRequestThread> mBufferRequestThread;
    sp<OutputThread> mOutputThread;
private:

    struct TrampolineSessionInterface_3_6 : public ICameraOfflineSession {
        TrampolineSessionInterface_3_6(sp<ExternalCameraOfflineSession> parent) :
                mParent(parent) {}

        virtual Return<void> setCallback(const sp<ICameraDeviceCallback>& cb) override {
            return mParent->setCallback(cb);
        }

        virtual Return<void> getCaptureResultMetadataQueue(
                V3_3::ICameraDeviceSession::getCaptureResultMetadataQueue_cb _hidl_cb) override {
            return mParent->getCaptureResultMetadataQueue(_hidl_cb);
        }

        virtual Return<void> close() override {
            return mParent->close();
        }

    private:
        sp<ExternalCameraOfflineSession> mParent;
    };
};

}  // namespace implementation
}  // namespace V3_6
}  // namespace device
}  // namespace camera
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_CAMERA_DEVICE_V3_6_EXTCAMERAOFFLINESESSION_H
