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
#define LOG_TAG "BcRadioDef.utils"
//#define LOG_NDEBUG 0

#include <broadcastradio-utils-2x/Utils.h>

#include <android-base/logging.h>
#include <log/log.h>

namespace android {
namespace hardware {
namespace broadcastradio {
namespace utils {

using V2_0::IdentifierType;
using V2_0::Metadata;
using V2_0::MetadataKey;
using V2_0::ProgramFilter;
using V2_0::ProgramIdentifier;
using V2_0::ProgramInfo;
using V2_0::ProgramListChunk;
using V2_0::ProgramSelector;
using V2_0::Properties;

using std::string;
using std::vector;

IdentifierType getType(uint32_t typeAsInt) {
    return static_cast<IdentifierType>(typeAsInt);
}

IdentifierType getType(const ProgramIdentifier& id) {
    return getType(id.type);
}

IdentifierIterator::IdentifierIterator(const V2_0::ProgramSelector& sel)
    : IdentifierIterator(sel, 0) {}

IdentifierIterator::IdentifierIterator(const V2_0::ProgramSelector& sel, size_t pos)
    : mSel(sel), mPos(pos) {}

IdentifierIterator IdentifierIterator::operator++(int) {
    auto i = *this;
    mPos++;
    return i;
}

IdentifierIterator& IdentifierIterator::operator++() {
    ++mPos;
    return *this;
}

IdentifierIterator::ref_type IdentifierIterator::operator*() const {
    if (mPos == 0) return sel().primaryId;

    // mPos is 1-based for secondary identifiers
    DCHECK(mPos <= sel().secondaryIds.size());
    return sel().secondaryIds[mPos - 1];
}

bool IdentifierIterator::operator==(const IdentifierIterator& rhs) const {
    // Check, if both iterators points at the same selector.
    if (reinterpret_cast<uintptr_t>(&sel()) != reinterpret_cast<uintptr_t>(&rhs.sel())) {
        return false;
    }

    return mPos == rhs.mPos;
}

FrequencyBand getBand(uint64_t freq) {
    // keep in sync with
    // frameworks/base/services/core/java/com/android/server/broadcastradio/hal2/Utils.java
    if (freq < 30) return FrequencyBand::UNKNOWN;
    if (freq < 500) return FrequencyBand::AM_LW;
    if (freq < 1705) return FrequencyBand::AM_MW;
    if (freq < 30000) return FrequencyBand::AM_SW;
    if (freq < 60000) return FrequencyBand::UNKNOWN;
    if (freq < 110000) return FrequencyBand::FM;
    return FrequencyBand::UNKNOWN;
}

static bool bothHaveId(const ProgramSelector& a, const ProgramSelector& b,
                       const IdentifierType type) {
    return hasId(a, type) && hasId(b, type);
}

static bool haveEqualIds(const ProgramSelector& a, const ProgramSelector& b,
                         const IdentifierType type) {
    if (!bothHaveId(a, b, type)) return false;
    /* We should check all Ids of a given type (ie. other AF),
     * but it doesn't matter for default implementation.
     */
    return getId(a, type) == getId(b, type);
}

static int getHdSubchannel(const ProgramSelector& sel) {
    auto hdsidext = getId(sel, IdentifierType::HD_STATION_ID_EXT, 0);
    hdsidext >>= 32;        // Station ID number
    return hdsidext & 0xF;  // HD Radio subchannel
}

bool tunesTo(const ProgramSelector& a, const ProgramSelector& b) {
    auto type = getType(b.primaryId);

    switch (type) {
        case IdentifierType::HD_STATION_ID_EXT:
        case IdentifierType::RDS_PI:
        case IdentifierType::AMFM_FREQUENCY:
            if (haveEqualIds(a, b, IdentifierType::HD_STATION_ID_EXT)) return true;
            if (haveEqualIds(a, b, IdentifierType::RDS_PI)) return true;
            return getHdSubchannel(b) == 0 && haveEqualIds(a, b, IdentifierType::AMFM_FREQUENCY);
        case IdentifierType::DAB_SID_EXT:
            return haveEqualIds(a, b, IdentifierType::DAB_SID_EXT);
        case IdentifierType::DRMO_SERVICE_ID:
            return haveEqualIds(a, b, IdentifierType::DRMO_SERVICE_ID);
        case IdentifierType::SXM_SERVICE_ID:
            return haveEqualIds(a, b, IdentifierType::SXM_SERVICE_ID);
        default:  // includes all vendor types
            ALOGW("Unsupported program type: %s", toString(type).c_str());
            return false;
    }
}

static bool maybeGetId(const ProgramSelector& sel, const IdentifierType type, uint64_t* val) {
    auto itype = static_cast<uint32_t>(type);

    if (sel.primaryId.type == itype) {
        if (val) *val = sel.primaryId.value;
        return true;
    }

    // TODO(twasilczyk): use IdentifierIterator
    // not optimal, but we don't care in default impl
    for (auto&& id : sel.secondaryIds) {
        if (id.type == itype) {
            if (val) *val = id.value;
            return true;
        }
    }

    return false;
}

bool hasId(const ProgramSelector& sel, const IdentifierType type) {
    return maybeGetId(sel, type, nullptr);
}

uint64_t getId(const ProgramSelector& sel, const IdentifierType type) {
    uint64_t val;

    if (maybeGetId(sel, type, &val)) {
        return val;
    }

    ALOGW("Identifier %s not found", toString(type).c_str());
    return 0;
}

uint64_t getId(const ProgramSelector& sel, const IdentifierType type, uint64_t defval) {
    if (!hasId(sel, type)) return defval;
    return getId(sel, type);
}

vector<uint64_t> getAllIds(const ProgramSelector& sel, const IdentifierType type) {
    vector<uint64_t> ret;
    auto itype = static_cast<uint32_t>(type);

    if (sel.primaryId.type == itype) ret.push_back(sel.primaryId.value);

    // TODO(twasilczyk): use IdentifierIterator
    for (auto&& id : sel.secondaryIds) {
        if (id.type == itype) ret.push_back(id.value);
    }

    return ret;
}

bool isSupported(const Properties& prop, const ProgramSelector& sel) {
    // TODO(twasilczyk): use IdentifierIterator
    // Not optimal, but it doesn't matter for default impl nor VTS tests.
    for (auto&& idType : prop.supportedIdentifierTypes) {
        if (hasId(sel, getType(idType))) return true;
    }
    return false;
}

bool isValid(const ProgramIdentifier& id) {
    auto val = id.value;
    bool valid = true;

    auto expect = [&valid](bool condition, std::string message) {
        if (!condition) {
            valid = false;
            ALOGE("Identifier not valid, expected %s", message.c_str());
        }
    };

    switch (getType(id)) {
        case IdentifierType::INVALID:
            expect(false, "IdentifierType::INVALID");
            break;
        case IdentifierType::DAB_FREQUENCY:
            expect(val > 100000u, "f > 100MHz");
            [[fallthrough]];
        case IdentifierType::AMFM_FREQUENCY:
        case IdentifierType::DRMO_FREQUENCY:
            expect(val > 100u, "f > 100kHz");
            expect(val < 10000000u, "f < 10GHz");
            break;
        case IdentifierType::RDS_PI:
            expect(val != 0u, "RDS PI != 0");
            expect(val <= 0xFFFFu, "16bit id");
            break;
        case IdentifierType::HD_STATION_ID_EXT: {
            auto stationId = val & 0xFFFFFFFF;  // 32bit
            val >>= 32;
            auto subchannel = val & 0xF;  // 4bit
            val >>= 4;
            auto freq = val & 0x3FFFF;  // 18bit
            expect(stationId != 0u, "HD station id != 0");
            expect(subchannel < 8u, "HD subch < 8");
            expect(freq > 100u, "f > 100kHz");
            expect(freq < 10000000u, "f < 10GHz");
            break;
        }
        case IdentifierType::HD_STATION_NAME: {
            while (val > 0) {
                auto ch = static_cast<char>(val & 0xFF);
                val >>= 8;
                expect((ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'Z'),
                       "HD_STATION_NAME does not match [A-Z0-9]+");
            }
            break;
        }
        case IdentifierType::DAB_SID_EXT: {
            auto sid = val & 0xFFFF;  // 16bit
            val >>= 16;
            auto ecc = val & 0xFF;  // 8bit
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
        case IdentifierType::VENDOR_START:
        case IdentifierType::VENDOR_END:
            // skip
            break;
    }

    return valid;
}

bool isValid(const ProgramSelector& sel) {
    if (!isValid(sel.primaryId)) return false;
    // TODO(twasilczyk): use IdentifierIterator
    for (auto&& id : sel.secondaryIds) {
        if (!isValid(id)) return false;
    }
    return true;
}

ProgramIdentifier make_identifier(IdentifierType type, uint64_t value) {
    return {static_cast<uint32_t>(type), value};
}

ProgramSelector make_selector_amfm(uint32_t frequency) {
    ProgramSelector sel = {};
    sel.primaryId = make_identifier(IdentifierType::AMFM_FREQUENCY, frequency);
    return sel;
}

Metadata make_metadata(MetadataKey key, int64_t value) {
    Metadata meta = {};
    meta.key = static_cast<uint32_t>(key);
    meta.intValue = value;
    return meta;
}

Metadata make_metadata(MetadataKey key, string value) {
    Metadata meta = {};
    meta.key = static_cast<uint32_t>(key);
    meta.stringValue = value;
    return meta;
}

bool satisfies(const ProgramFilter& filter, const ProgramSelector& sel) {
    if (filter.identifierTypes.size() > 0) {
        auto typeEquals = [](const V2_0::ProgramIdentifier& id, uint32_t type) {
            return id.type == type;
        };
        auto it = std::find_first_of(begin(sel), end(sel), filter.identifierTypes.begin(),
                                     filter.identifierTypes.end(), typeEquals);
        if (it == end(sel)) return false;
    }

    if (filter.identifiers.size() > 0) {
        auto it = std::find_first_of(begin(sel), end(sel), filter.identifiers.begin(),
                                     filter.identifiers.end());
        if (it == end(sel)) return false;
    }

    if (!filter.includeCategories) {
        if (getType(sel.primaryId) == IdentifierType::DAB_ENSEMBLE) return false;
    }

    return true;
}

size_t ProgramInfoHasher::operator()(const ProgramInfo& info) const {
    auto& id = info.selector.primaryId;

    /* This is not the best hash implementation, but good enough for default HAL
     * implementation and tests. */
    auto h = std::hash<uint32_t>{}(id.type);
    h += 0x9e3779b9;
    h ^= std::hash<uint64_t>{}(id.value);

    return h;
}

bool ProgramInfoKeyEqual::operator()(const ProgramInfo& info1, const ProgramInfo& info2) const {
    auto& id1 = info1.selector.primaryId;
    auto& id2 = info2.selector.primaryId;
    return id1.type == id2.type && id1.value == id2.value;
}

void updateProgramList(ProgramInfoSet& list, const ProgramListChunk& chunk) {
    if (chunk.purge) list.clear();

    list.insert(chunk.modified.begin(), chunk.modified.end());

    for (auto&& id : chunk.removed) {
        ProgramInfo info = {};
        info.selector.primaryId = id;
        list.erase(info);
    }
}

std::optional<std::string> getMetadataString(const V2_0::ProgramInfo& info,
                                             const V2_0::MetadataKey key) {
    auto isKey = [key](const V2_0::Metadata& item) {
        return static_cast<V2_0::MetadataKey>(item.key) == key;
    };

    auto it = std::find_if(info.metadata.begin(), info.metadata.end(), isKey);
    if (it == info.metadata.end()) return std::nullopt;

    return it->stringValue;
}

V2_0::ProgramIdentifier make_hdradio_station_name(const std::string& name) {
    constexpr size_t maxlen = 8;

    std::string shortName;
    shortName.reserve(maxlen);

    auto&& loc = std::locale::classic();
    for (char ch : name) {
        if (!std::isalnum(ch, loc)) continue;
        shortName.push_back(std::toupper(ch, loc));
        if (shortName.length() >= maxlen) break;
    }

    uint64_t val = 0;
    for (auto rit = shortName.rbegin(); rit != shortName.rend(); ++rit) {
        val <<= 8;
        val |= static_cast<uint8_t>(*rit);
    }

    return make_identifier(IdentifierType::HD_STATION_NAME, val);
}

}  // namespace utils

namespace V2_0 {

utils::IdentifierIterator begin(const ProgramSelector& sel) {
    return utils::IdentifierIterator(sel);
}

utils::IdentifierIterator end(const ProgramSelector& sel) {
    return utils::IdentifierIterator(sel) + 1 /* primary id */ + sel.secondaryIds.size();
}

}  // namespace V2_0
}  // namespace broadcastradio
}  // namespace hardware
}  // namespace android
