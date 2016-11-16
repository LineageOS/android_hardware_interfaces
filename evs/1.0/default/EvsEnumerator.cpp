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

#define LOG_TAG "android.hardware.evs@1.0-service"

#include "EvsEnumerator.h"
#include "EvsCamera.h"
#include "EvsDisplay.h"

namespace android {
namespace hardware {
namespace evs {
namespace V1_0 {
namespace implementation {


EvsEnumerator::EvsEnumerator() {
    ALOGD("EvsEnumerator created");

    // Add sample camera data to our list of cameras
    // NOTE:  The id strings trigger special initialization inside the EvsCamera constructor
    mCameraList.emplace_back( new EvsCamera(EvsCamera::kCameraName_Backup),    false );
    mCameraList.emplace_back( new EvsCamera("LaneView"),                       false );
    mCameraList.emplace_back( new EvsCamera(EvsCamera::kCameraName_RightTurn), false );
}

// Methods from ::android::hardware::evs::V1_0::IEvsEnumerator follow.
Return<void> EvsEnumerator::getCameraList(getCameraList_cb _hidl_cb)  {
    ALOGD("getCameraList");

    const unsigned numCameras = mCameraList.size();

    // Build up a packed array of CameraDesc for return
    // NOTE:  Only has to live until the callback returns
    std::vector<CameraDesc> descriptions;
    descriptions.reserve(numCameras);
    for (const auto& cam : mCameraList) {
        descriptions.push_back( cam.pCamera->getDesc() );
    }

    // Encapsulate our camera descriptions in the HIDL vec type
    hidl_vec<CameraDesc> hidlCameras(descriptions);

    // Send back the results
    ALOGD("reporting %zu cameras available", hidlCameras.size());
    _hidl_cb(hidlCameras);

    // HIDL convention says we return Void if we sent our result back via callback
    return Void();
}

Return<void> EvsEnumerator::openCamera(const hidl_string& cameraId,
                                             openCamera_cb callback) {
    ALOGD("openCamera");

    // Find the named camera
    CameraRecord *pRecord = nullptr;
    for (auto &&cam : mCameraList) {
        if (cam.pCamera->getDesc().cameraId == cameraId) {
            // Found a match!
            pRecord = &cam;
            break;
        }
    }

    if (!pRecord) {
        ALOGE("Requested camera %s not found", cameraId.c_str());
        callback(nullptr);
    }
    else if (pRecord->inUse) {
        ALOGE("Cannot open camera %s which is already in use", cameraId.c_str());
        callback(nullptr);
    }
    else {
        /* TODO(b/33492405):  Do this, When HIDL can give us back a recognizable pointer
        pRecord->inUse = true;
         */
        callback(pRecord->pCamera);
    }

    return Void();
}

Return<void> EvsEnumerator::closeCamera(const ::android::sp<IEvsCamera>& camera) {
    ALOGD("closeCamera");

    if (camera == nullptr) {
        ALOGE("Ignoring call to closeCamera with null camera pointer");
    }
    else {
        // Make sure the camera has stopped streaming
        camera->stopVideoStream();

        /* TODO(b/33492405):  Do this, When HIDL can give us back a recognizable pointer
        pRecord->inUse = false;
         */
    }

    return Void();
}

Return<void> EvsEnumerator::openDisplay(openDisplay_cb callback) {
    ALOGD("openDisplay");

    // If we already have a display active, then this request must be denied
    if (mActiveDisplay != nullptr) {
        ALOGW("Rejecting openDisplay request the display is already in use.");
        callback(nullptr);
    }
    else {
        // Create a new display interface and return it
        mActiveDisplay = new EvsDisplay();
        ALOGD("Returning new EvsDisplay object %p", mActiveDisplay.get());
        callback(mActiveDisplay);
    }

    return Void();
}

Return<void> EvsEnumerator::closeDisplay(const ::android::sp<IEvsDisplay>& display) {
    ALOGD("closeDisplay");

    if (mActiveDisplay == nullptr) {
        ALOGE("Ignoring closeDisplay when display is not active");
    }
    else if (display == nullptr) {
        ALOGE("Ignoring closeDisplay with null display pointer");
    }
    else {
        // Drop the active display
        // TODO(b/33492405):  When HIDL provides recognizable pointers, add validation here.
        mActiveDisplay = nullptr;
    }

    return Void();
}


// TODO(b/31632518):  Need to get notification when our client dies so we can close the camera.
// As possible work around would be to give the client a HIDL object to exclusively hold
// and use it's destructor to perform some work in the server side.


} // namespace implementation
} // namespace V1_0
} // namespace evs
} // namespace hardware
} // namespace android
