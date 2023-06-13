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

#include <aidl/Vintf.h>
#include <aidl/android/hardware/camera/common/VendorTagSection.h>
#include <aidl/android/hardware/camera/device/ICameraDevice.h>
#include <aidlcommonsupport/NativeHandle.h>
#include <camera_aidl_test.h>
#include <cutils/properties.h>
#include <device_cb.h>
#include <empty_device_cb.h>
#include <grallocusage/GrallocUsageConversion.h>
#include <gtest/gtest.h>
#include <hardware/gralloc.h>
#include <hardware/gralloc1.h>
#include <hidl/GtestPrinter.h>
#include <hidl/HidlSupport.h>
#include <torch_provider_cb.h>
#include <list>

using ::aidl::android::hardware::camera::common::CameraDeviceStatus;
using ::aidl::android::hardware::camera::common::CameraResourceCost;
using ::aidl::android::hardware::camera::common::TorchModeStatus;
using ::aidl::android::hardware::camera::common::VendorTagSection;
using ::aidl::android::hardware::camera::device::ICameraDevice;
using ::aidl::android::hardware::camera::metadata::RequestAvailableDynamicRangeProfilesMap;
using ::aidl::android::hardware::camera::metadata::SensorPixelMode;
using ::aidl::android::hardware::camera::provider::CameraIdAndStreamCombination;
using ::aidl::android::hardware::camera::provider::BnCameraProviderCallback;

using ::ndk::ScopedAStatus;

namespace {
const int32_t kBurstFrameCount = 10;
const uint32_t kMaxStillWidth = 2048;
const uint32_t kMaxStillHeight = 1536;

const int64_t kEmptyFlushTimeoutMSec = 200;

const static std::vector<int64_t> kMandatoryUseCases = {
        ANDROID_SCALER_AVAILABLE_STREAM_USE_CASES_DEFAULT,
        ANDROID_SCALER_AVAILABLE_STREAM_USE_CASES_PREVIEW,
        ANDROID_SCALER_AVAILABLE_STREAM_USE_CASES_STILL_CAPTURE,
        ANDROID_SCALER_AVAILABLE_STREAM_USE_CASES_VIDEO_RECORD,
        ANDROID_SCALER_AVAILABLE_STREAM_USE_CASES_PREVIEW_VIDEO_STILL,
        ANDROID_SCALER_AVAILABLE_STREAM_USE_CASES_VIDEO_CALL};
}  // namespace

TEST_P(CameraAidlTest, getCameraIdList) {
    std::vector<std::string> idList;
    ScopedAStatus ret = mProvider->getCameraIdList(&idList);
    ASSERT_TRUE(ret.isOk());

    for (size_t i = 0; i < idList.size(); i++) {
        ALOGI("Camera Id[%zu] is %s", i, idList[i].c_str());
    }
}

// Test if ICameraProvider::getVendorTags returns Status::OK
TEST_P(CameraAidlTest, getVendorTags) {
    std::vector<VendorTagSection> vendorTags;
    ScopedAStatus ret = mProvider->getVendorTags(&vendorTags);

    ASSERT_TRUE(ret.isOk());
    for (size_t i = 0; i < vendorTags.size(); i++) {
        ALOGI("Vendor tag section %zu name %s", i, vendorTags[i].sectionName.c_str());
        for (auto& tag : vendorTags[i].tags) {
            ALOGI("Vendor tag id %u name %s type %d", tag.tagId, tag.tagName.c_str(),
                  (int)tag.tagType);
        }
    }
}

// Test if ICameraProvider::setCallback returns Status::OK
TEST_P(CameraAidlTest, setCallback) {
    struct ProviderCb : public BnCameraProviderCallback {
        ScopedAStatus cameraDeviceStatusChange(const std::string& cameraDeviceName,
                                               CameraDeviceStatus newStatus) override {
            ALOGI("camera device status callback name %s, status %d", cameraDeviceName.c_str(),
                  (int)newStatus);
            return ScopedAStatus::ok();
        }
        ScopedAStatus torchModeStatusChange(const std::string& cameraDeviceName,
                                            TorchModeStatus newStatus) override {
            ALOGI("Torch mode status callback name %s, status %d", cameraDeviceName.c_str(),
                  (int)newStatus);
            return ScopedAStatus::ok();
        }
        ScopedAStatus physicalCameraDeviceStatusChange(const std::string& cameraDeviceName,
                                                       const std::string& physicalCameraDeviceName,
                                                       CameraDeviceStatus newStatus) override {
            ALOGI("physical camera device status callback name %s, physical camera name %s,"
                  " status %d",
                  cameraDeviceName.c_str(), physicalCameraDeviceName.c_str(), (int)newStatus);
            return ScopedAStatus::ok();
        }
    };

    std::shared_ptr<ProviderCb> cb = ndk::SharedRefBase::make<ProviderCb>();
    ScopedAStatus ret = mProvider->setCallback(cb);
    ASSERT_TRUE(ret.isOk());
    ret = mProvider->setCallback(nullptr);
    ASSERT_EQ(static_cast<int32_t>(Status::ILLEGAL_ARGUMENT), ret.getServiceSpecificError());
}

// Test if ICameraProvider::getCameraDeviceInterface returns Status::OK and non-null device
TEST_P(CameraAidlTest, getCameraDeviceInterface) {
    std::vector<std::string> cameraDeviceNames = getCameraDeviceNames(mProvider);

    for (const auto& name : cameraDeviceNames) {
        std::shared_ptr<ICameraDevice> cameraDevice;
        ScopedAStatus ret = mProvider->getCameraDeviceInterface(name, &cameraDevice);
        ALOGI("getCameraDeviceInterface returns: %d:%d", ret.getExceptionCode(),
              ret.getServiceSpecificError());
        ASSERT_TRUE(ret.isOk());
        ASSERT_NE(cameraDevice, nullptr);
    }
}

// Verify that the device resource cost can be retrieved and the values are
// correct.
TEST_P(CameraAidlTest, getResourceCost) {
    std::vector<std::string> cameraDeviceNames = getCameraDeviceNames(mProvider);

    for (const auto& deviceName : cameraDeviceNames) {
        std::shared_ptr<ICameraDevice> cameraDevice;
        ScopedAStatus ret = mProvider->getCameraDeviceInterface(deviceName, &cameraDevice);
        ALOGI("getCameraDeviceInterface returns: %d:%d", ret.getExceptionCode(),
              ret.getServiceSpecificError());
        ASSERT_TRUE(ret.isOk());
        ASSERT_NE(cameraDevice, nullptr);

        CameraResourceCost resourceCost;
        ret = cameraDevice->getResourceCost(&resourceCost);
        ALOGI("getResourceCost returns: %d:%d", ret.getExceptionCode(),
              ret.getServiceSpecificError());
        ASSERT_TRUE(ret.isOk());

        ALOGI("    Resource cost is %d", resourceCost.resourceCost);
        ASSERT_LE(resourceCost.resourceCost, 100u);

        for (const auto& name : resourceCost.conflictingDevices) {
            ALOGI("    Conflicting device: %s", name.c_str());
        }
    }
}

TEST_P(CameraAidlTest, systemCameraTest) {
    std::vector<std::string> cameraDeviceNames = getCameraDeviceNames(mProvider);
    std::map<std::string, std::vector<SystemCameraKind>> hiddenPhysicalIdToLogicalMap;
    for (const auto& name : cameraDeviceNames) {
        std::shared_ptr<ICameraDevice> device;
        ALOGI("systemCameraTest: Testing camera device %s", name.c_str());
        ndk::ScopedAStatus ret = mProvider->getCameraDeviceInterface(name, &device);
        ASSERT_TRUE(ret.isOk());
        ASSERT_NE(device, nullptr);

        CameraMetadata cameraCharacteristics;
        ret = device->getCameraCharacteristics(&cameraCharacteristics);
        ASSERT_TRUE(ret.isOk());

        const camera_metadata_t* staticMeta =
                reinterpret_cast<const camera_metadata_t*>(cameraCharacteristics.metadata.data());
        Status rc = isLogicalMultiCamera(staticMeta);
        if (rc == Status::OPERATION_NOT_SUPPORTED) {
            return;
        }

        ASSERT_EQ(rc, Status::OK);
        std::unordered_set<std::string> physicalIds;
        ASSERT_EQ(getPhysicalCameraIds(staticMeta, &physicalIds), Status::OK);
        SystemCameraKind systemCameraKind = SystemCameraKind::PUBLIC;
        Status retStatus = getSystemCameraKind(staticMeta, &systemCameraKind);
        ASSERT_EQ(retStatus, Status::OK);

        for (auto physicalId : physicalIds) {
            bool isPublicId = false;
            for (auto& deviceName : cameraDeviceNames) {
                std::string publicVersion, publicId;
                ASSERT_TRUE(matchDeviceName(deviceName, mProviderType, &publicVersion, &publicId));
                if (physicalId == publicId) {
                    isPublicId = true;
                    break;
                }
            }

            // For hidden physical cameras, collect their associated logical cameras
            // and store the system camera kind.
            if (!isPublicId) {
                auto it = hiddenPhysicalIdToLogicalMap.find(physicalId);
                if (it == hiddenPhysicalIdToLogicalMap.end()) {
                    hiddenPhysicalIdToLogicalMap.insert(std::make_pair(
                            physicalId, std::vector<SystemCameraKind>({systemCameraKind})));
                } else {
                    it->second.push_back(systemCameraKind);
                }
            }
        }
    }

    // Check that the system camera kind of the logical cameras associated with
    // each hidden physical camera is the same.
    for (const auto& it : hiddenPhysicalIdToLogicalMap) {
        SystemCameraKind neededSystemCameraKind = it.second.front();
        for (auto foundSystemCamera : it.second) {
            ASSERT_EQ(neededSystemCameraKind, foundSystemCamera);
        }
    }
}

// Verify that the static camera characteristics can be retrieved
// successfully.
TEST_P(CameraAidlTest, getCameraCharacteristics) {
    std::vector<std::string> cameraDeviceNames = getCameraDeviceNames(mProvider);

    for (const auto& name : cameraDeviceNames) {
        std::shared_ptr<ICameraDevice> device;
        ALOGI("getCameraCharacteristics: Testing camera device %s", name.c_str());
        ndk::ScopedAStatus ret = mProvider->getCameraDeviceInterface(name, &device);
        ALOGI("getCameraDeviceInterface returns: %d:%d", ret.getExceptionCode(),
              ret.getServiceSpecificError());
        ASSERT_TRUE(ret.isOk());
        ASSERT_NE(device, nullptr);

        CameraMetadata chars;
        ret = device->getCameraCharacteristics(&chars);
        ASSERT_TRUE(ret.isOk());
        verifyCameraCharacteristics(chars);
        verifyMonochromeCharacteristics(chars);
        verifyRecommendedConfigs(chars);
        verifyLogicalOrUltraHighResCameraMetadata(name, device, chars, cameraDeviceNames);

        ASSERT_TRUE(ret.isOk());

        // getPhysicalCameraCharacteristics will fail for publicly
        // advertised camera IDs.
        std::string version, cameraId;
        ASSERT_TRUE(matchDeviceName(name, mProviderType, &version, &cameraId));
        CameraMetadata devChars;
        ret = device->getPhysicalCameraCharacteristics(cameraId, &devChars);
        ASSERT_EQ(static_cast<int32_t>(Status::ILLEGAL_ARGUMENT), ret.getServiceSpecificError());
        ASSERT_EQ(0, devChars.metadata.size());
    }
}

// Verify that the torch strength level can be set and retrieved successfully.
TEST_P(CameraAidlTest, turnOnTorchWithStrengthLevel) {
    std::vector<std::string> cameraDeviceNames = getCameraDeviceNames(mProvider);

    std::shared_ptr<TorchProviderCb> cb = ndk::SharedRefBase::make<TorchProviderCb>(this);
    ndk::ScopedAStatus ret = mProvider->setCallback(cb);
    ASSERT_TRUE(ret.isOk());

    for (const auto& name : cameraDeviceNames) {
        int32_t defaultLevel;
        std::shared_ptr<ICameraDevice> device;
        ALOGI("%s: Testing camera device %s", __FUNCTION__, name.c_str());

        ret = mProvider->getCameraDeviceInterface(name, &device);
        ASSERT_TRUE(ret.isOk());
        ASSERT_NE(device, nullptr);

        CameraMetadata chars;
        ret = device->getCameraCharacteristics(&chars);
        ASSERT_TRUE(ret.isOk());

        const camera_metadata_t* staticMeta =
                reinterpret_cast<const camera_metadata_t*>(chars.metadata.data());
        bool torchStrengthControlSupported = isTorchStrengthControlSupported(staticMeta);
        camera_metadata_ro_entry entry;
        int rc = find_camera_metadata_ro_entry(staticMeta,
                                               ANDROID_FLASH_INFO_STRENGTH_DEFAULT_LEVEL, &entry);
        if (torchStrengthControlSupported) {
            ASSERT_EQ(rc, 0);
            ASSERT_GT(entry.count, 0);
            defaultLevel = *entry.data.i32;
            ALOGI("Default level is:%d", defaultLevel);
        }

        mTorchStatus = TorchModeStatus::NOT_AVAILABLE;
        ret = device->turnOnTorchWithStrengthLevel(2);
        ALOGI("turnOnTorchWithStrengthLevel returns status: %d", ret.getServiceSpecificError());
        // OPERATION_NOT_SUPPORTED check
        if (!torchStrengthControlSupported) {
            ALOGI("Torch strength control not supported.");
            ASSERT_EQ(static_cast<int32_t>(Status::OPERATION_NOT_SUPPORTED),
                      ret.getServiceSpecificError());
        } else {
            {
                ASSERT_TRUE(ret.isOk());
                std::unique_lock<std::mutex> l(mTorchLock);
                while (TorchModeStatus::NOT_AVAILABLE == mTorchStatus) {
                    auto timeout = std::chrono::system_clock::now() +
                                   std::chrono::seconds(kTorchTimeoutSec);
                    ASSERT_NE(std::cv_status::timeout, mTorchCond.wait_until(l, timeout));
                }
                ASSERT_EQ(TorchModeStatus::AVAILABLE_ON, mTorchStatus);
                mTorchStatus = TorchModeStatus::NOT_AVAILABLE;
            }
            ALOGI("getTorchStrengthLevel: Testing");
            int32_t strengthLevel;
            ret = device->getTorchStrengthLevel(&strengthLevel);
            ASSERT_TRUE(ret.isOk());
            ALOGI("Torch strength level is : %d", strengthLevel);
            ASSERT_EQ(strengthLevel, 2);

            // Turn OFF the torch and verify torch strength level is reset to default level.
            ALOGI("Testing torch strength level reset after turning the torch OFF.");
            ret = device->setTorchMode(false);
            ASSERT_TRUE(ret.isOk());
            {
                std::unique_lock<std::mutex> l(mTorchLock);
                while (TorchModeStatus::NOT_AVAILABLE == mTorchStatus) {
                    auto timeout = std::chrono::system_clock::now() +
                                   std::chrono::seconds(kTorchTimeoutSec);
                    ASSERT_NE(std::cv_status::timeout, mTorchCond.wait_until(l, timeout));
                }
                ASSERT_EQ(TorchModeStatus::AVAILABLE_OFF, mTorchStatus);
            }

            ret = device->getTorchStrengthLevel(&strengthLevel);
            ASSERT_TRUE(ret.isOk());
            ALOGI("Torch strength level after turning OFF torch is : %d", strengthLevel);
            ASSERT_EQ(strengthLevel, defaultLevel);
        }
    }
}

// In case it is supported verify that torch can be enabled.
// Check for corresponding torch callbacks as well.
TEST_P(CameraAidlTest, setTorchMode) {
    std::vector<std::string> cameraDeviceNames = getCameraDeviceNames(mProvider);

    std::shared_ptr<TorchProviderCb> cb = ndk::SharedRefBase::make<TorchProviderCb>(this);
    ndk::ScopedAStatus ret = mProvider->setCallback(cb);
    ALOGI("setCallback returns status: %d", ret.getServiceSpecificError());
    ASSERT_TRUE(ret.isOk());
    ASSERT_NE(cb, nullptr);

    for (const auto& name : cameraDeviceNames) {
        std::shared_ptr<ICameraDevice> device;
        ALOGI("setTorchMode: Testing camera device %s", name.c_str());
        ret = mProvider->getCameraDeviceInterface(name, &device);
        ALOGI("getCameraDeviceInterface returns status:%d:%d", ret.getExceptionCode(),
              ret.getServiceSpecificError());
        ASSERT_TRUE(ret.isOk());
        ASSERT_NE(device, nullptr);

        CameraMetadata metadata;
        ret = device->getCameraCharacteristics(&metadata);
        ALOGI("getCameraCharacteristics returns status:%d", ret.getServiceSpecificError());
        ASSERT_TRUE(ret.isOk());
        camera_metadata_t* staticMeta =
                reinterpret_cast<camera_metadata_t*>(metadata.metadata.data());
        bool torchSupported = isTorchSupported(staticMeta);

        mTorchStatus = TorchModeStatus::NOT_AVAILABLE;
        ret = device->setTorchMode(true);
        ALOGI("setTorchMode returns status: %d", ret.getServiceSpecificError());
        if (!torchSupported) {
            ASSERT_EQ(static_cast<int32_t>(Status::OPERATION_NOT_SUPPORTED),
                      ret.getServiceSpecificError());
        } else {
            ASSERT_TRUE(ret.isOk());
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

            ret = device->setTorchMode(false);
            ASSERT_TRUE(ret.isOk());
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

// Check dump functionality.
TEST_P(CameraAidlTest, dump) {
    std::vector<std::string> cameraDeviceNames = getCameraDeviceNames(mProvider);

    for (const auto& name : cameraDeviceNames) {
        std::shared_ptr<ICameraDevice> device;
        ALOGI("dump: Testing camera device %s", name.c_str());

        ndk::ScopedAStatus ret = mProvider->getCameraDeviceInterface(name, &device);
        ALOGI("getCameraDeviceInterface returns status:%d:%d", ret.getExceptionCode(),
              ret.getServiceSpecificError());
        ASSERT_TRUE(ret.isOk());
        ASSERT_NE(device, nullptr);

        int raw_handle = open(kDumpOutput, O_RDWR);
        ASSERT_GE(raw_handle, 0);

        auto retStatus = device->dump(raw_handle, nullptr, 0);
        ASSERT_EQ(retStatus, ::android::OK);
        close(raw_handle);
    }
}

// Open, dump, then close
TEST_P(CameraAidlTest, openClose) {
    std::vector<std::string> cameraDeviceNames = getCameraDeviceNames(mProvider);

    for (const auto& name : cameraDeviceNames) {
        std::shared_ptr<ICameraDevice> device;
        ALOGI("openClose: Testing camera device %s", name.c_str());
        ndk::ScopedAStatus ret = mProvider->getCameraDeviceInterface(name, &device);
        ALOGI("getCameraDeviceInterface returns status:%d:%d", ret.getExceptionCode(),
              ret.getServiceSpecificError());
        ASSERT_TRUE(ret.isOk());
        ASSERT_NE(device, nullptr);

        std::shared_ptr<EmptyDeviceCb> cb = ndk::SharedRefBase::make<EmptyDeviceCb>();

        ret = device->open(cb, &mSession);
        ASSERT_TRUE(ret.isOk());
        ALOGI("device::open returns status:%d:%d", ret.getExceptionCode(),
              ret.getServiceSpecificError());
        ASSERT_NE(mSession, nullptr);
        int raw_handle = open(kDumpOutput, O_RDWR);
        ASSERT_GE(raw_handle, 0);

        auto retStatus = device->dump(raw_handle, nullptr, 0);
        ASSERT_EQ(retStatus, ::android::OK);
        close(raw_handle);

        ret = mSession->close();
        mSession = nullptr;
        ASSERT_TRUE(ret.isOk());
        // TODO: test all session API calls return INTERNAL_ERROR after close
        // TODO: keep a wp copy here and verify session cannot be promoted out of this scope
    }
}

// Check whether all common default request settings can be successfully
// constructed.
TEST_P(CameraAidlTest, constructDefaultRequestSettings) {
    std::vector<std::string> cameraDeviceNames = getCameraDeviceNames(mProvider);

    for (const auto& name : cameraDeviceNames) {
        std::shared_ptr<ICameraDevice> device;
        ALOGI("constructDefaultRequestSettings: Testing camera device %s", name.c_str());
        ndk::ScopedAStatus ret = mProvider->getCameraDeviceInterface(name, &device);
        ALOGI("getCameraDeviceInterface returns status:%d:%d", ret.getExceptionCode(),
              ret.getServiceSpecificError());
        ASSERT_TRUE(ret.isOk());
        ASSERT_NE(device, nullptr);

        std::shared_ptr<EmptyDeviceCb> cb = ndk::SharedRefBase::make<EmptyDeviceCb>();
        ret = device->open(cb, &mSession);
        ALOGI("device::open returns status:%d:%d", ret.getExceptionCode(),
              ret.getServiceSpecificError());
        ASSERT_TRUE(ret.isOk());
        ASSERT_NE(mSession, nullptr);

        for (int32_t t = (int32_t)RequestTemplate::PREVIEW; t <= (int32_t)RequestTemplate::MANUAL;
             t++) {
            RequestTemplate reqTemplate = (RequestTemplate)t;
            CameraMetadata rawMetadata;
            ret = mSession->constructDefaultRequestSettings(reqTemplate, &rawMetadata);
            ALOGI("constructDefaultRequestSettings returns status:%d:%d", ret.getExceptionCode(),
                  ret.getServiceSpecificError());

            if (reqTemplate == RequestTemplate::ZERO_SHUTTER_LAG ||
                reqTemplate == RequestTemplate::MANUAL) {
                // optional templates
                ASSERT_TRUE(ret.isOk() || static_cast<int32_t>(Status::ILLEGAL_ARGUMENT) ==
                                                  ret.getServiceSpecificError());
            } else {
                ASSERT_TRUE(ret.isOk());
            }

            if (ret.isOk()) {
                const camera_metadata_t* metadata = (camera_metadata_t*)rawMetadata.metadata.data();
                size_t expectedSize = rawMetadata.metadata.size();
                int result = validate_camera_metadata_structure(metadata, &expectedSize);
                ASSERT_TRUE((result == 0) || (result == CAMERA_METADATA_VALIDATION_SHIFTED));
                verifyRequestTemplate(metadata, reqTemplate);
            } else {
                ASSERT_EQ(0u, rawMetadata.metadata.size());
            }
        }
        ret = mSession->close();
        mSession = nullptr;
        ASSERT_TRUE(ret.isOk());
    }
}

// Verify that all supported stream formats and sizes can be configured
// successfully.
TEST_P(CameraAidlTest, configureStreamsAvailableOutputs) {
    std::vector<std::string> cameraDeviceNames = getCameraDeviceNames(mProvider);
    std::vector<AvailableStream> outputStreams;

    for (const auto& name : cameraDeviceNames) {
        CameraMetadata meta;
        std::shared_ptr<ICameraDevice> device;

        openEmptyDeviceSession(name, mProvider, &mSession /*out*/, &meta /*out*/, &device /*out*/);

        camera_metadata_t* staticMeta = reinterpret_cast<camera_metadata_t*>(meta.metadata.data());
        outputStreams.clear();
        ASSERT_EQ(Status::OK, getAvailableOutputStreams(staticMeta, outputStreams));
        ASSERT_NE(0u, outputStreams.size());

        int32_t jpegBufferSize = 0;
        ASSERT_EQ(Status::OK, getJpegBufferSize(staticMeta, &jpegBufferSize));
        ASSERT_NE(0u, jpegBufferSize);

        int32_t streamId = 0;
        int32_t streamConfigCounter = 0;
        for (auto& it : outputStreams) {
            Stream stream;
            Dataspace dataspace = getDataspace(static_cast<PixelFormat>(it.format));
            stream.id = streamId;
            stream.streamType = StreamType::OUTPUT;
            stream.width = it.width;
            stream.height = it.height;
            stream.format = static_cast<PixelFormat>(it.format);
            stream.dataSpace = dataspace;
            stream.usage = static_cast<aidl::android::hardware::graphics::common::BufferUsage>(
                    GRALLOC1_CONSUMER_USAGE_HWCOMPOSER);
            stream.rotation = StreamRotation::ROTATION_0;
            stream.dynamicRangeProfile = RequestAvailableDynamicRangeProfilesMap::
                    ANDROID_REQUEST_AVAILABLE_DYNAMIC_RANGE_PROFILES_MAP_STANDARD;

            std::vector<Stream> streams = {stream};
            StreamConfiguration config;
            createStreamConfiguration(streams, StreamConfigurationMode::NORMAL_MODE, &config,
                                      jpegBufferSize);

            bool expectStreamCombQuery = (isLogicalMultiCamera(staticMeta) == Status::OK);
            verifyStreamCombination(device, config, /*expectedStatus*/ true, expectStreamCombQuery);

            config.streamConfigCounter = streamConfigCounter++;
            std::vector<HalStream> halConfigs;
            ndk::ScopedAStatus ret = mSession->configureStreams(config, &halConfigs);
            ASSERT_TRUE(ret.isOk());
            ASSERT_EQ(halConfigs.size(), 1);
            ASSERT_EQ(halConfigs[0].id, streamId);

            streamId++;
        }

        ndk::ScopedAStatus ret = mSession->close();
        mSession = nullptr;
        ASSERT_TRUE(ret.isOk());
    }
}

// Verify that mandatory concurrent streams and outputs are supported.
TEST_P(CameraAidlTest, configureConcurrentStreamsAvailableOutputs) {
    struct CameraTestInfo {
        CameraMetadata staticMeta;
        std::shared_ptr<ICameraDeviceSession> session;
        std::shared_ptr<ICameraDevice> cameraDevice;
        StreamConfiguration config;
    };

    std::map<std::string, std::string> idToNameMap = getCameraDeviceIdToNameMap(mProvider);
    std::vector<ConcurrentCameraIdCombination> concurrentDeviceCombinations =
            getConcurrentDeviceCombinations(mProvider);
    std::vector<AvailableStream> outputStreams;
    for (const auto& cameraDeviceIds : concurrentDeviceCombinations) {
        std::vector<CameraIdAndStreamCombination> cameraIdsAndStreamCombinations;
        std::vector<CameraTestInfo> cameraTestInfos;
        size_t i = 0;
        for (const auto& id : cameraDeviceIds.combination) {
            CameraTestInfo cti;
            auto it = idToNameMap.find(id);
            ASSERT_TRUE(idToNameMap.end() != it);
            std::string name = it->second;

            openEmptyDeviceSession(name, mProvider, &cti.session /*out*/, &cti.staticMeta /*out*/,
                                   &cti.cameraDevice /*out*/);

            outputStreams.clear();
            camera_metadata_t* staticMeta =
                    reinterpret_cast<camera_metadata_t*>(cti.staticMeta.metadata.data());
            ASSERT_EQ(Status::OK, getMandatoryConcurrentStreams(staticMeta, &outputStreams));
            ASSERT_NE(0u, outputStreams.size());

            int32_t jpegBufferSize = 0;
            ASSERT_EQ(Status::OK, getJpegBufferSize(staticMeta, &jpegBufferSize));
            ASSERT_NE(0u, jpegBufferSize);

            int32_t streamId = 0;
            std::vector<Stream> streams(outputStreams.size());
            size_t j = 0;
            for (const auto& s : outputStreams) {
                Stream stream;
                Dataspace dataspace = getDataspace(static_cast<PixelFormat>(s.format));
                stream.id = streamId++;
                stream.streamType = StreamType::OUTPUT;
                stream.width = s.width;
                stream.height = s.height;
                stream.format = static_cast<PixelFormat>(s.format);
                stream.usage = static_cast<aidl::android::hardware::graphics::common::BufferUsage>(
                        GRALLOC1_CONSUMER_USAGE_HWCOMPOSER);
                stream.dataSpace = dataspace;
                stream.rotation = StreamRotation::ROTATION_0;
                stream.sensorPixelModesUsed = {SensorPixelMode::ANDROID_SENSOR_PIXEL_MODE_DEFAULT};
                stream.dynamicRangeProfile = RequestAvailableDynamicRangeProfilesMap::
                        ANDROID_REQUEST_AVAILABLE_DYNAMIC_RANGE_PROFILES_MAP_STANDARD;
                streams[j] = stream;
                j++;
            }

            // Add the created stream configs to cameraIdsAndStreamCombinations
            createStreamConfiguration(streams, StreamConfigurationMode::NORMAL_MODE, &cti.config,
                                      jpegBufferSize);

            cti.config.streamConfigCounter = outputStreams.size();
            CameraIdAndStreamCombination cameraIdAndStreamCombination;
            cameraIdAndStreamCombination.cameraId = id;
            cameraIdAndStreamCombination.streamConfiguration = cti.config;
            cameraIdsAndStreamCombinations.push_back(cameraIdAndStreamCombination);
            i++;
            cameraTestInfos.push_back(cti);
        }
        // Now verify that concurrent streams are supported
        bool combinationSupported;
        ndk::ScopedAStatus ret = mProvider->isConcurrentStreamCombinationSupported(
                cameraIdsAndStreamCombinations, &combinationSupported);
        ASSERT_TRUE(ret.isOk());
        ASSERT_EQ(combinationSupported, true);

        // Test the stream can actually be configured
        for (auto& cti : cameraTestInfos) {
            if (cti.session != nullptr) {
                camera_metadata_t* staticMeta =
                        reinterpret_cast<camera_metadata_t*>(cti.staticMeta.metadata.data());
                bool expectStreamCombQuery = (isLogicalMultiCamera(staticMeta) == Status::OK);
                verifyStreamCombination(cti.cameraDevice, cti.config, /*expectedStatus*/ true,
                                        expectStreamCombQuery);
            }

            if (cti.session != nullptr) {
                std::vector<HalStream> streamConfigs;
                ret = cti.session->configureStreams(cti.config, &streamConfigs);
                ASSERT_TRUE(ret.isOk());
                ASSERT_EQ(cti.config.streams.size(), streamConfigs.size());
            }
        }

        for (auto& cti : cameraTestInfos) {
            ret = cti.session->close();
            ASSERT_TRUE(ret.isOk());
        }
    }
}

// Check for correct handling of invalid/incorrect configuration parameters.
TEST_P(CameraAidlTest, configureStreamsInvalidOutputs) {
    std::vector<std::string> cameraDeviceNames = getCameraDeviceNames(mProvider);
    std::vector<AvailableStream> outputStreams;

    for (const auto& name : cameraDeviceNames) {
        CameraMetadata meta;
        std::shared_ptr<ICameraDevice> cameraDevice;

        openEmptyDeviceSession(name, mProvider, &mSession /*out*/, &meta /*out*/,
                               &cameraDevice /*out*/);
        camera_metadata_t* staticMeta = reinterpret_cast<camera_metadata_t*>(meta.metadata.data());
        outputStreams.clear();

        ASSERT_EQ(Status::OK, getAvailableOutputStreams(staticMeta, outputStreams));
        ASSERT_NE(0u, outputStreams.size());

        int32_t jpegBufferSize = 0;
        ASSERT_EQ(Status::OK, getJpegBufferSize(staticMeta, &jpegBufferSize));
        ASSERT_NE(0u, jpegBufferSize);

        int32_t streamId = 0;
        Stream stream = {streamId++,
                         StreamType::OUTPUT,
                         static_cast<uint32_t>(0),
                         static_cast<uint32_t>(0),
                         static_cast<PixelFormat>(outputStreams[0].format),
                         static_cast<aidl::android::hardware::graphics::common::BufferUsage>(
                                 GRALLOC1_CONSUMER_USAGE_HWCOMPOSER),
                         Dataspace::UNKNOWN,
                         StreamRotation::ROTATION_0,
                         std::string(),
                         jpegBufferSize,
                         -1,
                         {SensorPixelMode::ANDROID_SENSOR_PIXEL_MODE_DEFAULT},
                         RequestAvailableDynamicRangeProfilesMap::
                                 ANDROID_REQUEST_AVAILABLE_DYNAMIC_RANGE_PROFILES_MAP_STANDARD};
        int32_t streamConfigCounter = 0;
        std::vector<Stream> streams = {stream};
        StreamConfiguration config;
        createStreamConfiguration(streams, StreamConfigurationMode::NORMAL_MODE, &config,
                                  jpegBufferSize);

        verifyStreamCombination(cameraDevice, config, /*expectedStatus*/ false,
                                /*expectStreamCombQuery*/ false);

        config.streamConfigCounter = streamConfigCounter++;
        std::vector<HalStream> halConfigs;
        ndk::ScopedAStatus ret = mSession->configureStreams(config, &halConfigs);
        ASSERT_TRUE(static_cast<int32_t>(Status::ILLEGAL_ARGUMENT) ==
                            ret.getServiceSpecificError() ||
                    static_cast<int32_t>(Status::INTERNAL_ERROR) == ret.getServiceSpecificError());

        stream = {streamId++,
                  StreamType::OUTPUT,
                  /*width*/ INT32_MAX,
                  /*height*/ INT32_MAX,
                  static_cast<PixelFormat>(outputStreams[0].format),
                  static_cast<aidl::android::hardware::graphics::common::BufferUsage>(
                          GRALLOC1_CONSUMER_USAGE_HWCOMPOSER),
                  Dataspace::UNKNOWN,
                  StreamRotation::ROTATION_0,
                  std::string(),
                  jpegBufferSize,
                  -1,
                  {SensorPixelMode::ANDROID_SENSOR_PIXEL_MODE_DEFAULT},
                  RequestAvailableDynamicRangeProfilesMap::
                          ANDROID_REQUEST_AVAILABLE_DYNAMIC_RANGE_PROFILES_MAP_STANDARD};

        streams[0] = stream;
        createStreamConfiguration(streams, StreamConfigurationMode::NORMAL_MODE, &config,
                                  jpegBufferSize);

        config.streamConfigCounter = streamConfigCounter++;
        halConfigs.clear();
        ret = mSession->configureStreams(config, &halConfigs);
        ASSERT_EQ(static_cast<int32_t>(Status::ILLEGAL_ARGUMENT), ret.getServiceSpecificError());

        for (auto& it : outputStreams) {
            stream = {streamId++,
                      StreamType::OUTPUT,
                      it.width,
                      it.height,
                      static_cast<PixelFormat>(UINT32_MAX),
                      static_cast<aidl::android::hardware::graphics::common::BufferUsage>(
                              GRALLOC1_CONSUMER_USAGE_HWCOMPOSER),
                      Dataspace::UNKNOWN,
                      StreamRotation::ROTATION_0,
                      std::string(),
                      jpegBufferSize,
                      -1,
                      {SensorPixelMode::ANDROID_SENSOR_PIXEL_MODE_DEFAULT},
                      RequestAvailableDynamicRangeProfilesMap::
                              ANDROID_REQUEST_AVAILABLE_DYNAMIC_RANGE_PROFILES_MAP_STANDARD};

            streams[0] = stream;
            createStreamConfiguration(streams, StreamConfigurationMode::NORMAL_MODE, &config,
                                      jpegBufferSize);
            config.streamConfigCounter = streamConfigCounter++;
            halConfigs.clear();
            ret = mSession->configureStreams(config, &halConfigs);
            ASSERT_EQ(static_cast<int32_t>(Status::ILLEGAL_ARGUMENT),
                      ret.getServiceSpecificError());

            stream = {streamId++,
                      StreamType::OUTPUT,
                      it.width,
                      it.height,
                      static_cast<PixelFormat>(it.format),
                      static_cast<aidl::android::hardware::graphics::common::BufferUsage>(
                              GRALLOC1_CONSUMER_USAGE_HWCOMPOSER),
                      Dataspace::UNKNOWN,
                      static_cast<StreamRotation>(UINT32_MAX),
                      std::string(),
                      jpegBufferSize,
                      -1,
                      {SensorPixelMode::ANDROID_SENSOR_PIXEL_MODE_DEFAULT},
                      RequestAvailableDynamicRangeProfilesMap::
                              ANDROID_REQUEST_AVAILABLE_DYNAMIC_RANGE_PROFILES_MAP_STANDARD};

            streams[0] = stream;
            createStreamConfiguration(streams, StreamConfigurationMode::NORMAL_MODE, &config,
                                      jpegBufferSize);

            config.streamConfigCounter = streamConfigCounter++;
            halConfigs.clear();
            ret = mSession->configureStreams(config, &halConfigs);
            ASSERT_EQ(static_cast<int32_t>(Status::ILLEGAL_ARGUMENT),
                      ret.getServiceSpecificError());
        }

        ret = mSession->close();
        mSession = nullptr;
        ASSERT_TRUE(ret.isOk());
    }
}

// Check whether all supported ZSL output stream combinations can be
// configured successfully.
TEST_P(CameraAidlTest, configureStreamsZSLInputOutputs) {
    std::vector<std::string> cameraDeviceNames = getCameraDeviceNames(mProvider);
    std::vector<AvailableStream> inputStreams;
    std::vector<AvailableZSLInputOutput> inputOutputMap;

    for (const auto& name : cameraDeviceNames) {
        CameraMetadata meta;
        std::shared_ptr<ICameraDevice> cameraDevice;

        openEmptyDeviceSession(name, mProvider, &mSession /*out*/, &meta /*out*/,
                               &cameraDevice /*out*/);
        camera_metadata_t* staticMeta = reinterpret_cast<camera_metadata_t*>(meta.metadata.data());

        Status rc = isZSLModeAvailable(staticMeta);
        if (Status::OPERATION_NOT_SUPPORTED == rc) {
            ndk::ScopedAStatus ret = mSession->close();
            mSession = nullptr;
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

        int32_t jpegBufferSize = 0;
        ASSERT_EQ(Status::OK, getJpegBufferSize(staticMeta, &jpegBufferSize));
        ASSERT_NE(0u, jpegBufferSize);

        int32_t streamId = 0;
        bool hasPrivToY8 = false, hasY8ToY8 = false, hasY8ToBlob = false;
        uint32_t streamConfigCounter = 0;
        for (auto& inputIter : inputOutputMap) {
            AvailableStream input;
            ASSERT_EQ(Status::OK, findLargestSize(inputStreams, inputIter.inputFormat, input));
            ASSERT_NE(0u, inputStreams.size());

            if (inputIter.inputFormat ==
                        static_cast<uint32_t>(PixelFormat::IMPLEMENTATION_DEFINED) &&
                inputIter.outputFormat == static_cast<uint32_t>(PixelFormat::Y8)) {
                hasPrivToY8 = true;
            } else if (inputIter.inputFormat == static_cast<uint32_t>(PixelFormat::Y8)) {
                if (inputIter.outputFormat == static_cast<uint32_t>(PixelFormat::BLOB)) {
                    hasY8ToBlob = true;
                } else if (inputIter.outputFormat == static_cast<uint32_t>(PixelFormat::Y8)) {
                    hasY8ToY8 = true;
                }
            }
            AvailableStream outputThreshold = {INT32_MAX, INT32_MAX, inputIter.outputFormat};
            std::vector<AvailableStream> outputStreams;
            ASSERT_EQ(Status::OK,
                      getAvailableOutputStreams(staticMeta, outputStreams, &outputThreshold));
            for (auto& outputIter : outputStreams) {
                Dataspace outputDataSpace =
                        getDataspace(static_cast<PixelFormat>(outputIter.format));
                Stream zslStream = {
                        streamId++,
                        StreamType::OUTPUT,
                        input.width,
                        input.height,
                        static_cast<PixelFormat>(input.format),
                        static_cast<aidl::android::hardware::graphics::common::BufferUsage>(
                                GRALLOC_USAGE_HW_CAMERA_ZSL),
                        Dataspace::UNKNOWN,
                        StreamRotation::ROTATION_0,
                        std::string(),
                        jpegBufferSize,
                        -1,
                        {SensorPixelMode::ANDROID_SENSOR_PIXEL_MODE_DEFAULT},
                        RequestAvailableDynamicRangeProfilesMap::
                                ANDROID_REQUEST_AVAILABLE_DYNAMIC_RANGE_PROFILES_MAP_STANDARD};
                Stream inputStream = {
                        streamId++,
                        StreamType::INPUT,
                        input.width,
                        input.height,
                        static_cast<PixelFormat>(input.format),
                        static_cast<aidl::android::hardware::graphics::common::BufferUsage>(0),
                        Dataspace::UNKNOWN,
                        StreamRotation::ROTATION_0,
                        std::string(),
                        jpegBufferSize,
                        -1,
                        {SensorPixelMode::ANDROID_SENSOR_PIXEL_MODE_DEFAULT},
                        RequestAvailableDynamicRangeProfilesMap::
                                ANDROID_REQUEST_AVAILABLE_DYNAMIC_RANGE_PROFILES_MAP_STANDARD};
                Stream outputStream = {
                        streamId++,
                        StreamType::OUTPUT,
                        outputIter.width,
                        outputIter.height,
                        static_cast<PixelFormat>(outputIter.format),
                        static_cast<aidl::android::hardware::graphics::common::BufferUsage>(
                                GRALLOC1_CONSUMER_USAGE_HWCOMPOSER),
                        outputDataSpace,
                        StreamRotation::ROTATION_0,
                        std::string(),
                        jpegBufferSize,
                        -1,
                        {SensorPixelMode::ANDROID_SENSOR_PIXEL_MODE_DEFAULT},
                        RequestAvailableDynamicRangeProfilesMap::
                                ANDROID_REQUEST_AVAILABLE_DYNAMIC_RANGE_PROFILES_MAP_STANDARD};

                std::vector<Stream> streams = {inputStream, zslStream, outputStream};

                StreamConfiguration config;
                createStreamConfiguration(streams, StreamConfigurationMode::NORMAL_MODE, &config,
                                          jpegBufferSize);

                verifyStreamCombination(cameraDevice, config, /*expectedStatus*/ true,
                                        /*expectStreamCombQuery*/ false);

                config.streamConfigCounter = streamConfigCounter++;
                std::vector<HalStream> halConfigs;
                ndk::ScopedAStatus ret = mSession->configureStreams(config, &halConfigs);
                ASSERT_TRUE(ret.isOk());
                ASSERT_EQ(3u, halConfigs.size());
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

        ndk::ScopedAStatus ret = mSession->close();
        mSession = nullptr;
        ASSERT_TRUE(ret.isOk());
    }
}

// Check whether session parameters are supported. If Hal support for them
// exist, then try to configure a preview stream using them.
TEST_P(CameraAidlTest, configureStreamsWithSessionParameters) {
    std::vector<std::string> cameraDeviceNames = getCameraDeviceNames(mProvider);
    std::vector<AvailableStream> outputPreviewStreams;
    AvailableStream previewThreshold = {kMaxPreviewWidth, kMaxPreviewHeight,
                                        static_cast<int32_t>(PixelFormat::IMPLEMENTATION_DEFINED)};

    for (const auto& name : cameraDeviceNames) {
        CameraMetadata meta;

        std::shared_ptr<ICameraDevice> unusedCameraDevice;
        openEmptyDeviceSession(name, mProvider, &mSession /*out*/, &meta /*out*/,
                               &unusedCameraDevice /*out*/);
        camera_metadata_t* staticMetaBuffer =
                reinterpret_cast<camera_metadata_t*>(meta.metadata.data());

        std::unordered_set<int32_t> availableSessionKeys;
        auto rc = getSupportedKeys(staticMetaBuffer, ANDROID_REQUEST_AVAILABLE_SESSION_KEYS,
                                   &availableSessionKeys);
        ASSERT_TRUE(Status::OK == rc);
        if (availableSessionKeys.empty()) {
            ndk::ScopedAStatus ret = mSession->close();
            mSession = nullptr;
            ASSERT_TRUE(ret.isOk());
            continue;
        }

        android::hardware::camera::common::V1_0::helper::CameraMetadata previewRequestSettings;
        android::hardware::camera::common::V1_0::helper::CameraMetadata sessionParams,
                modifiedSessionParams;
        constructFilteredSettings(mSession, availableSessionKeys, RequestTemplate::PREVIEW,
                                  &previewRequestSettings, &sessionParams);
        if (sessionParams.isEmpty()) {
            ndk::ScopedAStatus ret = mSession->close();
            mSession = nullptr;
            ASSERT_TRUE(ret.isOk());
            continue;
        }

        outputPreviewStreams.clear();

        ASSERT_EQ(Status::OK, getAvailableOutputStreams(staticMetaBuffer, outputPreviewStreams,
                                                        &previewThreshold));
        ASSERT_NE(0u, outputPreviewStreams.size());

        Stream previewStream = {
                0,
                StreamType::OUTPUT,
                outputPreviewStreams[0].width,
                outputPreviewStreams[0].height,
                static_cast<PixelFormat>(outputPreviewStreams[0].format),
                static_cast<aidl::android::hardware::graphics::common::BufferUsage>(
                        GRALLOC1_CONSUMER_USAGE_HWCOMPOSER),
                Dataspace::UNKNOWN,
                StreamRotation::ROTATION_0,
                std::string(),
                /*bufferSize*/ 0,
                /*groupId*/ -1,
                {SensorPixelMode::ANDROID_SENSOR_PIXEL_MODE_DEFAULT},
                RequestAvailableDynamicRangeProfilesMap::
                        ANDROID_REQUEST_AVAILABLE_DYNAMIC_RANGE_PROFILES_MAP_STANDARD};

        std::vector<Stream> streams = {previewStream};
        StreamConfiguration config;

        config.streams = streams;
        config.operationMode = StreamConfigurationMode::NORMAL_MODE;
        modifiedSessionParams = sessionParams;
        auto sessionParamsBuffer = sessionParams.release();
        std::vector<uint8_t> rawSessionParam =
                std::vector(reinterpret_cast<uint8_t*>(sessionParamsBuffer),
                            reinterpret_cast<uint8_t*>(sessionParamsBuffer) +
                                    get_camera_metadata_size(sessionParamsBuffer));

        config.sessionParams.metadata = rawSessionParam;
        config.streamConfigCounter = 0;
        config.streams = {previewStream};
        config.streamConfigCounter = 0;
        config.multiResolutionInputImage = false;

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
            verifySessionReconfigurationQuery(mSession, sessionParamsBuffer,
                                              modifiedSessionParamsBuffer);
            modifiedSessionParams.acquire(modifiedSessionParamsBuffer);
        }

        std::vector<HalStream> halConfigs;
        ndk::ScopedAStatus ret = mSession->configureStreams(config, &halConfigs);
        ASSERT_TRUE(ret.isOk());
        ASSERT_EQ(1u, halConfigs.size());

        sessionParams.acquire(sessionParamsBuffer);
        ret = mSession->close();
        mSession = nullptr;
        ASSERT_TRUE(ret.isOk());
    }
}

// Verify that all supported preview + still capture stream combinations
// can be configured successfully.
TEST_P(CameraAidlTest, configureStreamsPreviewStillOutputs) {
    std::vector<std::string> cameraDeviceNames = getCameraDeviceNames(mProvider);
    std::vector<AvailableStream> outputBlobStreams;
    std::vector<AvailableStream> outputPreviewStreams;
    AvailableStream previewThreshold = {kMaxPreviewWidth, kMaxPreviewHeight,
                                        static_cast<int32_t>(PixelFormat::IMPLEMENTATION_DEFINED)};
    AvailableStream blobThreshold = {INT32_MAX, INT32_MAX, static_cast<int32_t>(PixelFormat::BLOB)};

    for (const auto& name : cameraDeviceNames) {
        CameraMetadata meta;

        std::shared_ptr<ICameraDevice> cameraDevice;
        openEmptyDeviceSession(name, mProvider, &mSession /*out*/, &meta /*out*/,
                               &cameraDevice /*out*/);

        camera_metadata_t* staticMeta = reinterpret_cast<camera_metadata_t*>(meta.metadata.data());

        // Check if camera support depth only
        if (isDepthOnly(staticMeta)) {
            ndk::ScopedAStatus ret = mSession->close();
            mSession = nullptr;
            ASSERT_TRUE(ret.isOk());
            continue;
        }

        outputBlobStreams.clear();
        ASSERT_EQ(Status::OK,
                  getAvailableOutputStreams(staticMeta, outputBlobStreams, &blobThreshold));
        ASSERT_NE(0u, outputBlobStreams.size());

        outputPreviewStreams.clear();
        ASSERT_EQ(Status::OK,
                  getAvailableOutputStreams(staticMeta, outputPreviewStreams, &previewThreshold));
        ASSERT_NE(0u, outputPreviewStreams.size());

        int32_t jpegBufferSize = 0;
        ASSERT_EQ(Status::OK, getJpegBufferSize(staticMeta, &jpegBufferSize));
        ASSERT_NE(0u, jpegBufferSize);

        int32_t streamId = 0;
        uint32_t streamConfigCounter = 0;

        for (auto& blobIter : outputBlobStreams) {
            for (auto& previewIter : outputPreviewStreams) {
                Stream previewStream = {
                        streamId++,
                        StreamType::OUTPUT,
                        previewIter.width,
                        previewIter.height,
                        static_cast<PixelFormat>(previewIter.format),
                        static_cast<aidl::android::hardware::graphics::common::BufferUsage>(
                                GRALLOC1_CONSUMER_USAGE_HWCOMPOSER),
                        Dataspace::UNKNOWN,
                        StreamRotation::ROTATION_0,
                        std::string(),
                        /*bufferSize*/ 0,
                        /*groupId*/ -1,
                        {SensorPixelMode::ANDROID_SENSOR_PIXEL_MODE_DEFAULT},
                        RequestAvailableDynamicRangeProfilesMap::
                                ANDROID_REQUEST_AVAILABLE_DYNAMIC_RANGE_PROFILES_MAP_STANDARD};
                Stream blobStream = {
                        streamId++,
                        StreamType::OUTPUT,
                        blobIter.width,
                        blobIter.height,
                        static_cast<PixelFormat>(blobIter.format),
                        static_cast<aidl::android::hardware::graphics::common::BufferUsage>(
                                GRALLOC1_CONSUMER_USAGE_CPU_READ),
                        Dataspace::JFIF,
                        StreamRotation::ROTATION_0,
                        std::string(),
                        /*bufferSize*/ 0,
                        /*groupId*/ -1,
                        {SensorPixelMode::ANDROID_SENSOR_PIXEL_MODE_DEFAULT},
                        RequestAvailableDynamicRangeProfilesMap::
                                ANDROID_REQUEST_AVAILABLE_DYNAMIC_RANGE_PROFILES_MAP_STANDARD};
                std::vector<Stream> streams = {previewStream, blobStream};
                StreamConfiguration config;

                createStreamConfiguration(streams, StreamConfigurationMode::NORMAL_MODE, &config,
                                          jpegBufferSize);
                config.streamConfigCounter = streamConfigCounter++;
                verifyStreamCombination(cameraDevice, config, /*expectedStatus*/ true,
                                        /*expectStreamCombQuery*/ false);

                std::vector<HalStream> halConfigs;
                ndk::ScopedAStatus ret = mSession->configureStreams(config, &halConfigs);
                ASSERT_TRUE(ret.isOk());
                ASSERT_EQ(2u, halConfigs.size());
            }
        }

        ndk::ScopedAStatus ret = mSession->close();
        mSession = nullptr;
        ASSERT_TRUE(ret.isOk());
    }
}

// In case constrained mode is supported, test whether it can be
// configured. Additionally check for common invalid inputs when
// using this mode.
TEST_P(CameraAidlTest, configureStreamsConstrainedOutputs) {
    std::vector<std::string> cameraDeviceNames = getCameraDeviceNames(mProvider);

    for (const auto& name : cameraDeviceNames) {
        CameraMetadata meta;
        std::shared_ptr<ICameraDevice> cameraDevice;

        openEmptyDeviceSession(name, mProvider, &mSession /*out*/, &meta /*out*/,
                               &cameraDevice /*out*/);
        camera_metadata_t* staticMeta = reinterpret_cast<camera_metadata_t*>(meta.metadata.data());

        Status rc = isConstrainedModeAvailable(staticMeta);
        if (Status::OPERATION_NOT_SUPPORTED == rc) {
            ndk::ScopedAStatus ret = mSession->close();
            mSession = nullptr;
            ASSERT_TRUE(ret.isOk());
            continue;
        }
        ASSERT_EQ(Status::OK, rc);

        AvailableStream hfrStream;
        rc = pickConstrainedModeSize(staticMeta, hfrStream);
        ASSERT_EQ(Status::OK, rc);

        int32_t streamId = 0;
        uint32_t streamConfigCounter = 0;
        Stream stream = {streamId,
                         StreamType::OUTPUT,
                         hfrStream.width,
                         hfrStream.height,
                         static_cast<PixelFormat>(hfrStream.format),
                         static_cast<aidl::android::hardware::graphics::common::BufferUsage>(
                                 GRALLOC1_CONSUMER_USAGE_VIDEO_ENCODER),
                         Dataspace::UNKNOWN,
                         StreamRotation::ROTATION_0,
                         std::string(),
                         /*bufferSize*/ 0,
                         /*groupId*/ -1,
                         {SensorPixelMode::ANDROID_SENSOR_PIXEL_MODE_DEFAULT},
                         RequestAvailableDynamicRangeProfilesMap::
                                 ANDROID_REQUEST_AVAILABLE_DYNAMIC_RANGE_PROFILES_MAP_STANDARD};
        std::vector<Stream> streams = {stream};
        StreamConfiguration config;
        createStreamConfiguration(streams, StreamConfigurationMode::CONSTRAINED_HIGH_SPEED_MODE,
                                  &config);

        verifyStreamCombination(cameraDevice, config, /*expectedStatus*/ true,
                                /*expectStreamCombQuery*/ false);

        config.streamConfigCounter = streamConfigCounter++;
        std::vector<HalStream> halConfigs;
        ndk::ScopedAStatus ret = mSession->configureStreams(config, &halConfigs);
        ASSERT_TRUE(ret.isOk());
        ASSERT_EQ(1u, halConfigs.size());
        ASSERT_EQ(halConfigs[0].id, streamId);

        stream = {streamId++,
                  StreamType::OUTPUT,
                  static_cast<uint32_t>(0),
                  static_cast<uint32_t>(0),
                  static_cast<PixelFormat>(hfrStream.format),
                  static_cast<aidl::android::hardware::graphics::common::BufferUsage>(
                          GRALLOC1_CONSUMER_USAGE_VIDEO_ENCODER),
                  Dataspace::UNKNOWN,
                  StreamRotation::ROTATION_0,
                  std::string(),
                  /*bufferSize*/ 0,
                  /*groupId*/ -1,
                  {SensorPixelMode::ANDROID_SENSOR_PIXEL_MODE_DEFAULT},
                  RequestAvailableDynamicRangeProfilesMap::
                          ANDROID_REQUEST_AVAILABLE_DYNAMIC_RANGE_PROFILES_MAP_STANDARD};
        streams[0] = stream;
        createStreamConfiguration(streams, StreamConfigurationMode::CONSTRAINED_HIGH_SPEED_MODE,
                                  &config);

        config.streamConfigCounter = streamConfigCounter++;
        std::vector<HalStream> halConfig;
        ret = mSession->configureStreams(config, &halConfig);
        ASSERT_TRUE(static_cast<int32_t>(Status::ILLEGAL_ARGUMENT) ==
                            ret.getServiceSpecificError() ||
                    static_cast<int32_t>(Status::INTERNAL_ERROR) == ret.getServiceSpecificError());

        stream = {streamId++,
                  StreamType::OUTPUT,
                  INT32_MAX,
                  INT32_MAX,
                  static_cast<PixelFormat>(hfrStream.format),
                  static_cast<aidl::android::hardware::graphics::common::BufferUsage>(
                          GRALLOC1_CONSUMER_USAGE_VIDEO_ENCODER),
                  Dataspace::UNKNOWN,
                  StreamRotation::ROTATION_0,
                  std::string(),
                  /*bufferSize*/ 0,
                  /*groupId*/ -1,
                  {SensorPixelMode::ANDROID_SENSOR_PIXEL_MODE_DEFAULT},
                  RequestAvailableDynamicRangeProfilesMap::
                          ANDROID_REQUEST_AVAILABLE_DYNAMIC_RANGE_PROFILES_MAP_STANDARD};
        streams[0] = stream;
        createStreamConfiguration(streams, StreamConfigurationMode::CONSTRAINED_HIGH_SPEED_MODE,
                                  &config);

        config.streamConfigCounter = streamConfigCounter++;
        halConfigs.clear();
        ret = mSession->configureStreams(config, &halConfigs);
        ASSERT_EQ(static_cast<int32_t>(Status::ILLEGAL_ARGUMENT), ret.getServiceSpecificError());

        stream = {streamId++,
                  StreamType::OUTPUT,
                  hfrStream.width,
                  hfrStream.height,
                  static_cast<PixelFormat>(UINT32_MAX),
                  static_cast<aidl::android::hardware::graphics::common::BufferUsage>(
                          GRALLOC1_CONSUMER_USAGE_VIDEO_ENCODER),
                  Dataspace::UNKNOWN,
                  StreamRotation::ROTATION_0,
                  std::string(),
                  /*bufferSize*/ 0,
                  /*groupId*/ -1,
                  {SensorPixelMode::ANDROID_SENSOR_PIXEL_MODE_DEFAULT},
                  RequestAvailableDynamicRangeProfilesMap::
                          ANDROID_REQUEST_AVAILABLE_DYNAMIC_RANGE_PROFILES_MAP_STANDARD};
        streams[0] = stream;
        createStreamConfiguration(streams, StreamConfigurationMode::CONSTRAINED_HIGH_SPEED_MODE,
                                  &config);

        config.streamConfigCounter = streamConfigCounter++;
        halConfigs.clear();
        ret = mSession->configureStreams(config, &halConfigs);
        ASSERT_EQ(static_cast<int32_t>(Status::ILLEGAL_ARGUMENT), ret.getServiceSpecificError());

        ret = mSession->close();
        mSession = nullptr;
        ASSERT_TRUE(ret.isOk());
    }
}

// Verify that all supported video + snapshot stream combinations can
// be configured successfully.
TEST_P(CameraAidlTest, configureStreamsVideoStillOutputs) {
    std::vector<std::string> cameraDeviceNames = getCameraDeviceNames(mProvider);
    std::vector<AvailableStream> outputBlobStreams;
    std::vector<AvailableStream> outputVideoStreams;
    AvailableStream videoThreshold = {kMaxVideoWidth, kMaxVideoHeight,
                                      static_cast<int32_t>(PixelFormat::IMPLEMENTATION_DEFINED)};
    AvailableStream blobThreshold = {kMaxVideoWidth, kMaxVideoHeight,
                                     static_cast<int32_t>(PixelFormat::BLOB)};

    for (const auto& name : cameraDeviceNames) {
        CameraMetadata meta;
        std::shared_ptr<ICameraDevice> cameraDevice;

        openEmptyDeviceSession(name, mProvider, &mSession /*out*/, &meta /*out*/,
                               &cameraDevice /*out*/);

        camera_metadata_t* staticMeta = reinterpret_cast<camera_metadata_t*>(meta.metadata.data());

        // Check if camera support depth only
        if (isDepthOnly(staticMeta)) {
            ndk::ScopedAStatus ret = mSession->close();
            mSession = nullptr;
            ASSERT_TRUE(ret.isOk());
            continue;
        }

        outputBlobStreams.clear();
        ASSERT_EQ(Status::OK,
                  getAvailableOutputStreams(staticMeta, outputBlobStreams, &blobThreshold));
        ASSERT_NE(0u, outputBlobStreams.size());

        outputVideoStreams.clear();
        ASSERT_EQ(Status::OK,
                  getAvailableOutputStreams(staticMeta, outputVideoStreams, &videoThreshold));
        ASSERT_NE(0u, outputVideoStreams.size());

        int32_t jpegBufferSize = 0;
        ASSERT_EQ(Status::OK, getJpegBufferSize(staticMeta, &jpegBufferSize));
        ASSERT_NE(0u, jpegBufferSize);

        int32_t streamId = 0;
        uint32_t streamConfigCounter = 0;
        for (auto& blobIter : outputBlobStreams) {
            for (auto& videoIter : outputVideoStreams) {
                Stream videoStream = {
                        streamId++,
                        StreamType::OUTPUT,
                        videoIter.width,
                        videoIter.height,
                        static_cast<PixelFormat>(videoIter.format),
                        static_cast<aidl::android::hardware::graphics::common::BufferUsage>(
                                GRALLOC1_CONSUMER_USAGE_VIDEO_ENCODER),
                        Dataspace::UNKNOWN,
                        StreamRotation::ROTATION_0,
                        std::string(),
                        jpegBufferSize,
                        /*groupId*/ -1,
                        {SensorPixelMode::ANDROID_SENSOR_PIXEL_MODE_DEFAULT},
                        RequestAvailableDynamicRangeProfilesMap::
                                ANDROID_REQUEST_AVAILABLE_DYNAMIC_RANGE_PROFILES_MAP_STANDARD};
                Stream blobStream = {
                        streamId++,
                        StreamType::OUTPUT,
                        blobIter.width,
                        blobIter.height,
                        static_cast<PixelFormat>(blobIter.format),
                        static_cast<aidl::android::hardware::graphics::common::BufferUsage>(
                                GRALLOC1_CONSUMER_USAGE_CPU_READ),
                        Dataspace::JFIF,
                        StreamRotation::ROTATION_0,
                        std::string(),
                        jpegBufferSize,
                        /*groupId*/ -1,
                        {SensorPixelMode::ANDROID_SENSOR_PIXEL_MODE_DEFAULT},
                        RequestAvailableDynamicRangeProfilesMap::
                                ANDROID_REQUEST_AVAILABLE_DYNAMIC_RANGE_PROFILES_MAP_STANDARD};
                std::vector<Stream> streams = {videoStream, blobStream};
                StreamConfiguration config;

                createStreamConfiguration(streams, StreamConfigurationMode::NORMAL_MODE, &config,
                                          jpegBufferSize);
                verifyStreamCombination(cameraDevice, config, /*expectedStatus*/ true,
                                        /*expectStreamCombQuery*/ false);

                config.streamConfigCounter = streamConfigCounter++;
                std::vector<HalStream> halConfigs;
                ndk::ScopedAStatus ret = mSession->configureStreams(config, &halConfigs);
                ASSERT_TRUE(ret.isOk());
                ASSERT_EQ(2u, halConfigs.size());
            }
        }

        ndk::ScopedAStatus ret = mSession->close();
        mSession = nullptr;
        ASSERT_TRUE(ret.isOk());
    }
}

// Generate and verify a camera capture request
TEST_P(CameraAidlTest, processCaptureRequestPreview) {
    // TODO(b/220897574): Failing with BUFFER_ERROR
    processCaptureRequestInternal(GRALLOC1_CONSUMER_USAGE_HWCOMPOSER, RequestTemplate::PREVIEW,
                                  false /*secureOnlyCameras*/);
}

// Generate and verify a secure camera capture request
TEST_P(CameraAidlTest, processSecureCaptureRequest) {
    processCaptureRequestInternal(GRALLOC1_PRODUCER_USAGE_PROTECTED, RequestTemplate::STILL_CAPTURE,
                                  true /*secureOnlyCameras*/);
}

TEST_P(CameraAidlTest, processCaptureRequestPreviewStabilization) {
    std::unordered_map<std::string, nsecs_t> cameraDeviceToTimeLag;
    processPreviewStabilizationCaptureRequestInternal(/*previewStabilizationOn*/ false,
                                                      cameraDeviceToTimeLag);
    processPreviewStabilizationCaptureRequestInternal(/*previewStabilizationOn*/ true,
                                                      cameraDeviceToTimeLag);
}

// Generate and verify a multi-camera capture request
TEST_P(CameraAidlTest, processMultiCaptureRequestPreview) {
    std::vector<std::string> cameraDeviceNames = getCameraDeviceNames(mProvider);
    AvailableStream previewThreshold = {kMaxPreviewWidth, kMaxPreviewHeight,
                                        static_cast<int32_t>(PixelFormat::YCBCR_420_888)};
    int64_t bufferId = 1;
    uint32_t frameNumber = 1;
    std::vector<uint8_t> settings;
    std::vector<uint8_t> emptySettings;
    std::string invalidPhysicalId = "-1";

    for (const auto& name : cameraDeviceNames) {
        std::string version, deviceId;
        ALOGI("processMultiCaptureRequestPreview: Test device %s", name.c_str());
        ASSERT_TRUE(matchDeviceName(name, mProviderType, &version, &deviceId));
        CameraMetadata metadata;

        std::shared_ptr<ICameraDevice> unusedDevice;
        openEmptyDeviceSession(name, mProvider, &mSession /*out*/, &metadata /*out*/,
                               &unusedDevice /*out*/);

        camera_metadata_t* staticMeta =
                reinterpret_cast<camera_metadata_t*>(metadata.metadata.data());
        Status rc = isLogicalMultiCamera(staticMeta);
        if (Status::OPERATION_NOT_SUPPORTED == rc) {
            ndk::ScopedAStatus ret = mSession->close();
            mSession = nullptr;
            ASSERT_TRUE(ret.isOk());
            continue;
        }
        ASSERT_EQ(Status::OK, rc);

        std::unordered_set<std::string> physicalIds;
        rc = getPhysicalCameraIds(staticMeta, &physicalIds);
        ASSERT_TRUE(Status::OK == rc);
        ASSERT_TRUE(physicalIds.size() > 1);

        std::unordered_set<int32_t> physicalRequestKeyIDs;
        rc = getSupportedKeys(staticMeta, ANDROID_REQUEST_AVAILABLE_PHYSICAL_CAMERA_REQUEST_KEYS,
                              &physicalRequestKeyIDs);
        ASSERT_TRUE(Status::OK == rc);
        if (physicalRequestKeyIDs.empty()) {
            ndk::ScopedAStatus ret = mSession->close();
            mSession = nullptr;
            ASSERT_TRUE(ret.isOk());
            // The logical camera doesn't support any individual physical requests.
            continue;
        }

        android::hardware::camera::common::V1_0::helper::CameraMetadata defaultPreviewSettings;
        android::hardware::camera::common::V1_0::helper::CameraMetadata filteredSettings;
        constructFilteredSettings(mSession, physicalRequestKeyIDs, RequestTemplate::PREVIEW,
                                  &defaultPreviewSettings, &filteredSettings);
        if (filteredSettings.isEmpty()) {
            // No physical device settings in default request.
            ndk::ScopedAStatus ret = mSession->close();
            mSession = nullptr;
            ASSERT_TRUE(ret.isOk());
            continue;
        }

        const camera_metadata_t* settingsBuffer = defaultPreviewSettings.getAndLock();
        uint8_t* rawSettingsBuffer = (uint8_t*)settingsBuffer;
        settings.assign(rawSettingsBuffer,
                        rawSettingsBuffer + get_camera_metadata_size(settingsBuffer));
        CameraMetadata settingsMetadata = {settings};
        overrideRotateAndCrop(&settingsMetadata);

        ndk::ScopedAStatus ret = mSession->close();
        mSession = nullptr;
        ASSERT_TRUE(ret.isOk());

        // Leave only 2 physical devices in the id set.
        auto it = physicalIds.begin();
        std::string physicalDeviceId = *it;
        it++;
        physicalIds.erase(++it, physicalIds.end());
        ASSERT_EQ(physicalIds.size(), 2u);

        std::vector<HalStream> halStreams;
        bool supportsPartialResults = false;
        bool useHalBufManager = false;
        int32_t partialResultCount = 0;
        Stream previewStream;
        std::shared_ptr<DeviceCb> cb;

        configurePreviewStreams(
                name, mProvider, &previewThreshold, physicalIds, &mSession, &previewStream,
                &halStreams /*out*/, &supportsPartialResults /*out*/, &partialResultCount /*out*/,
                &useHalBufManager /*out*/, &cb /*out*/, 0 /*streamConfigCounter*/, true);
        if (mSession == nullptr) {
            // stream combination not supported by HAL, skip test for device
            continue;
        }

        ::aidl::android::hardware::common::fmq::MQDescriptor<
                int8_t, aidl::android::hardware::common::fmq::SynchronizedReadWrite>
                descriptor;
        auto resultQueueRet = mSession->getCaptureResultMetadataQueue(&descriptor);
        ASSERT_TRUE(resultQueueRet.isOk());
        std::shared_ptr<ResultMetadataQueue> resultQueue =
                std::make_shared<ResultMetadataQueue>(descriptor);
        if (!resultQueue->isValid() || resultQueue->availableToWrite() <= 0) {
            ALOGE("%s: HAL returns empty result metadata fmq, not use it", __func__);
            resultQueue = nullptr;
            // Don't use the queue onwards.
        }

        std::shared_ptr<InFlightRequest> inflightReq = std::make_shared<InFlightRequest>(
                static_cast<ssize_t>(halStreams.size()), false, supportsPartialResults,
                partialResultCount, physicalIds, resultQueue);

        std::vector<CaptureRequest> requests(1);
        CaptureRequest& request = requests[0];
        request.frameNumber = frameNumber;
        request.fmqSettingsSize = 0;
        request.settings = settingsMetadata;

        std::vector<StreamBuffer>& outputBuffers = request.outputBuffers;

        std::vector<buffer_handle_t> graphicBuffers;
        graphicBuffers.reserve(halStreams.size());
        outputBuffers.resize(halStreams.size());
        size_t k = 0;
        for (const auto& halStream : halStreams) {
            buffer_handle_t buffer_handle;
            if (useHalBufManager) {
                outputBuffers[k] = {halStream.id,     /*bufferId*/ 0, NativeHandle(),
                                    BufferStatus::OK, NativeHandle(), NativeHandle()};
            } else {
                allocateGraphicBuffer(previewStream.width, previewStream.height,
                                      android_convertGralloc1To0Usage(
                                              static_cast<uint64_t>(halStream.producerUsage),
                                              static_cast<uint64_t>(halStream.consumerUsage)),
                                      halStream.overrideFormat, &buffer_handle);
                graphicBuffers.push_back(buffer_handle);
                outputBuffers[k] = {
                        halStream.id,     bufferId,       ::android::makeToAidl(buffer_handle),
                        BufferStatus::OK, NativeHandle(), NativeHandle()};
                bufferId++;
            }
            k++;
        }

        std::vector<PhysicalCameraSetting> camSettings(1);
        const camera_metadata_t* filteredSettingsBuffer = filteredSettings.getAndLock();
        uint8_t* rawFilteredSettingsBuffer = (uint8_t*)filteredSettingsBuffer;
        camSettings[0].settings = {std::vector(
                rawFilteredSettingsBuffer,
                rawFilteredSettingsBuffer + get_camera_metadata_size(filteredSettingsBuffer))};
        overrideRotateAndCrop(&camSettings[0].settings);
        camSettings[0].fmqSettingsSize = 0;
        camSettings[0].physicalCameraId = physicalDeviceId;

        request.inputBuffer = {
                -1, 0, NativeHandle(), BufferStatus::ERROR, NativeHandle(), NativeHandle()};
        request.physicalCameraSettings = camSettings;

        {
            std::unique_lock<std::mutex> l(mLock);
            mInflightMap.clear();
            mInflightMap[frameNumber] = inflightReq;
        }

        int32_t numRequestProcessed = 0;
        std::vector<BufferCache> cachesToRemove;
        ndk::ScopedAStatus returnStatus =
                mSession->processCaptureRequest(requests, cachesToRemove, &numRequestProcessed);
        ASSERT_TRUE(returnStatus.isOk());
        ASSERT_EQ(numRequestProcessed, 1u);

        {
            std::unique_lock<std::mutex> l(mLock);
            while (!inflightReq->errorCodeValid &&
                   ((0 < inflightReq->numBuffersLeft) || (!inflightReq->haveResultMetadata))) {
                auto timeout = std::chrono::system_clock::now() +
                               std::chrono::seconds(kStreamBufferTimeoutSec);
                ASSERT_NE(std::cv_status::timeout, mResultCondition.wait_until(l, timeout));
            }

            ASSERT_FALSE(inflightReq->errorCodeValid);
            ASSERT_NE(inflightReq->resultOutputBuffers.size(), 0u);

            request.frameNumber++;
            // Empty settings should be supported after the first call
            // for repeating requests.
            request.settings.metadata.clear();
            request.physicalCameraSettings[0].settings.metadata.clear();
            // The buffer has been registered to HAL by bufferId, so per
            // API contract we should send a null handle for this buffer
            request.outputBuffers[0].buffer = NativeHandle();
            mInflightMap.clear();
            inflightReq = std::make_shared<InFlightRequest>(
                    static_cast<ssize_t>(physicalIds.size()), false, supportsPartialResults,
                    partialResultCount, physicalIds, resultQueue);
            mInflightMap[request.frameNumber] = inflightReq;
        }

        returnStatus =
                mSession->processCaptureRequest(requests, cachesToRemove, &numRequestProcessed);
        ASSERT_TRUE(returnStatus.isOk());
        ASSERT_EQ(numRequestProcessed, 1u);

        {
            std::unique_lock<std::mutex> l(mLock);
            while (!inflightReq->errorCodeValid &&
                   ((0 < inflightReq->numBuffersLeft) || (!inflightReq->haveResultMetadata))) {
                auto timeout = std::chrono::system_clock::now() +
                               std::chrono::seconds(kStreamBufferTimeoutSec);
                ASSERT_NE(std::cv_status::timeout, mResultCondition.wait_until(l, timeout));
            }

            ASSERT_FALSE(inflightReq->errorCodeValid);
            ASSERT_NE(inflightReq->resultOutputBuffers.size(), 0u);
        }

        // Invalid physical camera id should fail process requests
        frameNumber++;
        camSettings[0].physicalCameraId = invalidPhysicalId;
        camSettings[0].settings.metadata = settings;

        request.physicalCameraSettings = camSettings;  // Invalid camera settings
        returnStatus =
                mSession->processCaptureRequest(requests, cachesToRemove, &numRequestProcessed);
        ASSERT_EQ(static_cast<int32_t>(Status::ILLEGAL_ARGUMENT),
                  returnStatus.getServiceSpecificError());

        defaultPreviewSettings.unlock(settingsBuffer);
        filteredSettings.unlock(filteredSettingsBuffer);

        if (useHalBufManager) {
            std::vector<int32_t> streamIds(halStreams.size());
            for (size_t i = 0; i < streamIds.size(); i++) {
                streamIds[i] = halStreams[i].id;
            }
            verifyBuffersReturned(mSession, streamIds, cb);
        }

        ret = mSession->close();
        mSession = nullptr;
        ASSERT_TRUE(ret.isOk());
    }
}

// Generate and verify an ultra high resolution capture request
TEST_P(CameraAidlTest, processUltraHighResolutionRequest) {
    std::vector<std::string> cameraDeviceNames = getCameraDeviceNames(mProvider);
    int64_t bufferId = 1;
    int32_t frameNumber = 1;
    CameraMetadata settings;

    for (const auto& name : cameraDeviceNames) {
        std::string version, deviceId;
        ASSERT_TRUE(matchDeviceName(name, mProviderType, &version, &deviceId));
        CameraMetadata meta;

        std::shared_ptr<ICameraDevice> unusedDevice;
        openEmptyDeviceSession(name, mProvider, &mSession, &meta, &unusedDevice);
        camera_metadata_t* staticMeta = reinterpret_cast<camera_metadata_t*>(meta.metadata.data());
        if (!isUltraHighResolution(staticMeta)) {
            ndk::ScopedAStatus ret = mSession->close();
            mSession = nullptr;
            ASSERT_TRUE(ret.isOk());
            continue;
        }
        CameraMetadata req;
        android::hardware::camera::common::V1_0::helper::CameraMetadata defaultSettings;
        ndk::ScopedAStatus ret =
                mSession->constructDefaultRequestSettings(RequestTemplate::STILL_CAPTURE, &req);
        ASSERT_TRUE(ret.isOk());

        const camera_metadata_t* metadata =
                reinterpret_cast<const camera_metadata_t*>(req.metadata.data());
        size_t expectedSize = req.metadata.size();
        int result = validate_camera_metadata_structure(metadata, &expectedSize);
        ASSERT_TRUE((result == 0) || (result == CAMERA_METADATA_VALIDATION_SHIFTED));

        size_t entryCount = get_camera_metadata_entry_count(metadata);
        ASSERT_GT(entryCount, 0u);
        defaultSettings = metadata;
        uint8_t sensorPixelMode =
                static_cast<uint8_t>(ANDROID_SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION);
        ASSERT_EQ(::android::OK,
                  defaultSettings.update(ANDROID_SENSOR_PIXEL_MODE, &sensorPixelMode, 1));

        const camera_metadata_t* settingsBuffer = defaultSettings.getAndLock();
        uint8_t* rawSettingsBuffer = (uint8_t*)settingsBuffer;
        settings.metadata = std::vector(
                rawSettingsBuffer, rawSettingsBuffer + get_camera_metadata_size(settingsBuffer));
        overrideRotateAndCrop(&settings);

        ret = mSession->close();
        mSession = nullptr;
        ASSERT_TRUE(ret.isOk());

        std::vector<HalStream> halStreams;
        bool supportsPartialResults = false;
        bool useHalBufManager = false;
        int32_t partialResultCount = 0;
        Stream previewStream;
        std::shared_ptr<DeviceCb> cb;

        std::list<PixelFormat> pixelFormats = {PixelFormat::YCBCR_420_888, PixelFormat::RAW16};
        for (PixelFormat format : pixelFormats) {
            previewStream.usage =
                static_cast<aidl::android::hardware::graphics::common::BufferUsage>(
                        GRALLOC1_CONSUMER_USAGE_CPU_READ);
            previewStream.dataSpace = Dataspace::UNKNOWN;
            configureStreams(name, mProvider, format, &mSession, &previewStream, &halStreams,
                             &supportsPartialResults, &partialResultCount, &useHalBufManager, &cb,
                             0, /*maxResolution*/ true);
            ASSERT_NE(mSession, nullptr);

            ::aidl::android::hardware::common::fmq::MQDescriptor<
                    int8_t, aidl::android::hardware::common::fmq::SynchronizedReadWrite>
                    descriptor;
            auto resultQueueRet = mSession->getCaptureResultMetadataQueue(&descriptor);
            ASSERT_TRUE(resultQueueRet.isOk());

            std::shared_ptr<ResultMetadataQueue> resultQueue =
                    std::make_shared<ResultMetadataQueue>(descriptor);
            if (!resultQueue->isValid() || resultQueue->availableToWrite() <= 0) {
                ALOGE("%s: HAL returns empty result metadata fmq, not use it", __func__);
                resultQueue = nullptr;
                // Don't use the queue onwards.
            }

            std::vector<buffer_handle_t> graphicBuffers;
            graphicBuffers.reserve(halStreams.size());
            std::shared_ptr<InFlightRequest> inflightReq = std::make_shared<InFlightRequest>(
                    static_cast<ssize_t>(halStreams.size()), false, supportsPartialResults,
                    partialResultCount, std::unordered_set<std::string>(), resultQueue);

            std::vector<CaptureRequest> requests(1);
            CaptureRequest& request = requests[0];
            std::vector<StreamBuffer>& outputBuffers = request.outputBuffers;
            outputBuffers.resize(halStreams.size());

            size_t k = 0;
            for (const auto& halStream : halStreams) {
                buffer_handle_t buffer_handle;
                if (useHalBufManager) {
                    outputBuffers[k] = {halStream.id,   0,
                                        NativeHandle(), BufferStatus::OK,
                                        NativeHandle(), NativeHandle()};
                } else {
                    allocateGraphicBuffer(previewStream.width, previewStream.height,
                                          android_convertGralloc1To0Usage(
                                                  static_cast<uint64_t>(halStream.producerUsage),
                                                  static_cast<uint64_t>(halStream.consumerUsage)),
                                          halStream.overrideFormat, &buffer_handle);
                    graphicBuffers.push_back(buffer_handle);
                    outputBuffers[k] = {
                            halStream.id,     bufferId,       ::android::makeToAidl(buffer_handle),
                            BufferStatus::OK, NativeHandle(), NativeHandle()};
                    bufferId++;
                }
                k++;
            }

            request.inputBuffer = {
                    -1, 0, NativeHandle(), BufferStatus::ERROR, NativeHandle(), NativeHandle()};
            request.frameNumber = frameNumber;
            request.fmqSettingsSize = 0;
            request.settings = settings;
            request.inputWidth = 0;
            request.inputHeight = 0;

            {
                std::unique_lock<std::mutex> l(mLock);
                mInflightMap.clear();
                mInflightMap[frameNumber] = inflightReq;
            }

            int32_t numRequestProcessed = 0;
            std::vector<BufferCache> cachesToRemove;
            ndk::ScopedAStatus returnStatus =
                    mSession->processCaptureRequest(requests, cachesToRemove, &numRequestProcessed);
            ASSERT_TRUE(returnStatus.isOk());
            ASSERT_EQ(numRequestProcessed, 1u);

            {
                std::unique_lock<std::mutex> l(mLock);
                while (!inflightReq->errorCodeValid &&
                       ((0 < inflightReq->numBuffersLeft) || (!inflightReq->haveResultMetadata))) {
                    auto timeout = std::chrono::system_clock::now() +
                                   std::chrono::seconds(kStreamBufferTimeoutSec);
                    ASSERT_NE(std::cv_status::timeout, mResultCondition.wait_until(l, timeout));
                }

                ASSERT_FALSE(inflightReq->errorCodeValid);
                ASSERT_NE(inflightReq->resultOutputBuffers.size(), 0u);
            }
            if (useHalBufManager) {
                std::vector<int32_t> streamIds(halStreams.size());
                for (size_t i = 0; i < streamIds.size(); i++) {
                    streamIds[i] = halStreams[i].id;
                }
                verifyBuffersReturned(mSession, streamIds, cb);
            }

            ret = mSession->close();
            mSession = nullptr;
            ASSERT_TRUE(ret.isOk());
        }
    }
}

// Generate and verify 10-bit dynamic range request
TEST_P(CameraAidlTest, process10BitDynamicRangeRequest) {
    std::vector<std::string> cameraDeviceNames = getCameraDeviceNames(mProvider);
    CameraMetadata settings;

    for (const auto& name : cameraDeviceNames) {
        std::string version, deviceId;
        ASSERT_TRUE(matchDeviceName(name, mProviderType, &version, &deviceId));
        CameraMetadata meta;
        std::shared_ptr<ICameraDevice> device;
        openEmptyDeviceSession(name, mProvider, &mSession, &meta, &device);
        camera_metadata_t* staticMeta = reinterpret_cast<camera_metadata_t*>(meta.metadata.data());
        if (!is10BitDynamicRangeCapable(staticMeta)) {
            ndk::ScopedAStatus ret = mSession->close();
            mSession = nullptr;
            ASSERT_TRUE(ret.isOk());
            continue;
        }
        std::vector<RequestAvailableDynamicRangeProfilesMap> profileList;
        get10BitDynamicRangeProfiles(staticMeta, &profileList);
        ASSERT_FALSE(profileList.empty());

        CameraMetadata req;
        android::hardware::camera::common::V1_0::helper::CameraMetadata defaultSettings;
        ndk::ScopedAStatus ret =
                mSession->constructDefaultRequestSettings(RequestTemplate::PREVIEW, &req);
        ASSERT_TRUE(ret.isOk());

        const camera_metadata_t* metadata =
                reinterpret_cast<const camera_metadata_t*>(req.metadata.data());
        size_t expectedSize = req.metadata.size();
        int result = validate_camera_metadata_structure(metadata, &expectedSize);
        ASSERT_TRUE((result == 0) || (result == CAMERA_METADATA_VALIDATION_SHIFTED));

        size_t entryCount = get_camera_metadata_entry_count(metadata);
        ASSERT_GT(entryCount, 0u);
        defaultSettings = metadata;

        const camera_metadata_t* settingsBuffer = defaultSettings.getAndLock();
        uint8_t* rawSettingsBuffer = (uint8_t*)settingsBuffer;
        settings.metadata = std::vector(
                rawSettingsBuffer, rawSettingsBuffer + get_camera_metadata_size(settingsBuffer));
        overrideRotateAndCrop(&settings);

        ret = mSession->close();
        mSession = nullptr;
        ASSERT_TRUE(ret.isOk());

        std::vector<HalStream> halStreams;
        bool supportsPartialResults = false;
        bool useHalBufManager = false;
        int32_t partialResultCount = 0;
        Stream previewStream;
        std::shared_ptr<DeviceCb> cb;
        for (const auto& profile : profileList) {
            previewStream.usage =
                static_cast<aidl::android::hardware::graphics::common::BufferUsage>(
                        GRALLOC1_CONSUMER_USAGE_HWCOMPOSER);
            previewStream.dataSpace = getDataspace(PixelFormat::IMPLEMENTATION_DEFINED);
            configureStreams(name, mProvider, PixelFormat::IMPLEMENTATION_DEFINED, &mSession,
                             &previewStream, &halStreams, &supportsPartialResults,
                             &partialResultCount, &useHalBufManager, &cb, 0,
                             /*maxResolution*/ false, profile);
            ASSERT_NE(mSession, nullptr);

            ::aidl::android::hardware::common::fmq::MQDescriptor<
                    int8_t, aidl::android::hardware::common::fmq::SynchronizedReadWrite>
                    descriptor;
            auto resultQueueRet = mSession->getCaptureResultMetadataQueue(&descriptor);
            ASSERT_TRUE(resultQueueRet.isOk());

            std::shared_ptr<ResultMetadataQueue> resultQueue =
                    std::make_shared<ResultMetadataQueue>(descriptor);
            if (!resultQueue->isValid() || resultQueue->availableToWrite() <= 0) {
                ALOGE("%s: HAL returns empty result metadata fmq, not use it", __func__);
                resultQueue = nullptr;
                // Don't use the queue onwards.
            }

            mInflightMap.clear();
            // Stream as long as needed to fill the Hal inflight queue
            std::vector<CaptureRequest> requests(halStreams[0].maxBuffers);

            for (int32_t requestId = 0; requestId < requests.size(); requestId++) {
                std::shared_ptr<InFlightRequest> inflightReq = std::make_shared<InFlightRequest>(
                        static_cast<ssize_t>(halStreams.size()), false, supportsPartialResults,
                        partialResultCount, std::unordered_set<std::string>(), resultQueue);

                CaptureRequest& request = requests[requestId];
                std::vector<StreamBuffer>& outputBuffers = request.outputBuffers;
                outputBuffers.resize(halStreams.size());

                size_t k = 0;
                inflightReq->mOutstandingBufferIds.resize(halStreams.size());
                std::vector<buffer_handle_t> graphicBuffers;
                graphicBuffers.reserve(halStreams.size());

                auto bufferId = requestId + 1; // Buffer id value 0 is not valid
                for (const auto& halStream : halStreams) {
                    buffer_handle_t buffer_handle;
                    if (useHalBufManager) {
                        outputBuffers[k] = {halStream.id,   0,
                                            NativeHandle(), BufferStatus::OK,
                                            NativeHandle(), NativeHandle()};
                    } else {
                        auto usage = android_convertGralloc1To0Usage(
                                static_cast<uint64_t>(halStream.producerUsage),
                                static_cast<uint64_t>(halStream.consumerUsage));
                        allocateGraphicBuffer(previewStream.width, previewStream.height, usage,
                                              halStream.overrideFormat, &buffer_handle);

                        inflightReq->mOutstandingBufferIds[halStream.id][bufferId] = buffer_handle;
                        graphicBuffers.push_back(buffer_handle);
                        outputBuffers[k] = {halStream.id, bufferId,
                            android::makeToAidl(buffer_handle), BufferStatus::OK, NativeHandle(),
                            NativeHandle()};
                    }
                    k++;
                }

                request.inputBuffer = {
                        -1, 0, NativeHandle(), BufferStatus::ERROR, NativeHandle(), NativeHandle()};
                request.frameNumber = bufferId;
                request.fmqSettingsSize = 0;
                request.settings = settings;
                request.inputWidth = 0;
                request.inputHeight = 0;

                {
                    std::unique_lock<std::mutex> l(mLock);
                    mInflightMap[bufferId] = inflightReq;
                }

            }

            int32_t numRequestProcessed = 0;
            std::vector<BufferCache> cachesToRemove;
            ndk::ScopedAStatus returnStatus =
                mSession->processCaptureRequest(requests, cachesToRemove, &numRequestProcessed);
            ASSERT_TRUE(returnStatus.isOk());
            ASSERT_EQ(numRequestProcessed, requests.size());

            returnStatus = mSession->repeatingRequestEnd(requests.size() - 1,
                    std::vector<int32_t> {halStreams[0].id});
            ASSERT_TRUE(returnStatus.isOk());

            // We are keeping frame numbers and buffer ids consistent. Buffer id value of 0
            // is used to indicate a buffer that is not present/available so buffer ids as well
            // as frame numbers begin with 1.
            for (int32_t frameNumber = 1; frameNumber <= requests.size(); frameNumber++) {
                const auto& inflightReq = mInflightMap[frameNumber];
                std::unique_lock<std::mutex> l(mLock);
                while (!inflightReq->errorCodeValid &&
                       ((0 < inflightReq->numBuffersLeft) || (!inflightReq->haveResultMetadata))) {
                    auto timeout = std::chrono::system_clock::now() +
                                   std::chrono::seconds(kStreamBufferTimeoutSec);
                    ASSERT_NE(std::cv_status::timeout, mResultCondition.wait_until(l, timeout));
                }

                waitForReleaseFence(inflightReq->resultOutputBuffers);

                ASSERT_FALSE(inflightReq->errorCodeValid);
                ASSERT_NE(inflightReq->resultOutputBuffers.size(), 0u);
                verify10BitMetadata(mHandleImporter, *inflightReq, profile);
            }

            if (useHalBufManager) {
                std::vector<int32_t> streamIds(halStreams.size());
                for (size_t i = 0; i < streamIds.size(); i++) {
                    streamIds[i] = halStreams[i].id;
                }
                mSession->signalStreamFlush(streamIds, /*streamConfigCounter*/ 0);
                cb->waitForBuffersReturned();
            }

            ret = mSession->close();
            mSession = nullptr;
            ASSERT_TRUE(ret.isOk());
        }
    }
}

// Generate and verify a burst containing alternating sensor sensitivity values
TEST_P(CameraAidlTest, processCaptureRequestBurstISO) {
    std::vector<std::string> cameraDeviceNames = getCameraDeviceNames(mProvider);
    AvailableStream previewThreshold = {kMaxPreviewWidth, kMaxPreviewHeight,
                                        static_cast<int32_t>(PixelFormat::IMPLEMENTATION_DEFINED)};
    int64_t bufferId = 1;
    int32_t frameNumber = 1;
    float isoTol = .03f;
    CameraMetadata settings;

    for (const auto& name : cameraDeviceNames) {
        CameraMetadata meta;
        settings.metadata.clear();
        std::shared_ptr<ICameraDevice> unusedDevice;
        openEmptyDeviceSession(name, mProvider, &mSession /*out*/, &meta /*out*/,
                               &unusedDevice /*out*/);
        camera_metadata_t* staticMetaBuffer =
                clone_camera_metadata(reinterpret_cast<camera_metadata_t*>(meta.metadata.data()));
        ::android::hardware::camera::common::V1_0::helper::CameraMetadata staticMeta(
                staticMetaBuffer);

        camera_metadata_entry_t hwLevel = staticMeta.find(ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL);
        ASSERT_TRUE(0 < hwLevel.count);
        if (ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_LIMITED == hwLevel.data.u8[0] ||
            ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_EXTERNAL == hwLevel.data.u8[0]) {
            // Limited/External devices can skip this test
            ndk::ScopedAStatus ret = mSession->close();
            mSession = nullptr;
            ASSERT_TRUE(ret.isOk());
            continue;
        }

        camera_metadata_entry_t isoRange = staticMeta.find(ANDROID_SENSOR_INFO_SENSITIVITY_RANGE);
        ASSERT_EQ(isoRange.count, 2u);

        ndk::ScopedAStatus ret = mSession->close();
        mSession = nullptr;
        ASSERT_TRUE(ret.isOk());

        bool supportsPartialResults = false;
        bool useHalBufManager = false;
        int32_t partialResultCount = 0;
        Stream previewStream;
        std::vector<HalStream> halStreams;
        std::shared_ptr<DeviceCb> cb;
        configurePreviewStream(name, mProvider, &previewThreshold, &mSession /*out*/,
                               &previewStream /*out*/, &halStreams /*out*/,
                               &supportsPartialResults /*out*/, &partialResultCount /*out*/,
                               &useHalBufManager /*out*/, &cb /*out*/);

        ::aidl::android::hardware::common::fmq::MQDescriptor<
                int8_t, aidl::android::hardware::common::fmq::SynchronizedReadWrite>
                descriptor;
        auto resultQueueRet = mSession->getCaptureResultMetadataQueue(&descriptor);
        std::shared_ptr<ResultMetadataQueue> resultQueue =
                std::make_shared<ResultMetadataQueue>(descriptor);
        ASSERT_TRUE(resultQueueRet.isOk());
        if (!resultQueue->isValid() || resultQueue->availableToWrite() <= 0) {
            ALOGE("%s: HAL returns empty result metadata fmq, not use it", __func__);
            resultQueue = nullptr;
            // Don't use the queue onwards.
        }

        ret = mSession->constructDefaultRequestSettings(RequestTemplate::PREVIEW, &settings);
        ASSERT_TRUE(ret.isOk());

        ::android::hardware::camera::common::V1_0::helper::CameraMetadata requestMeta;
        std::vector<CaptureRequest> requests(kBurstFrameCount);
        std::vector<buffer_handle_t> buffers(kBurstFrameCount);
        std::vector<std::shared_ptr<InFlightRequest>> inflightReqs(kBurstFrameCount);
        std::vector<int32_t> isoValues(kBurstFrameCount);
        std::vector<CameraMetadata> requestSettings(kBurstFrameCount);

        for (int32_t i = 0; i < kBurstFrameCount; i++) {
            std::unique_lock<std::mutex> l(mLock);
            CaptureRequest& request = requests[i];
            std::vector<StreamBuffer>& outputBuffers = request.outputBuffers;
            outputBuffers.resize(1);
            StreamBuffer& outputBuffer = outputBuffers[0];

            isoValues[i] = ((i % 2) == 0) ? isoRange.data.i32[0] : isoRange.data.i32[1];
            if (useHalBufManager) {
                outputBuffer = {halStreams[0].id, 0,
                                NativeHandle(),   BufferStatus::OK,
                                NativeHandle(),   NativeHandle()};
            } else {
                allocateGraphicBuffer(previewStream.width, previewStream.height,
                                      android_convertGralloc1To0Usage(
                                              static_cast<uint64_t>(halStreams[0].producerUsage),
                                              static_cast<uint64_t>(halStreams[0].consumerUsage)),
                                      halStreams[0].overrideFormat, &buffers[i]);
                outputBuffer = {halStreams[0].id, bufferId + i,   ::android::makeToAidl(buffers[i]),
                                BufferStatus::OK, NativeHandle(), NativeHandle()};
            }

            requestMeta.append(reinterpret_cast<camera_metadata_t*>(settings.metadata.data()));

            // Disable all 3A routines
            uint8_t mode = static_cast<uint8_t>(ANDROID_CONTROL_MODE_OFF);
            ASSERT_EQ(::android::OK, requestMeta.update(ANDROID_CONTROL_MODE, &mode, 1));
            ASSERT_EQ(::android::OK,
                      requestMeta.update(ANDROID_SENSOR_SENSITIVITY, &isoValues[i], 1));
            camera_metadata_t* metaBuffer = requestMeta.release();
            uint8_t* rawMetaBuffer = reinterpret_cast<uint8_t*>(metaBuffer);
            requestSettings[i].metadata = std::vector(
                    rawMetaBuffer, rawMetaBuffer + get_camera_metadata_size(metaBuffer));
            overrideRotateAndCrop(&(requestSettings[i]));

            request.frameNumber = frameNumber + i;
            request.fmqSettingsSize = 0;
            request.settings = requestSettings[i];
            request.inputBuffer = {
                    -1, 0, NativeHandle(), BufferStatus::ERROR, NativeHandle(), NativeHandle()};

            inflightReqs[i] = std::make_shared<InFlightRequest>(1, false, supportsPartialResults,
                                                                partialResultCount, resultQueue);
            mInflightMap[frameNumber + i] = inflightReqs[i];
        }

        int32_t numRequestProcessed = 0;
        std::vector<BufferCache> cachesToRemove;

        ndk::ScopedAStatus returnStatus =
                mSession->processCaptureRequest(requests, cachesToRemove, &numRequestProcessed);
        ASSERT_TRUE(returnStatus.isOk());
        ASSERT_EQ(numRequestProcessed, kBurstFrameCount);

        for (size_t i = 0; i < kBurstFrameCount; i++) {
            std::unique_lock<std::mutex> l(mLock);
            while (!inflightReqs[i]->errorCodeValid && ((0 < inflightReqs[i]->numBuffersLeft) ||
                                                        (!inflightReqs[i]->haveResultMetadata))) {
                auto timeout = std::chrono::system_clock::now() +
                               std::chrono::seconds(kStreamBufferTimeoutSec);
                ASSERT_NE(std::cv_status::timeout, mResultCondition.wait_until(l, timeout));
            }

            ASSERT_FALSE(inflightReqs[i]->errorCodeValid);
            ASSERT_NE(inflightReqs[i]->resultOutputBuffers.size(), 0u);
            ASSERT_EQ(previewStream.id, inflightReqs[i]->resultOutputBuffers[0].buffer.streamId);
            ASSERT_FALSE(inflightReqs[i]->collectedResult.isEmpty());
            ASSERT_TRUE(inflightReqs[i]->collectedResult.exists(ANDROID_SENSOR_SENSITIVITY));
            camera_metadata_entry_t isoResult =
                    inflightReqs[i]->collectedResult.find(ANDROID_SENSOR_SENSITIVITY);
            ASSERT_TRUE(std::abs(isoResult.data.i32[0] - isoValues[i]) <=
                        std::round(isoValues[i] * isoTol));
        }

        if (useHalBufManager) {
            verifyBuffersReturned(mSession, previewStream.id, cb);
        }
        ret = mSession->close();
        mSession = nullptr;
        ASSERT_TRUE(ret.isOk());
    }
}

// Test whether an incorrect capture request with missing settings will
// be reported correctly.
TEST_P(CameraAidlTest, processCaptureRequestInvalidSinglePreview) {
    std::vector<std::string> cameraDeviceNames = getCameraDeviceNames(mProvider);
    std::vector<AvailableStream> outputPreviewStreams;
    AvailableStream previewThreshold = {kMaxPreviewWidth, kMaxPreviewHeight,
                                        static_cast<int32_t>(PixelFormat::IMPLEMENTATION_DEFINED)};
    int64_t bufferId = 1;
    int32_t frameNumber = 1;
    CameraMetadata settings;

    for (const auto& name : cameraDeviceNames) {
        Stream previewStream;
        std::vector<HalStream> halStreams;
        std::shared_ptr<DeviceCb> cb;
        bool supportsPartialResults = false;
        bool useHalBufManager = false;
        int32_t partialResultCount = 0;
        configurePreviewStream(name, mProvider, &previewThreshold, &mSession /*out*/,
                               &previewStream /*out*/, &halStreams /*out*/,
                               &supportsPartialResults /*out*/, &partialResultCount /*out*/,
                               &useHalBufManager /*out*/, &cb /*out*/);
        ASSERT_NE(mSession, nullptr);
        ASSERT_FALSE(halStreams.empty());

        buffer_handle_t buffer_handle = nullptr;

        if (useHalBufManager) {
            bufferId = 0;
        } else {
            allocateGraphicBuffer(previewStream.width, previewStream.height,
                                  android_convertGralloc1To0Usage(
                                          static_cast<uint64_t>(halStreams[0].producerUsage),
                                          static_cast<uint64_t>(halStreams[0].consumerUsage)),
                                  halStreams[0].overrideFormat, &buffer_handle);
        }

        std::vector<CaptureRequest> requests(1);
        CaptureRequest& request = requests[0];
        std::vector<StreamBuffer>& outputBuffers = request.outputBuffers;
        outputBuffers.resize(1);
        StreamBuffer& outputBuffer = outputBuffers[0];

        outputBuffer = {
                halStreams[0].id,
                bufferId,
                buffer_handle == nullptr ? NativeHandle() : ::android::makeToAidl(buffer_handle),
                BufferStatus::OK,
                NativeHandle(),
                NativeHandle()};

        request.inputBuffer = {
                -1, 0, NativeHandle(), BufferStatus::ERROR, NativeHandle(), NativeHandle()};
        request.frameNumber = frameNumber;
        request.fmqSettingsSize = 0;
        request.settings = settings;

        // Settings were not correctly initialized, we should fail here
        int32_t numRequestProcessed = 0;
        std::vector<BufferCache> cachesToRemove;
        ndk::ScopedAStatus ret =
                mSession->processCaptureRequest(requests, cachesToRemove, &numRequestProcessed);
        ASSERT_EQ(static_cast<int32_t>(Status::ILLEGAL_ARGUMENT), ret.getServiceSpecificError());
        ASSERT_EQ(numRequestProcessed, 0u);

        ret = mSession->close();
        mSession = nullptr;
        ASSERT_TRUE(ret.isOk());
    }
}

// Verify camera offline session behavior
TEST_P(CameraAidlTest, switchToOffline) {
    std::vector<std::string> cameraDeviceNames = getCameraDeviceNames(mProvider);
    AvailableStream threshold = {kMaxStillWidth, kMaxStillHeight,
                                 static_cast<int32_t>(PixelFormat::BLOB)};
    int64_t bufferId = 1;
    int32_t frameNumber = 1;
    CameraMetadata settings;

    for (const auto& name : cameraDeviceNames) {
        CameraMetadata meta;
        {
            std::shared_ptr<ICameraDevice> unusedDevice;
            openEmptyDeviceSession(name, mProvider, &mSession /*out*/, &meta /*out*/,
                                   &unusedDevice);
            camera_metadata_t* staticMetaBuffer = clone_camera_metadata(
                    reinterpret_cast<camera_metadata_t*>(meta.metadata.data()));
            ::android::hardware::camera::common::V1_0::helper::CameraMetadata staticMeta(
                    staticMetaBuffer);

            if (isOfflineSessionSupported(staticMetaBuffer) != Status::OK) {
                ndk::ScopedAStatus ret = mSession->close();
                mSession = nullptr;
                ASSERT_TRUE(ret.isOk());
                continue;
            }
            ndk::ScopedAStatus ret = mSession->close();
            mSession = nullptr;
            ASSERT_TRUE(ret.isOk());
        }

        bool supportsPartialResults = false;
        int32_t partialResultCount = 0;
        Stream stream;
        std::vector<HalStream> halStreams;
        std::shared_ptr<DeviceCb> cb;
        int32_t jpegBufferSize;
        bool useHalBufManager;
        configureOfflineStillStream(name, mProvider, &threshold, &mSession /*out*/, &stream /*out*/,
                                    &halStreams /*out*/, &supportsPartialResults /*out*/,
                                    &partialResultCount /*out*/, &cb /*out*/,
                                    &jpegBufferSize /*out*/, &useHalBufManager /*out*/);

        auto ret = mSession->constructDefaultRequestSettings(RequestTemplate::STILL_CAPTURE,
                                                             &settings);
        ASSERT_TRUE(ret.isOk());

        ::aidl::android::hardware::common::fmq::MQDescriptor<
                int8_t, aidl::android::hardware::common::fmq::SynchronizedReadWrite>
                descriptor;

        ndk::ScopedAStatus resultQueueRet = mSession->getCaptureResultMetadataQueue(&descriptor);
        ASSERT_TRUE(resultQueueRet.isOk());
        std::shared_ptr<ResultMetadataQueue> resultQueue =
                std::make_shared<ResultMetadataQueue>(descriptor);
        if (!resultQueue->isValid() || resultQueue->availableToWrite() <= 0) {
            ALOGE("%s: HAL returns empty result metadata fmq, not use it", __func__);
            resultQueue = nullptr;
            // Don't use the queue onwards.
        }

        ::android::hardware::camera::common::V1_0::helper::CameraMetadata requestMeta;

        std::vector<buffer_handle_t> buffers(kBurstFrameCount);
        std::vector<std::shared_ptr<InFlightRequest>> inflightReqs(kBurstFrameCount);
        std::vector<CameraMetadata> requestSettings(kBurstFrameCount);

        std::vector<CaptureRequest> requests(kBurstFrameCount);

        HalStream halStream = halStreams[0];
        for (uint32_t i = 0; i < kBurstFrameCount; i++) {
            CaptureRequest& request = requests[i];
            std::vector<StreamBuffer>& outputBuffers = request.outputBuffers;
            outputBuffers.resize(1);
            StreamBuffer& outputBuffer = outputBuffers[0];

            std::unique_lock<std::mutex> l(mLock);
            if (useHalBufManager) {
                outputBuffer = {halStream.id,  0, NativeHandle(), BufferStatus::OK, NativeHandle(),
                                NativeHandle()};
            } else {
                // jpeg buffer (w,h) = (blobLen, 1)
                allocateGraphicBuffer(jpegBufferSize, /*height*/ 1,
                                      android_convertGralloc1To0Usage(
                                              static_cast<uint64_t>(halStream.producerUsage),
                                              static_cast<uint64_t>(halStream.consumerUsage)),
                                      halStream.overrideFormat, &buffers[i]);
                outputBuffer = {halStream.id,     bufferId + i,   ::android::makeToAidl(buffers[i]),
                                BufferStatus::OK, NativeHandle(), NativeHandle()};
            }

            requestMeta.clear();
            requestMeta.append(reinterpret_cast<camera_metadata_t*>(settings.metadata.data()));

            camera_metadata_t* metaBuffer = requestMeta.release();
            uint8_t* rawMetaBuffer = reinterpret_cast<uint8_t*>(metaBuffer);
            requestSettings[i].metadata = std::vector(
                    rawMetaBuffer, rawMetaBuffer + get_camera_metadata_size(metaBuffer));
            overrideRotateAndCrop(&requestSettings[i]);

            request.frameNumber = frameNumber + i;
            request.fmqSettingsSize = 0;
            request.settings = requestSettings[i];
            request.inputBuffer = {/*streamId*/ -1,
                                   /*bufferId*/ 0,      NativeHandle(),
                                   BufferStatus::ERROR, NativeHandle(),
                                   NativeHandle()};

            inflightReqs[i] = std::make_shared<InFlightRequest>(1, false, supportsPartialResults,
                                                                partialResultCount, resultQueue);
            mInflightMap[frameNumber + i] = inflightReqs[i];
        }

        int32_t numRequestProcessed = 0;
        std::vector<BufferCache> cachesToRemove;

        ndk::ScopedAStatus returnStatus =
                mSession->processCaptureRequest(requests, cachesToRemove, &numRequestProcessed);
        ASSERT_TRUE(returnStatus.isOk());
        ASSERT_EQ(numRequestProcessed, kBurstFrameCount);

        std::vector<int32_t> offlineStreamIds = {halStream.id};
        CameraOfflineSessionInfo offlineSessionInfo;
        std::shared_ptr<ICameraOfflineSession> offlineSession;
        returnStatus =
                mSession->switchToOffline(offlineStreamIds, &offlineSessionInfo, &offlineSession);

        if (!halStreams[0].supportOffline) {
            ASSERT_EQ(static_cast<int32_t>(Status::ILLEGAL_ARGUMENT),
                      returnStatus.getServiceSpecificError());
            ret = mSession->close();
            mSession = nullptr;
            ASSERT_TRUE(ret.isOk());
            continue;
        }

        ASSERT_TRUE(returnStatus.isOk());
        // Hal might be unable to find any requests qualified for offline mode.
        if (offlineSession == nullptr) {
            ret = mSession->close();
            mSession = nullptr;
            ASSERT_TRUE(ret.isOk());
            continue;
        }

        ASSERT_EQ(offlineSessionInfo.offlineStreams.size(), 1u);
        ASSERT_EQ(offlineSessionInfo.offlineStreams[0].id, halStream.id);
        ASSERT_NE(offlineSessionInfo.offlineRequests.size(), 0u);

        // close device session to make sure offline session does not rely on it
        ret = mSession->close();
        mSession = nullptr;
        ASSERT_TRUE(ret.isOk());

        ::aidl::android::hardware::common::fmq::MQDescriptor<
                int8_t, aidl::android::hardware::common::fmq::SynchronizedReadWrite>
                offlineResultDescriptor;

        auto offlineResultQueueRet =
                offlineSession->getCaptureResultMetadataQueue(&offlineResultDescriptor);
        std::shared_ptr<ResultMetadataQueue> offlineResultQueue =
                std::make_shared<ResultMetadataQueue>(descriptor);
        if (!offlineResultQueue->isValid() || offlineResultQueue->availableToWrite() <= 0) {
            ALOGE("%s: offline session returns empty result metadata fmq, not use it", __func__);
            offlineResultQueue = nullptr;
            // Don't use the queue onwards.
        }
        ASSERT_TRUE(offlineResultQueueRet.isOk());

        updateInflightResultQueue(offlineResultQueue);

        ret = offlineSession->setCallback(cb);
        ASSERT_TRUE(ret.isOk());

        for (size_t i = 0; i < kBurstFrameCount; i++) {
            std::unique_lock<std::mutex> l(mLock);
            while (!inflightReqs[i]->errorCodeValid && ((0 < inflightReqs[i]->numBuffersLeft) ||
                                                        (!inflightReqs[i]->haveResultMetadata))) {
                auto timeout = std::chrono::system_clock::now() +
                               std::chrono::seconds(kStreamBufferTimeoutSec);
                ASSERT_NE(std::cv_status::timeout, mResultCondition.wait_until(l, timeout));
            }

            ASSERT_FALSE(inflightReqs[i]->errorCodeValid);
            ASSERT_NE(inflightReqs[i]->resultOutputBuffers.size(), 0u);
            ASSERT_EQ(stream.id, inflightReqs[i]->resultOutputBuffers[0].buffer.streamId);
            ASSERT_FALSE(inflightReqs[i]->collectedResult.isEmpty());
        }

        ret = offlineSession->close();
        ASSERT_TRUE(ret.isOk());
    }
}

// Check whether an invalid capture request with missing output buffers
// will be reported correctly.
TEST_P(CameraAidlTest, processCaptureRequestInvalidBuffer) {
    std::vector<std::string> cameraDeviceNames = getCameraDeviceNames(mProvider);
    std::vector<AvailableStream> outputBlobStreams;
    AvailableStream previewThreshold = {kMaxPreviewWidth, kMaxPreviewHeight,
                                        static_cast<int32_t>(PixelFormat::IMPLEMENTATION_DEFINED)};
    int32_t frameNumber = 1;
    CameraMetadata settings;

    for (const auto& name : cameraDeviceNames) {
        Stream previewStream;
        std::vector<HalStream> halStreams;
        std::shared_ptr<DeviceCb> cb;
        bool supportsPartialResults = false;
        bool useHalBufManager = false;
        int32_t partialResultCount = 0;
        configurePreviewStream(name, mProvider, &previewThreshold, &mSession /*out*/,
                               &previewStream /*out*/, &halStreams /*out*/,
                               &supportsPartialResults /*out*/, &partialResultCount /*out*/,
                               &useHalBufManager /*out*/, &cb /*out*/);

        RequestTemplate reqTemplate = RequestTemplate::PREVIEW;
        ndk::ScopedAStatus ret = mSession->constructDefaultRequestSettings(reqTemplate, &settings);
        ASSERT_TRUE(ret.isOk());
        overrideRotateAndCrop(&settings);

        std::vector<CaptureRequest> requests(1);
        CaptureRequest& request = requests[0];
        std::vector<StreamBuffer>& outputBuffers = request.outputBuffers;
        outputBuffers.resize(1);
        // Empty output buffer
        outputBuffers[0] = {
                -1, 0, NativeHandle(), BufferStatus::ERROR, NativeHandle(), NativeHandle()};

        request.inputBuffer = {
                -1, 0, NativeHandle(), BufferStatus::ERROR, NativeHandle(), NativeHandle()};
        request.frameNumber = frameNumber;
        request.fmqSettingsSize = 0;
        request.settings = settings;

        // Output buffers are missing, we should fail here
        int32_t numRequestProcessed = 0;
        std::vector<BufferCache> cachesToRemove;
        ret = mSession->processCaptureRequest(requests, cachesToRemove, &numRequestProcessed);
        ASSERT_EQ(static_cast<int32_t>(Status::ILLEGAL_ARGUMENT), ret.getServiceSpecificError());
        ASSERT_EQ(numRequestProcessed, 0u);

        ret = mSession->close();
        mSession = nullptr;
        ASSERT_TRUE(ret.isOk());
    }
}

// Generate, trigger and flush a preview request
TEST_P(CameraAidlTest, flushPreviewRequest) {
    std::vector<std::string> cameraDeviceNames = getCameraDeviceNames(mProvider);
    std::vector<AvailableStream> outputPreviewStreams;
    AvailableStream previewThreshold = {kMaxPreviewWidth, kMaxPreviewHeight,
                                        static_cast<int32_t>(PixelFormat::IMPLEMENTATION_DEFINED)};
    int64_t bufferId = 1;
    int32_t frameNumber = 1;
    CameraMetadata settings;

    for (const auto& name : cameraDeviceNames) {
        Stream previewStream;
        std::vector<HalStream> halStreams;
        std::shared_ptr<DeviceCb> cb;
        bool supportsPartialResults = false;
        bool useHalBufManager = false;
        int32_t partialResultCount = 0;

        configurePreviewStream(name, mProvider, &previewThreshold, &mSession /*out*/,
                               &previewStream /*out*/, &halStreams /*out*/,
                               &supportsPartialResults /*out*/, &partialResultCount /*out*/,
                               &useHalBufManager /*out*/, &cb /*out*/);

        ASSERT_NE(mSession, nullptr);
        ASSERT_NE(cb, nullptr);
        ASSERT_FALSE(halStreams.empty());

        ::aidl::android::hardware::common::fmq::MQDescriptor<
                int8_t, aidl::android::hardware::common::fmq::SynchronizedReadWrite>
                descriptor;

        auto resultQueueRet = mSession->getCaptureResultMetadataQueue(&descriptor);
        std::shared_ptr<ResultMetadataQueue> resultQueue =
                std::make_shared<ResultMetadataQueue>(descriptor);
        ASSERT_TRUE(resultQueueRet.isOk());
        if (!resultQueue->isValid() || resultQueue->availableToWrite() <= 0) {
            ALOGE("%s: HAL returns empty result metadata fmq, not use it", __func__);
            resultQueue = nullptr;
            // Don't use the queue onwards.
        }

        std::shared_ptr<InFlightRequest> inflightReq = std::make_shared<InFlightRequest>(
                1, false, supportsPartialResults, partialResultCount, resultQueue);
        RequestTemplate reqTemplate = RequestTemplate::PREVIEW;

        ndk::ScopedAStatus ret = mSession->constructDefaultRequestSettings(reqTemplate, &settings);
        ASSERT_TRUE(ret.isOk());
        overrideRotateAndCrop(&settings);

        buffer_handle_t buffer_handle;
        std::vector<CaptureRequest> requests(1);
        CaptureRequest& request = requests[0];
        std::vector<StreamBuffer>& outputBuffers = request.outputBuffers;
        outputBuffers.resize(1);
        StreamBuffer& outputBuffer = outputBuffers[0];
        if (useHalBufManager) {
            bufferId = 0;
            outputBuffer = {halStreams[0].id, bufferId,       NativeHandle(),
                            BufferStatus::OK, NativeHandle(), NativeHandle()};
        } else {
            allocateGraphicBuffer(previewStream.width, previewStream.height,
                                  android_convertGralloc1To0Usage(
                                          static_cast<uint64_t>(halStreams[0].producerUsage),
                                          static_cast<uint64_t>(halStreams[0].consumerUsage)),
                                  halStreams[0].overrideFormat, &buffer_handle);
            outputBuffer = {halStreams[0].id, bufferId,       ::android::makeToAidl(buffer_handle),
                            BufferStatus::OK, NativeHandle(), NativeHandle()};
        }

        request.frameNumber = frameNumber;
        request.fmqSettingsSize = 0;
        request.settings = settings;
        request.inputBuffer = {
                -1, 0, NativeHandle(), BufferStatus::ERROR, NativeHandle(), NativeHandle()};

        {
            std::unique_lock<std::mutex> l(mLock);
            mInflightMap.clear();
            mInflightMap[frameNumber] = inflightReq;
        }

        int32_t numRequestProcessed = 0;
        std::vector<BufferCache> cachesToRemove;
        ret = mSession->processCaptureRequest(requests, cachesToRemove, &numRequestProcessed);
        ASSERT_TRUE(ret.isOk());
        ASSERT_EQ(numRequestProcessed, 1u);

        // Flush before waiting for request to complete.
        ndk::ScopedAStatus returnStatus = mSession->flush();
        ASSERT_TRUE(returnStatus.isOk());

        {
            std::unique_lock<std::mutex> l(mLock);
            while (!inflightReq->errorCodeValid &&
                   ((0 < inflightReq->numBuffersLeft) || (!inflightReq->haveResultMetadata))) {
                auto timeout = std::chrono::system_clock::now() +
                               std::chrono::seconds(kStreamBufferTimeoutSec);
                ASSERT_NE(std::cv_status::timeout, mResultCondition.wait_until(l, timeout));
            }

            if (!inflightReq->errorCodeValid) {
                ASSERT_NE(inflightReq->resultOutputBuffers.size(), 0u);
                ASSERT_EQ(previewStream.id, inflightReq->resultOutputBuffers[0].buffer.streamId);
            } else {
                switch (inflightReq->errorCode) {
                    case ErrorCode::ERROR_REQUEST:
                    case ErrorCode::ERROR_RESULT:
                    case ErrorCode::ERROR_BUFFER:
                        // Expected
                        break;
                    case ErrorCode::ERROR_DEVICE:
                    default:
                        FAIL() << "Unexpected error:"
                               << static_cast<uint32_t>(inflightReq->errorCode);
                }
            }
        }

        if (useHalBufManager) {
            verifyBuffersReturned(mSession, previewStream.id, cb);
        }

        ret = mSession->close();
        mSession = nullptr;
        ASSERT_TRUE(ret.isOk());
    }
}

// Verify that camera flushes correctly without any pending requests.
TEST_P(CameraAidlTest, flushEmpty) {
    std::vector<std::string> cameraDeviceNames = getCameraDeviceNames(mProvider);
    std::vector<AvailableStream> outputPreviewStreams;
    AvailableStream previewThreshold = {kMaxPreviewWidth, kMaxPreviewHeight,
                                        static_cast<int32_t>(PixelFormat::IMPLEMENTATION_DEFINED)};

    for (const auto& name : cameraDeviceNames) {
        Stream previewStream;
        std::vector<HalStream> halStreams;
        std::shared_ptr<DeviceCb> cb;
        bool supportsPartialResults = false;
        bool useHalBufManager = false;

        int32_t partialResultCount = 0;
        configurePreviewStream(name, mProvider, &previewThreshold, &mSession /*out*/,
                               &previewStream /*out*/, &halStreams /*out*/,
                               &supportsPartialResults /*out*/, &partialResultCount /*out*/,
                               &useHalBufManager /*out*/, &cb /*out*/);

        ndk::ScopedAStatus returnStatus = mSession->flush();
        ASSERT_TRUE(returnStatus.isOk());

        {
            std::unique_lock<std::mutex> l(mLock);
            auto timeout = std::chrono::system_clock::now() +
                           std::chrono::milliseconds(kEmptyFlushTimeoutMSec);
            ASSERT_EQ(std::cv_status::timeout, mResultCondition.wait_until(l, timeout));
        }

        ndk::ScopedAStatus ret = mSession->close();
        mSession = nullptr;
        ASSERT_TRUE(ret.isOk());
    }
}

// Test camera provider notify method
TEST_P(CameraAidlTest, providerDeviceStateNotification) {
    notifyDeviceState(ICameraProvider::DEVICE_STATE_BACK_COVERED);
    notifyDeviceState(ICameraProvider::DEVICE_STATE_NORMAL);
}

// Verify that all supported stream formats and sizes can be configured
// successfully for injection camera.
TEST_P(CameraAidlTest, configureInjectionStreamsAvailableOutputs) {
    std::vector<std::string> cameraDeviceNames = getCameraDeviceNames(mProvider);
    std::vector<AvailableStream> outputStreams;

    for (const auto& name : cameraDeviceNames) {
        CameraMetadata metadata;

        std::shared_ptr<ICameraInjectionSession> injectionSession;
        std::shared_ptr<ICameraDevice> unusedDevice;
        openEmptyInjectionSession(name, mProvider, &injectionSession /*out*/, &metadata /*out*/,
                                  &unusedDevice /*out*/);
        if (injectionSession == nullptr) {
            continue;
        }

        camera_metadata_t* staticMetaBuffer =
                reinterpret_cast<camera_metadata_t*>(metadata.metadata.data());
        CameraMetadata chars;
        chars.metadata = metadata.metadata;

        outputStreams.clear();
        ASSERT_EQ(Status::OK, getAvailableOutputStreams(staticMetaBuffer, outputStreams));
        ASSERT_NE(0u, outputStreams.size());

        int32_t jpegBufferSize = 0;
        ASSERT_EQ(Status::OK, getJpegBufferSize(staticMetaBuffer, &jpegBufferSize));
        ASSERT_NE(0u, jpegBufferSize);

        int32_t streamId = 0;
        int32_t streamConfigCounter = 0;
        for (auto& it : outputStreams) {
            Dataspace dataspace = getDataspace(static_cast<PixelFormat>(it.format));
            Stream stream = {streamId,
                             StreamType::OUTPUT,
                             it.width,
                             it.height,
                             static_cast<PixelFormat>(it.format),
                             static_cast<aidl::android::hardware::graphics::common::BufferUsage>(
                                     GRALLOC1_CONSUMER_USAGE_HWCOMPOSER),
                             dataspace,
                             StreamRotation::ROTATION_0,
                             std::string(),
                             jpegBufferSize,
                             0,
                             {SensorPixelMode::ANDROID_SENSOR_PIXEL_MODE_DEFAULT},
                             RequestAvailableDynamicRangeProfilesMap::
                                     ANDROID_REQUEST_AVAILABLE_DYNAMIC_RANGE_PROFILES_MAP_STANDARD};

            std::vector<Stream> streams = {stream};
            StreamConfiguration config;
            createStreamConfiguration(streams, StreamConfigurationMode::NORMAL_MODE, &config,
                                      jpegBufferSize);

            config.streamConfigCounter = streamConfigCounter++;
            ndk::ScopedAStatus s = injectionSession->configureInjectionStreams(config, chars);
            ASSERT_TRUE(s.isOk());
            streamId++;
        }

        std::shared_ptr<ICameraDeviceSession> session;
        ndk::ScopedAStatus ret = injectionSession->getCameraDeviceSession(&session);
        ASSERT_TRUE(ret.isOk());
        ASSERT_NE(session, nullptr);
        ret = session->close();
        ASSERT_TRUE(ret.isOk());
    }
}

// Check for correct handling of invalid/incorrect configuration parameters for injection camera.
TEST_P(CameraAidlTest, configureInjectionStreamsInvalidOutputs) {
    std::vector<std::string> cameraDeviceNames = getCameraDeviceNames(mProvider);
    std::vector<AvailableStream> outputStreams;

    for (const auto& name : cameraDeviceNames) {
        CameraMetadata metadata;
        std::shared_ptr<ICameraInjectionSession> injectionSession;
        std::shared_ptr<ICameraDevice> unusedDevice;
        openEmptyInjectionSession(name, mProvider, &injectionSession /*out*/, &metadata /*out*/,
                                  &unusedDevice);
        if (injectionSession == nullptr) {
            continue;
        }

        camera_metadata_t* staticMetaBuffer =
                reinterpret_cast<camera_metadata_t*>(metadata.metadata.data());
        std::shared_ptr<ICameraDeviceSession> session;
        ndk::ScopedAStatus ret = injectionSession->getCameraDeviceSession(&session);
        ASSERT_TRUE(ret.isOk());
        ASSERT_NE(session, nullptr);

        CameraMetadata chars;
        chars.metadata = metadata.metadata;

        outputStreams.clear();
        ASSERT_EQ(Status::OK, getAvailableOutputStreams(staticMetaBuffer, outputStreams));
        ASSERT_NE(0u, outputStreams.size());

        int32_t jpegBufferSize = 0;
        ASSERT_EQ(Status::OK, getJpegBufferSize(staticMetaBuffer, &jpegBufferSize));
        ASSERT_NE(0u, jpegBufferSize);

        int32_t streamId = 0;
        Stream stream = {streamId++,
                         StreamType::OUTPUT,
                         0,
                         0,
                         static_cast<PixelFormat>(outputStreams[0].format),
                         static_cast<aidl::android::hardware::graphics::common::BufferUsage>(
                                 GRALLOC1_CONSUMER_USAGE_HWCOMPOSER),
                         Dataspace::UNKNOWN,
                         StreamRotation::ROTATION_0,
                         std::string(),
                         jpegBufferSize,
                         0,
                         {SensorPixelMode::ANDROID_SENSOR_PIXEL_MODE_DEFAULT},
                         RequestAvailableDynamicRangeProfilesMap::
                                 ANDROID_REQUEST_AVAILABLE_DYNAMIC_RANGE_PROFILES_MAP_STANDARD};

        int32_t streamConfigCounter = 0;
        std::vector<Stream> streams = {stream};
        StreamConfiguration config;
        createStreamConfiguration(streams, StreamConfigurationMode::NORMAL_MODE, &config,
                                  jpegBufferSize);

        config.streamConfigCounter = streamConfigCounter++;
        ndk::ScopedAStatus s = injectionSession->configureInjectionStreams(config, chars);
        ASSERT_TRUE(
                (static_cast<int32_t>(Status::ILLEGAL_ARGUMENT) == s.getServiceSpecificError()) ||
                (static_cast<int32_t>(Status::INTERNAL_ERROR) == s.getServiceSpecificError()));

        stream = {streamId++,
                  StreamType::OUTPUT,
                  INT32_MAX,
                  INT32_MAX,
                  static_cast<PixelFormat>(outputStreams[0].format),
                  static_cast<aidl::android::hardware::graphics::common::BufferUsage>(
                          GRALLOC1_CONSUMER_USAGE_HWCOMPOSER),
                  Dataspace::UNKNOWN,
                  StreamRotation::ROTATION_0,
                  std::string(),
                  jpegBufferSize,
                  0,
                  {SensorPixelMode::ANDROID_SENSOR_PIXEL_MODE_DEFAULT},
                  RequestAvailableDynamicRangeProfilesMap::
                          ANDROID_REQUEST_AVAILABLE_DYNAMIC_RANGE_PROFILES_MAP_STANDARD};

        streams[0] = stream;
        createStreamConfiguration(streams, StreamConfigurationMode::NORMAL_MODE, &config,
                                  jpegBufferSize);
        config.streamConfigCounter = streamConfigCounter++;
        s = injectionSession->configureInjectionStreams(config, chars);
        ASSERT_EQ(static_cast<int32_t>(Status::ILLEGAL_ARGUMENT), s.getServiceSpecificError());

        for (auto& it : outputStreams) {
            stream = {streamId++,
                      StreamType::OUTPUT,
                      it.width,
                      it.height,
                      static_cast<PixelFormat>(INT32_MAX),
                      static_cast<aidl::android::hardware::graphics::common::BufferUsage>(
                              GRALLOC1_CONSUMER_USAGE_HWCOMPOSER),
                      Dataspace::UNKNOWN,
                      StreamRotation::ROTATION_0,
                      std::string(),
                      jpegBufferSize,
                      0,
                      {SensorPixelMode::ANDROID_SENSOR_PIXEL_MODE_DEFAULT},
                      RequestAvailableDynamicRangeProfilesMap::
                              ANDROID_REQUEST_AVAILABLE_DYNAMIC_RANGE_PROFILES_MAP_STANDARD};
            streams[0] = stream;
            createStreamConfiguration(streams, StreamConfigurationMode::NORMAL_MODE, &config,
                                      jpegBufferSize);
            config.streamConfigCounter = streamConfigCounter++;
            s = injectionSession->configureInjectionStreams(config, chars);
            ASSERT_EQ(static_cast<int32_t>(Status::ILLEGAL_ARGUMENT), s.getServiceSpecificError());

            stream = {streamId++,
                      StreamType::OUTPUT,
                      it.width,
                      it.height,
                      static_cast<PixelFormat>(it.format),
                      static_cast<aidl::android::hardware::graphics::common::BufferUsage>(
                              GRALLOC1_CONSUMER_USAGE_HWCOMPOSER),
                      Dataspace::UNKNOWN,
                      static_cast<StreamRotation>(INT32_MAX),
                      std::string(),
                      jpegBufferSize,
                      0,
                      {SensorPixelMode::ANDROID_SENSOR_PIXEL_MODE_DEFAULT},
                      RequestAvailableDynamicRangeProfilesMap::
                              ANDROID_REQUEST_AVAILABLE_DYNAMIC_RANGE_PROFILES_MAP_STANDARD};
            streams[0] = stream;
            createStreamConfiguration(streams, StreamConfigurationMode::NORMAL_MODE, &config,
                                      jpegBufferSize);
            config.streamConfigCounter = streamConfigCounter++;
            s = injectionSession->configureInjectionStreams(config, chars);
            ASSERT_EQ(static_cast<int32_t>(Status::ILLEGAL_ARGUMENT), s.getServiceSpecificError());
        }

        ret = session->close();
        ASSERT_TRUE(ret.isOk());
    }
}

// Check whether session parameters are supported for injection camera. If Hal support for them
// exist, then try to configure a preview stream using them.
TEST_P(CameraAidlTest, configureInjectionStreamsWithSessionParameters) {
    std::vector<std::string> cameraDeviceNames = getCameraDeviceNames(mProvider);
    std::vector<AvailableStream> outputPreviewStreams;
    AvailableStream previewThreshold = {kMaxPreviewWidth, kMaxPreviewHeight,
                                        static_cast<int32_t>(PixelFormat::IMPLEMENTATION_DEFINED)};

    for (const auto& name : cameraDeviceNames) {
        CameraMetadata metadata;
        std::shared_ptr<ICameraInjectionSession> injectionSession;
        std::shared_ptr<ICameraDevice> unusedDevice;
        openEmptyInjectionSession(name, mProvider, &injectionSession /*out*/, &metadata /*out*/,
                                  &unusedDevice /*out*/);
        if (injectionSession == nullptr) {
            continue;
        }

        std::shared_ptr<ICameraDeviceSession> session;
        ndk::ScopedAStatus ret = injectionSession->getCameraDeviceSession(&session);
        ASSERT_TRUE(ret.isOk());
        ASSERT_NE(session, nullptr);

        camera_metadata_t* staticMetaBuffer =
                reinterpret_cast<camera_metadata_t*>(metadata.metadata.data());
        CameraMetadata chars;
        chars.metadata = metadata.metadata;

        std::unordered_set<int32_t> availableSessionKeys;
        Status rc = getSupportedKeys(staticMetaBuffer, ANDROID_REQUEST_AVAILABLE_SESSION_KEYS,
                                     &availableSessionKeys);
        ASSERT_EQ(Status::OK, rc);
        if (availableSessionKeys.empty()) {
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
            ret = session->close();
            ASSERT_TRUE(ret.isOk());
            continue;
        }

        outputPreviewStreams.clear();

        ASSERT_EQ(Status::OK, getAvailableOutputStreams(staticMetaBuffer, outputPreviewStreams,
                                                        &previewThreshold));
        ASSERT_NE(0u, outputPreviewStreams.size());

        Stream previewStream = {
                0,
                StreamType::OUTPUT,
                outputPreviewStreams[0].width,
                outputPreviewStreams[0].height,
                static_cast<PixelFormat>(outputPreviewStreams[0].format),
                static_cast<::aidl::android::hardware::graphics::common::BufferUsage>(
                        GRALLOC1_CONSUMER_USAGE_HWCOMPOSER),
                Dataspace::UNKNOWN,
                StreamRotation::ROTATION_0,
                std::string(),
                0,
                -1,
                {SensorPixelMode::ANDROID_SENSOR_PIXEL_MODE_DEFAULT},
                RequestAvailableDynamicRangeProfilesMap::
                        ANDROID_REQUEST_AVAILABLE_DYNAMIC_RANGE_PROFILES_MAP_STANDARD};
        std::vector<Stream> streams = {previewStream};
        StreamConfiguration config;
        config.streams = streams;
        config.operationMode = StreamConfigurationMode::NORMAL_MODE;

        modifiedSessionParams = sessionParams;
        camera_metadata_t* sessionParamsBuffer = sessionParams.release();
        uint8_t* rawSessionParamsBuffer = reinterpret_cast<uint8_t*>(sessionParamsBuffer);
        config.sessionParams.metadata =
                std::vector(rawSessionParamsBuffer,
                            rawSessionParamsBuffer + get_camera_metadata_size(sessionParamsBuffer));

        config.streamConfigCounter = 0;
        config.streamConfigCounter = 0;
        config.multiResolutionInputImage = false;

        ndk::ScopedAStatus s = injectionSession->configureInjectionStreams(config, chars);
        ASSERT_TRUE(s.isOk());

        sessionParams.acquire(sessionParamsBuffer);
        free_camera_metadata(staticMetaBuffer);
        ret = session->close();
        ASSERT_TRUE(ret.isOk());
    }
}

// Verify that  valid stream use cases can be configured successfully, and invalid use cases
// fail stream configuration.
TEST_P(CameraAidlTest, configureStreamsUseCases) {
    std::vector<std::string> cameraDeviceNames = getCameraDeviceNames(mProvider);

    for (const auto& name : cameraDeviceNames) {
        CameraMetadata meta;
        std::shared_ptr<ICameraDevice> cameraDevice;

        openEmptyDeviceSession(name, mProvider, &mSession /*out*/, &meta /*out*/,
                               &cameraDevice /*out*/);

        camera_metadata_t* staticMeta = reinterpret_cast<camera_metadata_t*>(meta.metadata.data());
        // Check if camera support depth only
        if (isDepthOnly(staticMeta)) {
            ndk::ScopedAStatus ret = mSession->close();
            mSession = nullptr;
            ASSERT_TRUE(ret.isOk());
            continue;
        }

        std::vector<AvailableStream> outputPreviewStreams;
        AvailableStream previewThreshold = {kMaxPreviewWidth, kMaxPreviewHeight,
                                            static_cast<int32_t>(PixelFormat::YCBCR_420_888)};
        ASSERT_EQ(Status::OK,
                  getAvailableOutputStreams(staticMeta, outputPreviewStreams, &previewThreshold));
        ASSERT_NE(0u, outputPreviewStreams.size());

        // Combine valid and invalid stream use cases
        std::vector<int64_t> useCases(kMandatoryUseCases);
        useCases.push_back(ANDROID_SCALER_AVAILABLE_STREAM_USE_CASES_VIDEO_CALL + 1);

        std::vector<int64_t> supportedUseCases;
        camera_metadata_ro_entry entry;
        auto retcode = find_camera_metadata_ro_entry(
                staticMeta, ANDROID_SCALER_AVAILABLE_STREAM_USE_CASES, &entry);
        if ((0 == retcode) && (entry.count > 0)) {
            supportedUseCases.insert(supportedUseCases.end(), entry.data.i64,
                                     entry.data.i64 + entry.count);
        } else {
            supportedUseCases.push_back(ANDROID_SCALER_AVAILABLE_STREAM_USE_CASES_DEFAULT);
        }

        std::vector<Stream> streams(1);
        streams[0] = {0,
                      StreamType::OUTPUT,
                      outputPreviewStreams[0].width,
                      outputPreviewStreams[0].height,
                      static_cast<PixelFormat>(outputPreviewStreams[0].format),
                      static_cast<::aidl::android::hardware::graphics::common::BufferUsage>(
                              GRALLOC1_CONSUMER_USAGE_CPU_READ),
                      Dataspace::UNKNOWN,
                      StreamRotation::ROTATION_0,
                      std::string(),
                      0,
                      -1,
                      {SensorPixelMode::ANDROID_SENSOR_PIXEL_MODE_DEFAULT},
                      RequestAvailableDynamicRangeProfilesMap::
                              ANDROID_REQUEST_AVAILABLE_DYNAMIC_RANGE_PROFILES_MAP_STANDARD};

        int32_t streamConfigCounter = 0;
        CameraMetadata req;
        StreamConfiguration config;
        RequestTemplate reqTemplate = RequestTemplate::STILL_CAPTURE;
        ndk::ScopedAStatus ret = mSession->constructDefaultRequestSettings(reqTemplate, &req);
        ASSERT_TRUE(ret.isOk());
        config.sessionParams = req;

        for (int64_t useCase : useCases) {
            bool useCaseSupported = std::find(supportedUseCases.begin(), supportedUseCases.end(),
                                              useCase) != supportedUseCases.end();

            streams[0].useCase = static_cast<
                    aidl::android::hardware::camera::metadata::ScalerAvailableStreamUseCases>(
                    useCase);
            config.streams = streams;
            config.operationMode = StreamConfigurationMode::NORMAL_MODE;
            config.streamConfigCounter = streamConfigCounter;
            config.multiResolutionInputImage = false;

            bool combSupported;
            ret = cameraDevice->isStreamCombinationSupported(config, &combSupported);
            if (static_cast<int32_t>(Status::OPERATION_NOT_SUPPORTED) ==
                ret.getServiceSpecificError()) {
                continue;
            }

            ASSERT_TRUE(ret.isOk());
            ASSERT_EQ(combSupported, useCaseSupported);

            std::vector<HalStream> halStreams;
            ret = mSession->configureStreams(config, &halStreams);
            ALOGI("configureStreams returns status: %d", ret.getServiceSpecificError());
            if (useCaseSupported) {
                ASSERT_TRUE(ret.isOk());
                ASSERT_EQ(1u, halStreams.size());
            } else {
                ASSERT_EQ(static_cast<int32_t>(Status::ILLEGAL_ARGUMENT),
                          ret.getServiceSpecificError());
            }
        }
        ret = mSession->close();
        mSession = nullptr;
        ASSERT_TRUE(ret.isOk());
    }
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(CameraAidlTest);
INSTANTIATE_TEST_SUITE_P(
        PerInstance, CameraAidlTest,
        testing::ValuesIn(android::getAidlHalInstanceNames(ICameraProvider::descriptor)),
        android::hardware::PrintInstanceNameToString);