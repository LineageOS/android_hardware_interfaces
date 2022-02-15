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

#include "EvsEnumerator.h"
#include "EvsCamera.h"
#include "EvsDisplay.h"
#include "EvsUltrasonicsArray.h"

using android::frameworks::automotive::display::V1_0::IAutomotiveDisplayProxyService;
using android::hardware::automotive::evs::V1_0::EvsResult;

namespace android::hardware::automotive::evs::V1_1::implementation {

namespace evs_v1_0 = ::android::hardware::automotive::evs::V1_0;

// NOTE:  All members values are static so that all clients operate on the same state
//        That is to say, this is effectively a singleton despite the fact that HIDL
//        constructs a new instance for each client.
std::list<EvsEnumerator::CameraRecord> EvsEnumerator::sCameraList;
wp<EvsDisplay> EvsEnumerator::sActiveDisplay;
std::unique_ptr<ConfigManager> EvsEnumerator::sConfigManager;
sp<IAutomotiveDisplayProxyService> EvsEnumerator::sDisplayProxyService;
std::unordered_map<uint8_t, uint64_t> EvsEnumerator::sDisplayPortList;
std::list<EvsEnumerator::UltrasonicsArrayRecord> EvsEnumerator::sUltrasonicsArrayRecordList;
uint64_t EvsEnumerator::sInternalDisplayId;

EvsEnumerator::EvsEnumerator(sp<IAutomotiveDisplayProxyService>& windowService) {
    ALOGD("%s", __FUNCTION__);

    // Add sample camera data to our list of cameras
    // In a real driver, this would be expected to can the available hardware
    sConfigManager =
            ConfigManager::Create("/vendor/etc/automotive/evs/evs_default_configuration.xml");

    // Add available cameras
    for (auto v : sConfigManager->getCameraList()) {
        CameraRecord rec(v.data());
        std::unique_ptr<ConfigManager::CameraInfo>& pInfo = sConfigManager->getCameraInfo(v);
        if (pInfo) {
            rec.desc.metadata.setToExternal(reinterpret_cast<uint8_t*>(pInfo->characteristics),
                                            get_camera_metadata_size(pInfo->characteristics));
        }
        sCameraList.push_back(std::move(rec));
    }

    if (!sDisplayProxyService) {
        /* sets a car-window service handle */
        sDisplayProxyService = windowService;
    }

    // Add available displays
    if (sDisplayProxyService) {
        // Get a display ID list.
        auto status = sDisplayProxyService->getDisplayIdList([](const auto& displayIds) {
            if (displayIds.size() > 0) {
                sInternalDisplayId = displayIds[0];
                for (const auto& id : displayIds) {
                    const auto port = id & 0xF;
                    sDisplayPortList.insert_or_assign(port, id);
                }
            }
        });

        if (!status.isOk()) {
            ALOGE("Failed to read a display list");
        }
    }

    // Add ultrasonics array desc.
    sUltrasonicsArrayRecordList.emplace_back(EvsUltrasonicsArray::GetMockArrayDesc("front_array"));
}

// Methods from ::android::hardware::automotive::evs::V1_0::IEvsEnumerator follow.
Return<void> EvsEnumerator::getCameraList(getCameraList_cb _hidl_cb) {
    ALOGD("%s", __FUNCTION__);

    const auto numCameras = sCameraList.size();

    // Build up a packed array of CameraDesc for return
    // NOTE:  Only has to live until the callback returns
    std::vector<evs_v1_0::CameraDesc> descriptions;
    descriptions.reserve(numCameras);
    for (const auto& cam : sCameraList) {
        descriptions.push_back(cam.desc.v1);
    }

    // Encapsulate our camera descriptions in the HIDL vec type
    hidl_vec<evs_v1_0::CameraDesc> hidlCameras(descriptions);

    // Send back the results
    ALOGD("reporting %zu cameras available", hidlCameras.size());
    _hidl_cb(hidlCameras);
    return {};
}

Return<sp<evs_v1_0::IEvsCamera>> EvsEnumerator::openCamera(const hidl_string& cameraId) {
    ALOGD("%s", __FUNCTION__);

    // Find the named camera
    auto it = std::find_if(sCameraList.begin(), sCameraList.end(), [&cameraId](const auto& cam) {
        return cameraId == cam.desc.v1.cameraId;
    });
    if (it == sCameraList.end()) {
        ALOGE("Requested camera %s not found", cameraId.c_str());
        return nullptr;
    }

    // Has this camera already been instantiated by another caller?
    sp<EvsCamera> pActiveCamera = it->activeInstance.promote();
    if (pActiveCamera != nullptr) {
        ALOGW("Killing previous camera because of new caller");
        closeCamera(pActiveCamera);
    }

    // Construct a camera instance for the caller
    if (!sConfigManager) {
        pActiveCamera = EvsCamera::Create(cameraId.c_str());
    } else {
        pActiveCamera =
                EvsCamera::Create(cameraId.c_str(), sConfigManager->getCameraInfo(cameraId));
    }
    it->activeInstance = pActiveCamera;
    if (!pActiveCamera) {
        ALOGE("Failed to allocate new EvsCamera object for %s\n", cameraId.c_str());
    }

    return pActiveCamera;
}

Return<void> EvsEnumerator::closeCamera(const ::android::sp<evs_v1_0::IEvsCamera>& pCamera) {
    ALOGD("%s", __FUNCTION__);

    auto pCamera_1_1 = IEvsCamera::castFrom(pCamera).withDefault(nullptr);
    if (!pCamera_1_1) {
        ALOGE("Ignoring call to closeCamera with null camera ptr");
        return {};
    }

    // Get the camera id so we can find it in our list
    std::string cameraId;
    pCamera_1_1->getCameraInfo_1_1([&cameraId](CameraDesc desc) { cameraId = desc.v1.cameraId; });

    // Find the named camera
    auto it = std::find_if(sCameraList.begin(), sCameraList.end(), [&cameraId](const auto& cam) {
        return cameraId == cam.desc.v1.cameraId;
    });
    if (it == sCameraList.end()) {
        ALOGE("Ignores a request to close unknown camera, %s", cameraId.data());
        return {};
    }

    sp<EvsCamera> pActiveCamera = it->activeInstance.promote();
    if (!pActiveCamera) {
        ALOGE("Somehow a camera is being destroyed when the enumerator didn't know one existed");
    } else if (pActiveCamera != pCamera_1_1) {
        // This can happen if the camera was aggressively reopened, orphaning this previous instance
        ALOGW("Ignoring close of previously orphaned camera - why did a client steal?");
    } else {
        // Drop the active camera
        pActiveCamera->forceShutdown();
        it->activeInstance = nullptr;
    }

    return {};
}

Return<sp<V1_0::IEvsDisplay>> EvsEnumerator::openDisplay() {
    ALOGD("%s", __FUNCTION__);

    // If we already have a display active, then we need to shut it down so we can
    // give exclusive access to the new caller.
    sp<EvsDisplay> pActiveDisplay = sActiveDisplay.promote();
    if (pActiveDisplay != nullptr) {
        ALOGW("Killing previous display because of new caller");
        closeDisplay(pActiveDisplay);
    }

    // Create a new display interface and return it
    pActiveDisplay = new EvsDisplay(sDisplayProxyService, sInternalDisplayId);
    sActiveDisplay = pActiveDisplay;

    ALOGD("Returning new EvsDisplay object %p", pActiveDisplay.get());
    return pActiveDisplay;
}

Return<void> EvsEnumerator::getDisplayIdList(getDisplayIdList_cb _list_cb) {
    hidl_vec<uint8_t> ids;
    ids.resize(sDisplayPortList.size());

    unsigned i = 0;
    std::for_each(sDisplayPortList.begin(), sDisplayPortList.end(),
                  [&](const auto& element) { ids[i++] = element.first; });

    _list_cb(ids);
    return {};
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

Return<void> EvsEnumerator::closeDisplay(const ::android::sp<V1_0::IEvsDisplay>& pDisplay) {
    ALOGD("%s", __FUNCTION__);

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

    return {};
}

Return<V1_0::DisplayState> EvsEnumerator::getDisplayState() {
    ALOGD("%s", __FUNCTION__);

    // Do we still have a display object we think should be active?
    sp<IEvsDisplay> pActiveDisplay = sActiveDisplay.promote();
    if (pActiveDisplay != nullptr) {
        return pActiveDisplay->getDisplayState();
    } else {
        return V1_0::DisplayState::NOT_OPEN;
    }
}

// Methods from ::android::hardware::automotive::evs::V1_1::IEvsEnumerator follow.
Return<void> EvsEnumerator::getCameraList_1_1(getCameraList_1_1_cb _hidl_cb) {
    ALOGD("%s", __FUNCTION__);

    const auto numCameras = sCameraList.size();

    // Build up a packed array of CameraDesc for return
    // NOTE:  Only has to live until the callback returns
    std::vector<CameraDesc> descriptions;
    descriptions.reserve(numCameras);
    std::for_each(sCameraList.begin(), sCameraList.end(),
                  [&](const auto& cam) { descriptions.push_back(cam.desc); });

    // Encapsulate our camera descriptions in the HIDL vec type
    hidl_vec<CameraDesc> hidlCameras(descriptions);

    // Send back the results
    ALOGD("reporting %zu cameras available", hidlCameras.size());
    _hidl_cb(hidlCameras);
    return {};
}

Return<sp<IEvsCamera>> EvsEnumerator::openCamera_1_1(const hidl_string& cameraId,
                                                     const Stream& streamCfg) {
    ALOGD("%s", __FUNCTION__);

    // Find the named camera
    auto it = std::find_if(sCameraList.begin(), sCameraList.end(), [&cameraId](const auto& cam) {
        return cameraId == cam.desc.v1.cameraId;
    });
    if (it == sCameraList.end()) {
        ALOGE("Requested camera %s not found", cameraId.c_str());
        return nullptr;
    }

    // Has this camera already been instantiated by another caller?
    sp<EvsCamera> pActiveCamera = it->activeInstance.promote();
    if (pActiveCamera != nullptr) {
        ALOGW("Killing previous camera because of new caller");
        closeCamera(pActiveCamera);
    }

    // Construct a camera instance for the caller
    if (!sConfigManager) {
        pActiveCamera = EvsCamera::Create(cameraId.c_str());
    } else {
        pActiveCamera = EvsCamera::Create(cameraId.c_str(), sConfigManager->getCameraInfo(cameraId),
                                          &streamCfg);
    }

    it->activeInstance = pActiveCamera;
    if (!pActiveCamera) {
        ALOGE("Failed to allocate new EvsCamera object for %s\n", cameraId.c_str());
    }

    return pActiveCamera;
}

EvsEnumerator::CameraRecord* EvsEnumerator::findCameraById(const std::string& cameraId) {
    ALOGD("%s", __FUNCTION__);

    // Find the named camera
    auto it = std::find_if(sCameraList.begin(), sCameraList.end(), [&cameraId](const auto& cam) {
        return cameraId == cam.desc.v1.cameraId;
    });
    return (it != sCameraList.end()) ? &*it : nullptr;
}

EvsEnumerator::UltrasonicsArrayRecord* EvsEnumerator::findUltrasonicsArrayById(
        const std::string& ultrasonicsArrayId) {
    ALOGD("%s", __FUNCTION__);

    auto recordIt =
            std::find_if(sUltrasonicsArrayRecordList.begin(), sUltrasonicsArrayRecordList.end(),
                         [&ultrasonicsArrayId](const UltrasonicsArrayRecord& record) {
                             return ultrasonicsArrayId == record.desc.ultrasonicsArrayId;
                         });

    return (recordIt != sUltrasonicsArrayRecordList.end()) ? &*recordIt : nullptr;
}

Return<void> EvsEnumerator::getUltrasonicsArrayList(getUltrasonicsArrayList_cb _hidl_cb) {
    ALOGD("%s", __FUNCTION__);

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
    return {};
}

Return<sp<IEvsUltrasonicsArray>> EvsEnumerator::openUltrasonicsArray(
        const hidl_string& ultrasonicsArrayId) {
    ALOGD("%s", __FUNCTION__);

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
    ALOGD("%s", __FUNCTION__);

    if (pEvsUltrasonicsArray.get() == nullptr) {
        ALOGE("Ignoring call to closeUltrasonicsArray with null ultrasonics array");
        return {};
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
        return {};
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

    return {};
}

}  // namespace android::hardware::automotive::evs::V1_1::implementation
