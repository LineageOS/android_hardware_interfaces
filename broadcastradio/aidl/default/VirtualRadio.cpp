/*
 * Copyright (C) 2022 The Android Open Source Project
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
#include <broadcastradio-utils-aidl/Utils.h>

namespace aidl::android::hardware::broadcastradio {

using ::aidl::android::hardware::broadcastradio::utils::makeSelectorAmfm;
using ::aidl::android::hardware::broadcastradio::utils::makeSelectorDab;
using ::std::string;
using ::std::vector;

VirtualRadio::VirtualRadio(const string& name, const vector<VirtualProgram>& initialList)
    : mName(name), mPrograms(initialList) {
    sort(mPrograms.begin(), mPrograms.end());
}

string VirtualRadio::getName() const {
    return mName;
}

const vector<VirtualProgram>& VirtualRadio::getProgramList() const {
    return mPrograms;
}

bool VirtualRadio::getProgram(const ProgramSelector& selector, VirtualProgram* programOut) const {
    for (const auto& program : mPrograms) {
        if (utils::tunesTo(selector, program.selector)) {
            *programOut = program;
            return true;
        }
    }
    return false;
}

// get singleton of AMFM Virtual Radio
const VirtualRadio& VirtualRadio::getAmFmRadio() {
    // clang-format off
    static VirtualRadio amFmRadioMock(
        "AM/FM radio mock",
        {
            {makeSelectorAmfm(94900), "Wild 94.9", "Drake ft. Rihanna", "Too Good"},
            {makeSelectorAmfm(96500), "KOIT", "Celine Dion", "All By Myself"},
            {makeSelectorAmfm(97300), "Alice@97.3", "Drops of Jupiter", "Train"},
            {makeSelectorAmfm(99700), "99.7 Now!", "The Chainsmokers", "Closer"},
            {makeSelectorAmfm(101300), "101-3 KISS-FM", "Justin Timberlake", "Rock Your Body"},
            {makeSelectorAmfm(103700), "iHeart80s @ 103.7", "Michael Jackson", "Billie Jean"},
            {makeSelectorAmfm(106100), "106 KMEL", "Drake", "Marvins Room"},
            {makeSelectorAmfm(700), "700 AM", "Artist700", "Title700"},
            {makeSelectorAmfm(1700), "1700 AM", "Artist1700", "Title1700"},
        });
    // clang-format on
    return amFmRadioMock;
}

// get singleton of DAB Virtual Radio
const VirtualRadio& VirtualRadio::getDabRadio() {
    // clang-format off
    static VirtualRadio dabRadioMock(
        "DAB radio mock",
        {
            {makeSelectorDab(0xA00001u, 0x0001u), "BBC Radio 1", "Khalid", "Talk"},
            {makeSelectorDab(0xB00001u, 0x1001u), "Classic FM", "Jean Sibelius", "Andante Festivo"},
            {makeSelectorDab(0xB00002u, 0x1001u), "Absolute Radio", "Coldplay", "Clocks"},
        });
    // clang-format on
    return dabRadioMock;
}

}  // namespace aidl::android::hardware::broadcastradio
