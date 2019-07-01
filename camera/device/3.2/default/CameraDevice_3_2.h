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

#ifndef ANDROID_HARDWARE_CAMERA_DEVICE_V3_2_CAMERADEVICE_H
#define ANDROID_HARDWARE_CAMERA_DEVICE_V3_2_CAMERADEVICE_H

#include "utils/Mutex.h"
#include "CameraModule.h"
#include "CameraMetadata.h"
#include "CameraDeviceSession.h"

#include <android/hardware/camera/device/3.2/ICameraDevice.h>
#include <hidl/Status.h>
#include <hidl/MQDescriptor.h>

namespace android {
namespace hardware {
namespace camera {
namespace device {
namespace V3_2 {
namespace implementation {

using ::android::hardware::camera::device::V3_2::RequestTemplate;
using ::android::hardware::camera::device::V3_2::ICameraDevice;
using ::android::hardware::camera::device::V3_2::ICameraDeviceCallback;
using ::android::hardware::camera::device::V3_2::ICameraDeviceSession;
using ::android::hardware::camera::common::V1_0::CameraResourceCost;
using ::android::hardware::camera::common::V1_0::Status;
using ::android::hardware::camera::common::V1_0::TorchMode;
using ::android::hardware::camera::common::V1_0::helper::CameraModule;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::hidl_vec;
using ::android::hardware::hidl_string;
using ::android::sp;
using ::android::Mutex;

/*
 * The camera device HAL implementation is opened lazily (via the open call)
 */
struct CameraDevice : public virtual RefBase {
    // Called by provider HAL. Provider HAL must ensure the uniqueness of
    // CameraDevice object per cameraId, or there could be multiple CameraDevice
    // trying to access the same physical camera.
    // Also, provider will have to keep track of all CameraDevice objects in
    // order to notify CameraDevice when the underlying camera is detached
    CameraDevice(sp<CameraModule> module,
                 const std::string& cameraId,
                 const SortedVector<std::pair<std::string, std::string>>& cameraDeviceNames);
    virtual ~CameraDevice();

    // Retrieve the HIDL interface, split into its own class to avoid inheritance issues when
    // dealing with minor version revs and simultaneous implementation and interface inheritance
    virtual sp<ICameraDevice> getInterface() {
        return new TrampolineDeviceInterface_3_2(this);
    }

    // Caller must use this method to check if CameraDevice ctor failed
    bool isInitFailed() { return mInitFail; }
    // Used by provider HAL to signal external camera disconnected
    void setConnectionStatus(bool connected);

    /* Methods from ::android::hardware::camera::device::V3_2::ICameraDevice follow. */
    // The following method can be called without opening the actual camera device
    Return<void> getResourceCost(ICameraDevice::getResourceCost_cb _hidl_cb);
    Return<void> getCameraCharacteristics(ICameraDevice::getCameraCharacteristics_cb _hidl_cb);
    Return<Status> setTorchMode(TorchMode mode);

    // Open the device HAL and also return a default capture session
    Return<void> open(const sp<ICameraDeviceCallback>& callback, ICameraDevice::open_cb _hidl_cb);


    // Forward the dump call to the opened session, or do nothing
    Return<void> dumpState(const ::android::hardware::hidl_handle& fd);
    /* End of Methods from ::android::hardware::camera::device::V3_2::ICameraDevice */

protected:

    // Overridden by child implementations for returning different versions of CameraDeviceSession
    virtual sp<CameraDeviceSession> createSession(camera3_device_t*,
            const camera_metadata_t* deviceInfo,
            const sp<ICameraDeviceCallback>&);

    const sp<CameraModule> mModule;
    const std::string mCameraId;
    // const after ctor
    int   mCameraIdInt;
    int   mDeviceVersion;
    bool  mInitFail = false;
    // Set by provider (when external camera is connected/disconnected)
    bool  mDisconnected;
    wp<CameraDeviceSession> mSession = nullptr;

    const SortedVector<std::pair<std::string, std::string>>& mCameraDeviceNames;

    // gating access to mSession and mDisconnected
    mutable Mutex mLock;

    // convert conventional HAL status to HIDL Status
    static Status getHidlStatus(int);

    Status initStatus() const;

private:
    struct TrampolineDeviceInterface_3_2 : public ICameraDevice {
        TrampolineDeviceInterface_3_2(sp<CameraDevice> parent) :
            mParent(parent) {}

        virtual Return<void> getResourceCost(V3_2::ICameraDevice::getResourceCost_cb _hidl_cb)
                override {
            return mParent->getResourceCost(_hidl_cb);
        }

        virtual Return<void> getCameraCharacteristics(
                V3_2::ICameraDevice::getCameraCharacteristics_cb _hidl_cb) override {
            return mParent->getCameraCharacteristics(_hidl_cb);
        }

        virtual Return<Status> setTorchMode(TorchMode mode) override {
            return mParent->setTorchMode(mode);
        }

        virtual Return<void> open(const sp<V3_2::ICameraDeviceCallback>& callback,
                V3_2::ICameraDevice::open_cb _hidl_cb) override {
            return mParent->open(callback, _hidl_cb);
        }

        virtual Return<void> dumpState(const hidl_handle& fd) override {
            return mParent->dumpState(fd);
        }

    private:
        sp<CameraDevice> mParent;
    };

};

}  // namespace implementation
}  // namespace V3_2
}  // namespace device
}  // namespace camera
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_CAMERA_DEVICE_V3_2_CAMERADEVICE_H
