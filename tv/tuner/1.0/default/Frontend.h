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

#ifndef ANDROID_HARDWARE_TV_TUNER_V1_0_FRONTEND_H_
#define ANDROID_HARDWARE_TV_TUNER_V1_0_FRONTEND_H_

#include <android/hardware/tv/tuner/1.0/IFrontend.h>
#include <android/hardware/tv/tuner/1.0/ITuner.h>

using namespace std;

namespace android {
namespace hardware {
namespace tv {
namespace tuner {
namespace V1_0 {
namespace implementation {

using ::android::hardware::tv::tuner::V1_0::FrontendId;
using ::android::hardware::tv::tuner::V1_0::FrontendType;
using ::android::hardware::tv::tuner::V1_0::IFrontend;
using ::android::hardware::tv::tuner::V1_0::IFrontendCallback;
using ::android::hardware::tv::tuner::V1_0::Result;

class Frontend : public IFrontend {
  public:
    Frontend();
    Frontend(FrontendType type, FrontendId id);

    virtual Return<Result> close() override;

    virtual Return<Result> setCallback(const sp<IFrontendCallback>& callback) override;

    virtual Return<Result> tune(const FrontendSettings& settings) override;

    virtual Return<Result> stopTune() override;

    FrontendType getFrontendType();

    FrontendId getFrontendId();

  private:
    virtual ~Frontend();
    sp<IFrontendCallback> mCallback;
    FrontendType mType = FrontendType::UNDEFINED;
    FrontendId mId = 0;
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace tuner
}  // namespace tv
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_TV_TUNER_V1_0_FRONTEND_H_
