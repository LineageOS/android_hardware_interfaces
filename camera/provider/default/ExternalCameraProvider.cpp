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

#define LOG_TAG "ExtCamPrvdr"
// #define LOG_NDEBUG 0

#include "ExternalCameraProvider.h"

#include <ExternalCameraDevice.h>
#include <aidl/android/hardware/camera/common/Status.h>
#include <convert.h>
#include <cutils/properties.h>
#include <linux/videodev2.h>
#include <log/log.h>
#include <sys/inotify.h>
#include <regex>

namespace android {
namespace hardware {
namespace camera {
namespace provider {
namespace implementation {

using ::aidl::android::hardware::camera::common::Status;
using ::android::hardware::camera::device::implementation::ExternalCameraDevice;
using ::android::hardware::camera::device::implementation::fromStatus;
using ::android::hardware::camera::external::common::ExternalCameraConfig;

namespace {
// "device@<version>/external/<id>"
const std::regex kDeviceNameRE("device@([0-9]+\\.[0-9]+)/external/(.+)");
const int kMaxDevicePathLen = 256;
constexpr char kDevicePath[] = "/dev/";
constexpr char kPrefix[] = "video";
constexpr int kPrefixLen = sizeof(kPrefix) - 1;
constexpr int kDevicePrefixLen = sizeof(kDevicePath) + kPrefixLen - 1;

bool matchDeviceName(int cameraIdOffset, const std::string& deviceName, std::string* deviceVersion,
                     std::string* cameraDevicePath) {
    std::smatch sm;
    if (std::regex_match(deviceName, sm, kDeviceNameRE)) {
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
}  // namespace

ExternalCameraProvider::ExternalCameraProvider() : mCfg(ExternalCameraConfig::loadFromCfg()) {
    mHotPlugThread = std::make_shared<HotplugThread>(this);
    mHotPlugThread->run();
}

ExternalCameraProvider::~ExternalCameraProvider() {
    mHotPlugThread->requestExitAndWait();
}

ndk::ScopedAStatus ExternalCameraProvider::setCallback(
        const std::shared_ptr<ICameraProviderCallback>& in_callback) {
    if (in_callback == nullptr) {
        return fromStatus(Status::ILLEGAL_ARGUMENT);
    }

    {
        Mutex::Autolock _l(mLock);
        mCallback = in_callback;
    }

    for (const auto& pair : mCameraStatusMap) {
        mCallback->cameraDeviceStatusChange(pair.first, pair.second);
    }
    return fromStatus(Status::OK);
}

ndk::ScopedAStatus ExternalCameraProvider::getVendorTags(
        std::vector<VendorTagSection>* _aidl_return) {
    if (_aidl_return == nullptr) {
        return fromStatus(Status::ILLEGAL_ARGUMENT);
    }
    // No vendor tag support for USB camera
    *_aidl_return = {};
    return fromStatus(Status::OK);
}

ndk::ScopedAStatus ExternalCameraProvider::getCameraIdList(std::vector<std::string>* _aidl_return) {
    if (_aidl_return == nullptr) {
        return fromStatus(Status::ILLEGAL_ARGUMENT);
    }
    // External camera HAL always report 0 camera, and extra cameras
    // are just reported via cameraDeviceStatusChange callbacks
    *_aidl_return = {};
    return fromStatus(Status::OK);
}

ndk::ScopedAStatus ExternalCameraProvider::getCameraDeviceInterface(
        const std::string& in_cameraDeviceName, std::shared_ptr<ICameraDevice>* _aidl_return) {
    if (_aidl_return == nullptr) {
        return fromStatus(Status::ILLEGAL_ARGUMENT);
    }
    std::string cameraDevicePath, deviceVersion;
    bool match = matchDeviceName(mCfg.cameraIdOffset, in_cameraDeviceName, &deviceVersion,
                                 &cameraDevicePath);

    if (!match) {
        *_aidl_return = nullptr;
        return fromStatus(Status::ILLEGAL_ARGUMENT);
    }

    if (mCameraStatusMap.count(in_cameraDeviceName) == 0 ||
        mCameraStatusMap[in_cameraDeviceName] != CameraDeviceStatus::PRESENT) {
        *_aidl_return = nullptr;
        return fromStatus(Status::ILLEGAL_ARGUMENT);
    }

    ALOGV("Constructing external camera device");
    std::shared_ptr<ExternalCameraDevice> deviceImpl =
            ndk::SharedRefBase::make<ExternalCameraDevice>(cameraDevicePath, mCfg);
    if (deviceImpl == nullptr || deviceImpl->isInitFailed()) {
        ALOGE("%s: camera device %s init failed!", __FUNCTION__, cameraDevicePath.c_str());
        *_aidl_return = nullptr;
        return fromStatus(Status::INTERNAL_ERROR);
    }

    IF_ALOGV() {
        int interfaceVersion;
        deviceImpl->getInterfaceVersion(&interfaceVersion);
        ALOGV("%s: device interface version: %d", __FUNCTION__, interfaceVersion);
    }

    *_aidl_return = deviceImpl;
    return fromStatus(Status::OK);
}

ndk::ScopedAStatus ExternalCameraProvider::notifyDeviceStateChange(int64_t) {
    return fromStatus(Status::OK);
}

ndk::ScopedAStatus ExternalCameraProvider::getConcurrentCameraIds(
        std::vector<ConcurrentCameraIdCombination>* _aidl_return) {
    if (_aidl_return == nullptr) {
        return fromStatus(Status::ILLEGAL_ARGUMENT);
    }
    *_aidl_return = {};
    return fromStatus(Status::OK);
}

ndk::ScopedAStatus ExternalCameraProvider::isConcurrentStreamCombinationSupported(
        const std::vector<CameraIdAndStreamCombination>&, bool* _aidl_return) {
    if (_aidl_return == nullptr) {
        return fromStatus(Status::ILLEGAL_ARGUMENT);
    }
    // No concurrent stream combinations are supported
    *_aidl_return = false;
    return fromStatus(Status::OK);
}

void ExternalCameraProvider::addExternalCamera(const char* devName) {
    ALOGV("%s: ExtCam: adding %s to External Camera HAL!", __FUNCTION__, devName);
    Mutex::Autolock _l(mLock);
    std::string deviceName;
    std::string cameraId =
            std::to_string(mCfg.cameraIdOffset + std::atoi(devName + kDevicePrefixLen));
    deviceName =
            std::string("device@") + ExternalCameraDevice::kDeviceVersion + "/external/" + cameraId;
    mCameraStatusMap[deviceName] = CameraDeviceStatus::PRESENT;
    if (mCallback != nullptr) {
        mCallback->cameraDeviceStatusChange(deviceName, CameraDeviceStatus::PRESENT);
    }
}

void ExternalCameraProvider::deviceAdded(const char* devName) {
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
    std::shared_ptr<ExternalCameraDevice> deviceImpl =
            ndk::SharedRefBase::make<ExternalCameraDevice>(devName, mCfg);
    if (deviceImpl == nullptr || deviceImpl->isInitFailed()) {
        ALOGW("%s: Attempt to init camera device %s failed!", __FUNCTION__, devName);
        return;
    }
    deviceImpl.reset();
    addExternalCamera(devName);
}

void ExternalCameraProvider::deviceRemoved(const char* devName) {
    Mutex::Autolock _l(mLock);
    std::string deviceName;
    std::string cameraId =
            std::to_string(mCfg.cameraIdOffset + std::atoi(devName + kDevicePrefixLen));

    deviceName =
            std::string("device@") + ExternalCameraDevice::kDeviceVersion + "/external/" + cameraId;

    if (mCameraStatusMap.erase(deviceName) == 0) {
        // Unknown device, do not fire callback
        ALOGE("%s: cannot find camera device to remove %s", __FUNCTION__, devName);
        return;
    }

    if (mCallback != nullptr) {
        mCallback->cameraDeviceStatusChange(deviceName, CameraDeviceStatus::NOT_PRESENT);
    }
}

void ExternalCameraProvider::updateAttachedCameras() {
    ALOGV("%s start scanning for existing V4L2 devices", __FUNCTION__);

    // Find existing /dev/video* devices
    DIR* devdir = opendir(kDevicePath);
    if (devdir == nullptr) {
        ALOGE("%s: cannot open %s! Exiting threadloop", __FUNCTION__, kDevicePath);
        return;
    }

    struct dirent* de;
    while ((de = readdir(devdir)) != nullptr) {
        // Find external v4l devices that's existing before we start watching and add them
        if (!strncmp(kPrefix, de->d_name, kPrefixLen)) {
            std::string deviceId(de->d_name + kPrefixLen);
            if (mCfg.mInternalDevices.count(deviceId) == 0) {
                ALOGV("Non-internal v4l device %s found", de->d_name);
                char v4l2DevicePath[kMaxDevicePathLen];
                snprintf(v4l2DevicePath, kMaxDevicePathLen, "%s%s", kDevicePath, de->d_name);
                deviceAdded(v4l2DevicePath);
            }
        }
    }
    closedir(devdir);
}

// Start ExternalCameraProvider::HotplugThread functions

ExternalCameraProvider::HotplugThread::HotplugThread(ExternalCameraProvider* parent)
    : mParent(parent), mInternalDevices(parent->mCfg.mInternalDevices) {}

ExternalCameraProvider::HotplugThread::~HotplugThread() {
    // Clean up inotify descriptor if needed.
    if (mINotifyFD >= 0) {
        close(mINotifyFD);
    }
}

bool ExternalCameraProvider::HotplugThread::initialize() {
    // Update existing cameras
    mParent->updateAttachedCameras();

    // Set up non-blocking fd. The threadLoop will be responsible for polling read at the
    // desired frequency
    mINotifyFD = inotify_init();
    if (mINotifyFD < 0) {
        ALOGE("%s: inotify init failed! Exiting threadloop", __FUNCTION__);
        return false;
    }

    // Start watching /dev/ directory for created and deleted files
    mWd = inotify_add_watch(mINotifyFD, kDevicePath, IN_CREATE | IN_DELETE);
    if (mWd < 0) {
        ALOGE("%s: inotify add watch failed! Exiting threadloop", __FUNCTION__);
        return false;
    }

    mPollFd = {.fd = mINotifyFD, .events = POLLIN};

    mIsInitialized = true;
    return true;
}

bool ExternalCameraProvider::HotplugThread::threadLoop() {
    // Initialize inotify descriptors if needed.
    if (!mIsInitialized && !initialize()) {
        return true;
    }

    // poll /dev/* and handle timeouts and error
    int pollRet = poll(&mPollFd, /* fd_count= */ 1, /* timeout= */ 250);
    if (pollRet == 0) {
        // no read event in 100ms
        mPollFd.revents = 0;
        return true;
    } else if (pollRet < 0) {
        ALOGE("%s: error while polling for /dev/*: %d", __FUNCTION__, errno);
        mPollFd.revents = 0;
        return true;
    } else if (mPollFd.revents & POLLERR) {
        ALOGE("%s: polling /dev/ returned POLLERR", __FUNCTION__);
        mPollFd.revents = 0;
        return true;
    } else if (mPollFd.revents & POLLHUP) {
        ALOGE("%s: polling /dev/ returned POLLHUP", __FUNCTION__);
        mPollFd.revents = 0;
        return true;
    } else if (mPollFd.revents & POLLNVAL) {
        ALOGE("%s: polling /dev/ returned POLLNVAL", __FUNCTION__);
        mPollFd.revents = 0;
        return true;
    }
    // mPollFd.revents must contain POLLIN, so safe to reset it before reading
    mPollFd.revents = 0;

    uint64_t offset = 0;
    ssize_t ret = read(mINotifyFD, mEventBuf, sizeof(mEventBuf));
    if (ret < sizeof(struct inotify_event)) {
        // invalid event. skip
        return true;
    }

    while (offset < ret) {
        struct inotify_event* event = (struct inotify_event*)&mEventBuf[offset];
        offset += sizeof(struct inotify_event) + event->len;

        if (event->wd != mWd) {
            // event for an unrelated descriptor. ignore.
            continue;
        }

        ALOGV("%s inotify_event %s", __FUNCTION__, event->name);
        if (strncmp(kPrefix, event->name, kPrefixLen) != 0) {
            // event not for /dev/video*. ignore.
            continue;
        }

        std::string deviceId = event->name + kPrefixLen;
        if (mInternalDevices.count(deviceId) != 0) {
            // update to an internal device. ignore.
            continue;
        }

        char v4l2DevicePath[kMaxDevicePathLen];
        snprintf(v4l2DevicePath, kMaxDevicePathLen, "%s%s", kDevicePath, event->name);

        if (event->mask & IN_CREATE) {
            mParent->deviceAdded(v4l2DevicePath);
        } else if (event->mask & IN_DELETE) {
            mParent->deviceRemoved(v4l2DevicePath);
        }
    }
    return true;
}

// End ExternalCameraProvider::HotplugThread functions

}  // namespace implementation
}  // namespace provider
}  // namespace camera
}  // namespace hardware
}  // namespace android
