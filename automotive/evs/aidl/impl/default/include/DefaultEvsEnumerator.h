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

#ifndef android_hardware_automotive_evs_aidl_impl_evshal_include_DefaultEvsHal_H_
#define android_hardware_automotive_evs_aidl_impl_evshal_include_DefaultEvsHal_H_

#include <aidl/android/hardware/automotive/evs/BnEvsEnumerator.h>

namespace aidl::android::hardware::automotive::evs::implementation {

class DefaultEvsEnumerator final
    : public ::aidl::android::hardware::automotive::evs::BnEvsEnumerator {
    ::ndk::ScopedAStatus isHardware(bool* flag) override;
    ::ndk::ScopedAStatus openCamera(
            const std::string& cameraId,
            const ::aidl::android::hardware::automotive::evs::Stream& streamConfig,
            std::shared_ptr<::aidl::android::hardware::automotive::evs::IEvsCamera>* obj) override;
    ::ndk::ScopedAStatus closeCamera(
            const std::shared_ptr<::aidl::android::hardware::automotive::evs::IEvsCamera>& obj)
            override;
    ::ndk::ScopedAStatus getCameraList(
            std::vector<::aidl::android::hardware::automotive::evs::CameraDesc>* list) override;
    ::ndk::ScopedAStatus getStreamList(
            const ::aidl::android::hardware::automotive::evs::CameraDesc& desc,
            std::vector<::aidl::android::hardware::automotive::evs::Stream>* _aidl_return) override;
    ::ndk::ScopedAStatus openDisplay(
            int8_t displayId,
            std::shared_ptr<::aidl::android::hardware::automotive::evs::IEvsDisplay>* obj) override;
    ::ndk::ScopedAStatus closeDisplay(
            const std::shared_ptr<::aidl::android::hardware::automotive::evs::IEvsDisplay>& obj)
            override;
    ::ndk::ScopedAStatus getDisplayIdList(std::vector<uint8_t>* list) override;
    ::ndk::ScopedAStatus getDisplayState(
            ::aidl::android::hardware::automotive::evs::DisplayState* state) override;
    ::ndk::ScopedAStatus registerStatusCallback(
            const std::shared_ptr<
                    ::aidl::android::hardware::automotive::evs::IEvsEnumeratorStatusCallback>&
                    callback) override;
    ::ndk::ScopedAStatus openUltrasonicsArray(
            const std::string& id,
            std::shared_ptr<::aidl::android::hardware::automotive::evs::IEvsUltrasonicsArray>* obj)
            override;
    ::ndk::ScopedAStatus closeUltrasonicsArray(
            const std::shared_ptr<::aidl::android::hardware::automotive::evs::IEvsUltrasonicsArray>&
                    arr) override;
    ::ndk::ScopedAStatus getUltrasonicsArrayList(
            std::vector<::aidl::android::hardware::automotive::evs::UltrasonicsArrayDesc>* list)
            override;
};

}  // namespace aidl::android::hardware::automotive::evs::implementation

#endif  // android_hardware_automotive_evs_aidl_impl_evshal_include_DefaultEvsHal_H_
