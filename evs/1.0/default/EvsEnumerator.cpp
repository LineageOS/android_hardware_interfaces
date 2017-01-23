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


// TODO(b/31632518):  Need to get notification when our client dies so we can close the camera.
// As it stands, if the client dies suddenly, the camera will be stuck "open".
// NOTE:  Display should already be safe by virtue of holding only a weak pointer.


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

Return<sp<IEvsCamera>> EvsEnumerator::openCamera(const hidl_string& cameraId) {
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
        return nullptr;
    } else if (pRecord->inUse) {
        ALOGE("Cannot open camera %s which is already in use", cameraId.c_str());
        return nullptr;
    } else {
        pRecord->inUse = true;
        return(pRecord->pCamera);
    }
}

Return<void> EvsEnumerator::closeCamera(const ::android::sp<IEvsCamera>& camera) {
    ALOGD("closeCamera");

    if (camera == nullptr) {
        ALOGE("Ignoring call to closeCamera with null camera pointer");
    } else {
        // Find this camera in our list
        auto it = std::find_if(mCameraList.begin(),
                               mCameraList.end(),
                               [camera](const CameraRecord& rec) {
                                   return (rec.pCamera == camera);
                               });
        if (it == mCameraList.end()) {
            ALOGE("Ignoring close on unrecognized camera");
        } else {
            // Make sure the camera has stopped streaming
            camera->stopVideoStream();

            it->inUse = false;
        }
    }

    return Void();
}

Return<sp<IEvsDisplay>> EvsEnumerator::openDisplay() {
    ALOGD("openDisplay");

    // If we already have a display active, then this request must be denied
    sp<IEvsDisplay> pActiveDisplay = mActiveDisplay.promote();
    if (pActiveDisplay != nullptr) {
        ALOGW("Rejecting openDisplay request the display is already in use.");
        return nullptr;
    } else {
        // Create a new display interface and return it
        pActiveDisplay = new EvsDisplay();
        mActiveDisplay = pActiveDisplay;
        ALOGD("Returning new EvsDisplay object %p", pActiveDisplay.get());
        return pActiveDisplay;
    }
}

Return<void> EvsEnumerator::closeDisplay(const ::android::sp<IEvsDisplay>& display) {
    ALOGD("closeDisplay");

    // Do we still have a display object we think should be active?
    sp<IEvsDisplay> pActiveDisplay = mActiveDisplay.promote();

    if (pActiveDisplay == nullptr) {
        ALOGE("Ignoring closeDisplay when there is no active display.");
    } else if (display != pActiveDisplay) {
        ALOGE("Ignoring closeDisplay on a display we didn't issue");
        ALOGI("Got %p while active display is %p.", display.get(), pActiveDisplay.get());
    } else {
        // Drop the active display
        mActiveDisplay = nullptr;
    }

    return Void();
}

Return<DisplayState> EvsEnumerator::getDisplayState()  {
    ALOGD("getDisplayState");

    // Do we still have a display object we think should be active?
    sp<IEvsDisplay> pActiveDisplay = mActiveDisplay.promote();
    if (pActiveDisplay != nullptr) {
        return pActiveDisplay->getDisplayState();
    } else {
        return DisplayState::NOT_OPEN;
    }
}

} // namespace implementation
} // namespace V1_0
} // namespace evs
} // namespace hardware
} // namespace android
