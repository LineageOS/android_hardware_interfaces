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
#include <unordered_set>

namespace aidl::android::hardware::broadcastradio {

using ::aidl::android::hardware::broadcastradio::utils::makeSelectorAmfm;
using ::aidl::android::hardware::broadcastradio::utils::makeSelectorDab;
using ::aidl::android::hardware::broadcastradio::utils::makeSelectorHd;
using ::std::string;
using ::std::unordered_set;
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
    for (auto it = mPrograms.begin(); it != mPrograms.end(); it++) {
        if (!utils::tunesTo(selector, it->selector)) {
            continue;
        }
        auto firstMatchIt = it;
        if (utils::hasAmFmFrequency(it->selector)) {
            uint32_t channelFreq = utils::getAmFmFrequency(it->selector);
            it++;
            while (it != mPrograms.end() && utils::hasAmFmFrequency(it->selector) &&
                   utils::getAmFmFrequency(it->selector) == channelFreq) {
                if (it->selector == selector) {
                    *programOut = *it;
                    return true;
                }
                it++;
            }
        }
        *programOut = *firstMatchIt;
        return true;
    }
    return false;
}

vector<IdentifierType> VirtualRadio::getSupportedIdentifierTypes() const {
    unordered_set<IdentifierType> supportedIdentifierTypeSet;
    for (const auto& program : mPrograms) {
        IdentifierType type = program.selector.primaryId.type;
        if (supportedIdentifierTypeSet.count(type)) {
            continue;
        }
        supportedIdentifierTypeSet.insert(type);
    }
    vector<IdentifierType> supportedIdentifierTypes(supportedIdentifierTypeSet.begin(),
                                                    supportedIdentifierTypeSet.end());
    return supportedIdentifierTypes;
}

// get singleton of AMFM Virtual Radio
const VirtualRadio& VirtualRadio::getAmFmRadio() {
    // clang-format off
    static VirtualRadio amFmRadioMock(
        "AM/FM radio mock",
        {
            {makeSelectorAmfm(/* frequency= */ 94900u), "Wild 94.9", "Drake ft. Rihanna",
                "Too Good"},
            {makeSelectorAmfm(/* frequency= */ 96500u), "KOIT", "Celine Dion", "All By Myself"},
            {makeSelectorAmfm(/* frequency= */ 101300u), "101-3 KISS-FM", "Justin Timberlake",
                "Rock Your Body"},
            {makeSelectorAmfm(/* frequency= */ 103700u), "iHeart80s @ 103.7", "Michael Jackson",
                "Billie Jean"},
            {makeSelectorAmfm(/* frequency= */ 106100u), "106 KMEL", "Drake", "Marvins Room"},
            {makeSelectorAmfm(/* frequency= */ 560u), "Talk Radio 560 KSFO", "Artist560", "Title560"},
            {makeSelectorAmfm(/* frequency= */ 680u), "KNBR 680", "Artist680", "Title680"},
            {makeSelectorAmfm(/* frequency= */ 97300u), "Alice@97.3", "Drops of Jupiter", "Train"},
            {makeSelectorAmfm(/* frequency= */ 99700u), "99.7 Now!", "The Chainsmokers", "Closer"},
            {makeSelectorHd(/* stationId= */ 0xA0000001u, /* subChannel= */ 0u, /* frequency= */ 97700u),
                "K-LOVE", "ArtistHd0", "TitleHd0"},
            {makeSelectorHd(/* stationId= */ 0xA0000001u, /* subChannel= */ 1u, /* frequency= */ 97700u),
                "Air1", "ArtistHd1", "TitleHd1"},
            {makeSelectorHd(/* stationId= */ 0xA0000001u, /* subChannel= */ 2u, /* frequency= */ 97700u),
                "K-LOVE Classics", "ArtistHd2", "TitleHd2"},
            {makeSelectorHd(/* stationId= */ 0xA0000001u, /* subChannel= */ 0u, /* frequency= */ 98500u),
                "98.5-1 South Bay's Classic Rock", "ArtistHd0", "TitleHd0"},
            {makeSelectorHd(/* stationId= */ 0xA0000001u, /* subChannel= */ 1u, /* frequency= */ 98500u),
                "Highway 1 - Different", "ArtistHd1", "TitleHd1"},
            {makeSelectorHd(/* stationId= */ 0xB0000001u, /* subChannel= */ 0u, /* frequency= */ 1170u),
                "KLOK", "ArtistHd1", "TitleHd1"},
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
            {makeSelectorDab(/* sidExt= */ 0x0E10000C221u, /* ensemble= */ 0xCE15u,
                /* freq= */ 225648u), "BBC Radio 1", "Khalid", "Talk"},
            {makeSelectorDab(/* sidExt= */ 0x0E10000C222u, /* ensemble= */ 0xCE15u,
                    /* freq= */ 225648u), "BBC Radio 2", "Khalid", "Talk"},
            {makeSelectorDab(/* sidExt= */ 0xE10000C224u, /* ensemble= */ 0xCE15u,
                    /* freq= */ 225648u), "BBC Radio 4", "ArtistBBC1", "TitleCountry1"},
            {makeSelectorDab(/* sidExt= */ 0x1E10000C224u, /* ensemble= */ 0xCE15u,
                    /* freq= */ 225648u), "BBC Radio 4 LW", "ArtistBBC2", "TitleCountry2"},
            {makeSelectorDab(/* sidExt= */ 0x0E10000C21Au, /* ensemble= */ 0xC181u,
                /* freq= */ 222064u), "Classic FM", "Jean Sibelius", "Andante Festivo"},
            {makeSelectorDab(/* sidExt= */ 0x0E10000C1C0u, /* ensemble= */ 0xC181u,
                /* freq= */ 223936u), "Absolute Radio", "Coldplay", "Clocks"},
            {makeSelectorDab(/* sidExt= */ 0x0E10000C1C0u, /* ensemble= */ 0xC181u,
                /* freq= */ 222064u), "Absolute Radio", "Coldplay", "Clocks"},
            {makeSelectorDab(/* sidExt= */ 0x0E10000CCE7u, /* ensemble= */ 0xC19Du,
                    /* freq= */ 218640u), "Absolute Radio Country", "ArtistCountry1", "TitleCountry1"},
            {makeSelectorDab(/* sidExt= */ 0x0E10000CCE7u, /* ensemble= */ 0xC1A0u,
                    /* freq= */ 218640u), "Absolute Radio Country", "ArtistCountry2", "TitleCountry2"},
        });
    // clang-format on
    return dabRadioMock;
}

}  // namespace aidl::android::hardware::broadcastradio
