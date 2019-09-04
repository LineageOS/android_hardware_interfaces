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
#include "VirtualProgram.h"

#include "resources.h"

#include <android-base/logging.h>
#include <broadcastradio-utils-2x/Utils.h>

namespace android {
namespace hardware {
namespace broadcastradio {
namespace V2_0 {
namespace implementation {

using utils::getType;
using utils::make_metadata;

using std::vector;

VirtualProgram::operator ProgramInfo() const {
    ProgramInfo info = {};

    info.selector = selector;

    auto pType = getType(selector.primaryId);
    auto isDigital = (pType != IdentifierType::AMFM_FREQUENCY && pType != IdentifierType::RDS_PI);

    auto selectId = [&info](IdentifierType type) {
        return utils::make_identifier(type, utils::getId(info.selector, type));
    };

    switch (pType) {
        case IdentifierType::AMFM_FREQUENCY:
            info.logicallyTunedTo = info.physicallyTunedTo =
                selectId(IdentifierType::AMFM_FREQUENCY);
            break;
        case IdentifierType::RDS_PI:
            info.logicallyTunedTo = selectId(IdentifierType::RDS_PI);
            info.physicallyTunedTo = selectId(IdentifierType::AMFM_FREQUENCY);
            break;
        case IdentifierType::HD_STATION_ID_EXT:
            info.logicallyTunedTo = selectId(IdentifierType::HD_STATION_ID_EXT);
            info.physicallyTunedTo = selectId(IdentifierType::AMFM_FREQUENCY);
            break;
        case IdentifierType::DAB_SID_EXT:
            info.logicallyTunedTo = selectId(IdentifierType::DAB_SID_EXT);
            info.physicallyTunedTo = selectId(IdentifierType::DAB_ENSEMBLE);
            break;
        case IdentifierType::DRMO_SERVICE_ID:
            info.logicallyTunedTo = selectId(IdentifierType::DRMO_SERVICE_ID);
            info.physicallyTunedTo = selectId(IdentifierType::DRMO_FREQUENCY);
            break;
        case IdentifierType::SXM_SERVICE_ID:
            info.logicallyTunedTo = selectId(IdentifierType::SXM_SERVICE_ID);
            info.physicallyTunedTo = selectId(IdentifierType::SXM_CHANNEL);
            break;
        default:
            LOG(FATAL) << "unsupported program type: " << toString(pType);
    }

    info.infoFlags |= ProgramInfoFlags::TUNED;
    info.infoFlags |= ProgramInfoFlags::STEREO;
    info.signalQuality = isDigital ? 100 : 80;

    info.metadata = hidl_vec<Metadata>({
        make_metadata(MetadataKey::RDS_PS, programName),
        make_metadata(MetadataKey::SONG_TITLE, songTitle),
        make_metadata(MetadataKey::SONG_ARTIST, songArtist),
        make_metadata(MetadataKey::STATION_ICON, resources::demoPngId),
        make_metadata(MetadataKey::ALBUM_ART, resources::demoPngId),
    });

    info.vendorInfo = hidl_vec<VendorKeyValue>({
        {"com.google.dummy", "dummy"},
        {"com.google.dummy.VirtualProgram", std::to_string(reinterpret_cast<uintptr_t>(this))},
    });

    return info;
}

bool operator<(const VirtualProgram& lhs, const VirtualProgram& rhs) {
    auto& l = lhs.selector;
    auto& r = rhs.selector;

    // Two programs with the same primaryId are considered the same.
    if (l.primaryId.type != r.primaryId.type) return l.primaryId.type < r.primaryId.type;
    if (l.primaryId.value != r.primaryId.value) return l.primaryId.value < r.primaryId.value;

    return false;
}

}  // namespace implementation
}  // namespace V2_0
}  // namespace broadcastradio
}  // namespace hardware
}  // namespace android
