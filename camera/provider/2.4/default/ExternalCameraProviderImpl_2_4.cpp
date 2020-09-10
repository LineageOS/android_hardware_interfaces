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

#define LOG_TAG "CamPrvdr@2.4-external"
//#define LOG_NDEBUG 0
#include <log/log.h>

#include <regex>
#include <sys/inotify.h>
#include <errno.h>
#include <linux/videodev2.h>
#include <cutils/properties.h>
#include "ExternalCameraProviderImpl_2_4.h"
#include "ExternalCameraDevice_3_4.h"
#include "ExternalCameraDevice_3_5.h"
#include "ExternalCameraDevice_3_6.h"

namespace android {
namespace hardware {
namespace camera {
namespace provider {
namespace V2_4 {
namespace implementation {

template struct CameraProvider<ExternalCameraProviderImpl_2_4>;

namespace {
// "device@<version>/external/<id>"
const std::regex kDeviceNameRE("device@([0-9]+\\.[0-9]+)/external/(.+)");
const int kMaxDevicePathLen = 256;
const char* kDevicePath = "/dev/";
constexpr char kPrefix[] = "video";
constexpr int kPrefixLen = sizeof(kPrefix) - 1;
constexpr int kDevicePrefixLen = sizeof(kDevicePath) + kPrefixLen + 1;

bool matchDeviceName(int cameraIdOffset,
                     const hidl_string& deviceName, std::string* deviceVersion,
                     std::string* cameraDevicePath) {
    std::string deviceNameStd(deviceName.c_str());
    std::smatch sm;
    if (std::regex_match(deviceNameStd, sm, kDeviceNameRE)) {
        if (deviceVersion != nullptr) {
            *deviceVersion = sm[1];
        }
        if (cameraDevicePath != nullptr) {
            *cameraDevicePath = "/dev/video" + std::to_string(std::stoi(sm[2]) - cameraIdOffset);
        }
        return true;
    }
    return false;
}

} // anonymous namespace

ExternalCameraProviderImpl_2_4::ExternalCameraProviderImpl_2_4() :
        mCfg(ExternalCameraConfig::loadFromCfg()),
        mHotPlugThread(this) {
    mHotPlugThread.run("ExtCamHotPlug", PRIORITY_BACKGROUND);

    mPreferredHal3MinorVersion =
        property_get_int32("ro.vendor.camera.external.hal3TrebleMinorVersion", 4);
    ALOGV("Preferred HAL 3 minor version is %d", mPreferredHal3MinorVersion);
    switch(mPreferredHal3MinorVersion) {
        case 4:
        case 5:
        case 6:
            // OK
            break;
        default:
            ALOGW("Unknown minor camera device HAL version %d in property "
                    "'camera.external.hal3TrebleMinorVersion', defaulting to 4",
                    mPreferredHal3MinorVersion);
            mPreferredHal3MinorVersion = 4;
    }
}

ExternalCameraProviderImpl_2_4::~ExternalCameraProviderImpl_2_4() {
    mHotPlugThread.requestExit();
}


Return<Status> ExternalCameraProviderImpl_2_4::setCallback(
        const sp<ICameraProviderCallback>& callback) {
    {
        Mutex::Autolock _l(mLock);
        mCallbacks = callback;
    }
    if (mCallbacks == nullptr) {
        return Status::OK;
    }
    // Send a callback for all devices to initialize
    {
        for (const auto& pair : mCameraStatusMap) {
            mCallbacks->cameraDeviceStatusChange(pair.first, pair.second);
        }
    }

    return Status::OK;
}

Return<void> ExternalCameraProviderImpl_2_4::getVendorTags(
        ICameraProvider::getVendorTags_cb _hidl_cb) {
    // No vendor tag support for USB camera
    hidl_vec<VendorTagSection> zeroSections;
    _hidl_cb(Status::OK, zeroSections);
    return Void();
}

Return<void> ExternalCameraProviderImpl_2_4::getCameraIdList(
        ICameraProvider::getCameraIdList_cb _hidl_cb) {
    // External camera HAL always report 0 camera, and extra cameras
    // are just reported via cameraDeviceStatusChange callbacks
    hidl_vec<hidl_string> hidlDeviceNameList;
    _hidl_cb(Status::OK, hidlDeviceNameList);
    return Void();
}

Return<void> ExternalCameraProviderImpl_2_4::isSetTorchModeSupported(
        ICameraProvider::isSetTorchModeSupported_cb _hidl_cb) {
    // setTorchMode API is supported, though right now no external camera device
    // has a flash unit.
    _hidl_cb (Status::OK, true);
    return Void();
}

Return<void> ExternalCameraProviderImpl_2_4::getCameraDeviceInterface_V1_x(
        const hidl_string&,
        ICameraProvider::getCameraDeviceInterface_V1_x_cb _hidl_cb) {
    // External Camera HAL does not support HAL1
    _hidl_cb(Status::OPERATION_NOT_SUPPORTED, nullptr);
    return Void();
}

Return<void> ExternalCameraProviderImpl_2_4::getCameraDeviceInterface_V3_x(
        const hidl_string& cameraDeviceName,
        ICameraProvider::getCameraDeviceInterface_V3_x_cb _hidl_cb) {

    std::string cameraDevicePath, deviceVersion;
    bool match = matchDeviceName(mCfg.cameraIdOffset, cameraDeviceName,
                                 &deviceVersion, &cameraDevicePath);
    if (!match) {
        _hidl_cb(Status::ILLEGAL_ARGUMENT, nullptr);
        return Void();
    }

    if (mCameraStatusMap.count(cameraDeviceName) == 0 ||
            mCameraStatusMap[cameraDeviceName] != CameraDeviceStatus::PRESENT) {
        _hidl_cb(Status::ILLEGAL_ARGUMENT, nullptr);
        return Void();
    }

    sp<device::V3_4::implementation::ExternalCameraDevice> deviceImpl;
    switch (mPreferredHal3MinorVersion) {
        case 4: {
            ALOGV("Constructing v3.4 external camera device");
            deviceImpl = new device::V3_4::implementation::ExternalCameraDevice(
                    cameraDevicePath, mCfg);
            break;
        }
        case 5: {
            ALOGV("Constructing v3.5 external camera device");
            deviceImpl = new device::V3_5::implementation::ExternalCameraDevice(
                    cameraDevicePath, mCfg);
            break;
        }
        case 6: {
            ALOGV("Constructing v3.6 external camera device");
            deviceImpl = new device::V3_6::implementation::ExternalCameraDevice(
                    cameraDevicePath, mCfg);
            break;
        }
        default:
            ALOGE("%s: Unknown HAL minor version %d!", __FUNCTION__, mPreferredHal3MinorVersion);
            _hidl_cb(Status::INTERNAL_ERROR, nullptr);
            return Void();
    }

    if (deviceImpl == nullptr || deviceImpl->isInitFailed()) {
        ALOGE("%s: camera device %s init failed!", __FUNCTION__, cameraDevicePath.c_str());
        _hidl_cb(Status::INTERNAL_ERROR, nullptr);
        return Void();
    }

    IF_ALOGV() {
        deviceImpl->getInterface()->interfaceChain([](
            ::android::hardware::hidl_vec<::android::hardware::hidl_string> interfaceChain) {
                ALOGV("Device interface chain:");
                for (auto iface : interfaceChain) {
                    ALOGV("  %s", iface.c_str());
                }
            });
    }

    _hidl_cb (Status::OK, deviceImpl->getInterface());

    return Void();
}

void ExternalCameraProviderImpl_2_4::addExternalCamera(const char* devName) {
    ALOGI("ExtCam: adding %s to External Camera HAL!", devName);
    Mutex::Autolock _l(mLock);
    std::string deviceName;
    std::string cameraId = std::to_string(mCfg.cameraIdOffset +
                                          std::atoi(devName + kDevicePrefixLen));
    if (mPreferredHal3MinorVersion == 6) {
        deviceName = std::string("device@3.6/external/") + cameraId;
    } else if (mPreferredHal3MinorVersion == 5) {
        deviceName = std::string("device@3.5/external/") + cameraId;
    } else {
        deviceName = std::string("device@3.4/external/") + cameraId;
    }
    mCameraStatusMap[deviceName] = CameraDeviceStatus::PRESENT;
    if (mCallbacks != nullptr) {
        mCallbacks->cameraDeviceStatusChange(deviceName, CameraDeviceStatus::PRESENT);
    }
}

void ExternalCameraProviderImpl_2_4::deviceAdded(const char* devName) {
    {
        base::unique_fd fd(::open(devName, O_RDWR));
        if (fd.get() < 0) {
            ALOGE("%s open v4l2 device %s failed:%s", __FUNCTION__, devName, strerror(errno));
            return;
        }

        struct v4l2_capability capability;
        int ret = ioctl(fd.get(), VIDIOC_QUERYCAP, &capability);
        if (ret < 0) {
            ALOGE("%s v4l2 QUERYCAP %s failed", __FUNCTION__, devName);
            return;
        }

        if (!(capability.device_caps & V4L2_CAP_VIDEO_CAPTURE)) {
            ALOGW("%s device %s does not support VIDEO_CAPTURE", __FUNCTION__, devName);
            return;
        }
    }
    // See if we can initialize ExternalCameraDevice correctly
    sp<device::V3_4::implementation::ExternalCameraDevice> deviceImpl =
            new device::V3_4::implementation::ExternalCameraDevice(devName, mCfg);
    if (deviceImpl == nullptr || deviceImpl->isInitFailed()) {
        ALOGW("%s: Attempt to init camera device %s failed!", __FUNCTION__, devName);
        return;
    }
    deviceImpl.clear();

    addExternalCamera(devName);
    return;
}

void ExternalCameraProviderImpl_2_4::deviceRemoved(const char* devName) {
    Mutex::Autolock _l(mLock);
    std::string deviceName;
    std::string cameraId = std::to_string(mCfg.cameraIdOffset +
                                          std::atoi(devName + kDevicePrefixLen));
    if (mPreferredHal3MinorVersion == 6) {
        deviceName = std::string("device@3.6/external/") + cameraId;
    } else if (mPreferredHal3MinorVersion == 5) {
        deviceName = std::string("device@3.5/external/") + cameraId;
    } else {
        deviceName = std::string("device@3.4/external/") + cameraId;
    }
    if (mCameraStatusMap.find(deviceName) != mCameraStatusMap.end()) {
        mCameraStatusMap.erase(deviceName);
        if (mCallbacks != nullptr) {
            mCallbacks->cameraDeviceStatusChange(deviceName, CameraDeviceStatus::NOT_PRESENT);
        }
    } else {
        ALOGE("%s: cannot find camera device %s", __FUNCTION__, devName);
    }
}

ExternalCameraProviderImpl_2_4::HotplugThread::HotplugThread(
        ExternalCameraProviderImpl_2_4* parent) :
        Thread(/*canCallJava*/false),
        mParent(parent),
        mInternalDevices(parent->mCfg.mInternalDevices) {}

ExternalCameraProviderImpl_2_4::HotplugThread::~HotplugThread() {}

bool ExternalCameraProviderImpl_2_4::HotplugThread::threadLoop() {
    // Find existing /dev/video* devices
    DIR* devdir = opendir(kDevicePath);
    if(devdir == 0) {
        ALOGE("%s: cannot open %s! Exiting threadloop", __FUNCTION__, kDevicePath);
        return false;
    }

    struct dirent* de;
    while ((de = readdir(devdir)) != 0) {
        // Find external v4l devices that's existing before we start watching and add them
        if (!strncmp(kPrefix, de->d_name, kPrefixLen)) {
            // TODO: This might reject some valid devices. Ex: internal is 33 and a device named 3
            //       is added.
            std::string deviceId(de->d_name + kPrefixLen);
            if (mInternalDevices.count(deviceId) == 0) {
                ALOGV("Non-internal v4l device %s found", de->d_name);
                char v4l2DevicePath[kMaxDevicePathLen];
                snprintf(v4l2DevicePath, kMaxDevicePathLen,
                        "%s%s", kDevicePath, de->d_name);
                mParent->deviceAdded(v4l2DevicePath);
            }
        }
    }
    closedir(devdir);

    // Watch new video devices
    mINotifyFD = inotify_init();
    if (mINotifyFD < 0) {
        ALOGE("%s: inotify init failed! Exiting threadloop", __FUNCTION__);
        return true;
    }

    mWd = inotify_add_watch(mINotifyFD, kDevicePath, IN_CREATE | IN_DELETE);
    if (mWd < 0) {
        ALOGE("%s: inotify add watch failed! Exiting threadloop", __FUNCTION__);
        return true;
    }

    ALOGI("%s start monitoring new V4L2 devices", __FUNCTION__);

    bool done = false;
    char eventBuf[512];
    while (!done) {
        int offset = 0;
        int ret = read(mINotifyFD, eventBuf, sizeof(eventBuf));
        if (ret >= (int)sizeof(struct inotify_event)) {
            while (offset < ret) {
                struct inotify_event* event = (struct inotify_event*)&eventBuf[offset];
                if (event->wd == mWd) {
                    if (!strncmp(kPrefix, event->name, kPrefixLen)) {
                        std::string deviceId(event->name + kPrefixLen);
                        if (mInternalDevices.count(deviceId) == 0) {
                            char v4l2DevicePath[kMaxDevicePathLen];
                            snprintf(v4l2DevicePath, kMaxDevicePathLen,
                                    "%s%s", kDevicePath, event->name);
                            if (event->mask & IN_CREATE) {
                                mParent->deviceAdded(v4l2DevicePath);
                            }
                            if (event->mask & IN_DELETE) {
                                mParent->deviceRemoved(v4l2DevicePath);
                            }
                        }
                    }
                }
                offset += sizeof(struct inotify_event) + event->len;
            }
        }
    }

    return true;
}

}  // namespace implementation
}  // namespace V2_4
}  // namespace provider
}  // namespace camera
}  // namespace hardware
}  // namespace android
