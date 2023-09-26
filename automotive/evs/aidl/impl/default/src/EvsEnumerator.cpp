/*
 * Copyright (C) 2023 The Android Open Source Project
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

#include "ConfigManager.h"
#include "EvsAllCameras.h"
#include "EvsCameraBase.h"
#include "EvsGlDisplay.h"

#include <aidl/android/hardware/automotive/evs/EvsResult.h>
#include <aidl/android/hardware/graphics/common/BufferUsage.h>
#include <aidl/android/hardware/graphics/common/PixelFormat.h>
#include <cutils/android_filesystem_config.h>

#include <set>
#include <string_view>

namespace {

using ::aidl::android::frameworks::automotive::display::ICarDisplayProxy;
using ::aidl::android::hardware::graphics::common::BufferUsage;
using ::ndk::ScopedAStatus;
using std::chrono_literals::operator""s;

// Constants
constexpr std::chrono::seconds kEnumerationTimeout = 10s;
constexpr uint64_t kInvalidDisplayId = std::numeric_limits<uint64_t>::max();
const std::set<uid_t> kAllowedUids = {AID_AUTOMOTIVE_EVS, AID_SYSTEM, AID_ROOT};

}  // namespace

namespace aidl::android::hardware::automotive::evs::implementation {

// NOTE:  All members values are static so that all clients operate on the same state
//        That is to say, this is effectively a singleton despite the fact that HIDL
//        constructs a new instance for each client.
std::unordered_map<std::string, EvsEnumerator::CameraRecord> EvsEnumerator::sCameraList;
std::mutex EvsEnumerator::sLock;
std::condition_variable EvsEnumerator::sCameraSignal;
std::unique_ptr<ConfigManager> EvsEnumerator::sConfigManager;
std::shared_ptr<ICarDisplayProxy> EvsEnumerator::sDisplayProxy;
std::unordered_map<uint8_t, uint64_t> EvsEnumerator::sDisplayPortList;

EvsEnumerator::ActiveDisplays& EvsEnumerator::mutableActiveDisplays() {
    static ActiveDisplays active_displays;
    return active_displays;
}

EvsEnumerator::EvsEnumerator(const std::shared_ptr<ICarDisplayProxy>& proxyService) {
    LOG(DEBUG) << "EvsEnumerator is created.";

    if (!sConfigManager) {
        /* loads and initializes ConfigManager in a separate thread */
        sConfigManager = ConfigManager::Create();
    }

    if (!sDisplayProxy) {
        /* sets a car-window service handle */
        sDisplayProxy = proxyService;
    }

    // Enumerate existing devices
    enumerateCameras();
    mInternalDisplayId = enumerateDisplays();
}

bool EvsEnumerator::checkPermission() {
    const auto uid = AIBinder_getCallingUid();
    if (kAllowedUids.find(uid) == kAllowedUids.end()) {
        LOG(ERROR) << "EVS access denied: "
                   << "pid = " << AIBinder_getCallingPid() << ", uid = " << uid;
        return false;
    }

    return true;
}

void EvsEnumerator::enumerateCameras() {
    if (!sConfigManager) {
        return;
    }

    for (auto id : sConfigManager->getCameraIdList()) {
        CameraRecord rec(id.data());
        std::unique_ptr<ConfigManager::CameraInfo>& pInfo = sConfigManager->getCameraInfo(id);
        if (pInfo) {
            uint8_t* ptr = reinterpret_cast<uint8_t*>(pInfo->characteristics);
            const size_t len = get_camera_metadata_size(pInfo->characteristics);
            rec.desc.metadata.insert(rec.desc.metadata.end(), ptr, ptr + len);
        }
        sCameraList.insert_or_assign(id, std::move(rec));
    }
}

uint64_t EvsEnumerator::enumerateDisplays() {
    LOG(INFO) << __FUNCTION__ << ": Starting display enumeration";
    uint64_t internalDisplayId = kInvalidDisplayId;
    if (!sDisplayProxy) {
        LOG(ERROR) << "ICarDisplayProxy is not available!";
        return internalDisplayId;
    }

    std::vector<int64_t> displayIds;
    if (auto status = sDisplayProxy->getDisplayIdList(&displayIds); !status.isOk()) {
        LOG(ERROR) << "Failed to retrieve a display id list"
                   << ::android::statusToString(status.getStatus());
        return internalDisplayId;
    }

    if (displayIds.size() > 0) {
        // The first entry of the list is the internal display.  See
        // SurfaceFlinger::getPhysicalDisplayIds() implementation.
        internalDisplayId = displayIds[0];
        for (const auto& id : displayIds) {
            const auto port = id & 0xFF;
            LOG(INFO) << "Display " << std::hex << id << " is detected on the port, " << port;
            sDisplayPortList.insert_or_assign(port, id);
        }
    }

    LOG(INFO) << "Found " << sDisplayPortList.size() << " displays";
    return internalDisplayId;
}

// Methods from ::android::hardware::automotive::evs::IEvsEnumerator follow.
ScopedAStatus EvsEnumerator::getCameraList(std::vector<CameraDesc>* _aidl_return) {
    LOG(DEBUG) << __FUNCTION__;
    if (!checkPermission()) {
        return ScopedAStatus::fromServiceSpecificError(
                static_cast<int>(EvsResult::PERMISSION_DENIED));
    }

    {
        std::unique_lock<std::mutex> lock(sLock);
        if (sCameraList.size() < 1) {
            // No qualified device has been found.  Wait until new device is ready,
            // for 10 seconds.
            if (!sCameraSignal.wait_for(lock, kEnumerationTimeout,
                                        [] { return sCameraList.size() > 0; })) {
                LOG(DEBUG) << "Timer expired.  No new device has been added.";
            }
        }
    }

    // Build up a packed array of CameraDesc for return
    _aidl_return->resize(sCameraList.size());
    unsigned i = 0;
    for (const auto& [key, cam] : sCameraList) {
        (*_aidl_return)[i++] = cam.desc;
    }

    if (sConfigManager) {
        // Adding camera groups that represent logical camera devices
        auto camGroups = sConfigManager->getCameraGroupIdList();
        for (auto&& id : camGroups) {
            if (sCameraList.find(id) != sCameraList.end()) {
                // Already exists in the _aidl_return
                continue;
            }

            std::unique_ptr<ConfigManager::CameraGroupInfo>& tempInfo =
                    sConfigManager->getCameraGroupInfo(id);
            CameraRecord cam(id.data());
            if (tempInfo) {
                uint8_t* ptr = reinterpret_cast<uint8_t*>(tempInfo->characteristics);
                const size_t len = get_camera_metadata_size(tempInfo->characteristics);
                cam.desc.metadata.insert(cam.desc.metadata.end(), ptr, ptr + len);
            }

            sCameraList.insert_or_assign(id, cam);
            _aidl_return->push_back(cam.desc);
        }
    }

    // Send back the results
    LOG(DEBUG) << "Reporting " << sCameraList.size() << " cameras available";
    return ScopedAStatus::ok();
}

ScopedAStatus EvsEnumerator::getStreamList(const CameraDesc& desc,
                                           std::vector<Stream>* _aidl_return) {
    using AidlPixelFormat = ::aidl::android::hardware::graphics::common::PixelFormat;

    camera_metadata_t* pMetadata = const_cast<camera_metadata_t*>(
            reinterpret_cast<const camera_metadata_t*>(desc.metadata.data()));
    camera_metadata_entry_t streamConfig;
    if (!find_camera_metadata_entry(pMetadata, ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS,
                                    &streamConfig)) {
        const unsigned numStreamConfigs = streamConfig.count / sizeof(StreamConfiguration);
        _aidl_return->resize(numStreamConfigs);
        const StreamConfiguration* pCurrentConfig =
                reinterpret_cast<StreamConfiguration*>(streamConfig.data.i32);
        for (unsigned i = 0; i < numStreamConfigs; ++i, ++pCurrentConfig) {
            // Build ::aidl::android::hardware::automotive::evs::Stream from
            // StreamConfiguration.
            Stream current = {
                    .id = pCurrentConfig->id,
                    .streamType =
                            pCurrentConfig->type ==
                                            ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_INPUT
                                    ? StreamType::INPUT
                                    : StreamType::OUTPUT,
                    .width = pCurrentConfig->width,
                    .height = pCurrentConfig->height,
                    .format = static_cast<AidlPixelFormat>(pCurrentConfig->format),
                    .usage = BufferUsage::CAMERA_INPUT,
                    .rotation = Rotation::ROTATION_0,
            };

            (*_aidl_return)[i] = current;
        }
    }

    return ScopedAStatus::ok();
}

ScopedAStatus EvsEnumerator::openCamera(const std::string& id, const Stream& cfg,
                                        std::shared_ptr<IEvsCamera>* obj) {
    LOG(DEBUG) << __FUNCTION__;
    if (!checkPermission()) {
        return ScopedAStatus::fromServiceSpecificError(
                static_cast<int>(EvsResult::PERMISSION_DENIED));
    }

    // Is this a recognized camera id?
    CameraRecord* pRecord = findCameraById(id);
    if (!pRecord) {
        LOG(ERROR) << id << " does not exist!";
        return ScopedAStatus::fromServiceSpecificError(static_cast<int>(EvsResult::INVALID_ARG));
    }

    // Has this camera already been instantiated by another caller?
    std::shared_ptr<EvsCameraBase> pActiveCamera = pRecord->activeInstance.lock();
    if (pActiveCamera) {
        LOG(WARNING) << "Killing previous camera because of new caller";
        closeCamera(pActiveCamera);
    }

    // Construct a camera instance for the caller
    if (!sConfigManager) {
        pActiveCamera = EvsMockCamera::Create(id.data());
    } else {
        auto& cameraInfo = sConfigManager->getCameraInfo(id);
        switch (cameraInfo->deviceType) {
            using DeviceType = ConfigManager::CameraInfo::DeviceType;

            // Default to MOCK for backward compatibility.
            case DeviceType::NONE:
            case DeviceType::MOCK:
                pActiveCamera = EvsMockCamera::Create(id.data(), cameraInfo, &cfg);
                break;

            case DeviceType::VIDEO:
                pActiveCamera = EvsVideoEmulatedCamera::Create(id.data(), cameraInfo, &cfg);
                break;

            default:
                LOG(ERROR) << __func__ << ": camera device type "
                           << static_cast<std::int32_t>(cameraInfo->deviceType)
                           << " is not supported.";
                break;
        }
    }

    pRecord->activeInstance = pActiveCamera;
    if (!pActiveCamera) {
        LOG(ERROR) << "Failed to create new EVS camera object for " << id;
        return ScopedAStatus::fromServiceSpecificError(
                static_cast<int>(EvsResult::UNDERLYING_SERVICE_ERROR));
    }

    *obj = pActiveCamera;
    return ScopedAStatus::ok();
}

ScopedAStatus EvsEnumerator::closeCamera(const std::shared_ptr<IEvsCamera>& cameraObj) {
    LOG(DEBUG) << __FUNCTION__;

    if (!cameraObj) {
        LOG(ERROR) << "Ignoring call to closeCamera with null camera ptr";
        return ScopedAStatus::fromServiceSpecificError(static_cast<int>(EvsResult::INVALID_ARG));
    }

    // Get the camera id so we can find it in our list
    CameraDesc desc;
    auto status = cameraObj->getCameraInfo(&desc);
    if (!status.isOk()) {
        LOG(ERROR) << "Failed to read a camera descriptor";
        return ScopedAStatus::fromServiceSpecificError(
                static_cast<int>(EvsResult::UNDERLYING_SERVICE_ERROR));
    }
    auto cameraId = desc.id;
    closeCamera_impl(cameraObj, cameraId);
    return ScopedAStatus::ok();
}

ScopedAStatus EvsEnumerator::openDisplay(int32_t id, std::shared_ptr<IEvsDisplay>* displayObj) {
    LOG(DEBUG) << __FUNCTION__;
    if (!checkPermission()) {
        return ScopedAStatus::fromServiceSpecificError(
                static_cast<int>(EvsResult::PERMISSION_DENIED));
    }

    auto& displays = mutableActiveDisplays();

    if (auto existing_display_search = displays.popDisplay(id)) {
        // If we already have a display active, then we need to shut it down so we can
        // give exclusive access to the new caller.
        std::shared_ptr<EvsGlDisplay> pActiveDisplay = existing_display_search->displayWeak.lock();
        if (pActiveDisplay) {
            LOG(WARNING) << "Killing previous display because of new caller";
            pActiveDisplay->forceShutdown();
        }
    }

    // Create a new display interface and return it
    uint64_t targetDisplayId = mInternalDisplayId;
    auto it = sDisplayPortList.find(id);
    if (it != sDisplayPortList.end()) {
        targetDisplayId = it->second;
    } else {
        LOG(WARNING) << "No display is available on the port " << static_cast<int32_t>(id)
                     << ". The main display " << mInternalDisplayId << " will be used instead";
    }

    // Create a new display interface and return it.
    std::shared_ptr<EvsGlDisplay> pActiveDisplay =
            ndk::SharedRefBase::make<EvsGlDisplay>(sDisplayProxy, targetDisplayId);

    if (auto insert_result = displays.tryInsert(id, pActiveDisplay); !insert_result) {
        LOG(ERROR) << "Display ID " << id << " has been used by another caller.";
        pActiveDisplay->forceShutdown();
        return ScopedAStatus::fromServiceSpecificError(static_cast<int>(EvsResult::RESOURCE_BUSY));
    }

    LOG(DEBUG) << "Returning new EvsGlDisplay object " << pActiveDisplay.get();
    *displayObj = pActiveDisplay;
    return ScopedAStatus::ok();
}

ScopedAStatus EvsEnumerator::closeDisplay(const std::shared_ptr<IEvsDisplay>& obj) {
    LOG(DEBUG) << __FUNCTION__;

    auto& displays = mutableActiveDisplays();
    const auto display_search = displays.popDisplay(obj);

    if (!display_search) {
        LOG(WARNING) << "Ignoring close of previously orphaned display - why did a client steal?";
        return ScopedAStatus::ok();
    }

    auto pActiveDisplay = display_search->displayWeak.lock();

    if (!pActiveDisplay) {
        LOG(ERROR) << "Somehow a display is being destroyed "
                   << "when the enumerator didn't know one existed";
        return ScopedAStatus::fromServiceSpecificError(static_cast<int>(EvsResult::OWNERSHIP_LOST));
    }

    pActiveDisplay->forceShutdown();
    return ScopedAStatus::ok();
}

ScopedAStatus EvsEnumerator::getDisplayState(DisplayState* state) {
    LOG(DEBUG) << __FUNCTION__;
    return getDisplayStateImpl(std::nullopt, state);
}

ScopedAStatus EvsEnumerator::getDisplayStateById(int32_t displayId, DisplayState* state) {
    LOG(DEBUG) << __FUNCTION__;
    return getDisplayStateImpl(displayId, state);
}

ScopedAStatus EvsEnumerator::getDisplayStateImpl(std::optional<int32_t> displayId,
                                                 DisplayState* state) {
    if (!checkPermission()) {
        *state = DisplayState::DEAD;
        return ScopedAStatus::fromServiceSpecificError(
                static_cast<int>(EvsResult::PERMISSION_DENIED));
    }

    const auto& all_displays = mutableActiveDisplays().getAllDisplays();

    const auto display_search = displayId ? all_displays.find(*displayId) : all_displays.begin();

    if (display_search == all_displays.end()) {
        *state = DisplayState::NOT_OPEN;
        return ScopedAStatus::fromServiceSpecificError(static_cast<int>(EvsResult::OWNERSHIP_LOST));
    }

    std::shared_ptr<IEvsDisplay> pActiveDisplay = display_search->second.displayWeak.lock();
    if (pActiveDisplay) {
        return pActiveDisplay->getDisplayState(state);
    } else {
        *state = DisplayState::NOT_OPEN;
        return ScopedAStatus::fromServiceSpecificError(static_cast<int>(EvsResult::OWNERSHIP_LOST));
    }
}

ScopedAStatus EvsEnumerator::getDisplayIdList(std::vector<uint8_t>* list) {
    std::vector<uint8_t>& output = *list;
    if (sDisplayPortList.size() > 0) {
        output.resize(sDisplayPortList.size());
        unsigned i = 0;
        output[i++] = mInternalDisplayId & 0xFF;
        for (const auto& [port, id] : sDisplayPortList) {
            if (mInternalDisplayId != id) {
                output[i++] = port;
            }
        }
    }

    return ScopedAStatus::ok();
}

ScopedAStatus EvsEnumerator::isHardware(bool* flag) {
    *flag = true;
    return ScopedAStatus::ok();
}

void EvsEnumerator::notifyDeviceStatusChange(const std::string_view& deviceName,
                                             DeviceStatusType type) {
    std::lock_guard lock(sLock);
    if (!mCallback) {
        return;
    }

    std::vector<DeviceStatus> status{{.id = std::string(deviceName), .status = type}};
    if (!mCallback->deviceStatusChanged(status).isOk()) {
        LOG(WARNING) << "Failed to notify a device status change, name = " << deviceName
                     << ", type = " << static_cast<int>(type);
    }
}

ScopedAStatus EvsEnumerator::registerStatusCallback(
        const std::shared_ptr<IEvsEnumeratorStatusCallback>& callback) {
    std::lock_guard lock(sLock);
    if (mCallback) {
        LOG(INFO) << "Replacing an existing device status callback";
    }
    mCallback = callback;
    return ScopedAStatus::ok();
}

void EvsEnumerator::closeCamera_impl(const std::shared_ptr<IEvsCamera>& pCamera,
                                     const std::string& cameraId) {
    // Find the named camera
    CameraRecord* pRecord = findCameraById(cameraId);

    // Is the display being destroyed actually the one we think is active?
    if (!pRecord) {
        LOG(ERROR) << "Asked to close a camera whose name isn't recognized";
    } else {
        std::shared_ptr<EvsCameraBase> pActiveCamera = pRecord->activeInstance.lock();
        if (!pActiveCamera) {
            LOG(WARNING) << "Somehow a camera is being destroyed "
                         << "when the enumerator didn't know one existed";
        } else if (pActiveCamera != pCamera) {
            // This can happen if the camera was aggressively reopened,
            // orphaning this previous instance
            LOG(WARNING) << "Ignoring close of previously orphaned camera "
                         << "- why did a client steal?";
        } else {
            // Shutdown the active camera
            pActiveCamera->shutdown();
        }
    }

    return;
}

EvsEnumerator::CameraRecord* EvsEnumerator::findCameraById(const std::string& cameraId) {
    // Find the named camera
    auto found = sCameraList.find(cameraId);
    if (found != sCameraList.end()) {
        // Found a match!
        return &found->second;
    }

    // We didn't find a match
    return nullptr;
}

std::optional<EvsEnumerator::ActiveDisplays::DisplayInfo> EvsEnumerator::ActiveDisplays::popDisplay(
        int32_t id) {
    std::lock_guard lck(mMutex);
    const auto search = mIdToDisplay.find(id);
    if (search == mIdToDisplay.end()) {
        return std::nullopt;
    }
    const auto display_info = search->second;
    mIdToDisplay.erase(search);
    mDisplayToId.erase(display_info.internalDisplayRawAddr);
    return display_info;
}

std::optional<EvsEnumerator::ActiveDisplays::DisplayInfo> EvsEnumerator::ActiveDisplays::popDisplay(
        const std::shared_ptr<IEvsDisplay>& display) {
    const auto display_ptr_val = reinterpret_cast<uintptr_t>(display.get());
    std::lock_guard lck(mMutex);
    const auto display_to_id_search = mDisplayToId.find(display_ptr_val);
    if (display_to_id_search == mDisplayToId.end()) {
        LOG(ERROR) << "Unknown display.";
        return std::nullopt;
    }
    const auto id = display_to_id_search->second;
    const auto id_to_display_search = mIdToDisplay.find(id);
    mDisplayToId.erase(display_to_id_search);
    if (id_to_display_search == mIdToDisplay.end()) {
        LOG(ERROR) << "No correspsonding ID for the display, probably orphaned.";
        return std::nullopt;
    }
    const auto display_info = id_to_display_search->second;
    mIdToDisplay.erase(id);
    return display_info;
}

std::unordered_map<int32_t, EvsEnumerator::ActiveDisplays::DisplayInfo>
EvsEnumerator::ActiveDisplays::getAllDisplays() {
    std::lock_guard lck(mMutex);
    auto id_to_display_map_copy = mIdToDisplay;
    return id_to_display_map_copy;
}

bool EvsEnumerator::ActiveDisplays::tryInsert(int32_t id,
                                              const std::shared_ptr<EvsGlDisplay>& display) {
    std::lock_guard lck(mMutex);
    const auto display_ptr_val = reinterpret_cast<uintptr_t>(display.get());

    auto id_to_display_insert_result =
            mIdToDisplay.emplace(id, DisplayInfo{
                                             .id = id,
                                             .displayWeak = display,
                                             .internalDisplayRawAddr = display_ptr_val,
                                     });
    if (!id_to_display_insert_result.second) {
        return false;
    }
    auto display_to_id_insert_result = mDisplayToId.emplace(display_ptr_val, id);
    if (!display_to_id_insert_result.second) {
        mIdToDisplay.erase(id);
        return false;
    }
    return true;
}

ScopedAStatus EvsEnumerator::getUltrasonicsArrayList(
        [[maybe_unused]] std::vector<UltrasonicsArrayDesc>* list) {
    // TODO(b/149874793): Add implementation for EVS Manager and Sample driver
    return ScopedAStatus::ok();
}

ScopedAStatus EvsEnumerator::openUltrasonicsArray(
        [[maybe_unused]] const std::string& id,
        [[maybe_unused]] std::shared_ptr<IEvsUltrasonicsArray>* obj) {
    // TODO(b/149874793): Add implementation for EVS Manager and Sample driver
    return ScopedAStatus::ok();
}

ScopedAStatus EvsEnumerator::closeUltrasonicsArray(
        [[maybe_unused]] const std::shared_ptr<IEvsUltrasonicsArray>& obj) {
    // TODO(b/149874793): Add implementation for EVS Manager and Sample driver
    return ScopedAStatus::ok();
}

}  // namespace aidl::android::hardware::automotive::evs::implementation
