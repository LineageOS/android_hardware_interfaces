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

#ifndef ANDROID_HARDWARE_CAMERA_DEVICE_V3_5_EXTCAMERADEVICESESSION_H
#define ANDROID_HARDWARE_CAMERA_DEVICE_V3_5_EXTCAMERADEVICESESSION_H

#include <android/hardware/camera/device/3.5/ICameraDeviceCallback.h>
#include <android/hardware/camera/device/3.5/ICameraDeviceSession.h>
#include <../../3.4/default/include/ext_device_v3_4_impl/ExternalCameraDeviceSession.h>

namespace android {
namespace hardware {
namespace camera {
namespace device {
namespace V3_5 {
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
using ::android::hardware::camera::device::V3_5::ICameraDeviceSession;
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
using ::android::hardware::camera::device::V3_4::implementation::HalStreamBuffer;

struct ExternalCameraDeviceSession : public V3_4::implementation::ExternalCameraDeviceSession {

    ExternalCameraDeviceSession(const sp<V3_2::ICameraDeviceCallback>&,
            const ExternalCameraConfig& cfg,
            const std::vector<SupportedV4L2Format>& sortedFormats,
            const CroppingType& croppingType,
            const common::V1_0::helper::CameraMetadata& chars,
            const std::string& cameraId,
            unique_fd v4l2Fd);
    virtual ~ExternalCameraDeviceSession();

    // Retrieve the HIDL interface, split into its own class to avoid inheritance issues when
    // dealing with minor version revs and simultaneous implementation and interface inheritance
    virtual sp<V3_4::ICameraDeviceSession> getInterface() override {
        return new TrampolineSessionInterface_3_5(this);
    }

    static Status isStreamCombinationSupported(const V3_2::StreamConfiguration& config,
            const std::vector<SupportedV4L2Format>& supportedFormats,
            const ExternalCameraConfig& devCfg) {
        return V3_4::implementation::ExternalCameraDeviceSession::isStreamCombinationSupported(
                config, supportedFormats, devCfg);
    }

    class BufferRequestThread : public android::Thread {
    public:
        BufferRequestThread(
                wp<OutputThreadInterface> parent,
                sp<V3_5::ICameraDeviceCallback> callbacks);

        int requestBufferStart(const std::vector<HalStreamBuffer>&);
        int waitForBufferRequestDone(
                /*out*/std::vector<HalStreamBuffer>*);

        virtual bool threadLoop() override;

    private:
        void waitForNextRequest();

        const wp<OutputThreadInterface> mParent;
        const sp<V3_5::ICameraDeviceCallback> mCallbacks;

        std::mutex mLock;
        bool mRequestingBuffer = false;

        std::vector<HalStreamBuffer> mBufferReqs;
        std::vector<HalStreamBuffer> mPendingReturnBufferReqs;
        // mHalBufferReqs is not under mLock protection during the HIDL transaction
        hidl_vec<BufferRequest>      mHalBufferReqs;

        // request buffers takes much less time in steady state, but can take much longer
        // when requesting 1st buffer from a stream.
        // TODO: consider a separate timeout for new vs. steady state?
        // TODO: or make sure framework is warming up the pipeline during configure new stream?
        static const int kReqProcTimeoutMs = 66;

        static const int kReqWaitTimeoutMs = 33;
        static const int kReqWaitTimesWarn = 90;  // 33ms * 90 ~= 3 sec
        std::condition_variable mRequestCond;     // signaled when a new buffer request incoming
        std::condition_variable mRequestDoneCond; // signaled when a request is done
    };

    class OutputThread :
            public V3_4::implementation::ExternalCameraDeviceSession::OutputThread {
    public:
        // TODO: pass buffer request thread to OutputThread ctor
        OutputThread(wp<OutputThreadInterface> parent, CroppingType,
                const common::V1_0::helper::CameraMetadata&,
                sp<BufferRequestThread> bufReqThread);
        virtual ~OutputThread();

    protected:
        // Methods to request output buffer in parallel
        virtual int requestBufferStart(const std::vector<HalStreamBuffer>&) override;
        virtual int waitForBufferRequestDone(
                /*out*/std::vector<HalStreamBuffer>*) override;

        const sp<BufferRequestThread> mBufferRequestThread;
    };

protected:
    // Methods from v3.4 and earlier will trampoline to inherited implementation
    Return<void> configureStreams_3_5(
            const StreamConfiguration& requestedConfiguration,
            ICameraDeviceSession::configureStreams_3_5_cb _hidl_cb);

    Return<void> signalStreamFlush(
            const hidl_vec<int32_t>& requests,
            uint32_t streamConfigCounter);

    Return<void> isReconfigurationRequired(const V3_2::CameraMetadata& oldSessionParams,
            const V3_2::CameraMetadata& newSessionParams,
            ICameraDeviceSession::isReconfigurationRequired_cb _hidl_cb);

    virtual void initOutputThread() override;
    virtual void closeOutputThread() override;
    void closeOutputThreadImpl();

    virtual Status importRequestLocked(
            const CaptureRequest& request,
            hidl_vec<buffer_handle_t*>& allBufPtrs,
            hidl_vec<int>& allFences) override;

    sp<BufferRequestThread> mBufferRequestThread;

    sp<V3_5::ICameraDeviceCallback> mCallback_3_5;
    bool mSupportBufMgr;

private:

    struct TrampolineSessionInterface_3_5 : public ICameraDeviceSession {
        TrampolineSessionInterface_3_5(sp<ExternalCameraDeviceSession> parent) :
                mParent(parent) {}

        virtual Return<void> constructDefaultRequestSettings(
                RequestTemplate type,
                V3_3::ICameraDeviceSession::constructDefaultRequestSettings_cb _hidl_cb) override {
            return mParent->constructDefaultRequestSettings(type, _hidl_cb);
        }

        virtual Return<void> configureStreams(
                const V3_2::StreamConfiguration& requestedConfiguration,
                V3_3::ICameraDeviceSession::configureStreams_cb _hidl_cb) override {
            return mParent->configureStreams(requestedConfiguration, _hidl_cb);
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

        virtual Return<void> processCaptureRequest_3_4(const hidl_vec<V3_4::CaptureRequest>& requests,
                const hidl_vec<V3_2::BufferCache>& cachesToRemove,
                ICameraDeviceSession::processCaptureRequest_3_4_cb _hidl_cb) override {
            return mParent->processCaptureRequest_3_4(requests, cachesToRemove, _hidl_cb);
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
        sp<ExternalCameraDeviceSession> mParent;
    };
};

}  // namespace implementation
}  // namespace V3_5
}  // namespace device
}  // namespace camera
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_CAMERA_DEVICE_V3_5_EXTCAMERADEVICESESSION_H
