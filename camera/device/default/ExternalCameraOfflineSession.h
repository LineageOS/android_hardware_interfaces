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

#ifndef HARDWARE_INTERFACES_CAMERA_DEVICE_DEFAULT_EXTERNALCAMERAOFFLINESESSION_H_
#define HARDWARE_INTERFACES_CAMERA_DEVICE_DEFAULT_EXTERNALCAMERAOFFLINESESSION_H_

#include <ExternalCameraDeviceSession.h>
#include <ExternalCameraUtils.h>
#include <aidl/android/hardware/camera/common/Status.h>
#include <aidl/android/hardware/camera/device/BnCameraOfflineSession.h>
#include <aidl/android/hardware/camera/device/Stream.h>
#include <fmq/AidlMessageQueue.h>
#include <utils/RefBase.h>
#include <deque>

namespace android {
namespace hardware {
namespace camera {
namespace device {
namespace implementation {

using ::aidl::android::hardware::camera::common::Status;
using ::aidl::android::hardware::camera::device::BnCameraOfflineSession;
using ::aidl::android::hardware::camera::device::ICameraDeviceCallback;
using ::aidl::android::hardware::camera::device::Stream;
using ::aidl::android::hardware::common::fmq::MQDescriptor;
using ::aidl::android::hardware::common::fmq::SynchronizedReadWrite;

class ExternalCameraOfflineSession final : public BnCameraOfflineSession,
                                           public virtual RefBase,
                                           public virtual OutputThreadInterface {
  public:
    ExternalCameraOfflineSession(const CroppingType& croppingType,
                                 const common::V1_0::helper::CameraMetadata& chars,
                                 const std::string& cameraId, const std::string& exifMake,
                                 const std::string& exifModel, uint32_t blobBufferSize,
                                 bool afTrigger, const std::vector<Stream>& offlineStreams,
                                 std::deque<std::shared_ptr<HalRequest>>& offlineReqs,
                                 const std::map<int, CirculatingBuffers>& circulatingBuffers);

    ~ExternalCameraOfflineSession() override;

    bool initialize();

    // Methods from OutputThreadInterface
    Status importBuffer(int32_t streamId, uint64_t bufId, buffer_handle_t buf,
                        /*out*/ buffer_handle_t** outBufPtr) override;

    Status processCaptureResult(std::shared_ptr<HalRequest>&) override;

    Status processCaptureRequestError(const std::shared_ptr<HalRequest>&,
                                      /*out*/ std::vector<NotifyMsg>* msgs,
                                      /*out*/ std::vector<CaptureResult>* results) override;

    ssize_t getJpegBufferSize(int32_t width, int32_t height) const override;

    void notifyError(int32_t frameNumber, int32_t streamId, ErrorCode ec) override;
    // End of OutputThreadInterface methods

    ScopedAStatus setCallback(const std::shared_ptr<ICameraDeviceCallback>& in_cb) override;
    ScopedAStatus getCaptureResultMetadataQueue(
            MQDescriptor<int8_t, SynchronizedReadWrite>* _aidl_return) override;
    ScopedAStatus close() override;

  private:
    class OutputThread : public ExternalCameraDeviceSession::OutputThread {
      public:
        OutputThread(std::weak_ptr<OutputThreadInterface> parent, CroppingType ct,
                     const common::V1_0::helper::CameraMetadata& chars,
                     std::shared_ptr<ExternalCameraDeviceSession::BufferRequestThread> bufReqThread,
                     std::deque<std::shared_ptr<HalRequest>>& offlineReqs)
            : ExternalCameraDeviceSession::OutputThread(std::move(parent), ct, chars,
                                                        std::move(bufReqThread)),
              mOfflineReqs(offlineReqs) {}

        bool threadLoop() override;

      protected:
        std::deque<std::shared_ptr<HalRequest>> mOfflineReqs;
    };  // OutputThread

    status_t fillCaptureResult(common::V1_0::helper::CameraMetadata md, nsecs_t timestamp);
    void invokeProcessCaptureResultCallback(std::vector<CaptureResult>& results, bool tryWriteFmq);
    void initOutputThread();
    void cleanupBuffersLocked(int32_t id);

    // Protect (most of) HIDL interface methods from synchronized-entering
    mutable Mutex mInterfaceLock;

    mutable Mutex mLock;  // Protect all data members except otherwise noted

    bool mClosed = false;
    const CroppingType mCroppingType;
    const common::V1_0::helper::CameraMetadata mChars;
    const std::string mCameraId;
    const std::string mExifMake;
    const std::string mExifModel;
    const uint32_t mBlobBufferSize;

    std::mutex mAfTriggerLock;  // protect mAfTrigger
    bool mAfTrigger;

    const std::vector<Stream> mOfflineStreams;
    std::deque<std::shared_ptr<HalRequest>> mOfflineReqs;

    // Protect mCirculatingBuffers, must not lock mLock after acquiring this lock
    mutable Mutex mCbsLock;
    std::map<int, CirculatingBuffers> mCirculatingBuffers;

    static HandleImporter sHandleImporter;

    using ResultMetadataQueue = AidlMessageQueue<int8_t, SynchronizedReadWrite>;
    std::shared_ptr<ResultMetadataQueue> mResultMetadataQueue;

    // Protect against invokeProcessCaptureResultCallback()
    Mutex mProcessCaptureResultLock;

    std::shared_ptr<ICameraDeviceCallback> mCallback;

    std::shared_ptr<ExternalCameraDeviceSession::BufferRequestThread> mBufferRequestThread;
    std::shared_ptr<OutputThread> mOutputThread;
};

}  // namespace implementation
}  // namespace device
}  // namespace camera
}  // namespace hardware
}  // namespace android

#endif  // HARDWARE_INTERFACES_CAMERA_DEVICE_DEFAULT_EXTERNALCAMERAOFFLINESESSION_H_
