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

#define LOG_TAG "camera_hidl_hal_test"
#include <android/hardware/camera/provider/2.4/ICameraProvider.h>
#include <android/hardware/camera/device/3.2/ICameraDevice.h>
#include <android/hardware/camera/device/1.0/ICameraDevice.h>
#include <android/log.h>
#include <ui/GraphicBuffer.h>
#include <VtsHalHidlTargetTestBase.h>
#include <regex>
#include "system/camera_metadata.h"
#include <hardware/gralloc.h>
#include <hardware/gralloc1.h>
#include <unordered_map>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <inttypes.h>

using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::hidl_handle;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::sp;
using ::android::GraphicBuffer;
using ::android::hardware::graphics::common::V1_0::PixelFormat;
using ::android::hardware::camera::common::V1_0::Status;
using ::android::hardware::camera::common::V1_0::CameraDeviceStatus;
using ::android::hardware::camera::common::V1_0::TorchMode;
using ::android::hardware::camera::common::V1_0::TorchModeStatus;
using ::android::hardware::camera::provider::V2_4::ICameraProvider;
using ::android::hardware::camera::provider::V2_4::ICameraProviderCallback;
using ::android::hardware::camera::device::V3_2::ICameraDevice;
using ::android::hardware::camera::device::V3_2::CaptureRequest;
using ::android::hardware::camera::device::V3_2::CaptureResult;
using ::android::hardware::camera::device::V3_2::ICameraDeviceCallback;
using ::android::hardware::camera::device::V3_2::ICameraDeviceSession;
using ::android::hardware::camera::device::V3_2::NotifyMsg;
using ::android::hardware::camera::device::V3_2::RequestTemplate;
using ::android::hardware::camera::device::V3_2::Stream;
using ::android::hardware::camera::device::V3_2::StreamType;
using ::android::hardware::camera::device::V3_2::StreamRotation;
using ::android::hardware::camera::device::V3_2::StreamConfiguration;
using ::android::hardware::camera::device::V3_2::StreamConfigurationMode;
using ::android::hardware::camera::device::V3_2::CameraMetadata;
using ::android::hardware::camera::device::V3_2::HalStreamConfiguration;
using ::android::hardware::camera::device::V3_2::BufferStatus;
using ::android::hardware::camera::device::V3_2::StreamBuffer;
using ::android::hardware::camera::device::V3_2::MsgType;
using ::android::hardware::camera::device::V3_2::ErrorMsg;
using ::android::hardware::camera::device::V3_2::ErrorCode;
using ::android::hardware::camera::device::V1_0::CameraFacing;
using ::android::hardware::camera::device::V1_0::NotifyCallbackMsg;
using ::android::hardware::camera::device::V1_0::DataCallbackMsg;
using ::android::hardware::camera::device::V1_0::CameraFrameMetadata;
using ::android::hardware::camera::device::V1_0::ICameraDevicePreviewCallback;

const char kCameraPassthroughServiceName[] = "legacy/0";
const uint32_t kMaxPreviewWidth = 1920;
const uint32_t kMaxPreviewHeight = 1080;
const uint32_t kMaxVideoWidth = 4096;
const uint32_t kMaxVideoHeight = 2160;
const int64_t kStreamBufferTimeoutSec = 3;
const int64_t kTorchTimeoutSec = 1;
const int64_t kEmptyFlushTimeoutMSec = 200;
const char kDumpOutput[] = "/dev/null";

struct AvailableStream {
    int32_t width;
    int32_t height;
    int32_t format;
};

struct AvailableZSLInputOutput {
    int32_t inputFormat;
    int32_t outputFormat;
};

namespace {
    // "device@<version>/legacy/<id>"
    const char *kDeviceNameRE = "device@([0-9]+\\.[0-9]+)/legacy/(.+)";
    const int CAMERA_DEVICE_API_VERSION_3_2 = 0x302;
    const int CAMERA_DEVICE_API_VERSION_1_0 = 0x100;
    const char *kHAL3_2 = "3.2";
    const char *kHAL1_0 = "1.0";

    bool matchDeviceName(const hidl_string& deviceName, std::smatch& sm) {
        std::regex e(kDeviceNameRE);
        std::string deviceNameStd(deviceName.c_str());
        return std::regex_match(deviceNameStd, sm, e);
    }

    int getCameraDeviceVersion(const hidl_string& deviceName) {
        std::smatch sm;
        bool match = matchDeviceName(deviceName, sm);
        if (!match) {
            return -1;
        }
        std::string version = sm[1].str();
        if (version.compare(kHAL3_2) == 0) {
            // maybe switched to 3.4 or define the hidl version enumlater
            return CAMERA_DEVICE_API_VERSION_3_2;
        } else if (version.compare(kHAL1_0) == 0) {
            return CAMERA_DEVICE_API_VERSION_1_0;
        }
        return 0;
    }
}

// Test environment for camera
class CameraHidlEnvironment : public ::testing::Environment {
public:
    // get the test environment singleton
    static CameraHidlEnvironment* Instance() {
        static CameraHidlEnvironment* instance = new CameraHidlEnvironment;
        return instance;
    }

    virtual void SetUp() override;
    virtual void TearDown() override;

    sp<ICameraProvider> mProvider;

private:
    CameraHidlEnvironment() {}

    GTEST_DISALLOW_COPY_AND_ASSIGN_(CameraHidlEnvironment);
};

void CameraHidlEnvironment::SetUp() {
    // TODO: test the binderized mode
    mProvider = ::testing::VtsHalHidlTargetTestBase::getService<ICameraProvider>(kCameraPassthroughServiceName);
    // TODO: handle the device doesn't have any camera case
    ALOGI_IF(mProvider, "provider is not nullptr, %p", mProvider.get());
    ASSERT_NE(mProvider, nullptr);
}

void CameraHidlEnvironment::TearDown() {
    ALOGI("TearDown CameraHidlEnvironment");
}

// The main test class for camera HIDL HAL.
class CameraHidlTest : public ::testing::VtsHalHidlTargetTestBase {
public:
    virtual void SetUp() override {}
    virtual void TearDown() override {}

    hidl_vec<hidl_string> getCameraDeviceNames();

    struct EmptyDeviceCb : public ICameraDeviceCallback {
        virtual Return<void> processCaptureResult(const CaptureResult& /*result*/) override {
            ALOGI("processCaptureResult callback");
            ADD_FAILURE(); // Empty callback should not reach here
            return Void();
        }

        virtual Return<void> notify(const NotifyMsg& /*msg*/) override {
            ALOGI("notify callback");
            ADD_FAILURE(); // Empty callback should not reach here
            return Void();
        }
    };

    struct DeviceCb : public ICameraDeviceCallback {
        DeviceCb(CameraHidlTest *parent) : mParent(parent) {}
        Return<void> processCaptureResult(const CaptureResult& result) override;
        Return<void> notify(const NotifyMsg& msg) override;

     private:
        CameraHidlTest *mParent;               // Parent object
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

     private:
        CameraHidlTest *mParent;               // Parent object
    };

    void openEmptyDeviceSession(const std::string &name,
            const CameraHidlEnvironment* env,
            sp<ICameraDeviceSession> *session /*out*/,
            camera_metadata_t **staticMeta /*out*/);
    void configurePreviewStream(const std::string &name,
            const CameraHidlEnvironment* env,
            const AvailableStream *previewThreshold,
            sp<ICameraDeviceSession> *session /*out*/,
            Stream *previewStream /*out*/,
            HalStreamConfiguration *halStreamConfig /*out*/);
    static Status getAvailableOutputStreams(camera_metadata_t *staticMeta,
            std::vector<AvailableStream> &outputStreams,
            const AvailableStream *threshold = nullptr);
    static Status isConstrainedModeAvailable(camera_metadata_t *staticMeta);
    static Status pickConstrainedModeSize(camera_metadata_t *staticMeta,
            AvailableStream &hfrStream);
    static Status isZSLModeAvailable(camera_metadata_t *staticMeta);
    static Status getZSLInputOutputMap(camera_metadata_t *staticMeta,
            std::vector<AvailableZSLInputOutput> &inputOutputMap);
    static Status findLargestSize(
            const std::vector<AvailableStream> &streamSizes,
            int32_t format, AvailableStream &result);

protected:
    std::mutex mLock;                          // Synchronize access to member variables
    std::condition_variable mResultCondition;  // Condition variable for incoming results
    uint32_t mResultFrameNumber;               // Expected result frame number
    std::vector<StreamBuffer> mResultBuffers;  // Holds stream buffers from capture result
    std::vector<ErrorMsg> mErrors;             // Holds incoming error notifications

    std::mutex mTorchLock;                     // Synchronize access to torch status
    std::condition_variable mTorchCond;        // Condition variable for torch status
    TorchModeStatus mTorchStatus;              // Current torch status
};

Return<void> CameraHidlTest::Camera1DeviceCb::notifyCallback(
        NotifyCallbackMsg msgType __unused, int32_t ext1 __unused,
        int32_t ext2 __unused) {
    return Void();
}

Return<uint32_t> CameraHidlTest::Camera1DeviceCb::registerMemory(
        const hidl_handle& descriptor __unused, uint32_t bufferSize __unused,
        uint32_t bufferCount __unused) {
    return 0;
}

Return<void> CameraHidlTest::Camera1DeviceCb::unregisterMemory(
        uint32_t memId __unused) {
    return Void();
}

Return<void> CameraHidlTest::Camera1DeviceCb::dataCallback(
        DataCallbackMsg msgType __unused, uint32_t data __unused,
        uint32_t bufferIndex __unused,
        const CameraFrameMetadata& metadata __unused) {
    return Void();
}

Return<void> CameraHidlTest::Camera1DeviceCb::dataCallbackTimestamp(
        DataCallbackMsg msgType __unused, uint32_t data __unused,
        uint32_t bufferIndex __unused, int64_t timestamp __unused) {
    return Void();
}

Return<void> CameraHidlTest::Camera1DeviceCb::handleCallbackTimestamp(
        DataCallbackMsg msgType __unused, const hidl_handle& frameData __unused,
        uint32_t data __unused, uint32_t bufferIndex __unused,
        int64_t timestamp __unused) {
    return Void();
}

Return<void> CameraHidlTest::DeviceCb::processCaptureResult(
        const CaptureResult& result) {
    if (nullptr == mParent) {
        return Void();
    }

    std::unique_lock<std::mutex> l(mParent->mLock);

    if(mParent->mResultFrameNumber != result.frameNumber) {
        ALOGE("%s: Unexpected frame number! Expected: %u received: %u",
              __func__, mParent->mResultFrameNumber, result.frameNumber);
        ADD_FAILURE();
    }

    size_t resultLength = result.outputBuffers.size();
    for (size_t i = 0; i < resultLength; i++) {
        mParent->mResultBuffers.push_back(result.outputBuffers[i]);
    }

    // TODO(epeev): Handle partial results in case client supports them and
    //              verify the result against request settings.

    l.unlock();
    mParent->mResultCondition.notify_one();

    return Void();
}

Return<void> CameraHidlTest::DeviceCb::notify(
        const NotifyMsg& message) {

    if (MsgType::ERROR == message.type) {
        {
            std::lock_guard<std::mutex> l(mParent->mLock);
            mParent->mErrors.push_back(message.msg.error);
        }

        if ((ErrorCode::ERROR_REQUEST == message.msg.error.errorCode)
                || (ErrorCode::ERROR_BUFFER == message.msg.error.errorCode)) {
            mParent->mResultCondition.notify_one();
        }
    }

    return Void();
}

hidl_vec<hidl_string> CameraHidlTest::getCameraDeviceNames() {
    CameraHidlEnvironment* env = CameraHidlEnvironment::Instance();
    hidl_vec<hidl_string> cameraDeviceNames;
    env->mProvider->getCameraIdList(
        [&](auto status, const auto& idList) {
            ALOGI("getCameraIdList returns status:%d", (int)status);
            for (size_t i = 0; i < idList.size(); i++) {
                ALOGI("Camera Id[%zu] is %s", i, idList[i].c_str());
            }
            ASSERT_EQ(Status::OK, status);
            cameraDeviceNames = idList;
        });
    return cameraDeviceNames;
}

// Test if ICameraProvider::isTorchModeSupported returns Status::OK
TEST_F(CameraHidlTest, isTorchModeSupported) {
    CameraHidlEnvironment::Instance()->mProvider->isSetTorchModeSupported(
        [&](auto status, bool support) {
            ALOGI("isSetTorchModeSupported returns status:%d supported:%d",
                    (int)status, support);
            ASSERT_EQ(Status::OK, status);
        });
}

// TODO: consider removing this test if getCameraDeviceNames() has the same coverage
TEST_F(CameraHidlTest, getCameraIdList) {
    CameraHidlEnvironment::Instance()->mProvider->getCameraIdList(
        [&](auto status, const auto& idList) {
            ALOGI("getCameraIdList returns status:%d", (int)status);
            for (size_t i = 0; i < idList.size(); i++) {
                ALOGI("Camera Id[%zu] is %s", i, idList[i].c_str());
            }
            ASSERT_EQ(Status::OK, status);
            // This is true for internal camera provider.
            // Not necessary hold for external cameras providers
            ASSERT_GT(idList.size(), 0u);
        });
}

// Test if ICameraProvider::getVendorTags returns Status::OK
TEST_F(CameraHidlTest, getVendorTags) {
    CameraHidlEnvironment::Instance()->mProvider->getVendorTags(
        [&](auto status, const auto& vendorTagSecs) {
            ALOGI("getVendorTags returns status:%d numSections %zu",
                    (int)status, vendorTagSecs.size());
            for (size_t i = 0; i < vendorTagSecs.size(); i++) {
                ALOGI("Vendor tag section %zu name %s",
                        i, vendorTagSecs[i].sectionName.c_str());
                for (size_t j = 0; j < vendorTagSecs[i].tags.size(); j++) {
                    const auto& tag = vendorTagSecs[i].tags[j];
                    ALOGI("Vendor tag id %u name %s type %d",
                            tag.tagId,
                            tag.tagName.c_str(),
                            (int) tag.tagType);
                }
            }
            ASSERT_EQ(Status::OK, status);
        });
}

// Test if ICameraProvider::setCallback returns Status::OK
TEST_F(CameraHidlTest, setCallback) {
    CameraHidlEnvironment* env = CameraHidlEnvironment::Instance();
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
    sp<ProviderCb> cb = new ProviderCb;
    auto status = env->mProvider->setCallback(cb);
    ASSERT_EQ(Status::OK, status);
}

// Test if ICameraProvider::getCameraDeviceInterface returns Status::OK and non-null device
TEST_F(CameraHidlTest, getCameraDeviceInterface) {
    CameraHidlEnvironment* env = CameraHidlEnvironment::Instance();
    hidl_vec<hidl_string> cameraDeviceNames = getCameraDeviceNames();

    for (const auto& name : cameraDeviceNames) {
        if (getCameraDeviceVersion(name) == CAMERA_DEVICE_API_VERSION_3_2) {
            env->mProvider->getCameraDeviceInterface_V3_x(
                name,
                [&](auto status, const auto& device3_2) {
                    ALOGI("getCameraDeviceInterface_V3_x returns status:%d", (int)status);
                    ASSERT_EQ(Status::OK, status);
                    ASSERT_NE(device3_2, nullptr);
                });
        } else if (getCameraDeviceVersion(name) == CAMERA_DEVICE_API_VERSION_1_0) {
            env->mProvider->getCameraDeviceInterface_V1_x(
                name,
                [&](auto status, const auto& device1) {
                    ALOGI("getCameraDeviceInterface_V1_x returns status:%d", (int)status);
                    ASSERT_EQ(Status::OK, status);
                    ASSERT_NE(device1, nullptr);
                });
        }
    }
}

// Verify that the device resource cost can be retrieved and the values are
// sane.
TEST_F(CameraHidlTest, getResourceCost) {
    CameraHidlEnvironment* env = CameraHidlEnvironment::Instance();
    hidl_vec<hidl_string> cameraDeviceNames = getCameraDeviceNames();

    for (const auto& name : cameraDeviceNames) {
        if (getCameraDeviceVersion(name) == CAMERA_DEVICE_API_VERSION_3_2) {
            ::android::sp<::android::hardware::camera::device::V3_2::ICameraDevice> device3_2;
            ALOGI("getResourceCost: Testing camera device %s", name.c_str());
            env->mProvider->getCameraDeviceInterface_V3_x(
                name,
                [&](auto status, const auto& device) {
                    ALOGI("getCameraDeviceInterface_V3_x returns status:%d", (int)status);
                    ASSERT_EQ(Status::OK, status);
                    ASSERT_NE(device, nullptr);
                    device3_2 = device;
                });

            device3_2->getResourceCost(
                [&](auto status, const auto& resourceCost) {
                    ALOGI("getResourceCost returns status:%d", (int)status);
                    ASSERT_EQ(Status::OK, status);
                    ALOGI("    Resource cost is %d", resourceCost.resourceCost);
                    ASSERT_LE(resourceCost.resourceCost, 100u);
                    for (const auto& name : resourceCost.conflictingDevices) {
                        ALOGI("    Conflicting device: %s", name.c_str());
                    }
                });
        } else {
            ::android::sp<::android::hardware::camera::device::V1_0::ICameraDevice> device1;
            ALOGI("getResourceCost: Testing camera device %s", name.c_str());
            env->mProvider->getCameraDeviceInterface_V1_x(
                name,
                [&](auto status, const auto& device) {
                    ALOGI("getCameraDeviceInterface_V1_x returns status:%d", (int)status);
                    ASSERT_EQ(Status::OK, status);
                    ASSERT_NE(device, nullptr);
                    device1 = device;
                });

            device1->getResourceCost(
                [&](auto status, const auto& resourceCost) {
                    ALOGI("getResourceCost returns status:%d", (int)status);
                    ASSERT_EQ(Status::OK, status);
                    ALOGI("    Resource cost is %d", resourceCost.resourceCost);
                    ASSERT_LE(resourceCost.resourceCost, 100u);
                    for (const auto& name : resourceCost.conflictingDevices) {
                        ALOGI("    Conflicting device: %s", name.c_str());
                    }
                });
        }
    }
}

// Verify that the static camera info can be retrieved
// successfully.
TEST_F(CameraHidlTest, getCameraInfo) {
    CameraHidlEnvironment* env = CameraHidlEnvironment::Instance();
    hidl_vec<hidl_string> cameraDeviceNames = getCameraDeviceNames();

    for (const auto& name : cameraDeviceNames) {
        if (getCameraDeviceVersion(name) == CAMERA_DEVICE_API_VERSION_1_0) {
            ::android::sp<::android::hardware::camera::device::V1_0::ICameraDevice> device1;
            ALOGI("getCameraCharacteristics: Testing camera device %s", name.c_str());
            env->mProvider->getCameraDeviceInterface_V1_x(
                name,
                [&](auto status, const auto& device) {
                    ALOGI("getCameraDeviceInterface_V1_x returns status:%d", (int)status);
                    ASSERT_EQ(Status::OK, status);
                    ASSERT_NE(device, nullptr);
                    device1 = device;
                });

            device1->getCameraInfo(
                [&](auto status, const auto& info) {
                    ALOGI("getCameraInfo returns status:%d", (int)status);
                    ASSERT_EQ(Status::OK, status);
                    switch(info.orientation) {
                        case 0:
                        case 90:
                        case 180:
                        case 270:
                            //Expected cases
                            ALOGI("camera orientation: %d", info.orientation);
                            break;
                        default:
                            FAIL() << "Unexpected camera orientation:" << info.orientation;
                    }
                    switch(info.facing) {
                        case CameraFacing::BACK:
                        case CameraFacing::FRONT:
                        case CameraFacing::EXTERNAL:
                            //Expected cases
                            ALOGI("camera facing: %d", info.facing);
                            break;
                        default:
                            FAIL() << "Unexpected camera facing:" << static_cast<uint32_t> (info.facing);
                    }
                });
        }
    }
}

// Verify that the static camera characteristics can be retrieved
// successfully.
TEST_F(CameraHidlTest, getCameraCharacteristics) {
    CameraHidlEnvironment* env = CameraHidlEnvironment::Instance();
    hidl_vec<hidl_string> cameraDeviceNames = getCameraDeviceNames();

    for (const auto& name : cameraDeviceNames) {
        if (getCameraDeviceVersion(name) == CAMERA_DEVICE_API_VERSION_3_2) {
            ::android::sp<::android::hardware::camera::device::V3_2::ICameraDevice> device3_2;
            ALOGI("getCameraCharacteristics: Testing camera device %s", name.c_str());
            env->mProvider->getCameraDeviceInterface_V3_x(
                name,
                [&](auto status, const auto& device) {
                    ALOGI("getCameraDeviceInterface_V3_x returns status:%d", (int)status);
                    ASSERT_EQ(Status::OK, status);
                    ASSERT_NE(device, nullptr);
                    device3_2 = device;
                });

            device3_2->getCameraCharacteristics(
                [&](auto status, const auto& chars) {
                    ALOGI("getCameraCharacteristics returns status:%d", (int)status);
                    ASSERT_EQ(Status::OK, status);
                    const camera_metadata_t* metadata = (camera_metadata_t*) chars.data();
                    size_t expectedSize = chars.size();
                    ASSERT_EQ(0, validate_camera_metadata_structure(metadata, &expectedSize));
                    size_t entryCount = get_camera_metadata_entry_count(metadata);
                    // TODO: we can do better than 0 here. Need to check how many required
                    // characteristics keys we've defined.
                    ASSERT_GT(entryCount, 0u);
                    ALOGI("getCameraCharacteristics metadata entry count is %zu", entryCount);
                });
        }
    }
}

//In case it is supported verify that torch can be enabled.
//Check for corresponding toch callbacks as well.
TEST_F(CameraHidlTest, setTorchMode) {
    CameraHidlEnvironment* env = CameraHidlEnvironment::Instance();
    hidl_vec<hidl_string> cameraDeviceNames = getCameraDeviceNames();
    bool torchControlSupported = false;

    CameraHidlEnvironment::Instance()->mProvider->isSetTorchModeSupported(
        [&](auto status, bool support) {
            ALOGI("isSetTorchModeSupported returns status:%d supported:%d",
                    (int)status, support);
            ASSERT_EQ(Status::OK, status);
            torchControlSupported = support;
        });

    sp<TorchProviderCb> cb = new TorchProviderCb(this);
    auto status = env->mProvider->setCallback(cb);
    ASSERT_EQ(Status::OK, status);

    for (const auto& name : cameraDeviceNames) {
        if (getCameraDeviceVersion(name) == CAMERA_DEVICE_API_VERSION_3_2) {
            ::android::sp<::android::hardware::camera::device::V3_2::ICameraDevice> device3_2;
            ALOGI("setTorchMode: Testing camera device %s", name.c_str());
            env->mProvider->getCameraDeviceInterface_V3_x(
                name,
                [&](auto status, const auto& device) {
                    ALOGI("getCameraDeviceInterface_V3_x returns status:%d", (int)status);
                    ASSERT_EQ(Status::OK, status);
                    ASSERT_NE(device, nullptr);
                    device3_2 = device;
                });

            mTorchStatus = TorchModeStatus::NOT_AVAILABLE;
            Status status = device3_2->setTorchMode(TorchMode::ON);
            ALOGI("setTorchMode return status %d", (int)status);
            if (!torchControlSupported) {
                ASSERT_EQ(Status::METHOD_NOT_SUPPORTED, status);
            } else {
                ASSERT_TRUE(status == Status::OK || status == Status::OPERATION_NOT_SUPPORTED);
                if (status == Status::OK) {
                    {
                        std::unique_lock<std::mutex> l(mTorchLock);
                        while (TorchModeStatus::NOT_AVAILABLE == mTorchStatus) {
                            auto timeout = std::chrono::system_clock::now() +
                                    std::chrono::seconds(kTorchTimeoutSec);
                            ASSERT_NE(std::cv_status::timeout,
                                    mTorchCond.wait_until(l, timeout));
                        }
                        ASSERT_EQ(TorchModeStatus::AVAILABLE_ON, mTorchStatus);
                        mTorchStatus = TorchModeStatus::NOT_AVAILABLE;
                    }

                    status = device3_2->setTorchMode(TorchMode::OFF);
                    ASSERT_EQ(Status::OK, status);

                    {
                        std::unique_lock<std::mutex> l(mTorchLock);
                        while (TorchModeStatus::NOT_AVAILABLE == mTorchStatus) {
                            auto timeout = std::chrono::system_clock::now() +
                                    std::chrono::seconds(kTorchTimeoutSec);
                            ASSERT_NE(std::cv_status::timeout,
                                    mTorchCond.wait_until(l, timeout));
                        }
                        ASSERT_EQ(TorchModeStatus::AVAILABLE_OFF, mTorchStatus);
                    }
                }
            }
        } else if (getCameraDeviceVersion(name) == CAMERA_DEVICE_API_VERSION_1_0) {
            ::android::sp<::android::hardware::camera::device::V1_0::ICameraDevice> device1;
            ALOGI("setTorchMode: Testing camera device %s", name.c_str());
            env->mProvider->getCameraDeviceInterface_V1_x(
                name,
                [&](auto status, const auto& device) {
                    ALOGI("getCameraDeviceInterface_V1_x returns status:%d", (int)status);
                    ASSERT_EQ(Status::OK, status);
                    ASSERT_NE(device, nullptr);
                    device1 = device;
                });

            mTorchStatus = TorchModeStatus::NOT_AVAILABLE;
            Status status = device1->setTorchMode(TorchMode::ON);
            ALOGI("setTorchMode return status %d", (int)status);
            if (!torchControlSupported) {
                ASSERT_EQ(Status::METHOD_NOT_SUPPORTED, status);
            } else {
                ASSERT_TRUE(status == Status::OK || status == Status::OPERATION_NOT_SUPPORTED);
                if (status == Status::OK) {
                    {
                        std::unique_lock<std::mutex> l(mTorchLock);
                        while (TorchModeStatus::NOT_AVAILABLE == mTorchStatus) {
                            auto timeout = std::chrono::system_clock::now() +
                                    std::chrono::seconds(kTorchTimeoutSec);
                            ASSERT_NE(std::cv_status::timeout,
                                    mTorchCond.wait_until(l, timeout));
                        }
                        ASSERT_EQ(TorchModeStatus::AVAILABLE_ON, mTorchStatus);
                        mTorchStatus = TorchModeStatus::NOT_AVAILABLE;
                    }

                    status = device1->setTorchMode(TorchMode::OFF);
                    ASSERT_EQ(Status::OK, status);

                    {
                        std::unique_lock<std::mutex> l(mTorchLock);
                        while (TorchModeStatus::NOT_AVAILABLE == mTorchStatus) {
                            auto timeout = std::chrono::system_clock::now() +
                                    std::chrono::seconds(kTorchTimeoutSec);
                            ASSERT_NE(std::cv_status::timeout,
                                    mTorchCond.wait_until(l, timeout));
                        }
                        ASSERT_EQ(TorchModeStatus::AVAILABLE_OFF, mTorchStatus);
                    }

                }
            }
        }
    }

    status = env->mProvider->setCallback(nullptr);
    ASSERT_EQ(Status::OK, status);
}

// Check dump functionality.
TEST_F(CameraHidlTest, dumpState) {
    CameraHidlEnvironment* env = CameraHidlEnvironment::Instance();
    hidl_vec<hidl_string> cameraDeviceNames = getCameraDeviceNames();

    for (const auto& name : cameraDeviceNames) {
        if (getCameraDeviceVersion(name) == CAMERA_DEVICE_API_VERSION_3_2) {
            ::android::sp<::android::hardware::camera::device::V3_2::ICameraDevice> device3_2;
            ALOGI("dumpState: Testing camera device %s", name.c_str());
            env->mProvider->getCameraDeviceInterface_V3_x(
                name,
                [&](auto status, const auto& device) {
                    ALOGI("getCameraDeviceInterface_V3_x returns status:%d", (int)status);
                    ASSERT_EQ(Status::OK, status);
                    ASSERT_NE(device, nullptr);
                    device3_2 = device;
                });

            native_handle_t* raw_handle = native_handle_create(1, 0);
            raw_handle->data[0] = open(kDumpOutput, O_RDWR);
            ASSERT_GE(raw_handle->data[0], 0);
            hidl_handle handle = raw_handle;
            device3_2->dumpState(handle);
            close(raw_handle->data[0]);
            native_handle_delete(raw_handle);
        } else if (getCameraDeviceVersion(name) == CAMERA_DEVICE_API_VERSION_1_0) {
            ::android::sp<::android::hardware::camera::device::V1_0::ICameraDevice> device1;
            ALOGI("dumpState: Testing camera device %s", name.c_str());
            env->mProvider->getCameraDeviceInterface_V1_x(
                name,
                [&](auto status, const auto& device) {
                    ALOGI("getCameraDeviceInterface_V1_x returns status:%d", (int)status);
                    ASSERT_EQ(Status::OK, status);
                    ASSERT_NE(device, nullptr);
                    device1 = device;
                });

            native_handle_t* raw_handle = native_handle_create(1, 0);
            raw_handle->data[0] = open(kDumpOutput, O_RDWR);
            ASSERT_GE(raw_handle->data[0], 0);
            hidl_handle handle = raw_handle;
            device1->dumpState(handle);
            close(raw_handle->data[0]);
            native_handle_delete(raw_handle);
        }
    }
}

// Open, dumpStates, then close
TEST_F(CameraHidlTest, openClose) {
    CameraHidlEnvironment* env = CameraHidlEnvironment::Instance();
    hidl_vec<hidl_string> cameraDeviceNames = getCameraDeviceNames();

    for (const auto& name : cameraDeviceNames) {
        if (getCameraDeviceVersion(name) == CAMERA_DEVICE_API_VERSION_3_2) {
            ::android::sp<::android::hardware::camera::device::V3_2::ICameraDevice> device3_2;
            ALOGI("openClose: Testing camera device %s", name.c_str());
            env->mProvider->getCameraDeviceInterface_V3_x(
                name,
                [&](auto status, const auto& device) {
                    ALOGI("getCameraDeviceInterface_V3_x returns status:%d", (int)status);
                    ASSERT_EQ(Status::OK, status);
                    ASSERT_NE(device, nullptr);
                    device3_2 = device;
                });

            sp<EmptyDeviceCb> cb = new EmptyDeviceCb;
            sp<ICameraDeviceSession> session;
            device3_2->open(
                cb,
                [&](auto status, const auto& newSession) {
                    ALOGI("device::open returns status:%d", (int)status);
                    ASSERT_EQ(Status::OK, status);
                    ASSERT_NE(newSession, nullptr);
                    session = newSession;
                });

            native_handle_t* raw_handle = native_handle_create(1, 0);
            raw_handle->data[0] = open(kDumpOutput, O_RDWR);
            ASSERT_GE(raw_handle->data[0], 0);
            hidl_handle handle = raw_handle;
            device3_2->dumpState(handle);
            close(raw_handle->data[0]);
            native_handle_delete(raw_handle);

            session->close();
            // TODO: test all session API calls return INTERNAL_ERROR after close
            // TODO: keep a wp copy here and verify session cannot be promoted out of this scope
        } else if (getCameraDeviceVersion(name) == CAMERA_DEVICE_API_VERSION_1_0) {
            ::android::sp<::android::hardware::camera::device::V1_0::ICameraDevice> device1;
            ALOGI("openClose: Testing camera device %s", name.c_str());
            env->mProvider->getCameraDeviceInterface_V1_x(
                name,
                [&](auto status, const auto& device) {
                    ALOGI("getCameraDeviceInterface_V1_x returns status:%d", (int)status);
                    ASSERT_EQ(Status::OK, status);
                    ASSERT_NE(device, nullptr);
                    device1 = device;
                });
            sp<Camera1DeviceCb> cb = new Camera1DeviceCb(this);
            ASSERT_EQ(Status::OK, device1->open(cb));

            native_handle_t* raw_handle = native_handle_create(1, 0);
            raw_handle->data[0] = open(kDumpOutput, O_RDWR);
            ASSERT_GE(raw_handle->data[0], 0);
            hidl_handle handle = raw_handle;
            device1->dumpState(handle);
            close(raw_handle->data[0]);
            native_handle_delete(raw_handle);

            device1->close();
        }
    }
}

// Check whether all common default request settings can be sucessfully
// constructed.
TEST_F(CameraHidlTest, constructDefaultRequestSettings) {
    CameraHidlEnvironment* env = CameraHidlEnvironment::Instance();
    hidl_vec<hidl_string> cameraDeviceNames = getCameraDeviceNames();

    for (const auto& name : cameraDeviceNames) {
        if (getCameraDeviceVersion(name) == CAMERA_DEVICE_API_VERSION_3_2) {
            ::android::sp<::android::hardware::camera::device::V3_2::ICameraDevice> device3_2;
            ALOGI("constructDefaultRequestSettings: Testing camera device %s", name.c_str());
            env->mProvider->getCameraDeviceInterface_V3_x(
                name,
                [&](auto status, const auto& device) {
                    ALOGI("getCameraDeviceInterface_V3_x returns status:%d", (int)status);
                    ASSERT_EQ(Status::OK, status);
                    ASSERT_NE(device, nullptr);
                    device3_2 = device;
                });

            sp<EmptyDeviceCb> cb = new EmptyDeviceCb;
            sp<ICameraDeviceSession> session;
            device3_2->open(
                cb,
                [&](auto status, const auto& newSession) {
                    ALOGI("device::open returns status:%d", (int)status);
                    ASSERT_EQ(Status::OK, status);
                    ASSERT_NE(newSession, nullptr);
                    session = newSession;
                });

            for (uint32_t t = (uint32_t) RequestTemplate::PREVIEW;
                    t <= (uint32_t) RequestTemplate::MANUAL; t++) {
                RequestTemplate reqTemplate = (RequestTemplate) t;
                session->constructDefaultRequestSettings(
                    reqTemplate,
                    [&](auto status, const auto& req) {
                        ALOGI("constructDefaultRequestSettings returns status:%d", (int)status);
                        if (reqTemplate == RequestTemplate::ZERO_SHUTTER_LAG ||
                                reqTemplate == RequestTemplate::MANUAL) {
                            // optional templates
                            ASSERT_TRUE(status == Status::OK || status == Status::ILLEGAL_ARGUMENT);
                        } else {
                            ASSERT_EQ(Status::OK, status);
                        }

                        if (status == Status::OK) {
                            const camera_metadata_t* metadata =
                                (camera_metadata_t*) req.data();
                            size_t expectedSize = req.size();
                            ASSERT_EQ(0, validate_camera_metadata_structure(
                                    metadata, &expectedSize));
                            size_t entryCount = get_camera_metadata_entry_count(metadata);
                            // TODO: we can do better than 0 here. Need to check how many required
                            // request keys we've defined for each template
                            ASSERT_GT(entryCount, 0u);
                            ALOGI("template %u metadata entry count is %zu", t, entryCount);
                        } else {
                            ASSERT_EQ(0u, req.size());
                        }
                    });
            }
            session->close();
        }
    }
}

// Verify that all supported stream formats and sizes can be configured
// successfully.
TEST_F(CameraHidlTest, configureStreamsAvailableOutputs) {
    CameraHidlEnvironment* env = CameraHidlEnvironment::Instance();
    hidl_vec<hidl_string> cameraDeviceNames = getCameraDeviceNames();
    std::vector<AvailableStream> outputStreams;

    for (const auto& name : cameraDeviceNames) {
        if (getCameraDeviceVersion(name) == CAMERA_DEVICE_API_VERSION_3_2) {
            camera_metadata_t *staticMeta;
            sp<ICameraDeviceSession> session;
            openEmptyDeviceSession(name, env, &session /*out*/,
                    &staticMeta /*out*/);

            outputStreams.clear();
            ASSERT_EQ(Status::OK, getAvailableOutputStreams(staticMeta,
                    outputStreams));
            ASSERT_NE(0u, outputStreams.size());

            int32_t streamId = 0;
            for (auto &it : outputStreams) {
                Stream stream = {streamId, StreamType::OUTPUT,
                        static_cast<uint32_t> (it.width),
                        static_cast<uint32_t> (it.height),
                        static_cast<PixelFormat> (it.format), 0, 0,
                        StreamRotation::ROTATION_0};
                ::android::hardware::hidl_vec<Stream> streams = {stream};
                StreamConfiguration config = {streams,
                        StreamConfigurationMode::NORMAL_MODE};
                session->configureStreams(config, [streamId] (Status s,
                        HalStreamConfiguration halConfig) {
                    ASSERT_EQ(Status::OK, s);
                    ASSERT_EQ(1u, halConfig.streams.size());
                    ASSERT_EQ(halConfig.streams[0].id, streamId);
                });
                streamId++;
            }

            free_camera_metadata(staticMeta);
            session->close();
        }
    }
}

// Check for correct handling of invalid/incorrect configuration parameters.
TEST_F(CameraHidlTest, configureStreamsInvalidOutputs) {
    CameraHidlEnvironment* env = CameraHidlEnvironment::Instance();
    hidl_vec<hidl_string> cameraDeviceNames = getCameraDeviceNames();
    std::vector<AvailableStream> outputStreams;

    for (const auto& name : cameraDeviceNames) {
        if (getCameraDeviceVersion(name) == CAMERA_DEVICE_API_VERSION_3_2) {
            camera_metadata_t *staticMeta;
            sp<ICameraDeviceSession> session;
            openEmptyDeviceSession(name, env, &session /*out*/,
                    &staticMeta /*out*/);

            outputStreams.clear();
            ASSERT_EQ(Status::OK, getAvailableOutputStreams(staticMeta,
                    outputStreams));
            ASSERT_NE(0u, outputStreams.size());

            int32_t streamId = 0;
            Stream stream = {streamId++, StreamType::OUTPUT,
                    static_cast<uint32_t> (0),
                    static_cast<uint32_t> (0),
                    static_cast<PixelFormat> (outputStreams[0].format),
                    0, 0, StreamRotation::ROTATION_0};
            ::android::hardware::hidl_vec<Stream> streams = {stream};
            StreamConfiguration config = {streams,
                    StreamConfigurationMode::NORMAL_MODE};
            session->configureStreams(config, [] (Status s,
                    HalStreamConfiguration) {
                ASSERT_TRUE((Status::ILLEGAL_ARGUMENT == s) ||
                            (Status::INTERNAL_ERROR == s));
            });

            stream = {streamId++, StreamType::OUTPUT,
                    static_cast<uint32_t> (UINT32_MAX),
                    static_cast<uint32_t> (UINT32_MAX),
                    static_cast<PixelFormat> (outputStreams[0].format),
                    0, 0, StreamRotation::ROTATION_0};
            streams[0] = stream;
            config = {streams,
                    StreamConfigurationMode::NORMAL_MODE};
            session->configureStreams(config, [] (Status s,
                    HalStreamConfiguration) {
                ASSERT_EQ(Status::ILLEGAL_ARGUMENT, s);
            });

            for (auto &it : outputStreams) {
                stream = {streamId++, StreamType::OUTPUT,
                        static_cast<uint32_t> (it.width),
                        static_cast<uint32_t> (it.height),
                        static_cast<PixelFormat> (UINT32_MAX),
                        0, 0, StreamRotation::ROTATION_0};
                streams[0] = stream;
                config = {streams,
                        StreamConfigurationMode::NORMAL_MODE};
                session->configureStreams(config, [] (Status s,
                        HalStreamConfiguration) {
                    ASSERT_EQ(Status::ILLEGAL_ARGUMENT, s);
                });

                stream = {streamId++, StreamType::OUTPUT,
                        static_cast<uint32_t> (it.width),
                        static_cast<uint32_t> (it.height),
                        static_cast<PixelFormat> (it.format),
                        0, 0, static_cast<StreamRotation> (UINT32_MAX)};
                streams[0] = stream;
                config = {streams,
                        StreamConfigurationMode::NORMAL_MODE};
                session->configureStreams(config, [] (Status s,
                        HalStreamConfiguration) {
                    ASSERT_EQ(Status::ILLEGAL_ARGUMENT, s);
                });
            }

            free_camera_metadata(staticMeta);
            session->close();
        }
    }
}

// Check whether all supported ZSL output stream combinations can be
// configured successfully.
TEST_F(CameraHidlTest, configureStreamsZSLInputOutputs) {
    CameraHidlEnvironment* env = CameraHidlEnvironment::Instance();
    hidl_vec<hidl_string> cameraDeviceNames = getCameraDeviceNames();
    std::vector<AvailableStream> inputStreams;
    std::vector<AvailableZSLInputOutput> inputOutputMap;

    for (const auto& name : cameraDeviceNames) {
        if (getCameraDeviceVersion(name) == CAMERA_DEVICE_API_VERSION_3_2) {
            camera_metadata_t *staticMeta;
            sp<ICameraDeviceSession> session;
            openEmptyDeviceSession(name, env, &session /*out*/,
                    &staticMeta /*out*/);

            Status ret = isZSLModeAvailable(staticMeta);
            if (Status::METHOD_NOT_SUPPORTED == ret) {
                session->close();
                continue;
            }
            ASSERT_EQ(Status::OK, ret);

            inputStreams.clear();
            ASSERT_EQ(Status::OK, getAvailableOutputStreams(staticMeta,
                    inputStreams));
            ASSERT_NE(0u, inputStreams.size());

            inputOutputMap.clear();
            ASSERT_EQ(Status::OK, getZSLInputOutputMap(staticMeta,
                    inputOutputMap));
            ASSERT_NE(0u, inputOutputMap.size());

            int32_t streamId = 0;
            for (auto &inputIter : inputOutputMap) {
                AvailableStream input;
                ASSERT_EQ(Status::OK,
                        findLargestSize(inputStreams, inputIter.inputFormat, input));
                ASSERT_NE(0u, inputStreams.size());

                AvailableStream outputThreshold = {INT32_MAX, INT32_MAX,
                        inputIter.outputFormat};
                std::vector<AvailableStream> outputStreams;
                ASSERT_EQ(Status::OK, getAvailableOutputStreams(staticMeta,
                        outputStreams, &outputThreshold));
                for (auto &outputIter : outputStreams) {
                    Stream zslStream = {streamId++, StreamType::OUTPUT,
                            static_cast<uint32_t> (input.width),
                            static_cast<uint32_t> (input.height),
                            static_cast<PixelFormat> (input.format),
                            GRALLOC_USAGE_HW_CAMERA_ZSL, 0,
                            StreamRotation::ROTATION_0};
                    Stream inputStream = {streamId++, StreamType::INPUT,
                            static_cast<uint32_t> (input.width),
                            static_cast<uint32_t> (input.height),
                            static_cast<PixelFormat> (input.format), 0, 0,
                            StreamRotation::ROTATION_0};
                    Stream outputStream = {streamId++, StreamType::OUTPUT,
                            static_cast<uint32_t> (outputIter.width),
                            static_cast<uint32_t> (outputIter.height),
                            static_cast<PixelFormat> (outputIter.format), 0, 0,
                            StreamRotation::ROTATION_0};

                    ::android::hardware::hidl_vec<Stream> streams = {
                            inputStream, zslStream, outputStream};
                    StreamConfiguration config = {streams,
                            StreamConfigurationMode::NORMAL_MODE};
                    session->configureStreams(config, [streamId] (Status s,
                            HalStreamConfiguration halConfig) {
                        ASSERT_EQ(Status::OK, s);
                        ASSERT_EQ(3u, halConfig.streams.size());
                    });
                }
            }

            free_camera_metadata(staticMeta);
            session->close();
        }
    }
}

// Verify that all supported preview + still capture stream combinations
// can be configured successfully.
TEST_F(CameraHidlTest, configureStreamsPreviewStillOutputs) {
    CameraHidlEnvironment* env = CameraHidlEnvironment::Instance();
    hidl_vec<hidl_string> cameraDeviceNames = getCameraDeviceNames();
    std::vector<AvailableStream> outputBlobStreams;
    std::vector<AvailableStream> outputPreviewStreams;
    AvailableStream previewThreshold = {kMaxPreviewWidth, kMaxPreviewHeight,
            static_cast<int32_t>(PixelFormat::IMPLEMENTATION_DEFINED)};
    AvailableStream blobThreshold = {INT32_MAX, INT32_MAX,
            static_cast<int32_t>(PixelFormat::BLOB)};

    for (const auto& name : cameraDeviceNames) {
        if (getCameraDeviceVersion(name) == CAMERA_DEVICE_API_VERSION_3_2) {
            camera_metadata_t *staticMeta;
            sp<ICameraDeviceSession> session;
            openEmptyDeviceSession(name, env, &session /*out*/,
                    &staticMeta /*out*/);

            outputBlobStreams.clear();
            ASSERT_EQ(Status::OK, getAvailableOutputStreams(staticMeta,
                    outputBlobStreams, &blobThreshold));
            ASSERT_NE(0u, outputBlobStreams.size());

            outputPreviewStreams.clear();
            ASSERT_EQ(Status::OK, getAvailableOutputStreams(staticMeta,
                    outputPreviewStreams, &previewThreshold));
            ASSERT_NE(0u, outputPreviewStreams.size());

            int32_t streamId = 0;
            for (auto &blobIter : outputBlobStreams) {
                for (auto &previewIter : outputPreviewStreams) {
                    Stream previewStream = {streamId++, StreamType::OUTPUT,
                            static_cast<uint32_t> (previewIter.width),
                            static_cast<uint32_t> (previewIter.height),
                            static_cast<PixelFormat> (previewIter.format), 0, 0,
                            StreamRotation::ROTATION_0};
                    Stream blobStream = {streamId++, StreamType::OUTPUT,
                            static_cast<uint32_t> (blobIter.width),
                            static_cast<uint32_t> (blobIter.height),
                            static_cast<PixelFormat> (blobIter.format), 0, 0,
                            StreamRotation::ROTATION_0};
                    ::android::hardware::hidl_vec<Stream> streams = {
                            previewStream, blobStream};
                    StreamConfiguration config = {streams,
                            StreamConfigurationMode::NORMAL_MODE};
                    session->configureStreams(config, [streamId] (Status s,
                            HalStreamConfiguration halConfig) {
                        ASSERT_EQ(Status::OK, s);
                        ASSERT_EQ(2u, halConfig.streams.size());
                    });
                }
            }

            free_camera_metadata(staticMeta);
            session->close();
        }
    }
}

// In case constrained mode is supported, test whether it can be
// configured. Additionally check for common invalid inputs when
// using this mode.
TEST_F(CameraHidlTest, configureStreamsConstrainedOutputs) {
    CameraHidlEnvironment* env = CameraHidlEnvironment::Instance();
    hidl_vec<hidl_string> cameraDeviceNames = getCameraDeviceNames();

    for (const auto& name : cameraDeviceNames) {
        if (getCameraDeviceVersion(name) == CAMERA_DEVICE_API_VERSION_3_2) {
            camera_metadata_t *staticMeta;
            sp<ICameraDeviceSession> session;
            openEmptyDeviceSession(name, env, &session /*out*/,
                    &staticMeta /*out*/);

            Status rc = isConstrainedModeAvailable(staticMeta);
            if (Status::METHOD_NOT_SUPPORTED == rc) {
                session->close();
                continue;
            }
            ASSERT_EQ(Status::OK, rc);

            AvailableStream hfrStream;
            rc = pickConstrainedModeSize(staticMeta, hfrStream);
            ASSERT_EQ(Status::OK, rc);

            int32_t streamId = 0;
            Stream stream = {streamId, StreamType::OUTPUT,
                    static_cast<uint32_t> (hfrStream.width),
                    static_cast<uint32_t> (hfrStream.height),
                    static_cast<PixelFormat> (hfrStream.format), 0, 0,
                    StreamRotation::ROTATION_0};
            ::android::hardware::hidl_vec<Stream> streams = {stream};
            StreamConfiguration config = {streams,
                    StreamConfigurationMode::CONSTRAINED_HIGH_SPEED_MODE};
            session->configureStreams(config, [streamId] (Status s,
                    HalStreamConfiguration halConfig) {
                ASSERT_EQ(Status::OK, s);
                ASSERT_EQ(1u, halConfig.streams.size());
                ASSERT_EQ(halConfig.streams[0].id, streamId);
            });

            stream = {streamId++, StreamType::OUTPUT,
                    static_cast<uint32_t> (0),
                    static_cast<uint32_t> (0),
                    static_cast<PixelFormat> (hfrStream.format), 0, 0,
                    StreamRotation::ROTATION_0};
            streams[0] = stream;
            config = {streams,
                    StreamConfigurationMode::CONSTRAINED_HIGH_SPEED_MODE};
            session->configureStreams(config, [streamId] (Status s,
                    HalStreamConfiguration) {
                ASSERT_TRUE((Status::ILLEGAL_ARGUMENT == s) ||
                            (Status::INTERNAL_ERROR == s));
            });

            stream = {streamId++, StreamType::OUTPUT,
                    static_cast<uint32_t> (UINT32_MAX),
                    static_cast<uint32_t> (UINT32_MAX),
                    static_cast<PixelFormat> (hfrStream.format), 0, 0,
                    StreamRotation::ROTATION_0};
            streams[0] = stream;
            config = {streams,
                    StreamConfigurationMode::CONSTRAINED_HIGH_SPEED_MODE};
            session->configureStreams(config, [streamId] (Status s,
                    HalStreamConfiguration) {
                ASSERT_EQ(Status::ILLEGAL_ARGUMENT, s);
            });

            stream = {streamId++, StreamType::OUTPUT,
                    static_cast<uint32_t> (hfrStream.width),
                    static_cast<uint32_t> (hfrStream.height),
                    static_cast<PixelFormat> (UINT32_MAX), 0, 0,
                    StreamRotation::ROTATION_0};
            streams[0] = stream;
            config = {streams,
                    StreamConfigurationMode::CONSTRAINED_HIGH_SPEED_MODE};
            session->configureStreams(config, [streamId] (Status s,
                    HalStreamConfiguration) {
                ASSERT_EQ(Status::ILLEGAL_ARGUMENT, s);
            });

            free_camera_metadata(staticMeta);
            session->close();
        }
    }
}

// Verify that all supported video + snapshot stream combinations can
// be configured successfully.
TEST_F(CameraHidlTest, configureStreamsVideoStillOutputs) {
    CameraHidlEnvironment* env = CameraHidlEnvironment::Instance();
    hidl_vec<hidl_string> cameraDeviceNames = getCameraDeviceNames();
    std::vector<AvailableStream> outputBlobStreams;
    std::vector<AvailableStream> outputVideoStreams;
    AvailableStream videoThreshold = {kMaxVideoWidth, kMaxVideoHeight,
            static_cast<int32_t>(PixelFormat::IMPLEMENTATION_DEFINED)};
    AvailableStream blobThreshold = {kMaxVideoWidth, kMaxVideoHeight,
            static_cast<int32_t>(PixelFormat::BLOB)};

    for (const auto& name : cameraDeviceNames) {
        if (getCameraDeviceVersion(name) == CAMERA_DEVICE_API_VERSION_3_2) {
            camera_metadata_t *staticMeta;
            sp<ICameraDeviceSession> session;
            openEmptyDeviceSession(name, env, &session /*out*/,
                    &staticMeta /*out*/);

            outputBlobStreams.clear();
            ASSERT_EQ(Status::OK, getAvailableOutputStreams(staticMeta,
                    outputBlobStreams, &blobThreshold));
            ASSERT_NE(0u, outputBlobStreams.size());

            outputVideoStreams.clear();
            ASSERT_EQ(Status::OK, getAvailableOutputStreams(staticMeta,
                    outputVideoStreams, &videoThreshold));
            ASSERT_NE(0u, outputVideoStreams.size());

            int32_t streamId = 0;
            for (auto &blobIter : outputBlobStreams) {
                for (auto &videoIter : outputVideoStreams) {
                    Stream videoStream = {streamId++, StreamType::OUTPUT,
                            static_cast<uint32_t> (videoIter.width),
                            static_cast<uint32_t> (videoIter.height),
                            static_cast<PixelFormat> (videoIter.format), 0, 0,
                            StreamRotation::ROTATION_0};
                    Stream blobStream = {streamId++, StreamType::OUTPUT,
                            static_cast<uint32_t> (blobIter.width),
                            static_cast<uint32_t> (blobIter.height),
                            static_cast<PixelFormat> (blobIter.format),
                            GRALLOC_USAGE_HW_VIDEO_ENCODER, 0,
                            StreamRotation::ROTATION_0};
                    ::android::hardware::hidl_vec<Stream> streams = {
                            videoStream, blobStream};
                    StreamConfiguration config = {streams,
                            StreamConfigurationMode::NORMAL_MODE};
                    session->configureStreams(config, [streamId] (Status s,
                            HalStreamConfiguration halConfig) {
                        ASSERT_EQ(Status::OK, s);
                        ASSERT_EQ(2u, halConfig.streams.size());
                    });
                }
            }

            free_camera_metadata(staticMeta);
            session->close();
        }
    }
}

// Generate and verify a camera capture request
TEST_F(CameraHidlTest, processCaptureRequestPreview) {
    CameraHidlEnvironment* env = CameraHidlEnvironment::Instance();
    hidl_vec<hidl_string> cameraDeviceNames = getCameraDeviceNames();
    AvailableStream previewThreshold = {kMaxPreviewWidth, kMaxPreviewHeight,
            static_cast<int32_t>(PixelFormat::IMPLEMENTATION_DEFINED)};
    uint64_t bufferId = 1;
    uint32_t frameNumber = 1;
    ::android::hardware::hidl_vec<uint8_t> settings;

    for (const auto& name : cameraDeviceNames) {
        if (getCameraDeviceVersion(name) == CAMERA_DEVICE_API_VERSION_3_2) {
            Stream previewStream;
            HalStreamConfiguration halStreamConfig;
            sp<ICameraDeviceSession> session;
            configurePreviewStream(name, env, &previewThreshold,
                    &session /*out*/, &previewStream /*out*/,
                    &halStreamConfig /*out*/);

            RequestTemplate reqTemplate = RequestTemplate::PREVIEW;
            session->constructDefaultRequestSettings(reqTemplate,
                [&](auto status, const auto& req) {
                    ASSERT_EQ(Status::OK, status);
                    settings = req; });

            sp<GraphicBuffer> gb = new GraphicBuffer(previewStream.width,
                    previewStream.height,
                    static_cast<int32_t> (halStreamConfig.streams[0].overrideFormat),
                    1, halStreamConfig.streams[0].producerUsage,
                    halStreamConfig.streams[0].consumerUsage);
            ASSERT_NE(nullptr, gb.get());
            StreamBuffer outputBuffer = {halStreamConfig.streams[0].id,
                    bufferId, hidl_handle(gb->getNativeBuffer()->handle),
                    BufferStatus::OK, nullptr, nullptr};
            ::android::hardware::hidl_vec<StreamBuffer> outputBuffers = {
                    outputBuffer};
            StreamBuffer emptyInputBuffer = {-1, 0, nullptr,
                    BufferStatus::ERROR, nullptr, nullptr};
            CaptureRequest request = {frameNumber, settings,
                    emptyInputBuffer, outputBuffers};

            {
                std::unique_lock<std::mutex> l(mLock);
                mResultBuffers.clear();
                mResultFrameNumber = frameNumber;
            }

            ASSERT_EQ(Status::OK, session->processCaptureRequest(request));

            {
                std::unique_lock<std::mutex> l(mLock);
                while (0 == mResultBuffers.size()) {
                    auto timeout = std::chrono::system_clock::now() +
                            std::chrono::seconds(kStreamBufferTimeoutSec);
                    ASSERT_NE(std::cv_status::timeout,
                            mResultCondition.wait_until(l, timeout));
                }

                ASSERT_EQ(BufferStatus::OK, mResultBuffers[0].status);
                ASSERT_EQ(previewStream.id, mResultBuffers[0].streamId);

                request.frameNumber++;
                //Empty settings should be supported after the first call
                //for repeating requests.
                request.settings.setToExternal(nullptr, 0, true);
                mResultBuffers.clear();
                mResultFrameNumber++;
            }

            ASSERT_EQ(Status::OK, session->processCaptureRequest(request));

            {
                std::unique_lock<std::mutex> l(mLock);
                while (0 == mResultBuffers.size()) {
                    auto timeout = std::chrono::system_clock::now() +
                            std::chrono::seconds(kStreamBufferTimeoutSec);
                    ASSERT_NE(std::cv_status::timeout,
                            mResultCondition.wait_until(l, timeout));
                }
                ASSERT_EQ(BufferStatus::OK, mResultBuffers[0].status);
                ASSERT_EQ(previewStream.id, mResultBuffers[0].streamId);
            }

            session->close();
        }
    }
}

// Test whether an incorrect capture request with missing settings will
// be reported correctly.
TEST_F(CameraHidlTest, processCaptureRequestInvalidSinglePreview) {
    CameraHidlEnvironment* env = CameraHidlEnvironment::Instance();
    hidl_vec<hidl_string> cameraDeviceNames = getCameraDeviceNames();
    std::vector<AvailableStream> outputPreviewStreams;
    AvailableStream previewThreshold = {kMaxPreviewWidth, kMaxPreviewHeight,
            static_cast<int32_t>(PixelFormat::IMPLEMENTATION_DEFINED)};
    uint64_t bufferId = 1;
    uint32_t frameNumber = 1;
    ::android::hardware::hidl_vec<uint8_t> settings;

    for (const auto& name : cameraDeviceNames) {
        if (getCameraDeviceVersion(name) == CAMERA_DEVICE_API_VERSION_3_2) {
            Stream previewStream;
            HalStreamConfiguration halStreamConfig;
            sp<ICameraDeviceSession> session;
            configurePreviewStream(name, env, &previewThreshold,
                    &session /*out*/, &previewStream /*out*/,
                    &halStreamConfig /*out*/);

            sp<GraphicBuffer> gb = new GraphicBuffer(previewStream.width,
                    previewStream.height,
                    static_cast<int32_t> (halStreamConfig.streams[0].overrideFormat),
                    1, halStreamConfig.streams[0].producerUsage,
                    halStreamConfig.streams[0].consumerUsage);

            StreamBuffer outputBuffer = {halStreamConfig.streams[0].id,
                    bufferId, hidl_handle(gb->getNativeBuffer()->handle),
                    BufferStatus::OK, nullptr, nullptr};
            ::android::hardware::hidl_vec<StreamBuffer> outputBuffers = {
                    outputBuffer};
            StreamBuffer emptyInputBuffer = {-1, 0, nullptr,
                    BufferStatus::ERROR, nullptr, nullptr};
            CaptureRequest request = {frameNumber, settings,
                    emptyInputBuffer, outputBuffers};

            //Settings were not correctly initialized, we should fail here
            ASSERT_EQ(Status::INTERNAL_ERROR,
                    session->processCaptureRequest(request));

            session->close();
        }
    }
}

// Check whether an invalid capture request with missing output buffers
// will be reported correctly.
TEST_F(CameraHidlTest, processCaptureRequestInvalidBuffer) {
    CameraHidlEnvironment* env = CameraHidlEnvironment::Instance();
    hidl_vec<hidl_string> cameraDeviceNames = getCameraDeviceNames();
    std::vector<AvailableStream> outputBlobStreams;
    AvailableStream previewThreshold = {kMaxPreviewWidth, kMaxPreviewHeight,
            static_cast<int32_t>(PixelFormat::IMPLEMENTATION_DEFINED)};
    uint32_t frameNumber = 1;
    ::android::hardware::hidl_vec<uint8_t> settings;

    for (const auto& name : cameraDeviceNames) {
        if (getCameraDeviceVersion(name) == CAMERA_DEVICE_API_VERSION_3_2) {
            Stream previewStream;
            HalStreamConfiguration halStreamConfig;
            sp<ICameraDeviceSession> session;
            configurePreviewStream(name, env, &previewThreshold,
                    &session /*out*/, &previewStream /*out*/,
                    &halStreamConfig /*out*/);

            RequestTemplate reqTemplate = RequestTemplate::PREVIEW;
            session->constructDefaultRequestSettings(reqTemplate,
                [&](auto status, const auto& req) {
                    ASSERT_EQ(Status::OK, status);
                    settings = req; });

            ::android::hardware::hidl_vec<StreamBuffer> emptyOutputBuffers;
            StreamBuffer emptyInputBuffer = {-1, 0, nullptr,
                    BufferStatus::ERROR, nullptr, nullptr};
            CaptureRequest request = {frameNumber, settings,
                    emptyInputBuffer, emptyOutputBuffers};

            //Output buffers are missing, we should fail here
            ASSERT_EQ(Status::INTERNAL_ERROR,
                    session->processCaptureRequest(request));

            session->close();
        }
    }
}

// Generate, trigger and flush a preview request
TEST_F(CameraHidlTest, flushPreviewRequest) {
    CameraHidlEnvironment* env = CameraHidlEnvironment::Instance();
    hidl_vec<hidl_string> cameraDeviceNames = getCameraDeviceNames();
    std::vector<AvailableStream> outputPreviewStreams;
    AvailableStream previewThreshold = {kMaxPreviewWidth, kMaxPreviewHeight,
            static_cast<int32_t>(PixelFormat::IMPLEMENTATION_DEFINED)};
    uint64_t bufferId = 1;
    uint32_t frameNumber = 1;
    ::android::hardware::hidl_vec<uint8_t> settings;

    for (const auto& name : cameraDeviceNames) {
        if (getCameraDeviceVersion(name) == CAMERA_DEVICE_API_VERSION_3_2) {
            Stream previewStream;
            HalStreamConfiguration halStreamConfig;
            sp<ICameraDeviceSession> session;
            configurePreviewStream(name, env, &previewThreshold,
                    &session /*out*/, &previewStream /*out*/,
                    &halStreamConfig /*out*/);

            RequestTemplate reqTemplate = RequestTemplate::PREVIEW;
            session->constructDefaultRequestSettings(reqTemplate,
                [&](auto status, const auto& req) {
                    ASSERT_EQ(Status::OK, status);
                    settings = req; });

            sp<GraphicBuffer> gb = new GraphicBuffer(previewStream.width,
                    previewStream.height,
                    static_cast<int32_t> (halStreamConfig.streams[0].overrideFormat),
                    1, halStreamConfig.streams[0].producerUsage,
                    halStreamConfig.streams[0].consumerUsage);
            ASSERT_NE(nullptr, gb.get());
            StreamBuffer outputBuffer = {halStreamConfig.streams[0].id,
                    bufferId, hidl_handle(gb->getNativeBuffer()->handle),
                    BufferStatus::OK, nullptr, nullptr};
            ::android::hardware::hidl_vec<StreamBuffer> outputBuffers = {
                    outputBuffer};
            const StreamBuffer emptyInputBuffer = {-1, 0, nullptr,
                    BufferStatus::ERROR, nullptr, nullptr};
            CaptureRequest request = {frameNumber, settings,
                    emptyInputBuffer, outputBuffers};

            {
                std::unique_lock<std::mutex> l(mLock);
                mResultBuffers.clear();
                mErrors.clear();
                mResultFrameNumber = frameNumber;
            }

            ASSERT_EQ(Status::OK, session->processCaptureRequest(request));
            //Flush before waiting for request to complete.
            ASSERT_EQ(Status::OK, session->flush());

            {
                std::unique_lock<std::mutex> l(mLock);
                while ((0 == mResultBuffers.size()) && (0 == mErrors.size())) {
                    auto timeout = std::chrono::system_clock::now() +
                            std::chrono::seconds(kStreamBufferTimeoutSec);
                    ASSERT_NE(std::cv_status::timeout,
                            mResultCondition.wait_until(l, timeout));
                }

                if (mErrors.empty()) {
                    ASSERT_EQ(BufferStatus::OK, mResultBuffers[0].status);
                    ASSERT_EQ(previewStream.id, mResultBuffers[0].streamId);
                } else {
                    for (auto &error : mErrors) {
                        switch (error.errorCode) {
                            case ErrorCode::ERROR_REQUEST:
                            case ErrorCode::ERROR_RESULT:
                                //Expected
                                break;
                            case ErrorCode::ERROR_BUFFER:
                                //Expected as well
                                ASSERT_EQ(frameNumber, error.frameNumber);
                                ASSERT_EQ(previewStream.id, error.errorStreamId);
                                break;
                            case ErrorCode::ERROR_DEVICE:
                            default:
                                FAIL() <<"Unexpected error:" << static_cast<uint32_t> (error.errorCode);
                        }
                    }
                }
            }

            session->close();
        }
    }
}

// Verify that camera flushes correctly without any pending requests.
TEST_F(CameraHidlTest, flushEmpty) {
    CameraHidlEnvironment* env = CameraHidlEnvironment::Instance();
    hidl_vec<hidl_string> cameraDeviceNames = getCameraDeviceNames();
    std::vector<AvailableStream> outputPreviewStreams;
    AvailableStream previewThreshold = {kMaxPreviewWidth, kMaxPreviewHeight,
            static_cast<int32_t>(PixelFormat::IMPLEMENTATION_DEFINED)};

    for (const auto& name : cameraDeviceNames) {
        if (getCameraDeviceVersion(name) == CAMERA_DEVICE_API_VERSION_3_2) {
            Stream previewStream;
            HalStreamConfiguration halStreamConfig;
            sp<ICameraDeviceSession> session;
            configurePreviewStream(name, env, &previewThreshold,
                    &session /*out*/, &previewStream /*out*/,
                    &halStreamConfig /*out*/);

            {
                std::unique_lock<std::mutex> l(mLock);
                mResultBuffers.clear();
                mErrors.clear();
                mResultFrameNumber = 0;
            }

            ASSERT_EQ(Status::OK, session->flush());

            {
                std::unique_lock<std::mutex> l(mLock);
                auto timeout = std::chrono::system_clock::now() +
                        std::chrono::milliseconds(kEmptyFlushTimeoutMSec);
                ASSERT_EQ(std::cv_status::timeout,
                        mResultCondition.wait_until(l, timeout));
                ASSERT_TRUE(mErrors.empty());
                ASSERT_TRUE(mResultBuffers.empty());
            }

            session->close();
        }
    }
}

// Retrieve all valid output stream resolutions from the camera
// static characteristics.
Status CameraHidlTest::getAvailableOutputStreams(camera_metadata_t *staticMeta,
        std::vector<AvailableStream> &outputStreams,
        const AvailableStream *threshold) {
    if (nullptr == staticMeta) {
        return Status::ILLEGAL_ARGUMENT;
    }

    camera_metadata_ro_entry entry;
    int rc = find_camera_metadata_ro_entry(staticMeta,
            ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS, &entry);
    if ((0 != rc) || (0 != (entry.count % 4))) {
        return Status::ILLEGAL_ARGUMENT;
    }

    for (size_t i = 0; i < entry.count; i+=4) {
        if (ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT ==
                entry.data.i32[i + 3]) {
            if(nullptr == threshold) {
                AvailableStream s = {entry.data.i32[i+1],
                        entry.data.i32[i+2], entry.data.i32[i]};
                outputStreams.push_back(s);
            } else {
                if ((threshold->format == entry.data.i32[i]) &&
                        (threshold->width >= entry.data.i32[i+1]) &&
                        (threshold->height >= entry.data.i32[i+2])) {
                    AvailableStream s = {entry.data.i32[i+1],
                            entry.data.i32[i+2], threshold->format};
                    outputStreams.push_back(s);
                }
            }
        }

    }

    return Status::OK;
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
Status CameraHidlTest::isZSLModeAvailable(camera_metadata_t *staticMeta) {
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
        if ((ANDROID_REQUEST_AVAILABLE_CAPABILITIES_PRIVATE_REPROCESSING ==
                entry.data.u8[i]) ||
                (ANDROID_REQUEST_AVAILABLE_CAPABILITIES_YUV_REPROCESSING ==
                        entry.data.u8[i]) ){
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

// Open a device session and configure a preview stream.
void CameraHidlTest::configurePreviewStream(const std::string &name,
        const CameraHidlEnvironment* env,
        const AvailableStream *previewThreshold,
        sp<ICameraDeviceSession> *session /*out*/,
        Stream *previewStream /*out*/,
        HalStreamConfiguration *halStreamConfig /*out*/) {
    ASSERT_NE(nullptr, env);
    ASSERT_NE(nullptr, session);
    ASSERT_NE(nullptr, previewStream);
    ASSERT_NE(nullptr, halStreamConfig);

    std::vector<AvailableStream> outputPreviewStreams;
    ::android::sp<ICameraDevice> device3_2;
    ALOGI("configureStreams: Testing camera device %s", name.c_str());
    env->mProvider->getCameraDeviceInterface_V3_x(
        name,
        [&](auto status, const auto& device) {
            ALOGI("getCameraDeviceInterface_V3_x returns status:%d",
                  (int)status);
            ASSERT_EQ(Status::OK, status);
            ASSERT_NE(device, nullptr);
            device3_2 = device;
        });

    sp<DeviceCb> cb = new DeviceCb(this);
    device3_2->open(
        cb,
        [&](auto status, const auto& newSession) {
            ALOGI("device::open returns status:%d", (int)status);
            ASSERT_EQ(Status::OK, status);
            ASSERT_NE(newSession, nullptr);
            *session = newSession;
        });

    camera_metadata_t *staticMeta;
    device3_2->getCameraCharacteristics([&] (Status s,
            CameraMetadata metadata) {
        ASSERT_EQ(Status::OK, s);
        staticMeta = clone_camera_metadata(
                reinterpret_cast<const camera_metadata_t*>(metadata.data()));
         ASSERT_NE(nullptr, staticMeta);
    });

    outputPreviewStreams.clear();
    auto rc = getAvailableOutputStreams(staticMeta,
            outputPreviewStreams, previewThreshold);
    free_camera_metadata(staticMeta);
    ASSERT_EQ(Status::OK, rc);
    ASSERT_FALSE(outputPreviewStreams.empty());

    *previewStream = {0, StreamType::OUTPUT,
            static_cast<uint32_t> (outputPreviewStreams[0].width),
            static_cast<uint32_t> (outputPreviewStreams[0].height),
            static_cast<PixelFormat> (outputPreviewStreams[0].format),
            0, 0, StreamRotation::ROTATION_0};
    ::android::hardware::hidl_vec<Stream> streams = {*previewStream};
    StreamConfiguration config = {streams,
            StreamConfigurationMode::NORMAL_MODE};
    (*session)->configureStreams(config, [&] (Status s,
            HalStreamConfiguration halConfig) {
        ASSERT_EQ(Status::OK, s);
        ASSERT_EQ(1u, halConfig.streams.size());
        *halStreamConfig = halConfig;
    });
}

// Open a device session with empty callbacks and return static metadata.
void CameraHidlTest::openEmptyDeviceSession(const std::string &name,
        const CameraHidlEnvironment* env,
        sp<ICameraDeviceSession> *session /*out*/,
        camera_metadata_t **staticMeta /*out*/) {
    ASSERT_NE(nullptr, env);
    ASSERT_NE(nullptr, session);
    ASSERT_NE(nullptr, staticMeta);

    ::android::sp<ICameraDevice> device3_2;
    ALOGI("configureStreams: Testing camera device %s", name.c_str());
    env->mProvider->getCameraDeviceInterface_V3_x(
        name,
        [&](auto status, const auto& device) {
            ALOGI("getCameraDeviceInterface_V3_x returns status:%d",
                  (int)status);
            ASSERT_EQ(Status::OK, status);
            ASSERT_NE(device, nullptr);
            device3_2 = device;
        });

    sp<EmptyDeviceCb> cb = new EmptyDeviceCb();
    device3_2->open(cb, [&](auto status, const auto& newSession) {
            ALOGI("device::open returns status:%d", (int)status);
            ASSERT_EQ(Status::OK, status);
            ASSERT_NE(newSession, nullptr);
            *session = newSession;
        });

    device3_2->getCameraCharacteristics([&] (Status s,
            CameraMetadata metadata) {
        ASSERT_EQ(Status::OK, s);
        *staticMeta = clone_camera_metadata(
                reinterpret_cast<const camera_metadata_t*>(metadata.data()));
        ASSERT_NE(nullptr, *staticMeta);
    });
}

int main(int argc, char **argv) {
  ::testing::AddGlobalTestEnvironment(CameraHidlEnvironment::Instance());
  ::testing::InitGoogleTest(&argc, argv);
  int status = RUN_ALL_TESTS();
  ALOGI("Test result = %d", status);
  return status;
}
