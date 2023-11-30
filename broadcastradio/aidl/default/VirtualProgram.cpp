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

#include "VirtualProgram.h"

#include <broadcastradio-utils-aidl/Utils.h>
#include "resources.h"

#include <android-base/logging.h>

namespace aidl::android::hardware::broadcastradio {

using ::std::vector;

VirtualProgram::operator ProgramInfo() const {
    ProgramInfo info = {};

    info.selector = selector;

    IdentifierType programType = selector.primaryId.type;
    bool isDigital = (programType != IdentifierType::AMFM_FREQUENCY_KHZ &&
                      programType != IdentifierType::RDS_PI);

    auto selectId = [&info](const IdentifierType& type) {
        return utils::makeIdentifier(type, utils::getId(info.selector, type));
    };

    switch (programType) {
        case IdentifierType::AMFM_FREQUENCY_KHZ:
            info.logicallyTunedTo = info.physicallyTunedTo =
                    selectId(IdentifierType::AMFM_FREQUENCY_KHZ);
            break;
        case IdentifierType::RDS_PI:
            info.logicallyTunedTo = selectId(IdentifierType::RDS_PI);
            info.physicallyTunedTo = selectId(IdentifierType::AMFM_FREQUENCY_KHZ);
            break;
        case IdentifierType::HD_STATION_ID_EXT:
            info.logicallyTunedTo = selectId(IdentifierType::HD_STATION_ID_EXT);
            if (utils::hasId(info.selector, IdentifierType::AMFM_FREQUENCY_KHZ)) {
                info.physicallyTunedTo = selectId(IdentifierType::AMFM_FREQUENCY_KHZ);
            } else {
                info.physicallyTunedTo = utils::makeIdentifier(
                        IdentifierType::AMFM_FREQUENCY_KHZ, utils::getHdFrequency(info.selector));
            }
            break;
        case IdentifierType::DAB_SID_EXT:
            info.logicallyTunedTo = selectId(IdentifierType::DAB_SID_EXT);
            info.physicallyTunedTo = selectId(IdentifierType::DAB_FREQUENCY_KHZ);
            break;
        case IdentifierType::DRMO_SERVICE_ID:
            info.logicallyTunedTo = selectId(IdentifierType::DRMO_SERVICE_ID);
            info.physicallyTunedTo = selectId(IdentifierType::DRMO_FREQUENCY_KHZ);
            break;
        case IdentifierType::SXM_SERVICE_ID:
            info.logicallyTunedTo = selectId(IdentifierType::SXM_SERVICE_ID);
            info.physicallyTunedTo = selectId(IdentifierType::SXM_CHANNEL);
            break;
        default:
            LOG(FATAL) << "unsupported program type: " << toString(programType);
            return {};
    }

    info.infoFlags |= (ProgramInfo::FLAG_TUNABLE | ProgramInfo::FLAG_STEREO);
    info.signalQuality = isDigital ? kSignalQualityDigital : kSignalQualityNonDigital;

    info.metadata = vector<Metadata>({
            Metadata::make<Metadata::rdsPs>(programName),
            Metadata::make<Metadata::songTitle>(songTitle),
            Metadata::make<Metadata::songArtist>(songArtist),
            Metadata::make<Metadata::stationIcon>(resources::kDemoPngId),
            Metadata::make<Metadata::albumArt>(resources::kDemoPngId),
    });

    info.vendorInfo = vector<VendorKeyValue>({
            {"com.android.sample", "sample"},
            {"com.android.sample.VirtualProgram", "VirtualProgram"},
    });

    return info;
}

bool operator<(const VirtualProgram& lhs, const VirtualProgram& rhs) {
    auto& l = lhs.selector;
    auto& r = rhs.selector;

    if ((utils::hasId(l, IdentifierType::AMFM_FREQUENCY_KHZ) ||
         l.primaryId.type == IdentifierType::HD_STATION_ID_EXT) &&
        (utils::hasId(r, IdentifierType::AMFM_FREQUENCY_KHZ) ||
         r.primaryId.type == IdentifierType::HD_STATION_ID_EXT)) {
        uint32_t freq1 = utils::getAmFmFrequency(l);
        int subChannel1 = l.primaryId.type == IdentifierType::HD_STATION_ID_EXT
                                  ? utils::getHdSubchannel(l)
                                  : 0;
        uint32_t freq2 = utils::getAmFmFrequency(r);
        int subChannel2 = r.primaryId.type == IdentifierType::HD_STATION_ID_EXT
                                  ? utils::getHdSubchannel(r)
                                  : 0;
        return freq1 < freq2 || (freq1 == freq2 && (l.primaryId.type < r.primaryId.type ||
                                                    subChannel1 < subChannel2));
    } else if (l.primaryId.type == IdentifierType::DAB_SID_EXT &&
               l.primaryId.type == IdentifierType::DAB_SID_EXT) {
        uint64_t dabFreq1 = utils::getId(l, IdentifierType::DAB_FREQUENCY_KHZ);
        uint64_t dabFreq2 = utils::getId(r, IdentifierType::DAB_FREQUENCY_KHZ);
        if (dabFreq1 != dabFreq2) {
            return dabFreq1 < dabFreq2;
        }
        return utils::getId(l, IdentifierType::DAB_ENSEMBLE) <
               utils::getId(r, IdentifierType::DAB_ENSEMBLE);
    }

    if (l.primaryId.type != r.primaryId.type) {
        return l.primaryId.type < r.primaryId.type;
    }
    return l.primaryId.value < r.primaryId.value;
}

}  // namespace aidl::android::hardware::broadcastradio
