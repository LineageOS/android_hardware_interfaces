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

#ifndef ANDROID_HARDWARE_TV_TUNER_V1_0_TUNER_H_
#define ANDROID_HARDWARE_TV_TUNER_V1_0_TUNER_H_

#include <android/hardware/tv/tuner/1.0/ITuner.h>
#include "Frontend.h"

using namespace std;

namespace android {
namespace hardware {
namespace tv {
namespace tuner {
namespace V1_0 {
namespace implementation {

class Tuner : public ITuner {
  public:
    Tuner();
    virtual Return<void> getFrontendIds(getFrontendIds_cb _hidl_cb) override;

    virtual Return<void> openFrontendById(uint32_t frontendId,
                                          openFrontendById_cb _hidl_cb) override;

  private:
    virtual ~Tuner();
    // Static mFrontends array to maintain local frontends information
    vector<sp<Frontend>> mFrontends;
    // To maintain how many Frontends we have
    int mFrontendSize;
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace tuner
}  // namespace tv
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_TV_TUNER_V1_0_TUNER_H_
