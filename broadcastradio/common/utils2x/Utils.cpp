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

#include <log/log.h>

namespace android {
namespace hardware {
namespace broadcastradio {
namespace utils {

using V2_0::IdentifierType;
using V2_0::Metadata;
using V2_0::MetadataKey;
using V2_0::ProgramIdentifier;
using V2_0::ProgramSelector;

using std::string;

IdentifierType getType(const ProgramIdentifier& id) {
    return static_cast<IdentifierType>(id.type);
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

bool isSupported(const V2_0::Properties& prop, const V2_0::ProgramSelector& sel) {
    // Not optimal, but it doesn't matter for default impl nor VTS tests.
    for (auto&& idTypeI : prop.supportedIdentifierTypes) {
        auto idType = static_cast<IdentifierType>(idTypeI);
        if (hasId(sel, idType)) return true;
    }
    return false;
}

static bool isValid(const ProgramIdentifier& id) {
    auto val = id.value;
    bool valid = true;

    auto expect = [&valid](bool condition, std::string message) {
        if (!condition) {
            valid = false;
            ALOGE("Identifier not valid, expected %s", message.c_str());
        }
    };

    switch (static_cast<IdentifierType>(id.type)) {
        case IdentifierType::AMFM_FREQUENCY:
        case IdentifierType::DAB_FREQUENCY:
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

bool isValid(const V2_0::ProgramSelector& sel) {
    if (!isValid(sel.primaryId)) return false;
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

}  // namespace utils
}  // namespace broadcastradio
}  // namespace hardware
}  // namespace android
