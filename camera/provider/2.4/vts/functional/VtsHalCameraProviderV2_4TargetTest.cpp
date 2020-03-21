/*
 * Copyright (C) 2016-2018 The Android Open Source Project
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

#define LOG_TAG "camera_hidl_hal_test"

#include <algorithm>
#include <chrono>
#include <mutex>
#include <regex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <condition_variable>

#include <inttypes.h>

#include <CameraMetadata.h>
#include <CameraParameters.h>
#include <android/hardware/camera/device/1.0/ICameraDevice.h>
#include <android/hardware/camera/device/3.2/ICameraDevice.h>
#include <android/hardware/camera/device/3.3/ICameraDeviceSession.h>
#include <android/hardware/camera/device/3.4/ICameraDeviceCallback.h>
#include <android/hardware/camera/device/3.4/ICameraDeviceSession.h>
#include <android/hardware/camera/device/3.5/ICameraDevice.h>
#include <android/hardware/camera/device/3.5/ICameraDeviceCallback.h>
#include <android/hardware/camera/device/3.5/ICameraDeviceSession.h>
#include <android/hardware/camera/device/3.6/ICameraDevice.h>
#include <android/hardware/camera/device/3.6/ICameraDeviceSession.h>
#include <android/hardware/camera/metadata/3.4/types.h>
#include <android/hardware/camera/provider/2.4/ICameraProvider.h>
#include <android/hardware/camera/provider/2.5/ICameraProvider.h>
#include <android/hardware/camera/provider/2.6/ICameraProvider.h>
#include <android/hardware/camera/provider/2.6/ICameraProviderCallback.h>
#include <android/hidl/manager/1.0/IServiceManager.h>
#include <binder/MemoryHeapBase.h>
#include <cutils/properties.h>
#include <fmq/MessageQueue.h>
#include <grallocusage/GrallocUsageConversion.h>
#include <gtest/gtest.h>
#include <gui/BufferItemConsumer.h>
#include <gui/BufferQueue.h>
#include <gui/Surface.h>
#include <hardware/gralloc.h>
#include <hardware/gralloc1.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>
#include <log/log.h>
#include <system/camera.h>
#include <system/camera_metadata.h>
#include <ui/GraphicBuffer.h>
#include <ui/GraphicBufferAllocator.h>
#include <ui/GraphicBufferMapper.h>

#include <android/hidl/allocator/1.0/IAllocator.h>
#include <android/hidl/memory/1.0/IMapper.h>
#include <android/hidl/memory/1.0/IMemory.h>

using namespace ::android::hardware::camera::device;
using ::android::BufferItemConsumer;
using ::android::BufferQueue;
using ::android::GraphicBuffer;
using ::android::IGraphicBufferConsumer;
using ::android::IGraphicBufferProducer;
using ::android::sp;
using ::android::Surface;
using ::android::wp;
using ::android::hardware::hidl_bitfield;
using ::android::hardware::hidl_handle;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::kSynchronizedReadWrite;
using ::android::hardware::MessageQueue;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::camera::common::V1_0::CameraDeviceStatus;
using ::android::hardware::camera::common::V1_0::Status;
using ::android::hardware::camera::common::V1_0::TorchMode;
using ::android::hardware::camera::common::V1_0::TorchModeStatus;
using ::android::hardware::camera::common::V1_0::helper::CameraParameters;
using ::android::hardware::camera::common::V1_0::helper::Size;
using ::android::hardware::camera::device::V1_0::CameraFacing;
using ::android::hardware::camera::device::V1_0::CameraFrameMetadata;
using ::android::hardware::camera::device::V1_0::CommandType;
using ::android::hardware::camera::device::V1_0::DataCallbackMsg;
using ::android::hardware::camera::device::V1_0::FrameCallbackFlag;
using ::android::hardware::camera::device::V1_0::HandleTimestampMessage;
using ::android::hardware::camera::device::V1_0::ICameraDevicePreviewCallback;
using ::android::hardware::camera::device::V1_0::NotifyCallbackMsg;
using ::android::hardware::camera::device::V3_2::BufferCache;
using ::android::hardware::camera::device::V3_2::BufferStatus;
using ::android::hardware::camera::device::V3_2::CameraMetadata;
using ::android::hardware::camera::device::V3_2::CaptureRequest;
using ::android::hardware::camera::device::V3_2::CaptureResult;
using ::android::hardware::camera::device::V3_2::ErrorCode;
using ::android::hardware::camera::device::V3_2::ErrorMsg;
using ::android::hardware::camera::device::V3_2::HalStreamConfiguration;
using ::android::hardware::camera::device::V3_2::ICameraDevice;
using ::android::hardware::camera::device::V3_2::ICameraDeviceSession;
using ::android::hardware::camera::device::V3_2::MsgType;
using ::android::hardware::camera::device::V3_2::NotifyMsg;
using ::android::hardware::camera::device::V3_2::RequestTemplate;
using ::android::hardware::camera::device::V3_2::StreamBuffer;
using ::android::hardware::camera::device::V3_2::StreamConfiguration;
using ::android::hardware::camera::device::V3_2::StreamConfigurationMode;
using ::android::hardware::camera::device::V3_2::StreamRotation;
using ::android::hardware::camera::device::V3_2::StreamType;
using ::android::hardware::camera::device::V3_4::PhysicalCameraMetadata;
using ::android::hardware::camera::metadata::V3_4::
        CameraMetadataEnumAndroidSensorInfoColorFilterArrangement;
using ::android::hardware::camera::metadata::V3_4::CameraMetadataTag;
using ::android::hardware::camera::provider::V2_4::ICameraProvider;
using ::android::hardware::camera::provider::V2_4::ICameraProviderCallback;
using ::android::hardware::camera::provider::V2_6::CameraIdAndStreamCombination;
using ::android::hardware::graphics::common::V1_0::BufferUsage;
using ::android::hardware::graphics::common::V1_0::Dataspace;
using ::android::hardware::graphics::common::V1_0::PixelFormat;
using ::android::hidl::allocator::V1_0::IAllocator;
using ::android::hidl::memory::V1_0::IMapper;
using ::android::hidl::memory::V1_0::IMemory;
using ResultMetadataQueue = MessageQueue<uint8_t, kSynchronizedReadWrite>;
using ::android::hidl::manager::V1_0::IServiceManager;

using namespace ::android::hardware::camera;

const uint32_t kMaxPreviewWidth = 1920;
const uint32_t kMaxPreviewHeight = 1080;
const uint32_t kMaxStillWidth = 2048;
const uint32_t kMaxStillHeight = 1536;
const uint32_t kMaxVideoWidth = 4096;
const uint32_t kMaxVideoHeight = 2160;
const int64_t kStreamBufferTimeoutSec = 3;
const int64_t kAutoFocusTimeoutSec = 5;
const int64_t kTorchTimeoutSec = 1;
const int64_t kEmptyFlushTimeoutMSec = 200;
const char kDumpOutput[] = "/dev/null";
const uint32_t kBurstFrameCount = 10;
const int64_t kBufferReturnTimeoutSec = 1;

struct AvailableStream {
    int32_t width;
    int32_t height;
    int32_t format;
};

struct AvailableZSLInputOutput {
    int32_t inputFormat;
    int32_t outputFormat;
};

enum ReprocessType {
    PRIV_REPROCESS,
    YUV_REPROCESS,
};

namespace {
    // "device@<version>/legacy/<id>"
    const char *kDeviceNameRE = "device@([0-9]+\\.[0-9]+)/%s/(.+)";
    const int CAMERA_DEVICE_API_VERSION_3_6 = 0x306;
    const int CAMERA_DEVICE_API_VERSION_3_5 = 0x305;
    const int CAMERA_DEVICE_API_VERSION_3_4 = 0x304;
    const int CAMERA_DEVICE_API_VERSION_3_3 = 0x303;
    const int CAMERA_DEVICE_API_VERSION_3_2 = 0x302;
    const int CAMERA_DEVICE_API_VERSION_1_0 = 0x100;
    const char *kHAL3_6 = "3.6";
    const char *kHAL3_5 = "3.5";
    const char *kHAL3_4 = "3.4";
    const char *kHAL3_3 = "3.3";
    const char *kHAL3_2 = "3.2";
    const char *kHAL1_0 = "1.0";

    bool matchDeviceName(const hidl_string& deviceName,
            const hidl_string &providerType,
            std::string* deviceVersion,
            std::string* cameraId) {
        ::android::String8 pattern;
        pattern.appendFormat(kDeviceNameRE, providerType.c_str());
        std::regex e(pattern.string());
        std::string deviceNameStd(deviceName.c_str());
        std::smatch sm;
        if (std::regex_match(deviceNameStd, sm, e)) {
            if (deviceVersion != nullptr) {
                *deviceVersion = sm[1];
            }
            if (cameraId != nullptr) {
                *cameraId = sm[2];
            }
            return true;
        }
        return false;
    }

    int getCameraDeviceVersion(const hidl_string& deviceName,
            const hidl_string &providerType) {
        std::string version;
        bool match = matchDeviceName(deviceName, providerType, &version, nullptr);
        if (!match) {
            return -1;
        }

        if (version.compare(kHAL3_6) == 0) {
            return CAMERA_DEVICE_API_VERSION_3_6;
        } else if (version.compare(kHAL3_5) == 0) {
            return CAMERA_DEVICE_API_VERSION_3_5;
        } else if (version.compare(kHAL3_4) == 0) {
            return CAMERA_DEVICE_API_VERSION_3_4;
        } else if (version.compare(kHAL3_3) == 0) {
            return CAMERA_DEVICE_API_VERSION_3_3;
        } else if (version.compare(kHAL3_2) == 0) {
            return CAMERA_DEVICE_API_VERSION_3_2;
        } else if (version.compare(kHAL1_0) == 0) {
            return CAMERA_DEVICE_API_VERSION_1_0;
        }
        return 0;
    }

    bool parseProviderName(const std::string& name, std::string *type /*out*/,
            uint32_t *id /*out*/) {
        if (!type || !id) {
            ADD_FAILURE();
            return false;
        }

        std::string::size_type slashIdx = name.find('/');
        if (slashIdx == std::string::npos || slashIdx == name.size() - 1) {
            ADD_FAILURE() << "Provider name does not have / separator between type"
                    "and id";
            return false;
        }

        std::string typeVal = name.substr(0, slashIdx);

        char *endPtr;
        errno = 0;
        long idVal = strtol(name.c_str() + slashIdx + 1, &endPtr, 10);
        if (errno != 0) {
            ADD_FAILURE() << "cannot parse provider id as an integer:" <<
                    name.c_str() << strerror(errno) << errno;
            return false;
        }
        if (endPtr != name.c_str() + name.size()) {
            ADD_FAILURE() << "provider id has unexpected length " << name.c_str();
            return false;
        }
        if (idVal < 0) {
            ADD_FAILURE() << "id is negative: " << name.c_str() << idVal;
            return false;
        }

        *type = typeVal;
        *id = static_cast<uint32_t>(idVal);

        return true;
    }

    Status mapToStatus(::android::status_t s)  {
        switch(s) {
            case ::android::OK:
                return Status::OK ;
            case ::android::BAD_VALUE:
                return Status::ILLEGAL_ARGUMENT ;
            case -EBUSY:
                return Status::CAMERA_IN_USE;
            case -EUSERS:
                return Status::MAX_CAMERAS_IN_USE;
            case ::android::UNKNOWN_TRANSACTION:
                return Status::METHOD_NOT_SUPPORTED;
            case ::android::INVALID_OPERATION:
                return Status::OPERATION_NOT_SUPPORTED;
            case ::android::DEAD_OBJECT:
                return Status::CAMERA_DISCONNECTED;
        }
        ALOGW("Unexpected HAL status code %d", s);
        return Status::OPERATION_NOT_SUPPORTED;
    }

    void getFirstApiLevel(/*out*/int32_t* outApiLevel) {
        int32_t firstApiLevel = property_get_int32("ro.product.first_api_level", /*default*/-1);
        if (firstApiLevel < 0) {
            firstApiLevel = property_get_int32("ro.build.version.sdk", /*default*/-1);
        }
        ASSERT_GT(firstApiLevel, 0); // first_api_level must exist
        *outApiLevel = firstApiLevel;
        return;
    }
}

struct BufferItemHander: public BufferItemConsumer::FrameAvailableListener {
    BufferItemHander(wp<BufferItemConsumer> consumer) : mConsumer(consumer) {}

    void onFrameAvailable(const android::BufferItem&) override {
        sp<BufferItemConsumer> consumer = mConsumer.promote();
        ASSERT_NE(nullptr, consumer.get());

        android::BufferItem buffer;
        ASSERT_EQ(android::OK, consumer->acquireBuffer(&buffer, 0));
        ASSERT_EQ(android::OK, consumer->releaseBuffer(buffer));
    }

 private:
    wp<BufferItemConsumer> mConsumer;
};

struct PreviewWindowCb : public ICameraDevicePreviewCallback {
    PreviewWindowCb(sp<ANativeWindow> anw) : mPreviewWidth(0),
            mPreviewHeight(0), mFormat(0), mPreviewUsage(0),
            mPreviewSwapInterval(-1), mCrop{-1, -1, -1, -1}, mAnw(anw) {}

    using dequeueBuffer_cb =
            std::function<void(Status status, uint64_t bufferId,
                    const hidl_handle& buffer, uint32_t stride)>;
    Return<void> dequeueBuffer(dequeueBuffer_cb _hidl_cb) override;

    Return<Status> enqueueBuffer(uint64_t bufferId) override;

    Return<Status> cancelBuffer(uint64_t bufferId) override;

    Return<Status> setBufferCount(uint32_t count) override;

    Return<Status> setBuffersGeometry(uint32_t w,
            uint32_t h, PixelFormat format) override;

    Return<Status> setCrop(int32_t left, int32_t top,
            int32_t right, int32_t bottom) override;

    Return<Status> setUsage(BufferUsage usage) override;

    Return<Status> setSwapInterval(int32_t interval) override;

    using getMinUndequeuedBufferCount_cb =
            std::function<void(Status status, uint32_t count)>;
    Return<void> getMinUndequeuedBufferCount(
            getMinUndequeuedBufferCount_cb _hidl_cb) override;

    Return<Status> setTimestamp(int64_t timestamp) override;

 private:
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
        bool operator()(const buffer_handle_t& buf1,
                const buffer_handle_t& buf2) const {
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

    std::pair<bool, uint64_t> getBufferId(ANativeWindowBuffer* anb);
    void cleanupCirculatingBuffers();

    std::mutex mBufferIdMapLock; // protecting mBufferIdMap and mNextBufferId
    typedef std::unordered_map<const buffer_handle_t, uint64_t,
            BufferHasher, BufferComparator> BufferIdMap;

    BufferIdMap mBufferIdMap; // stream ID -> per stream buffer ID map
    std::unordered_map<uint64_t, ANativeWindowBuffer*> mReversedBufMap;
    uint64_t mNextBufferId = 1;

    uint32_t mPreviewWidth, mPreviewHeight;
    int mFormat, mPreviewUsage;
    int32_t mPreviewSwapInterval;
    android_native_rect_t mCrop;
    sp<ANativeWindow> mAnw;     //Native window reference
};

std::pair<bool, uint64_t> PreviewWindowCb::getBufferId(
        ANativeWindowBuffer* anb) {
    std::lock_guard<std::mutex> lock(mBufferIdMapLock);

    buffer_handle_t& buf = anb->handle;
    auto it = mBufferIdMap.find(buf);
    if (it == mBufferIdMap.end()) {
        uint64_t bufId = mNextBufferId++;
        mBufferIdMap[buf] = bufId;
        mReversedBufMap[bufId] = anb;
        return std::make_pair(true, bufId);
    } else {
        return std::make_pair(false, it->second);
    }
}

void PreviewWindowCb::cleanupCirculatingBuffers() {
    std::lock_guard<std::mutex> lock(mBufferIdMapLock);
    mBufferIdMap.clear();
    mReversedBufMap.clear();
}

Return<void> PreviewWindowCb::dequeueBuffer(dequeueBuffer_cb _hidl_cb) {
    ANativeWindowBuffer* anb;
    auto rc = native_window_dequeue_buffer_and_wait(mAnw.get(), &anb);
    uint64_t bufferId = 0;
    uint32_t stride = 0;
    hidl_handle buf = nullptr;
    if (rc == ::android::OK) {
        auto pair = getBufferId(anb);
        buf = (pair.first) ? anb->handle : nullptr;
        bufferId = pair.second;
        stride = anb->stride;
    }

    _hidl_cb(mapToStatus(rc), bufferId, buf, stride);
    return Void();
}

Return<Status> PreviewWindowCb::enqueueBuffer(uint64_t bufferId) {
    if (mReversedBufMap.count(bufferId) == 0) {
        ALOGE("%s: bufferId %" PRIu64 " not found", __FUNCTION__, bufferId);
        return Status::ILLEGAL_ARGUMENT;
    }
    return mapToStatus(mAnw->queueBuffer(mAnw.get(),
            mReversedBufMap.at(bufferId), -1));
}

Return<Status> PreviewWindowCb::cancelBuffer(uint64_t bufferId) {
    if (mReversedBufMap.count(bufferId) == 0) {
        ALOGE("%s: bufferId %" PRIu64 " not found", __FUNCTION__, bufferId);
        return Status::ILLEGAL_ARGUMENT;
    }
    return mapToStatus(mAnw->cancelBuffer(mAnw.get(),
            mReversedBufMap.at(bufferId), -1));
}

Return<Status> PreviewWindowCb::setBufferCount(uint32_t count) {
    if (mAnw.get() != nullptr) {
        // WAR for b/27039775
        native_window_api_disconnect(mAnw.get(), NATIVE_WINDOW_API_CAMERA);
        native_window_api_connect(mAnw.get(), NATIVE_WINDOW_API_CAMERA);
        if (mPreviewWidth != 0) {
            native_window_set_buffers_dimensions(mAnw.get(),
                    mPreviewWidth, mPreviewHeight);
            native_window_set_buffers_format(mAnw.get(), mFormat);
        }
        if (mPreviewUsage != 0) {
            native_window_set_usage(mAnw.get(), mPreviewUsage);
        }
        if (mPreviewSwapInterval >= 0) {
            mAnw->setSwapInterval(mAnw.get(), mPreviewSwapInterval);
        }
        if (mCrop.left >= 0) {
            native_window_set_crop(mAnw.get(), &(mCrop));
        }
    }

    auto rc = native_window_set_buffer_count(mAnw.get(), count);
    if (rc == ::android::OK) {
        cleanupCirculatingBuffers();
    }

    return mapToStatus(rc);
}

Return<Status> PreviewWindowCb::setBuffersGeometry(uint32_t w, uint32_t h,
        PixelFormat format) {
    auto rc = native_window_set_buffers_dimensions(mAnw.get(), w, h);
    if (rc == ::android::OK) {
        mPreviewWidth = w;
        mPreviewHeight = h;
        rc = native_window_set_buffers_format(mAnw.get(),
                static_cast<int>(format));
        if (rc == ::android::OK) {
            mFormat = static_cast<int>(format);
        }
    }

    return mapToStatus(rc);
}

Return<Status> PreviewWindowCb::setCrop(int32_t left, int32_t top,
        int32_t right, int32_t bottom) {
    android_native_rect_t crop = { left, top, right, bottom };
    auto rc = native_window_set_crop(mAnw.get(), &crop);
    if (rc == ::android::OK) {
        mCrop = crop;
    }
    return mapToStatus(rc);
}

Return<Status> PreviewWindowCb::setUsage(BufferUsage usage) {
    auto rc = native_window_set_usage(mAnw.get(), static_cast<int>(usage));
    if (rc == ::android::OK) {
        mPreviewUsage =  static_cast<int>(usage);
    }
    return mapToStatus(rc);
}

Return<Status> PreviewWindowCb::setSwapInterval(int32_t interval) {
    auto rc = mAnw->setSwapInterval(mAnw.get(), interval);
    if (rc == ::android::OK) {
        mPreviewSwapInterval = interval;
    }
    return mapToStatus(rc);
}

Return<void> PreviewWindowCb::getMinUndequeuedBufferCount(
        getMinUndequeuedBufferCount_cb _hidl_cb) {
    int count = 0;
    auto rc = mAnw->query(mAnw.get(),
            NATIVE_WINDOW_MIN_UNDEQUEUED_BUFFERS, &count);
    _hidl_cb(mapToStatus(rc), count);
    return Void();
}

Return<Status> PreviewWindowCb::setTimestamp(int64_t timestamp) {
    return mapToStatus(native_window_set_buffers_timestamp(mAnw.get(),
            timestamp));
}

// The main test class for camera HIDL HAL.
class CameraHidlTest : public ::testing::TestWithParam<std::string> {
public:
 virtual void SetUp() override {
     std::string service_name = GetParam();
     ALOGI("get service with name: %s", service_name.c_str());
     mProvider = ICameraProvider::getService(service_name);

     ASSERT_NE(mProvider, nullptr);

     uint32_t id;
     ASSERT_TRUE(parseProviderName(service_name, &mProviderType, &id));

     castProvider(mProvider, &mProvider2_5, &mProvider2_6);
     notifyDeviceState(provider::V2_5::DeviceState::NORMAL);
 }
 virtual void TearDown() override {}

 hidl_vec<hidl_string> getCameraDeviceNames(sp<ICameraProvider> provider);

 std::map<hidl_string, hidl_string> getCameraDeviceIdToNameMap(sp<ICameraProvider> provider);

 hidl_vec<hidl_vec<hidl_string>> getConcurrentDeviceCombinations(
         sp<::android::hardware::camera::provider::V2_6::ICameraProvider>&);

 struct EmptyDeviceCb : public V3_5::ICameraDeviceCallback {
     virtual Return<void> processCaptureResult(
         const hidl_vec<CaptureResult>& /*results*/) override {
         ALOGI("processCaptureResult callback");
         ADD_FAILURE();  // Empty callback should not reach here
         return Void();
     }

     virtual Return<void> processCaptureResult_3_4(
         const hidl_vec<V3_4::CaptureResult>& /*results*/) override {
         ALOGI("processCaptureResult_3_4 callback");
         ADD_FAILURE();  // Empty callback should not reach here
         return Void();
     }

     virtual Return<void> notify(const hidl_vec<NotifyMsg>& /*msgs*/) override {
         ALOGI("notify callback");
         ADD_FAILURE();  // Empty callback should not reach here
         return Void();
     }

     virtual Return<void> requestStreamBuffers(
             const hidl_vec<V3_5::BufferRequest>&,
             requestStreamBuffers_cb _hidl_cb) override {
         ALOGI("requestStreamBuffers callback");
         // HAL might want to request buffer after configureStreams, but tests with EmptyDeviceCb
         // doesn't actually need to send capture requests, so just return an error.
         hidl_vec<V3_5::StreamBufferRet> emptyBufRets;
         _hidl_cb(V3_5::BufferRequestStatus::FAILED_UNKNOWN, emptyBufRets);
         return Void();
     }

     virtual Return<void> returnStreamBuffers(const hidl_vec<StreamBuffer>&) override {
         ALOGI("returnStreamBuffers");
         ADD_FAILURE();  // Empty callback should not reach here
         return Void();
     }
 };

    struct DeviceCb : public V3_5::ICameraDeviceCallback {
        DeviceCb(CameraHidlTest *parent, int deviceVersion, const camera_metadata_t *staticMeta) :
                mParent(parent), mDeviceVersion(deviceVersion) {
            mStaticMetadata = staticMeta;
        }

        Return<void> processCaptureResult_3_4(
                const hidl_vec<V3_4::CaptureResult>& results) override;
        Return<void> processCaptureResult(const hidl_vec<CaptureResult>& results) override;
        Return<void> notify(const hidl_vec<NotifyMsg>& msgs) override;

        Return<void> requestStreamBuffers(
                const hidl_vec<V3_5::BufferRequest>& bufReqs,
                requestStreamBuffers_cb _hidl_cb) override;

        Return<void> returnStreamBuffers(const hidl_vec<StreamBuffer>& buffers) override;

        void setCurrentStreamConfig(const hidl_vec<V3_4::Stream>& streams,
                const hidl_vec<V3_2::HalStream>& halStreams);

        void waitForBuffersReturned();

     private:
        bool processCaptureResultLocked(const CaptureResult& results,
                hidl_vec<PhysicalCameraMetadata> physicalCameraMetadata);

        CameraHidlTest *mParent; // Parent object
        int mDeviceVersion;
        android::hardware::camera::common::V1_0::helper::CameraMetadata mStaticMetadata;
        bool hasOutstandingBuffersLocked();

        /* members for requestStreamBuffers() and returnStreamBuffers()*/
        std::mutex mLock; // protecting members below
        bool                      mUseHalBufManager = false;
        hidl_vec<V3_4::Stream>    mStreams;
        hidl_vec<V3_2::HalStream> mHalStreams;
        uint64_t mNextBufferId = 1;
        using OutstandingBuffers = std::unordered_map<uint64_t, hidl_handle>;
        // size == mStreams.size(). Tracking each streams outstanding buffers
        std::vector<OutstandingBuffers> mOutstandingBufferIds;
        std::condition_variable mFlushedCondition;
    };

    struct TorchProviderCb : public ICameraProviderCallback {
        TorchProviderCb(CameraHidlTest *parent) : mParent(parent) {}
        virtual Return<void> cameraDeviceStatusChange(
                const hidl_string&, CameraDeviceStatus) override {
            return Void();
        }

        virtual Return<void> torchModeStatusChange(
                const hidl_string&, TorchModeStatus newStatus) override {
            std::lock_guard<std::mutex> l(mParent->mTorchLock);
            mParent->mTorchStatus = newStatus;
            mParent->mTorchCond.notify_one();
            return Void();
        }

     private:
        CameraHidlTest *mParent;               // Parent object
    };

    struct Camera1DeviceCb :
            public ::android::hardware::camera::device::V1_0::ICameraDeviceCallback {
        Camera1DeviceCb(CameraHidlTest *parent) : mParent(parent) {}

        Return<void> notifyCallback(NotifyCallbackMsg msgType,
                int32_t ext1, int32_t ext2) override;

        Return<uint32_t> registerMemory(const hidl_handle& descriptor,
                uint32_t bufferSize, uint32_t bufferCount) override;

        Return<void> unregisterMemory(uint32_t memId) override;

        Return<void> dataCallback(DataCallbackMsg msgType,
                uint32_t data, uint32_t bufferIndex,
                const CameraFrameMetadata& metadata) override;

        Return<void> dataCallbackTimestamp(DataCallbackMsg msgType,
                uint32_t data, uint32_t bufferIndex,
                int64_t timestamp) override;

        Return<void> handleCallbackTimestamp(DataCallbackMsg msgType,
                const hidl_handle& frameData,uint32_t data,
                uint32_t bufferIndex, int64_t timestamp) override;

        Return<void> handleCallbackTimestampBatch(DataCallbackMsg msgType,
                const ::android::hardware::hidl_vec<HandleTimestampMessage>& batch) override;


     private:
        CameraHidlTest *mParent;               // Parent object
    };

    void notifyDeviceState(::android::hardware::camera::provider::V2_5::DeviceState newState);

    void openCameraDevice(const std::string &name, sp<ICameraProvider> provider,
            sp<::android::hardware::camera::device::V1_0::ICameraDevice> *device /*out*/);
    void setupPreviewWindow(
            const sp<::android::hardware::camera::device::V1_0::ICameraDevice> &device,
            sp<BufferItemConsumer> *bufferItemConsumer /*out*/,
            sp<BufferItemHander> *bufferHandler /*out*/);
    void stopPreviewAndClose(
            const sp<::android::hardware::camera::device::V1_0::ICameraDevice> &device);
    void startPreview(
            const sp<::android::hardware::camera::device::V1_0::ICameraDevice> &device);
    void enableMsgType(unsigned int msgType,
            const sp<::android::hardware::camera::device::V1_0::ICameraDevice> &device);
    void disableMsgType(unsigned int msgType,
            const sp<::android::hardware::camera::device::V1_0::ICameraDevice> &device);
    void getParameters(
            const sp<::android::hardware::camera::device::V1_0::ICameraDevice> &device,
            CameraParameters *cameraParams /*out*/);
    void setParameters(
            const sp<::android::hardware::camera::device::V1_0::ICameraDevice> &device,
            const CameraParameters &cameraParams);
    void allocateGraphicBuffer(uint32_t width, uint32_t height, uint64_t usage,
            PixelFormat format, hidl_handle *buffer_handle /*out*/);
    void waitForFrameLocked(DataCallbackMsg msgFrame,
            std::unique_lock<std::mutex> &l);
    void openEmptyDeviceSession(const std::string &name,
            sp<ICameraProvider> provider,
            sp<ICameraDeviceSession> *session /*out*/,
            camera_metadata_t **staticMeta /*out*/,
            ::android::sp<ICameraDevice> *device = nullptr/*out*/);
    void castProvider(const sp<provider::V2_4::ICameraProvider>& provider,
                      sp<provider::V2_5::ICameraProvider>* provider2_5 /*out*/,
                      sp<provider::V2_6::ICameraProvider>* provider2_6 /*out*/);
    void castSession(const sp<ICameraDeviceSession> &session, int32_t deviceVersion,
            sp<device::V3_3::ICameraDeviceSession> *session3_3 /*out*/,
            sp<device::V3_4::ICameraDeviceSession> *session3_4 /*out*/,
            sp<device::V3_5::ICameraDeviceSession> *session3_5 /*out*/,
            sp<device::V3_6::ICameraDeviceSession> *session3_6 /*out*/);
    void castDevice(const sp<device::V3_2::ICameraDevice> &device, int32_t deviceVersion,
            sp<device::V3_5::ICameraDevice> *device3_5/*out*/);
    void createStreamConfiguration(const ::android::hardware::hidl_vec<V3_2::Stream>& streams3_2,
            StreamConfigurationMode configMode,
            ::android::hardware::camera::device::V3_2::StreamConfiguration *config3_2,
            ::android::hardware::camera::device::V3_4::StreamConfiguration *config3_4,
            ::android::hardware::camera::device::V3_5::StreamConfiguration *config3_5,
            uint32_t jpegBufferSize = 0);

    void configureOfflineStillStream(const std::string &name, int32_t deviceVersion,
            sp<ICameraProvider> provider,
            const AvailableStream *threshold,
            sp<device::V3_6::ICameraDeviceSession> *session/*out*/,
            V3_2::Stream *stream /*out*/,
            device::V3_6::HalStreamConfiguration *halStreamConfig /*out*/,
            bool *supportsPartialResults /*out*/,
            uint32_t *partialResultCount /*out*/,
            sp<DeviceCb> *outCb /*out*/,
            uint32_t *jpegBufferSize /*out*/,
            bool *useHalBufManager /*out*/);
    void configurePreviewStreams3_4(const std::string &name, int32_t deviceVersion,
            sp<ICameraProvider> provider,
            const AvailableStream *previewThreshold,
            const std::unordered_set<std::string>& physicalIds,
            sp<device::V3_4::ICameraDeviceSession> *session3_4 /*out*/,
            sp<device::V3_5::ICameraDeviceSession> *session3_5 /*out*/,
            V3_2::Stream* previewStream /*out*/,
            device::V3_4::HalStreamConfiguration *halStreamConfig /*out*/,
            bool *supportsPartialResults /*out*/,
            uint32_t *partialResultCount /*out*/,
            bool *useHalBufManager /*out*/,
            sp<DeviceCb> *cb /*out*/,
            uint32_t streamConfigCounter = 0,
            bool allowUnsupport = false);
    void configurePreviewStream(const std::string &name, int32_t deviceVersion,
            sp<ICameraProvider> provider,
            const AvailableStream *previewThreshold,
            sp<ICameraDeviceSession> *session /*out*/,
            V3_2::Stream *previewStream /*out*/,
            HalStreamConfiguration *halStreamConfig /*out*/,
            bool *supportsPartialResults /*out*/,
            uint32_t *partialResultCount /*out*/,
            bool *useHalBufManager /*out*/,
            sp<DeviceCb> *cb /*out*/,
            uint32_t streamConfigCounter = 0);

    void verifyLogicalCameraMetadata(const std::string& cameraName,
            const ::android::sp<::android::hardware::camera::device::V3_2::ICameraDevice>& device,
            const CameraMetadata& chars, int deviceVersion,
            const hidl_vec<hidl_string>& deviceNames);
    void verifyCameraCharacteristics(Status status, const CameraMetadata& chars);
    void verifyExtendedSceneModeCharacteristics(const camera_metadata_t* metadata);
    void verifyZoomCharacteristics(const camera_metadata_t* metadata);
    void verifyRecommendedConfigs(const CameraMetadata& metadata);
    void verifyMonochromeCharacteristics(const CameraMetadata& chars, int deviceVersion);
    void verifyMonochromeCameraResult(
            const ::android::hardware::camera::common::V1_0::helper::CameraMetadata& metadata);
    void verifyStreamCombination(sp<device::V3_5::ICameraDevice> cameraDevice3_5,
            const ::android::hardware::camera::device::V3_4::StreamConfiguration &config3_4,
            bool expectedStatus, bool expectStreamCombQuery);
    void verifyLogicalCameraResult(const camera_metadata_t* staticMetadata,
            const ::android::hardware::camera::common::V1_0::helper::CameraMetadata& resultMetadata);

    void verifyBuffersReturned(sp<device::V3_2::ICameraDeviceSession> session,
            int deviceVerison, int32_t streamId, sp<DeviceCb> cb,
            uint32_t streamConfigCounter = 0);

    void verifyBuffersReturned(sp<device::V3_4::ICameraDeviceSession> session,
            hidl_vec<int32_t> streamIds, sp<DeviceCb> cb,
            uint32_t streamConfigCounter = 0);

    void verifySessionReconfigurationQuery(sp<device::V3_5::ICameraDeviceSession> session3_5,
            camera_metadata* oldSessionParams, camera_metadata* newSessionParams);

    void verifyRequestTemplate(const camera_metadata_t* metadata, RequestTemplate requestTemplate);

    bool isDepthOnly(camera_metadata_t* staticMeta);

    static Status getAvailableOutputStreams(const camera_metadata_t *staticMeta,
            std::vector<AvailableStream> &outputStreams,
            const AvailableStream *threshold = nullptr);

    static Status getMaxOutputSizeForFormat(const camera_metadata_t* staticMeta, PixelFormat format,
                                            Size* size);

    static Status getMandatoryConcurrentStreams(const camera_metadata_t* staticMeta,
                                                std::vector<AvailableStream>* outputStreams);

    static Status getJpegBufferSize(camera_metadata_t *staticMeta,
            uint32_t* outBufSize);
    static Status isConstrainedModeAvailable(camera_metadata_t *staticMeta);
    static Status isLogicalMultiCamera(const camera_metadata_t *staticMeta);
    static Status isOfflineSessionSupported(const camera_metadata_t *staticMeta);
    static Status getPhysicalCameraIds(const camera_metadata_t *staticMeta,
            std::unordered_set<std::string> *physicalIds/*out*/);
    static Status getSupportedKeys(camera_metadata_t *staticMeta,
            uint32_t tagId, std::unordered_set<int32_t> *requestIDs/*out*/);
    static void fillOutputStreams(camera_metadata_ro_entry_t* entry,
            std::vector<AvailableStream>& outputStreams,
            const AvailableStream *threshold = nullptr,
            const int32_t availableConfigOutputTag = 0u);
    static void constructFilteredSettings(const sp<ICameraDeviceSession>& session,
            const std::unordered_set<int32_t>& availableKeys, RequestTemplate reqTemplate,
            android::hardware::camera::common::V1_0::helper::CameraMetadata* defaultSettings/*out*/,
            android::hardware::camera::common::V1_0::helper::CameraMetadata* filteredSettings
            /*out*/);
    static Status pickConstrainedModeSize(camera_metadata_t *staticMeta,
            AvailableStream &hfrStream);
    static Status isZSLModeAvailable(const camera_metadata_t *staticMeta);
    static Status isZSLModeAvailable(const camera_metadata_t *staticMeta, ReprocessType reprocType);
    static Status getZSLInputOutputMap(camera_metadata_t *staticMeta,
            std::vector<AvailableZSLInputOutput> &inputOutputMap);
    static Status findLargestSize(
            const std::vector<AvailableStream> &streamSizes,
            int32_t format, AvailableStream &result);
    static Status isAutoFocusModeAvailable(
            CameraParameters &cameraParams, const char *mode) ;
    static Status isMonochromeCamera(const camera_metadata_t *staticMeta);

    // Used by switchToOffline where a new result queue is created for offline reqs
    void updateInflightResultQueue(std::shared_ptr<ResultMetadataQueue> resultQueue);

protected:

    // In-flight queue for tracking completion of capture requests.
    struct InFlightRequest {
        // Set by notify() SHUTTER call.
        nsecs_t shutterTimestamp;

        bool errorCodeValid;
        ErrorCode errorCode;

        //Is partial result supported
        bool usePartialResult;

        //Partial result count expected
        uint32_t numPartialResults;

        // Message queue
        std::shared_ptr<ResultMetadataQueue> resultQueue;

        // Set by process_capture_result call with valid metadata
        bool haveResultMetadata;

        // Decremented by calls to process_capture_result with valid output
        // and input buffers
        ssize_t numBuffersLeft;

         // A 64bit integer to index the frame number associated with this result.
        int64_t frameNumber;

         // The partial result count (index) for this capture result.
        int32_t partialResultCount;

        // For buffer drop errors, the stream ID for the stream that lost a buffer.
        // For physical sub-camera result errors, the Id of the physical stream
        // for the physical sub-camera.
        // Otherwise -1.
        int32_t errorStreamId;

        // If this request has any input buffer
        bool hasInputBuffer;

        // Result metadata
        ::android::hardware::camera::common::V1_0::helper::CameraMetadata collectedResult;

        // Buffers are added by process_capture_result when output buffers
        // return from HAL but framework.
        ::android::Vector<StreamBuffer> resultOutputBuffers;

        std::unordered_set<std::string> expectedPhysicalResults;

        InFlightRequest() :
                shutterTimestamp(0),
                errorCodeValid(false),
                errorCode(ErrorCode::ERROR_BUFFER),
                usePartialResult(false),
                numPartialResults(0),
                resultQueue(nullptr),
                haveResultMetadata(false),
                numBuffersLeft(0),
                frameNumber(0),
                partialResultCount(0),
                errorStreamId(-1),
                hasInputBuffer(false) {}

        InFlightRequest(ssize_t numBuffers, bool hasInput,
                bool partialResults, uint32_t partialCount,
                std::shared_ptr<ResultMetadataQueue> queue = nullptr) :
                shutterTimestamp(0),
                errorCodeValid(false),
                errorCode(ErrorCode::ERROR_BUFFER),
                usePartialResult(partialResults),
                numPartialResults(partialCount),
                resultQueue(queue),
                haveResultMetadata(false),
                numBuffersLeft(numBuffers),
                frameNumber(0),
                partialResultCount(0),
                errorStreamId(-1),
                hasInputBuffer(hasInput) {}

        InFlightRequest(ssize_t numBuffers, bool hasInput,
                bool partialResults, uint32_t partialCount,
                const std::unordered_set<std::string>& extraPhysicalResult,
                std::shared_ptr<ResultMetadataQueue> queue = nullptr) :
                shutterTimestamp(0),
                errorCodeValid(false),
                errorCode(ErrorCode::ERROR_BUFFER),
                usePartialResult(partialResults),
                numPartialResults(partialCount),
                resultQueue(queue),
                haveResultMetadata(false),
                numBuffersLeft(numBuffers),
                frameNumber(0),
                partialResultCount(0),
                errorStreamId(-1),
                hasInputBuffer(hasInput),
                expectedPhysicalResults(extraPhysicalResult) {}
    };

    // Map from frame number to the in-flight request state
    typedef ::android::KeyedVector<uint32_t, InFlightRequest*> InFlightMap;

    std::mutex mLock;                          // Synchronize access to member variables
    std::condition_variable mResultCondition;  // Condition variable for incoming results
    InFlightMap mInflightMap;                  // Map of all inflight requests

    DataCallbackMsg mDataMessageTypeReceived;  // Most recent message type received through data callbacks
    uint32_t mVideoBufferIndex;                // Buffer index of the most recent video buffer
    uint32_t mVideoData;                       // Buffer data of the most recent video buffer
    hidl_handle mVideoNativeHandle;            // Most recent video buffer native handle
    NotifyCallbackMsg mNotifyMessage;          // Current notification message

    std::mutex mTorchLock;                     // Synchronize access to torch status
    std::condition_variable mTorchCond;        // Condition variable for torch status
    TorchModeStatus mTorchStatus;              // Current torch status

    // Holds camera registered buffers
    std::unordered_map<uint32_t, sp<::android::MemoryHeapBase> > mMemoryPool;

    // Camera provider service
    sp<ICameraProvider> mProvider;
    sp<::android::hardware::camera::provider::V2_5::ICameraProvider> mProvider2_5;
    sp<::android::hardware::camera::provider::V2_6::ICameraProvider> mProvider2_6;

    // Camera provider type.
    std::string mProviderType;
};

Return<void> CameraHidlTest::Camera1DeviceCb::notifyCallback(
        NotifyCallbackMsg msgType, int32_t ext1 __unused,
        int32_t ext2 __unused) {
    std::unique_lock<std::mutex> l(mParent->mLock);
    mParent->mNotifyMessage = msgType;
    mParent->mResultCondition.notify_one();

    return Void();
}

Return<uint32_t> CameraHidlTest::Camera1DeviceCb::registerMemory(
        const hidl_handle& descriptor, uint32_t bufferSize,
        uint32_t bufferCount) {
    if (descriptor->numFds != 1) {
        ADD_FAILURE() << "camera memory descriptor has"
                " numFds " <<  descriptor->numFds << " (expect 1)" ;
        return 0;
    }
    if (descriptor->data[0] < 0) {
        ADD_FAILURE() << "camera memory descriptor has"
                " FD " << descriptor->data[0] << " (expect >= 0)";
        return 0;
    }

    sp<::android::MemoryHeapBase> pool = new ::android::MemoryHeapBase(
            descriptor->data[0], bufferSize*bufferCount, 0, 0);
    mParent->mMemoryPool.emplace(pool->getHeapID(), pool);

    return pool->getHeapID();
}

Return<void> CameraHidlTest::Camera1DeviceCb::unregisterMemory(uint32_t memId) {
    if (mParent->mMemoryPool.count(memId) == 0) {
        ALOGE("%s: memory pool ID %d not found", __FUNCTION__, memId);
        ADD_FAILURE();
        return Void();
    }

    mParent->mMemoryPool.erase(memId);
    return Void();
}

Return<void> CameraHidlTest::Camera1DeviceCb::dataCallback(
        DataCallbackMsg msgType __unused, uint32_t data __unused,
        uint32_t bufferIndex __unused,
        const CameraFrameMetadata& metadata __unused) {
    std::unique_lock<std::mutex> l(mParent->mLock);
    mParent->mDataMessageTypeReceived = msgType;
    mParent->mResultCondition.notify_one();

    return Void();
}

Return<void> CameraHidlTest::Camera1DeviceCb::dataCallbackTimestamp(
        DataCallbackMsg msgType, uint32_t data,
        uint32_t bufferIndex, int64_t timestamp __unused) {
    std::unique_lock<std::mutex> l(mParent->mLock);
    mParent->mDataMessageTypeReceived = msgType;
    mParent->mVideoBufferIndex = bufferIndex;
    if (mParent->mMemoryPool.count(data) == 0) {
        ADD_FAILURE() << "memory pool ID " << data << "not found";
    }
    mParent->mVideoData = data;
    mParent->mResultCondition.notify_one();

    return Void();
}

Return<void> CameraHidlTest::Camera1DeviceCb::handleCallbackTimestamp(
        DataCallbackMsg msgType, const hidl_handle& frameData,
        uint32_t data __unused, uint32_t bufferIndex,
        int64_t timestamp __unused) {
    std::unique_lock<std::mutex> l(mParent->mLock);
    mParent->mDataMessageTypeReceived = msgType;
    mParent->mVideoBufferIndex = bufferIndex;
    if (mParent->mMemoryPool.count(data) == 0) {
        ADD_FAILURE() << "memory pool ID " << data << " not found";
    }
    mParent->mVideoData = data;
    mParent->mVideoNativeHandle = frameData;
    mParent->mResultCondition.notify_one();

    return Void();
}

Return<void> CameraHidlTest::Camera1DeviceCb::handleCallbackTimestampBatch(
        DataCallbackMsg msgType,
        const hidl_vec<HandleTimestampMessage>& batch) {
    std::unique_lock<std::mutex> l(mParent->mLock);
    for (auto& msg : batch) {
        mParent->mDataMessageTypeReceived = msgType;
        mParent->mVideoBufferIndex = msg.bufferIndex;
        if (mParent->mMemoryPool.count(msg.data) == 0) {
            ADD_FAILURE() << "memory pool ID " << msg.data << " not found";
        }
        mParent->mVideoData = msg.data;
        mParent->mVideoNativeHandle = msg.frameData;
        mParent->mResultCondition.notify_one();
    }
    return Void();
}

Return<void> CameraHidlTest::DeviceCb::processCaptureResult_3_4(
        const hidl_vec<V3_4::CaptureResult>& results) {

    if (nullptr == mParent) {
        return Void();
    }

    bool notify = false;
    std::unique_lock<std::mutex> l(mParent->mLock);
    for (size_t i = 0 ; i < results.size(); i++) {
        notify = processCaptureResultLocked(results[i].v3_2, results[i].physicalCameraMetadata);
    }

    l.unlock();
    if (notify) {
        mParent->mResultCondition.notify_one();
    }

    return Void();
}

Return<void> CameraHidlTest::DeviceCb::processCaptureResult(
        const hidl_vec<CaptureResult>& results) {
    if (nullptr == mParent) {
        return Void();
    }

    bool notify = false;
    std::unique_lock<std::mutex> l(mParent->mLock);
    ::android::hardware::hidl_vec<PhysicalCameraMetadata> noPhysMetadata;
    for (size_t i = 0 ; i < results.size(); i++) {
        notify = processCaptureResultLocked(results[i], noPhysMetadata);
    }

    l.unlock();
    if (notify) {
        mParent->mResultCondition.notify_one();
    }

    return Void();
}

bool CameraHidlTest::DeviceCb::processCaptureResultLocked(const CaptureResult& results,
        hidl_vec<PhysicalCameraMetadata> physicalCameraMetadata) {
    bool notify = false;
    uint32_t frameNumber = results.frameNumber;

    if ((results.result.size() == 0) &&
            (results.outputBuffers.size() == 0) &&
            (results.inputBuffer.buffer == nullptr) &&
            (results.fmqResultSize == 0)) {
        ALOGE("%s: No result data provided by HAL for frame %d result count: %d",
                __func__, frameNumber, (int) results.fmqResultSize);
        ADD_FAILURE();
        return notify;
    }

    ssize_t idx = mParent->mInflightMap.indexOfKey(frameNumber);
    if (::android::NAME_NOT_FOUND == idx) {
        ALOGE("%s: Unexpected frame number! received: %u",
                __func__, frameNumber);
        ADD_FAILURE();
        return notify;
    }

    bool isPartialResult = false;
    bool hasInputBufferInRequest = false;
    InFlightRequest *request = mParent->mInflightMap.editValueAt(idx);
    ::android::hardware::camera::device::V3_2::CameraMetadata resultMetadata;
    size_t resultSize = 0;
    if (results.fmqResultSize > 0) {
        resultMetadata.resize(results.fmqResultSize);
        if (request->resultQueue == nullptr) {
            ADD_FAILURE();
            return notify;
        }
        if (!request->resultQueue->read(resultMetadata.data(),
                    results.fmqResultSize)) {
            ALOGE("%s: Frame %d: Cannot read camera metadata from fmq,"
                    "size = %" PRIu64, __func__, frameNumber,
                    results.fmqResultSize);
            ADD_FAILURE();
            return notify;
        }

        if (physicalCameraMetadata.size() != request->expectedPhysicalResults.size()) {
            ALOGE("%s: Frame %d: Returned physical metadata count %zu "
                    "must be equal to expected count %zu", __func__, frameNumber,
                    physicalCameraMetadata.size(), request->expectedPhysicalResults.size());
            ADD_FAILURE();
            return notify;
        }
        std::vector<::android::hardware::camera::device::V3_2::CameraMetadata> physResultMetadata;
        physResultMetadata.resize(physicalCameraMetadata.size());
        for (size_t i = 0; i < physicalCameraMetadata.size(); i++) {
            physResultMetadata[i].resize(physicalCameraMetadata[i].fmqMetadataSize);
            if (!request->resultQueue->read(physResultMetadata[i].data(),
                    physicalCameraMetadata[i].fmqMetadataSize)) {
                ALOGE("%s: Frame %d: Cannot read physical camera metadata from fmq,"
                        "size = %" PRIu64, __func__, frameNumber,
                        physicalCameraMetadata[i].fmqMetadataSize);
                ADD_FAILURE();
                return notify;
            }
        }
        resultSize = resultMetadata.size();
    } else if (results.result.size() > 0) {
        resultMetadata.setToExternal(const_cast<uint8_t *>(
                    results.result.data()), results.result.size());
        resultSize = resultMetadata.size();
    }

    if (!request->usePartialResult && (resultSize > 0) &&
            (results.partialResult != 1)) {
        ALOGE("%s: Result is malformed for frame %d: partial_result %u "
                "must be 1  if partial result is not supported", __func__,
                frameNumber, results.partialResult);
        ADD_FAILURE();
        return notify;
    }

    if (results.partialResult != 0) {
        request->partialResultCount = results.partialResult;
    }

    // Check if this result carries only partial metadata
    if (request->usePartialResult && (resultSize > 0)) {
        if ((results.partialResult > request->numPartialResults) ||
                (results.partialResult < 1)) {
            ALOGE("%s: Result is malformed for frame %d: partial_result %u"
                    " must be  in the range of [1, %d] when metadata is "
                    "included in the result", __func__, frameNumber,
                    results.partialResult, request->numPartialResults);
            ADD_FAILURE();
            return notify;
        }
        request->collectedResult.append(
                reinterpret_cast<const camera_metadata_t*>(
                    resultMetadata.data()));

        isPartialResult =
            (results.partialResult < request->numPartialResults);
    } else if (resultSize > 0) {
        request->collectedResult.append(reinterpret_cast<const camera_metadata_t*>(
                    resultMetadata.data()));
        isPartialResult = false;
    }

    hasInputBufferInRequest = request->hasInputBuffer;

    // Did we get the (final) result metadata for this capture?
    if ((resultSize > 0) && !isPartialResult) {
        if (request->haveResultMetadata) {
            ALOGE("%s: Called multiple times with metadata for frame %d",
                    __func__, frameNumber);
            ADD_FAILURE();
            return notify;
        }
        request->haveResultMetadata = true;
        request->collectedResult.sort();

        // Verify final result metadata
        bool isAtLeast_3_5 = mDeviceVersion >= CAMERA_DEVICE_API_VERSION_3_5;
        if (isAtLeast_3_5) {
            auto staticMetadataBuffer = mStaticMetadata.getAndLock();
            bool isMonochrome = Status::OK ==
                    CameraHidlTest::isMonochromeCamera(staticMetadataBuffer);
            if (isMonochrome) {
                mParent->verifyMonochromeCameraResult(request->collectedResult);
            }

            // Verify logical camera result metadata
            bool isLogicalCamera =
                    Status::OK == CameraHidlTest::isLogicalMultiCamera(staticMetadataBuffer);
            if (isLogicalCamera) {
                mParent->verifyLogicalCameraResult(staticMetadataBuffer, request->collectedResult);
            }
            mStaticMetadata.unlock(staticMetadataBuffer);
        }
    }

    uint32_t numBuffersReturned = results.outputBuffers.size();
    if (results.inputBuffer.buffer != nullptr) {
        if (hasInputBufferInRequest) {
            numBuffersReturned += 1;
        } else {
            ALOGW("%s: Input buffer should be NULL if there is no input"
                    " buffer sent in the request", __func__);
        }
    }
    request->numBuffersLeft -= numBuffersReturned;
    if (request->numBuffersLeft < 0) {
        ALOGE("%s: Too many buffers returned for frame %d", __func__,
                frameNumber);
        ADD_FAILURE();
        return notify;
    }

    request->resultOutputBuffers.appendArray(results.outputBuffers.data(),
            results.outputBuffers.size());
    // If shutter event is received notify the pending threads.
    if (request->shutterTimestamp != 0) {
        notify = true;
    }

    if (mUseHalBufManager) {
        // Don't return buffers of bufId 0 (empty buffer)
        std::vector<StreamBuffer> buffers;
        for (const auto& sb : results.outputBuffers) {
            if (sb.bufferId != 0) {
                buffers.push_back(sb);
            }
        }
        returnStreamBuffers(buffers);
    }
    return notify;
}

void CameraHidlTest::DeviceCb::setCurrentStreamConfig(
        const hidl_vec<V3_4::Stream>& streams, const hidl_vec<V3_2::HalStream>& halStreams) {
    ASSERT_EQ(streams.size(), halStreams.size());
    ASSERT_NE(streams.size(), 0);
    for (size_t i = 0; i < streams.size(); i++) {
        ASSERT_EQ(streams[i].v3_2.id, halStreams[i].id);
    }
    std::lock_guard<std::mutex> l(mLock);
    mUseHalBufManager = true;
    mStreams = streams;
    mHalStreams = halStreams;
    mOutstandingBufferIds.clear();
    for (size_t i = 0; i < streams.size(); i++) {
        mOutstandingBufferIds.emplace_back();
    }
}

bool CameraHidlTest::DeviceCb::hasOutstandingBuffersLocked() {
    if (!mUseHalBufManager) {
        return false;
    }
    for (const auto& outstandingBuffers : mOutstandingBufferIds) {
        if (!outstandingBuffers.empty()) {
            return true;
        }
    }
    return false;
}

void CameraHidlTest::DeviceCb::waitForBuffersReturned() {
    std::unique_lock<std::mutex> lk(mLock);
    if (hasOutstandingBuffersLocked()) {
        auto timeout = std::chrono::seconds(kBufferReturnTimeoutSec);
        auto st = mFlushedCondition.wait_for(lk, timeout);
        ASSERT_NE(std::cv_status::timeout, st);
    }
}

Return<void> CameraHidlTest::DeviceCb::notify(
        const hidl_vec<NotifyMsg>& messages) {
    std::lock_guard<std::mutex> l(mParent->mLock);

    for (size_t i = 0; i < messages.size(); i++) {
        switch(messages[i].type) {
            case MsgType::ERROR:
                if (ErrorCode::ERROR_DEVICE == messages[i].msg.error.errorCode) {
                    ALOGE("%s: Camera reported serious device error",
                          __func__);
                    ADD_FAILURE();
                } else {
                    ssize_t idx = mParent->mInflightMap.indexOfKey(
                            messages[i].msg.error.frameNumber);
                    if (::android::NAME_NOT_FOUND == idx) {
                        ALOGE("%s: Unexpected error frame number! received: %u",
                              __func__, messages[i].msg.error.frameNumber);
                        ADD_FAILURE();
                        break;
                    }
                    InFlightRequest *r = mParent->mInflightMap.editValueAt(idx);

                    if (ErrorCode::ERROR_RESULT == messages[i].msg.error.errorCode &&
                            messages[i].msg.error.errorStreamId != -1) {
                        if (r->haveResultMetadata) {
                            ALOGE("%s: Camera must report physical camera result error before "
                                    "the final capture result!", __func__);
                            ADD_FAILURE();
                        } else {
                            for (size_t j = 0; j < mStreams.size(); j++) {
                                if (mStreams[j].v3_2.id == messages[i].msg.error.errorStreamId) {
                                    hidl_string physicalCameraId = mStreams[j].physicalCameraId;
                                    bool idExpected = r->expectedPhysicalResults.find(
                                            physicalCameraId) != r->expectedPhysicalResults.end();
                                    if (!idExpected) {
                                        ALOGE("%s: ERROR_RESULT's error stream's physicalCameraId "
                                                "%s must be expected", __func__,
                                                physicalCameraId.c_str());
                                        ADD_FAILURE();
                                    } else {
                                        r->expectedPhysicalResults.erase(physicalCameraId);
                                    }
                                    break;
                                }
                            }
                        }
                    } else {
                        r->errorCodeValid = true;
                        r->errorCode = messages[i].msg.error.errorCode;
                        r->errorStreamId = messages[i].msg.error.errorStreamId;
                  }
                }
                break;
            case MsgType::SHUTTER:
            {
                ssize_t idx = mParent->mInflightMap.indexOfKey(messages[i].msg.shutter.frameNumber);
                if (::android::NAME_NOT_FOUND == idx) {
                    ALOGE("%s: Unexpected shutter frame number! received: %u",
                          __func__, messages[i].msg.shutter.frameNumber);
                    ADD_FAILURE();
                    break;
                }
                InFlightRequest *r = mParent->mInflightMap.editValueAt(idx);
                r->shutterTimestamp = messages[i].msg.shutter.timestamp;
            }
                break;
            default:
                ALOGE("%s: Unsupported notify message %d", __func__,
                      messages[i].type);
                ADD_FAILURE();
                break;
        }
    }

    mParent->mResultCondition.notify_one();
    return Void();
}

Return<void> CameraHidlTest::DeviceCb::requestStreamBuffers(
        const hidl_vec<V3_5::BufferRequest>& bufReqs,
        requestStreamBuffers_cb _hidl_cb) {
    using V3_5::BufferRequestStatus;
    using V3_5::StreamBufferRet;
    using V3_5::StreamBufferRequestError;
    hidl_vec<StreamBufferRet> bufRets;
    std::unique_lock<std::mutex> l(mLock);

    if (!mUseHalBufManager) {
        ALOGE("%s: Camera does not support HAL buffer management", __FUNCTION__);
        ADD_FAILURE();
        _hidl_cb(BufferRequestStatus::FAILED_ILLEGAL_ARGUMENTS, bufRets);
        return Void();
    }

    if (bufReqs.size() > mStreams.size()) {
        ALOGE("%s: illegal buffer request: too many requests!", __FUNCTION__);
        ADD_FAILURE();
        _hidl_cb(BufferRequestStatus::FAILED_ILLEGAL_ARGUMENTS, bufRets);
        return Void();
    }

    std::vector<int32_t> indexes(bufReqs.size());
    for (size_t i = 0; i < bufReqs.size(); i++) {
        bool found = false;
        for (size_t idx = 0; idx < mStreams.size(); idx++) {
            if (bufReqs[i].streamId == mStreams[idx].v3_2.id) {
                found = true;
                indexes[i] = idx;
                break;
            }
        }
        if (!found) {
            ALOGE("%s: illegal buffer request: unknown streamId %d!",
                    __FUNCTION__, bufReqs[i].streamId);
            ADD_FAILURE();
            _hidl_cb(BufferRequestStatus::FAILED_ILLEGAL_ARGUMENTS, bufRets);
            return Void();
        }
    }

    bool allStreamOk = true;
    bool atLeastOneStreamOk = false;
    bufRets.resize(bufReqs.size());
    for (size_t i = 0; i < bufReqs.size(); i++) {
        int32_t idx = indexes[i];
        const auto& stream = mStreams[idx];
        const auto& halStream = mHalStreams[idx];
        const V3_5::BufferRequest& bufReq = bufReqs[i];
        if (mOutstandingBufferIds[idx].size() + bufReq.numBuffersRequested > halStream.maxBuffers) {
            bufRets[i].streamId = stream.v3_2.id;
            bufRets[i].val.error(StreamBufferRequestError::MAX_BUFFER_EXCEEDED);
            allStreamOk = false;
            continue;
        }

        hidl_vec<StreamBuffer> tmpRetBuffers(bufReq.numBuffersRequested);
        for (size_t j = 0; j < bufReq.numBuffersRequested; j++) {
            hidl_handle buffer_handle;
            uint32_t w = stream.v3_2.width;
            uint32_t h = stream.v3_2.height;
            if (stream.v3_2.format == PixelFormat::BLOB) {
                w = stream.bufferSize;
                h = 1;
            }
            mParent->allocateGraphicBuffer(w, h,
                    android_convertGralloc1To0Usage(
                            halStream.producerUsage, halStream.consumerUsage),
                    halStream.overrideFormat, &buffer_handle);

            tmpRetBuffers[j] = {stream.v3_2.id, mNextBufferId, buffer_handle, BufferStatus::OK,
                                nullptr, nullptr};
            mOutstandingBufferIds[idx].insert(std::make_pair(mNextBufferId++, buffer_handle));
        }
        atLeastOneStreamOk = true;
        bufRets[i].streamId = stream.v3_2.id;
        bufRets[i].val.buffers(std::move(tmpRetBuffers));
    }

    if (allStreamOk) {
        _hidl_cb(BufferRequestStatus::OK, bufRets);
    } else if (atLeastOneStreamOk) {
        _hidl_cb(BufferRequestStatus::FAILED_PARTIAL, bufRets);
    } else {
        _hidl_cb(BufferRequestStatus::FAILED_UNKNOWN, bufRets);
    }

    if (!hasOutstandingBuffersLocked()) {
        l.unlock();
        mFlushedCondition.notify_one();
    }
    return Void();
}

Return<void> CameraHidlTest::DeviceCb::returnStreamBuffers(
        const hidl_vec<StreamBuffer>& buffers) {
    if (!mUseHalBufManager) {
        ALOGE("%s: Camera does not support HAL buffer management", __FUNCTION__);
        ADD_FAILURE();
    }

    std::unique_lock<std::mutex> l(mLock);
    for (const auto& buf : buffers) {
        bool found = false;
        for (size_t idx = 0; idx < mOutstandingBufferIds.size(); idx++) {
            if (mStreams[idx].v3_2.id == buf.streamId &&
                    mOutstandingBufferIds[idx].count(buf.bufferId) == 1) {
                mOutstandingBufferIds[idx].erase(buf.bufferId);
                // TODO: check do we need to close/delete native handle or assume we have enough
                // memory to run till the test finish? since we do not capture much requests (and
                // most of time one buffer is sufficient)
                found = true;
                break;
            }
        }
        if (found) {
            continue;
        }
        ALOGE("%s: unknown buffer ID %" PRIu64, __FUNCTION__, buf.bufferId);
        ADD_FAILURE();
    }
    if (!hasOutstandingBuffersLocked()) {
        l.unlock();
        mFlushedCondition.notify_one();
    }
    return Void();
}

std::map<hidl_string, hidl_string> CameraHidlTest::getCameraDeviceIdToNameMap(
        sp<ICameraProvider> provider) {
    hidl_vec<hidl_string> cameraDeviceNames = getCameraDeviceNames(provider);
    std::map<hidl_string, hidl_string> idToNameMap;
    for (auto& name : cameraDeviceNames) {
        std::string version, cameraId;
        if (!matchDeviceName(name, mProviderType, &version, &cameraId)) {
            ADD_FAILURE();
        }
        idToNameMap.insert(std::make_pair(hidl_string(cameraId), name));
    }
    return idToNameMap;
}

hidl_vec<hidl_string> CameraHidlTest::getCameraDeviceNames(sp<ICameraProvider> provider) {
    std::vector<std::string> cameraDeviceNames;
    Return<void> ret;
    ret = provider->getCameraIdList(
        [&](auto status, const auto& idList) {
            ALOGI("getCameraIdList returns status:%d", (int)status);
            for (size_t i = 0; i < idList.size(); i++) {
                ALOGI("Camera Id[%zu] is %s", i, idList[i].c_str());
            }
            ASSERT_EQ(Status::OK, status);
            for (const auto& id : idList) {
                cameraDeviceNames.push_back(id);
            }
        });
    if (!ret.isOk()) {
        ADD_FAILURE();
    }

    // External camera devices are reported through cameraDeviceStatusChange
    struct ProviderCb : public ICameraProviderCallback {
        virtual Return<void> cameraDeviceStatusChange(
                const hidl_string& devName,
                CameraDeviceStatus newStatus) override {
            ALOGI("camera device status callback name %s, status %d",
                    devName.c_str(), (int) newStatus);
            if (newStatus == CameraDeviceStatus::PRESENT) {
                externalCameraDeviceNames.push_back(devName);

            }
            return Void();
        }

        virtual Return<void> torchModeStatusChange(
                const hidl_string&, TorchModeStatus) override {
            return Void();
        }

        std::vector<std::string> externalCameraDeviceNames;
    };
    sp<ProviderCb> cb = new ProviderCb;
    auto status = mProvider->setCallback(cb);

    for (const auto& devName : cb->externalCameraDeviceNames) {
        if (cameraDeviceNames.end() == std::find(
                cameraDeviceNames.begin(), cameraDeviceNames.end(), devName)) {
            cameraDeviceNames.push_back(devName);
        }
    }

    hidl_vec<hidl_string> retList(cameraDeviceNames.size());
    for (size_t i = 0; i < cameraDeviceNames.size(); i++) {
        retList[i] = cameraDeviceNames[i];
    }
    return retList;
}

hidl_vec<hidl_vec<hidl_string>> CameraHidlTest::getConcurrentDeviceCombinations(
        sp<::android::hardware::camera::provider::V2_6::ICameraProvider>& provider2_6) {
    hidl_vec<hidl_vec<hidl_string>> combinations;
    Return<void> ret = provider2_6->getConcurrentStreamingCameraIds(
            [&combinations](Status concurrentIdStatus,
                            const hidl_vec<hidl_vec<hidl_string>>& cameraDeviceIdCombinations) {
                ASSERT_EQ(concurrentIdStatus, Status::OK);
                combinations = cameraDeviceIdCombinations;
            });
    if (!ret.isOk()) {
        ADD_FAILURE();
    }
    return combinations;
}

// Test devices with first_api_level >= P does not advertise device@1.0
TEST_P(CameraHidlTest, noHal1AfterP) {
    constexpr int32_t HAL1_PHASE_OUT_API_LEVEL = 28;
    int32_t firstApiLevel = 0;
    getFirstApiLevel(&firstApiLevel);

    // all devices with first API level == 28 and <= 1GB of RAM must set low_ram
    // and thus be allowed to continue using HAL1
    if ((firstApiLevel == HAL1_PHASE_OUT_API_LEVEL) &&
        (property_get_bool("ro.config.low_ram", /*default*/ false))) {
        ALOGI("Hal1 allowed for low ram device");
        return;
    }

    if (firstApiLevel >= HAL1_PHASE_OUT_API_LEVEL) {
        hidl_vec<hidl_string> cameraDeviceNames = getCameraDeviceNames(mProvider);
        for (const auto& name : cameraDeviceNames) {
            int deviceVersion = getCameraDeviceVersion(name, mProviderType);
            ASSERT_NE(deviceVersion, 0); // Must be a valid device version
            ASSERT_NE(deviceVersion, CAMERA_DEVICE_API_VERSION_1_0); // Must not be device@1.0
        }
    }
}

// Test if ICameraProvider::isTorchModeSupported returns Status::OK
// Also if first_api_level >= Q torch API must be supported.
TEST_P(CameraHidlTest, isTorchModeSupported) {
    constexpr int32_t API_LEVEL_Q = 29;
    int32_t firstApiLevel = 0;
    getFirstApiLevel(&firstApiLevel);

    Return<void> ret;
    ret = mProvider->isSetTorchModeSupported([&](auto status, bool support) {
        ALOGI("isSetTorchModeSupported returns status:%d supported:%d", (int)status, support);
        ASSERT_EQ(Status::OK, status);
        if (firstApiLevel >= API_LEVEL_Q) {
            ASSERT_EQ(true, support);
        }
    });
    ASSERT_TRUE(ret.isOk());
}

// TODO: consider removing this test if getCameraDeviceNames() has the same coverage
TEST_P(CameraHidlTest, getCameraIdList) {
    Return<void> ret;
    ret = mProvider->getCameraIdList([&](auto status, const auto& idList) {
        ALOGI("getCameraIdList returns status:%d", (int)status);
        for (size_t i = 0; i < idList.size(); i++) {
            ALOGI("Camera Id[%zu] is %s", i, idList[i].c_str());
        }
        ASSERT_EQ(Status::OK, status);
    });
    ASSERT_TRUE(ret.isOk());
}

// Test if ICameraProvider::getVendorTags returns Status::OK
TEST_P(CameraHidlTest, getVendorTags) {
    Return<void> ret;
    ret = mProvider->getVendorTags([&](auto status, const auto& vendorTagSecs) {
        ALOGI("getVendorTags returns status:%d numSections %zu", (int)status, vendorTagSecs.size());
        for (size_t i = 0; i < vendorTagSecs.size(); i++) {
            ALOGI("Vendor tag section %zu name %s", i, vendorTagSecs[i].sectionName.c_str());
            for (size_t j = 0; j < vendorTagSecs[i].tags.size(); j++) {
                const auto& tag = vendorTagSecs[i].tags[j];
                ALOGI("Vendor tag id %u name %s type %d", tag.tagId, tag.tagName.c_str(),
                      (int)tag.tagType);
            }
        }
        ASSERT_EQ(Status::OK, status);
    });
    ASSERT_TRUE(ret.isOk());
}

// Test if ICameraProvider::setCallback returns Status::OK
TEST_P(CameraHidlTest, setCallback) {
    struct ProviderCb : public ICameraProviderCallback {
        virtual Return<void> cameraDeviceStatusChange(
                const hidl_string& cameraDeviceName,
                CameraDeviceStatus newStatus) override {
            ALOGI("camera device status callback name %s, status %d",
                    cameraDeviceName.c_str(), (int) newStatus);
            return Void();
        }

        virtual Return<void> torchModeStatusChange(
                const hidl_string& cameraDeviceName,
                TorchModeStatus newStatus) override {
            ALOGI("Torch mode status callback name %s, status %d",
                    cameraDeviceName.c_str(), (int) newStatus);
            return Void();
        }
    };

    struct ProviderCb2_6
        : public ::android::hardware::camera::provider::V2_6::ICameraProviderCallback {
        virtual Return<void> cameraDeviceStatusChange(const hidl_string& cameraDeviceName,
                                                      CameraDeviceStatus newStatus) override {
            ALOGI("camera device status callback name %s, status %d", cameraDeviceName.c_str(),
                  (int)newStatus);
            return Void();
        }

        virtual Return<void> torchModeStatusChange(const hidl_string& cameraDeviceName,
                                                   TorchModeStatus newStatus) override {
            ALOGI("Torch mode status callback name %s, status %d", cameraDeviceName.c_str(),
                  (int)newStatus);
            return Void();
        }

        virtual Return<void> physicalCameraDeviceStatusChange(
                const hidl_string& cameraDeviceName, const hidl_string& physicalCameraDeviceName,
                CameraDeviceStatus newStatus) override {
            ALOGI("physical camera device status callback name %s, physical camera name %s,"
                  " status %d",
                  cameraDeviceName.c_str(), physicalCameraDeviceName.c_str(), (int)newStatus);
            return Void();
        }
    };

    sp<ProviderCb> cb = new ProviderCb;
    auto status = mProvider->setCallback(cb);
    ASSERT_TRUE(status.isOk());
    ASSERT_EQ(Status::OK, status);
    status = mProvider->setCallback(nullptr);
    ASSERT_TRUE(status.isOk());
    ASSERT_EQ(Status::OK, status);

    if (mProvider2_6.get() != nullptr) {
        sp<ProviderCb2_6> cb = new ProviderCb2_6;
        auto status = mProvider2_6->setCallback(cb);
        ASSERT_TRUE(status.isOk());
        ASSERT_EQ(Status::OK, status);
        status = mProvider2_6->setCallback(nullptr);
        ASSERT_TRUE(status.isOk());
        ASSERT_EQ(Status::OK, status);
    }
}

// Test if ICameraProvider::getCameraDeviceInterface returns Status::OK and non-null device
TEST_P(CameraHidlTest, getCameraDeviceInterface) {
    hidl_vec<hidl_string> cameraDeviceNames = getCameraDeviceNames(mProvider);

    for (const auto& name : cameraDeviceNames) {
        int deviceVersion = getCameraDeviceVersion(name, mProviderType);
        switch (deviceVersion) {
            case CAMERA_DEVICE_API_VERSION_3_6:
            case CAMERA_DEVICE_API_VERSION_3_5:
            case CAMERA_DEVICE_API_VERSION_3_4:
            case CAMERA_DEVICE_API_VERSION_3_3:
            case CAMERA_DEVICE_API_VERSION_3_2: {
                Return<void> ret;
                ret = mProvider->getCameraDeviceInterface_V3_x(
                    name, [&](auto status, const auto& device3_x) {
                        ALOGI("getCameraDeviceInterface_V3_x returns status:%d", (int)status);
                        ASSERT_EQ(Status::OK, status);
                        ASSERT_NE(device3_x, nullptr);
                    });
                ASSERT_TRUE(ret.isOk());
            }
            break;
            case CAMERA_DEVICE_API_VERSION_1_0: {
                Return<void> ret;
                ret = mProvider->getCameraDeviceInterface_V1_x(
                    name, [&](auto status, const auto& device1) {
                        ALOGI("getCameraDeviceInterface_V1_x returns status:%d", (int)status);
                        ASSERT_EQ(Status::OK, status);
                        ASSERT_NE(device1, nullptr);
                    });
                ASSERT_TRUE(ret.isOk());
            }
            break;
            default: {
                ALOGE("%s: Unsupported device version %d", __func__, deviceVersion);
                ADD_FAILURE();
            }
            break;
        }
    }
}

// Verify that the device resource cost can be retrieved and the values are
// sane.
TEST_P(CameraHidlTest, getResourceCost) {
    hidl_vec<hidl_string> cameraDeviceNames = getCameraDeviceNames(mProvider);

    for (const auto& name : cameraDeviceNames) {
        int deviceVersion = getCameraDeviceVersion(name, mProviderType);
        switch (deviceVersion) {
            case CAMERA_DEVICE_API_VERSION_3_6:
            case CAMERA_DEVICE_API_VERSION_3_5:
            case CAMERA_DEVICE_API_VERSION_3_4:
            case CAMERA_DEVICE_API_VERSION_3_3:
            case CAMERA_DEVICE_API_VERSION_3_2: {
                ::android::sp<::android::hardware::camera::device::V3_2::ICameraDevice> device3_x;
                ALOGI("getResourceCost: Testing camera device %s", name.c_str());
                Return<void> ret;
                ret = mProvider->getCameraDeviceInterface_V3_x(
                    name, [&](auto status, const auto& device) {
                        ALOGI("getCameraDeviceInterface_V3_x returns status:%d", (int)status);
                        ASSERT_EQ(Status::OK, status);
                        ASSERT_NE(device, nullptr);
                        device3_x = device;
                    });
                ASSERT_TRUE(ret.isOk());

                ret = device3_x->getResourceCost([&](auto status, const auto& resourceCost) {
                    ALOGI("getResourceCost returns status:%d", (int)status);
                    ASSERT_EQ(Status::OK, status);
                    ALOGI("    Resource cost is %d", resourceCost.resourceCost);
                    ASSERT_LE(resourceCost.resourceCost, 100u);
                    for (const auto& name : resourceCost.conflictingDevices) {
                        ALOGI("    Conflicting device: %s", name.c_str());
                    }
                });
                ASSERT_TRUE(ret.isOk());
            }
            break;
            case CAMERA_DEVICE_API_VERSION_1_0: {
                ::android::sp<::android::hardware::camera::device::V1_0::ICameraDevice> device1;
                ALOGI("getResourceCost: Testing camera device %s", name.c_str());
                Return<void> ret;
                ret = mProvider->getCameraDeviceInterface_V1_x(
                    name, [&](auto status, const auto& device) {
                        ALOGI("getCameraDeviceInterface_V1_x returns status:%d", (int)status);
                        ASSERT_EQ(Status::OK, status);
                        ASSERT_NE(device, nullptr);
                        device1 = device;
                    });
                ASSERT_TRUE(ret.isOk());

                ret = device1->getResourceCost([&](auto status, const auto& resourceCost) {
                    ALOGI("getResourceCost returns status:%d", (int)status);
                    ASSERT_EQ(Status::OK, status);
                    ALOGI("    Resource cost is %d", resourceCost.resourceCost);
                    ASSERT_LE(resourceCost.resourceCost, 100u);
                    for (const auto& name : resourceCost.conflictingDevices) {
                        ALOGI("    Conflicting device: %s", name.c_str());
                    }
                });
                ASSERT_TRUE(ret.isOk());
            }
            break;
            default: {
                ALOGE("%s: Unsupported device version %d", __func__, deviceVersion);
                ADD_FAILURE();
            }
            break;
        }
    }
}

// Verify that the static camera info can be retrieved
// successfully.
TEST_P(CameraHidlTest, getCameraInfo) {
    hidl_vec<hidl_string> cameraDeviceNames = getCameraDeviceNames(mProvider);

    for (const auto& name : cameraDeviceNames) {
        if (getCameraDeviceVersion(name, mProviderType) == CAMERA_DEVICE_API_VERSION_1_0) {
            ::android::sp<::android::hardware::camera::device::V1_0::ICameraDevice> device1;
            ALOGI("getCameraCharacteristics: Testing camera device %s", name.c_str());
            Return<void> ret;
            ret = mProvider->getCameraDeviceInterface_V1_x(
                name, [&](auto status, const auto& device) {
                    ALOGI("getCameraDeviceInterface_V1_x returns status:%d", (int)status);
                    ASSERT_EQ(Status::OK, status);
                    ASSERT_NE(device, nullptr);
                    device1 = device;
                });
            ASSERT_TRUE(ret.isOk());

            ret = device1->getCameraInfo([&](auto status, const auto& info) {
                ALOGI("getCameraInfo returns status:%d", (int)status);
                ASSERT_EQ(Status::OK, status);
                switch (info.orientation) {
                    case 0:
                    case 90:
                    case 180:
                    case 270:
                        // Expected cases
                        ALOGI("camera orientation: %d", info.orientation);
                        break;
                    default:
                        FAIL() << "Unexpected camera orientation:" << info.orientation;
                }
                switch (info.facing) {
                    case CameraFacing::BACK:
                    case CameraFacing::FRONT:
                    case CameraFacing::EXTERNAL:
                        // Expected cases
                        ALOGI("camera facing: %d", info.facing);
                        break;
                    default:
                        FAIL() << "Unexpected camera facing:" << static_cast<uint32_t>(info.facing);
                }
            });
            ASSERT_TRUE(ret.isOk());
        }
    }
}

// Check whether preview window can be configured
TEST_P(CameraHidlTest, setPreviewWindow) {
    hidl_vec<hidl_string> cameraDeviceNames = getCameraDeviceNames(mProvider);

    for (const auto& name : cameraDeviceNames) {
        if (getCameraDeviceVersion(name, mProviderType) == CAMERA_DEVICE_API_VERSION_1_0) {
            sp<::android::hardware::camera::device::V1_0::ICameraDevice> device1;
            openCameraDevice(name, mProvider, &device1 /*out*/);
            ASSERT_NE(nullptr, device1.get());
            sp<BufferItemConsumer> bufferItemConsumer;
            sp<BufferItemHander> bufferHandler;
            setupPreviewWindow(device1, &bufferItemConsumer /*out*/, &bufferHandler /*out*/);

            Return<void> ret;
            ret = device1->close();
            ASSERT_TRUE(ret.isOk());
        }
    }
}

// Verify that setting preview window fails in case device is not open
TEST_P(CameraHidlTest, setPreviewWindowInvalid) {
    hidl_vec<hidl_string> cameraDeviceNames = getCameraDeviceNames(mProvider);

    for (const auto& name : cameraDeviceNames) {
        if (getCameraDeviceVersion(name, mProviderType) == CAMERA_DEVICE_API_VERSION_1_0) {
            ::android::sp<::android::hardware::camera::device::V1_0::ICameraDevice> device1;
            ALOGI("getCameraCharacteristics: Testing camera device %s", name.c_str());
            Return<void> ret;
            ret = mProvider->getCameraDeviceInterface_V1_x(
                name, [&](auto status, const auto& device) {
                    ALOGI("getCameraDeviceInterface_V1_x returns status:%d", (int)status);
                    ASSERT_EQ(Status::OK, status);
                    ASSERT_NE(device, nullptr);
                    device1 = device;
                });
            ASSERT_TRUE(ret.isOk());

            Return<Status> returnStatus = device1->setPreviewWindow(nullptr);
            ASSERT_TRUE(returnStatus.isOk());
            ASSERT_EQ(Status::OPERATION_NOT_SUPPORTED, returnStatus);
        }
    }
}

// Start and stop preview checking whether it gets enabled in between.
TEST_P(CameraHidlTest, startStopPreview) {
    hidl_vec<hidl_string> cameraDeviceNames = getCameraDeviceNames(mProvider);

    for (const auto& name : cameraDeviceNames) {
        if (getCameraDeviceVersion(name, mProviderType) == CAMERA_DEVICE_API_VERSION_1_0) {
            sp<::android::hardware::camera::device::V1_0::ICameraDevice> device1;
            openCameraDevice(name, mProvider, &device1 /*out*/);
            ASSERT_NE(nullptr, device1.get());
            sp<BufferItemConsumer> bufferItemConsumer;
            sp<BufferItemHander> bufferHandler;
            setupPreviewWindow(device1, &bufferItemConsumer /*out*/, &bufferHandler /*out*/);

            startPreview(device1);

            Return<bool> returnBoolStatus = device1->previewEnabled();
            ASSERT_TRUE(returnBoolStatus.isOk());
            ASSERT_TRUE(returnBoolStatus);

            stopPreviewAndClose(device1);
        }
    }
}

// Start preview without active preview window. Preview should start as soon
// as a valid active window gets configured.
TEST_P(CameraHidlTest, startStopPreviewDelayed) {
    hidl_vec<hidl_string> cameraDeviceNames = getCameraDeviceNames(mProvider);

    for (const auto& name : cameraDeviceNames) {
        if (getCameraDeviceVersion(name, mProviderType) == CAMERA_DEVICE_API_VERSION_1_0) {
            sp<::android::hardware::camera::device::V1_0::ICameraDevice> device1;
            openCameraDevice(name, mProvider, &device1 /*out*/);
            ASSERT_NE(nullptr, device1.get());

            Return<Status> returnStatus = device1->setPreviewWindow(nullptr);
            ASSERT_TRUE(returnStatus.isOk());
            ASSERT_EQ(Status::OK, returnStatus);

            startPreview(device1);

            sp<BufferItemConsumer> bufferItemConsumer;
            sp<BufferItemHander> bufferHandler;
            setupPreviewWindow(device1, &bufferItemConsumer /*out*/, &bufferHandler /*out*/);

            // Preview should get enabled now
            Return<bool> returnBoolStatus = device1->previewEnabled();
            ASSERT_TRUE(returnBoolStatus.isOk());
            ASSERT_TRUE(returnBoolStatus);

            stopPreviewAndClose(device1);
        }
    }
}

// Verify that image capture behaves as expected along with preview callbacks.
TEST_P(CameraHidlTest, takePicture) {
    hidl_vec<hidl_string> cameraDeviceNames = getCameraDeviceNames(mProvider);

    for (const auto& name : cameraDeviceNames) {
        if (getCameraDeviceVersion(name, mProviderType) == CAMERA_DEVICE_API_VERSION_1_0) {
            sp<::android::hardware::camera::device::V1_0::ICameraDevice> device1;
            openCameraDevice(name, mProvider, &device1 /*out*/);
            ASSERT_NE(nullptr, device1.get());
            sp<BufferItemConsumer> bufferItemConsumer;
            sp<BufferItemHander> bufferHandler;
            setupPreviewWindow(device1, &bufferItemConsumer /*out*/, &bufferHandler /*out*/);

            {
                std::unique_lock<std::mutex> l(mLock);
                mDataMessageTypeReceived = DataCallbackMsg::RAW_IMAGE_NOTIFY;
            }

            enableMsgType((unsigned int)DataCallbackMsg::PREVIEW_FRAME, device1);
            startPreview(device1);

            {
                std::unique_lock<std::mutex> l(mLock);
                waitForFrameLocked(DataCallbackMsg::PREVIEW_FRAME, l);
            }

            disableMsgType((unsigned int)DataCallbackMsg::PREVIEW_FRAME, device1);
            enableMsgType((unsigned int)DataCallbackMsg::COMPRESSED_IMAGE, device1);

            {
                std::unique_lock<std::mutex> l(mLock);
                mDataMessageTypeReceived = DataCallbackMsg::RAW_IMAGE_NOTIFY;
            }

            Return<Status> returnStatus = device1->takePicture();
            ASSERT_TRUE(returnStatus.isOk());
            ASSERT_EQ(Status::OK, returnStatus);

            {
                std::unique_lock<std::mutex> l(mLock);
                waitForFrameLocked(DataCallbackMsg::COMPRESSED_IMAGE, l);
            }

            disableMsgType((unsigned int)DataCallbackMsg::COMPRESSED_IMAGE, device1);
            stopPreviewAndClose(device1);
        }
    }
}

// Image capture should fail in case preview didn't get enabled first.
TEST_P(CameraHidlTest, takePictureFail) {
    hidl_vec<hidl_string> cameraDeviceNames = getCameraDeviceNames(mProvider);

    for (const auto& name : cameraDeviceNames) {
        if (getCameraDeviceVersion(name, mProviderType) == CAMERA_DEVICE_API_VERSION_1_0) {
            sp<::android::hardware::camera::device::V1_0::ICameraDevice> device1;
            openCameraDevice(name, mProvider, &device1 /*out*/);
            ASSERT_NE(nullptr, device1.get());

            Return<Status> returnStatus = device1->takePicture();
            ASSERT_TRUE(returnStatus.isOk());
            ASSERT_NE(Status::OK, returnStatus);

            Return<void> ret = device1->close();
            ASSERT_TRUE(ret.isOk());
        }
    }
}

// Verify that image capture can be cancelled.
TEST_P(CameraHidlTest, cancelPicture) {
    hidl_vec<hidl_string> cameraDeviceNames = getCameraDeviceNames(mProvider);

    for (const auto& name : cameraDeviceNames) {
        if (getCameraDeviceVersion(name, mProviderType) == CAMERA_DEVICE_API_VERSION_1_0) {
            sp<::android::hardware::camera::device::V1_0::ICameraDevice> device1;
            openCameraDevice(name, mProvider, &device1 /*out*/);
            ASSERT_NE(nullptr, device1.get());
            sp<BufferItemConsumer> bufferItemConsumer;
            sp<BufferItemHander> bufferHandler;
            setupPreviewWindow(device1, &bufferItemConsumer /*out*/, &bufferHandler /*out*/);
            startPreview(device1);

            Return<Status> returnStatus = device1->takePicture();
            ASSERT_TRUE(returnStatus.isOk());
            ASSERT_EQ(Status::OK, returnStatus);

            returnStatus = device1->cancelPicture();
            ASSERT_TRUE(returnStatus.isOk());
            ASSERT_EQ(Status::OK, returnStatus);

            stopPreviewAndClose(device1);
        }
    }
}

// Image capture cancel is a no-op when image capture is not running.
TEST_P(CameraHidlTest, cancelPictureNOP) {
    hidl_vec<hidl_string> cameraDeviceNames = getCameraDeviceNames(mProvider);

    for (const auto& name : cameraDeviceNames) {
        if (getCameraDeviceVersion(name, mProviderType) == CAMERA_DEVICE_API_VERSION_1_0) {
            sp<::android::hardware::camera::device::V1_0::ICameraDevice> device1;
            openCameraDevice(name, mProvider, &device1 /*out*/);
            ASSERT_NE(nullptr, device1.get());
            sp<BufferItemConsumer> bufferItemConsumer;
            sp<BufferItemHander> bufferHandler;
            setupPreviewWindow(device1, &bufferItemConsumer /*out*/, &bufferHandler /*out*/);
            startPreview(device1);

            Return<Status> returnStatus = device1->cancelPicture();
            ASSERT_TRUE(returnStatus.isOk());
            ASSERT_EQ(Status::OK, returnStatus);

            stopPreviewAndClose(device1);
        }
    }
}

// Test basic video recording.
TEST_P(CameraHidlTest, startStopRecording) {
    hidl_vec<hidl_string> cameraDeviceNames = getCameraDeviceNames(mProvider);

    for (const auto& name : cameraDeviceNames) {
        if (getCameraDeviceVersion(name, mProviderType) == CAMERA_DEVICE_API_VERSION_1_0) {
            sp<::android::hardware::camera::device::V1_0::ICameraDevice> device1;
            openCameraDevice(name, mProvider, &device1 /*out*/);
            ASSERT_NE(nullptr, device1.get());
            sp<BufferItemConsumer> bufferItemConsumer;
            sp<BufferItemHander> bufferHandler;
            setupPreviewWindow(device1, &bufferItemConsumer /*out*/, &bufferHandler /*out*/);

            {
                std::unique_lock<std::mutex> l(mLock);
                mDataMessageTypeReceived = DataCallbackMsg::RAW_IMAGE_NOTIFY;
            }

            enableMsgType((unsigned int)DataCallbackMsg::PREVIEW_FRAME, device1);
            startPreview(device1);

            {
                std::unique_lock<std::mutex> l(mLock);
                waitForFrameLocked(DataCallbackMsg::PREVIEW_FRAME, l);
                mDataMessageTypeReceived = DataCallbackMsg::RAW_IMAGE_NOTIFY;
                mVideoBufferIndex = UINT32_MAX;
            }

            disableMsgType((unsigned int)DataCallbackMsg::PREVIEW_FRAME, device1);

            bool videoMetaEnabled = false;
            Return<Status> returnStatus = device1->storeMetaDataInBuffers(true);
            ASSERT_TRUE(returnStatus.isOk());
            // It is allowed for devices to not support this feature
            ASSERT_TRUE((Status::OK == returnStatus) ||
                        (Status::OPERATION_NOT_SUPPORTED == returnStatus));
            if (Status::OK == returnStatus) {
                videoMetaEnabled = true;
            }

            enableMsgType((unsigned int)DataCallbackMsg::VIDEO_FRAME, device1);
            Return<bool> returnBoolStatus = device1->recordingEnabled();
            ASSERT_TRUE(returnBoolStatus.isOk());
            ASSERT_FALSE(returnBoolStatus);

            returnStatus = device1->startRecording();
            ASSERT_TRUE(returnStatus.isOk());
            ASSERT_EQ(Status::OK, returnStatus);

            {
                std::unique_lock<std::mutex> l(mLock);
                waitForFrameLocked(DataCallbackMsg::VIDEO_FRAME, l);
                ASSERT_NE(UINT32_MAX, mVideoBufferIndex);
                disableMsgType((unsigned int)DataCallbackMsg::VIDEO_FRAME, device1);
            }

            returnBoolStatus = device1->recordingEnabled();
            ASSERT_TRUE(returnBoolStatus.isOk());
            ASSERT_TRUE(returnBoolStatus);

            Return<void> ret;
            if (videoMetaEnabled) {
                ret = device1->releaseRecordingFrameHandle(mVideoData, mVideoBufferIndex,
                                                           mVideoNativeHandle);
                ASSERT_TRUE(ret.isOk());
            } else {
                ret = device1->releaseRecordingFrame(mVideoData, mVideoBufferIndex);
                ASSERT_TRUE(ret.isOk());
            }

            ret = device1->stopRecording();
            ASSERT_TRUE(ret.isOk());

            stopPreviewAndClose(device1);
        }
    }
}

// It shouldn't be possible to start recording without enabling preview first.
TEST_P(CameraHidlTest, startRecordingFail) {
    hidl_vec<hidl_string> cameraDeviceNames = getCameraDeviceNames(mProvider);

    for (const auto& name : cameraDeviceNames) {
        if (getCameraDeviceVersion(name, mProviderType) == CAMERA_DEVICE_API_VERSION_1_0) {
            sp<::android::hardware::camera::device::V1_0::ICameraDevice> device1;
            openCameraDevice(name, mProvider, &device1 /*out*/);
            ASSERT_NE(nullptr, device1.get());

            Return<bool> returnBoolStatus = device1->recordingEnabled();
            ASSERT_TRUE(returnBoolStatus.isOk());
            ASSERT_FALSE(returnBoolStatus);

            Return<Status> returnStatus = device1->startRecording();
            ASSERT_TRUE(returnStatus.isOk());
            ASSERT_NE(Status::OK, returnStatus);

            Return<void> ret = device1->close();
            ASSERT_TRUE(ret.isOk());
        }
    }
}

// Check autofocus support if available.
TEST_P(CameraHidlTest, autoFocus) {
    hidl_vec<hidl_string> cameraDeviceNames = getCameraDeviceNames(mProvider);
    std::vector<const char*> focusModes = {CameraParameters::FOCUS_MODE_AUTO,
                                           CameraParameters::FOCUS_MODE_CONTINUOUS_PICTURE,
                                           CameraParameters::FOCUS_MODE_CONTINUOUS_VIDEO};

    for (const auto& name : cameraDeviceNames) {
        if (getCameraDeviceVersion(name, mProviderType) == CAMERA_DEVICE_API_VERSION_1_0) {
            sp<::android::hardware::camera::device::V1_0::ICameraDevice> device1;
            openCameraDevice(name, mProvider, &device1 /*out*/);
            ASSERT_NE(nullptr, device1.get());

            CameraParameters cameraParams;
            getParameters(device1, &cameraParams /*out*/);

            if (Status::OK !=
                isAutoFocusModeAvailable(cameraParams, CameraParameters::FOCUS_MODE_AUTO)) {
                Return<void> ret = device1->close();
                ASSERT_TRUE(ret.isOk());
                continue;
            }

            sp<BufferItemConsumer> bufferItemConsumer;
            sp<BufferItemHander> bufferHandler;
            setupPreviewWindow(device1, &bufferItemConsumer /*out*/, &bufferHandler /*out*/);
            startPreview(device1);
            enableMsgType((unsigned int)NotifyCallbackMsg::FOCUS, device1);

            for (auto& iter : focusModes) {
                if (Status::OK != isAutoFocusModeAvailable(cameraParams, iter)) {
                    continue;
                }

                cameraParams.set(CameraParameters::KEY_FOCUS_MODE, iter);
                setParameters(device1, cameraParams);
                {
                    std::unique_lock<std::mutex> l(mLock);
                    mNotifyMessage = NotifyCallbackMsg::ERROR;
                }

                Return<Status> returnStatus = device1->autoFocus();
                ASSERT_TRUE(returnStatus.isOk());
                ASSERT_EQ(Status::OK, returnStatus);

                {
                    std::unique_lock<std::mutex> l(mLock);
                    while (NotifyCallbackMsg::FOCUS != mNotifyMessage) {
                        auto timeout = std::chrono::system_clock::now() +
                                       std::chrono::seconds(kAutoFocusTimeoutSec);
                        ASSERT_NE(std::cv_status::timeout, mResultCondition.wait_until(l, timeout));
                    }
                }
            }

            disableMsgType((unsigned int)NotifyCallbackMsg::FOCUS, device1);
            stopPreviewAndClose(device1);
        }
    }
}

// In case autofocus is supported verify that it can be cancelled.
TEST_P(CameraHidlTest, cancelAutoFocus) {
    hidl_vec<hidl_string> cameraDeviceNames = getCameraDeviceNames(mProvider);

    for (const auto& name : cameraDeviceNames) {
        if (getCameraDeviceVersion(name, mProviderType) == CAMERA_DEVICE_API_VERSION_1_0) {
            sp<::android::hardware::camera::device::V1_0::ICameraDevice> device1;
            openCameraDevice(name, mProvider, &device1 /*out*/);
            ASSERT_NE(nullptr, device1.get());

            CameraParameters cameraParams;
            getParameters(device1, &cameraParams /*out*/);

            if (Status::OK !=
                isAutoFocusModeAvailable(cameraParams, CameraParameters::FOCUS_MODE_AUTO)) {
                Return<void> ret = device1->close();
                ASSERT_TRUE(ret.isOk());
                continue;
            }

            // It should be fine to call before preview starts.
            ASSERT_EQ(Status::OK, device1->cancelAutoFocus());

            sp<BufferItemConsumer> bufferItemConsumer;
            sp<BufferItemHander> bufferHandler;
            setupPreviewWindow(device1, &bufferItemConsumer /*out*/, &bufferHandler /*out*/);
            startPreview(device1);

            // It should be fine to call after preview starts too.
            Return<Status> returnStatus = device1->cancelAutoFocus();
            ASSERT_TRUE(returnStatus.isOk());
            ASSERT_EQ(Status::OK, returnStatus);

            returnStatus = device1->autoFocus();
            ASSERT_TRUE(returnStatus.isOk());
            ASSERT_EQ(Status::OK, returnStatus);

            returnStatus = device1->cancelAutoFocus();
            ASSERT_TRUE(returnStatus.isOk());
            ASSERT_EQ(Status::OK, returnStatus);

            stopPreviewAndClose(device1);
        }
    }
}

// Check whether face detection is available and try to enable&disable.
TEST_P(CameraHidlTest, sendCommandFaceDetection) {
    hidl_vec<hidl_string> cameraDeviceNames = getCameraDeviceNames(mProvider);

    for (const auto& name : cameraDeviceNames) {
        if (getCameraDeviceVersion(name, mProviderType) == CAMERA_DEVICE_API_VERSION_1_0) {
            sp<::android::hardware::camera::device::V1_0::ICameraDevice> device1;
            openCameraDevice(name, mProvider, &device1 /*out*/);
            ASSERT_NE(nullptr, device1.get());

            CameraParameters cameraParams;
            getParameters(device1, &cameraParams /*out*/);

            int32_t hwFaces = cameraParams.getInt(CameraParameters::KEY_MAX_NUM_DETECTED_FACES_HW);
            int32_t swFaces = cameraParams.getInt(CameraParameters::KEY_MAX_NUM_DETECTED_FACES_SW);
            if ((0 >= hwFaces) && (0 >= swFaces)) {
                Return<void> ret = device1->close();
                ASSERT_TRUE(ret.isOk());
                continue;
            }

            sp<BufferItemConsumer> bufferItemConsumer;
            sp<BufferItemHander> bufferHandler;
            setupPreviewWindow(device1, &bufferItemConsumer /*out*/, &bufferHandler /*out*/);
            startPreview(device1);

            if (0 < hwFaces) {
                Return<Status> returnStatus = device1->sendCommand(
                    CommandType::START_FACE_DETECTION, CAMERA_FACE_DETECTION_HW, 0);
                ASSERT_TRUE(returnStatus.isOk());
                ASSERT_EQ(Status::OK, returnStatus);
                // TODO(epeev) : Enable and check for face notifications
                returnStatus = device1->sendCommand(CommandType::STOP_FACE_DETECTION,
                                                    CAMERA_FACE_DETECTION_HW, 0);
                ASSERT_TRUE(returnStatus.isOk());
                ASSERT_EQ(Status::OK, returnStatus);
            }

            if (0 < swFaces) {
                Return<Status> returnStatus = device1->sendCommand(
                    CommandType::START_FACE_DETECTION, CAMERA_FACE_DETECTION_SW, 0);
                ASSERT_TRUE(returnStatus.isOk());
                ASSERT_EQ(Status::OK, returnStatus);
                // TODO(epeev) : Enable and check for face notifications
                returnStatus = device1->sendCommand(CommandType::STOP_FACE_DETECTION,
                                                    CAMERA_FACE_DETECTION_SW, 0);
                ASSERT_TRUE(returnStatus.isOk());
                ASSERT_EQ(Status::OK, returnStatus);
            }

            stopPreviewAndClose(device1);
        }
    }
}

// Check whether smooth zoom is available and try to enable&disable.
TEST_P(CameraHidlTest, sendCommandSmoothZoom) {
    hidl_vec<hidl_string> cameraDeviceNames = getCameraDeviceNames(mProvider);

    for (const auto& name : cameraDeviceNames) {
        if (getCameraDeviceVersion(name, mProviderType) == CAMERA_DEVICE_API_VERSION_1_0) {
            sp<::android::hardware::camera::device::V1_0::ICameraDevice> device1;
            openCameraDevice(name, mProvider, &device1 /*out*/);
            ASSERT_NE(nullptr, device1.get());

            CameraParameters cameraParams;
            getParameters(device1, &cameraParams /*out*/);

            const char* smoothZoomStr =
                cameraParams.get(CameraParameters::KEY_SMOOTH_ZOOM_SUPPORTED);
            bool smoothZoomSupported =
                ((nullptr != smoothZoomStr) && (strcmp(smoothZoomStr, CameraParameters::TRUE) == 0))
                    ? true
                    : false;
            if (!smoothZoomSupported) {
                Return<void> ret = device1->close();
                ASSERT_TRUE(ret.isOk());
                continue;
            }

            int32_t maxZoom = cameraParams.getInt(CameraParameters::KEY_MAX_ZOOM);
            ASSERT_TRUE(0 < maxZoom);

            sp<BufferItemConsumer> bufferItemConsumer;
            sp<BufferItemHander> bufferHandler;
            setupPreviewWindow(device1, &bufferItemConsumer /*out*/, &bufferHandler /*out*/);
            startPreview(device1);
            setParameters(device1, cameraParams);

            Return<Status> returnStatus =
                device1->sendCommand(CommandType::START_SMOOTH_ZOOM, maxZoom, 0);
            ASSERT_TRUE(returnStatus.isOk());
            ASSERT_EQ(Status::OK, returnStatus);
            // TODO(epeev) : Enable and check for face notifications
            returnStatus = device1->sendCommand(CommandType::STOP_SMOOTH_ZOOM, 0, 0);
            ASSERT_TRUE(returnStatus.isOk());
            ASSERT_EQ(Status::OK, returnStatus);

            stopPreviewAndClose(device1);
        }
    }
}

// Basic sanity tests related to camera parameters.
TEST_P(CameraHidlTest, getSetParameters) {
    hidl_vec<hidl_string> cameraDeviceNames = getCameraDeviceNames(mProvider);

    for (const auto& name : cameraDeviceNames) {
        if (getCameraDeviceVersion(name, mProviderType) == CAMERA_DEVICE_API_VERSION_1_0) {
            sp<::android::hardware::camera::device::V1_0::ICameraDevice> device1;
            openCameraDevice(name, mProvider, &device1 /*out*/);
            ASSERT_NE(nullptr, device1.get());

            CameraParameters cameraParams;
            getParameters(device1, &cameraParams /*out*/);

            int32_t width, height;
            cameraParams.getPictureSize(&width, &height);
            ASSERT_TRUE((0 < width) && (0 < height));
            cameraParams.getPreviewSize(&width, &height);
            ASSERT_TRUE((0 < width) && (0 < height));
            int32_t minFps, maxFps;
            cameraParams.getPreviewFpsRange(&minFps, &maxFps);
            ASSERT_TRUE((0 < minFps) && (0 < maxFps));
            ASSERT_NE(nullptr, cameraParams.getPreviewFormat());
            ASSERT_NE(nullptr, cameraParams.getPictureFormat());
            ASSERT_TRUE(
                strcmp(CameraParameters::PIXEL_FORMAT_JPEG, cameraParams.getPictureFormat()) == 0);

            const char* flashMode = cameraParams.get(CameraParameters::KEY_FLASH_MODE);
            ASSERT_TRUE((nullptr == flashMode) ||
                        (strcmp(CameraParameters::FLASH_MODE_OFF, flashMode) == 0));

            const char* wbMode = cameraParams.get(CameraParameters::KEY_WHITE_BALANCE);
            ASSERT_TRUE((nullptr == wbMode) ||
                        (strcmp(CameraParameters::WHITE_BALANCE_AUTO, wbMode) == 0));

            const char* effect = cameraParams.get(CameraParameters::KEY_EFFECT);
            ASSERT_TRUE((nullptr == effect) ||
                        (strcmp(CameraParameters::EFFECT_NONE, effect) == 0));

            ::android::Vector<Size> previewSizes;
            cameraParams.getSupportedPreviewSizes(previewSizes);
            ASSERT_FALSE(previewSizes.empty());
            ::android::Vector<Size> pictureSizes;
            cameraParams.getSupportedPictureSizes(pictureSizes);
            ASSERT_FALSE(pictureSizes.empty());
            const char* previewFormats =
                cameraParams.get(CameraParameters::KEY_SUPPORTED_PREVIEW_FORMATS);
            ASSERT_NE(nullptr, previewFormats);
            ::android::String8 previewFormatsString(previewFormats);
            ASSERT_TRUE(previewFormatsString.contains(CameraParameters::PIXEL_FORMAT_YUV420SP));
            ASSERT_NE(nullptr, cameraParams.get(CameraParameters::KEY_SUPPORTED_PICTURE_FORMATS));
            ASSERT_NE(nullptr,
                      cameraParams.get(CameraParameters::KEY_SUPPORTED_PREVIEW_FRAME_RATES));
            const char* focusModes = cameraParams.get(CameraParameters::KEY_SUPPORTED_FOCUS_MODES);
            ASSERT_NE(nullptr, focusModes);
            ::android::String8 focusModesString(focusModes);
            const char* focusMode = cameraParams.get(CameraParameters::KEY_FOCUS_MODE);
            ASSERT_NE(nullptr, focusMode);
            // Auto focus mode should be default
            if (focusModesString.contains(CameraParameters::FOCUS_MODE_AUTO)) {
                ASSERT_TRUE(strcmp(CameraParameters::FOCUS_MODE_AUTO, focusMode) == 0);
            }
            ASSERT_TRUE(0 < cameraParams.getInt(CameraParameters::KEY_FOCAL_LENGTH));
            int32_t horizontalViewAngle =
                cameraParams.getInt(CameraParameters::KEY_HORIZONTAL_VIEW_ANGLE);
            ASSERT_TRUE((0 < horizontalViewAngle) && (360 >= horizontalViewAngle));
            int32_t verticalViewAngle =
                cameraParams.getInt(CameraParameters::KEY_VERTICAL_VIEW_ANGLE);
            ASSERT_TRUE((0 < verticalViewAngle) && (360 >= verticalViewAngle));
            int32_t jpegQuality = cameraParams.getInt(CameraParameters::KEY_JPEG_QUALITY);
            ASSERT_TRUE((1 <= jpegQuality) && (100 >= jpegQuality));
            int32_t jpegThumbQuality =
                cameraParams.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_QUALITY);
            ASSERT_TRUE((1 <= jpegThumbQuality) && (100 >= jpegThumbQuality));

            cameraParams.setPictureSize(pictureSizes[0].width, pictureSizes[0].height);
            cameraParams.setPreviewSize(previewSizes[0].width, previewSizes[0].height);

            setParameters(device1, cameraParams);
            getParameters(device1, &cameraParams /*out*/);

            cameraParams.getPictureSize(&width, &height);
            ASSERT_TRUE((pictureSizes[0].width == width) && (pictureSizes[0].height == height));
            cameraParams.getPreviewSize(&width, &height);
            ASSERT_TRUE((previewSizes[0].width == width) && (previewSizes[0].height == height));

            Return<void> ret = device1->close();
            ASSERT_TRUE(ret.isOk());
        }
    }
}

// Verify that the static camera characteristics can be retrieved
// successfully.
TEST_P(CameraHidlTest, getCameraCharacteristics) {
    hidl_vec<hidl_string> cameraDeviceNames = getCameraDeviceNames(mProvider);

    for (const auto& name : cameraDeviceNames) {
        int deviceVersion = getCameraDeviceVersion(name, mProviderType);
        switch (deviceVersion) {
            case CAMERA_DEVICE_API_VERSION_3_6:
            case CAMERA_DEVICE_API_VERSION_3_5:
            case CAMERA_DEVICE_API_VERSION_3_4:
            case CAMERA_DEVICE_API_VERSION_3_3:
            case CAMERA_DEVICE_API_VERSION_3_2: {
                ::android::sp<::android::hardware::camera::device::V3_2::ICameraDevice> device3_x;
                ALOGI("getCameraCharacteristics: Testing camera device %s", name.c_str());
                Return<void> ret;
                ret = mProvider->getCameraDeviceInterface_V3_x(
                    name, [&](auto status, const auto& device) {
                        ALOGI("getCameraDeviceInterface_V3_x returns status:%d", (int)status);
                        ASSERT_EQ(Status::OK, status);
                        ASSERT_NE(device, nullptr);
                        device3_x = device;
                    });
                ASSERT_TRUE(ret.isOk());

                ret = device3_x->getCameraCharacteristics([&](auto status, const auto& chars) {
                    verifyCameraCharacteristics(status, chars);
                    verifyMonochromeCharacteristics(chars, deviceVersion);
                    verifyRecommendedConfigs(chars);
                    verifyLogicalCameraMetadata(name, device3_x, chars, deviceVersion,
                            cameraDeviceNames);
                });
                ASSERT_TRUE(ret.isOk());

                //getPhysicalCameraCharacteristics will fail for publicly
                //advertised camera IDs.
                if (deviceVersion >= CAMERA_DEVICE_API_VERSION_3_5) {
                    auto castResult = device::V3_5::ICameraDevice::castFrom(device3_x);
                    ASSERT_TRUE(castResult.isOk());
                    ::android::sp<::android::hardware::camera::device::V3_5::ICameraDevice>
                            device3_5 = castResult;
                    ASSERT_NE(device3_5, nullptr);

                    std::string version, cameraId;
                    ASSERT_TRUE(::matchDeviceName(name, mProviderType, &version, &cameraId));
                    Return<void> ret = device3_5->getPhysicalCameraCharacteristics(cameraId,
                            [&](auto status, const auto& chars) {
                        ASSERT_TRUE(Status::ILLEGAL_ARGUMENT == status);
                        ASSERT_EQ(0, chars.size());
                    });
                    ASSERT_TRUE(ret.isOk());
                }
            }
            break;
            case CAMERA_DEVICE_API_VERSION_1_0: {
                //Not applicable
            }
            break;
            default: {
                ALOGE("%s: Unsupported device version %d", __func__, deviceVersion);
                ADD_FAILURE();
            }
            break;
        }
    }
}

//In case it is supported verify that torch can be enabled.
//Check for corresponding toch callbacks as well.
TEST_P(CameraHidlTest, setTorchMode) {
    hidl_vec<hidl_string> cameraDeviceNames = getCameraDeviceNames(mProvider);
    bool torchControlSupported = false;
    Return<void> ret;

    ret = mProvider->isSetTorchModeSupported([&](auto status, bool support) {
        ALOGI("isSetTorchModeSupported returns status:%d supported:%d", (int)status, support);
        ASSERT_EQ(Status::OK, status);
        torchControlSupported = support;
    });

    sp<TorchProviderCb> cb = new TorchProviderCb(this);
    Return<Status> returnStatus = mProvider->setCallback(cb);
    ASSERT_TRUE(returnStatus.isOk());
    ASSERT_EQ(Status::OK, returnStatus);

    for (const auto& name : cameraDeviceNames) {
        int deviceVersion = getCameraDeviceVersion(name, mProviderType);
        switch (deviceVersion) {
            case CAMERA_DEVICE_API_VERSION_3_6:
            case CAMERA_DEVICE_API_VERSION_3_5:
            case CAMERA_DEVICE_API_VERSION_3_4:
            case CAMERA_DEVICE_API_VERSION_3_3:
            case CAMERA_DEVICE_API_VERSION_3_2: {
                ::android::sp<::android::hardware::camera::device::V3_2::ICameraDevice> device3_x;
                ALOGI("setTorchMode: Testing camera device %s", name.c_str());
                ret = mProvider->getCameraDeviceInterface_V3_x(
                    name, [&](auto status, const auto& device) {
                        ALOGI("getCameraDeviceInterface_V3_x returns status:%d", (int)status);
                        ASSERT_EQ(Status::OK, status);
                        ASSERT_NE(device, nullptr);
                        device3_x = device;
                    });
                ASSERT_TRUE(ret.isOk());

                mTorchStatus = TorchModeStatus::NOT_AVAILABLE;
                returnStatus = device3_x->setTorchMode(TorchMode::ON);
                ASSERT_TRUE(returnStatus.isOk());
                if (!torchControlSupported) {
                    ASSERT_EQ(Status::METHOD_NOT_SUPPORTED, returnStatus);
                } else {
                    ASSERT_TRUE(returnStatus == Status::OK ||
                                returnStatus == Status::OPERATION_NOT_SUPPORTED);
                    if (returnStatus == Status::OK) {
                        {
                            std::unique_lock<std::mutex> l(mTorchLock);
                            while (TorchModeStatus::NOT_AVAILABLE == mTorchStatus) {
                                auto timeout = std::chrono::system_clock::now() +
                                               std::chrono::seconds(kTorchTimeoutSec);
                                ASSERT_NE(std::cv_status::timeout, mTorchCond.wait_until(l, timeout));
                            }
                            ASSERT_EQ(TorchModeStatus::AVAILABLE_ON, mTorchStatus);
                            mTorchStatus = TorchModeStatus::NOT_AVAILABLE;
                        }

                        returnStatus = device3_x->setTorchMode(TorchMode::OFF);
                        ASSERT_TRUE(returnStatus.isOk());
                        ASSERT_EQ(Status::OK, returnStatus);

                        {
                            std::unique_lock<std::mutex> l(mTorchLock);
                            while (TorchModeStatus::NOT_AVAILABLE == mTorchStatus) {
                                auto timeout = std::chrono::system_clock::now() +
                                               std::chrono::seconds(kTorchTimeoutSec);
                                ASSERT_NE(std::cv_status::timeout, mTorchCond.wait_until(l, timeout));
                            }
                            ASSERT_EQ(TorchModeStatus::AVAILABLE_OFF, mTorchStatus);
                        }
                    }
                }
            }
            break;
            case CAMERA_DEVICE_API_VERSION_1_0: {
                ::android::sp<::android::hardware::camera::device::V1_0::ICameraDevice> device1;
                ALOGI("dumpState: Testing camera device %s", name.c_str());
                ret = mProvider->getCameraDeviceInterface_V1_x(
                    name, [&](auto status, const auto& device) {
                        ALOGI("getCameraDeviceInterface_V1_x returns status:%d", (int)status);
                        ASSERT_EQ(Status::OK, status);
                        ASSERT_NE(device, nullptr);
                        device1 = device;
                    });
                ASSERT_TRUE(ret.isOk());

                mTorchStatus = TorchModeStatus::NOT_AVAILABLE;
                returnStatus = device1->setTorchMode(TorchMode::ON);
                ASSERT_TRUE(returnStatus.isOk());
                if (!torchControlSupported) {
                    ASSERT_EQ(Status::METHOD_NOT_SUPPORTED, returnStatus);
                } else {
                    ASSERT_TRUE(returnStatus == Status::OK ||
                                returnStatus == Status::OPERATION_NOT_SUPPORTED);
                    if (returnStatus == Status::OK) {
                        {
                            std::unique_lock<std::mutex> l(mTorchLock);
                            while (TorchModeStatus::NOT_AVAILABLE == mTorchStatus) {
                                auto timeout = std::chrono::system_clock::now() +
                                               std::chrono::seconds(kTorchTimeoutSec);
                                ASSERT_NE(std::cv_status::timeout, mTorchCond.wait_until(l,
                                        timeout));
                            }
                            ASSERT_EQ(TorchModeStatus::AVAILABLE_ON, mTorchStatus);
                            mTorchStatus = TorchModeStatus::NOT_AVAILABLE;
                        }

                        returnStatus = device1->setTorchMode(TorchMode::OFF);
                        ASSERT_TRUE(returnStatus.isOk());
                        ASSERT_EQ(Status::OK, returnStatus);

                        {
                            std::unique_lock<std::mutex> l(mTorchLock);
                            while (TorchModeStatus::NOT_AVAILABLE == mTorchStatus) {
                                auto timeout = std::chrono::system_clock::now() +
                                               std::chrono::seconds(kTorchTimeoutSec);
                                ASSERT_NE(std::cv_status::timeout, mTorchCond.wait_until(l,
                                        timeout));
                            }
                            ASSERT_EQ(TorchModeStatus::AVAILABLE_OFF, mTorchStatus);
                        }
                    }
                }
                ret = device1->close();
                ASSERT_TRUE(ret.isOk());
            }
            break;
            default: {
                ALOGE("%s: Unsupported device version %d", __func__, deviceVersion);
                ADD_FAILURE();
            }
            break;
        }
    }

    returnStatus = mProvider->setCallback(nullptr);
    ASSERT_TRUE(returnStatus.isOk());
    ASSERT_EQ(Status::OK, returnStatus);
}

// Check dump functionality.
TEST_P(CameraHidlTest, dumpState) {
    hidl_vec<hidl_string> cameraDeviceNames = getCameraDeviceNames(mProvider);
    Return<void> ret;

    for (const auto& name : cameraDeviceNames) {
        int deviceVersion = getCameraDeviceVersion(name, mProviderType);
        switch (deviceVersion) {
            case CAMERA_DEVICE_API_VERSION_3_6:
            case CAMERA_DEVICE_API_VERSION_3_5:
            case CAMERA_DEVICE_API_VERSION_3_4:
            case CAMERA_DEVICE_API_VERSION_3_3:
            case CAMERA_DEVICE_API_VERSION_3_2: {
                ::android::sp<ICameraDevice> device3_x;
                ALOGI("dumpState: Testing camera device %s", name.c_str());
                ret = mProvider->getCameraDeviceInterface_V3_x(
                    name, [&](auto status, const auto& device) {
                        ALOGI("getCameraDeviceInterface_V3_x returns status:%d", (int)status);
                        ASSERT_EQ(Status::OK, status);
                        ASSERT_NE(device, nullptr);
                        device3_x = device;
                    });
                ASSERT_TRUE(ret.isOk());

                native_handle_t* raw_handle = native_handle_create(1, 0);
                raw_handle->data[0] = open(kDumpOutput, O_RDWR);
                ASSERT_GE(raw_handle->data[0], 0);
                hidl_handle handle = raw_handle;
                ret = device3_x->dumpState(handle);
                ASSERT_TRUE(ret.isOk());
                close(raw_handle->data[0]);
                native_handle_delete(raw_handle);
            }
            break;
            case CAMERA_DEVICE_API_VERSION_1_0: {
                ::android::sp<::android::hardware::camera::device::V1_0::ICameraDevice> device1;
                ALOGI("dumpState: Testing camera device %s", name.c_str());
                ret = mProvider->getCameraDeviceInterface_V1_x(
                    name, [&](auto status, const auto& device) {
                        ALOGI("getCameraDeviceInterface_V1_x returns status:%d", (int)status);
                        ASSERT_EQ(Status::OK, status);
                        ASSERT_NE(device, nullptr);
                        device1 = device;
                    });
                ASSERT_TRUE(ret.isOk());

                native_handle_t* raw_handle = native_handle_create(1, 0);
                raw_handle->data[0] = open(kDumpOutput, O_RDWR);
                ASSERT_GE(raw_handle->data[0], 0);
                hidl_handle handle = raw_handle;
                Return<Status> returnStatus = device1->dumpState(handle);
                ASSERT_TRUE(returnStatus.isOk());
                ASSERT_EQ(Status::OK, returnStatus);
                close(raw_handle->data[0]);
                native_handle_delete(raw_handle);
            }
            break;
            default: {
                ALOGE("%s: Unsupported device version %d", __func__, deviceVersion);
                ADD_FAILURE();
            }
            break;
        }
    }
}

// Open, dumpStates, then close
TEST_P(CameraHidlTest, openClose) {
    hidl_vec<hidl_string> cameraDeviceNames = getCameraDeviceNames(mProvider);
    Return<void> ret;

    for (const auto& name : cameraDeviceNames) {
        int deviceVersion = getCameraDeviceVersion(name, mProviderType);
        switch (deviceVersion) {
            case CAMERA_DEVICE_API_VERSION_3_6:
            case CAMERA_DEVICE_API_VERSION_3_5:
            case CAMERA_DEVICE_API_VERSION_3_4:
            case CAMERA_DEVICE_API_VERSION_3_3:
            case CAMERA_DEVICE_API_VERSION_3_2: {
                ::android::sp<::android::hardware::camera::device::V3_2::ICameraDevice> device3_x;
                ALOGI("openClose: Testing camera device %s", name.c_str());
                ret = mProvider->getCameraDeviceInterface_V3_x(
                    name, [&](auto status, const auto& device) {
                        ALOGI("getCameraDeviceInterface_V3_x returns status:%d", (int)status);
                        ASSERT_EQ(Status::OK, status);
                        ASSERT_NE(device, nullptr);
                        device3_x = device;
                    });
                ASSERT_TRUE(ret.isOk());

                sp<EmptyDeviceCb> cb = new EmptyDeviceCb;
                sp<ICameraDeviceSession> session;
                ret = device3_x->open(cb, [&](auto status, const auto& newSession) {
                    ALOGI("device::open returns status:%d", (int)status);
                    ASSERT_EQ(Status::OK, status);
                    ASSERT_NE(newSession, nullptr);
                    session = newSession;
                });
                ASSERT_TRUE(ret.isOk());
                // Ensure that a device labeling itself as 3.3/3.4 can have its session interface
                // cast the 3.3/3.4 interface, and that lower versions can't be cast to it.
                sp<device::V3_3::ICameraDeviceSession> sessionV3_3;
                sp<device::V3_4::ICameraDeviceSession> sessionV3_4;
                sp<device::V3_5::ICameraDeviceSession> sessionV3_5;
                sp<device::V3_6::ICameraDeviceSession> sessionV3_6;
                castSession(session, deviceVersion, &sessionV3_3,
                        &sessionV3_4, &sessionV3_5, &sessionV3_6);
                if (deviceVersion == CAMERA_DEVICE_API_VERSION_3_6) {
                    ASSERT_TRUE(sessionV3_6.get() != nullptr);
                } else if (deviceVersion == CAMERA_DEVICE_API_VERSION_3_5) {
                    ASSERT_TRUE(sessionV3_5.get() != nullptr);
                } else if (deviceVersion == CAMERA_DEVICE_API_VERSION_3_4) {
                    ASSERT_TRUE(sessionV3_4.get() != nullptr);
                } else if (deviceVersion == CAMERA_DEVICE_API_VERSION_3_3) {
                    ASSERT_TRUE(sessionV3_3.get() != nullptr);
                } else { //V3_2
                    ASSERT_TRUE(sessionV3_3.get() == nullptr);
                    ASSERT_TRUE(sessionV3_4.get() == nullptr);
                    ASSERT_TRUE(sessionV3_5.get() == nullptr);
                }
                native_handle_t* raw_handle = native_handle_create(1, 0);
                raw_handle->data[0] = open(kDumpOutput, O_RDWR);
                ASSERT_GE(raw_handle->data[0], 0);
                hidl_handle handle = raw_handle;
                ret = device3_x->dumpState(handle);
                ASSERT_TRUE(ret.isOk());
                close(raw_handle->data[0]);
                native_handle_delete(raw_handle);

                ret = session->close();
                ASSERT_TRUE(ret.isOk());
                // TODO: test all session API calls return INTERNAL_ERROR after close
                // TODO: keep a wp copy here and verify session cannot be promoted out of this scope
            }
            break;
            case CAMERA_DEVICE_API_VERSION_1_0: {
                sp<::android::hardware::camera::device::V1_0::ICameraDevice> device1;
                openCameraDevice(name, mProvider, &device1 /*out*/);
                ASSERT_NE(nullptr, device1.get());

                native_handle_t* raw_handle = native_handle_create(1, 0);
                raw_handle->data[0] = open(kDumpOutput, O_RDWR);
                ASSERT_GE(raw_handle->data[0], 0);
                hidl_handle handle = raw_handle;
                Return<Status> returnStatus = device1->dumpState(handle);
                ASSERT_TRUE(returnStatus.isOk());
                ASSERT_EQ(Status::OK, returnStatus);
                close(raw_handle->data[0]);
                native_handle_delete(raw_handle);

                ret = device1->close();
                ASSERT_TRUE(ret.isOk());
            }
            break;
            default: {
                ALOGE("%s: Unsupported device version %d", __func__, deviceVersion);
                ADD_FAILURE();
            }
            break;
        }
    }
}

// Check whether all common default request settings can be sucessfully
// constructed.
TEST_P(CameraHidlTest, constructDefaultRequestSettings) {
    hidl_vec<hidl_string> cameraDeviceNames = getCameraDeviceNames(mProvider);

    for (const auto& name : cameraDeviceNames) {
        int deviceVersion = getCameraDeviceVersion(name, mProviderType);
        switch (deviceVersion) {
            case CAMERA_DEVICE_API_VERSION_3_6:
            case CAMERA_DEVICE_API_VERSION_3_5:
            case CAMERA_DEVICE_API_VERSION_3_4:
            case CAMERA_DEVICE_API_VERSION_3_3:
            case CAMERA_DEVICE_API_VERSION_3_2: {
                ::android::sp<::android::hardware::camera::device::V3_2::ICameraDevice> device3_x;
                Return<void> ret;
                ALOGI("constructDefaultRequestSettings: Testing camera device %s", name.c_str());
                ret = mProvider->getCameraDeviceInterface_V3_x(
                    name, [&](auto status, const auto& device) {
                        ALOGI("getCameraDeviceInterface_V3_x returns status:%d", (int)status);
                        ASSERT_EQ(Status::OK, status);
                        ASSERT_NE(device, nullptr);
                        device3_x = device;
                    });
                ASSERT_TRUE(ret.isOk());

                sp<EmptyDeviceCb> cb = new EmptyDeviceCb;
                sp<ICameraDeviceSession> session;
                ret = device3_x->open(cb, [&](auto status, const auto& newSession) {
                    ALOGI("device::open returns status:%d", (int)status);
                    ASSERT_EQ(Status::OK, status);
                    ASSERT_NE(newSession, nullptr);
                    session = newSession;
                });
                ASSERT_TRUE(ret.isOk());

                for (uint32_t t = (uint32_t)RequestTemplate::PREVIEW;
                     t <= (uint32_t)RequestTemplate::MANUAL; t++) {
                    RequestTemplate reqTemplate = (RequestTemplate)t;
                    ret =
                        session->constructDefaultRequestSettings(
                            reqTemplate, [&](auto status, const auto& req) {
                                ALOGI("constructDefaultRequestSettings returns status:%d",
                                      (int)status);
                                if (reqTemplate == RequestTemplate::ZERO_SHUTTER_LAG ||
                                        reqTemplate == RequestTemplate::MANUAL) {
                                    // optional templates
                                    ASSERT_TRUE((status == Status::OK) ||
                                            (status == Status::ILLEGAL_ARGUMENT));
                                } else {
                                    ASSERT_EQ(Status::OK, status);
                                }

                                if (status == Status::OK) {
                                    const camera_metadata_t* metadata =
                                        (camera_metadata_t*) req.data();
                                    size_t expectedSize = req.size();
                                    int result = validate_camera_metadata_structure(
                                            metadata, &expectedSize);
                                    ASSERT_TRUE((result == 0) ||
                                            (result == CAMERA_METADATA_VALIDATION_SHIFTED));
                                    verifyRequestTemplate(metadata, reqTemplate);
                                } else {
                                    ASSERT_EQ(0u, req.size());
                                }
                            });
                    ASSERT_TRUE(ret.isOk());
                }
                ret = session->close();
                ASSERT_TRUE(ret.isOk());
            }
            break;
            case CAMERA_DEVICE_API_VERSION_1_0: {
                //Not applicable
            }
            break;
            default: {
                ALOGE("%s: Unsupported device version %d", __func__, deviceVersion);
                ADD_FAILURE();
            }
            break;
        }
    }
}


// Verify that all supported stream formats and sizes can be configured
// successfully.
TEST_P(CameraHidlTest, configureStreamsAvailableOutputs) {
    hidl_vec<hidl_string> cameraDeviceNames = getCameraDeviceNames(mProvider);
    std::vector<AvailableStream> outputStreams;

    for (const auto& name : cameraDeviceNames) {
        int deviceVersion = getCameraDeviceVersion(name, mProviderType);
        if (deviceVersion == CAMERA_DEVICE_API_VERSION_1_0) {
            continue;
        } else if (deviceVersion <= 0) {
            ALOGE("%s: Unsupported device version %d", __func__, deviceVersion);
            ADD_FAILURE();
            return;
        }

        camera_metadata_t* staticMeta;
        Return<void> ret;
        sp<ICameraDeviceSession> session;
        sp<device::V3_3::ICameraDeviceSession> session3_3;
        sp<device::V3_4::ICameraDeviceSession> session3_4;
        sp<device::V3_5::ICameraDeviceSession> session3_5;
        sp<device::V3_6::ICameraDeviceSession> session3_6;
        sp<device::V3_2::ICameraDevice> cameraDevice;
        sp<device::V3_5::ICameraDevice> cameraDevice3_5;
        openEmptyDeviceSession(name, mProvider,
                &session /*out*/, &staticMeta /*out*/, &cameraDevice /*out*/);
        castSession(session, deviceVersion, &session3_3, &session3_4, &session3_5, &session3_6);
        castDevice(cameraDevice, deviceVersion, &cameraDevice3_5);

        outputStreams.clear();
        ASSERT_EQ(Status::OK, getAvailableOutputStreams(staticMeta, outputStreams));
        ASSERT_NE(0u, outputStreams.size());

        uint32_t jpegBufferSize = 0;
        ASSERT_EQ(Status::OK, getJpegBufferSize(staticMeta, &jpegBufferSize));
        ASSERT_NE(0u, jpegBufferSize);

        int32_t streamId = 0;
        uint32_t streamConfigCounter = 0;
        for (auto& it : outputStreams) {
            V3_2::Stream stream3_2;
            V3_2::DataspaceFlags dataspaceFlag = 0;
            switch (static_cast<PixelFormat>(it.format)) {
                case PixelFormat::BLOB:
                    dataspaceFlag = static_cast<V3_2::DataspaceFlags>(Dataspace::V0_JFIF);
                    break;
                case PixelFormat::Y16:
                    dataspaceFlag = static_cast<V3_2::DataspaceFlags>(Dataspace::DEPTH);
                    break;
                default:
                    dataspaceFlag = static_cast<V3_2::DataspaceFlags>(Dataspace::UNKNOWN);
            }
            stream3_2 = {streamId,
                             StreamType::OUTPUT,
                             static_cast<uint32_t>(it.width),
                             static_cast<uint32_t>(it.height),
                             static_cast<PixelFormat>(it.format),
                             GRALLOC1_CONSUMER_USAGE_HWCOMPOSER,
                             dataspaceFlag,
                             StreamRotation::ROTATION_0};
            ::android::hardware::hidl_vec<V3_2::Stream> streams3_2 = {stream3_2};
            ::android::hardware::camera::device::V3_5::StreamConfiguration config3_5;
            ::android::hardware::camera::device::V3_4::StreamConfiguration config3_4;
            ::android::hardware::camera::device::V3_2::StreamConfiguration config3_2;
            createStreamConfiguration(streams3_2, StreamConfigurationMode::NORMAL_MODE,
                                      &config3_2, &config3_4, &config3_5, jpegBufferSize);
            if (session3_5 != nullptr) {
                bool expectStreamCombQuery = (isLogicalMultiCamera(staticMeta) == Status::OK);
                verifyStreamCombination(cameraDevice3_5, config3_4,
                        /*expectedStatus*/ true, expectStreamCombQuery);
                config3_5.streamConfigCounter = streamConfigCounter++;
                ret = session3_5->configureStreams_3_5(config3_5,
                        [streamId](Status s, device::V3_4::HalStreamConfiguration halConfig) {
                            ASSERT_EQ(Status::OK, s);
                            ASSERT_EQ(1u, halConfig.streams.size());
                            ASSERT_EQ(halConfig.streams[0].v3_3.v3_2.id, streamId);
                        });
            } else if (session3_4 != nullptr) {
                ret = session3_4->configureStreams_3_4(config3_4,
                        [streamId](Status s, device::V3_4::HalStreamConfiguration halConfig) {
                            ASSERT_EQ(Status::OK, s);
                            ASSERT_EQ(1u, halConfig.streams.size());
                            ASSERT_EQ(halConfig.streams[0].v3_3.v3_2.id, streamId);
                        });
            } else if (session3_3 != nullptr) {
                ret = session3_3->configureStreams_3_3(config3_2,
                        [streamId](Status s, device::V3_3::HalStreamConfiguration halConfig) {
                            ASSERT_EQ(Status::OK, s);
                            ASSERT_EQ(1u, halConfig.streams.size());
                            ASSERT_EQ(halConfig.streams[0].v3_2.id, streamId);
                        });
            } else {
                ret = session->configureStreams(config3_2,
                        [streamId](Status s, HalStreamConfiguration halConfig) {
                            ASSERT_EQ(Status::OK, s);
                            ASSERT_EQ(1u, halConfig.streams.size());
                            ASSERT_EQ(halConfig.streams[0].id, streamId);
                        });
            }
            ASSERT_TRUE(ret.isOk());
            streamId++;
        }

        free_camera_metadata(staticMeta);
        ret = session->close();
        ASSERT_TRUE(ret.isOk());
    }
}

// Verify that mandatory concurrent streams and outputs are supported.
TEST_P(CameraHidlTest, configureConcurrentStreamsAvailableOutputs) {
    struct CameraTestInfo {
        camera_metadata_t* staticMeta = nullptr;
        sp<ICameraDeviceSession> session;
        sp<device::V3_3::ICameraDeviceSession> session3_3;
        sp<device::V3_4::ICameraDeviceSession> session3_4;
        sp<device::V3_5::ICameraDeviceSession> session3_5;
        sp<device::V3_6::ICameraDeviceSession> session3_6;
        sp<device::V3_2::ICameraDevice> cameraDevice;
        sp<device::V3_5::ICameraDevice> cameraDevice3_5;
        ::android::hardware::camera::device::V3_5::StreamConfiguration config3_5;
        ::android::hardware::camera::device::V3_4::StreamConfiguration config3_4;
        ::android::hardware::camera::device::V3_2::StreamConfiguration config3_2;
    };
    if (mProvider2_6 == nullptr) {
        // This test is provider@2.6 specific
        ALOGW("%s provider not 2_6, skipping", __func__);
        return;
    }

    std::map<hidl_string, hidl_string> idToNameMap = getCameraDeviceIdToNameMap(mProvider2_6);
    hidl_vec<hidl_vec<hidl_string>> concurrentDeviceCombinations =
            getConcurrentDeviceCombinations(mProvider2_6);
    std::vector<AvailableStream> outputStreams;
    for (const auto& cameraDeviceIds : concurrentDeviceCombinations) {
        std::vector<CameraIdAndStreamCombination> cameraIdsAndStreamCombinations;
        std::vector<CameraTestInfo> cameraTestInfos;
        size_t i = 0;
        for (const auto& id : cameraDeviceIds) {
            CameraTestInfo cti;
            Return<void> ret;
            auto it = idToNameMap.find(id);
            ASSERT_TRUE(idToNameMap.end() != it);
            hidl_string name = it->second;
            int deviceVersion = getCameraDeviceVersion(name, mProviderType);
            if (deviceVersion == CAMERA_DEVICE_API_VERSION_1_0) {
                continue;
            } else if (deviceVersion <= 0) {
                ALOGE("%s: Unsupported device version %d", __func__, deviceVersion);
                ADD_FAILURE();
                return;
            }
            openEmptyDeviceSession(name, mProvider2_6, &cti.session /*out*/,
                                   &cti.staticMeta /*out*/, &cti.cameraDevice /*out*/);
            castSession(cti.session, deviceVersion, &cti.session3_3, &cti.session3_4,
                        &cti.session3_5, &cti.session3_6);
            castDevice(cti.cameraDevice, deviceVersion, &cti.cameraDevice3_5);

            outputStreams.clear();
            ASSERT_EQ(Status::OK, getMandatoryConcurrentStreams(cti.staticMeta, &outputStreams));
            ASSERT_NE(0u, outputStreams.size());

            uint32_t jpegBufferSize = 0;
            ASSERT_EQ(Status::OK, getJpegBufferSize(cti.staticMeta, &jpegBufferSize));
            ASSERT_NE(0u, jpegBufferSize);

            int32_t streamId = 0;
            ::android::hardware::hidl_vec<V3_2::Stream> streams3_2(outputStreams.size());
            size_t j = 0;
            for (const auto& it : outputStreams) {
                V3_2::Stream stream3_2;
                V3_2::DataspaceFlags dataspaceFlag = 0;
                switch (static_cast<PixelFormat>(it.format)) {
                    case PixelFormat::BLOB:
                        dataspaceFlag = static_cast<V3_2::DataspaceFlags>(Dataspace::V0_JFIF);
                        break;
                    case PixelFormat::Y16:
                        dataspaceFlag = static_cast<V3_2::DataspaceFlags>(Dataspace::DEPTH);
                        break;
                    default:
                        dataspaceFlag = static_cast<V3_2::DataspaceFlags>(Dataspace::UNKNOWN);
                }
                stream3_2 = {streamId++,
                             StreamType::OUTPUT,
                             static_cast<uint32_t>(it.width),
                             static_cast<uint32_t>(it.height),
                             static_cast<PixelFormat>(it.format),
                             GRALLOC1_CONSUMER_USAGE_HWCOMPOSER,
                             dataspaceFlag,
                             StreamRotation::ROTATION_0};
                streams3_2[j] = stream3_2;
                j++;
            }

            // Add the created stream configs to cameraIdsAndStreamCombinations
            createStreamConfiguration(streams3_2, StreamConfigurationMode::NORMAL_MODE,
                                      &cti.config3_2, &cti.config3_4, &cti.config3_5,
                                      jpegBufferSize);

            cti.config3_5.streamConfigCounter = outputStreams.size();
            CameraIdAndStreamCombination cameraIdAndStreamCombination;
            cameraIdAndStreamCombination.cameraId = id;
            cameraIdAndStreamCombination.streamConfiguration = cti.config3_4;
            cameraIdsAndStreamCombinations.push_back(cameraIdAndStreamCombination);
            i++;
            cameraTestInfos.push_back(cti);
        }
        // Now verify that concurrent streams are supported
        auto cb = [](Status s, bool supported) {
            ASSERT_EQ(Status::OK, s);
            ASSERT_EQ(supported, true);
        };

        auto ret = mProvider2_6->isConcurrentStreamCombinationSupported(
                cameraIdsAndStreamCombinations, cb);

        // Test the stream can actually be configured
        for (const auto& cti : cameraTestInfos) {
            if (cti.session3_5 != nullptr) {
                bool expectStreamCombQuery = (isLogicalMultiCamera(cti.staticMeta) == Status::OK);
                verifyStreamCombination(cti.cameraDevice3_5, cti.config3_4,
                                        /*expectedStatus*/ true, expectStreamCombQuery);
                ret = cti.session3_5->configureStreams_3_5(
                        cti.config3_5,
                        [&cti](Status s, device::V3_4::HalStreamConfiguration halConfig) {
                            ASSERT_EQ(Status::OK, s);
                            ASSERT_EQ(cti.config3_5.v3_4.streams.size(), halConfig.streams.size());
                        });
            } else if (cti.session3_4 != nullptr) {
                ret = cti.session3_4->configureStreams_3_4(
                        cti.config3_4,
                        [&cti](Status s, device::V3_4::HalStreamConfiguration halConfig) {
                            ASSERT_EQ(Status::OK, s);
                            ASSERT_EQ(cti.config3_4.streams.size(), halConfig.streams.size());
                        });
            } else if (cti.session3_3 != nullptr) {
                ret = cti.session3_3->configureStreams_3_3(
                        cti.config3_2,
                        [&cti](Status s, device::V3_3::HalStreamConfiguration halConfig) {
                            ASSERT_EQ(Status::OK, s);
                            ASSERT_EQ(cti.config3_2.streams.size(), halConfig.streams.size());
                        });
            } else {
                ret = cti.session->configureStreams(
                        cti.config3_2, [&cti](Status s, HalStreamConfiguration halConfig) {
                            ASSERT_EQ(Status::OK, s);
                            ASSERT_EQ(cti.config3_2.streams.size(), halConfig.streams.size());
                        });
            }
            ASSERT_TRUE(ret.isOk());
        }

        for (const auto& cti : cameraTestInfos) {
            free_camera_metadata(cti.staticMeta);
            ret = cti.session->close();
            ASSERT_TRUE(ret.isOk());
        }
    }
}

// Check for correct handling of invalid/incorrect configuration parameters.
TEST_P(CameraHidlTest, configureStreamsInvalidOutputs) {
    hidl_vec<hidl_string> cameraDeviceNames = getCameraDeviceNames(mProvider);
    std::vector<AvailableStream> outputStreams;

    for (const auto& name : cameraDeviceNames) {
        int deviceVersion = getCameraDeviceVersion(name, mProviderType);
        if (deviceVersion == CAMERA_DEVICE_API_VERSION_1_0) {
            continue;
        } else if (deviceVersion <= 0) {
            ALOGE("%s: Unsupported device version %d", __func__, deviceVersion);
            ADD_FAILURE();
            return;
        }

        camera_metadata_t* staticMeta;
        Return<void> ret;
        sp<ICameraDeviceSession> session;
        sp<device::V3_3::ICameraDeviceSession> session3_3;
        sp<device::V3_4::ICameraDeviceSession> session3_4;
        sp<device::V3_5::ICameraDeviceSession> session3_5;
        sp<device::V3_6::ICameraDeviceSession> session3_6;
        sp<device::V3_2::ICameraDevice> cameraDevice;
        sp<device::V3_5::ICameraDevice> cameraDevice3_5;
        openEmptyDeviceSession(name, mProvider, &session /*out*/, &staticMeta /*out*/,
                &cameraDevice /*out*/);
        castSession(session, deviceVersion, &session3_3, &session3_4, &session3_5, &session3_6);
        castDevice(cameraDevice, deviceVersion, &cameraDevice3_5);

        outputStreams.clear();
        ASSERT_EQ(Status::OK, getAvailableOutputStreams(staticMeta, outputStreams));
        ASSERT_NE(0u, outputStreams.size());

        uint32_t jpegBufferSize = 0;
        ASSERT_EQ(Status::OK, getJpegBufferSize(staticMeta, &jpegBufferSize));
        ASSERT_NE(0u, jpegBufferSize);

        int32_t streamId = 0;
        V3_2::Stream stream3_2 = {streamId++,
                         StreamType::OUTPUT,
                         static_cast<uint32_t>(0),
                         static_cast<uint32_t>(0),
                         static_cast<PixelFormat>(outputStreams[0].format),
                         GRALLOC1_CONSUMER_USAGE_HWCOMPOSER,
                         0,
                         StreamRotation::ROTATION_0};
        uint32_t streamConfigCounter = 0;
        ::android::hardware::hidl_vec<V3_2::Stream> streams = {stream3_2};
        ::android::hardware::camera::device::V3_5::StreamConfiguration config3_5;
        ::android::hardware::camera::device::V3_4::StreamConfiguration config3_4;
        ::android::hardware::camera::device::V3_2::StreamConfiguration config3_2;
        createStreamConfiguration(streams, StreamConfigurationMode::NORMAL_MODE,
                                  &config3_2, &config3_4, &config3_5, jpegBufferSize);
        if (session3_5 != nullptr) {
            verifyStreamCombination(cameraDevice3_5, config3_4, /*expectedStatus*/ false,
                    /*expectStreamCombQuery*/false);
            config3_5.streamConfigCounter = streamConfigCounter++;
            ret = session3_5->configureStreams_3_5(config3_5,
                    [](Status s, device::V3_4::HalStreamConfiguration) {
                        ASSERT_TRUE((Status::ILLEGAL_ARGUMENT == s) ||
                            (Status::INTERNAL_ERROR == s));
                    });
        } else if (session3_4 != nullptr) {
            ret = session3_4->configureStreams_3_4(config3_4,
                    [](Status s, device::V3_4::HalStreamConfiguration) {
                        ASSERT_TRUE((Status::ILLEGAL_ARGUMENT == s) ||
                                (Status::INTERNAL_ERROR == s));
                    });
        } else if(session3_3 != nullptr) {
            ret = session3_3->configureStreams_3_3(config3_2,
                    [](Status s, device::V3_3::HalStreamConfiguration) {
                        ASSERT_TRUE((Status::ILLEGAL_ARGUMENT == s) ||
                                (Status::INTERNAL_ERROR == s));
                    });
        } else {
            ret = session->configureStreams(config3_2,
                    [](Status s, HalStreamConfiguration) {
                        ASSERT_TRUE((Status::ILLEGAL_ARGUMENT == s) ||
                                (Status::INTERNAL_ERROR == s));
                    });
        }
        ASSERT_TRUE(ret.isOk());

        stream3_2 = {streamId++,
                  StreamType::OUTPUT,
                  static_cast<uint32_t>(UINT32_MAX),
                  static_cast<uint32_t>(UINT32_MAX),
                  static_cast<PixelFormat>(outputStreams[0].format),
                  GRALLOC1_CONSUMER_USAGE_HWCOMPOSER,
                  0,
                  StreamRotation::ROTATION_0};
        streams[0] = stream3_2;
        createStreamConfiguration(streams, StreamConfigurationMode::NORMAL_MODE,
                &config3_2, &config3_4, &config3_5, jpegBufferSize);
        if (session3_5 != nullptr) {
            config3_5.streamConfigCounter = streamConfigCounter++;
            ret = session3_5->configureStreams_3_5(config3_5, [](Status s,
                        device::V3_4::HalStreamConfiguration) {
                    ASSERT_EQ(Status::ILLEGAL_ARGUMENT, s);
                });
        } else if(session3_4 != nullptr) {
            ret = session3_4->configureStreams_3_4(config3_4, [](Status s,
                        device::V3_4::HalStreamConfiguration) {
                    ASSERT_EQ(Status::ILLEGAL_ARGUMENT, s);
                });
        } else if(session3_3 != nullptr) {
            ret = session3_3->configureStreams_3_3(config3_2, [](Status s,
                        device::V3_3::HalStreamConfiguration) {
                    ASSERT_EQ(Status::ILLEGAL_ARGUMENT, s);
                });
        } else {
            ret = session->configureStreams(config3_2, [](Status s,
                        HalStreamConfiguration) {
                    ASSERT_EQ(Status::ILLEGAL_ARGUMENT, s);
                });
        }
        ASSERT_TRUE(ret.isOk());

        for (auto& it : outputStreams) {
            stream3_2 = {streamId++,
                      StreamType::OUTPUT,
                      static_cast<uint32_t>(it.width),
                      static_cast<uint32_t>(it.height),
                      static_cast<PixelFormat>(UINT32_MAX),
                      GRALLOC1_CONSUMER_USAGE_HWCOMPOSER,
                      0,
                      StreamRotation::ROTATION_0};
            streams[0] = stream3_2;
            createStreamConfiguration(streams, StreamConfigurationMode::NORMAL_MODE,
                    &config3_2, &config3_4, &config3_5, jpegBufferSize);
            if (session3_5 != nullptr) {
                config3_5.streamConfigCounter = streamConfigCounter++;
                ret = session3_5->configureStreams_3_5(config3_5,
                        [](Status s, device::V3_4::HalStreamConfiguration) {
                            ASSERT_EQ(Status::ILLEGAL_ARGUMENT, s);
                        });
            } else if(session3_4 != nullptr) {
                ret = session3_4->configureStreams_3_4(config3_4,
                        [](Status s, device::V3_4::HalStreamConfiguration) {
                            ASSERT_EQ(Status::ILLEGAL_ARGUMENT, s);
                        });
            } else if(session3_3 != nullptr) {
                ret = session3_3->configureStreams_3_3(config3_2,
                        [](Status s, device::V3_3::HalStreamConfiguration) {
                            ASSERT_EQ(Status::ILLEGAL_ARGUMENT, s);
                        });
            } else {
                ret = session->configureStreams(config3_2,
                        [](Status s, HalStreamConfiguration) {
                            ASSERT_EQ(Status::ILLEGAL_ARGUMENT, s);
                        });
            }
            ASSERT_TRUE(ret.isOk());

            stream3_2 = {streamId++,
                      StreamType::OUTPUT,
                      static_cast<uint32_t>(it.width),
                      static_cast<uint32_t>(it.height),
                      static_cast<PixelFormat>(it.format),
                      GRALLOC1_CONSUMER_USAGE_HWCOMPOSER,
                      0,
                      static_cast<StreamRotation>(UINT32_MAX)};
            streams[0] = stream3_2;
            createStreamConfiguration(streams, StreamConfigurationMode::NORMAL_MODE,
                    &config3_2, &config3_4, &config3_5, jpegBufferSize);
            if (session3_5 != nullptr) {
                config3_5.streamConfigCounter = streamConfigCounter++;
                ret = session3_5->configureStreams_3_5(config3_5,
                        [](Status s, device::V3_4::HalStreamConfiguration) {
                            ASSERT_EQ(Status::ILLEGAL_ARGUMENT, s);
                        });
            } else if(session3_4 != nullptr) {
                ret = session3_4->configureStreams_3_4(config3_4,
                        [](Status s, device::V3_4::HalStreamConfiguration) {
                            ASSERT_EQ(Status::ILLEGAL_ARGUMENT, s);
                        });
            } else if(session3_3 != nullptr) {
                ret = session3_3->configureStreams_3_3(config3_2,
                        [](Status s, device::V3_3::HalStreamConfiguration) {
                            ASSERT_EQ(Status::ILLEGAL_ARGUMENT, s);
                        });
            } else {
                ret = session->configureStreams(config3_2,
                        [](Status s, HalStreamConfiguration) {
                            ASSERT_EQ(Status::ILLEGAL_ARGUMENT, s);
                        });
            }
            ASSERT_TRUE(ret.isOk());
        }

        free_camera_metadata(staticMeta);
        ret = session->close();
        ASSERT_TRUE(ret.isOk());
    }
}

// Check whether all supported ZSL output stream combinations can be
// configured successfully.
TEST_P(CameraHidlTest, configureStreamsZSLInputOutputs) {
    hidl_vec<hidl_string> cameraDeviceNames = getCameraDeviceNames(mProvider);
    std::vector<AvailableStream> inputStreams;
    std::vector<AvailableZSLInputOutput> inputOutputMap;

    for (const auto& name : cameraDeviceNames) {
        int deviceVersion = getCameraDeviceVersion(name, mProviderType);
        if (deviceVersion == CAMERA_DEVICE_API_VERSION_1_0) {
            continue;
        } else if (deviceVersion <= 0) {
            ALOGE("%s: Unsupported device version %d", __func__, deviceVersion);
            ADD_FAILURE();
            return;
        }

        camera_metadata_t* staticMeta;
        Return<void> ret;
        sp<ICameraDeviceSession> session;
        sp<device::V3_3::ICameraDeviceSession> session3_3;
        sp<device::V3_4::ICameraDeviceSession> session3_4;
        sp<device::V3_5::ICameraDeviceSession> session3_5;
        sp<device::V3_6::ICameraDeviceSession> session3_6;
        sp<device::V3_2::ICameraDevice> cameraDevice;
        sp<device::V3_5::ICameraDevice> cameraDevice3_5;
        openEmptyDeviceSession(name, mProvider, &session /*out*/, &staticMeta /*out*/,
                &cameraDevice /*out*/);
        castSession(session, deviceVersion, &session3_3, &session3_4, &session3_5, &session3_6);
        castDevice(cameraDevice, deviceVersion, &cameraDevice3_5);

        Status rc = isZSLModeAvailable(staticMeta);
        if (Status::METHOD_NOT_SUPPORTED == rc) {
            ret = session->close();
            ASSERT_TRUE(ret.isOk());
            continue;
        }
        ASSERT_EQ(Status::OK, rc);

        inputStreams.clear();
        ASSERT_EQ(Status::OK, getAvailableOutputStreams(staticMeta, inputStreams));
        ASSERT_NE(0u, inputStreams.size());

        inputOutputMap.clear();
        ASSERT_EQ(Status::OK, getZSLInputOutputMap(staticMeta, inputOutputMap));
        ASSERT_NE(0u, inputOutputMap.size());

        bool supportMonoY8 = false;
        if (Status::OK == isMonochromeCamera(staticMeta)) {
            for (auto& it : inputStreams) {
                if (it.format == static_cast<uint32_t>(PixelFormat::Y8)) {
                    supportMonoY8 = true;
                    break;
                }
            }
        }

        uint32_t jpegBufferSize = 0;
        ASSERT_EQ(Status::OK, getJpegBufferSize(staticMeta, &jpegBufferSize));
        ASSERT_NE(0u, jpegBufferSize);

        int32_t streamId = 0;
        bool hasPrivToY8 = false, hasY8ToY8 = false, hasY8ToBlob = false;
        uint32_t streamConfigCounter = 0;
        for (auto& inputIter : inputOutputMap) {
            AvailableStream input;
            ASSERT_EQ(Status::OK, findLargestSize(inputStreams, inputIter.inputFormat,
                    input));
            ASSERT_NE(0u, inputStreams.size());

            if (inputIter.inputFormat == static_cast<uint32_t>(PixelFormat::IMPLEMENTATION_DEFINED)
                    && inputIter.outputFormat == static_cast<uint32_t>(PixelFormat::Y8)) {
                hasPrivToY8 = true;
            } else if (inputIter.inputFormat == static_cast<uint32_t>(PixelFormat::Y8)) {
                if (inputIter.outputFormat == static_cast<uint32_t>(PixelFormat::BLOB)) {
                    hasY8ToBlob = true;
                } else if (inputIter.outputFormat == static_cast<uint32_t>(PixelFormat::Y8)) {
                    hasY8ToY8 = true;
                }
            }
            AvailableStream outputThreshold = {INT32_MAX, INT32_MAX,
                                               inputIter.outputFormat};
            std::vector<AvailableStream> outputStreams;
            ASSERT_EQ(Status::OK,
                      getAvailableOutputStreams(staticMeta, outputStreams,
                              &outputThreshold));
            for (auto& outputIter : outputStreams) {
                V3_2::Stream zslStream = {streamId++,
                                    StreamType::OUTPUT,
                                    static_cast<uint32_t>(input.width),
                                    static_cast<uint32_t>(input.height),
                                    static_cast<PixelFormat>(input.format),
                                    GRALLOC_USAGE_HW_CAMERA_ZSL,
                                    0,
                                    StreamRotation::ROTATION_0};
                V3_2::Stream inputStream = {streamId++,
                                      StreamType::INPUT,
                                      static_cast<uint32_t>(input.width),
                                      static_cast<uint32_t>(input.height),
                                      static_cast<PixelFormat>(input.format),
                                      0,
                                      0,
                                      StreamRotation::ROTATION_0};
                V3_2::Stream outputStream = {streamId++,
                                       StreamType::OUTPUT,
                                       static_cast<uint32_t>(outputIter.width),
                                       static_cast<uint32_t>(outputIter.height),
                                       static_cast<PixelFormat>(outputIter.format),
                                       GRALLOC1_CONSUMER_USAGE_HWCOMPOSER,
                                       0,
                                       StreamRotation::ROTATION_0};

                ::android::hardware::hidl_vec<V3_2::Stream> streams = {inputStream, zslStream,
                                                                 outputStream};
                ::android::hardware::camera::device::V3_5::StreamConfiguration config3_5;
                ::android::hardware::camera::device::V3_4::StreamConfiguration config3_4;
                ::android::hardware::camera::device::V3_2::StreamConfiguration config3_2;
                createStreamConfiguration(streams, StreamConfigurationMode::NORMAL_MODE,
                                          &config3_2, &config3_4, &config3_5, jpegBufferSize);
                if (session3_5 != nullptr) {
                    verifyStreamCombination(cameraDevice3_5, config3_4,
                            /*expectedStatus*/ true, /*expectStreamCombQuery*/ false);
                    config3_5.streamConfigCounter = streamConfigCounter++;
                    ret = session3_5->configureStreams_3_5(config3_5,
                            [](Status s, device::V3_4::HalStreamConfiguration halConfig) {
                                ASSERT_EQ(Status::OK, s);
                                ASSERT_EQ(3u, halConfig.streams.size());
                            });
                } else if (session3_4 != nullptr) {
                    ret = session3_4->configureStreams_3_4(config3_4,
                            [](Status s, device::V3_4::HalStreamConfiguration halConfig) {
                                ASSERT_EQ(Status::OK, s);
                                ASSERT_EQ(3u, halConfig.streams.size());
                            });
                } else if (session3_3 != nullptr) {
                    ret = session3_3->configureStreams_3_3(config3_2,
                            [](Status s, device::V3_3::HalStreamConfiguration halConfig) {
                                ASSERT_EQ(Status::OK, s);
                                ASSERT_EQ(3u, halConfig.streams.size());
                            });
                } else {
                    ret = session->configureStreams(config3_2,
                            [](Status s, HalStreamConfiguration halConfig) {
                                ASSERT_EQ(Status::OK, s);
                                ASSERT_EQ(3u, halConfig.streams.size());
                            });
                }
                ASSERT_TRUE(ret.isOk());
            }
        }

        if (supportMonoY8) {
            if (Status::OK == isZSLModeAvailable(staticMeta, PRIV_REPROCESS)) {
                ASSERT_TRUE(hasPrivToY8);
            }
            if (Status::OK == isZSLModeAvailable(staticMeta, YUV_REPROCESS)) {
                ASSERT_TRUE(hasY8ToY8);
                ASSERT_TRUE(hasY8ToBlob);
            }
        }

        free_camera_metadata(staticMeta);
        ret = session->close();
        ASSERT_TRUE(ret.isOk());
    }
}

// Check whether session parameters are supported. If Hal support for them
// exist, then try to configure a preview stream using them.
TEST_P(CameraHidlTest, configureStreamsWithSessionParameters) {
    hidl_vec<hidl_string> cameraDeviceNames = getCameraDeviceNames(mProvider);
    std::vector<AvailableStream> outputPreviewStreams;
    AvailableStream previewThreshold = {kMaxPreviewWidth, kMaxPreviewHeight,
                                        static_cast<int32_t>(PixelFormat::IMPLEMENTATION_DEFINED)};

    for (const auto& name : cameraDeviceNames) {
        int deviceVersion = getCameraDeviceVersion(name, mProviderType);
        if (deviceVersion <= 0) {
            ALOGE("%s: Unsupported device version %d", __func__, deviceVersion);
            ADD_FAILURE();
            return;
        } else if (deviceVersion < CAMERA_DEVICE_API_VERSION_3_4) {
            continue;
        }

        camera_metadata_t* staticMetaBuffer;
        Return<void> ret;
        sp<ICameraDeviceSession> session;
        sp<device::V3_3::ICameraDeviceSession> session3_3;
        sp<device::V3_4::ICameraDeviceSession> session3_4;
        sp<device::V3_5::ICameraDeviceSession> session3_5;
        sp<device::V3_6::ICameraDeviceSession> session3_6;
        openEmptyDeviceSession(name, mProvider, &session /*out*/, &staticMetaBuffer /*out*/);
        castSession(session, deviceVersion, &session3_3, &session3_4, &session3_5, &session3_6);
        if (deviceVersion == CAMERA_DEVICE_API_VERSION_3_4) {
            ASSERT_NE(session3_4, nullptr);
        } else {
            ASSERT_NE(session3_5, nullptr);
        }

        std::unordered_set<int32_t> availableSessionKeys;
        auto rc = getSupportedKeys(staticMetaBuffer, ANDROID_REQUEST_AVAILABLE_SESSION_KEYS,
                &availableSessionKeys);
        ASSERT_TRUE(Status::OK == rc);
        if (availableSessionKeys.empty()) {
            free_camera_metadata(staticMetaBuffer);
            ret = session->close();
            ASSERT_TRUE(ret.isOk());
            continue;
        }

        android::hardware::camera::common::V1_0::helper::CameraMetadata previewRequestSettings;
        android::hardware::camera::common::V1_0::helper::CameraMetadata sessionParams,
                modifiedSessionParams;
        constructFilteredSettings(session, availableSessionKeys, RequestTemplate::PREVIEW,
                &previewRequestSettings, &sessionParams);
        if (sessionParams.isEmpty()) {
            free_camera_metadata(staticMetaBuffer);
            ret = session->close();
            ASSERT_TRUE(ret.isOk());
            continue;
        }

        outputPreviewStreams.clear();

        ASSERT_EQ(Status::OK, getAvailableOutputStreams(staticMetaBuffer, outputPreviewStreams,
                &previewThreshold));
        ASSERT_NE(0u, outputPreviewStreams.size());

        V3_4::Stream previewStream;
        previewStream.v3_2 = {0,
                                StreamType::OUTPUT,
                                static_cast<uint32_t>(outputPreviewStreams[0].width),
                                static_cast<uint32_t>(outputPreviewStreams[0].height),
                                static_cast<PixelFormat>(outputPreviewStreams[0].format),
                                GRALLOC1_CONSUMER_USAGE_HWCOMPOSER,
                                0,
                                StreamRotation::ROTATION_0};
        previewStream.bufferSize = 0;
        ::android::hardware::hidl_vec<V3_4::Stream> streams = {previewStream};
        ::android::hardware::camera::device::V3_4::StreamConfiguration config;
        ::android::hardware::camera::device::V3_5::StreamConfiguration config3_5;
        config.streams = streams;
        config.operationMode = StreamConfigurationMode::NORMAL_MODE;
        modifiedSessionParams = sessionParams;
        auto sessionParamsBuffer = sessionParams.release();
        config.sessionParams.setToExternal(reinterpret_cast<uint8_t *> (sessionParamsBuffer),
                get_camera_metadata_size(sessionParamsBuffer));
        config3_5.v3_4 = config;
        config3_5.streamConfigCounter = 0;
        if (session3_5 != nullptr) {
            bool newSessionParamsAvailable = false;
            for (const auto& it : availableSessionKeys) {
                if (modifiedSessionParams.exists(it)) {
                    modifiedSessionParams.erase(it);
                    newSessionParamsAvailable = true;
                    break;
                }
            }
            if (newSessionParamsAvailable) {
                auto modifiedSessionParamsBuffer = modifiedSessionParams.release();
                verifySessionReconfigurationQuery(session3_5, sessionParamsBuffer,
                        modifiedSessionParamsBuffer);
                modifiedSessionParams.acquire(modifiedSessionParamsBuffer);
            }

            ret = session3_5->configureStreams_3_5(config3_5,
                    [](Status s, device::V3_4::HalStreamConfiguration halConfig) {
                        ASSERT_EQ(Status::OK, s);
                        ASSERT_EQ(1u, halConfig.streams.size());
                    });
        } else {
            ret = session3_4->configureStreams_3_4(config,
                    [](Status s, device::V3_4::HalStreamConfiguration halConfig) {
                        ASSERT_EQ(Status::OK, s);
                        ASSERT_EQ(1u, halConfig.streams.size());
                    });
        }
        sessionParams.acquire(sessionParamsBuffer);
        ASSERT_TRUE(ret.isOk());

        free_camera_metadata(staticMetaBuffer);
        ret = session->close();
        ASSERT_TRUE(ret.isOk());
    }
}

// Verify that all supported preview + still capture stream combinations
// can be configured successfully.
TEST_P(CameraHidlTest, configureStreamsPreviewStillOutputs) {
    hidl_vec<hidl_string> cameraDeviceNames = getCameraDeviceNames(mProvider);
    std::vector<AvailableStream> outputBlobStreams;
    std::vector<AvailableStream> outputPreviewStreams;
    AvailableStream previewThreshold = {kMaxPreviewWidth, kMaxPreviewHeight,
                                        static_cast<int32_t>(PixelFormat::IMPLEMENTATION_DEFINED)};
    AvailableStream blobThreshold = {INT32_MAX, INT32_MAX,
                                     static_cast<int32_t>(PixelFormat::BLOB)};

    for (const auto& name : cameraDeviceNames) {
        int deviceVersion = getCameraDeviceVersion(name, mProviderType);
        if (deviceVersion == CAMERA_DEVICE_API_VERSION_1_0) {
            continue;
        } else if (deviceVersion <= 0) {
            ALOGE("%s: Unsupported device version %d", __func__, deviceVersion);
            ADD_FAILURE();
            return;
        }

        camera_metadata_t* staticMeta;
        Return<void> ret;
        sp<ICameraDeviceSession> session;
        sp<device::V3_3::ICameraDeviceSession> session3_3;
        sp<device::V3_4::ICameraDeviceSession> session3_4;
        sp<device::V3_5::ICameraDeviceSession> session3_5;
        sp<device::V3_6::ICameraDeviceSession> session3_6;
        sp<device::V3_2::ICameraDevice> cameraDevice;
        sp<device::V3_5::ICameraDevice> cameraDevice3_5;
        openEmptyDeviceSession(name, mProvider, &session /*out*/, &staticMeta /*out*/,
                &cameraDevice /*out*/);
        castSession(session, deviceVersion, &session3_3, &session3_4, &session3_5, &session3_6);
        castDevice(cameraDevice, deviceVersion, &cameraDevice3_5);

        // Check if camera support depth only
        if (isDepthOnly(staticMeta)) {
            free_camera_metadata(staticMeta);
            ret = session->close();
            ASSERT_TRUE(ret.isOk());
            continue;
        }

        outputBlobStreams.clear();
        ASSERT_EQ(Status::OK,
                  getAvailableOutputStreams(staticMeta, outputBlobStreams,
                          &blobThreshold));
        ASSERT_NE(0u, outputBlobStreams.size());

        outputPreviewStreams.clear();
        ASSERT_EQ(Status::OK, getAvailableOutputStreams(staticMeta, outputPreviewStreams,
                &previewThreshold));
        ASSERT_NE(0u, outputPreviewStreams.size());

        uint32_t jpegBufferSize = 0;
        ASSERT_EQ(Status::OK, getJpegBufferSize(staticMeta, &jpegBufferSize));
        ASSERT_NE(0u, jpegBufferSize);

        int32_t streamId = 0;
        uint32_t streamConfigCounter = 0;
        for (auto& blobIter : outputBlobStreams) {
            for (auto& previewIter : outputPreviewStreams) {
                V3_2::Stream previewStream = {streamId++,
                                        StreamType::OUTPUT,
                                        static_cast<uint32_t>(previewIter.width),
                                        static_cast<uint32_t>(previewIter.height),
                                        static_cast<PixelFormat>(previewIter.format),
                                        GRALLOC1_CONSUMER_USAGE_HWCOMPOSER,
                                        0,
                                        StreamRotation::ROTATION_0};
                V3_2::Stream blobStream = {streamId++,
                                     StreamType::OUTPUT,
                                     static_cast<uint32_t>(blobIter.width),
                                     static_cast<uint32_t>(blobIter.height),
                                     static_cast<PixelFormat>(blobIter.format),
                                     GRALLOC1_CONSUMER_USAGE_CPU_READ,
                                     static_cast<V3_2::DataspaceFlags>(Dataspace::V0_JFIF),
                                     StreamRotation::ROTATION_0};
                ::android::hardware::hidl_vec<V3_2::Stream> streams = {previewStream,
                                                                 blobStream};
                ::android::hardware::camera::device::V3_5::StreamConfiguration config3_5;
                ::android::hardware::camera::device::V3_4::StreamConfiguration config3_4;
                ::android::hardware::camera::device::V3_2::StreamConfiguration config3_2;
                createStreamConfiguration(streams, StreamConfigurationMode::NORMAL_MODE,
                                          &config3_2, &config3_4, &config3_5, jpegBufferSize);
                if (session3_5 != nullptr) {
                    verifyStreamCombination(cameraDevice3_5, config3_4,
                            /*expectedStatus*/ true, /*expectStreamCombQuery*/ false);
                    config3_5.streamConfigCounter = streamConfigCounter++;
                    ret = session3_5->configureStreams_3_5(config3_5,
                            [](Status s, device::V3_4::HalStreamConfiguration halConfig) {
                                ASSERT_EQ(Status::OK, s);
                                ASSERT_EQ(2u, halConfig.streams.size());
                            });
                } else if (session3_4 != nullptr) {
                    ret = session3_4->configureStreams_3_4(config3_4,
                            [](Status s, device::V3_4::HalStreamConfiguration halConfig) {
                                ASSERT_EQ(Status::OK, s);
                                ASSERT_EQ(2u, halConfig.streams.size());
                            });
                } else if (session3_3 != nullptr) {
                    ret = session3_3->configureStreams_3_3(config3_2,
                            [](Status s, device::V3_3::HalStreamConfiguration halConfig) {
                                ASSERT_EQ(Status::OK, s);
                                ASSERT_EQ(2u, halConfig.streams.size());
                            });
                } else {
                    ret = session->configureStreams(config3_2,
                            [](Status s, HalStreamConfiguration halConfig) {
                                ASSERT_EQ(Status::OK, s);
                                ASSERT_EQ(2u, halConfig.streams.size());
                            });
                }
                ASSERT_TRUE(ret.isOk());
            }
        }

        free_camera_metadata(staticMeta);
        ret = session->close();
        ASSERT_TRUE(ret.isOk());
    }
}

// In case constrained mode is supported, test whether it can be
// configured. Additionally check for common invalid inputs when
// using this mode.
TEST_P(CameraHidlTest, configureStreamsConstrainedOutputs) {
    hidl_vec<hidl_string> cameraDeviceNames = getCameraDeviceNames(mProvider);

    for (const auto& name : cameraDeviceNames) {
        int deviceVersion = getCameraDeviceVersion(name, mProviderType);
        if (deviceVersion == CAMERA_DEVICE_API_VERSION_1_0) {
            continue;
        } else if (deviceVersion <= 0) {
            ALOGE("%s: Unsupported device version %d", __func__, deviceVersion);
            ADD_FAILURE();
            return;
        }

        camera_metadata_t* staticMeta;
        Return<void> ret;
        sp<ICameraDeviceSession> session;
        sp<device::V3_3::ICameraDeviceSession> session3_3;
        sp<device::V3_4::ICameraDeviceSession> session3_4;
        sp<device::V3_5::ICameraDeviceSession> session3_5;
        sp<device::V3_6::ICameraDeviceSession> session3_6;
        sp<device::V3_2::ICameraDevice> cameraDevice;
        sp<device::V3_5::ICameraDevice> cameraDevice3_5;
        openEmptyDeviceSession(name, mProvider, &session /*out*/, &staticMeta /*out*/,
                &cameraDevice /*out*/);
        castSession(session, deviceVersion, &session3_3, &session3_4, &session3_5, &session3_6);
        castDevice(cameraDevice, deviceVersion, &cameraDevice3_5);

        Status rc = isConstrainedModeAvailable(staticMeta);
        if (Status::METHOD_NOT_SUPPORTED == rc) {
            ret = session->close();
            ASSERT_TRUE(ret.isOk());
            continue;
        }
        ASSERT_EQ(Status::OK, rc);

        AvailableStream hfrStream;
        rc = pickConstrainedModeSize(staticMeta, hfrStream);
        ASSERT_EQ(Status::OK, rc);

        int32_t streamId = 0;
        uint32_t streamConfigCounter = 0;
        V3_2::Stream stream = {streamId,
                         StreamType::OUTPUT,
                         static_cast<uint32_t>(hfrStream.width),
                         static_cast<uint32_t>(hfrStream.height),
                         static_cast<PixelFormat>(hfrStream.format),
                         GRALLOC1_CONSUMER_USAGE_VIDEO_ENCODER,
                         0,
                         StreamRotation::ROTATION_0};
        ::android::hardware::hidl_vec<V3_2::Stream> streams = {stream};
        ::android::hardware::camera::device::V3_5::StreamConfiguration config3_5;
        ::android::hardware::camera::device::V3_4::StreamConfiguration config3_4;
        ::android::hardware::camera::device::V3_2::StreamConfiguration config3_2;
        createStreamConfiguration(streams, StreamConfigurationMode::CONSTRAINED_HIGH_SPEED_MODE,
                                  &config3_2, &config3_4, &config3_5);
        if (session3_5 != nullptr) {
            verifyStreamCombination(cameraDevice3_5, config3_4,
                    /*expectedStatus*/ true, /*expectStreamCombQuery*/ false);
            config3_5.streamConfigCounter = streamConfigCounter++;
            ret = session3_5->configureStreams_3_5(config3_5,
                    [streamId](Status s, device::V3_4::HalStreamConfiguration halConfig) {
                        ASSERT_EQ(Status::OK, s);
                        ASSERT_EQ(1u, halConfig.streams.size());
                        ASSERT_EQ(halConfig.streams[0].v3_3.v3_2.id, streamId);
                    });
        } else if (session3_4 != nullptr) {
            ret = session3_4->configureStreams_3_4(config3_4,
                    [streamId](Status s, device::V3_4::HalStreamConfiguration halConfig) {
                        ASSERT_EQ(Status::OK, s);
                        ASSERT_EQ(1u, halConfig.streams.size());
                        ASSERT_EQ(halConfig.streams[0].v3_3.v3_2.id, streamId);
                    });
        } else if (session3_3 != nullptr) {
            ret = session3_3->configureStreams_3_3(config3_2,
                    [streamId](Status s, device::V3_3::HalStreamConfiguration halConfig) {
                        ASSERT_EQ(Status::OK, s);
                        ASSERT_EQ(1u, halConfig.streams.size());
                        ASSERT_EQ(halConfig.streams[0].v3_2.id, streamId);
                    });
        } else {
            ret = session->configureStreams(config3_2,
                    [streamId](Status s, HalStreamConfiguration halConfig) {
                        ASSERT_EQ(Status::OK, s);
                        ASSERT_EQ(1u, halConfig.streams.size());
                        ASSERT_EQ(halConfig.streams[0].id, streamId);
                    });
        }
        ASSERT_TRUE(ret.isOk());

        stream = {streamId++,
                  StreamType::OUTPUT,
                  static_cast<uint32_t>(0),
                  static_cast<uint32_t>(0),
                  static_cast<PixelFormat>(hfrStream.format),
                  GRALLOC1_CONSUMER_USAGE_VIDEO_ENCODER,
                  0,
                  StreamRotation::ROTATION_0};
        streams[0] = stream;
        createStreamConfiguration(streams, StreamConfigurationMode::CONSTRAINED_HIGH_SPEED_MODE,
                                  &config3_2, &config3_4, &config3_5);
        if (session3_5 != nullptr) {
            config3_5.streamConfigCounter = streamConfigCounter++;
            ret = session3_5->configureStreams_3_5(config3_5,
                    [](Status s, device::V3_4::HalStreamConfiguration) {
                        ASSERT_TRUE((Status::ILLEGAL_ARGUMENT == s) ||
                                (Status::INTERNAL_ERROR == s));
                    });
        } else if (session3_4 != nullptr) {
            ret = session3_4->configureStreams_3_4(config3_4,
                    [](Status s, device::V3_4::HalStreamConfiguration) {
                        ASSERT_TRUE((Status::ILLEGAL_ARGUMENT == s) ||
                                (Status::INTERNAL_ERROR == s));
                    });
        } else if (session3_3 != nullptr) {
            ret = session3_3->configureStreams_3_3(config3_2,
                    [](Status s, device::V3_3::HalStreamConfiguration) {
                        ASSERT_TRUE((Status::ILLEGAL_ARGUMENT == s) ||
                                (Status::INTERNAL_ERROR == s));
                    });
        } else {
            ret = session->configureStreams(config3_2,
                    [](Status s, HalStreamConfiguration) {
                        ASSERT_TRUE((Status::ILLEGAL_ARGUMENT == s) ||
                                (Status::INTERNAL_ERROR == s));
                    });
        }
        ASSERT_TRUE(ret.isOk());

        stream = {streamId++,
                  StreamType::OUTPUT,
                  static_cast<uint32_t>(UINT32_MAX),
                  static_cast<uint32_t>(UINT32_MAX),
                  static_cast<PixelFormat>(hfrStream.format),
                  GRALLOC1_CONSUMER_USAGE_VIDEO_ENCODER,
                  0,
                  StreamRotation::ROTATION_0};
        streams[0] = stream;
        createStreamConfiguration(streams, StreamConfigurationMode::CONSTRAINED_HIGH_SPEED_MODE,
                                  &config3_2, &config3_4, &config3_5);
        if (session3_5 != nullptr) {
            config3_5.streamConfigCounter = streamConfigCounter++;
            ret = session3_5->configureStreams_3_5(config3_5,
                    [](Status s, device::V3_4::HalStreamConfiguration) {
                        ASSERT_EQ(Status::ILLEGAL_ARGUMENT, s);
                    });
        } else if (session3_4 != nullptr) {
            ret = session3_4->configureStreams_3_4(config3_4,
                    [](Status s, device::V3_4::HalStreamConfiguration) {
                        ASSERT_EQ(Status::ILLEGAL_ARGUMENT, s);
                    });
        } else if (session3_3 != nullptr) {
            ret = session3_3->configureStreams_3_3(config3_2,
                    [](Status s, device::V3_3::HalStreamConfiguration) {
                        ASSERT_EQ(Status::ILLEGAL_ARGUMENT, s);
                    });
        } else {
            ret = session->configureStreams(config3_2,
                    [](Status s, HalStreamConfiguration) {
                        ASSERT_EQ(Status::ILLEGAL_ARGUMENT, s);
                    });
        }
        ASSERT_TRUE(ret.isOk());

        stream = {streamId++,
                  StreamType::OUTPUT,
                  static_cast<uint32_t>(hfrStream.width),
                  static_cast<uint32_t>(hfrStream.height),
                  static_cast<PixelFormat>(UINT32_MAX),
                  GRALLOC1_CONSUMER_USAGE_VIDEO_ENCODER,
                  0,
                  StreamRotation::ROTATION_0};
        streams[0] = stream;
        createStreamConfiguration(streams, StreamConfigurationMode::CONSTRAINED_HIGH_SPEED_MODE,
                                  &config3_2, &config3_4, &config3_5);
        if (session3_5 != nullptr) {
            config3_5.streamConfigCounter = streamConfigCounter++;
            ret = session3_5->configureStreams_3_5(config3_5,
                    [](Status s, device::V3_4::HalStreamConfiguration) {
                        ASSERT_EQ(Status::ILLEGAL_ARGUMENT, s);
                    });
        } else if (session3_4 != nullptr) {
            ret = session3_4->configureStreams_3_4(config3_4,
                    [](Status s, device::V3_4::HalStreamConfiguration) {
                        ASSERT_EQ(Status::ILLEGAL_ARGUMENT, s);
                    });
        } else if (session3_3 != nullptr) {
            ret = session3_3->configureStreams_3_3(config3_2,
                    [](Status s, device::V3_3::HalStreamConfiguration) {
                        ASSERT_EQ(Status::ILLEGAL_ARGUMENT, s);
                    });
        } else {
            ret = session->configureStreams(config3_2,
                    [](Status s, HalStreamConfiguration) {
                        ASSERT_EQ(Status::ILLEGAL_ARGUMENT, s);
                    });
        }
        ASSERT_TRUE(ret.isOk());

        free_camera_metadata(staticMeta);
        ret = session->close();
        ASSERT_TRUE(ret.isOk());
    }
}

// Verify that all supported video + snapshot stream combinations can
// be configured successfully.
TEST_P(CameraHidlTest, configureStreamsVideoStillOutputs) {
    hidl_vec<hidl_string> cameraDeviceNames = getCameraDeviceNames(mProvider);
    std::vector<AvailableStream> outputBlobStreams;
    std::vector<AvailableStream> outputVideoStreams;
    AvailableStream videoThreshold = {kMaxVideoWidth, kMaxVideoHeight,
                                      static_cast<int32_t>(PixelFormat::IMPLEMENTATION_DEFINED)};
    AvailableStream blobThreshold = {kMaxVideoWidth, kMaxVideoHeight,
                                     static_cast<int32_t>(PixelFormat::BLOB)};

    for (const auto& name : cameraDeviceNames) {
        int deviceVersion = getCameraDeviceVersion(name, mProviderType);
        if (deviceVersion == CAMERA_DEVICE_API_VERSION_1_0) {
            continue;
        } else if (deviceVersion <= 0) {
            ALOGE("%s: Unsupported device version %d", __func__, deviceVersion);
            ADD_FAILURE();
            return;
        }

        camera_metadata_t* staticMeta;
        Return<void> ret;
        sp<ICameraDeviceSession> session;
        sp<device::V3_3::ICameraDeviceSession> session3_3;
        sp<device::V3_4::ICameraDeviceSession> session3_4;
        sp<device::V3_5::ICameraDeviceSession> session3_5;
        sp<device::V3_6::ICameraDeviceSession> session3_6;
        sp<device::V3_2::ICameraDevice> cameraDevice;
        sp<device::V3_5::ICameraDevice> cameraDevice3_5;
        openEmptyDeviceSession(name, mProvider, &session /*out*/, &staticMeta /*out*/,
                &cameraDevice /*out*/);
        castSession(session, deviceVersion, &session3_3, &session3_4, &session3_5, &session3_6);
        castDevice(cameraDevice, deviceVersion, &cameraDevice3_5);

        // Check if camera support depth only
        if (isDepthOnly(staticMeta)) {
            free_camera_metadata(staticMeta);
            ret = session->close();
            ASSERT_TRUE(ret.isOk());
            continue;
        }

        outputBlobStreams.clear();
        ASSERT_EQ(Status::OK,
                  getAvailableOutputStreams(staticMeta, outputBlobStreams,
                          &blobThreshold));
        ASSERT_NE(0u, outputBlobStreams.size());

        outputVideoStreams.clear();
        ASSERT_EQ(Status::OK,
                  getAvailableOutputStreams(staticMeta, outputVideoStreams,
                          &videoThreshold));
        ASSERT_NE(0u, outputVideoStreams.size());

        uint32_t jpegBufferSize = 0;
        ASSERT_EQ(Status::OK, getJpegBufferSize(staticMeta, &jpegBufferSize));
        ASSERT_NE(0u, jpegBufferSize);

        int32_t streamId = 0;
        uint32_t streamConfigCounter = 0;
        for (auto& blobIter : outputBlobStreams) {
            for (auto& videoIter : outputVideoStreams) {
                V3_2::Stream videoStream = {streamId++,
                                      StreamType::OUTPUT,
                                      static_cast<uint32_t>(videoIter.width),
                                      static_cast<uint32_t>(videoIter.height),
                                      static_cast<PixelFormat>(videoIter.format),
                                      GRALLOC1_CONSUMER_USAGE_VIDEO_ENCODER,
                                      0,
                                      StreamRotation::ROTATION_0};
                V3_2::Stream blobStream = {streamId++,
                                     StreamType::OUTPUT,
                                     static_cast<uint32_t>(blobIter.width),
                                     static_cast<uint32_t>(blobIter.height),
                                     static_cast<PixelFormat>(blobIter.format),
                                     GRALLOC1_CONSUMER_USAGE_CPU_READ,
                                     static_cast<V3_2::DataspaceFlags>(Dataspace::V0_JFIF),
                                     StreamRotation::ROTATION_0};
                ::android::hardware::hidl_vec<V3_2::Stream> streams = {videoStream, blobStream};
                ::android::hardware::camera::device::V3_5::StreamConfiguration config3_5;
                ::android::hardware::camera::device::V3_4::StreamConfiguration config3_4;
                ::android::hardware::camera::device::V3_2::StreamConfiguration config3_2;
                createStreamConfiguration(streams, StreamConfigurationMode::NORMAL_MODE,
                                          &config3_2, &config3_4, &config3_5, jpegBufferSize);
                if (session3_5 != nullptr) {
                    verifyStreamCombination(cameraDevice3_5, config3_4,
                            /*expectedStatus*/ true, /*expectStreamCombQuery*/ false);
                    config3_5.streamConfigCounter = streamConfigCounter++;
                    ret = session3_5->configureStreams_3_5(config3_5,
                            [](Status s, device::V3_4::HalStreamConfiguration halConfig) {
                                ASSERT_EQ(Status::OK, s);
                                ASSERT_EQ(2u, halConfig.streams.size());
                            });
                } else if (session3_4 != nullptr) {
                    ret = session3_4->configureStreams_3_4(config3_4,
                            [](Status s, device::V3_4::HalStreamConfiguration halConfig) {
                                ASSERT_EQ(Status::OK, s);
                                ASSERT_EQ(2u, halConfig.streams.size());
                            });
                } else if (session3_3 != nullptr) {
                    ret = session3_3->configureStreams_3_3(config3_2,
                            [](Status s, device::V3_3::HalStreamConfiguration halConfig) {
                                ASSERT_EQ(Status::OK, s);
                                ASSERT_EQ(2u, halConfig.streams.size());
                            });
                } else {
                    ret = session->configureStreams(config3_2,
                            [](Status s, HalStreamConfiguration halConfig) {
                                ASSERT_EQ(Status::OK, s);
                                ASSERT_EQ(2u, halConfig.streams.size());
                            });
                }
                ASSERT_TRUE(ret.isOk());
            }
        }

        free_camera_metadata(staticMeta);
        ret = session->close();
        ASSERT_TRUE(ret.isOk());
    }
}

// Generate and verify a camera capture request
TEST_P(CameraHidlTest, processCaptureRequestPreview) {
    hidl_vec<hidl_string> cameraDeviceNames = getCameraDeviceNames(mProvider);
    AvailableStream previewThreshold = {kMaxPreviewWidth, kMaxPreviewHeight,
                                        static_cast<int32_t>(PixelFormat::IMPLEMENTATION_DEFINED)};
    uint64_t bufferId = 1;
    uint32_t frameNumber = 1;
    ::android::hardware::hidl_vec<uint8_t> settings;

    for (const auto& name : cameraDeviceNames) {
        int deviceVersion = getCameraDeviceVersion(name, mProviderType);
        if (deviceVersion == CAMERA_DEVICE_API_VERSION_1_0) {
            continue;
        } else if (deviceVersion <= 0) {
            ALOGE("%s: Unsupported device version %d", __func__, deviceVersion);
            ADD_FAILURE();
            return;
        }

        V3_2::Stream previewStream;
        HalStreamConfiguration halStreamConfig;
        sp<ICameraDeviceSession> session;
        sp<DeviceCb> cb;
        bool supportsPartialResults = false;
        bool useHalBufManager = false;
        uint32_t partialResultCount = 0;
        configurePreviewStream(name, deviceVersion, mProvider, &previewThreshold, &session /*out*/,
                &previewStream /*out*/, &halStreamConfig /*out*/,
                &supportsPartialResults /*out*/,
                &partialResultCount /*out*/, &useHalBufManager /*out*/, &cb /*out*/);

        std::shared_ptr<ResultMetadataQueue> resultQueue;
        auto resultQueueRet =
            session->getCaptureResultMetadataQueue(
                [&resultQueue](const auto& descriptor) {
                    resultQueue = std::make_shared<ResultMetadataQueue>(
                            descriptor);
                    if (!resultQueue->isValid() ||
                            resultQueue->availableToWrite() <= 0) {
                        ALOGE("%s: HAL returns empty result metadata fmq,"
                                " not use it", __func__);
                        resultQueue = nullptr;
                        // Don't use the queue onwards.
                    }
                });
        ASSERT_TRUE(resultQueueRet.isOk());

        InFlightRequest inflightReq = {1, false, supportsPartialResults,
                                       partialResultCount, resultQueue};

        RequestTemplate reqTemplate = RequestTemplate::PREVIEW;
        Return<void> ret;
        ret = session->constructDefaultRequestSettings(reqTemplate,
                                                       [&](auto status, const auto& req) {
                                                           ASSERT_EQ(Status::OK, status);
                                                           settings = req;
                                                       });
        ASSERT_TRUE(ret.isOk());

        hidl_handle buffer_handle;
        StreamBuffer outputBuffer;
        if (useHalBufManager) {
            outputBuffer = {halStreamConfig.streams[0].id,
                            /*bufferId*/ 0,
                            buffer_handle,
                            BufferStatus::OK,
                            nullptr,
                            nullptr};
        } else {
            allocateGraphicBuffer(previewStream.width, previewStream.height,
                    android_convertGralloc1To0Usage(halStreamConfig.streams[0].producerUsage,
                        halStreamConfig.streams[0].consumerUsage),
                    halStreamConfig.streams[0].overrideFormat, &buffer_handle);
            outputBuffer = {halStreamConfig.streams[0].id,
                            bufferId,
                            buffer_handle,
                            BufferStatus::OK,
                            nullptr,
                            nullptr};
        }
        ::android::hardware::hidl_vec<StreamBuffer> outputBuffers = {outputBuffer};
        StreamBuffer emptyInputBuffer = {-1, 0, nullptr, BufferStatus::ERROR, nullptr,
                                         nullptr};
        CaptureRequest request = {frameNumber, 0 /* fmqSettingsSize */, settings,
                                  emptyInputBuffer, outputBuffers};

        {
            std::unique_lock<std::mutex> l(mLock);
            mInflightMap.clear();
            mInflightMap.add(frameNumber, &inflightReq);
        }

        Status status = Status::INTERNAL_ERROR;
        uint32_t numRequestProcessed = 0;
        hidl_vec<BufferCache> cachesToRemove;
        Return<void> returnStatus = session->processCaptureRequest(
            {request}, cachesToRemove, [&status, &numRequestProcessed](auto s,
                    uint32_t n) {
                status = s;
                numRequestProcessed = n;
            });
        ASSERT_TRUE(returnStatus.isOk());
        ASSERT_EQ(Status::OK, status);
        ASSERT_EQ(numRequestProcessed, 1u);

        {
            std::unique_lock<std::mutex> l(mLock);
            while (!inflightReq.errorCodeValid &&
                   ((0 < inflightReq.numBuffersLeft) ||
                           (!inflightReq.haveResultMetadata))) {
                auto timeout = std::chrono::system_clock::now() +
                               std::chrono::seconds(kStreamBufferTimeoutSec);
                ASSERT_NE(std::cv_status::timeout,
                        mResultCondition.wait_until(l, timeout));
            }

            ASSERT_FALSE(inflightReq.errorCodeValid);
            ASSERT_NE(inflightReq.resultOutputBuffers.size(), 0u);
            ASSERT_EQ(previewStream.id, inflightReq.resultOutputBuffers[0].streamId);

            request.frameNumber++;
            // Empty settings should be supported after the first call
            // for repeating requests.
            request.settings.setToExternal(nullptr, 0, true);
            // The buffer has been registered to HAL by bufferId, so per
            // API contract we should send a null handle for this buffer
            request.outputBuffers[0].buffer = nullptr;
            mInflightMap.clear();
            inflightReq = {1, false, supportsPartialResults, partialResultCount,
                           resultQueue};
            mInflightMap.add(request.frameNumber, &inflightReq);
        }

        returnStatus = session->processCaptureRequest(
            {request}, cachesToRemove, [&status, &numRequestProcessed](auto s,
                    uint32_t n) {
                status = s;
                numRequestProcessed = n;
            });
        ASSERT_TRUE(returnStatus.isOk());
        ASSERT_EQ(Status::OK, status);
        ASSERT_EQ(numRequestProcessed, 1u);

        {
            std::unique_lock<std::mutex> l(mLock);
            while (!inflightReq.errorCodeValid &&
                   ((0 < inflightReq.numBuffersLeft) ||
                           (!inflightReq.haveResultMetadata))) {
                auto timeout = std::chrono::system_clock::now() +
                               std::chrono::seconds(kStreamBufferTimeoutSec);
                ASSERT_NE(std::cv_status::timeout,
                        mResultCondition.wait_until(l, timeout));
            }

            ASSERT_FALSE(inflightReq.errorCodeValid);
            ASSERT_NE(inflightReq.resultOutputBuffers.size(), 0u);
            ASSERT_EQ(previewStream.id, inflightReq.resultOutputBuffers[0].streamId);
        }

        if (useHalBufManager) {
            verifyBuffersReturned(session, deviceVersion, previewStream.id, cb);
        }

        ret = session->close();
        ASSERT_TRUE(ret.isOk());
    }
}

// Generate and verify a multi-camera capture request
TEST_P(CameraHidlTest, processMultiCaptureRequestPreview) {
    hidl_vec<hidl_string> cameraDeviceNames = getCameraDeviceNames(mProvider);
    AvailableStream previewThreshold = {kMaxPreviewWidth, kMaxPreviewHeight,
                                        static_cast<int32_t>(PixelFormat::YCBCR_420_888)};
    uint64_t bufferId = 1;
    uint32_t frameNumber = 1;
    ::android::hardware::hidl_vec<uint8_t> settings;
    ::android::hardware::hidl_vec<uint8_t> emptySettings;
    hidl_string invalidPhysicalId = "-1";

    for (const auto& name : cameraDeviceNames) {
        int deviceVersion = getCameraDeviceVersion(name, mProviderType);
        if (deviceVersion < CAMERA_DEVICE_API_VERSION_3_5) {
            continue;
        }
        std::string version, deviceId;
        ASSERT_TRUE(::matchDeviceName(name, mProviderType, &version, &deviceId));
        camera_metadata_t* staticMeta;
        Return<void> ret;
        sp<ICameraDeviceSession> session;
        openEmptyDeviceSession(name, mProvider, &session /*out*/, &staticMeta /*out*/);

        Status rc = isLogicalMultiCamera(staticMeta);
        if (Status::METHOD_NOT_SUPPORTED == rc) {
            free_camera_metadata(staticMeta);
            ret = session->close();
            ASSERT_TRUE(ret.isOk());
            continue;
        }
        std::unordered_set<std::string> physicalIds;
        rc = getPhysicalCameraIds(staticMeta, &physicalIds);
        ASSERT_TRUE(Status::OK == rc);
        ASSERT_TRUE(physicalIds.size() > 1);

        std::unordered_set<int32_t> physicalRequestKeyIDs;
        rc = getSupportedKeys(staticMeta,
                ANDROID_REQUEST_AVAILABLE_PHYSICAL_CAMERA_REQUEST_KEYS, &physicalRequestKeyIDs);
        ASSERT_TRUE(Status::OK == rc);
        if (physicalRequestKeyIDs.empty()) {
            free_camera_metadata(staticMeta);
            ret = session->close();
            ASSERT_TRUE(ret.isOk());
            // The logical camera doesn't support any individual physical requests.
            continue;
        }

        android::hardware::camera::common::V1_0::helper::CameraMetadata defaultPreviewSettings;
        android::hardware::camera::common::V1_0::helper::CameraMetadata filteredSettings;
        constructFilteredSettings(session, physicalRequestKeyIDs, RequestTemplate::PREVIEW,
                &defaultPreviewSettings, &filteredSettings);
        if (filteredSettings.isEmpty()) {
            // No physical device settings in default request.
            free_camera_metadata(staticMeta);
            ret = session->close();
            ASSERT_TRUE(ret.isOk());
            continue;
        }

        const camera_metadata_t *settingsBuffer = defaultPreviewSettings.getAndLock();
        settings.setToExternal(
                reinterpret_cast<uint8_t *> (const_cast<camera_metadata_t *> (settingsBuffer)),
                get_camera_metadata_size(settingsBuffer));

        free_camera_metadata(staticMeta);
        ret = session->close();
        ASSERT_TRUE(ret.isOk());

        // Leave only 2 physical devices in the id set.
        auto it = physicalIds.begin();
        std::string physicalDeviceId = *it; it++;
        physicalIds.erase(++it, physicalIds.end());
        ASSERT_EQ(physicalIds.size(), 2u);

        V3_4::HalStreamConfiguration halStreamConfig;
        bool supportsPartialResults = false;
        bool useHalBufManager = false;
        uint32_t partialResultCount = 0;
        V3_2::Stream previewStream;
        sp<device::V3_4::ICameraDeviceSession> session3_4;
        sp<device::V3_5::ICameraDeviceSession> session3_5;
        sp<DeviceCb> cb;
        configurePreviewStreams3_4(name, deviceVersion, mProvider, &previewThreshold, physicalIds,
                &session3_4, &session3_5, &previewStream, &halStreamConfig /*out*/,
                &supportsPartialResults /*out*/, &partialResultCount /*out*/,
                &useHalBufManager /*out*/, &cb /*out*/, 0 /*streamConfigCounter*/,
                true /*allowUnsupport*/);
        if (session3_5 == nullptr) {
            ret = session3_4->close();
            ASSERT_TRUE(ret.isOk());
            continue;
        }

        std::shared_ptr<ResultMetadataQueue> resultQueue;
        auto resultQueueRet =
            session3_4->getCaptureResultMetadataQueue(
                [&resultQueue](const auto& descriptor) {
                    resultQueue = std::make_shared<ResultMetadataQueue>(
                            descriptor);
                    if (!resultQueue->isValid() ||
                            resultQueue->availableToWrite() <= 0) {
                        ALOGE("%s: HAL returns empty result metadata fmq,"
                                " not use it", __func__);
                        resultQueue = nullptr;
                        // Don't use the queue onwards.
                    }
                });
        ASSERT_TRUE(resultQueueRet.isOk());

        InFlightRequest inflightReq = {static_cast<ssize_t> (halStreamConfig.streams.size()), false,
            supportsPartialResults, partialResultCount, physicalIds, resultQueue};

        std::vector<hidl_handle> graphicBuffers;
        graphicBuffers.reserve(halStreamConfig.streams.size());
        ::android::hardware::hidl_vec<StreamBuffer> outputBuffers;
        outputBuffers.resize(halStreamConfig.streams.size());
        size_t k = 0;
        for (const auto& halStream : halStreamConfig.streams) {
            hidl_handle buffer_handle;
            if (useHalBufManager) {
                outputBuffers[k] = {halStream.v3_3.v3_2.id, /*bufferId*/0, buffer_handle,
                    BufferStatus::OK, nullptr, nullptr};
            } else {
                allocateGraphicBuffer(previewStream.width, previewStream.height,
                        android_convertGralloc1To0Usage(halStream.v3_3.v3_2.producerUsage,
                            halStream.v3_3.v3_2.consumerUsage),
                        halStream.v3_3.v3_2.overrideFormat, &buffer_handle);
                graphicBuffers.push_back(buffer_handle);
                outputBuffers[k] = {halStream.v3_3.v3_2.id, bufferId, buffer_handle,
                    BufferStatus::OK, nullptr, nullptr};
                bufferId++;
            }
            k++;
        }
        hidl_vec<V3_4::PhysicalCameraSetting> camSettings(1);
        const camera_metadata_t *filteredSettingsBuffer = filteredSettings.getAndLock();
        camSettings[0].settings.setToExternal(
                reinterpret_cast<uint8_t *> (const_cast<camera_metadata_t *> (
                        filteredSettingsBuffer)),
                get_camera_metadata_size(filteredSettingsBuffer));
        camSettings[0].fmqSettingsSize = 0;
        camSettings[0].physicalCameraId = physicalDeviceId;

        StreamBuffer emptyInputBuffer = {-1, 0, nullptr, BufferStatus::ERROR, nullptr, nullptr};
        V3_4::CaptureRequest request = {{frameNumber, 0 /* fmqSettingsSize */, settings,
                                  emptyInputBuffer, outputBuffers}, camSettings};

        {
            std::unique_lock<std::mutex> l(mLock);
            mInflightMap.clear();
            mInflightMap.add(frameNumber, &inflightReq);
        }

        Status stat = Status::INTERNAL_ERROR;
        uint32_t numRequestProcessed = 0;
        hidl_vec<BufferCache> cachesToRemove;
        Return<void> returnStatus = session3_4->processCaptureRequest_3_4(
            {request}, cachesToRemove, [&stat, &numRequestProcessed](auto s, uint32_t n) {
                stat = s;
                numRequestProcessed = n;
            });
        ASSERT_TRUE(returnStatus.isOk());
        ASSERT_EQ(Status::OK, stat);
        ASSERT_EQ(numRequestProcessed, 1u);

        {
            std::unique_lock<std::mutex> l(mLock);
            while (!inflightReq.errorCodeValid &&
                    ((0 < inflightReq.numBuffersLeft) ||
                     (!inflightReq.haveResultMetadata))) {
                auto timeout = std::chrono::system_clock::now() +
                    std::chrono::seconds(kStreamBufferTimeoutSec);
                ASSERT_NE(std::cv_status::timeout,
                        mResultCondition.wait_until(l, timeout));
            }

            ASSERT_FALSE(inflightReq.errorCodeValid);
            ASSERT_NE(inflightReq.resultOutputBuffers.size(), 0u);

            request.v3_2.frameNumber++;
            // Empty settings should be supported after the first call
            // for repeating requests.
            request.v3_2.settings.setToExternal(nullptr, 0, true);
            request.physicalCameraSettings[0].settings.setToExternal(nullptr, 0, true);
            // The buffer has been registered to HAL by bufferId, so per
            // API contract we should send a null handle for this buffer
            request.v3_2.outputBuffers[0].buffer = nullptr;
            mInflightMap.clear();
            inflightReq = {static_cast<ssize_t> (physicalIds.size()), false,
                supportsPartialResults, partialResultCount, physicalIds, resultQueue};
            mInflightMap.add(request.v3_2.frameNumber, &inflightReq);
        }

        returnStatus = session3_4->processCaptureRequest_3_4(
            {request}, cachesToRemove, [&stat, &numRequestProcessed](auto s, uint32_t n) {
                stat = s;
                numRequestProcessed = n;
            });
        ASSERT_TRUE(returnStatus.isOk());
        ASSERT_EQ(Status::OK, stat);
        ASSERT_EQ(numRequestProcessed, 1u);

        {
            std::unique_lock<std::mutex> l(mLock);
            while (!inflightReq.errorCodeValid &&
                    ((0 < inflightReq.numBuffersLeft) ||
                     (!inflightReq.haveResultMetadata))) {
                auto timeout = std::chrono::system_clock::now() +
                    std::chrono::seconds(kStreamBufferTimeoutSec);
                ASSERT_NE(std::cv_status::timeout,
                        mResultCondition.wait_until(l, timeout));
            }

            ASSERT_FALSE(inflightReq.errorCodeValid);
            ASSERT_NE(inflightReq.resultOutputBuffers.size(), 0u);
        }

        // Invalid physical camera id should fail process requests
        frameNumber++;
        camSettings[0].physicalCameraId = invalidPhysicalId;
        camSettings[0].settings = settings;
        request = {{frameNumber, 0 /* fmqSettingsSize */, settings,
            emptyInputBuffer, outputBuffers}, camSettings};
        returnStatus = session3_4->processCaptureRequest_3_4(
            {request}, cachesToRemove, [&stat, &numRequestProcessed](auto s, uint32_t n) {
                stat = s;
                numRequestProcessed = n;
            });
        ASSERT_TRUE(returnStatus.isOk());
        ASSERT_EQ(Status::ILLEGAL_ARGUMENT, stat);

        defaultPreviewSettings.unlock(settingsBuffer);
        filteredSettings.unlock(filteredSettingsBuffer);

        if (useHalBufManager) {
            hidl_vec<int32_t> streamIds(halStreamConfig.streams.size());
            for (size_t i = 0; i < streamIds.size(); i++) {
                streamIds[i] = halStreamConfig.streams[i].v3_3.v3_2.id;
            }
            verifyBuffersReturned(session3_4, streamIds, cb);
        }

        ret = session3_4->close();
        ASSERT_TRUE(ret.isOk());
    }
}

// Generate and verify a burst containing alternating sensor sensitivity values
TEST_P(CameraHidlTest, processCaptureRequestBurstISO) {
    hidl_vec<hidl_string> cameraDeviceNames = getCameraDeviceNames(mProvider);
    AvailableStream previewThreshold = {kMaxPreviewWidth, kMaxPreviewHeight,
                                        static_cast<int32_t>(PixelFormat::IMPLEMENTATION_DEFINED)};
    uint64_t bufferId = 1;
    uint32_t frameNumber = 1;
    float isoTol = .03f;
    ::android::hardware::hidl_vec<uint8_t> settings;

    for (const auto& name : cameraDeviceNames) {
        int deviceVersion = getCameraDeviceVersion(name, mProviderType);
        if (deviceVersion == CAMERA_DEVICE_API_VERSION_1_0) {
            continue;
        } else if (deviceVersion <= 0) {
            ALOGE("%s: Unsupported device version %d", __func__, deviceVersion);
            ADD_FAILURE();
            return;
        }
        camera_metadata_t* staticMetaBuffer;
        Return<void> ret;
        sp<ICameraDeviceSession> session;
        openEmptyDeviceSession(name, mProvider, &session /*out*/, &staticMetaBuffer /*out*/);
        ::android::hardware::camera::common::V1_0::helper::CameraMetadata staticMeta(
                staticMetaBuffer);

        camera_metadata_entry_t hwLevel = staticMeta.find(ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL);
        ASSERT_TRUE(0 < hwLevel.count);
        if (ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_LIMITED == hwLevel.data.u8[0] ||
                ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_EXTERNAL == hwLevel.data.u8[0]) {
            //Limited/External devices can skip this test
            ret = session->close();
            ASSERT_TRUE(ret.isOk());
            continue;
        }

        camera_metadata_entry_t isoRange = staticMeta.find(ANDROID_SENSOR_INFO_SENSITIVITY_RANGE);
        ASSERT_EQ(isoRange.count, 2u);

        ret = session->close();
        ASSERT_TRUE(ret.isOk());

        bool supportsPartialResults = false;
        bool useHalBufManager = false;
        uint32_t partialResultCount = 0;
        V3_2::Stream previewStream;
        HalStreamConfiguration halStreamConfig;
        sp<DeviceCb> cb;
        configurePreviewStream(name, deviceVersion, mProvider, &previewThreshold,
                &session /*out*/, &previewStream /*out*/, &halStreamConfig /*out*/,
                &supportsPartialResults /*out*/, &partialResultCount /*out*/,
                &useHalBufManager /*out*/, &cb /*out*/);
        std::shared_ptr<ResultMetadataQueue> resultQueue;

        auto resultQueueRet = session->getCaptureResultMetadataQueue(
            [&resultQueue](const auto& descriptor) {
                resultQueue = std::make_shared<ResultMetadataQueue>(descriptor);
                if (!resultQueue->isValid() || resultQueue->availableToWrite() <= 0) {
                    ALOGE("%s: HAL returns empty result metadata fmq,"
                            " not use it", __func__);
                    resultQueue = nullptr;
                    // Don't use the queue onwards.
                }
            });
        ASSERT_TRUE(resultQueueRet.isOk());
        ASSERT_NE(nullptr, resultQueue);

        ret = session->constructDefaultRequestSettings(RequestTemplate::PREVIEW,
            [&](auto status, const auto& req) {
                ASSERT_EQ(Status::OK, status);
                settings = req; });
        ASSERT_TRUE(ret.isOk());

        ::android::hardware::camera::common::V1_0::helper::CameraMetadata requestMeta;
        StreamBuffer emptyInputBuffer = {-1, 0, nullptr, BufferStatus::ERROR, nullptr, nullptr};
        hidl_handle buffers[kBurstFrameCount];
        StreamBuffer outputBuffers[kBurstFrameCount];
        CaptureRequest requests[kBurstFrameCount];
        InFlightRequest inflightReqs[kBurstFrameCount];
        int32_t isoValues[kBurstFrameCount];
        hidl_vec<uint8_t> requestSettings[kBurstFrameCount];
        for (uint32_t i = 0; i < kBurstFrameCount; i++) {
            std::unique_lock<std::mutex> l(mLock);

            isoValues[i] = ((i % 2) == 0) ? isoRange.data.i32[0] : isoRange.data.i32[1];
            if (useHalBufManager) {
                outputBuffers[i] = {halStreamConfig.streams[0].id, /*bufferId*/0,
                    nullptr, BufferStatus::OK, nullptr, nullptr};
            } else {
                allocateGraphicBuffer(previewStream.width, previewStream.height,
                        android_convertGralloc1To0Usage(halStreamConfig.streams[0].producerUsage,
                            halStreamConfig.streams[0].consumerUsage),
                        halStreamConfig.streams[0].overrideFormat, &buffers[i]);
                outputBuffers[i] = {halStreamConfig.streams[0].id, bufferId + i,
                    buffers[i], BufferStatus::OK, nullptr, nullptr};
            }

            requestMeta.append(reinterpret_cast<camera_metadata_t *> (settings.data()));

            // Disable all 3A routines
            uint8_t mode = static_cast<uint8_t>(ANDROID_CONTROL_MODE_OFF);
            ASSERT_EQ(::android::OK, requestMeta.update(ANDROID_CONTROL_MODE, &mode, 1));
            ASSERT_EQ(::android::OK, requestMeta.update(ANDROID_SENSOR_SENSITIVITY, &isoValues[i],
                        1));
            camera_metadata_t *metaBuffer = requestMeta.release();
            requestSettings[i].setToExternal(reinterpret_cast<uint8_t *> (metaBuffer),
                    get_camera_metadata_size(metaBuffer), true);

            requests[i] = {frameNumber + i, 0 /* fmqSettingsSize */, requestSettings[i],
                emptyInputBuffer, {outputBuffers[i]}};

            inflightReqs[i] = {1, false, supportsPartialResults, partialResultCount, resultQueue};
            mInflightMap.add(frameNumber + i, &inflightReqs[i]);
        }

        Status status = Status::INTERNAL_ERROR;
        uint32_t numRequestProcessed = 0;
        hidl_vec<BufferCache> cachesToRemove;
        hidl_vec<CaptureRequest> burstRequest;
        burstRequest.setToExternal(requests, kBurstFrameCount);
        Return<void> returnStatus = session->processCaptureRequest(burstRequest, cachesToRemove,
                [&status, &numRequestProcessed] (auto s, uint32_t n) {
                    status = s;
                    numRequestProcessed = n;
                });
        ASSERT_TRUE(returnStatus.isOk());
        ASSERT_EQ(Status::OK, status);
        ASSERT_EQ(numRequestProcessed, kBurstFrameCount);

        for (size_t i = 0; i < kBurstFrameCount; i++) {
            std::unique_lock<std::mutex> l(mLock);
            while (!inflightReqs[i].errorCodeValid && ((0 < inflightReqs[i].numBuffersLeft) ||
                            (!inflightReqs[i].haveResultMetadata))) {
                auto timeout = std::chrono::system_clock::now() +
                        std::chrono::seconds(kStreamBufferTimeoutSec);
                ASSERT_NE(std::cv_status::timeout, mResultCondition.wait_until(l, timeout));
            }

            ASSERT_FALSE(inflightReqs[i].errorCodeValid);
            ASSERT_NE(inflightReqs[i].resultOutputBuffers.size(), 0u);
            ASSERT_EQ(previewStream.id, inflightReqs[i].resultOutputBuffers[0].streamId);
            ASSERT_FALSE(inflightReqs[i].collectedResult.isEmpty());
            ASSERT_TRUE(inflightReqs[i].collectedResult.exists(ANDROID_SENSOR_SENSITIVITY));
            camera_metadata_entry_t isoResult = inflightReqs[i].collectedResult.find(
                    ANDROID_SENSOR_SENSITIVITY);
            ASSERT_TRUE(std::abs(isoResult.data.i32[0] - isoValues[i]) <=
                        std::round(isoValues[i]*isoTol));
        }

        if (useHalBufManager) {
            verifyBuffersReturned(session, deviceVersion, previewStream.id, cb);
        }
        ret = session->close();
        ASSERT_TRUE(ret.isOk());
    }
}

// Test whether an incorrect capture request with missing settings will
// be reported correctly.
TEST_P(CameraHidlTest, processCaptureRequestInvalidSinglePreview) {
    hidl_vec<hidl_string> cameraDeviceNames = getCameraDeviceNames(mProvider);
    std::vector<AvailableStream> outputPreviewStreams;
    AvailableStream previewThreshold = {kMaxPreviewWidth, kMaxPreviewHeight,
                                        static_cast<int32_t>(PixelFormat::IMPLEMENTATION_DEFINED)};
    uint64_t bufferId = 1;
    uint32_t frameNumber = 1;
    ::android::hardware::hidl_vec<uint8_t> settings;

    for (const auto& name : cameraDeviceNames) {
        int deviceVersion = getCameraDeviceVersion(name, mProviderType);
        if (deviceVersion == CAMERA_DEVICE_API_VERSION_1_0) {
            continue;
        } else if (deviceVersion <= 0) {
            ALOGE("%s: Unsupported device version %d", __func__, deviceVersion);
            ADD_FAILURE();
            return;
        }

        V3_2::Stream previewStream;
        HalStreamConfiguration halStreamConfig;
        sp<ICameraDeviceSession> session;
        sp<DeviceCb> cb;
        bool supportsPartialResults = false;
        bool useHalBufManager = false;
        uint32_t partialResultCount = 0;
        configurePreviewStream(name, deviceVersion, mProvider, &previewThreshold, &session /*out*/,
                &previewStream /*out*/, &halStreamConfig /*out*/,
                &supportsPartialResults /*out*/,
                &partialResultCount /*out*/, &useHalBufManager /*out*/, &cb /*out*/);

        hidl_handle buffer_handle;

        if (useHalBufManager) {
            bufferId = 0;
        } else {
            allocateGraphicBuffer(previewStream.width, previewStream.height,
                    android_convertGralloc1To0Usage(halStreamConfig.streams[0].producerUsage,
                        halStreamConfig.streams[0].consumerUsage),
                    halStreamConfig.streams[0].overrideFormat, &buffer_handle);
        }

        StreamBuffer outputBuffer = {halStreamConfig.streams[0].id,
                                     bufferId,
                                     buffer_handle,
                                     BufferStatus::OK,
                                     nullptr,
                                     nullptr};
        ::android::hardware::hidl_vec<StreamBuffer> outputBuffers = {outputBuffer};
        StreamBuffer emptyInputBuffer = {-1, 0, nullptr, BufferStatus::ERROR, nullptr,
                                         nullptr};
        CaptureRequest request = {frameNumber, 0 /* fmqSettingsSize */, settings,
                                  emptyInputBuffer, outputBuffers};

        // Settings were not correctly initialized, we should fail here
        Status status = Status::OK;
        uint32_t numRequestProcessed = 0;
        hidl_vec<BufferCache> cachesToRemove;
        Return<void> ret = session->processCaptureRequest(
            {request}, cachesToRemove, [&status, &numRequestProcessed](auto s,
                    uint32_t n) {
                status = s;
                numRequestProcessed = n;
            });
        ASSERT_TRUE(ret.isOk());
        ASSERT_EQ(Status::ILLEGAL_ARGUMENT, status);
        ASSERT_EQ(numRequestProcessed, 0u);

        ret = session->close();
        ASSERT_TRUE(ret.isOk());
    }
}

// Verify camera offline session behavior
TEST_P(CameraHidlTest, switchToOffline) {
    hidl_vec<hidl_string> cameraDeviceNames = getCameraDeviceNames(mProvider);
    AvailableStream threshold = {kMaxStillWidth, kMaxStillHeight,
                                        static_cast<int32_t>(PixelFormat::BLOB)};
    uint64_t bufferId = 1;
    uint32_t frameNumber = 1;
    ::android::hardware::hidl_vec<uint8_t> settings;

    for (const auto& name : cameraDeviceNames) {
        int deviceVersion = getCameraDeviceVersion(name, mProviderType);
        if (deviceVersion == CAMERA_DEVICE_API_VERSION_1_0) {
            continue;
        } else if (deviceVersion <= 0) {
            ALOGE("%s: Unsupported device version %d", __func__, deviceVersion);
            ADD_FAILURE();
            return;
        }

        camera_metadata_t* staticMetaBuffer;
        {
            Return<void> ret;
            sp<ICameraDeviceSession> session;
            openEmptyDeviceSession(name, mProvider, &session /*out*/, &staticMetaBuffer /*out*/);
            ::android::hardware::camera::common::V1_0::helper::CameraMetadata staticMeta(
                    staticMetaBuffer);

            if (isOfflineSessionSupported(staticMetaBuffer) != Status::OK) {
                ret = session->close();
                ASSERT_TRUE(ret.isOk());
                continue;
            }
            ret = session->close();
            ASSERT_TRUE(ret.isOk());
        }

        bool supportsPartialResults = false;
        uint32_t partialResultCount = 0;
        V3_2::Stream stream;
        V3_6::HalStreamConfiguration halStreamConfig;
        sp<V3_6::ICameraDeviceSession> session;
        sp<DeviceCb> cb;
        uint32_t jpegBufferSize;
        bool useHalBufManager;
        configureOfflineStillStream(name, deviceVersion, mProvider, &threshold,
                &session /*out*/, &stream /*out*/, &halStreamConfig /*out*/,
                &supportsPartialResults /*out*/, &partialResultCount /*out*/, &cb /*out*/,
                &jpegBufferSize /*out*/, &useHalBufManager /*out*/);

        auto ret = session->constructDefaultRequestSettings(RequestTemplate::STILL_CAPTURE,
            [&](auto status, const auto& req) {
                ASSERT_EQ(Status::OK, status);
                settings = req; });
        ASSERT_TRUE(ret.isOk());

        std::shared_ptr<ResultMetadataQueue> resultQueue;
        auto resultQueueRet =
            session->getCaptureResultMetadataQueue(
                [&resultQueue](const auto& descriptor) {
                    resultQueue = std::make_shared<ResultMetadataQueue>(
                            descriptor);
                    if (!resultQueue->isValid() ||
                            resultQueue->availableToWrite() <= 0) {
                        ALOGE("%s: HAL returns empty result metadata fmq,"
                                " not use it", __func__);
                        resultQueue = nullptr;
                        // Don't use the queue onwards.
                    }
                });
        ASSERT_TRUE(resultQueueRet.isOk());

        ::android::hardware::camera::common::V1_0::helper::CameraMetadata requestMeta;
        StreamBuffer emptyInputBuffer = {-1, 0, nullptr, BufferStatus::ERROR, nullptr, nullptr};
        hidl_handle buffers[kBurstFrameCount];
        StreamBuffer outputBuffers[kBurstFrameCount];
        CaptureRequest requests[kBurstFrameCount];
        InFlightRequest inflightReqs[kBurstFrameCount];
        hidl_vec<uint8_t> requestSettings[kBurstFrameCount];
        auto halStreamConfig3_2 = halStreamConfig.streams[0].v3_4.v3_3.v3_2;
        for (uint32_t i = 0; i < kBurstFrameCount; i++) {
            std::unique_lock<std::mutex> l(mLock);

            if (useHalBufManager) {
                outputBuffers[i] = {halStreamConfig3_2.id, /*bufferId*/ 0,
                        buffers[i], BufferStatus::OK, nullptr, nullptr};
            } else {
                // jpeg buffer (w,h) = (blobLen, 1)
                allocateGraphicBuffer(jpegBufferSize, /*height*/1,
                        android_convertGralloc1To0Usage(halStreamConfig3_2.producerUsage,
                            halStreamConfig3_2.consumerUsage),
                        halStreamConfig3_2.overrideFormat, &buffers[i]);
                outputBuffers[i] = {halStreamConfig3_2.id, bufferId + i,
                    buffers[i], BufferStatus::OK, nullptr, nullptr};
            }

            requestMeta.clear();
            requestMeta.append(reinterpret_cast<camera_metadata_t *> (settings.data()));

            camera_metadata_t *metaBuffer = requestMeta.release();
            requestSettings[i].setToExternal(reinterpret_cast<uint8_t *> (metaBuffer),
                    get_camera_metadata_size(metaBuffer), true);

            requests[i] = {frameNumber + i, 0 /* fmqSettingsSize */, requestSettings[i],
                emptyInputBuffer, {outputBuffers[i]}};

            inflightReqs[i] = {1, false, supportsPartialResults, partialResultCount,
                    resultQueue};
            mInflightMap.add(frameNumber + i, &inflightReqs[i]);
        }

        Status status = Status::INTERNAL_ERROR;
        uint32_t numRequestProcessed = 0;
        hidl_vec<BufferCache> cachesToRemove;
        hidl_vec<CaptureRequest> burstRequest;
        burstRequest.setToExternal(requests, kBurstFrameCount);
        Return<void> returnStatus = session->processCaptureRequest(burstRequest, cachesToRemove,
                [&status, &numRequestProcessed] (auto s, uint32_t n) {
                    status = s;
                    numRequestProcessed = n;
                });
        ASSERT_TRUE(returnStatus.isOk());
        ASSERT_EQ(Status::OK, status);
        ASSERT_EQ(numRequestProcessed, kBurstFrameCount);

        hidl_vec<int32_t> offlineStreamIds = {halStreamConfig3_2.id};
        V3_6::CameraOfflineSessionInfo offlineSessionInfo;
        sp<device::V3_6::ICameraOfflineSession> offlineSession;
        returnStatus = session->switchToOffline(offlineStreamIds,
                [&status, &offlineSessionInfo, &offlineSession] (auto stat, auto info,
                    auto offSession) {
                    status = stat;
                    offlineSessionInfo = info;
                    offlineSession = offSession;
                });
        ASSERT_TRUE(returnStatus.isOk());

        if (!halStreamConfig.streams[0].supportOffline) {
            ASSERT_EQ(status, Status::ILLEGAL_ARGUMENT);
            ret = session->close();
            ASSERT_TRUE(ret.isOk());
            continue;
        }

        ASSERT_EQ(status, Status::OK);
        // Hal might be unable to find any requests qualified for offline mode.
        if (offlineSession == nullptr) {
            ret = session->close();
            ASSERT_TRUE(ret.isOk());
            continue;
        }

        ASSERT_EQ(offlineSessionInfo.offlineStreams.size(), 1u);
        ASSERT_EQ(offlineSessionInfo.offlineStreams[0].id, halStreamConfig3_2.id);
        ASSERT_NE(offlineSessionInfo.offlineRequests.size(), 0u);

        // close device session to make sure offline session does not rely on it
        ret = session->close();
        ASSERT_TRUE(ret.isOk());

        std::shared_ptr<ResultMetadataQueue> offlineResultQueue;
        auto offlineResultQueueRet =
            offlineSession->getCaptureResultMetadataQueue(
                [&offlineResultQueue](const auto& descriptor) {
                    offlineResultQueue = std::make_shared<ResultMetadataQueue>(
                            descriptor);
                    if (!offlineResultQueue->isValid() ||
                            offlineResultQueue->availableToWrite() <= 0) {
                        ALOGE("%s: offline session returns empty result metadata fmq,"
                                " not use it", __func__);
                        offlineResultQueue = nullptr;
                        // Don't use the queue onwards.
                    }
                });
        ASSERT_TRUE(offlineResultQueueRet.isOk());

        updateInflightResultQueue(offlineResultQueue);

        ret = offlineSession->setCallback(cb);
        ASSERT_TRUE(ret.isOk());

        for (size_t i = 0; i < kBurstFrameCount; i++) {
            std::unique_lock<std::mutex> l(mLock);
            while (!inflightReqs[i].errorCodeValid && ((0 < inflightReqs[i].numBuffersLeft) ||
                            (!inflightReqs[i].haveResultMetadata))) {
                auto timeout = std::chrono::system_clock::now() +
                        std::chrono::seconds(kStreamBufferTimeoutSec);
                ASSERT_NE(std::cv_status::timeout, mResultCondition.wait_until(l, timeout));
            }

            ASSERT_FALSE(inflightReqs[i].errorCodeValid);
            ASSERT_NE(inflightReqs[i].resultOutputBuffers.size(), 0u);
            ASSERT_EQ(stream.id, inflightReqs[i].resultOutputBuffers[0].streamId);
            ASSERT_FALSE(inflightReqs[i].collectedResult.isEmpty());
        }


        ret = offlineSession->close();
        ASSERT_TRUE(ret.isOk());
    }
}

// Check whether an invalid capture request with missing output buffers
// will be reported correctly.
TEST_P(CameraHidlTest, processCaptureRequestInvalidBuffer) {
    hidl_vec<hidl_string> cameraDeviceNames = getCameraDeviceNames(mProvider);
    std::vector<AvailableStream> outputBlobStreams;
    AvailableStream previewThreshold = {kMaxPreviewWidth, kMaxPreviewHeight,
                                        static_cast<int32_t>(PixelFormat::IMPLEMENTATION_DEFINED)};
    uint32_t frameNumber = 1;
    ::android::hardware::hidl_vec<uint8_t> settings;

    for (const auto& name : cameraDeviceNames) {
        int deviceVersion = getCameraDeviceVersion(name, mProviderType);
        if (deviceVersion == CAMERA_DEVICE_API_VERSION_1_0) {
            continue;
        } else if (deviceVersion <= 0) {
            ALOGE("%s: Unsupported device version %d", __func__, deviceVersion);
            ADD_FAILURE();
            return;
        }

        V3_2::Stream previewStream;
        HalStreamConfiguration halStreamConfig;
        sp<ICameraDeviceSession> session;
        sp<DeviceCb> cb;
        bool supportsPartialResults = false;
        bool useHalBufManager = false;
        uint32_t partialResultCount = 0;
        configurePreviewStream(name, deviceVersion, mProvider, &previewThreshold, &session /*out*/,
                &previewStream /*out*/, &halStreamConfig /*out*/,
                &supportsPartialResults /*out*/,
                &partialResultCount /*out*/, &useHalBufManager /*out*/, &cb /*out*/);

        RequestTemplate reqTemplate = RequestTemplate::PREVIEW;
        Return<void> ret;
        ret = session->constructDefaultRequestSettings(reqTemplate,
                                                       [&](auto status, const auto& req) {
                                                           ASSERT_EQ(Status::OK, status);
                                                           settings = req;
                                                       });
        ASSERT_TRUE(ret.isOk());

        ::android::hardware::hidl_vec<StreamBuffer> emptyOutputBuffers;
        StreamBuffer emptyInputBuffer = {-1, 0, nullptr, BufferStatus::ERROR, nullptr,
                                         nullptr};
        CaptureRequest request = {frameNumber, 0 /* fmqSettingsSize */, settings,
                                  emptyInputBuffer, emptyOutputBuffers};

        // Output buffers are missing, we should fail here
        Status status = Status::OK;
        uint32_t numRequestProcessed = 0;
        hidl_vec<BufferCache> cachesToRemove;
        ret = session->processCaptureRequest(
            {request}, cachesToRemove, [&status, &numRequestProcessed](auto s,
                    uint32_t n) {
                status = s;
                numRequestProcessed = n;
            });
        ASSERT_TRUE(ret.isOk());
        ASSERT_EQ(Status::ILLEGAL_ARGUMENT, status);
        ASSERT_EQ(numRequestProcessed, 0u);

        ret = session->close();
        ASSERT_TRUE(ret.isOk());
    }
}

// Generate, trigger and flush a preview request
TEST_P(CameraHidlTest, flushPreviewRequest) {
    hidl_vec<hidl_string> cameraDeviceNames = getCameraDeviceNames(mProvider);
    std::vector<AvailableStream> outputPreviewStreams;
    AvailableStream previewThreshold = {kMaxPreviewWidth, kMaxPreviewHeight,
                                        static_cast<int32_t>(PixelFormat::IMPLEMENTATION_DEFINED)};
    uint64_t bufferId = 1;
    uint32_t frameNumber = 1;
    ::android::hardware::hidl_vec<uint8_t> settings;

    for (const auto& name : cameraDeviceNames) {
        int deviceVersion = getCameraDeviceVersion(name, mProviderType);
        if (deviceVersion == CAMERA_DEVICE_API_VERSION_1_0) {
            continue;
        } else if (deviceVersion <= 0) {
            ALOGE("%s: Unsupported device version %d", __func__, deviceVersion);
            ADD_FAILURE();
            return;
        }

        V3_2::Stream previewStream;
        HalStreamConfiguration halStreamConfig;
        sp<ICameraDeviceSession> session;
        sp<DeviceCb> cb;
        bool supportsPartialResults = false;
        bool useHalBufManager = false;
        uint32_t partialResultCount = 0;
        configurePreviewStream(name, deviceVersion, mProvider, &previewThreshold, &session /*out*/,
                &previewStream /*out*/, &halStreamConfig /*out*/,
                &supportsPartialResults /*out*/,
                &partialResultCount /*out*/, &useHalBufManager /*out*/, &cb /*out*/);

        std::shared_ptr<ResultMetadataQueue> resultQueue;
        auto resultQueueRet =
            session->getCaptureResultMetadataQueue(
                [&resultQueue](const auto& descriptor) {
                    resultQueue = std::make_shared<ResultMetadataQueue>(
                            descriptor);
                    if (!resultQueue->isValid() ||
                            resultQueue->availableToWrite() <= 0) {
                        ALOGE("%s: HAL returns empty result metadata fmq,"
                                " not use it", __func__);
                        resultQueue = nullptr;
                        // Don't use the queue onwards.
                    }
                });
        ASSERT_TRUE(resultQueueRet.isOk());

        InFlightRequest inflightReq = {1, false, supportsPartialResults,
                                       partialResultCount, resultQueue};
        RequestTemplate reqTemplate = RequestTemplate::PREVIEW;
        Return<void> ret;
        ret = session->constructDefaultRequestSettings(reqTemplate,
                                                       [&](auto status, const auto& req) {
                                                           ASSERT_EQ(Status::OK, status);
                                                           settings = req;
                                                       });
        ASSERT_TRUE(ret.isOk());

        hidl_handle buffer_handle;
        if (useHalBufManager) {
            bufferId = 0;
        } else {
            allocateGraphicBuffer(previewStream.width, previewStream.height,
                    android_convertGralloc1To0Usage(halStreamConfig.streams[0].producerUsage,
                        halStreamConfig.streams[0].consumerUsage),
                    halStreamConfig.streams[0].overrideFormat, &buffer_handle);
        }

        StreamBuffer outputBuffer = {halStreamConfig.streams[0].id,
                                     bufferId,
                                     buffer_handle,
                                     BufferStatus::OK,
                                     nullptr,
                                     nullptr};
        ::android::hardware::hidl_vec<StreamBuffer> outputBuffers = {outputBuffer};
        const StreamBuffer emptyInputBuffer = {-1, 0, nullptr,
                                               BufferStatus::ERROR, nullptr, nullptr};
        CaptureRequest request = {frameNumber, 0 /* fmqSettingsSize */, settings,
                                  emptyInputBuffer, outputBuffers};

        {
            std::unique_lock<std::mutex> l(mLock);
            mInflightMap.clear();
            mInflightMap.add(frameNumber, &inflightReq);
        }

        Status status = Status::INTERNAL_ERROR;
        uint32_t numRequestProcessed = 0;
        hidl_vec<BufferCache> cachesToRemove;
        ret = session->processCaptureRequest(
            {request}, cachesToRemove, [&status, &numRequestProcessed](auto s,
                    uint32_t n) {
                status = s;
                numRequestProcessed = n;
            });

        ASSERT_TRUE(ret.isOk());
        ASSERT_EQ(Status::OK, status);
        ASSERT_EQ(numRequestProcessed, 1u);
        // Flush before waiting for request to complete.
        Return<Status> returnStatus = session->flush();
        ASSERT_TRUE(returnStatus.isOk());
        ASSERT_EQ(Status::OK, returnStatus);

        {
            std::unique_lock<std::mutex> l(mLock);
            while (!inflightReq.errorCodeValid &&
                   ((0 < inflightReq.numBuffersLeft) ||
                           (!inflightReq.haveResultMetadata))) {
                auto timeout = std::chrono::system_clock::now() +
                               std::chrono::seconds(kStreamBufferTimeoutSec);
                ASSERT_NE(std::cv_status::timeout, mResultCondition.wait_until(l,
                        timeout));
            }

            if (!inflightReq.errorCodeValid) {
                ASSERT_NE(inflightReq.resultOutputBuffers.size(), 0u);
                ASSERT_EQ(previewStream.id, inflightReq.resultOutputBuffers[0].streamId);
            } else {
                switch (inflightReq.errorCode) {
                    case ErrorCode::ERROR_REQUEST:
                    case ErrorCode::ERROR_RESULT:
                    case ErrorCode::ERROR_BUFFER:
                        // Expected
                        break;
                    case ErrorCode::ERROR_DEVICE:
                    default:
                        FAIL() << "Unexpected error:"
                               << static_cast<uint32_t>(inflightReq.errorCode);
                }
            }
        }

        if (useHalBufManager) {
            verifyBuffersReturned(session, deviceVersion, previewStream.id, cb);
        }

        ret = session->close();
        ASSERT_TRUE(ret.isOk());
    }
}

// Verify that camera flushes correctly without any pending requests.
TEST_P(CameraHidlTest, flushEmpty) {
    hidl_vec<hidl_string> cameraDeviceNames = getCameraDeviceNames(mProvider);
    std::vector<AvailableStream> outputPreviewStreams;
    AvailableStream previewThreshold = {kMaxPreviewWidth, kMaxPreviewHeight,
                                        static_cast<int32_t>(PixelFormat::IMPLEMENTATION_DEFINED)};

    for (const auto& name : cameraDeviceNames) {
        int deviceVersion = getCameraDeviceVersion(name, mProviderType);
        if (deviceVersion == CAMERA_DEVICE_API_VERSION_1_0) {
            continue;
        } else if (deviceVersion <= 0) {
            ALOGE("%s: Unsupported device version %d", __func__, deviceVersion);
            ADD_FAILURE();
            return;
        }

        V3_2::Stream previewStream;
        HalStreamConfiguration halStreamConfig;
        sp<ICameraDeviceSession> session;
        sp<DeviceCb> cb;
        bool supportsPartialResults = false;
        bool useHalBufManager = false;
        uint32_t partialResultCount = 0;
        configurePreviewStream(name, deviceVersion, mProvider, &previewThreshold, &session /*out*/,
                &previewStream /*out*/, &halStreamConfig /*out*/,
                &supportsPartialResults /*out*/,
                &partialResultCount /*out*/, &useHalBufManager /*out*/, &cb /*out*/);

        Return<Status> returnStatus = session->flush();
        ASSERT_TRUE(returnStatus.isOk());
        ASSERT_EQ(Status::OK, returnStatus);

        {
            std::unique_lock<std::mutex> l(mLock);
            auto timeout = std::chrono::system_clock::now() +
                           std::chrono::milliseconds(kEmptyFlushTimeoutMSec);
            ASSERT_EQ(std::cv_status::timeout, mResultCondition.wait_until(l, timeout));
        }

        Return<void> ret = session->close();
        ASSERT_TRUE(ret.isOk());
    }
}

// Test camera provider@2.5 notify method
TEST_P(CameraHidlTest, providerDeviceStateNotification) {

    notifyDeviceState(provider::V2_5::DeviceState::BACK_COVERED);
    notifyDeviceState(provider::V2_5::DeviceState::NORMAL);
}

// Retrieve all valid output stream resolutions from the camera
// static characteristics.
Status CameraHidlTest::getAvailableOutputStreams(const camera_metadata_t *staticMeta,
        std::vector<AvailableStream> &outputStreams,
        const AvailableStream *threshold) {
    AvailableStream depthPreviewThreshold = {kMaxPreviewWidth, kMaxPreviewHeight,
                                             static_cast<int32_t>(PixelFormat::Y16)};
    if (nullptr == staticMeta) {
        return Status::ILLEGAL_ARGUMENT;
    }

    camera_metadata_ro_entry scalarEntry;
    camera_metadata_ro_entry depthEntry;
    int foundScalar = find_camera_metadata_ro_entry(staticMeta,
            ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS, &scalarEntry);
    int foundDepth = find_camera_metadata_ro_entry(staticMeta,
            ANDROID_DEPTH_AVAILABLE_DEPTH_STREAM_CONFIGURATIONS, &depthEntry);
    if ((0 != foundScalar || (0 != (scalarEntry.count % 4))) &&
        (0 != foundDepth || (0 != (depthEntry.count % 4)))) {
        return Status::ILLEGAL_ARGUMENT;
    }

    if(foundScalar == 0 && (0 == (scalarEntry.count % 4))) {
        fillOutputStreams(&scalarEntry, outputStreams, threshold,
                ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT);
    }

    if(foundDepth == 0 && (0 == (depthEntry.count % 4))) {
        fillOutputStreams(&depthEntry, outputStreams, &depthPreviewThreshold,
                ANDROID_DEPTH_AVAILABLE_DEPTH_STREAM_CONFIGURATIONS_OUTPUT);
    }

    return Status::OK;
}

static Size getMinSize(Size a, Size b) {
    if (a.width * a.height < b.width * b.height) {
        return a;
    }
    return b;
}

// TODO: Add more combinations
Status CameraHidlTest::getMandatoryConcurrentStreams(const camera_metadata_t* staticMeta,
                                                     std::vector<AvailableStream>* outputStreams) {
    if (nullptr == staticMeta) {
        return Status::ILLEGAL_ARGUMENT;
    }
    Size yuvMaxSize(1280, 720);
    Size jpegMaxSize(1920, 1440);
    Size maxAvailableYuvSize;
    Size maxAvailableJpegSize;
    getMaxOutputSizeForFormat(staticMeta, PixelFormat::YCBCR_420_888, &maxAvailableYuvSize);
    getMaxOutputSizeForFormat(staticMeta, PixelFormat::BLOB, &maxAvailableJpegSize);
    Size yuvChosenSize = getMinSize(yuvMaxSize, maxAvailableYuvSize);
    Size jpegChosenSize = getMinSize(jpegMaxSize, maxAvailableJpegSize);

    AvailableStream yuvStream = {.width = yuvChosenSize.width,
                                 .height = yuvChosenSize.height,
                                 .format = static_cast<int32_t>(PixelFormat::YCBCR_420_888)};

    AvailableStream jpegStream = {.width = jpegChosenSize.width,
                                  .height = jpegChosenSize.height,
                                  .format = static_cast<int32_t>(PixelFormat::BLOB)};
    outputStreams->push_back(yuvStream);
    outputStreams->push_back(jpegStream);

    return Status::OK;
}

Status CameraHidlTest::getMaxOutputSizeForFormat(const camera_metadata_t* staticMeta,
                                                 PixelFormat format, Size* size) {
    std::vector<AvailableStream> outputStreams;
    if (size == nullptr || getAvailableOutputStreams(staticMeta, outputStreams) != Status::OK) {
        return Status::ILLEGAL_ARGUMENT;
    }
    Size maxSize;
    bool found = false;
    for (auto& outputStream : outputStreams) {
        if (static_cast<int32_t>(format) == outputStream.format &&
            (outputStream.width * outputStream.height > maxSize.width * maxSize.height)) {
            maxSize.width = outputStream.width;
            maxSize.height = outputStream.height;
            found = true;
        }
    }
    if (!found) {
        ALOGE("%s :chosen format %d not found", __FUNCTION__, static_cast<int32_t>(format));
        return Status::ILLEGAL_ARGUMENT;
    }
    *size = maxSize;
    return Status::OK;
}

void CameraHidlTest::fillOutputStreams(camera_metadata_ro_entry_t* entry,
        std::vector<AvailableStream>& outputStreams, const AvailableStream* threshold,
        const int32_t availableConfigOutputTag) {
    for (size_t i = 0; i < entry->count; i+=4) {
        if (availableConfigOutputTag == entry->data.i32[i + 3]) {
            if(nullptr == threshold) {
                AvailableStream s = {entry->data.i32[i+1],
                        entry->data.i32[i+2], entry->data.i32[i]};
                outputStreams.push_back(s);
            } else {
                if ((threshold->format == entry->data.i32[i]) &&
                        (threshold->width >= entry->data.i32[i+1]) &&
                        (threshold->height >= entry->data.i32[i+2])) {
                    AvailableStream s = {entry->data.i32[i+1],
                            entry->data.i32[i+2], threshold->format};
                    outputStreams.push_back(s);
                }
            }
        }
    }
}

// Get max jpeg buffer size in android.jpeg.maxSize
Status CameraHidlTest::getJpegBufferSize(camera_metadata_t *staticMeta, uint32_t* outBufSize) {
    if (nullptr == staticMeta || nullptr == outBufSize) {
        return Status::ILLEGAL_ARGUMENT;
    }

    camera_metadata_ro_entry entry;
    int rc = find_camera_metadata_ro_entry(staticMeta,
            ANDROID_JPEG_MAX_SIZE, &entry);
    if ((0 != rc) || (1 != entry.count)) {
        return Status::ILLEGAL_ARGUMENT;
    }

    *outBufSize = static_cast<uint32_t>(entry.data.i32[0]);
    return Status::OK;
}

// Check if the camera device has logical multi-camera capability.
Status CameraHidlTest::isLogicalMultiCamera(const camera_metadata_t *staticMeta) {
    Status ret = Status::METHOD_NOT_SUPPORTED;
    if (nullptr == staticMeta) {
        return Status::ILLEGAL_ARGUMENT;
    }

    camera_metadata_ro_entry entry;
    int rc = find_camera_metadata_ro_entry(staticMeta,
            ANDROID_REQUEST_AVAILABLE_CAPABILITIES, &entry);
    if (0 != rc) {
        return Status::ILLEGAL_ARGUMENT;
    }

    for (size_t i = 0; i < entry.count; i++) {
        if (ANDROID_REQUEST_AVAILABLE_CAPABILITIES_LOGICAL_MULTI_CAMERA == entry.data.u8[i]) {
            ret = Status::OK;
            break;
        }
    }

    return ret;
}

// Check if the camera device has logical multi-camera capability.
Status CameraHidlTest::isOfflineSessionSupported(const camera_metadata_t *staticMeta) {
    Status ret = Status::METHOD_NOT_SUPPORTED;
    if (nullptr == staticMeta) {
        return Status::ILLEGAL_ARGUMENT;
    }

    camera_metadata_ro_entry entry;
    int rc = find_camera_metadata_ro_entry(staticMeta,
            ANDROID_REQUEST_AVAILABLE_CAPABILITIES, &entry);
    if (0 != rc) {
        return Status::ILLEGAL_ARGUMENT;
    }

    for (size_t i = 0; i < entry.count; i++) {
        if (ANDROID_REQUEST_AVAILABLE_CAPABILITIES_OFFLINE_PROCESSING == entry.data.u8[i]) {
            ret = Status::OK;
            break;
        }
    }

    return ret;
}

// Generate a list of physical camera ids backing a logical multi-camera.
Status CameraHidlTest::getPhysicalCameraIds(const camera_metadata_t *staticMeta,
        std::unordered_set<std::string> *physicalIds) {
    if ((nullptr == staticMeta) || (nullptr == physicalIds)) {
        return Status::ILLEGAL_ARGUMENT;
    }

    camera_metadata_ro_entry entry;
    int rc = find_camera_metadata_ro_entry(staticMeta, ANDROID_LOGICAL_MULTI_CAMERA_PHYSICAL_IDS,
            &entry);
    if (0 != rc) {
        return Status::ILLEGAL_ARGUMENT;
    }

    const uint8_t* ids = entry.data.u8;
    size_t start = 0;
    for (size_t i = 0; i < entry.count; i++) {
        if (ids[i] == '\0') {
            if (start != i) {
                std::string currentId(reinterpret_cast<const char *> (ids + start));
                physicalIds->emplace(currentId);
            }
            start = i + 1;
        }
    }

    return Status::OK;
}

// Generate a set of suported camera key ids.
Status CameraHidlTest::getSupportedKeys(camera_metadata_t *staticMeta,
        uint32_t tagId, std::unordered_set<int32_t> *requestIDs) {
    if ((nullptr == staticMeta) || (nullptr == requestIDs)) {
        return Status::ILLEGAL_ARGUMENT;
    }

    camera_metadata_ro_entry entry;
    int rc = find_camera_metadata_ro_entry(staticMeta, tagId, &entry);
    if ((0 != rc) || (entry.count == 0)) {
        return Status::OK;
    }

    requestIDs->insert(entry.data.i32, entry.data.i32 + entry.count);

    return Status::OK;
}

void CameraHidlTest::constructFilteredSettings(const sp<ICameraDeviceSession>& session,
        const std::unordered_set<int32_t>& availableKeys, RequestTemplate reqTemplate,
        android::hardware::camera::common::V1_0::helper::CameraMetadata* defaultSettings,
        android::hardware::camera::common::V1_0::helper::CameraMetadata* filteredSettings) {
    ASSERT_NE(defaultSettings, nullptr);
    ASSERT_NE(filteredSettings, nullptr);

    auto ret = session->constructDefaultRequestSettings(reqTemplate,
            [&defaultSettings] (auto status, const auto& req) mutable {
                ASSERT_EQ(Status::OK, status);

                const camera_metadata_t *metadata = reinterpret_cast<const camera_metadata_t*> (
                        req.data());
                size_t expectedSize = req.size();
                int result = validate_camera_metadata_structure(metadata, &expectedSize);
                ASSERT_TRUE((result == 0) || (result == CAMERA_METADATA_VALIDATION_SHIFTED));

                size_t entryCount = get_camera_metadata_entry_count(metadata);
                ASSERT_GT(entryCount, 0u);
                *defaultSettings = metadata;
                });
    ASSERT_TRUE(ret.isOk());
    const android::hardware::camera::common::V1_0::helper::CameraMetadata &constSettings =
        *defaultSettings;
    for (const auto& keyIt : availableKeys) {
        camera_metadata_ro_entry entry = constSettings.find(keyIt);
        if (entry.count > 0) {
            filteredSettings->update(entry);
        }
    }
}

// Check if constrained mode is supported by using the static
// camera characteristics.
Status CameraHidlTest::isConstrainedModeAvailable(camera_metadata_t *staticMeta) {
    Status ret = Status::METHOD_NOT_SUPPORTED;
    if (nullptr == staticMeta) {
        return Status::ILLEGAL_ARGUMENT;
    }

    camera_metadata_ro_entry entry;
    int rc = find_camera_metadata_ro_entry(staticMeta,
            ANDROID_REQUEST_AVAILABLE_CAPABILITIES, &entry);
    if (0 != rc) {
        return Status::ILLEGAL_ARGUMENT;
    }

    for (size_t i = 0; i < entry.count; i++) {
        if (ANDROID_REQUEST_AVAILABLE_CAPABILITIES_CONSTRAINED_HIGH_SPEED_VIDEO ==
                entry.data.u8[i]) {
            ret = Status::OK;
            break;
        }
    }

    return ret;
}

// Pick the largest supported HFR mode from the static camera
// characteristics.
Status CameraHidlTest::pickConstrainedModeSize(camera_metadata_t *staticMeta,
        AvailableStream &hfrStream) {
    if (nullptr == staticMeta) {
        return Status::ILLEGAL_ARGUMENT;
    }

    camera_metadata_ro_entry entry;
    int rc = find_camera_metadata_ro_entry(staticMeta,
            ANDROID_CONTROL_AVAILABLE_HIGH_SPEED_VIDEO_CONFIGURATIONS, &entry);
    if (0 != rc) {
        return Status::METHOD_NOT_SUPPORTED;
    } else if (0 != (entry.count % 5)) {
        return Status::ILLEGAL_ARGUMENT;
    }

    hfrStream = {0, 0,
            static_cast<uint32_t>(PixelFormat::IMPLEMENTATION_DEFINED)};
    for (size_t i = 0; i < entry.count; i+=5) {
        int32_t w = entry.data.i32[i];
        int32_t h = entry.data.i32[i+1];
        if ((hfrStream.width * hfrStream.height) < (w *h)) {
            hfrStream.width = w;
            hfrStream.height = h;
        }
    }

    return Status::OK;
}

// Check whether ZSL is available using the static camera
// characteristics.
Status CameraHidlTest::isZSLModeAvailable(const camera_metadata_t *staticMeta) {
    if (Status::OK == isZSLModeAvailable(staticMeta, PRIV_REPROCESS)) {
        return Status::OK;
    } else {
        return isZSLModeAvailable(staticMeta, YUV_REPROCESS);
    }
}

Status CameraHidlTest::isZSLModeAvailable(const camera_metadata_t *staticMeta,
        ReprocessType reprocType) {

    Status ret = Status::METHOD_NOT_SUPPORTED;
    if (nullptr == staticMeta) {
        return Status::ILLEGAL_ARGUMENT;
    }

    camera_metadata_ro_entry entry;
    int rc = find_camera_metadata_ro_entry(staticMeta,
            ANDROID_REQUEST_AVAILABLE_CAPABILITIES, &entry);
    if (0 != rc) {
        return Status::ILLEGAL_ARGUMENT;
    }

    for (size_t i = 0; i < entry.count; i++) {
        if ((reprocType == PRIV_REPROCESS &&
                ANDROID_REQUEST_AVAILABLE_CAPABILITIES_PRIVATE_REPROCESSING == entry.data.u8[i]) ||
                (reprocType == YUV_REPROCESS &&
                ANDROID_REQUEST_AVAILABLE_CAPABILITIES_YUV_REPROCESSING == entry.data.u8[i])) {
            ret = Status::OK;
            break;
        }
    }

    return ret;
}

// Check whether this is a monochrome camera using the static camera characteristics.
Status CameraHidlTest::isMonochromeCamera(const camera_metadata_t *staticMeta) {
    Status ret = Status::METHOD_NOT_SUPPORTED;
    if (nullptr == staticMeta) {
        return Status::ILLEGAL_ARGUMENT;
    }

    camera_metadata_ro_entry entry;
    int rc = find_camera_metadata_ro_entry(staticMeta,
            ANDROID_REQUEST_AVAILABLE_CAPABILITIES, &entry);
    if (0 != rc) {
        return Status::ILLEGAL_ARGUMENT;
    }

    for (size_t i = 0; i < entry.count; i++) {
        if (ANDROID_REQUEST_AVAILABLE_CAPABILITIES_MONOCHROME == entry.data.u8[i]) {
            ret = Status::OK;
            break;
        }
    }

    return ret;
}

// Retrieve the reprocess input-output format map from the static
// camera characteristics.
Status CameraHidlTest::getZSLInputOutputMap(camera_metadata_t *staticMeta,
        std::vector<AvailableZSLInputOutput> &inputOutputMap) {
    if (nullptr == staticMeta) {
        return Status::ILLEGAL_ARGUMENT;
    }

    camera_metadata_ro_entry entry;
    int rc = find_camera_metadata_ro_entry(staticMeta,
            ANDROID_SCALER_AVAILABLE_INPUT_OUTPUT_FORMATS_MAP, &entry);
    if ((0 != rc) || (0 >= entry.count)) {
        return Status::ILLEGAL_ARGUMENT;
    }

    const int32_t* contents = &entry.data.i32[0];
    for (size_t i = 0; i < entry.count; ) {
        int32_t inputFormat = contents[i++];
        int32_t length = contents[i++];
        for (int32_t j = 0; j < length; j++) {
            int32_t outputFormat = contents[i+j];
            AvailableZSLInputOutput zslEntry = {inputFormat, outputFormat};
            inputOutputMap.push_back(zslEntry);
        }
        i += length;
    }

    return Status::OK;
}

// Search for the largest stream size for a given format.
Status CameraHidlTest::findLargestSize(
        const std::vector<AvailableStream> &streamSizes, int32_t format,
        AvailableStream &result) {
    result = {0, 0, 0};
    for (auto &iter : streamSizes) {
        if (format == iter.format) {
            if ((result.width * result.height) < (iter.width * iter.height)) {
                result = iter;
            }
        }
    }

    return (result.format == format) ? Status::OK : Status::ILLEGAL_ARGUMENT;
}

// Check whether the camera device supports specific focus mode.
Status CameraHidlTest::isAutoFocusModeAvailable(
        CameraParameters &cameraParams,
        const char *mode) {
    ::android::String8 focusModes(cameraParams.get(
            CameraParameters::KEY_SUPPORTED_FOCUS_MODES));
    if (focusModes.contains(mode)) {
        return Status::OK;
    }

    return Status::METHOD_NOT_SUPPORTED;
}

void CameraHidlTest::createStreamConfiguration(
        const ::android::hardware::hidl_vec<V3_2::Stream>& streams3_2,
        StreamConfigurationMode configMode,
        ::android::hardware::camera::device::V3_2::StreamConfiguration *config3_2 /*out*/,
        ::android::hardware::camera::device::V3_4::StreamConfiguration *config3_4 /*out*/,
        ::android::hardware::camera::device::V3_5::StreamConfiguration *config3_5 /*out*/,
        uint32_t jpegBufferSize) {
    ASSERT_NE(nullptr, config3_2);
    ASSERT_NE(nullptr, config3_4);
    ASSERT_NE(nullptr, config3_5);

    ::android::hardware::hidl_vec<V3_4::Stream> streams3_4(streams3_2.size());
    size_t idx = 0;
    for (auto& stream3_2 : streams3_2) {
        V3_4::Stream stream;
        stream.v3_2 = stream3_2;
        stream.bufferSize = 0;
        if (stream3_2.format == PixelFormat::BLOB &&
                stream3_2.dataSpace == static_cast<V3_2::DataspaceFlags>(Dataspace::V0_JFIF)) {
            stream.bufferSize = jpegBufferSize;
        }
        streams3_4[idx++] = stream;
    }
    // Caller is responsible to fill in non-zero config3_5->streamConfigCounter after this returns
    *config3_5 = {{streams3_4, configMode, {}}, 0};
    *config3_4 = config3_5->v3_4;
    *config3_2 = {streams3_2, configMode};
}

// Configure multiple preview streams using different physical ids.
void CameraHidlTest::configurePreviewStreams3_4(const std::string &name, int32_t deviceVersion,
        sp<ICameraProvider> provider,
        const AvailableStream *previewThreshold,
        const std::unordered_set<std::string>& physicalIds,
        sp<device::V3_4::ICameraDeviceSession> *session3_4 /*out*/,
        sp<device::V3_5::ICameraDeviceSession> *session3_5 /*out*/,
        V3_2::Stream *previewStream /*out*/,
        device::V3_4::HalStreamConfiguration *halStreamConfig /*out*/,
        bool *supportsPartialResults /*out*/,
        uint32_t *partialResultCount /*out*/,
        bool *useHalBufManager /*out*/,
        sp<DeviceCb> *outCb /*out*/,
        uint32_t streamConfigCounter,
        bool allowUnsupport) {
    ASSERT_NE(nullptr, session3_4);
    ASSERT_NE(nullptr, session3_5);
    ASSERT_NE(nullptr, halStreamConfig);
    ASSERT_NE(nullptr, previewStream);
    ASSERT_NE(nullptr, supportsPartialResults);
    ASSERT_NE(nullptr, partialResultCount);
    ASSERT_NE(nullptr, useHalBufManager);
    ASSERT_NE(nullptr, outCb);
    ASSERT_FALSE(physicalIds.empty());

    std::vector<AvailableStream> outputPreviewStreams;
    ::android::sp<ICameraDevice> device3_x;
    ALOGI("configureStreams: Testing camera device %s", name.c_str());
    Return<void> ret;
    ret = provider->getCameraDeviceInterface_V3_x(
        name,
        [&](auto status, const auto& device) {
            ALOGI("getCameraDeviceInterface_V3_x returns status:%d",
                  (int)status);
            ASSERT_EQ(Status::OK, status);
            ASSERT_NE(device, nullptr);
            device3_x = device;
        });
    ASSERT_TRUE(ret.isOk());

    camera_metadata_t *staticMeta;
    ret = device3_x->getCameraCharacteristics([&] (Status s,
            CameraMetadata metadata) {
        ASSERT_EQ(Status::OK, s);
        staticMeta = clone_camera_metadata(
                reinterpret_cast<const camera_metadata_t*>(metadata.data()));
        ASSERT_NE(nullptr, staticMeta);
    });
    ASSERT_TRUE(ret.isOk());

    camera_metadata_ro_entry entry;
    auto status = find_camera_metadata_ro_entry(staticMeta,
            ANDROID_REQUEST_PARTIAL_RESULT_COUNT, &entry);
    if ((0 == status) && (entry.count > 0)) {
        *partialResultCount = entry.data.i32[0];
        *supportsPartialResults = (*partialResultCount > 1);
    }

    sp<DeviceCb> cb = new DeviceCb(this, deviceVersion, staticMeta);
    sp<ICameraDeviceSession> session;
    ret = device3_x->open(
        cb,
        [&session](auto status, const auto& newSession) {
            ALOGI("device::open returns status:%d", (int)status);
            ASSERT_EQ(Status::OK, status);
            ASSERT_NE(newSession, nullptr);
            session = newSession;
        });
    ASSERT_TRUE(ret.isOk());
    *outCb = cb;

    sp<device::V3_3::ICameraDeviceSession> session3_3;
    sp<device::V3_6::ICameraDeviceSession> session3_6;
    castSession(session, deviceVersion, &session3_3, session3_4, session3_5, &session3_6);
    ASSERT_NE(nullptr, (*session3_4).get());

    *useHalBufManager = false;
    status = find_camera_metadata_ro_entry(staticMeta,
            ANDROID_INFO_SUPPORTED_BUFFER_MANAGEMENT_VERSION, &entry);
    if ((0 == status) && (entry.count == 1)) {
        *useHalBufManager = (entry.data.u8[0] ==
            ANDROID_INFO_SUPPORTED_BUFFER_MANAGEMENT_VERSION_HIDL_DEVICE_3_5);
    }

    outputPreviewStreams.clear();
    auto rc = getAvailableOutputStreams(staticMeta,
            outputPreviewStreams, previewThreshold);
    free_camera_metadata(staticMeta);
    ASSERT_EQ(Status::OK, rc);
    ASSERT_FALSE(outputPreviewStreams.empty());

    ::android::hardware::hidl_vec<V3_4::Stream> streams3_4(physicalIds.size());
    int32_t streamId = 0;
    for (auto const& physicalId : physicalIds) {
        V3_4::Stream stream3_4 = {{streamId, StreamType::OUTPUT,
            static_cast<uint32_t> (outputPreviewStreams[0].width),
            static_cast<uint32_t> (outputPreviewStreams[0].height),
            static_cast<PixelFormat> (outputPreviewStreams[0].format),
            GRALLOC1_CONSUMER_USAGE_HWCOMPOSER, 0, StreamRotation::ROTATION_0},
            physicalId.c_str(), /*bufferSize*/ 0};
        streams3_4[streamId++] = stream3_4;
    }

    ::android::hardware::camera::device::V3_4::StreamConfiguration config3_4;
    ::android::hardware::camera::device::V3_5::StreamConfiguration config3_5;
    config3_4 = {streams3_4, StreamConfigurationMode::NORMAL_MODE, {}};
    RequestTemplate reqTemplate = RequestTemplate::PREVIEW;
    ret = (*session3_4)->constructDefaultRequestSettings(reqTemplate,
            [&config3_4](auto status, const auto& req) {
            ASSERT_EQ(Status::OK, status);
            config3_4.sessionParams = req;
            });
    ASSERT_TRUE(ret.isOk());

    ASSERT_TRUE(!allowUnsupport || deviceVersion == CAMERA_DEVICE_API_VERSION_3_5);
    if (allowUnsupport) {
        sp<device::V3_5::ICameraDevice> cameraDevice3_5;
        castDevice(device3_x, deviceVersion, &cameraDevice3_5);

        bool supported = false;
        ret = cameraDevice3_5->isStreamCombinationSupported(config3_4,
                [&supported](Status s, bool combStatus) {
                    ASSERT_TRUE((Status::OK == s) ||
                            (Status::METHOD_NOT_SUPPORTED == s));
                    if (Status::OK == s) {
                        supported = combStatus;
                    }
                });
        ASSERT_TRUE(ret.isOk());
        // If stream combination is not supported, return null session.
        if (!supported) {
            *session3_5 = nullptr;
            return;
        }
    }

    if (*session3_5 != nullptr) {
        config3_5.v3_4 = config3_4;
        config3_5.streamConfigCounter = streamConfigCounter;
        ret = (*session3_5)->configureStreams_3_5(config3_5,
                [&] (Status s, device::V3_4::HalStreamConfiguration halConfig) {
                    ASSERT_EQ(Status::OK, s);
                    ASSERT_EQ(physicalIds.size(), halConfig.streams.size());
                    *halStreamConfig = halConfig;
                    if (*useHalBufManager) {
                        hidl_vec<V3_4::Stream> streams(physicalIds.size());
                        hidl_vec<V3_2::HalStream> halStreams(physicalIds.size());
                        for (size_t i = 0; i < physicalIds.size(); i++) {
                            streams[i] = streams3_4[i];
                            halStreams[i] = halConfig.streams[i].v3_3.v3_2;
                        }
                        cb->setCurrentStreamConfig(streams, halStreams);
                    }
                });
    } else {
        ret = (*session3_4)->configureStreams_3_4(config3_4,
                [&] (Status s, device::V3_4::HalStreamConfiguration halConfig) {
                ASSERT_EQ(Status::OK, s);
                ASSERT_EQ(physicalIds.size(), halConfig.streams.size());
                *halStreamConfig = halConfig;
                });
    }
    *previewStream = streams3_4[0].v3_2;
    ASSERT_TRUE(ret.isOk());
}

// Configure preview stream with possible offline session support
void CameraHidlTest::configureOfflineStillStream(const std::string &name,
        int32_t deviceVersion,
        sp<ICameraProvider> provider,
        const AvailableStream *threshold,
        sp<device::V3_6::ICameraDeviceSession> *session/*out*/,
        V3_2::Stream *stream /*out*/,
        device::V3_6::HalStreamConfiguration *halStreamConfig /*out*/,
        bool *supportsPartialResults /*out*/,
        uint32_t *partialResultCount /*out*/,
        sp<DeviceCb> *outCb /*out*/,
        uint32_t *jpegBufferSize /*out*/,
        bool *useHalBufManager /*out*/) {
    ASSERT_NE(nullptr, session);
    ASSERT_NE(nullptr, halStreamConfig);
    ASSERT_NE(nullptr, stream);
    ASSERT_NE(nullptr, supportsPartialResults);
    ASSERT_NE(nullptr, partialResultCount);
    ASSERT_NE(nullptr, outCb);
    ASSERT_NE(nullptr, jpegBufferSize);
    ASSERT_NE(nullptr, useHalBufManager);

    std::vector<AvailableStream> outputStreams;
    ::android::sp<device::V3_6::ICameraDevice> cameraDevice;
    ALOGI("configureStreams: Testing camera device %s", name.c_str());
    Return<void> ret;
    ret = provider->getCameraDeviceInterface_V3_x(
        name,
        [&cameraDevice](auto status, const auto& device) {
            ALOGI("getCameraDeviceInterface_V3_x returns status:%d",
                  (int)status);
            ASSERT_EQ(Status::OK, status);
            ASSERT_NE(device, nullptr);
            auto castResult = device::V3_6::ICameraDevice::castFrom(device);
            ASSERT_TRUE(castResult.isOk());
            cameraDevice = castResult;
        });
    ASSERT_TRUE(ret.isOk());

    camera_metadata_t *staticMeta;
    ret = cameraDevice->getCameraCharacteristics([&] (Status s,
            CameraMetadata metadata) {
        ASSERT_EQ(Status::OK, s);
        staticMeta = clone_camera_metadata(
                reinterpret_cast<const camera_metadata_t*>(metadata.data()));
        ASSERT_NE(nullptr, staticMeta);
    });
    ASSERT_TRUE(ret.isOk());

    camera_metadata_ro_entry entry;
    auto status = find_camera_metadata_ro_entry(staticMeta,
            ANDROID_REQUEST_PARTIAL_RESULT_COUNT, &entry);
    if ((0 == status) && (entry.count > 0)) {
        *partialResultCount = entry.data.i32[0];
        *supportsPartialResults = (*partialResultCount > 1);
    }

    *useHalBufManager = false;
    status = find_camera_metadata_ro_entry(staticMeta,
            ANDROID_INFO_SUPPORTED_BUFFER_MANAGEMENT_VERSION, &entry);
    if ((0 == status) && (entry.count == 1)) {
        *useHalBufManager = (entry.data.u8[0] ==
            ANDROID_INFO_SUPPORTED_BUFFER_MANAGEMENT_VERSION_HIDL_DEVICE_3_5);
    }

    auto st = getJpegBufferSize(staticMeta, jpegBufferSize);
    ASSERT_EQ(st, Status::OK);

    sp<DeviceCb> cb = new DeviceCb(this, deviceVersion, staticMeta);
    ret = cameraDevice->open(cb, [&session](auto status, const auto& newSession) {
            ALOGI("device::open returns status:%d", (int)status);
            ASSERT_EQ(Status::OK, status);
            ASSERT_NE(newSession, nullptr);
            auto castResult = device::V3_6::ICameraDeviceSession::castFrom(newSession);
            ASSERT_TRUE(castResult.isOk());
            *session = castResult;
        });
    ASSERT_TRUE(ret.isOk());
    *outCb = cb;

    outputStreams.clear();
    auto rc = getAvailableOutputStreams(staticMeta,
            outputStreams, threshold);
    size_t idx = 0;
    int currLargest = outputStreams[0].width * outputStreams[0].height;
    for (size_t i = 0; i < outputStreams.size(); i++) {
        int area = outputStreams[i].width * outputStreams[i].height;
        if (area > currLargest) {
            idx = i;
            currLargest = area;
        }
    }
    free_camera_metadata(staticMeta);
    ASSERT_EQ(Status::OK, rc);
    ASSERT_FALSE(outputStreams.empty());

    V3_2::DataspaceFlags dataspaceFlag = 0;
    switch (static_cast<PixelFormat>(outputStreams[idx].format)) {
        case PixelFormat::BLOB:
            dataspaceFlag = static_cast<V3_2::DataspaceFlags>(Dataspace::V0_JFIF);
            break;
        case PixelFormat::Y16:
            dataspaceFlag = static_cast<V3_2::DataspaceFlags>(Dataspace::DEPTH);
            break;
        default:
            dataspaceFlag = static_cast<V3_2::DataspaceFlags>(Dataspace::UNKNOWN);
    }

    ::android::hardware::hidl_vec<V3_4::Stream> streams3_4(/*size*/1);
    V3_4::Stream stream3_4 = {{ 0 /*streamId*/, StreamType::OUTPUT,
        static_cast<uint32_t> (outputStreams[idx].width),
        static_cast<uint32_t> (outputStreams[idx].height),
        static_cast<PixelFormat> (outputStreams[idx].format),
        GRALLOC1_CONSUMER_USAGE_CPU_READ, dataspaceFlag, StreamRotation::ROTATION_0},
        nullptr /*physicalId*/, /*bufferSize*/ *jpegBufferSize};
    streams3_4[0] = stream3_4;

    ::android::hardware::camera::device::V3_4::StreamConfiguration config3_4;
    ::android::hardware::camera::device::V3_5::StreamConfiguration config3_5;
    config3_4 = {streams3_4, StreamConfigurationMode::NORMAL_MODE, {}};

    config3_5.v3_4 = config3_4;
    config3_5.streamConfigCounter = 0;
    ret = (*session)->configureStreams_3_6(config3_5,
            [&] (Status s, device::V3_6::HalStreamConfiguration halConfig) {
                ASSERT_EQ(Status::OK, s);
                *halStreamConfig = halConfig;

                if (*useHalBufManager) {
                    hidl_vec<V3_2::HalStream> halStreams3_2(1);
                    halStreams3_2[0] = halConfig.streams[0].v3_4.v3_3.v3_2;
                    cb->setCurrentStreamConfig(streams3_4, halStreams3_2);
                }
            });
    *stream = streams3_4[0].v3_2;
    ASSERT_TRUE(ret.isOk());
}

bool CameraHidlTest::isDepthOnly(camera_metadata_t* staticMeta) {
    camera_metadata_ro_entry scalarEntry;
    camera_metadata_ro_entry depthEntry;

    int rc = find_camera_metadata_ro_entry(
        staticMeta, ANDROID_REQUEST_AVAILABLE_CAPABILITIES, &scalarEntry);
    if (rc == 0) {
        for (uint32_t i = 0; i < scalarEntry.count; i++) {
            if (scalarEntry.data.u8[i] == ANDROID_REQUEST_AVAILABLE_CAPABILITIES_BACKWARD_COMPATIBLE) {
                return false;
            }
        }
    }

    for (uint32_t i = 0; i < scalarEntry.count; i++) {
        if (scalarEntry.data.u8[i] == ANDROID_REQUEST_AVAILABLE_CAPABILITIES_DEPTH_OUTPUT) {

            rc = find_camera_metadata_ro_entry(
                staticMeta, ANDROID_DEPTH_AVAILABLE_DEPTH_STREAM_CONFIGURATIONS, &depthEntry);
            size_t i = 0;
            if (rc == 0 && depthEntry.data.i32[i] == static_cast<int32_t>(PixelFormat::Y16)) {
                // only Depth16 format is supported now
                return true;
            }
            break;
        }
    }

    return false;
}

void CameraHidlTest::updateInflightResultQueue(std::shared_ptr<ResultMetadataQueue> resultQueue) {
    std::unique_lock<std::mutex> l(mLock);
    for (size_t i = 0; i < mInflightMap.size(); i++) {
        auto& req = mInflightMap.editValueAt(i);
        req->resultQueue = resultQueue;
    }
}

// Open a device session and configure a preview stream.
void CameraHidlTest::configurePreviewStream(const std::string &name, int32_t deviceVersion,
        sp<ICameraProvider> provider,
        const AvailableStream *previewThreshold,
        sp<ICameraDeviceSession> *session /*out*/,
        V3_2::Stream *previewStream /*out*/,
        HalStreamConfiguration *halStreamConfig /*out*/,
        bool *supportsPartialResults /*out*/,
        uint32_t *partialResultCount /*out*/,
        bool *useHalBufManager /*out*/,
        sp<DeviceCb> *outCb /*out*/,
        uint32_t streamConfigCounter) {
    ASSERT_NE(nullptr, session);
    ASSERT_NE(nullptr, previewStream);
    ASSERT_NE(nullptr, halStreamConfig);
    ASSERT_NE(nullptr, supportsPartialResults);
    ASSERT_NE(nullptr, partialResultCount);
    ASSERT_NE(nullptr, useHalBufManager);
    ASSERT_NE(nullptr, outCb);

    std::vector<AvailableStream> outputPreviewStreams;
    ::android::sp<ICameraDevice> device3_x;
    ALOGI("configureStreams: Testing camera device %s", name.c_str());
    Return<void> ret;
    ret = provider->getCameraDeviceInterface_V3_x(
        name,
        [&](auto status, const auto& device) {
            ALOGI("getCameraDeviceInterface_V3_x returns status:%d",
                  (int)status);
            ASSERT_EQ(Status::OK, status);
            ASSERT_NE(device, nullptr);
            device3_x = device;
        });
    ASSERT_TRUE(ret.isOk());

    camera_metadata_t *staticMeta;
    ret = device3_x->getCameraCharacteristics([&] (Status s,
            CameraMetadata metadata) {
        ASSERT_EQ(Status::OK, s);
        staticMeta = clone_camera_metadata(
                reinterpret_cast<const camera_metadata_t*>(metadata.data()));
        ASSERT_NE(nullptr, staticMeta);
    });
    ASSERT_TRUE(ret.isOk());

    camera_metadata_ro_entry entry;
    auto status = find_camera_metadata_ro_entry(staticMeta,
            ANDROID_REQUEST_PARTIAL_RESULT_COUNT, &entry);
    if ((0 == status) && (entry.count > 0)) {
        *partialResultCount = entry.data.i32[0];
        *supportsPartialResults = (*partialResultCount > 1);
    }

    sp<DeviceCb> cb = new DeviceCb(this, deviceVersion, staticMeta);
    ret = device3_x->open(
        cb,
        [&](auto status, const auto& newSession) {
            ALOGI("device::open returns status:%d", (int)status);
            ASSERT_EQ(Status::OK, status);
            ASSERT_NE(newSession, nullptr);
            *session = newSession;
        });
    ASSERT_TRUE(ret.isOk());
    *outCb = cb;

    sp<device::V3_3::ICameraDeviceSession> session3_3;
    sp<device::V3_4::ICameraDeviceSession> session3_4;
    sp<device::V3_5::ICameraDeviceSession> session3_5;
    sp<device::V3_6::ICameraDeviceSession> session3_6;
    castSession(*session, deviceVersion, &session3_3, &session3_4, &session3_5, &session3_6);

    *useHalBufManager = false;
    status = find_camera_metadata_ro_entry(staticMeta,
            ANDROID_INFO_SUPPORTED_BUFFER_MANAGEMENT_VERSION, &entry);
    if ((0 == status) && (entry.count == 1)) {
        *useHalBufManager = (entry.data.u8[0] ==
            ANDROID_INFO_SUPPORTED_BUFFER_MANAGEMENT_VERSION_HIDL_DEVICE_3_5);
    }

    outputPreviewStreams.clear();
    auto rc = getAvailableOutputStreams(staticMeta,
            outputPreviewStreams, previewThreshold);

    uint32_t jpegBufferSize = 0;
    ASSERT_EQ(Status::OK, getJpegBufferSize(staticMeta, &jpegBufferSize));
    ASSERT_NE(0u, jpegBufferSize);

    free_camera_metadata(staticMeta);
    ASSERT_EQ(Status::OK, rc);
    ASSERT_FALSE(outputPreviewStreams.empty());

    V3_2::DataspaceFlags dataspaceFlag = 0;
    switch (static_cast<PixelFormat>(outputPreviewStreams[0].format)) {
        case PixelFormat::Y16:
            dataspaceFlag = static_cast<V3_2::DataspaceFlags>(Dataspace::DEPTH);
            break;
        default:
            dataspaceFlag = static_cast<V3_2::DataspaceFlags>(Dataspace::UNKNOWN);
    }

    V3_2::Stream stream3_2 = {0, StreamType::OUTPUT,
            static_cast<uint32_t> (outputPreviewStreams[0].width),
            static_cast<uint32_t> (outputPreviewStreams[0].height),
            static_cast<PixelFormat> (outputPreviewStreams[0].format),
            GRALLOC1_CONSUMER_USAGE_HWCOMPOSER, dataspaceFlag, StreamRotation::ROTATION_0};
    ::android::hardware::hidl_vec<V3_2::Stream> streams3_2 = {stream3_2};
    ::android::hardware::camera::device::V3_2::StreamConfiguration config3_2;
    ::android::hardware::camera::device::V3_4::StreamConfiguration config3_4;
    ::android::hardware::camera::device::V3_5::StreamConfiguration config3_5;
    createStreamConfiguration(streams3_2, StreamConfigurationMode::NORMAL_MODE,
                              &config3_2, &config3_4, &config3_5, jpegBufferSize);
    if (session3_5 != nullptr) {
        RequestTemplate reqTemplate = RequestTemplate::PREVIEW;
        ret = session3_5->constructDefaultRequestSettings(reqTemplate,
                                                       [&config3_5](auto status, const auto& req) {
                                                           ASSERT_EQ(Status::OK, status);
                                                           config3_5.v3_4.sessionParams = req;
                                                       });
        ASSERT_TRUE(ret.isOk());
        config3_5.streamConfigCounter = streamConfigCounter;
        ret = session3_5->configureStreams_3_5(config3_5,
                [&] (Status s, device::V3_4::HalStreamConfiguration halConfig) {
                    ASSERT_EQ(Status::OK, s);
                    ASSERT_EQ(1u, halConfig.streams.size());
                    halStreamConfig->streams.resize(1);
                    halStreamConfig->streams[0] = halConfig.streams[0].v3_3.v3_2;
                    if (*useHalBufManager) {
                        hidl_vec<V3_4::Stream> streams(1);
                        hidl_vec<V3_2::HalStream> halStreams(1);
                        streams[0] = config3_4.streams[0];
                        halStreams[0] = halConfig.streams[0].v3_3.v3_2;
                        cb->setCurrentStreamConfig(streams, halStreams);
                    }
                });
    } else if (session3_4 != nullptr) {
        RequestTemplate reqTemplate = RequestTemplate::PREVIEW;
        ret = session3_4->constructDefaultRequestSettings(reqTemplate,
                                                       [&config3_4](auto status, const auto& req) {
                                                           ASSERT_EQ(Status::OK, status);
                                                           config3_4.sessionParams = req;
                                                       });
        ASSERT_TRUE(ret.isOk());
        ret = session3_4->configureStreams_3_4(config3_4,
                [&] (Status s, device::V3_4::HalStreamConfiguration halConfig) {
                    ASSERT_EQ(Status::OK, s);
                    ASSERT_EQ(1u, halConfig.streams.size());
                    halStreamConfig->streams.resize(halConfig.streams.size());
                    for (size_t i = 0; i < halConfig.streams.size(); i++) {
                        halStreamConfig->streams[i] = halConfig.streams[i].v3_3.v3_2;
                    }
                });
    } else if (session3_3 != nullptr) {
        ret = session3_3->configureStreams_3_3(config3_2,
                [&] (Status s, device::V3_3::HalStreamConfiguration halConfig) {
                    ASSERT_EQ(Status::OK, s);
                    ASSERT_EQ(1u, halConfig.streams.size());
                    halStreamConfig->streams.resize(halConfig.streams.size());
                    for (size_t i = 0; i < halConfig.streams.size(); i++) {
                        halStreamConfig->streams[i] = halConfig.streams[i].v3_2;
                    }
                });
    } else {
        ret = (*session)->configureStreams(config3_2,
                [&] (Status s, HalStreamConfiguration halConfig) {
                    ASSERT_EQ(Status::OK, s);
                    ASSERT_EQ(1u, halConfig.streams.size());
                    *halStreamConfig = halConfig;
                });
    }
    *previewStream = stream3_2;
    ASSERT_TRUE(ret.isOk());
}

void CameraHidlTest::castDevice(const sp<device::V3_2::ICameraDevice> &device,
        int32_t deviceVersion, sp<device::V3_5::ICameraDevice> *device3_5/*out*/) {
    ASSERT_NE(nullptr, device3_5);
    if (deviceVersion == CAMERA_DEVICE_API_VERSION_3_5) {
        auto castResult = device::V3_5::ICameraDevice::castFrom(device);
        ASSERT_TRUE(castResult.isOk());
        *device3_5 = castResult;
    }
}

//Cast camera provider to corresponding version if available
void CameraHidlTest::castProvider(const sp<ICameraProvider>& provider,
                                  sp<provider::V2_5::ICameraProvider>* provider2_5 /*out*/,
                                  sp<provider::V2_6::ICameraProvider>* provider2_6 /*out*/) {
    ASSERT_NE(nullptr, provider2_5);
    auto castResult2_5 = provider::V2_5::ICameraProvider::castFrom(provider);
    if (castResult2_5.isOk()) {
        *provider2_5 = castResult2_5;
    }

    ASSERT_NE(nullptr, provider2_6);
    auto castResult2_6 = provider::V2_6::ICameraProvider::castFrom(provider);
    if (castResult2_6.isOk()) {
        *provider2_6 = castResult2_6;
    }
}

//Cast camera device session to corresponding version
void CameraHidlTest::castSession(const sp<ICameraDeviceSession> &session, int32_t deviceVersion,
        sp<device::V3_3::ICameraDeviceSession> *session3_3 /*out*/,
        sp<device::V3_4::ICameraDeviceSession> *session3_4 /*out*/,
        sp<device::V3_5::ICameraDeviceSession> *session3_5 /*out*/,
        sp<device::V3_6::ICameraDeviceSession> *session3_6 /*out*/) {
    ASSERT_NE(nullptr, session3_3);
    ASSERT_NE(nullptr, session3_4);
    ASSERT_NE(nullptr, session3_5);
    ASSERT_NE(nullptr, session3_6);

    switch (deviceVersion) {
        case CAMERA_DEVICE_API_VERSION_3_6: {
            auto castResult = device::V3_6::ICameraDeviceSession::castFrom(session);
            ASSERT_TRUE(castResult.isOk());
            *session3_6 = castResult;
        }
        [[fallthrough]];
        case CAMERA_DEVICE_API_VERSION_3_5: {
            auto castResult = device::V3_5::ICameraDeviceSession::castFrom(session);
            ASSERT_TRUE(castResult.isOk());
            *session3_5 = castResult;
        }
        [[fallthrough]];
        case CAMERA_DEVICE_API_VERSION_3_4: {
            auto castResult = device::V3_4::ICameraDeviceSession::castFrom(session);
            ASSERT_TRUE(castResult.isOk());
            *session3_4 = castResult;
        }
        [[fallthrough]];
        case CAMERA_DEVICE_API_VERSION_3_3: {
            auto castResult = device::V3_3::ICameraDeviceSession::castFrom(session);
            ASSERT_TRUE(castResult.isOk());
            *session3_3 = castResult;
            break;
        }
        default:
            //no-op
            return;
    }
}

void CameraHidlTest::verifyStreamCombination(sp<device::V3_5::ICameraDevice> cameraDevice3_5,
        const ::android::hardware::camera::device::V3_4::StreamConfiguration &config3_4,
        bool expectedStatus, bool expectMethodSupported) {
    if (cameraDevice3_5.get() != nullptr) {
        auto ret = cameraDevice3_5->isStreamCombinationSupported(config3_4,
                [expectedStatus, expectMethodSupported] (Status s, bool combStatus) {
                    ASSERT_TRUE((Status::OK == s) ||
                            (!expectMethodSupported && Status::METHOD_NOT_SUPPORTED == s));
                    if (Status::OK == s) {
                        ASSERT_TRUE(combStatus == expectedStatus);
                    }
                });
        ASSERT_TRUE(ret.isOk());
    }
}

// Verify logical camera static metadata
void CameraHidlTest::verifyLogicalCameraMetadata(const std::string& cameraName,
        const ::android::sp<::android::hardware::camera::device::V3_2::ICameraDevice>& device,
        const CameraMetadata &chars, int deviceVersion,
        const hidl_vec<hidl_string>& deviceNames) {
    const camera_metadata_t* metadata = (camera_metadata_t*)chars.data();
    ASSERT_NE(nullptr, metadata);

    Status rc = isLogicalMultiCamera(metadata);
    ASSERT_TRUE(Status::OK == rc || Status::METHOD_NOT_SUPPORTED == rc);
    if (Status::METHOD_NOT_SUPPORTED == rc) {
        return;
    }

    camera_metadata_ro_entry entry;
    int retcode = find_camera_metadata_ro_entry(metadata,
            ANDROID_CONTROL_ZOOM_RATIO_RANGE, &entry);
    bool hasZoomRatioRange = (0 == retcode && entry.count == 2);

    std::string version, cameraId;
    ASSERT_TRUE(::matchDeviceName(cameraName, mProviderType, &version, &cameraId));
    std::unordered_set<std::string> physicalIds;
    ASSERT_TRUE(Status::OK == getPhysicalCameraIds(metadata, &physicalIds));
    for (auto physicalId : physicalIds) {
        ASSERT_NE(physicalId, cameraId);
        bool isPublicId = false;
        std::string fullPublicId;
        for (auto& deviceName : deviceNames) {
            std::string publicVersion, publicId;
            ASSERT_TRUE(::matchDeviceName(deviceName, mProviderType, &publicVersion, &publicId));
            if (physicalId == publicId) {
                isPublicId = true;
                fullPublicId = deviceName;
                break;
            }
        }
        if (isPublicId) {
            ::android::sp<::android::hardware::camera::device::V3_2::ICameraDevice> subDevice;
            Return<void> ret;
            ret = mProvider->getCameraDeviceInterface_V3_x(
                fullPublicId, [&](auto status, const auto& device) {
                    ASSERT_EQ(Status::OK, status);
                    ASSERT_NE(device, nullptr);
                    subDevice = device;
                });
            ASSERT_TRUE(ret.isOk());

            ret = subDevice->getCameraCharacteristics(
                    [&](auto status, const auto& chars) {
                ASSERT_EQ(Status::OK, status);
                retcode = find_camera_metadata_ro_entry(
                        (const camera_metadata_t *)chars.data(),
                        ANDROID_CONTROL_ZOOM_RATIO_RANGE, &entry);
                bool subCameraHasZoomRatioRange = (0 == retcode && entry.count == 2);
                ASSERT_EQ(hasZoomRatioRange, subCameraHasZoomRatioRange);
            });
            ASSERT_TRUE(ret.isOk());
            continue;
        }

        ASSERT_TRUE(deviceVersion >= CAMERA_DEVICE_API_VERSION_3_5);
        auto castResult = device::V3_5::ICameraDevice::castFrom(device);
        ASSERT_TRUE(castResult.isOk());
        ::android::sp<::android::hardware::camera::device::V3_5::ICameraDevice> device3_5 =
                castResult;
        ASSERT_NE(device3_5, nullptr);

        // Check camera characteristics for hidden camera id
        Return<void> ret = device3_5->getPhysicalCameraCharacteristics(physicalId,
                [&](auto status, const auto& chars) {
            verifyCameraCharacteristics(status, chars);
            verifyMonochromeCharacteristics(chars, deviceVersion);

            retcode = find_camera_metadata_ro_entry(
                    (const camera_metadata_t *)chars.data(),
                    ANDROID_CONTROL_ZOOM_RATIO_RANGE, &entry);
            bool subCameraHasZoomRatioRange = (0 == retcode && entry.count == 2);
            ASSERT_EQ(hasZoomRatioRange, subCameraHasZoomRatioRange);
        });
        ASSERT_TRUE(ret.isOk());

        // Check calling getCameraDeviceInterface_V3_x() on hidden camera id returns
        // ILLEGAL_ARGUMENT.
        std::stringstream s;
        s << "device@" << version << "/" << mProviderType << "/" << physicalId;
        hidl_string fullPhysicalId(s.str());
        ret = mProvider->getCameraDeviceInterface_V3_x(fullPhysicalId,
                [&](auto status, const auto& device3_x) {
            ASSERT_EQ(Status::ILLEGAL_ARGUMENT, status);
            ASSERT_EQ(device3_x, nullptr);
        });
        ASSERT_TRUE(ret.isOk());
    }

    // Make sure ANDROID_LOGICAL_MULTI_CAMERA_ACTIVE_PHYSICAL_ID is available in
    // result keys.
    if (deviceVersion >= CAMERA_DEVICE_API_VERSION_3_5) {
        retcode = find_camera_metadata_ro_entry(metadata,
                ANDROID_REQUEST_AVAILABLE_RESULT_KEYS, &entry);
        if ((0 == retcode) && (entry.count > 0)) {
                ASSERT_NE(std::find(entry.data.i32, entry.data.i32 + entry.count,
                    static_cast<int32_t>(
                            CameraMetadataTag::ANDROID_LOGICAL_MULTI_CAMERA_ACTIVE_PHYSICAL_ID)),
                    entry.data.i32 + entry.count);
        } else {
            ADD_FAILURE() << "Get camera availableResultKeys failed!";
        }
    }
}

void CameraHidlTest::verifyCameraCharacteristics(Status status, const CameraMetadata& chars) {
    ASSERT_EQ(Status::OK, status);
    const camera_metadata_t* metadata = (camera_metadata_t*)chars.data();
    size_t expectedSize = chars.size();
    int result = validate_camera_metadata_structure(metadata, &expectedSize);
    ASSERT_TRUE((result == 0) || (result == CAMERA_METADATA_VALIDATION_SHIFTED));
    size_t entryCount = get_camera_metadata_entry_count(metadata);
    // TODO: we can do better than 0 here. Need to check how many required
    // characteristics keys we've defined.
    ASSERT_GT(entryCount, 0u);

    camera_metadata_ro_entry entry;
    int retcode = find_camera_metadata_ro_entry(metadata,
            ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL, &entry);
    if ((0 == retcode) && (entry.count > 0)) {
        uint8_t hardwareLevel = entry.data.u8[0];
        ASSERT_TRUE(
                hardwareLevel == ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_LIMITED ||
                hardwareLevel == ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_FULL ||
                hardwareLevel == ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_3 ||
                hardwareLevel == ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_EXTERNAL);
    } else {
        ADD_FAILURE() << "Get camera hardware level failed!";
    }

    entry.count = 0;
    retcode = find_camera_metadata_ro_entry(metadata,
            ANDROID_REQUEST_CHARACTERISTIC_KEYS_NEEDING_PERMISSION, &entry);
    if ((0 == retcode) || (entry.count > 0)) {
        ADD_FAILURE() << "ANDROID_REQUEST_CHARACTERISTIC_KEYS_NEEDING_PERMISSION "
            << " per API contract should never be set by Hal!";
    }
    retcode = find_camera_metadata_ro_entry(metadata,
            ANDROID_DEPTH_AVAILABLE_DYNAMIC_DEPTH_STREAM_CONFIGURATIONS, &entry);
    if ((0 == retcode) || (entry.count > 0)) {
        ADD_FAILURE() << "ANDROID_DEPTH_AVAILABLE_DYNAMIC_DEPTH_STREAM_CONFIGURATIONS"
            << " per API contract should never be set by Hal!";
    }
    retcode = find_camera_metadata_ro_entry(metadata,
            ANDROID_DEPTH_AVAILABLE_DYNAMIC_DEPTH_MIN_FRAME_DURATIONS, &entry);
    if ((0 == retcode) || (entry.count > 0)) {
        ADD_FAILURE() << "ANDROID_DEPTH_AVAILABLE_DYNAMIC_DEPTH_MIN_FRAME_DURATIONS"
            << " per API contract should never be set by Hal!";
    }
    retcode = find_camera_metadata_ro_entry(metadata,
            ANDROID_DEPTH_AVAILABLE_DYNAMIC_DEPTH_STALL_DURATIONS, &entry);
    if ((0 == retcode) || (entry.count > 0)) {
        ADD_FAILURE() << "ANDROID_DEPTH_AVAILABLE_DYNAMIC_DEPTH_STALL_DURATIONS"
            << " per API contract should never be set by Hal!";
    }

    retcode = find_camera_metadata_ro_entry(metadata,
            ANDROID_HEIC_AVAILABLE_HEIC_STREAM_CONFIGURATIONS, &entry);
    if (0 == retcode || entry.count > 0) {
        ADD_FAILURE() << "ANDROID_HEIC_AVAILABLE_HEIC_STREAM_CONFIGURATIONS "
            << " per API contract should never be set by Hal!";
    }

    retcode = find_camera_metadata_ro_entry(metadata,
            ANDROID_HEIC_AVAILABLE_HEIC_MIN_FRAME_DURATIONS, &entry);
    if (0 == retcode || entry.count > 0) {
        ADD_FAILURE() << "ANDROID_HEIC_AVAILABLE_HEIC_MIN_FRAME_DURATIONS "
            << " per API contract should never be set by Hal!";
    }

    retcode = find_camera_metadata_ro_entry(metadata,
            ANDROID_HEIC_AVAILABLE_HEIC_STALL_DURATIONS, &entry);
    if (0 == retcode || entry.count > 0) {
        ADD_FAILURE() << "ANDROID_HEIC_AVAILABLE_HEIC_STALL_DURATIONS "
            << " per API contract should never be set by Hal!";
    }

    retcode = find_camera_metadata_ro_entry(metadata,
            ANDROID_HEIC_INFO_SUPPORTED, &entry);
    if (0 == retcode && entry.count > 0) {
        retcode = find_camera_metadata_ro_entry(metadata,
            ANDROID_HEIC_INFO_MAX_JPEG_APP_SEGMENTS_COUNT, &entry);
        if (0 == retcode && entry.count > 0) {
            uint8_t maxJpegAppSegmentsCount = entry.data.u8[0];
            ASSERT_TRUE(maxJpegAppSegmentsCount >= 1 &&
                    maxJpegAppSegmentsCount <= 16);
        } else {
            ADD_FAILURE() << "Get Heic maxJpegAppSegmentsCount failed!";
        }
    }

    retcode = find_camera_metadata_ro_entry(metadata,
            ANDROID_LENS_POSE_REFERENCE, &entry);
    if (0 == retcode && entry.count > 0) {
        uint8_t poseReference = entry.data.u8[0];
        ASSERT_TRUE(poseReference <= ANDROID_LENS_POSE_REFERENCE_UNDEFINED &&
                poseReference >= ANDROID_LENS_POSE_REFERENCE_PRIMARY_CAMERA);
    }

    verifyExtendedSceneModeCharacteristics(metadata);
    verifyZoomCharacteristics(metadata);
}

void CameraHidlTest::verifyExtendedSceneModeCharacteristics(const camera_metadata_t* metadata) {
    camera_metadata_ro_entry entry;
    int retcode = 0;

    retcode = find_camera_metadata_ro_entry(metadata, ANDROID_CONTROL_AVAILABLE_MODES, &entry);
    if ((0 == retcode) && (entry.count > 0)) {
        for (auto i = 0; i < entry.count; i++) {
            ASSERT_TRUE(entry.data.u8[i] >= ANDROID_CONTROL_MODE_OFF &&
                        entry.data.u8[i] <= ANDROID_CONTROL_MODE_USE_EXTENDED_SCENE_MODE);
        }
    } else {
        ADD_FAILURE() << "Get camera controlAvailableModes failed!";
    }

    // Check key availability in capabilities, request and result.

    retcode = find_camera_metadata_ro_entry(metadata,
            ANDROID_REQUEST_AVAILABLE_REQUEST_KEYS, &entry);
    bool hasExtendedSceneModeRequestKey = false;
    if ((0 == retcode) && (entry.count > 0)) {
        hasExtendedSceneModeRequestKey =
                std::find(entry.data.i32, entry.data.i32 + entry.count,
                          ANDROID_CONTROL_EXTENDED_SCENE_MODE) != entry.data.i32 + entry.count;
    } else {
        ADD_FAILURE() << "Get camera availableRequestKeys failed!";
    }

    retcode = find_camera_metadata_ro_entry(metadata,
            ANDROID_REQUEST_AVAILABLE_RESULT_KEYS, &entry);
    bool hasExtendedSceneModeResultKey = false;
    if ((0 == retcode) && (entry.count > 0)) {
        hasExtendedSceneModeResultKey =
                std::find(entry.data.i32, entry.data.i32 + entry.count,
                          ANDROID_CONTROL_EXTENDED_SCENE_MODE) != entry.data.i32 + entry.count;
    } else {
        ADD_FAILURE() << "Get camera availableResultKeys failed!";
    }

    retcode = find_camera_metadata_ro_entry(metadata,
            ANDROID_REQUEST_AVAILABLE_CHARACTERISTICS_KEYS, &entry);
    bool hasExtendedSceneModeMaxSizesKey = false;
    bool hasExtendedSceneModeZoomRatioRangesKey = false;
    if ((0 == retcode) && (entry.count > 0)) {
        hasExtendedSceneModeMaxSizesKey =
                std::find(entry.data.i32, entry.data.i32 + entry.count,
                          ANDROID_CONTROL_AVAILABLE_EXTENDED_SCENE_MODE_MAX_SIZES) !=
                entry.data.i32 + entry.count;
        hasExtendedSceneModeZoomRatioRangesKey =
                std::find(entry.data.i32, entry.data.i32 + entry.count,
                          ANDROID_CONTROL_AVAILABLE_EXTENDED_SCENE_MODE_ZOOM_RATIO_RANGES) !=
                entry.data.i32 + entry.count;
    } else {
        ADD_FAILURE() << "Get camera availableCharacteristicsKeys failed!";
    }

    camera_metadata_ro_entry maxSizesEntry;
    retcode = find_camera_metadata_ro_entry(
            metadata, ANDROID_CONTROL_AVAILABLE_EXTENDED_SCENE_MODE_MAX_SIZES, &maxSizesEntry);
    bool hasExtendedSceneModeMaxSizes = (0 == retcode && maxSizesEntry.count > 0);

    camera_metadata_ro_entry zoomRatioRangesEntry;
    retcode = find_camera_metadata_ro_entry(
            metadata, ANDROID_CONTROL_AVAILABLE_EXTENDED_SCENE_MODE_ZOOM_RATIO_RANGES,
            &zoomRatioRangesEntry);
    bool hasExtendedSceneModeZoomRatioRanges = (0 == retcode && zoomRatioRangesEntry.count > 0);

    // Extended scene mode keys must all be available, or all be unavailable.
    bool noExtendedSceneMode =
            !hasExtendedSceneModeRequestKey && !hasExtendedSceneModeResultKey &&
            !hasExtendedSceneModeMaxSizesKey && !hasExtendedSceneModeZoomRatioRangesKey &&
            !hasExtendedSceneModeMaxSizes && !hasExtendedSceneModeZoomRatioRanges;
    if (noExtendedSceneMode) {
        return;
    }
    bool hasExtendedSceneMode = hasExtendedSceneModeRequestKey && hasExtendedSceneModeResultKey &&
                                hasExtendedSceneModeMaxSizesKey &&
                                hasExtendedSceneModeZoomRatioRangesKey &&
                                hasExtendedSceneModeMaxSizes && hasExtendedSceneModeZoomRatioRanges;
    ASSERT_TRUE(hasExtendedSceneMode);

    // Must have DISABLED, and must have one of BOKEH_STILL_CAPTURE, BOKEH_CONTINUOUS, or a VENDOR
    // mode.
    ASSERT_TRUE((maxSizesEntry.count == 6 && zoomRatioRangesEntry.count == 2) ||
            (maxSizesEntry.count == 9 && zoomRatioRangesEntry.count == 4));
    bool hasDisabledMode = false;
    bool hasBokehStillCaptureMode = false;
    bool hasBokehContinuousMode = false;
    bool hasVendorMode = false;
    std::vector<AvailableStream> outputStreams;
    ASSERT_EQ(Status::OK, getAvailableOutputStreams(metadata, outputStreams));
    for (int i = 0, j = 0; i < maxSizesEntry.count && j < zoomRatioRangesEntry.count; i += 3) {
        int32_t mode = maxSizesEntry.data.i32[i];
        int32_t maxWidth = maxSizesEntry.data.i32[i+1];
        int32_t maxHeight = maxSizesEntry.data.i32[i+2];
        switch (mode) {
            case ANDROID_CONTROL_EXTENDED_SCENE_MODE_DISABLED:
                hasDisabledMode = true;
                ASSERT_TRUE(maxWidth == 0 && maxHeight == 0);
                break;
            case ANDROID_CONTROL_EXTENDED_SCENE_MODE_BOKEH_STILL_CAPTURE:
                hasBokehStillCaptureMode = true;
                j += 2;
                break;
            case ANDROID_CONTROL_EXTENDED_SCENE_MODE_BOKEH_CONTINUOUS:
                hasBokehContinuousMode = true;
                j += 2;
                break;
            default:
                if (mode < ANDROID_CONTROL_EXTENDED_SCENE_MODE_VENDOR_START) {
                    ADD_FAILURE() << "Invalid extended scene mode advertised: " << mode;
                } else {
                    hasVendorMode = true;
                    j += 2;
                }
                break;
        }

        if (mode != ANDROID_CONTROL_EXTENDED_SCENE_MODE_DISABLED) {
            // Make sure size is supported.
            bool sizeSupported = false;
            for (const auto& stream : outputStreams) {
                if ((stream.format == static_cast<int32_t>(PixelFormat::YCBCR_420_888) ||
                        stream.format == static_cast<int32_t>(PixelFormat::IMPLEMENTATION_DEFINED))
                        && stream.width == maxWidth && stream.height == maxHeight) {
                    sizeSupported = true;
                    break;
                }
            }
            ASSERT_TRUE(sizeSupported);

            // Make sure zoom range is valid
            float minZoomRatio = zoomRatioRangesEntry.data.f[0];
            float maxZoomRatio = zoomRatioRangesEntry.data.f[1];
            ASSERT_GT(minZoomRatio, 0.0f);
            ASSERT_LE(minZoomRatio, maxZoomRatio);
        }
    }
    ASSERT_TRUE(hasDisabledMode);
    ASSERT_TRUE(hasBokehStillCaptureMode || hasBokehContinuousMode || hasVendorMode);
}

void CameraHidlTest::verifyZoomCharacteristics(const camera_metadata_t* metadata) {
    camera_metadata_ro_entry entry;
    int retcode = 0;

    // Check key availability in capabilities, request and result.
    retcode = find_camera_metadata_ro_entry(metadata,
            ANDROID_SCALER_AVAILABLE_MAX_DIGITAL_ZOOM, &entry);
    float maxDigitalZoom = 1.0;
    if ((0 == retcode) && (entry.count == 1)) {
        maxDigitalZoom = entry.data.f[0];
    } else {
        ADD_FAILURE() << "Get camera scalerAvailableMaxDigitalZoom failed!";
    }

    retcode = find_camera_metadata_ro_entry(metadata,
            ANDROID_REQUEST_AVAILABLE_REQUEST_KEYS, &entry);
    bool hasZoomRequestKey = false;
    if ((0 == retcode) && (entry.count > 0)) {
        hasZoomRequestKey = std::find(entry.data.i32, entry.data.i32+entry.count,
                ANDROID_CONTROL_ZOOM_RATIO) != entry.data.i32+entry.count;
    } else {
        ADD_FAILURE() << "Get camera availableRequestKeys failed!";
    }

    retcode = find_camera_metadata_ro_entry(metadata,
            ANDROID_REQUEST_AVAILABLE_RESULT_KEYS, &entry);
    bool hasZoomResultKey = false;
    if ((0 == retcode) && (entry.count > 0)) {
        hasZoomResultKey = std::find(entry.data.i32, entry.data.i32+entry.count,
                ANDROID_CONTROL_ZOOM_RATIO) != entry.data.i32+entry.count;
    } else {
        ADD_FAILURE() << "Get camera availableResultKeys failed!";
    }

    retcode = find_camera_metadata_ro_entry(metadata,
            ANDROID_REQUEST_AVAILABLE_CHARACTERISTICS_KEYS, &entry);
    bool hasZoomCharacteristicsKey = false;
    if ((0 == retcode) && (entry.count > 0)) {
        hasZoomCharacteristicsKey = std::find(entry.data.i32, entry.data.i32+entry.count,
                ANDROID_CONTROL_ZOOM_RATIO_RANGE) != entry.data.i32+entry.count;
    } else {
        ADD_FAILURE() << "Get camera availableCharacteristicsKeys failed!";
    }

    retcode = find_camera_metadata_ro_entry(metadata,
            ANDROID_CONTROL_ZOOM_RATIO_RANGE, &entry);
    bool hasZoomRatioRange = (0 == retcode && entry.count == 2);

    // Zoom keys must all be available, or all be unavailable.
    bool noZoomRatio = !hasZoomRequestKey && !hasZoomResultKey && !hasZoomCharacteristicsKey &&
            !hasZoomRatioRange;
    if (noZoomRatio) {
        return;
    }
    bool hasZoomRatio = hasZoomRequestKey && hasZoomResultKey && hasZoomCharacteristicsKey &&
            hasZoomRatioRange;
    ASSERT_TRUE(hasZoomRatio);

    float minZoomRatio = entry.data.f[0];
    float maxZoomRatio = entry.data.f[1];
    if (maxDigitalZoom != maxZoomRatio) {
        ADD_FAILURE() << "Maximum zoom ratio is different than maximum digital zoom!";
    }
    if (minZoomRatio > maxZoomRatio) {
        ADD_FAILURE() << "Maximum zoom ratio is less than minimum zoom ratio!";
    }
    if (minZoomRatio > 1.0f) {
        ADD_FAILURE() << "Minimum zoom ratio is more than 1.0!";
    }
    if (maxZoomRatio < 1.0f) {
        ADD_FAILURE() << "Maximum zoom ratio is less than 1.0!";
    }

    // Make sure CROPPING_TYPE is CENTER_ONLY
    retcode = find_camera_metadata_ro_entry(metadata,
            ANDROID_SCALER_CROPPING_TYPE, &entry);
    if ((0 == retcode) && (entry.count == 1)) {
        int8_t croppingType = entry.data.u8[0];
        ASSERT_EQ(croppingType, ANDROID_SCALER_CROPPING_TYPE_CENTER_ONLY);
    } else {
        ADD_FAILURE() << "Get camera scalerCroppingType failed!";
    }
}

void CameraHidlTest::verifyMonochromeCharacteristics(const CameraMetadata& chars,
        int deviceVersion) {
    const camera_metadata_t* metadata = (camera_metadata_t*)chars.data();
    Status rc = isMonochromeCamera(metadata);
    if (Status::METHOD_NOT_SUPPORTED == rc) {
        return;
    }
    ASSERT_EQ(Status::OK, rc);

    camera_metadata_ro_entry entry;
    // Check capabilities
    int retcode = find_camera_metadata_ro_entry(metadata,
                ANDROID_REQUEST_AVAILABLE_CAPABILITIES, &entry);
    if ((0 == retcode) && (entry.count > 0)) {
        ASSERT_EQ(std::find(entry.data.u8, entry.data.u8 + entry.count,
                ANDROID_REQUEST_AVAILABLE_CAPABILITIES_MANUAL_POST_PROCESSING),
                entry.data.u8 + entry.count);
        if (deviceVersion < CAMERA_DEVICE_API_VERSION_3_5) {
            ASSERT_EQ(std::find(entry.data.u8, entry.data.u8 + entry.count,
                    ANDROID_REQUEST_AVAILABLE_CAPABILITIES_RAW),
                    entry.data.u8 + entry.count);
        }
    }

    if (deviceVersion >= CAMERA_DEVICE_API_VERSION_3_5) {
        // Check Cfa
        retcode = find_camera_metadata_ro_entry(metadata,
                ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT, &entry);
        if ((0 == retcode) && (entry.count == 1)) {
            ASSERT_TRUE(entry.data.i32[0] == static_cast<int32_t>(
                    CameraMetadataEnumAndroidSensorInfoColorFilterArrangement::ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_MONO)
                    || entry.data.i32[0] == static_cast<int32_t>(
                    CameraMetadataEnumAndroidSensorInfoColorFilterArrangement::ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_NIR));
        }

        // Check availableRequestKeys
        retcode = find_camera_metadata_ro_entry(metadata,
                ANDROID_REQUEST_AVAILABLE_REQUEST_KEYS, &entry);
        if ((0 == retcode) && (entry.count > 0)) {
            for (size_t i = 0; i < entry.count; i++) {
                ASSERT_NE(entry.data.i32[i], ANDROID_COLOR_CORRECTION_MODE);
                ASSERT_NE(entry.data.i32[i], ANDROID_COLOR_CORRECTION_TRANSFORM);
                ASSERT_NE(entry.data.i32[i], ANDROID_COLOR_CORRECTION_GAINS);
            }
        } else {
            ADD_FAILURE() << "Get camera availableRequestKeys failed!";
        }

        // Check availableResultKeys
        retcode = find_camera_metadata_ro_entry(metadata,
                ANDROID_REQUEST_AVAILABLE_RESULT_KEYS, &entry);
        if ((0 == retcode) && (entry.count > 0)) {
            for (size_t i = 0; i < entry.count; i++) {
                ASSERT_NE(entry.data.i32[i], ANDROID_SENSOR_GREEN_SPLIT);
                ASSERT_NE(entry.data.i32[i], ANDROID_SENSOR_NEUTRAL_COLOR_POINT);
                ASSERT_NE(entry.data.i32[i], ANDROID_COLOR_CORRECTION_MODE);
                ASSERT_NE(entry.data.i32[i], ANDROID_COLOR_CORRECTION_TRANSFORM);
                ASSERT_NE(entry.data.i32[i], ANDROID_COLOR_CORRECTION_GAINS);
            }
        } else {
            ADD_FAILURE() << "Get camera availableResultKeys failed!";
        }

        // Check availableCharacteristicKeys
        retcode = find_camera_metadata_ro_entry(metadata,
                ANDROID_REQUEST_AVAILABLE_CHARACTERISTICS_KEYS, &entry);
        if ((0 == retcode) && (entry.count > 0)) {
            for (size_t i = 0; i < entry.count; i++) {
                ASSERT_NE(entry.data.i32[i], ANDROID_SENSOR_REFERENCE_ILLUMINANT1);
                ASSERT_NE(entry.data.i32[i], ANDROID_SENSOR_REFERENCE_ILLUMINANT2);
                ASSERT_NE(entry.data.i32[i], ANDROID_SENSOR_CALIBRATION_TRANSFORM1);
                ASSERT_NE(entry.data.i32[i], ANDROID_SENSOR_CALIBRATION_TRANSFORM2);
                ASSERT_NE(entry.data.i32[i], ANDROID_SENSOR_COLOR_TRANSFORM1);
                ASSERT_NE(entry.data.i32[i], ANDROID_SENSOR_COLOR_TRANSFORM2);
                ASSERT_NE(entry.data.i32[i], ANDROID_SENSOR_FORWARD_MATRIX1);
                ASSERT_NE(entry.data.i32[i], ANDROID_SENSOR_FORWARD_MATRIX2);
            }
        } else {
            ADD_FAILURE() << "Get camera availableResultKeys failed!";
        }

        // Check blackLevelPattern
        retcode = find_camera_metadata_ro_entry(metadata,
                ANDROID_SENSOR_BLACK_LEVEL_PATTERN, &entry);
        if ((0 == retcode) && (entry.count > 0)) {
            ASSERT_EQ(entry.count, 4);
            for (size_t i = 1; i < entry.count; i++) {
                ASSERT_EQ(entry.data.i32[i], entry.data.i32[0]);
            }
        }
    }
}

void CameraHidlTest::verifyMonochromeCameraResult(
        const ::android::hardware::camera::common::V1_0::helper::CameraMetadata& metadata) {
    camera_metadata_ro_entry entry;

    // Check tags that are not applicable for monochrome camera
    ASSERT_FALSE(metadata.exists(ANDROID_SENSOR_GREEN_SPLIT));
    ASSERT_FALSE(metadata.exists(ANDROID_SENSOR_NEUTRAL_COLOR_POINT));
    ASSERT_FALSE(metadata.exists(ANDROID_COLOR_CORRECTION_MODE));
    ASSERT_FALSE(metadata.exists(ANDROID_COLOR_CORRECTION_TRANSFORM));
    ASSERT_FALSE(metadata.exists(ANDROID_COLOR_CORRECTION_GAINS));

    // Check dynamicBlackLevel
    entry = metadata.find(ANDROID_SENSOR_DYNAMIC_BLACK_LEVEL);
    if (entry.count > 0) {
        ASSERT_EQ(entry.count, 4);
        for (size_t i = 1; i < entry.count; i++) {
            ASSERT_FLOAT_EQ(entry.data.f[i], entry.data.f[0]);
        }
    }

    // Check noiseProfile
    entry = metadata.find(ANDROID_SENSOR_NOISE_PROFILE);
    if (entry.count > 0) {
        ASSERT_EQ(entry.count, 2);
    }

    // Check lensShadingMap
    entry = metadata.find(ANDROID_STATISTICS_LENS_SHADING_MAP);
    if (entry.count > 0) {
        ASSERT_EQ(entry.count % 4, 0);
        for (size_t i = 0; i < entry.count/4; i++) {
            ASSERT_FLOAT_EQ(entry.data.f[i*4+1], entry.data.f[i*4]);
            ASSERT_FLOAT_EQ(entry.data.f[i*4+2], entry.data.f[i*4]);
            ASSERT_FLOAT_EQ(entry.data.f[i*4+3], entry.data.f[i*4]);
        }
    }

    // Check tonemapCurve
    camera_metadata_ro_entry curveRed = metadata.find(ANDROID_TONEMAP_CURVE_RED);
    camera_metadata_ro_entry curveGreen = metadata.find(ANDROID_TONEMAP_CURVE_GREEN);
    camera_metadata_ro_entry curveBlue = metadata.find(ANDROID_TONEMAP_CURVE_BLUE);
    if (curveRed.count > 0 && curveGreen.count > 0 && curveBlue.count > 0) {
        ASSERT_EQ(curveRed.count, curveGreen.count);
        ASSERT_EQ(curveRed.count, curveBlue.count);
        for (size_t i = 0; i < curveRed.count; i++) {
            ASSERT_FLOAT_EQ(curveGreen.data.f[i], curveRed.data.f[i]);
            ASSERT_FLOAT_EQ(curveBlue.data.f[i], curveRed.data.f[i]);
        }
    }
}

void CameraHidlTest::verifyBuffersReturned(
        sp<device::V3_2::ICameraDeviceSession> session,
        int deviceVersion, int32_t streamId,
        sp<DeviceCb> cb, uint32_t streamConfigCounter) {
    sp<device::V3_3::ICameraDeviceSession> session3_3;
    sp<device::V3_4::ICameraDeviceSession> session3_4;
    sp<device::V3_5::ICameraDeviceSession> session3_5;
    sp<device::V3_6::ICameraDeviceSession> session3_6;
    castSession(session, deviceVersion, &session3_3, &session3_4, &session3_5, &session3_6);
    ASSERT_NE(nullptr, session3_5.get());

    hidl_vec<int32_t> streamIds(1);
    streamIds[0] = streamId;
    session3_5->signalStreamFlush(streamIds, /*streamConfigCounter*/streamConfigCounter);
    cb->waitForBuffersReturned();
}

void CameraHidlTest::verifyBuffersReturned(
        sp<device::V3_4::ICameraDeviceSession> session3_4,
        hidl_vec<int32_t> streamIds, sp<DeviceCb> cb, uint32_t streamConfigCounter) {
    auto castResult = device::V3_5::ICameraDeviceSession::castFrom(session3_4);
    ASSERT_TRUE(castResult.isOk());
    sp<device::V3_5::ICameraDeviceSession> session3_5 = castResult;
    ASSERT_NE(nullptr, session3_5.get());

    session3_5->signalStreamFlush(streamIds, /*streamConfigCounter*/streamConfigCounter);
    cb->waitForBuffersReturned();
}

void CameraHidlTest::verifyLogicalCameraResult(const camera_metadata_t* staticMetadata,
        const ::android::hardware::camera::common::V1_0::helper::CameraMetadata& resultMetadata) {
    std::unordered_set<std::string> physicalIds;
    Status rc = getPhysicalCameraIds(staticMetadata, &physicalIds);
    ASSERT_TRUE(Status::OK == rc);
    ASSERT_TRUE(physicalIds.size() > 1);

    camera_metadata_ro_entry entry;
    // Check mainPhysicalId
    entry = resultMetadata.find(ANDROID_LOGICAL_MULTI_CAMERA_ACTIVE_PHYSICAL_ID);
    if (entry.count > 0) {
        std::string mainPhysicalId(reinterpret_cast<const char *>(entry.data.u8));
        ASSERT_NE(physicalIds.find(mainPhysicalId), physicalIds.end());
    } else {
        ADD_FAILURE() << "Get LOGICAL_MULTI_CAMERA_ACTIVE_PHYSICAL_ID failed!";
    }
}

// Open a device session with empty callbacks and return static metadata.
void CameraHidlTest::openEmptyDeviceSession(const std::string &name, sp<ICameraProvider> provider,
        sp<ICameraDeviceSession> *session /*out*/, camera_metadata_t **staticMeta /*out*/,
        ::android::sp<ICameraDevice> *cameraDevice /*out*/) {
    ASSERT_NE(nullptr, session);
    ASSERT_NE(nullptr, staticMeta);

    ::android::sp<ICameraDevice> device3_x;
    ALOGI("configureStreams: Testing camera device %s", name.c_str());
    Return<void> ret;
    ret = provider->getCameraDeviceInterface_V3_x(
        name,
        [&](auto status, const auto& device) {
            ALOGI("getCameraDeviceInterface_V3_x returns status:%d",
                  (int)status);
            ASSERT_EQ(Status::OK, status);
            ASSERT_NE(device, nullptr);
            device3_x = device;
        });
    ASSERT_TRUE(ret.isOk());
    if (cameraDevice != nullptr) {
        *cameraDevice = device3_x;
    }

    sp<EmptyDeviceCb> cb = new EmptyDeviceCb();
    ret = device3_x->open(cb, [&](auto status, const auto& newSession) {
            ALOGI("device::open returns status:%d", (int)status);
            ASSERT_EQ(Status::OK, status);
            ASSERT_NE(newSession, nullptr);
            *session = newSession;
        });
    ASSERT_TRUE(ret.isOk());

    ret = device3_x->getCameraCharacteristics([&] (Status s,
            CameraMetadata metadata) {
        ASSERT_EQ(Status::OK, s);
        *staticMeta = clone_camera_metadata(
                reinterpret_cast<const camera_metadata_t*>(metadata.data()));
        ASSERT_NE(nullptr, *staticMeta);
    });
    ASSERT_TRUE(ret.isOk());
}

void CameraHidlTest::notifyDeviceState(provider::V2_5::DeviceState newState) {
    if (mProvider2_5.get() == nullptr) return;

    mProvider2_5->notifyDeviceStateChange(
            static_cast<hidl_bitfield<provider::V2_5::DeviceState>>(newState));
}

// Open a particular camera device.
void CameraHidlTest::openCameraDevice(const std::string &name,
        sp<ICameraProvider> provider,
        sp<::android::hardware::camera::device::V1_0::ICameraDevice> *device1 /*out*/) {
    ASSERT_TRUE(nullptr != device1);

    Return<void> ret;
    ret = provider->getCameraDeviceInterface_V1_x(
            name,
            [&](auto status, const auto& device) {
            ALOGI("getCameraDeviceInterface_V1_x returns status:%d",
                  (int)status);
            ASSERT_EQ(Status::OK, status);
            ASSERT_NE(device, nullptr);
            *device1 = device;
        });
    ASSERT_TRUE(ret.isOk());

    sp<Camera1DeviceCb> deviceCb = new Camera1DeviceCb(this);
    Return<Status> returnStatus = (*device1)->open(deviceCb);
    ASSERT_TRUE(returnStatus.isOk());
    ASSERT_EQ(Status::OK, returnStatus);
}

// Initialize and configure a preview window.
void CameraHidlTest::setupPreviewWindow(
        const sp<::android::hardware::camera::device::V1_0::ICameraDevice> &device,
        sp<BufferItemConsumer> *bufferItemConsumer /*out*/,
        sp<BufferItemHander> *bufferHandler /*out*/) {
    ASSERT_NE(nullptr, device.get());
    ASSERT_NE(nullptr, bufferItemConsumer);
    ASSERT_NE(nullptr, bufferHandler);

    sp<IGraphicBufferProducer> producer;
    sp<IGraphicBufferConsumer> consumer;
    BufferQueue::createBufferQueue(&producer, &consumer);
    *bufferItemConsumer = new BufferItemConsumer(consumer,
            GraphicBuffer::USAGE_HW_TEXTURE); //Use GLConsumer default usage flags
    ASSERT_NE(nullptr, (*bufferItemConsumer).get());
    *bufferHandler = new BufferItemHander(*bufferItemConsumer);
    ASSERT_NE(nullptr, (*bufferHandler).get());
    (*bufferItemConsumer)->setFrameAvailableListener(*bufferHandler);
    sp<Surface> surface = new Surface(producer);
    sp<PreviewWindowCb> previewCb = new PreviewWindowCb(surface);

    auto rc = device->setPreviewWindow(previewCb);
    ASSERT_TRUE(rc.isOk());
    ASSERT_EQ(Status::OK, rc);
}

// Stop camera preview and close camera.
void CameraHidlTest::stopPreviewAndClose(
        const sp<::android::hardware::camera::device::V1_0::ICameraDevice> &device) {
    Return<void> ret = device->stopPreview();
    ASSERT_TRUE(ret.isOk());

    ret = device->close();
    ASSERT_TRUE(ret.isOk());
}

// Enable a specific camera message type.
void CameraHidlTest::enableMsgType(unsigned int msgType,
        const sp<::android::hardware::camera::device::V1_0::ICameraDevice> &device) {
    Return<void> ret = device->enableMsgType(msgType);
    ASSERT_TRUE(ret.isOk());

    Return<bool> returnBoolStatus = device->msgTypeEnabled(msgType);
    ASSERT_TRUE(returnBoolStatus.isOk());
    ASSERT_TRUE(returnBoolStatus);
}

// Disable a specific camera message type.
void CameraHidlTest::disableMsgType(unsigned int msgType,
        const sp<::android::hardware::camera::device::V1_0::ICameraDevice> &device) {
    Return<void> ret = device->disableMsgType(msgType);
    ASSERT_TRUE(ret.isOk());

    Return<bool> returnBoolStatus = device->msgTypeEnabled(msgType);
    ASSERT_TRUE(returnBoolStatus.isOk());
    ASSERT_FALSE(returnBoolStatus);
}

// Wait until a specific frame notification arrives.
void CameraHidlTest::waitForFrameLocked(DataCallbackMsg msgFrame,
        std::unique_lock<std::mutex> &l) {
    while (msgFrame != mDataMessageTypeReceived) {
        auto timeout = std::chrono::system_clock::now() +
                std::chrono::seconds(kStreamBufferTimeoutSec);
        ASSERT_NE(std::cv_status::timeout,
                mResultCondition.wait_until(l, timeout));
    }
}

// Start preview on a particular camera device
void CameraHidlTest::startPreview(
        const sp<::android::hardware::camera::device::V1_0::ICameraDevice> &device) {
    Return<Status> returnStatus = device->startPreview();
    ASSERT_TRUE(returnStatus.isOk());
    ASSERT_EQ(Status::OK, returnStatus);
}

// Retrieve camera parameters.
void CameraHidlTest::getParameters(
        const sp<::android::hardware::camera::device::V1_0::ICameraDevice> &device,
        CameraParameters *cameraParams /*out*/) {
    ASSERT_NE(nullptr, cameraParams);

    Return<void> ret;
    ret = device->getParameters([&] (const ::android::hardware::hidl_string& params) {
        ASSERT_FALSE(params.empty());
        ::android::String8 paramString(params.c_str());
        (*cameraParams).unflatten(paramString);
    });
    ASSERT_TRUE(ret.isOk());
}

// Set camera parameters.
void CameraHidlTest::setParameters(
        const sp<::android::hardware::camera::device::V1_0::ICameraDevice> &device,
        const CameraParameters &cameraParams) {
    Return<Status> returnStatus = device->setParameters(
            cameraParams.flatten().string());
    ASSERT_TRUE(returnStatus.isOk());
    ASSERT_EQ(Status::OK, returnStatus);
}

void CameraHidlTest::allocateGraphicBuffer(uint32_t width, uint32_t height, uint64_t usage,
        PixelFormat format, hidl_handle *buffer_handle /*out*/) {
    ASSERT_NE(buffer_handle, nullptr);

    buffer_handle_t buffer;
    uint32_t stride;

    android::status_t err = android::GraphicBufferAllocator::get().allocateRawHandle(
            width, height, static_cast<int32_t>(format), 1u /*layerCount*/, usage, &buffer, &stride,
            "VtsHalCameraProviderV2_4");
    ASSERT_EQ(err, android::NO_ERROR);

    buffer_handle->setTo(const_cast<native_handle_t*>(buffer), true /*shouldOwn*/);
}

void CameraHidlTest::verifyRecommendedConfigs(const CameraMetadata& chars) {
    size_t CONFIG_ENTRY_SIZE = 5;
    size_t CONFIG_ENTRY_TYPE_OFFSET = 3;
    size_t CONFIG_ENTRY_BITFIELD_OFFSET = 4;
    uint32_t maxPublicUsecase =
            ANDROID_SCALER_AVAILABLE_RECOMMENDED_STREAM_CONFIGURATIONS_PUBLIC_END;
    uint32_t vendorUsecaseStart =
            ANDROID_SCALER_AVAILABLE_RECOMMENDED_STREAM_CONFIGURATIONS_VENDOR_START;
    uint32_t usecaseMask = (1 << vendorUsecaseStart) - 1;
    usecaseMask &= ~((1 << maxPublicUsecase) - 1);

    const camera_metadata_t* metadata = reinterpret_cast<const camera_metadata_t*> (chars.data());

    camera_metadata_ro_entry recommendedConfigsEntry, recommendedDepthConfigsEntry, ioMapEntry;
    recommendedConfigsEntry.count = recommendedDepthConfigsEntry.count = ioMapEntry.count = 0;
    int retCode = find_camera_metadata_ro_entry(metadata,
            ANDROID_SCALER_AVAILABLE_RECOMMENDED_STREAM_CONFIGURATIONS, &recommendedConfigsEntry);
    int depthRetCode = find_camera_metadata_ro_entry(metadata,
            ANDROID_DEPTH_AVAILABLE_RECOMMENDED_DEPTH_STREAM_CONFIGURATIONS,
            &recommendedDepthConfigsEntry);
    int ioRetCode = find_camera_metadata_ro_entry(metadata,
            ANDROID_SCALER_AVAILABLE_RECOMMENDED_INPUT_OUTPUT_FORMATS_MAP, &ioMapEntry);
    if ((0 != retCode) && (0 != depthRetCode)) {
        //In case both regular and depth recommended configurations are absent,
        //I/O should be absent as well.
        ASSERT_NE(ioRetCode, 0);
        return;
    }

    camera_metadata_ro_entry availableKeysEntry;
    retCode = find_camera_metadata_ro_entry(metadata,
            ANDROID_REQUEST_AVAILABLE_CHARACTERISTICS_KEYS, &availableKeysEntry);
    ASSERT_TRUE((0 == retCode) && (availableKeysEntry.count > 0));
    std::vector<int32_t> availableKeys;
    availableKeys.reserve(availableKeysEntry.count);
    availableKeys.insert(availableKeys.end(), availableKeysEntry.data.i32,
            availableKeysEntry.data.i32 + availableKeysEntry.count);

    if (recommendedConfigsEntry.count > 0) {
        ASSERT_NE(std::find(availableKeys.begin(), availableKeys.end(),
                    ANDROID_SCALER_AVAILABLE_RECOMMENDED_STREAM_CONFIGURATIONS),
                availableKeys.end());
        ASSERT_EQ((recommendedConfigsEntry.count % CONFIG_ENTRY_SIZE), 0);
        for (size_t i = 0; i < recommendedConfigsEntry.count; i += CONFIG_ENTRY_SIZE) {
            int32_t entryType =
                recommendedConfigsEntry.data.i32[i + CONFIG_ENTRY_TYPE_OFFSET];
            uint32_t bitfield =
                recommendedConfigsEntry.data.i32[i + CONFIG_ENTRY_BITFIELD_OFFSET];
            ASSERT_TRUE((entryType ==
                     ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT) ||
                    (entryType ==
                     ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_INPUT));
            ASSERT_TRUE((bitfield & usecaseMask) == 0);
        }
    }

    if (recommendedDepthConfigsEntry.count > 0) {
        ASSERT_NE(std::find(availableKeys.begin(), availableKeys.end(),
                    ANDROID_DEPTH_AVAILABLE_RECOMMENDED_DEPTH_STREAM_CONFIGURATIONS),
                availableKeys.end());
        ASSERT_EQ((recommendedDepthConfigsEntry.count % CONFIG_ENTRY_SIZE), 0);
        for (size_t i = 0; i < recommendedDepthConfigsEntry.count; i += CONFIG_ENTRY_SIZE) {
            int32_t entryType =
                recommendedDepthConfigsEntry.data.i32[i + CONFIG_ENTRY_TYPE_OFFSET];
            uint32_t bitfield =
                recommendedDepthConfigsEntry.data.i32[i + CONFIG_ENTRY_BITFIELD_OFFSET];
            ASSERT_TRUE((entryType ==
                     ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT) ||
                    (entryType ==
                     ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_INPUT));
            ASSERT_TRUE((bitfield & usecaseMask) == 0);
        }

        if (recommendedConfigsEntry.count == 0) {
            //In case regular recommended configurations are absent but suggested depth
            //configurations are present, I/O should be absent.
            ASSERT_NE(ioRetCode, 0);
        }
    }

    if ((ioRetCode == 0) && (ioMapEntry.count > 0)) {
        ASSERT_NE(std::find(availableKeys.begin(), availableKeys.end(),
                    ANDROID_SCALER_AVAILABLE_RECOMMENDED_INPUT_OUTPUT_FORMATS_MAP),
                availableKeys.end());
        ASSERT_EQ(isZSLModeAvailable(metadata), Status::OK);
    }
}

void CameraHidlTest::verifySessionReconfigurationQuery(
        sp<device::V3_5::ICameraDeviceSession> session3_5, camera_metadata* oldSessionParams,
        camera_metadata* newSessionParams) {
    ASSERT_NE(nullptr, session3_5.get());
    ASSERT_NE(nullptr, oldSessionParams);
    ASSERT_NE(nullptr, newSessionParams);

    android::hardware::hidl_vec<uint8_t> oldParams, newParams;
    oldParams.setToExternal(reinterpret_cast<uint8_t*>(oldSessionParams),
            get_camera_metadata_size(oldSessionParams));
    newParams.setToExternal(reinterpret_cast<uint8_t*>(newSessionParams),
            get_camera_metadata_size(newSessionParams));
    android::hardware::camera::common::V1_0::Status callStatus;
    auto hidlCb = [&callStatus] (android::hardware::camera::common::V1_0::Status s,
            bool /*requiredFlag*/) {
        callStatus = s;
    };
    auto ret = session3_5->isReconfigurationRequired(oldParams, newParams, hidlCb);
    ASSERT_TRUE(ret.isOk());
    switch (callStatus) {
        case android::hardware::camera::common::V1_0::Status::OK:
        case android::hardware::camera::common::V1_0::Status::METHOD_NOT_SUPPORTED:
            break;
        case android::hardware::camera::common::V1_0::Status::INTERNAL_ERROR:
        default:
            ADD_FAILURE() << "Query calllback failed";
    }
}

void CameraHidlTest::verifyRequestTemplate(const camera_metadata_t* metadata,
        RequestTemplate requestTemplate) {
    ASSERT_NE(nullptr, metadata);
    size_t entryCount =
            get_camera_metadata_entry_count(metadata);
    ALOGI("template %u metadata entry count is %zu", (int32_t)requestTemplate, entryCount);
    // TODO: we can do better than 0 here. Need to check how many required
    // request keys we've defined for each template
    ASSERT_GT(entryCount, 0u);

    // Check zoomRatio
    camera_metadata_ro_entry zoomRatioEntry;
    int foundZoomRatio = find_camera_metadata_ro_entry(metadata,
            ANDROID_CONTROL_ZOOM_RATIO, &zoomRatioEntry);
    if (foundZoomRatio == 0) {
        ASSERT_EQ(zoomRatioEntry.count, 1);
        ASSERT_EQ(zoomRatioEntry.data.f[0], 1.0f);
    }
}

INSTANTIATE_TEST_SUITE_P(
        PerInstance, CameraHidlTest,
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(ICameraProvider::descriptor)),
        android::hardware::PrintInstanceNameToString);
