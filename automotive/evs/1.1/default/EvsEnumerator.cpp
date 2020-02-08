/*
 * Copyright (C) 2019 The Android Open Source Project
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

#define LOG_TAG "android.hardware.automotive.evs@1.1-service"

#include "EvsEnumerator.h"
#include "EvsCamera.h"
#include "EvsDisplay.h"
#include "EvsUltrasonicsArray.h"

namespace android {
namespace hardware {
namespace automotive {
namespace evs {
namespace V1_1 {
namespace implementation {


// NOTE:  All members values are static so that all clients operate on the same state
//        That is to say, this is effectively a singleton despite the fact that HIDL
//        constructs a new instance for each client.
std::list<EvsEnumerator::CameraRecord>              EvsEnumerator::sCameraList;
wp<EvsDisplay>                                      EvsEnumerator::sActiveDisplay;
unique_ptr<ConfigManager>                           EvsEnumerator::sConfigManager;
sp<IAutomotiveDisplayProxyService>                  EvsEnumerator::sDisplayProxyService;
std::unordered_map<uint8_t, uint64_t>               EvsEnumerator::sDisplayPortList;
std::list<EvsEnumerator::UltrasonicsArrayRecord>    EvsEnumerator::sUltrasonicsArrayRecordList;

EvsEnumerator::EvsEnumerator(sp<IAutomotiveDisplayProxyService> windowService) {
    ALOGD("EvsEnumerator created");

    // Add sample camera data to our list of cameras
    // In a real driver, this would be expected to can the available hardware
    sConfigManager =
        ConfigManager::Create("/vendor/etc/automotive/evs/evs_default_configuration.xml");

    // Add available cameras
    for (auto v : sConfigManager->getCameraList()) {
        sCameraList.emplace_back(v.c_str());
    }

    if (sDisplayProxyService == nullptr) {
        /* sets a car-window service handle */
        sDisplayProxyService = windowService;
    }

    // Add available displays
    if (sDisplayProxyService != nullptr) {
        // Get a display ID list.
        sDisplayProxyService->getDisplayIdList([](const auto& displayIds) {
            for (const auto& id : displayIds) {
                const auto port = id & 0xF;
                sDisplayPortList.insert_or_assign(port, id);
            }
        });
    }

    // Add ultrasonics array desc.
    sUltrasonicsArrayRecordList.emplace_back(
            EvsUltrasonicsArray::GetDummyArrayDesc("front_array"));
}


// Methods from ::android::hardware::automotive::evs::V1_0::IEvsEnumerator follow.
Return<void> EvsEnumerator::getCameraList(getCameraList_cb _hidl_cb)  {
    ALOGD("getCameraList");

    const unsigned numCameras = sCameraList.size();

    // Build up a packed array of CameraDesc for return
    // NOTE:  Only has to live until the callback returns
    std::vector<CameraDesc_1_0> descriptions;
    descriptions.reserve(numCameras);
    for (const auto& cam : sCameraList) {
        descriptions.push_back( cam.desc.v1 );
    }

    // Encapsulate our camera descriptions in the HIDL vec type
    hidl_vec<CameraDesc_1_0> hidlCameras(descriptions);

    // Send back the results
    ALOGD("reporting %zu cameras available", hidlCameras.size());
    _hidl_cb(hidlCameras);

    // HIDL convention says we return Void if we sent our result back via callback
    return Void();
}


Return<sp<IEvsCamera_1_0>> EvsEnumerator::openCamera(const hidl_string& cameraId) {
    ALOGD("openCamera");

    // Find the named camera
    CameraRecord *pRecord = nullptr;
    for (auto &&cam : sCameraList) {
        if (cam.desc.v1.cameraId == cameraId) {
            // Found a match!
            pRecord = &cam;
            break;
        }
    }

    // Is this a recognized camera id?
    if (!pRecord) {
        ALOGE("Requested camera %s not found", cameraId.c_str());
        return nullptr;
    }

    // Has this camera already been instantiated by another caller?
    sp<EvsCamera> pActiveCamera = pRecord->activeInstance.promote();
    if (pActiveCamera != nullptr) {
        ALOGW("Killing previous camera because of new caller");
        closeCamera(pActiveCamera);
    }

    // Construct a camera instance for the caller
    if (sConfigManager == nullptr) {
        pActiveCamera = EvsCamera::Create(cameraId.c_str());
    } else {
        pActiveCamera = EvsCamera::Create(cameraId.c_str(),
                                          sConfigManager->getCameraInfo(cameraId));
    }
    pRecord->activeInstance = pActiveCamera;
    if (pActiveCamera == nullptr) {
        ALOGE("Failed to allocate new EvsCamera object for %s\n", cameraId.c_str());
    }

    return pActiveCamera;
}


Return<void> EvsEnumerator::closeCamera(const ::android::sp<IEvsCamera_1_0>& pCamera) {
    ALOGD("closeCamera");

    auto pCamera_1_1 = IEvsCamera_1_1::castFrom(pCamera).withDefault(nullptr);
    if (pCamera_1_1 == nullptr) {
        ALOGE("Ignoring call to closeCamera with null camera ptr");
        return Void();
    }

    // Get the camera id so we can find it in our list
    std::string cameraId;
    pCamera_1_1->getCameraInfo_1_1([&cameraId](CameraDesc desc) {
                               cameraId = desc.v1.cameraId;
                           }
    );

    // Find the named camera
    CameraRecord *pRecord = nullptr;
    for (auto &&cam : sCameraList) {
        if (cam.desc.v1.cameraId == cameraId) {
            // Found a match!
            pRecord = &cam;
            break;
        }
    }

    // Is the display being destroyed actually the one we think is active?
    if (!pRecord) {
        ALOGE("Asked to close a camera who's name isn't recognized");
    } else {
        sp<EvsCamera> pActiveCamera = pRecord->activeInstance.promote();

        if (pActiveCamera == nullptr) {
            ALOGE("Somehow a camera is being destroyed when the enumerator didn't know one existed");
        } else if (pActiveCamera != pCamera_1_1) {
            // This can happen if the camera was aggressively reopened, orphaning this previous instance
            ALOGW("Ignoring close of previously orphaned camera - why did a client steal?");
        } else {
            // Drop the active camera
            pActiveCamera->forceShutdown();
            pRecord->activeInstance = nullptr;
        }
    }

    return Void();
}


Return<sp<IEvsDisplay_1_0>> EvsEnumerator::openDisplay() {
    ALOGD("openDisplay");

    // If we already have a display active, then we need to shut it down so we can
    // give exclusive access to the new caller.
    sp<EvsDisplay> pActiveDisplay = sActiveDisplay.promote();
    if (pActiveDisplay != nullptr) {
        ALOGW("Killing previous display because of new caller");
        closeDisplay(pActiveDisplay);
    }

    // Create a new display interface and return it
    pActiveDisplay = new EvsDisplay();
    sActiveDisplay = pActiveDisplay;

    ALOGD("Returning new EvsDisplay object %p", pActiveDisplay.get());
    return pActiveDisplay;
}


Return<void> EvsEnumerator::getDisplayIdList(getDisplayIdList_cb _list_cb) {
    hidl_vec<uint8_t> ids;

    ids.resize(sDisplayPortList.size());
    unsigned i = 0;
    for (const auto& [port, id] : sDisplayPortList) {
        ids[i++] = port;
    }

    _list_cb(ids);
    return Void();
}


Return<sp<IEvsDisplay>> EvsEnumerator::openDisplay_1_1(uint8_t port) {
    ALOGD("%s", __FUNCTION__);

    // If we already have a display active, then we need to shut it down so we can
    // give exclusive access to the new caller.
    sp<EvsDisplay> pActiveDisplay = sActiveDisplay.promote();
    if (pActiveDisplay != nullptr) {
        ALOGW("Killing previous display because of new caller");
        closeDisplay(pActiveDisplay);
    }

    // Create a new display interface and return it
    pActiveDisplay = new EvsDisplay(sDisplayProxyService, sDisplayPortList[port]);
    sActiveDisplay = pActiveDisplay;

    ALOGD("Returning new EvsDisplay object %p", pActiveDisplay.get());
    return pActiveDisplay;
}



Return<void> EvsEnumerator::closeDisplay(const ::android::sp<IEvsDisplay_1_0>& pDisplay) {
    ALOGD("closeDisplay");

    // Do we still have a display object we think should be active?
    sp<EvsDisplay> pActiveDisplay = sActiveDisplay.promote();
    if (pActiveDisplay == nullptr) {
        ALOGE("Somehow a display is being destroyed when the enumerator didn't know one existed");
    } else if (sActiveDisplay != pDisplay) {
        ALOGW("Ignoring close of previously orphaned display - why did a client steal?");
    } else {
        // Drop the active display
        pActiveDisplay->forceShutdown();
        sActiveDisplay = nullptr;
    }

    return Void();
}


Return<DisplayState> EvsEnumerator::getDisplayState()  {
    ALOGD("getDisplayState");

    // Do we still have a display object we think should be active?
    sp<IEvsDisplay> pActiveDisplay = sActiveDisplay.promote();
    if (pActiveDisplay != nullptr) {
        return pActiveDisplay->getDisplayState();
    } else {
        return DisplayState::NOT_OPEN;
    }
}


// Methods from ::android::hardware::automotive::evs::V1_1::IEvsEnumerator follow.
Return<void> EvsEnumerator::getCameraList_1_1(getCameraList_1_1_cb _hidl_cb)  {
    ALOGD("getCameraList");

    const unsigned numCameras = sCameraList.size();

    // Build up a packed array of CameraDesc for return
    // NOTE:  Only has to live until the callback returns
    std::vector<CameraDesc_1_1> descriptions;
    descriptions.reserve(numCameras);
    for (const auto& cam : sCameraList) {
        descriptions.push_back( cam.desc );
    }

    // Encapsulate our camera descriptions in the HIDL vec type
    hidl_vec<CameraDesc_1_1> hidlCameras(descriptions);

    // Send back the results
    ALOGD("reporting %zu cameras available", hidlCameras.size());
    _hidl_cb(hidlCameras);

    // HIDL convention says we return Void if we sent our result back via callback
    return Void();
}

Return<sp<IEvsCamera_1_1>>
EvsEnumerator::openCamera_1_1(const hidl_string& cameraId,
                              const Stream& streamCfg) {
    // Find the named camera
    CameraRecord *pRecord = nullptr;
    for (auto &&cam : sCameraList) {
        if (cam.desc.v1.cameraId == cameraId) {
            // Found a match!
            pRecord = &cam;
            break;
        }
    }

    // Is this a recognized camera id?
    if (!pRecord) {
        ALOGE("Requested camera %s not found", cameraId.c_str());
        return nullptr;
    }

    // Has this camera already been instantiated by another caller?
    sp<EvsCamera> pActiveCamera = pRecord->activeInstance.promote();
    if (pActiveCamera != nullptr) {
        ALOGW("Killing previous camera because of new caller");
        closeCamera(pActiveCamera);
    }

    // Construct a camera instance for the caller
    if (sConfigManager == nullptr) {
        pActiveCamera = EvsCamera::Create(cameraId.c_str());
    } else {
        pActiveCamera = EvsCamera::Create(cameraId.c_str(),
                                          sConfigManager->getCameraInfo(cameraId),
                                          &streamCfg);
    }

    pRecord->activeInstance = pActiveCamera;
    if (pActiveCamera == nullptr) {
        ALOGE("Failed to allocate new EvsCamera object for %s\n", cameraId.c_str());
    }

    return pActiveCamera;
}


EvsEnumerator::CameraRecord* EvsEnumerator::findCameraById(const std::string& cameraId) {
    // Find the named camera
    CameraRecord *pRecord = nullptr;
    for (auto &&cam : sCameraList) {
        if (cam.desc.v1.cameraId == cameraId) {
            // Found a match!
            pRecord = &cam;
            break;
        }
    }

    return pRecord;
}

EvsEnumerator::UltrasonicsArrayRecord* EvsEnumerator::findUltrasonicsArrayById(
        const std::string& ultrasonicsArrayId) {
    auto recordIt = std::find_if(
            sUltrasonicsArrayRecordList.begin(), sUltrasonicsArrayRecordList.end(),
                    [&ultrasonicsArrayId](const UltrasonicsArrayRecord& record) {
                            return ultrasonicsArrayId == record.desc.ultrasonicsArrayId;});

    return (recordIt != sUltrasonicsArrayRecordList.end()) ? &*recordIt : nullptr;
}

Return<void> EvsEnumerator::getUltrasonicsArrayList(getUltrasonicsArrayList_cb _hidl_cb) {
    hidl_vec<UltrasonicsArrayDesc> desc;
    desc.resize(sUltrasonicsArrayRecordList.size());

    // Copy over desc from sUltrasonicsArrayRecordList.
    for (auto p = std::make_pair(sUltrasonicsArrayRecordList.begin(), desc.begin());
            p.first != sUltrasonicsArrayRecordList.end(); p.first++, p.second++) {
        *p.second = p.first->desc;
    }

    // Send back the results
    ALOGD("reporting %zu ultrasonics arrays available", desc.size());
    _hidl_cb(desc);

    // HIDL convention says we return Void if we sent our result back via callback
    return Void();
}

Return<sp<IEvsUltrasonicsArray>> EvsEnumerator::openUltrasonicsArray(
        const hidl_string& ultrasonicsArrayId) {
    // Find the named ultrasonic array.
    UltrasonicsArrayRecord* pRecord = findUltrasonicsArrayById(ultrasonicsArrayId);

    // Is this a recognized ultrasonic array id?
    if (!pRecord) {
        ALOGE("Requested ultrasonics array %s not found", ultrasonicsArrayId.c_str());
        return nullptr;
    }

    // Has this ultrasonic array already been instantiated by another caller?
    sp<EvsUltrasonicsArray> pActiveUltrasonicsArray = pRecord->activeInstance.promote();
    if (pActiveUltrasonicsArray != nullptr) {
        ALOGW("Killing previous ultrasonics array because of new caller");
        closeUltrasonicsArray(pActiveUltrasonicsArray);
    }

    // Construct a ultrasonic array instance for the caller
    pActiveUltrasonicsArray = EvsUltrasonicsArray::Create(ultrasonicsArrayId.c_str());
    pRecord->activeInstance = pActiveUltrasonicsArray;
    if (pActiveUltrasonicsArray == nullptr) {
        ALOGE("Failed to allocate new EvsUltrasonicsArray object for %s\n",
              ultrasonicsArrayId.c_str());
    }

    return pActiveUltrasonicsArray;
}

Return<void> EvsEnumerator::closeUltrasonicsArray(
        const sp<IEvsUltrasonicsArray>& pEvsUltrasonicsArray) {

    if (pEvsUltrasonicsArray.get() == nullptr) {
        ALOGE("Ignoring call to closeUltrasonicsArray with null ultrasonics array");
        return Void();
    }

    // Get the ultrasonics array id so we can find it in our list.
    std::string ultrasonicsArrayId;
    pEvsUltrasonicsArray->getUltrasonicArrayInfo([&ultrasonicsArrayId](UltrasonicsArrayDesc desc) {
        ultrasonicsArrayId.assign(desc.ultrasonicsArrayId);
    });

    // Find the named ultrasonics array
    UltrasonicsArrayRecord* pRecord = findUltrasonicsArrayById(ultrasonicsArrayId);
    if (!pRecord) {
        ALOGE("Asked to close a ultrasonics array whose name isnt not found");
        return Void();
    }

    sp<EvsUltrasonicsArray> pActiveUltrasonicsArray = pRecord->activeInstance.promote();

    if (pActiveUltrasonicsArray.get() == nullptr) {
        ALOGE("Somehow a ultrasonics array is being destroyed when the enumerator didn't know "
              "one existed");
    } else if (pActiveUltrasonicsArray != pEvsUltrasonicsArray) {
        // This can happen if the ultrasonics array was aggressively reopened,
        // orphaning this previous instance
        ALOGW("Ignoring close of previously orphaned ultrasonics array - why did a client steal?");
    } else {
        // Drop the active ultrasonics array
        pActiveUltrasonicsArray->forceShutdown();
        pRecord->activeInstance = nullptr;
    }

    return Void();
}

} // namespace implementation
} // namespace V1_1
} // namespace evs
} // namespace automotive
} // namespace hardware
} // namespace android
