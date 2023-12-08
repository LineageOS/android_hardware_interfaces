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

#define LOG_TAG "BcRadioAidlDef.utils"

#include "broadcastradio-utils-aidl/Utils.h"

#include <android-base/logging.h>
#include <android-base/parseint.h>
#include <android-base/strings.h>

#include <math/HashCombine.h>

namespace aidl::android::hardware::broadcastradio {

namespace utils {

namespace {

using ::android::base::EqualsIgnoreCase;
using ::std::vector;

const int64_t kValueForNotFoundIdentifier = 0;

bool bothHaveId(const ProgramSelector& a, const ProgramSelector& b, const IdentifierType& type) {
    return hasId(a, type) && hasId(b, type);
}

bool haveEqualIds(const ProgramSelector& a, const ProgramSelector& b, const IdentifierType& type) {
    if (!bothHaveId(a, b, type)) {
        return false;
    }
    /* We should check all Ids of a given type (ie. other AF),
     * but it doesn't matter for default implementation.
     */
    return getId(a, type) == getId(b, type);
}

bool maybeGetId(const ProgramSelector& sel, const IdentifierType& type, int64_t* val) {
    // iterate through primaryId and secondaryIds
    for (auto it = begin(sel); it != end(sel); it++) {
        if (it->type == type) {
            if (val != nullptr) {
                *val = it->value;
            }
            return true;
        }
    }

    return false;
}

}  // namespace

IdentifierIterator::IdentifierIterator(const ProgramSelector& sel) : IdentifierIterator(sel, 0) {}

IdentifierIterator::IdentifierIterator(const ProgramSelector& sel, size_t pos)
    : mSel(sel), mPos(pos) {}

const IdentifierIterator IdentifierIterator::operator++(int) {
    IdentifierIterator i = *this;
    mPos++;
    return i;
}

IdentifierIterator& IdentifierIterator::operator++() {
    ++mPos;
    return *this;
}

IdentifierIterator::refType IdentifierIterator::operator*() const {
    if (mPos == 0) {
        return getSelector().primaryId;
    }

    // mPos is 1-based for secondary identifiers
    DCHECK(mPos <= getSelector().secondaryIds.size());
    return getSelector().secondaryIds[mPos - 1];
}

bool IdentifierIterator::operator==(const IdentifierIterator& rhs) const {
    // Check, if both iterators points at the same selector.
    if (reinterpret_cast<intptr_t>(&getSelector()) !=
        reinterpret_cast<intptr_t>(&rhs.getSelector())) {
        return false;
    }

    return mPos == rhs.mPos;
}

int32_t resultToInt(Result result) {
    return static_cast<int32_t>(result);
}

FrequencyBand getBand(int64_t freq) {
    // keep in sync with
    // frameworks/base/services/core/java/com/android/server/broadcastradio/aidl/Utils.java
    if (freq < 30) return FrequencyBand::UNKNOWN;
    if (freq < 500) return FrequencyBand::AM_LW;
    if (freq < 1705) return FrequencyBand::AM_MW;
    if (freq < 30000) return FrequencyBand::AM_SW;
    if (freq < 60000) return FrequencyBand::UNKNOWN;
    if (freq < 110000) return FrequencyBand::FM;
    return FrequencyBand::UNKNOWN;
}

bool tunesTo(const ProgramSelector& a, const ProgramSelector& b) {
    IdentifierType type = b.primaryId.type;

    switch (type) {
        case IdentifierType::HD_STATION_ID_EXT:
        case IdentifierType::RDS_PI:
        case IdentifierType::AMFM_FREQUENCY_KHZ:
            if (haveEqualIds(a, b, IdentifierType::HD_STATION_ID_EXT)) return true;
            if (haveEqualIds(a, b, IdentifierType::RDS_PI)) return true;
            if (getHdSubchannel(b) != 0) {  // supplemental program services
                return false;
            }
            return haveEqualIds(a, b, IdentifierType::AMFM_FREQUENCY_KHZ) ||
                   (b.primaryId.type == IdentifierType::HD_STATION_ID_EXT &&
                    static_cast<uint32_t>(getId(a, IdentifierType::AMFM_FREQUENCY_KHZ)) ==
                            getAmFmFrequency(b));
        case IdentifierType::DAB_SID_EXT:
            if (!haveEqualIds(a, b, IdentifierType::DAB_SID_EXT)) {
                return false;
            }
            if (hasId(a, IdentifierType::DAB_ENSEMBLE) &&
                !haveEqualIds(a, b, IdentifierType::DAB_ENSEMBLE)) {
                return false;
            }
            if (hasId(a, IdentifierType::DAB_FREQUENCY_KHZ) &&
                !haveEqualIds(a, b, IdentifierType::DAB_FREQUENCY_KHZ)) {
                return false;
            }
            return true;
        case IdentifierType::DRMO_SERVICE_ID:
            return haveEqualIds(a, b, IdentifierType::DRMO_SERVICE_ID);
        case IdentifierType::SXM_SERVICE_ID:
            return haveEqualIds(a, b, IdentifierType::SXM_SERVICE_ID);
        default:  // includes all vendor types
            LOG(WARNING) << "unsupported program type: " << toString(type);
            return false;
    }
}

bool hasId(const ProgramSelector& sel, const IdentifierType& type) {
    return maybeGetId(sel, type, /* val */ nullptr);
}

int64_t getId(const ProgramSelector& sel, const IdentifierType& type) {
    int64_t val;

    if (maybeGetId(sel, type, &val)) {
        return val;
    }

    LOG(WARNING) << "identifier not found: " << toString(type);
    return kValueForNotFoundIdentifier;
}

int64_t getId(const ProgramSelector& sel, const IdentifierType& type, int64_t defaultValue) {
    if (!hasId(sel, type)) {
        return defaultValue;
    }
    return getId(sel, type);
}

vector<int> getAllIds(const ProgramSelector& sel, const IdentifierType& type) {
    vector<int> ret;

    // iterate through primaryId and secondaryIds
    for (auto it = begin(sel); it != end(sel); it++) {
        if (it->type == type) {
            ret.push_back(it->value);
        }
    }

    return ret;
}

bool isSupported(const Properties& prop, const ProgramSelector& sel) {
    for (auto it = prop.supportedIdentifierTypes.begin(); it != prop.supportedIdentifierTypes.end();
         it++) {
        if (hasId(sel, *it)) {
            return true;
        }
    }
    return false;
}

bool isValid(const ProgramIdentifier& id) {
    uint64_t val = static_cast<uint64_t>(id.value);
    bool valid = true;

    auto expect = [&valid](bool condition, const std::string& message) {
        if (!condition) {
            valid = false;
            LOG(ERROR) << "identifier not valid, expected " << message;
        }
    };

    switch (id.type) {
        case IdentifierType::INVALID:
            expect(false, "IdentifierType::INVALID");
            break;
        case IdentifierType::DAB_FREQUENCY_KHZ:
            expect(val > 100000u, "f > 100MHz");
            [[fallthrough]];
        case IdentifierType::AMFM_FREQUENCY_KHZ:
        case IdentifierType::DRMO_FREQUENCY_KHZ:
            expect(val > 100u, "f > 100kHz");
            expect(val < 10000000u, "f < 10GHz");
            break;
        case IdentifierType::RDS_PI:
            expect(val != 0u, "RDS PI != 0");
            expect(val <= 0xFFFFu, "16bit id");
            break;
        case IdentifierType::HD_STATION_ID_EXT: {
            uint64_t stationId = val & 0xFFFFFFFF;  // 32bit
            val >>= 32;
            uint64_t subchannel = val & 0xF;  // 4bit
            val >>= 4;
            uint64_t freq = val & 0x3FFFF;  // 18bit
            expect(stationId != 0u, "HD station id != 0");
            expect(subchannel < 8u, "HD subch < 8");
            expect(freq > 100u, "f > 100kHz");
            expect(freq < 10000000u, "f < 10GHz");
            break;
        }
        case IdentifierType::HD_STATION_NAME: {
            while (val > 0) {
                char ch = static_cast<char>(val & 0xFF);
                val >>= 8;
                expect((ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'Z'),
                       "HD_STATION_NAME does not match [A-Z0-9]+");
            }
            break;
        }
        case IdentifierType::DAB_SID_EXT: {
            uint64_t sid = val & 0xFFFFFFFF;  // 32bit
            val >>= 32;
            uint64_t ecc = val & 0xFF;  // 8bit
            expect(sid != 0u, "DAB SId != 0");
            expect(ecc >= 0xA0u && ecc <= 0xF6u, "Invalid ECC, see ETSI TS 101 756 V2.1.1");
            break;
        }
        case IdentifierType::DAB_ENSEMBLE:
            expect(val != 0u, "DAB ensemble != 0");
            expect(val <= 0xFFFFu, "16bit id");
            break;
        case IdentifierType::DAB_SCID:
            expect(val > 0xFu, "12bit SCId (not 4bit SCIdS)");
            expect(val <= 0xFFFu, "12bit id");
            break;
        case IdentifierType::DRMO_SERVICE_ID:
            expect(val != 0u, "DRM SId != 0");
            expect(val <= 0xFFFFFFu, "24bit id");
            break;
        case IdentifierType::SXM_SERVICE_ID:
            expect(val != 0u, "SXM SId != 0");
            expect(val <= 0xFFFFFFFFu, "32bit id");
            break;
        case IdentifierType::SXM_CHANNEL:
            expect(val < 1000u, "SXM channel < 1000");
            break;
        default:
            expect(id.type >= IdentifierType::VENDOR_START && id.type <= IdentifierType::VENDOR_END,
                   "Undefined identifier type");
            break;
    }

    return valid;
}

bool isValid(const ProgramSelector& sel) {
    if (sel.primaryId.type != IdentifierType::AMFM_FREQUENCY_KHZ &&
        sel.primaryId.type != IdentifierType::RDS_PI &&
        sel.primaryId.type != IdentifierType::HD_STATION_ID_EXT &&
        sel.primaryId.type != IdentifierType::DAB_SID_EXT &&
        sel.primaryId.type != IdentifierType::DRMO_SERVICE_ID &&
        sel.primaryId.type != IdentifierType::SXM_SERVICE_ID &&
        (sel.primaryId.type < IdentifierType::VENDOR_START ||
         sel.primaryId.type > IdentifierType::VENDOR_END)) {
        return false;
    }
    return isValid(sel.primaryId);
}

ProgramIdentifier makeIdentifier(IdentifierType type, int64_t value) {
    return {type, value};
}

ProgramSelector makeSelectorAmfm(uint32_t frequency) {
    ProgramSelector sel = {};
    sel.primaryId = makeIdentifier(IdentifierType::AMFM_FREQUENCY_KHZ, frequency);
    return sel;
}

ProgramSelector makeSelectorDab(uint64_t sidExt) {
    ProgramSelector sel = {};
    sel.primaryId = makeIdentifier(IdentifierType::DAB_SID_EXT, sidExt);
    return sel;
}

ProgramSelector makeSelectorHd(uint64_t stationId, uint64_t subChannel, uint64_t frequency) {
    ProgramSelector sel = {};
    uint64_t sidExt = stationId | (subChannel << 32) | (frequency << 36);
    sel.primaryId = makeIdentifier(IdentifierType::HD_STATION_ID_EXT, sidExt);
    return sel;
}

ProgramSelector makeSelectorDab(uint64_t sidExt, uint32_t ensemble, uint64_t freq) {
    ProgramSelector sel = {};
    sel.primaryId = makeIdentifier(IdentifierType::DAB_SID_EXT, sidExt);
    vector<ProgramIdentifier> secondaryIds = {
            makeIdentifier(IdentifierType::DAB_ENSEMBLE, ensemble),
            makeIdentifier(IdentifierType::DAB_FREQUENCY_KHZ, freq)};
    sel.secondaryIds = std::move(secondaryIds);
    return sel;
}

bool satisfies(const ProgramFilter& filter, const ProgramSelector& sel) {
    if (filter.identifierTypes.size() > 0) {
        auto typeEquals = [](const ProgramIdentifier& id, IdentifierType type) {
            return id.type == type;
        };
        auto it = std::find_first_of(begin(sel), end(sel), filter.identifierTypes.begin(),
                                     filter.identifierTypes.end(), typeEquals);
        if (it == end(sel)) {
            return false;
        }
    }

    if (filter.identifiers.size() > 0) {
        auto it = std::find_first_of(begin(sel), end(sel), filter.identifiers.begin(),
                                     filter.identifiers.end());
        if (it == end(sel)) {
            return false;
        }
    }

    return true;
}

bool ProgramSelectorComparator::operator()(const ProgramSelector& lhs,
                                           const ProgramSelector& rhs) const {
    if ((utils::hasId(lhs, IdentifierType::AMFM_FREQUENCY_KHZ) ||
         lhs.primaryId.type == IdentifierType::HD_STATION_ID_EXT) &&
        (utils::hasId(rhs, IdentifierType::AMFM_FREQUENCY_KHZ) ||
         rhs.primaryId.type == IdentifierType::HD_STATION_ID_EXT)) {
        uint32_t freq1 = utils::getAmFmFrequency(lhs);
        int subChannel1 = lhs.primaryId.type == IdentifierType::HD_STATION_ID_EXT
                                  ? utils::getHdSubchannel(lhs)
                                  : 0;
        uint32_t freq2 = utils::getAmFmFrequency(rhs);
        int subChannel2 = rhs.primaryId.type == IdentifierType::HD_STATION_ID_EXT
                                  ? utils::getHdSubchannel(rhs)
                                  : 0;
        return freq1 < freq2 || (freq1 == freq2 && (lhs.primaryId.type < rhs.primaryId.type ||
                                                    subChannel1 < subChannel2));
    }
    if (lhs.primaryId.type == IdentifierType::DAB_SID_EXT &&
        rhs.primaryId.type == IdentifierType::DAB_SID_EXT) {
        uint64_t dabFreq1 = utils::getId(lhs, IdentifierType::DAB_FREQUENCY_KHZ);
        uint64_t dabFreq2 = utils::getId(rhs, IdentifierType::DAB_FREQUENCY_KHZ);
        if (dabFreq1 != dabFreq2) {
            return dabFreq1 < dabFreq2;
        }
        uint32_t ecc1 = utils::getDabEccCode(lhs);
        uint32_t ecc2 = utils::getDabEccCode(rhs);
        if (ecc1 != ecc2) {
            return ecc1 < ecc2;
        }
        uint64_t dabEnsemble1 = utils::getId(lhs, IdentifierType::DAB_ENSEMBLE);
        uint64_t dabEnsemble2 = utils::getId(rhs, IdentifierType::DAB_ENSEMBLE);
        if (dabEnsemble1 != dabEnsemble2) {
            return dabEnsemble1 < dabEnsemble2;
        }
        uint32_t sId1 = utils::getDabSId(lhs);
        uint32_t sId2 = utils::getDabSId(rhs);
        return sId1 < sId2 || (sId1 == sId2 && utils::getDabSCIdS(lhs) < utils::getDabSCIdS(rhs));
    }

    if (lhs.primaryId.type != rhs.primaryId.type) {
        return lhs.primaryId.type < rhs.primaryId.type;
    }
    return lhs.primaryId.value < rhs.primaryId.value;
}

bool ProgramInfoComparator::operator()(const ProgramInfo& lhs, const ProgramInfo& rhs) const {
    return ProgramSelectorComparator()(lhs.selector, rhs.selector);
}

size_t ProgramInfoHasher::operator()(const ProgramInfo& info) const {
    const ProgramIdentifier& id = info.selector.primaryId;

    // This is not the best hash implementation, but good enough for default HAL
    // implementation and tests.
    size_t h = 0;
    ::android::hashCombineSingle(h, id.type);
    ::android::hashCombineSingle(h, id.value);
    return h;
}

bool ProgramInfoKeyEqual::operator()(const ProgramInfo& info1, const ProgramInfo& info2) const {
    const ProgramIdentifier& id1 = info1.selector.primaryId;
    const ProgramIdentifier& id2 = info2.selector.primaryId;
    return id1.type == id2.type && id1.value == id2.value;
}

void updateProgramList(const ProgramListChunk& chunk, ProgramInfoSet* list) {
    if (chunk.purge) {
        list->clear();
    }

    list->insert(chunk.modified.begin(), chunk.modified.end());

    if (!chunk.removed.has_value()) {
        return;
    }

    for (auto& id : chunk.removed.value()) {
        if (id.has_value()) {
            ProgramInfo info = {};
            info.selector.primaryId = id.value();
            list->erase(info);
        }
    }
}

std::optional<std::string> getMetadataString(const ProgramInfo& info, const Metadata::Tag& tag) {
    auto isRdsPs = [tag](const Metadata& item) { return item.getTag() == tag; };

    auto it = std::find_if(info.metadata.begin(), info.metadata.end(), isRdsPs);
    if (it == info.metadata.end()) {
        return std::nullopt;
    }

    std::string metadataString;
    switch (it->getTag()) {
        case Metadata::rdsPs:
            metadataString = it->get<Metadata::rdsPs>();
            break;
        case Metadata::rdsPty:
            metadataString = std::to_string(it->get<Metadata::rdsPty>());
            break;
        case Metadata::rbdsPty:
            metadataString = std::to_string(it->get<Metadata::rbdsPty>());
            break;
        case Metadata::rdsRt:
            metadataString = it->get<Metadata::rdsRt>();
            break;
        case Metadata::songTitle:
            metadataString = it->get<Metadata::songTitle>();
            break;
        case Metadata::songArtist:
            metadataString = it->get<Metadata::songArtist>();
            break;
        case Metadata::songAlbum:
            metadataString = it->get<Metadata::songAlbum>();
            break;
        case Metadata::stationIcon:
            metadataString = std::to_string(it->get<Metadata::stationIcon>());
            break;
        case Metadata::albumArt:
            metadataString = std::to_string(it->get<Metadata::albumArt>());
            break;
        case Metadata::programName:
            metadataString = it->get<Metadata::programName>();
            break;
        case Metadata::dabEnsembleName:
            metadataString = it->get<Metadata::dabEnsembleName>();
            break;
        case Metadata::dabEnsembleNameShort:
            metadataString = it->get<Metadata::dabEnsembleNameShort>();
            break;
        case Metadata::dabServiceName:
            metadataString = it->get<Metadata::dabServiceName>();
            break;
        case Metadata::dabServiceNameShort:
            metadataString = it->get<Metadata::dabServiceNameShort>();
            break;
        case Metadata::dabComponentName:
            metadataString = it->get<Metadata::dabComponentName>();
            break;
        case Metadata::dabComponentNameShort:
            metadataString = it->get<Metadata::dabComponentNameShort>();
            break;
        default:
            LOG(ERROR) << "Metadata " << it->toString() << " is not converted.";
            return std::nullopt;
    }
    return metadataString;
}

ProgramIdentifier makeHdRadioStationName(const std::string& name) {
    constexpr size_t maxlen = 8;

    std::string shortName;
    shortName.reserve(maxlen);

    const auto& loc = std::locale::classic();
    for (const char& ch : name) {
        if (!std::isalnum(ch, loc)) {
            continue;
        }
        shortName.push_back(std::toupper(ch, loc));
        if (shortName.length() >= maxlen) {
            break;
        }
    }

    // Short name is converted to HD_STATION_NAME by encoding each char into its ASCII value in
    // in little-endian order. For example, "Abc" is converted to 0x434241.
    int64_t val = 0;
    for (auto rit = shortName.rbegin(); rit != shortName.rend(); ++rit) {
        val <<= 8;
        val |= static_cast<char>(*rit);
    }

    return makeIdentifier(IdentifierType::HD_STATION_NAME, val);
}

IdentifierType getType(int typeAsInt) {
    return static_cast<IdentifierType>(typeAsInt);
}

uint32_t getDabSId(const ProgramSelector& sel) {
    int64_t dabSidExt = getId(sel, IdentifierType::DAB_SID_EXT, /* defaultValue */ 0);
    return static_cast<uint32_t>(dabSidExt & 0xFFFFFFFF);
}

int getDabEccCode(const ProgramSelector& sel) {
    int64_t dabSidExt = getId(sel, IdentifierType::DAB_SID_EXT, /* defaultValue */ 0);
    return static_cast<uint32_t>((dabSidExt >> 32) & 0xFF);
}

int getDabSCIdS(const ProgramSelector& sel) {
    int64_t dabSidExt = getId(sel, IdentifierType::DAB_SID_EXT, /* defaultValue */ 0);
    return static_cast<uint32_t>((dabSidExt >> 40) & 0xF);
}

int getHdSubchannel(const ProgramSelector& sel) {
    int64_t hdSidExt = getId(sel, IdentifierType::HD_STATION_ID_EXT, kValueForNotFoundIdentifier);
    hdSidExt >>= 32;        // Station ID number
    return hdSidExt & 0xF;  // HD Radio subchannel
}

uint32_t getHdFrequency(const ProgramSelector& sel) {
    int64_t hdSidExt = getId(sel, IdentifierType::HD_STATION_ID_EXT, kValueForNotFoundIdentifier);
    if (hdSidExt == kValueForNotFoundIdentifier) {
        return kValueForNotFoundIdentifier;
    }
    return static_cast<uint32_t>((hdSidExt >> 36) & 0x3FFFF);  // HD Radio subchannel
}

bool hasAmFmFrequency(const ProgramSelector& sel) {
    return hasId(sel, IdentifierType::AMFM_FREQUENCY_KHZ) ||
           sel.primaryId.type == IdentifierType::HD_STATION_ID_EXT;
}

uint32_t getAmFmFrequency(const ProgramSelector& sel) {
    if (hasId(sel, IdentifierType::AMFM_FREQUENCY_KHZ)) {
        return static_cast<uint32_t>(getId(sel, IdentifierType::AMFM_FREQUENCY_KHZ));
    }
    return getHdFrequency(sel);
}

bool parseArgInt(const std::string& s, int* out) {
    return ::android::base::ParseInt(s, out);
}

bool parseArgLong(const std::string& s, long* out) {
    return ::android::base::ParseInt(s, out);
}

bool parseArgBool(const std::string& s, bool* out) {
    if (EqualsIgnoreCase(s, "true")) {
        *out = true;
    } else if (EqualsIgnoreCase(s, "false")) {
        *out = false;
    } else {
        return false;
    }
    return true;
}

bool parseArgDirection(const std::string& s, bool* out) {
    if (EqualsIgnoreCase(s, "up")) {
        *out = true;
    } else if (EqualsIgnoreCase(s, "down")) {
        *out = false;
    } else {
        return false;
    }
    return true;
}

bool parseArgIdentifierTypeArray(const std::string& s, vector<IdentifierType>* out) {
    for (const std::string& val : ::android::base::Split(s, ",")) {
        int outInt;
        if (!parseArgInt(val, &outInt)) {
            return false;
        }
        out->push_back(getType(outInt));
    }
    return true;
}

bool parseProgramIdentifierList(const std::string& s, vector<ProgramIdentifier>* out) {
    for (const std::string& idStr : ::android::base::Split(s, ",")) {
        const vector<std::string> idStrPair = ::android::base::Split(idStr, ":");
        if (idStrPair.size() != 2) {
            return false;
        }
        int idType;
        if (!parseArgInt(idStrPair[0], &idType)) {
            return false;
        }
        long idVal;
        if (!parseArgLong(idStrPair[1], &idVal)) {
            return false;
        }
        ProgramIdentifier id = {getType(idType), idVal};
        out->push_back(id);
    }
    return true;
}

}  // namespace utils

utils::IdentifierIterator begin(const ProgramSelector& sel) {
    return utils::IdentifierIterator(sel);
}

utils::IdentifierIterator end(const ProgramSelector& sel) {
    return utils::IdentifierIterator(sel) + 1 /* primary id */ + sel.secondaryIds.size();
}

}  // namespace aidl::android::hardware::broadcastradio
