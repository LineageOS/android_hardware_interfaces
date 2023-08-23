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

#include "EvsVideoEmulatedCamera.h"

#include <aidl/android/hardware/automotive/evs/EvsResult.h>

#include <android-base/logging.h>

#include <cstddef>
#include <cstdint>

namespace aidl::android::hardware::automotive::evs::implementation {

EvsVideoEmulatedCamera::EvsVideoEmulatedCamera(Sigil, const char* deviceName,
                                               std::unique_ptr<ConfigManager::CameraInfo>& camInfo)
    : mCameraInfo(camInfo) {
    mDescription.id = deviceName;

    /* set camera metadata */
    if (camInfo) {
        uint8_t* ptr = reinterpret_cast<uint8_t*>(camInfo->characteristics);
        const size_t len = get_camera_metadata_size(camInfo->characteristics);
        mDescription.metadata.insert(mDescription.metadata.end(), ptr, ptr + len);
    }

    initializeParameters();
}

void EvsVideoEmulatedCamera::initializeParameters() {
    mParams.emplace(
            CameraParam::BRIGHTNESS,
            new CameraParameterDesc(/* min= */ 0, /* max= */ 255, /* step= */ 1, /* value= */ 255));
    mParams.emplace(
            CameraParam::CONTRAST,
            new CameraParameterDesc(/* min= */ 0, /* max= */ 255, /* step= */ 1, /* value= */ 255));
    mParams.emplace(
            CameraParam::SHARPNESS,
            new CameraParameterDesc(/* min= */ 0, /* max= */ 255, /* step= */ 1, /* value= */ 255));
}

::android::status_t EvsVideoEmulatedCamera::allocateOneFrame(buffer_handle_t* /* handle */) {
    LOG(FATAL) << __func__ << ": Not implemented yet.";
    return ::android::UNKNOWN_ERROR;
}

bool EvsVideoEmulatedCamera::startVideoStreamImpl_locked(
        const std::shared_ptr<evs::IEvsCameraStream>& /* receiver */,
        ndk::ScopedAStatus& /* status */, std::unique_lock<std::mutex>& /* lck */) {
    LOG(FATAL) << __func__ << ": Not implemented yet.";
    return false;
}

bool EvsVideoEmulatedCamera::stopVideoStreamImpl_locked(ndk::ScopedAStatus& /* status */,
                                                        std::unique_lock<std::mutex>& /* lck */) {
    LOG(FATAL) << __func__ << ": Not implemented yet.";
    return false;
}

ndk::ScopedAStatus EvsVideoEmulatedCamera::forcePrimaryClient(
        const std::shared_ptr<evs::IEvsDisplay>& /* display */) {
    /* Because EVS HW module reference implementation expects a single client at
     * a time, this returns a success code always.
     */
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus EvsVideoEmulatedCamera::getCameraInfo(evs::CameraDesc* _aidl_return) {
    *_aidl_return = mDescription;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus EvsVideoEmulatedCamera::getExtendedInfo(int32_t opaqueIdentifier,
                                                           std::vector<uint8_t>* value) {
    const auto it = mExtInfo.find(opaqueIdentifier);
    if (it == mExtInfo.end()) {
        return ndk::ScopedAStatus::fromServiceSpecificError(
                static_cast<int>(EvsResult::INVALID_ARG));
    } else {
        *value = mExtInfo[opaqueIdentifier];
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus EvsVideoEmulatedCamera::getIntParameter(evs::CameraParam id,
                                                           std::vector<int32_t>* value) {
    const auto it = mParams.find(id);
    if (it == mParams.end()) {
        return ndk::ScopedAStatus::fromServiceSpecificError(
                static_cast<int>(EvsResult::NOT_SUPPORTED));
    }
    value->push_back(it->second->value);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus EvsVideoEmulatedCamera::getIntParameterRange(evs::CameraParam id,
                                                                evs::ParameterRange* _aidl_return) {
    const auto it = mParams.find(id);
    if (it == mParams.end()) {
        return ndk::ScopedAStatus::fromServiceSpecificError(
                static_cast<int>(EvsResult::NOT_SUPPORTED));
    }
    _aidl_return->min = it->second->range.min;
    _aidl_return->max = it->second->range.max;
    _aidl_return->step = it->second->range.step;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus EvsVideoEmulatedCamera::getParameterList(
        std::vector<evs::CameraParam>* _aidl_return) {
    if (mCameraInfo) {
        _aidl_return->resize(mCameraInfo->controls.size());
        std::size_t idx = 0;
        for (const auto& [name, range] : mCameraInfo->controls) {
            (*_aidl_return)[idx++] = name;
        }
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus EvsVideoEmulatedCamera::getPhysicalCameraInfo(const std::string& /* deviceId */,
                                                                 evs::CameraDesc* _aidl_return) {
    return getCameraInfo(_aidl_return);
}

ndk::ScopedAStatus EvsVideoEmulatedCamera::setExtendedInfo(
        int32_t opaqueIdentifier, const std::vector<uint8_t>& opaqueValue) {
    mExtInfo.insert_or_assign(opaqueIdentifier, opaqueValue);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus EvsVideoEmulatedCamera::setIntParameter(evs::CameraParam id, int32_t value,
                                                           std::vector<int32_t>* effectiveValue) {
    const auto it = mParams.find(id);
    if (it == mParams.end()) {
        return ndk::ScopedAStatus::fromServiceSpecificError(
                static_cast<int>(EvsResult::NOT_SUPPORTED));
    }
    // Rounding down to the closest value.
    int32_t candidate = value / it->second->range.step * it->second->range.step;
    if (candidate < it->second->range.min || candidate > it->second->range.max) {
        return ndk::ScopedAStatus::fromServiceSpecificError(
                static_cast<int>(EvsResult::INVALID_ARG));
    }
    it->second->value = candidate;
    effectiveValue->push_back(candidate);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus EvsVideoEmulatedCamera::setPrimaryClient() {
    /* Because EVS HW module reference implementation expects a single client at
     * a time, this returns a success code always.
     */
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus EvsVideoEmulatedCamera::unsetPrimaryClient() {
    /* Because EVS HW module reference implementation expects a single client at
     * a time, there is no chance that this is called by the secondary client and
     * therefore returns a success code always.
     */
    return ndk::ScopedAStatus::ok();
}

std::shared_ptr<EvsVideoEmulatedCamera> EvsVideoEmulatedCamera::Create(const char* deviceName) {
    std::unique_ptr<ConfigManager::CameraInfo> nullCamInfo = nullptr;
    return Create(deviceName, nullCamInfo);
}

std::shared_ptr<EvsVideoEmulatedCamera> EvsVideoEmulatedCamera::Create(
        const char* deviceName, std::unique_ptr<ConfigManager::CameraInfo>& camInfo,
        const evs::Stream* /* streamCfg */) {
    std::shared_ptr<EvsVideoEmulatedCamera> c =
            ndk::SharedRefBase::make<EvsVideoEmulatedCamera>(Sigil{}, deviceName, camInfo);
    if (!c) {
        LOG(ERROR) << "Failed to instantiate EvsVideoEmulatedCamera.";
        return nullptr;
    }
    c->mDescription.vendorFlags = 0xFFFFFFFF;  // Arbitrary test value
    return c;
}

EvsVideoEmulatedCamera::~EvsVideoEmulatedCamera() {}

}  // namespace aidl::android::hardware::automotive::evs::implementation
