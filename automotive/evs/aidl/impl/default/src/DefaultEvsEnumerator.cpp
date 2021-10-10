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

// TODO(b/203661081): Remove below lines to disable compiler warnings.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

#define LOG_TAG "DefaultEvsEnumerator"

#include <DefaultEvsEnumerator.h>

namespace aidl::android::hardware::automotive::evs::implementation {

using ::ndk::ScopedAStatus;

ScopedAStatus DefaultEvsEnumerator::isHardware(bool* flag) {
    // This returns true always.
    *flag = true;
    return ScopedAStatus::ok();
}

ScopedAStatus DefaultEvsEnumerator::openCamera(const std::string& cameraId,
                                               const Stream& streamConfig,
                                               std::shared_ptr<IEvsCamera>* obj) {
    return ScopedAStatus::ok();
}

ScopedAStatus DefaultEvsEnumerator::closeCamera(const std::shared_ptr<IEvsCamera>& obj) {
    return ScopedAStatus::ok();
}

ScopedAStatus DefaultEvsEnumerator::getCameraList(std::vector<CameraDesc>* list) {
    return ScopedAStatus::ok();
}

ScopedAStatus DefaultEvsEnumerator::getStreamList(const CameraDesc& desc,
                                                  std::vector<Stream>* _aidl_return) {
    return ScopedAStatus::ok();
}

ScopedAStatus DefaultEvsEnumerator::openDisplay(int8_t displayId,
                                                std::shared_ptr<IEvsDisplay>* obj) {
    return ScopedAStatus::ok();
}

ScopedAStatus DefaultEvsEnumerator::closeDisplay(const std::shared_ptr<IEvsDisplay>& state) {
    return ScopedAStatus::ok();
}

ScopedAStatus DefaultEvsEnumerator::getDisplayIdList(std::vector<uint8_t>* list) {
    return ScopedAStatus::ok();
}

ScopedAStatus DefaultEvsEnumerator::getDisplayState(DisplayState* state) {
    return ScopedAStatus::ok();
}

ScopedAStatus DefaultEvsEnumerator::registerStatusCallback(
        const std::shared_ptr<IEvsEnumeratorStatusCallback>& callback) {
    return ScopedAStatus::ok();
}

ScopedAStatus DefaultEvsEnumerator::openUltrasonicsArray(
        const std::string& id, std::shared_ptr<IEvsUltrasonicsArray>* obj) {
    return ScopedAStatus::ok();
}

ScopedAStatus DefaultEvsEnumerator::closeUltrasonicsArray(
        const std::shared_ptr<IEvsUltrasonicsArray>& obj) {
    return ScopedAStatus::ok();
}

ScopedAStatus DefaultEvsEnumerator::getUltrasonicsArrayList(
        std::vector<UltrasonicsArrayDesc>* list) {
    return ScopedAStatus::ok();
}

}  // namespace aidl::android::hardware::automotive::evs::implementation

#pragma clang diagnostic pop
