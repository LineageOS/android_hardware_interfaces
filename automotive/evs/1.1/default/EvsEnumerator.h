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

#ifndef ANDROID_HARDWARE_AUTOMOTIVE_EVS_V1_1_EVSCAMERAENUMERATOR_H
#define ANDROID_HARDWARE_AUTOMOTIVE_EVS_V1_1_EVSCAMERAENUMERATOR_H

#include <android/hardware/automotive/evs/1.1/IEvsEnumerator.h>
#include <android/hardware/automotive/evs/1.1/IEvsCamera.h>

#include <list>

#include "ConfigManager.h"

using ::android::hardware::automotive::evs::V1_0::EvsResult;
using ::android::hardware::automotive::evs::V1_0::IEvsDisplay;
using ::android::hardware::automotive::evs::V1_0::DisplayState;
using IEvsCamera_1_0 = ::android::hardware::automotive::evs::V1_0::IEvsCamera;
using IEvsCamera_1_1 = ::android::hardware::automotive::evs::V1_1::IEvsCamera;
using CameraDesc_1_0 = ::android::hardware::automotive::evs::V1_0::CameraDesc;
using CameraDesc_1_1 = ::android::hardware::automotive::evs::V1_1::CameraDesc;


namespace android {
namespace hardware {
namespace automotive {
namespace evs {
namespace V1_1 {
namespace implementation {


class EvsCamera;    // from EvsCamera.h
class EvsDisplay;   // from EvsDisplay.h


class EvsEnumerator : public IEvsEnumerator {
public:
    // Methods from ::android::hardware::automotive::evs::V1_0::IEvsEnumerator follow.
    Return<void>                getCameraList(getCameraList_cb _hidl_cb)  override;
    Return<sp<IEvsCamera_1_0>>  openCamera(const hidl_string& cameraId) override;
    Return<void>                closeCamera(const ::android::sp<IEvsCamera_1_0>& carCamera)  override;
    Return<sp<IEvsDisplay>>     openDisplay()  override;
    Return<void>                closeDisplay(const ::android::sp<IEvsDisplay>& display)  override;
    Return<DisplayState>        getDisplayState()  override;

    // Methods from ::android::hardware::automotive::evs::V1_1::IEvsEnumerator follow.
    Return<void> getCameraList_1_1(getCameraList_1_1_cb _hidl_cb)  override;
    Return<sp<IEvsCamera_1_1>>  openCamera_1_1(const hidl_string& cameraId,
                                               const Stream& streamCfg) override;
    Return<bool> isHardware() override { return true; }

    // Implementation details
    EvsEnumerator();

private:
    // NOTE:  All members values are static so that all clients operate on the same state
    //        That is to say, this is effectively a singleton despite the fact that HIDL
    //        constructs a new instance for each client.
    struct CameraRecord {
        CameraDesc_1_1      desc;
        wp<EvsCamera>       activeInstance;

        CameraRecord(const char *cameraId) : desc() { desc.v1.cameraId = cameraId; }
    };

    static CameraRecord* findCameraById(const std::string& cameraId);

    static std::list<CameraRecord>   sCameraList;

    // Weak pointer. Object destructs if client dies.
    static wp<EvsDisplay>            sActiveDisplay;

    static unique_ptr<ConfigManager> sConfigManager;
};

} // namespace implementation
} // namespace V1_1
} // namespace evs
} // namespace automotive
} // namespace hardware
} // namespace android

#endif  // ANDROID_HARDWARE_AUTOMOTIVE_EVS_V1_1_EVSCAMERAENUMERATOR_H
