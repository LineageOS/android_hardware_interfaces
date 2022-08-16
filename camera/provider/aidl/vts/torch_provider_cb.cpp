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

#include "torch_provider_cb.h"

TorchProviderCb::TorchProviderCb(CameraAidlTest* parent) {
    mParent = parent;
}

ndk::ScopedAStatus TorchProviderCb::torchModeStatusChange(const std::string&,
                                                          TorchModeStatus newStatus) {
    std::lock_guard<std::mutex> l(mParent->mTorchLock);
    mParent->mTorchStatus = newStatus;
    mParent->mTorchCond.notify_one();
    return ndk::ScopedAStatus::ok();
}
ScopedAStatus TorchProviderCb::cameraDeviceStatusChange(
        const std::string&, ::aidl::android::hardware::camera::common::CameraDeviceStatus) {
    // Should not be called
    return ndk::ScopedAStatus::fromStatus(STATUS_UNKNOWN_TRANSACTION);
}
ScopedAStatus TorchProviderCb::physicalCameraDeviceStatusChange(
        const std::string&, const std::string&,
        ::aidl::android::hardware::camera::common::CameraDeviceStatus) {
    // Should not be called
    return ndk::ScopedAStatus::fromStatus(STATUS_UNKNOWN_TRANSACTION);
}
