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

#include "common.h"

using ::android::hardware::automotive::evs::V1_0::DisplayDesc;
using ::android::hardware::camera::device::V3_2::Stream;
using IEvsCamera_1_1 = ::android::hardware::automotive::evs::V1_1::IEvsCamera;

extern "C" int LLVMFuzzerInitialize(int* argc, char*** argv) {
    UNUSED(argc);
    UNUSED(argv);
    pEnumerator = IEvsEnumerator::getService(kEnumeratorName);
    sp<EvsDeathRecipient> dr = new EvsDeathRecipient();

    pEnumerator->linkToDeath(dr, 0);

    loadCameraList();
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    FuzzedDataProvider fdp(data, size);

    std::vector<sp<IEvsCamera_1_1>> camList;
    Stream nullCfg = {};

    while (fdp.remaining_bytes() > 4) {
        switch (fdp.ConsumeIntegralInRange<uint32_t>(0, 3)) {
            case 0:  // open camera
            {
                uint32_t whichCam = fdp.ConsumeIntegralInRange<uint32_t>(0, cameraInfo.size() - 1);
                sp<IEvsCamera_1_1> pCam =
                        IEvsCamera_1_1::castFrom(pEnumerator->openCamera_1_1(
                                                         cameraInfo[whichCam].v1.cameraId, nullCfg))
                                .withDefault(nullptr);
                camList.emplace_back(pCam);
                break;
            }
            case 1:  // close camera
            {
                if (!camList.empty()) {
                    uint32_t whichCam = fdp.ConsumeIntegralInRange<uint32_t>(0, camList.size() - 1);
                    pEnumerator->closeCamera(camList[whichCam]);
                }
                break;
            }
            case 2:  // get camera info
            {
                if (!camList.empty()) {
                    uint32_t whichCam = fdp.ConsumeIntegralInRange<uint32_t>(0, camList.size() - 1);
                    camList[whichCam]->getCameraInfo_1_1([](CameraDesc desc) { UNUSED(desc); });
                }
                break;
            }
            case 3:  // setMaxFramesInFlight
            {
                if (!camList.empty()) {
                    uint32_t whichCam = fdp.ConsumeIntegralInRange<uint32_t>(0, camList.size() - 1);
                    int32_t numFrames = fdp.ConsumeIntegral<int32_t>();
                    camList[whichCam]->setMaxFramesInFlight(numFrames);
                }
                break;
            }
            default:
                break;
        }
    }

    return 0;
}
