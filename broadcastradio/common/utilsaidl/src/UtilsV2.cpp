/*
 * Copyright (C) 2023 The Android Open Source Project
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

#define LOG_TAG "BcRadioAidlDef.utilsV2"

#include "broadcastradio-utils-aidl/UtilsV2.h"

#include <android-base/logging.h>
#include <android-base/strings.h>

namespace aidl::android::hardware::broadcastradio {

namespace utils {

bool isValidV2(const ProgramIdentifier& id) {
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
        case IdentifierType::HD_STATION_LOCATION: {
            uint64_t latitudeBit = val & 0x1;
            expect(latitudeBit == 1u, "Latitude comes first");
            val >>= 27;
            uint64_t latitudePad = val & 0x1Fu;
            expect(latitudePad == 0u, "Latitude padding");
            val >>= 5;
            uint64_t longitudeBit = val & 0x1;
            expect(longitudeBit == 1u, "Longitude comes next");
            val >>= 27;
            uint64_t longitudePad = val & 0x1Fu;
            expect(longitudePad == 0u, "Latitude padding");
            break;
        }
        default:
            expect(id.type >= IdentifierType::VENDOR_START && id.type <= IdentifierType::VENDOR_END,
                   "Undefined identifier type");
            break;
    }

    return valid;
}

bool isValidV2(const ProgramSelector& sel) {
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
    return isValidV2(sel.primaryId);
}

std::optional<std::string> getMetadataStringV2(const ProgramInfo& info, const Metadata::Tag& tag) {
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
        case Metadata::genre:
            metadataString = it->get<Metadata::genre>();
            break;
        case Metadata::commentShortDescription:
            metadataString = it->get<Metadata::commentShortDescription>();
            break;
        case Metadata::commentActualText:
            metadataString = it->get<Metadata::commentActualText>();
            break;
        case Metadata::commercial:
            metadataString = it->get<Metadata::commercial>();
            break;
        case Metadata::ufids: {
            auto& ufids = it->get<Metadata::ufids>();
            metadataString = "[";
            for (const auto& ufid : ufids) {
                metadataString += std::string(ufid) + ",";
            }
            if (ufids.empty()) {
                metadataString += "]";
            } else {
                metadataString[metadataString.size() - 1] = ']';
            }
        } break;
        case Metadata::hdStationNameShort:
            metadataString = it->get<Metadata::hdStationNameShort>();
            break;
        case Metadata::hdStationNameLong:
            metadataString = it->get<Metadata::hdStationNameLong>();
            break;
        case Metadata::hdSubChannelsAvailable:
            metadataString = std::to_string(it->get<Metadata::hdSubChannelsAvailable>());
            break;
        default:
            LOG(ERROR) << "Metadata " << it->toString() << " is not converted.";
            return std::nullopt;
    }
    return metadataString;
}

}  // namespace utils

}  // namespace aidl::android::hardware::broadcastradio
