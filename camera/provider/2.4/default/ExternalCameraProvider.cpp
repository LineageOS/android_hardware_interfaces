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

#define LOG_TAG "CamPvdr@2.4-external"
//#define LOG_NDEBUG 0
#include <log/log.h>

#include <regex>
#include <sys/inotify.h>
#include <errno.h>
#include <linux/videodev2.h>
#include "ExternalCameraProvider.h"
#include "ExternalCameraDevice_3_4.h"

namespace android {
namespace hardware {
namespace camera {
namespace provider {
namespace V2_4 {
namespace implementation {

namespace {
// "device@<version>/external/<id>"
const std::regex kDeviceNameRE("device@([0-9]+\\.[0-9]+)/external/(.+)");
const int kMaxDevicePathLen = 256;
const char* kDevicePath = "/dev/";

bool matchDeviceName(const hidl_string& deviceName, std::string* deviceVersion,
                     std::string* cameraId) {
    std::string deviceNameStd(deviceName.c_str());
    std::smatch sm;
    if (std::regex_match(deviceNameStd, sm, kDeviceNameRE)) {
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

} // anonymous namespace

ExternalCameraProvider::ExternalCameraProvider() : mHotPlugThread(this) {
    mHotPlugThread.run("ExtCamHotPlug", PRIORITY_BACKGROUND);
}

ExternalCameraProvider::~ExternalCameraProvider() {
    mHotPlugThread.requestExit();
}


Return<Status> ExternalCameraProvider::setCallback(
        const sp<ICameraProviderCallback>& callback) {
    Mutex::Autolock _l(mLock);
    mCallbacks = callback;
    return Status::OK;
}

Return<void> ExternalCameraProvider::getVendorTags(getVendorTags_cb _hidl_cb) {
    // No vendor tag support for USB camera
    hidl_vec<VendorTagSection> zeroSections;
    _hidl_cb(Status::OK, zeroSections);
    return Void();
}

Return<void> ExternalCameraProvider::getCameraIdList(getCameraIdList_cb _hidl_cb) {
    std::vector<hidl_string> deviceNameList;
    for (auto const& kvPair : mCameraStatusMap) {
        if (kvPair.second == CameraDeviceStatus::PRESENT) {
            deviceNameList.push_back(kvPair.first);
        }
    }
    hidl_vec<hidl_string> hidlDeviceNameList(deviceNameList);
    ALOGV("ExtCam: number of cameras is %zu", deviceNameList.size());
    _hidl_cb(Status::OK, hidlDeviceNameList);
    return Void();
}

Return<void> ExternalCameraProvider::isSetTorchModeSupported(
        isSetTorchModeSupported_cb _hidl_cb) {
    // No torch mode support for USB camera
    _hidl_cb (Status::OK, false);
    return Void();
}

Return<void> ExternalCameraProvider::getCameraDeviceInterface_V1_x(
        const hidl_string&,
        getCameraDeviceInterface_V1_x_cb _hidl_cb) {
    // External Camera HAL does not support HAL1
    _hidl_cb(Status::OPERATION_NOT_SUPPORTED, nullptr);
    return Void();
}

Return<void> ExternalCameraProvider::getCameraDeviceInterface_V3_x(
        const hidl_string& cameraDeviceName,
        getCameraDeviceInterface_V3_x_cb _hidl_cb) {

    std::string cameraId, deviceVersion;
    bool match = matchDeviceName(cameraDeviceName, &deviceVersion, &cameraId);
    if (!match) {
        _hidl_cb(Status::ILLEGAL_ARGUMENT, nullptr);
        return Void();
    }

    if (mCameraStatusMap.count(cameraDeviceName) == 0 ||
            mCameraStatusMap[cameraDeviceName] != CameraDeviceStatus::PRESENT) {
        _hidl_cb(Status::ILLEGAL_ARGUMENT, nullptr);
        return Void();
    }

    ALOGV("Constructing v3.4 external camera device");
    sp<device::V3_2::ICameraDevice> device;
    sp<device::V3_4::implementation::ExternalCameraDevice> deviceImpl =
            new device::V3_4::implementation::ExternalCameraDevice(
                    cameraId);
    if (deviceImpl == nullptr || deviceImpl->isInitFailed()) {
        ALOGE("%s: camera device %s init failed!", __FUNCTION__, cameraId.c_str());
        device = nullptr;
        _hidl_cb(Status::INTERNAL_ERROR, nullptr);
        return Void();
    }
    device = deviceImpl;

    _hidl_cb (Status::OK, device);

    return Void();
}

void ExternalCameraProvider::addExternalCamera(const char* devName) {
    ALOGE("ExtCam: adding %s to External Camera HAL!", devName);
    Mutex::Autolock _l(mLock);
    std::string deviceName = std::string("device@3.4/external/") + devName;
    mCameraStatusMap[deviceName] = CameraDeviceStatus::PRESENT;
    if (mCallbacks != nullptr) {
        mCallbacks->cameraDeviceStatusChange(deviceName, CameraDeviceStatus::PRESENT);
    }
}

void ExternalCameraProvider::deviceAdded(const char* devName) {
    int fd = -1;
    if ((fd = ::open(devName, O_RDWR)) < 0) {
        ALOGE("%s open v4l2 device %s failed:%s", __FUNCTION__, devName, strerror(errno));
        return;
    }

    do {
        struct v4l2_capability capability;
        int ret = ioctl(fd, VIDIOC_QUERYCAP, &capability);
        if (ret < 0) {
            ALOGE("%s v4l2 QUERYCAP %s failed", __FUNCTION__, devName);
            break;
        }

        if (!(capability.device_caps & V4L2_CAP_VIDEO_CAPTURE)) {
            ALOGW("%s device %s does not support VIDEO_CAPTURE", __FUNCTION__, devName);
            break;
        }

        addExternalCamera(devName);
    } while (0);

    close(fd);
    return;
}

void ExternalCameraProvider::deviceRemoved(const char* devName) {
    Mutex::Autolock _l(mLock);
    std::string deviceName = std::string("device@3.4/external/") + devName;
    if (mCameraStatusMap.find(deviceName) != mCameraStatusMap.end()) {
        mCameraStatusMap.erase(deviceName);
        if (mCallbacks != nullptr) {
            mCallbacks->cameraDeviceStatusChange(deviceName, CameraDeviceStatus::NOT_PRESENT);
        }
    } else {
        ALOGE("%s: cannot find camera device %s", __FUNCTION__, devName);
    }
}

ExternalCameraProvider::HotplugThread::HotplugThread(ExternalCameraProvider* parent) :
        Thread(/*canCallJava*/false), mParent(parent) {}

ExternalCameraProvider::HotplugThread::~HotplugThread() {}

bool ExternalCameraProvider::HotplugThread::threadLoop() {
    // Find existing /dev/video* devices
    DIR* devdir = opendir(kDevicePath);
    if(devdir == 0) {
        ALOGE("%s: cannot open %s! Exiting threadloop", __FUNCTION__, kDevicePath);
        return false;
    }

    struct dirent* de;
    // This list is device dependent. TODO: b/72261897 allow setting it from setprop/device boot
    std::string internalDevices = "0,1";
    while ((de = readdir(devdir)) != 0) {
        // Find external v4l devices that's existing before we start watching and add them
        if (!strncmp("video", de->d_name, 5)) {
            // TODO: This might reject some valid devices. Ex: internal is 33 and a device named 3
            //       is added.
            if (internalDevices.find(de->d_name + 5) == std::string::npos) {
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
                    if (!strncmp("video", event->name, 5)) {
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
