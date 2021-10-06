/*
 * Copyright 2020 The Android Open Source Project
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

#ifndef ANDROID_HARDWARE_TV_TUNER_V1_1_FRONTEND_H_
#define ANDROID_HARDWARE_TV_TUNER_V1_1_FRONTEND_H_

#include <android/hardware/tv/tuner/1.1/IFrontend.h>
#include <fstream>
#include <iostream>
#include "Tuner.h"

using namespace std;

namespace android {
namespace hardware {
namespace tv {
namespace tuner {
namespace V1_0 {
namespace implementation {

class Tuner;

class Frontend : public V1_1::IFrontend {
  public:
    Frontend(FrontendType type, FrontendId id, sp<Tuner> tuner);

    virtual Return<Result> close() override;

    virtual Return<Result> setCallback(const sp<IFrontendCallback>& callback) override;

    virtual Return<Result> tune(const FrontendSettings& settings) override;

    virtual Return<Result> tune_1_1(const FrontendSettings& settings,
                                    const V1_1::FrontendSettingsExt1_1& settingsExt1_1) override;

    virtual Return<Result> stopTune() override;

    virtual Return<Result> scan(const FrontendSettings& settings, FrontendScanType type) override;

    virtual Return<Result> scan_1_1(const FrontendSettings& settings, FrontendScanType type,
                                    const V1_1::FrontendSettingsExt1_1& settingsExt1_1) override;

    virtual Return<Result> stopScan() override;

    virtual Return<void> getStatus(const hidl_vec<FrontendStatusType>& statusTypes,
                                   getStatus_cb _hidl_cb) override;

    virtual Return<void> getStatusExt1_1(
            const hidl_vec<V1_1::FrontendStatusTypeExt1_1>& statusTypes,
            V1_1::IFrontend::getStatusExt1_1_cb _hidl_cb) override;

    virtual Return<Result> setLna(bool bEnable) override;

    virtual Return<Result> setLnb(uint32_t lnb) override;

    virtual Return<void> linkCiCam(uint32_t ciCamId, linkCiCam_cb _hidl_cb) override;

    virtual Return<Result> unlinkCiCam(uint32_t ciCamId) override;

    FrontendType getFrontendType();

    FrontendId getFrontendId();

    string getSourceFile();

    bool isLocked();

  private:
    virtual ~Frontend();
    bool supportsSatellite();
    sp<IFrontendCallback> mCallback;
    sp<Tuner> mTunerService;
    FrontendType mType = FrontendType::UNDEFINED;
    FrontendId mId = 0;
    bool mIsLocked = false;
    uint32_t mCiCamId;

    std::ifstream mFrontendData;
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace tuner
}  // namespace tv
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_TV_TUNER_V1_1_FRONTEND_H_
