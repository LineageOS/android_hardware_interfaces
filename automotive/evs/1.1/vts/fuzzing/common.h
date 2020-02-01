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

#define LOG_TAG "VtsHalEvsTest"
#define UNUSED(x) (void)(x)

const static char kEnumeratorName[] = "EvsEnumeratorHw";

#include <android/hardware/automotive/evs/1.1/IEvsEnumerator.h>
#include <fuzzer/FuzzedDataProvider.h>
#include <hidl/HidlTransportSupport.h>
#include <utils/StrongPointer.h>

using namespace ::android::hardware::automotive::evs::V1_1;

using ::android::sp;
using ::android::hardware::hidl_death_recipient;
using ::android::hardware::hidl_vec;

static sp<IEvsEnumerator> pEnumerator;
static std::vector<CameraDesc> cameraInfo;

class EvsDeathRecipient : public hidl_death_recipient {
  public:
    void serviceDied(uint64_t /*cookie*/,
                     const android::wp<::android::hidl::base::V1_0::IBase>& /*who*/) override {
        abort();
    }
};

void loadCameraList() {
    // SetUp() must run first!
    assert(pEnumerator != nullptr);

    // Get the camera list
    pEnumerator->getCameraList_1_1([](hidl_vec<CameraDesc> cameraList) {
        ALOGI("Camera list callback received %zu cameras", cameraList.size());
        cameraInfo.reserve(cameraList.size());
        for (auto&& cam : cameraList) {
            ALOGI("Found camera %s", cam.v1.cameraId.c_str());
            cameraInfo.push_back(cam);
        }
    });
}
