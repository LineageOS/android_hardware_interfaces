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
#include <android/log.h>
#include <gtest/gtest.h>
#include <regex>
#include "system/camera_metadata.h"

using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::hidl_handle;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::sp;
using ::android::hardware::camera::common::V1_0::Status;
using ::android::hardware::camera::common::V1_0::CameraDeviceStatus;
using ::android::hardware::camera::common::V1_0::TorchMode;
using ::android::hardware::camera::common::V1_0::TorchModeStatus;
using ::android::hardware::camera::provider::V2_4::ICameraProvider;
using ::android::hardware::camera::provider::V2_4::ICameraProviderCallback;
using ::android::hardware::camera::device::V3_2::CaptureRequest;
using ::android::hardware::camera::device::V3_2::CaptureResult;
using ::android::hardware::camera::device::V3_2::ICameraDeviceCallback;
using ::android::hardware::camera::device::V3_2::ICameraDeviceSession;
using ::android::hardware::camera::device::V3_2::NotifyMsg;
using ::android::hardware::camera::device::V3_2::RequestTemplate;

#define CAMERA_PASSTHROUGH_SERVICE_NAME "legacy/0"

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
        if (sm[1].compare(kHAL3_2) == 0) {
            // maybe switched to 3.4 or define the hidl version enumlater
            return CAMERA_DEVICE_API_VERSION_3_2;
        } else if (sm[1].compare(kHAL1_0) == 0) {
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
    mProvider = ICameraProvider::getService(CAMERA_PASSTHROUGH_SERVICE_NAME, true);
    // TODO: handle the device doesn't have any camera case
    ALOGI_IF(mProvider, "provider is not nullptr, %p", mProvider.get());
    ASSERT_NE(mProvider, nullptr);
}

void CameraHidlEnvironment::TearDown() {
    ALOGI("TearDown CameraHidlEnvironment");
}

// The main test class for camera HIDL HAL.
class CameraHidlTest : public ::testing::Test {
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
};

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
    // TODO: right now no callbacks are fired because there is no external camera
    //       or torch mode change. Need to test torch API in CameraDevice test later.
}

// Test if ICameraProvider::getCameraDeviceInterface_V3_x returns Status::OK and non-null device
TEST_F(CameraHidlTest, getCameraDeviceInterface_V3_x) {
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
        }
    }
}

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
        }
    }
}

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

            Status status = device3_2->setTorchMode(TorchMode::ON);
            ALOGI("setTorchMode return status %d", (int)status);
            if (!torchControlSupported) {
                ASSERT_EQ(Status::METHOD_NOT_SUPPORTED, status);
            } else {
                ASSERT_TRUE(status == Status::OK || status == Status::OPERATION_NOT_SUPPORTED);
                if (status == Status::OK) {
                    status = device3_2->setTorchMode(TorchMode::OFF);
                    ASSERT_EQ(Status::OK, status);
                }
            }
        }
    }
}

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
            raw_handle->data[0] = 1; // std out
            hidl_handle handle = raw_handle;
            device3_2->dumpState(handle);
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
            raw_handle->data[0] = 1; // std out
            hidl_handle handle = raw_handle;
            device3_2->dumpState(handle);
            native_handle_delete(raw_handle);

            session->close();
            // TODO: test all session API calls return INTERNAL_ERROR after close
            // TODO: keep a wp copy here and verify session cannot be promoted out of this scope
        }
    }
}

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

TEST_F(CameraHidlTest, configureStreams) {
    CameraHidlEnvironment* env = CameraHidlEnvironment::Instance();
    hidl_vec<hidl_string> cameraDeviceNames = getCameraDeviceNames();

    for (const auto& name : cameraDeviceNames) {
        if (getCameraDeviceVersion(name) == CAMERA_DEVICE_API_VERSION_3_2) {
            ::android::sp<::android::hardware::camera::device::V3_2::ICameraDevice> device3_2;
            ALOGI("configureStreams: Testing camera device %s", name.c_str());
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


            session->close();
        }
    }
}

int main(int argc, char **argv) {
  ::testing::AddGlobalTestEnvironment(CameraHidlEnvironment::Instance());
  ::testing::InitGoogleTest(&argc, argv);
  int status = RUN_ALL_TESTS();
  ALOGI("Test result = %d", status);
  return status;
}
