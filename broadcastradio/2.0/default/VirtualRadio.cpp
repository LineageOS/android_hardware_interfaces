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

#include <broadcastradio-utils-2x/Utils.h>

namespace android {
namespace hardware {
namespace broadcastradio {
namespace V2_0 {
namespace implementation {

using std::lock_guard;
using std::move;
using std::mutex;
using std::vector;
using utils::make_selector_amfm;

VirtualRadio gAmFmRadio(
    "AM/FM radio mock",
    {
        {make_selector_amfm(94900), "Wild 94.9", "Drake ft. Rihanna", "Too Good"},
        {make_selector_amfm(96500), "KOIT", "Celine Dion", "All By Myself"},
        {make_selector_amfm(97300), "Alice@97.3", "Drops of Jupiter", "Train"},
        {make_selector_amfm(99700), "99.7 Now!", "The Chainsmokers", "Closer"},
        {make_selector_amfm(101300), "101-3 KISS-FM", "Justin Timberlake", "Rock Your Body"},
        {make_selector_amfm(103700), "iHeart80s @ 103.7", "Michael Jackson", "Billie Jean"},
        {make_selector_amfm(106100), "106 KMEL", "Drake", "Marvins Room"},
    });

VirtualRadio::VirtualRadio(const std::string& name, const vector<VirtualProgram>& initialList)
    : mName(name), mPrograms(initialList) {}

std::string VirtualRadio::getName() const {
    return mName;
}

vector<VirtualProgram> VirtualRadio::getProgramList() const {
    lock_guard<mutex> lk(mMut);
    return mPrograms;
}

bool VirtualRadio::getProgram(const ProgramSelector& selector, VirtualProgram& programOut) const {
    lock_guard<mutex> lk(mMut);
    for (auto&& program : mPrograms) {
        if (utils::tunesTo(selector, program.selector)) {
            programOut = program;
            return true;
        }
    }
    return false;
}

}  // namespace implementation
}  // namespace V2_0
}  // namespace broadcastradio
}  // namespace hardware
}  // namespace android
