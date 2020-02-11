/*
 * Copyright (C) 2020 The Android Open Source Project
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

#include "SurroundViewService.h"

#include <utils/Log.h>

namespace android {
namespace hardware {
namespace automotive {
namespace sv {
namespace V1_0 {
namespace implementation {

const std::string kCameraIds[] = {"0", "1", "2", "3"};

Return<void> SurroundViewService::getCameraIds(getCameraIds_cb _hidl_cb) {
  std::vector<hidl_string> cameraIds = {kCameraIds[0], kCameraIds[1],
          kCameraIds[2], kCameraIds[3]};
  _hidl_cb(cameraIds);
  return android::hardware::Void();
}

Return<void> SurroundViewService::start2dSession(start2dSession_cb _hidl_cb) {
    ALOGD("SurroundViewService::start2dSession");
    if (mSurroundView2dSession != nullptr) {
        ALOGW("Only one 2d session is supported at the same time");
        _hidl_cb(nullptr, SvResult::INTERNAL_ERROR);
    } else {
        mSurroundView2dSession = new SurroundView2dSession();
        _hidl_cb(mSurroundView2dSession, SvResult::OK);
    }
    return android::hardware::Void();
}

Return<SvResult> SurroundViewService::stop2dSession(
    const sp<ISurroundView2dSession>& sv2dSession) {
    ALOGD("SurroundViewService::stop2dSession");
    if (sv2dSession != nullptr && sv2dSession == mSurroundView2dSession) {
        mSurroundView2dSession = nullptr;
        return SvResult::OK;
    } else {
        ALOGE("Invalid arg for stop2dSession");
        return SvResult::INVALID_ARG;
    }
}

Return<void> SurroundViewService::start3dSession(start3dSession_cb _hidl_cb) {
    ALOGD("SurroundViewService::start3dSession");
    if (mSurroundView3dSession != nullptr) {
        ALOGW("Only one 3d session is supported at the same time");
        _hidl_cb(nullptr, SvResult::INTERNAL_ERROR);
    } else {
        mSurroundView3dSession = new SurroundView3dSession();
        _hidl_cb(mSurroundView3dSession, SvResult::OK);
    }
    return android::hardware::Void();
}

Return<SvResult> SurroundViewService::stop3dSession(
    const sp<ISurroundView3dSession>& sv3dSession) {
    ALOGD("SurroundViewService::stop3dSession");
    if (sv3dSession != nullptr && sv3dSession == mSurroundView3dSession) {
        mSurroundView3dSession = nullptr;
        return SvResult::OK;
    } else {
        ALOGE("Invalid arg for stop3dSession");
        return SvResult::INVALID_ARG;
    }
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace sv
}  // namespace automotive
}  // namespace hardware
}  // namespace android

