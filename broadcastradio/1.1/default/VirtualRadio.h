/*
 * Copyright (C) 2017 The Android Open Source Project
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
#ifndef ANDROID_HARDWARE_BROADCASTRADIO_V1_1_VIRTUALRADIO_H
#define ANDROID_HARDWARE_BROADCASTRADIO_V1_1_VIRTUALRADIO_H

#include "VirtualProgram.h"

#include <mutex>
#include <vector>

namespace android {
namespace hardware {
namespace broadcastradio {
namespace V1_1 {
namespace implementation {

class VirtualRadio {
   public:
    VirtualRadio(VirtualRadio&& o);
    VirtualRadio(std::vector<VirtualProgram> initialList);

    std::vector<VirtualProgram> getProgramList();
    bool getProgram(const ProgramSelector& selector, VirtualProgram& program);

   private:
    std::mutex mMut;
    std::vector<VirtualProgram> mPrograms;
};

VirtualRadio make_fm_radio();

}  // namespace implementation
}  // namespace V1_1
}  // namespace broadcastradio
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_BROADCASTRADIO_V1_1_VIRTUALRADIO_H
