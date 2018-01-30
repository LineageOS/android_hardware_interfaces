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

#ifndef ANDROID_HARDWARE_CAMERA_DEVICE_V3_4_EXTCAMERADEVICE3SESSION_H
#define ANDROID_HARDWARE_CAMERA_DEVICE_V3_4_EXTCAMERADEVICE3SESSION_H

#include <android/hardware/camera/device/3.2/ICameraDevice.h>
#include <android/hardware/camera/device/3.4/ICameraDeviceSession.h>
#include <fmq/MessageQueue.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>
#include <include/convert.h>
#include <chrono>
#include <condition_variable>
#include <list>
#include <unordered_map>
#include <unordered_set>
#include "CameraMetadata.h"
#include "HandleImporter.h"
#include "Exif.h"
#include "utils/KeyedVector.h"
#include "utils/Mutex.h"
#include "utils/Thread.h"
#include "android-base/unique_fd.h"

namespace android {
namespace hardware {
namespace camera {
namespace device {
namespace V3_4 {
namespace implementation {

using ::android::hardware::camera::device::V3_2::BufferCache;
using ::android::hardware::camera::device::V3_2::BufferStatus;
using ::android::hardware::camera::device::V3_2::CameraMetadata;
using ::android::hardware::camera::device::V3_2::CaptureRequest;
using ::android::hardware::camera::device::V3_2::CaptureResult;
using ::android::hardware::camera::device::V3_2::ErrorCode;
using ::android::hardware::camera::device::V3_2::ICameraDeviceCallback;
using ::android::hardware::camera::device::V3_2::MsgType;
using ::android::hardware::camera::device::V3_2::NotifyMsg;
using ::android::hardware::camera::device::V3_4::RequestTemplate;
using ::android::hardware::camera::device::V3_2::Stream;
using ::android::hardware::camera::device::V3_4::StreamConfiguration;
using ::android::hardware::camera::device::V3_2::StreamConfigurationMode;
using ::android::hardware::camera::device::V3_2::StreamRotation;
using ::android::hardware::camera::device::V3_2::StreamType;
using ::android::hardware::camera::device::V3_2::DataspaceFlags;
using ::android::hardware::camera::device::V3_2::CameraBlob;
using ::android::hardware::camera::device::V3_2::CameraBlobId;
using ::android::hardware::camera::device::V3_4::HalStreamConfiguration;
using ::android::hardware::camera::device::V3_4::ICameraDeviceSession;
using ::android::hardware::camera::common::V1_0::Status;
using ::android::hardware::camera::common::V1_0::helper::HandleImporter;
using ::android::hardware::camera::common::V1_0::helper::ExifUtils;
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

// TODO: put V4L2 related structs into separate header?
struct SupportedV4L2Format {
    uint32_t width;
    uint32_t height;
    uint32_t fourcc;
    // All supported frame rate for this w/h/fourcc combination
    std::vector<float> frameRates;
};

// A class provide access to a dequeued V4L2 frame buffer (mostly in MJPG format)
// Also contains necessary information to enqueue the buffer back to V4L2 buffer queue
class V4L2Frame : public virtual VirtualLightRefBase {
public:
    V4L2Frame(uint32_t w, uint32_t h, uint32_t fourcc, int bufIdx, int fd, uint32_t dataSize);
    ~V4L2Frame() override;
    const uint32_t mWidth;
    const uint32_t mHeight;
    const uint32_t mFourcc;
    const int mBufferIndex; // for later enqueue
    int map(uint8_t** data, size_t* dataSize);
    int unmap();
private:
    Mutex mLock;
    const int mFd; // used for mmap but doesn't claim ownership
    const size_t mDataSize;
    uint8_t* mData = nullptr;
    bool  mMapped = false;
};

// A RAII class representing a CPU allocated YUV frame used as intermeidate buffers
// when generating output images.
class AllocatedFrame : public virtual VirtualLightRefBase {
public:
    AllocatedFrame(uint32_t w, uint32_t h); // TODO: use Size?
    ~AllocatedFrame() override;
    const uint32_t mWidth;
    const uint32_t mHeight;
    const uint32_t mFourcc; // Only support YU12 format for now
    int allocate(YCbCrLayout* out = nullptr);
    int getLayout(YCbCrLayout* out);
    int getCroppedLayout(const IMapper::Rect&, YCbCrLayout* out); // return non-zero for bad input
private:
    Mutex mLock;
    std::vector<uint8_t> mData;
};

struct Size {
    uint32_t width;
    uint32_t height;

    bool operator==(const Size& other) const {
        return (width == other.width && height == other.height);
    }
};

struct SizeHasher {
    size_t operator()(const Size& sz) const {
        size_t result = 1;
        result = 31 * result + sz.width;
        result = 31 * result + sz.height;
        return result;
    }
};

enum CroppingType {
    HORIZONTAL = 0,
    VERTICAL = 1
};

struct ExternalCameraDeviceSession : public virtual RefBase {

    ExternalCameraDeviceSession(const sp<ICameraDeviceCallback>&,
            const std::vector<SupportedV4L2Format>& supportedFormats,
            const common::V1_0::helper::CameraMetadata& chars,
            unique_fd v4l2Fd);
    virtual ~ExternalCameraDeviceSession();
    // Call by CameraDevice to dump active device states
    void dumpState(const native_handle_t*);
    // Caller must use this method to check if CameraDeviceSession ctor failed
    bool isInitFailed() { return mInitFail; }
    bool isClosed();

    // Retrieve the HIDL interface, split into its own class to avoid inheritance issues when
    // dealing with minor version revs and simultaneous implementation and interface inheritance
    virtual sp<ICameraDeviceSession> getInterface() {
        return new TrampolineSessionInterface_3_4(this);
    }

    static const int kMaxProcessedStream = 2;
    static const int kMaxStallStream = 1;

protected:

    // Methods from ::android::hardware::camera::device::V3_2::ICameraDeviceSession follow

    Return<void> constructDefaultRequestSettings(
            V3_2::RequestTemplate,
            ICameraDeviceSession::constructDefaultRequestSettings_cb _hidl_cb);

    Return<void> constructDefaultRequestSettings_3_4(
            RequestTemplate type,
            ICameraDeviceSession::constructDefaultRequestSettings_cb _hidl_cb);

    Return<void> configureStreams(
            const V3_2::StreamConfiguration&,
            ICameraDeviceSession::configureStreams_cb);

    Return<void> getCaptureRequestMetadataQueue(
        ICameraDeviceSession::getCaptureRequestMetadataQueue_cb);

    Return<void> getCaptureResultMetadataQueue(
        ICameraDeviceSession::getCaptureResultMetadataQueue_cb);

    Return<void> processCaptureRequest(
            const hidl_vec<CaptureRequest>&,
            const hidl_vec<BufferCache>&,
            ICameraDeviceSession::processCaptureRequest_cb);

    Return<Status> flush();
    Return<void> close();

    Return<void> configureStreams_3_3(
            const V3_2::StreamConfiguration&,
            ICameraDeviceSession::configureStreams_3_3_cb);

    Return<void> configureStreams_3_4(
            const V3_4::StreamConfiguration& requestedConfiguration,
            ICameraDeviceSession::configureStreams_3_4_cb _hidl_cb);

    Return<void> processCaptureRequest_3_4(
            const hidl_vec<V3_4::CaptureRequest>& requests,
            const hidl_vec<V3_2::BufferCache>& cachesToRemove,
            ICameraDeviceSession::processCaptureRequest_3_4_cb _hidl_cb);

protected:
    struct HalStreamBuffer {
        int32_t streamId;
        uint64_t bufferId;
        uint32_t width;
        uint32_t height;
        PixelFormat format;
        V3_2::BufferUsageFlags usage;
        buffer_handle_t* bufPtr;
        int acquireFence;
        bool fenceTimeout;
    };

    struct HalRequest {
        uint32_t frameNumber;
        common::V1_0::helper::CameraMetadata setting;
        sp<V4L2Frame> frameIn;
        nsecs_t shutterTs;
        std::vector<HalStreamBuffer> buffers;
    };

    Status constructDefaultRequestSettingsRaw(RequestTemplate type,
            V3_2::CameraMetadata *outMetadata);

    static std::vector<SupportedV4L2Format> sortFormats(
            const std::vector<SupportedV4L2Format>&);
    static CroppingType initCroppingType(const std::vector<SupportedV4L2Format>&);
    bool initialize();
    Status initStatus() const;
    status_t initDefaultRequests();
    status_t fillCaptureResult(common::V1_0::helper::CameraMetadata& md, nsecs_t timestamp);
    Status configureStreams(const V3_2::StreamConfiguration&, V3_3::HalStreamConfiguration* out);
    int configureV4l2StreamLocked(const SupportedV4L2Format& fmt);
    int v4l2StreamOffLocked();

    // TODO: change to unique_ptr for better tracking
    sp<V4L2Frame> dequeueV4l2FrameLocked(); // Called with mLock hold
    void enqueueV4l2Frame(const sp<V4L2Frame>&);

    // Check if input Stream is one of supported stream setting on this device
    bool isSupported(const Stream&);

    // Validate and import request's output buffers and acquire fence
    Status importRequest(
            const CaptureRequest& request,
            hidl_vec<buffer_handle_t*>& allBufPtrs,
            hidl_vec<int>& allFences);
    static void cleanupInflightFences(
            hidl_vec<int>& allFences, size_t numFences);
    void cleanupBuffersLocked(int id);
    void updateBufferCaches(const hidl_vec<BufferCache>& cachesToRemove);

    Status processOneCaptureRequest(const CaptureRequest& request);

    Status processCaptureResult(HalRequest&);
    Status processCaptureRequestError(HalRequest&);
    void notifyShutter(uint32_t frameNumber, nsecs_t shutterTs);
    void notifyError(uint32_t frameNumber, int32_t streamId, ErrorCode ec);
    void invokeProcessCaptureResultCallback(
            hidl_vec<CaptureResult> &results, bool tryWriteFmq);
    static void freeReleaseFences(hidl_vec<CaptureResult>&);

    Size getMaxJpegResolution() const;
    Size getMaxThumbResolution() const;

    ssize_t getJpegBufferSize(uint32_t width, uint32_t height) const;

    class OutputThread : public android::Thread {
    public:
        OutputThread(wp<ExternalCameraDeviceSession> parent, CroppingType);
        ~OutputThread();

        Status allocateIntermediateBuffers(
                const Size& v4lSize, const Size& thumbSize,
                const hidl_vec<Stream>& streams);
        Status submitRequest(const HalRequest&);
        void flush();
        virtual bool threadLoop() override;

    private:
        static const uint32_t FLEX_YUV_GENERIC = static_cast<uint32_t>('F') |
                static_cast<uint32_t>('L') << 8 | static_cast<uint32_t>('E') << 16 |
                static_cast<uint32_t>('X') << 24;
        // returns FLEX_YUV_GENERIC for formats other than YV12/YU12/NV12/NV21
        static uint32_t getFourCcFromLayout(const YCbCrLayout&);
        static int getCropRect(
                CroppingType ct, const Size& inSize, const Size& outSize, IMapper::Rect* out);

        static const int kReqWaitTimeoutSec = 3;

        void waitForNextRequest(HalRequest* out);
        int cropAndScaleLocked(
                sp<AllocatedFrame>& in, const Size& outSize,
                YCbCrLayout* out);

        int cropAndScaleThumbLocked(
                sp<AllocatedFrame>& in, const Size& outSize,
                YCbCrLayout* out);

        int formatConvertLocked(const YCbCrLayout& in, const YCbCrLayout& out,
                Size sz, uint32_t format);

        static int encodeJpegYU12(const Size &inSz,
                const YCbCrLayout& inLayout, int jpegQuality,
                const void *app1Buffer, size_t app1Size,
                void *out, size_t maxOutSize,
                size_t &actualCodeSize);

        int createJpegLocked(HalStreamBuffer &halBuf, HalRequest &req);

        mutable std::mutex mLock;
        std::condition_variable mRequestCond;
        wp<ExternalCameraDeviceSession> mParent;
        CroppingType mCroppingType;
        std::list<HalRequest> mRequestList;
        // V4L2 frameIn
        // (MJPG decode)-> mYu12Frame
        // (Scale)-> mScaledYu12Frames
        // (Format convert) -> output gralloc frames
        sp<AllocatedFrame> mYu12Frame;
        sp<AllocatedFrame> mYu12ThumbFrame;
        std::unordered_map<Size, sp<AllocatedFrame>, SizeHasher> mIntermediateBuffers;
        std::unordered_map<Size, sp<AllocatedFrame>, SizeHasher> mScaledYu12Frames;
        YCbCrLayout mYu12FrameLayout;
        YCbCrLayout mYu12ThumbFrameLayout;
    };

    // Protect (most of) HIDL interface methods from synchronized-entering
    mutable Mutex mInterfaceLock;

    mutable Mutex mLock; // Protect all private members except otherwise noted
    const sp<ICameraDeviceCallback> mCallback;
    const common::V1_0::helper::CameraMetadata mCameraCharacteristics;
    unique_fd mV4l2Fd;
    // device is closed either
    //    - closed by user
    //    - init failed
    //    - camera disconnected
    bool mClosed = false;
    bool mInitFail = false;
    bool mFirstRequest = false;
    common::V1_0::helper::CameraMetadata mLatestReqSetting;

    bool mV4l2Streaming = false;
    SupportedV4L2Format mV4l2StreamingFmt;
    std::vector<unique_fd> mV4l2Buffers;

    static const int kBufferWaitTimeoutSec = 3; // TODO: handle long exposure (or not allowing)
    std::mutex mV4l2BufferLock; // protect the buffer count and condition below
    std::condition_variable mV4L2BufferReturned;
    size_t mNumDequeuedV4l2Buffers = 0;

    const std::vector<SupportedV4L2Format> mSupportedFormats;
    const CroppingType mCroppingType;
    sp<OutputThread> mOutputThread;

    // Stream ID -> Camera3Stream cache
    std::unordered_map<int, Stream> mStreamMap;
    std::unordered_set<uint32_t>  mInflightFrames;

    // buffers currently circulating between HAL and camera service
    // key: bufferId sent via HIDL interface
    // value: imported buffer_handle_t
    // Buffer will be imported during processCaptureRequest and will be freed
    // when the its stream is deleted or camera device session is closed
    typedef std::unordered_map<uint64_t, buffer_handle_t> CirculatingBuffers;
    // Stream ID -> circulating buffers map
    std::map<int, CirculatingBuffers> mCirculatingBuffers;

    bool mAfTrigger = false;

    static HandleImporter sHandleImporter;

    /* Beginning of members not changed after initialize() */
    using RequestMetadataQueue = MessageQueue<uint8_t, kSynchronizedReadWrite>;
    std::unique_ptr<RequestMetadataQueue> mRequestMetadataQueue;
    using ResultMetadataQueue = MessageQueue<uint8_t, kSynchronizedReadWrite>;
    std::shared_ptr<ResultMetadataQueue> mResultMetadataQueue;

    // Protect against invokeProcessCaptureResultCallback()
    Mutex mProcessCaptureResultLock;

    std::unordered_map<RequestTemplate, CameraMetadata> mDefaultRequests;

    const Size mMaxThumbResolution;
    const Size mMaxJpegResolution;
    /* End of members not changed after initialize() */

private:

    struct TrampolineSessionInterface_3_4 : public ICameraDeviceSession {
        TrampolineSessionInterface_3_4(sp<ExternalCameraDeviceSession> parent) :
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

        virtual Return<void> constructDefaultRequestSettings_3_4(
                RequestTemplate type,
                ICameraDeviceSession::constructDefaultRequestSettings_cb _hidl_cb) override {
            return mParent->constructDefaultRequestSettings_3_4(type, _hidl_cb);
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

    private:
        sp<ExternalCameraDeviceSession> mParent;
    };
};

}  // namespace implementation
}  // namespace V3_4
}  // namespace device
}  // namespace camera
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_CAMERA_DEVICE_V3_4_EXTCAMERADEVICE3SESSION_H
