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
#include "VirtualRadio.h"

namespace android {
namespace hardware {
namespace broadcastradio {
namespace V1_1 {
namespace implementation {

using std::lock_guard;
using std::move;
using std::mutex;
using std::vector;

vector<VirtualProgram> gInitialFmPrograms{
    {94900}, {96500}, {97300}, {99700}, {101300}, {103700}, {106100},
};

VirtualRadio::VirtualRadio(VirtualRadio&& o) : mPrograms(move(o.mPrograms)) {}

VirtualRadio::VirtualRadio(vector<VirtualProgram> initialList) : mPrograms(initialList) {}

vector<VirtualProgram> VirtualRadio::getProgramList() {
    lock_guard<mutex> lk(mMut);
    return mPrograms;
}

bool VirtualRadio::getProgram(uint32_t channel, VirtualProgram& programOut) {
    lock_guard<mutex> lk(mMut);
    for (auto&& program : mPrograms) {
        if (program.channel == channel) {
            programOut = program;
            return true;
        }
    }
    return false;
}

VirtualRadio make_fm_radio() {
    return VirtualRadio(gInitialFmPrograms);
}

}  // namespace implementation
}  // namespace V1_1
}  // namespace broadcastradio
}  // namespace hardware
}  // namespace android
