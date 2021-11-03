/*
 * Copyright (C) 2021 The Android Open Source Project
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

#include "utils.h"

namespace android::hardware::radio::compat {

namespace RAF {
using E = V1_4::RadioAccessFamily;
constexpr auto GSM = E::GSM | E::GPRS;
constexpr auto CDMA = E::IS95A | E::IS95B | E::ONE_X_RTT;
constexpr auto EVDO = E::EVDO_0 | E::EVDO_A | E::EVDO_B | E::EHRPD;
constexpr auto HS = E::HSUPA | E::HSDPA | E::HSPA | E::HSPAP;
constexpr auto WCDMA = HS | E::UMTS;
constexpr auto LTE = E::LTE | E::LTE_CA;
constexpr auto NR = E::NR;
}  // namespace RAF

static hidl_bitfield<V1_4::RadioAccessFamily>  //
getAdjustedRaf(hidl_bitfield<V1_4::RadioAccessFamily> raf) {
    if (raf & RAF::GSM) raf |= RAF::GSM;
    if (raf & RAF::WCDMA) raf |= RAF::WCDMA;
    if (raf & RAF::CDMA) raf |= RAF::CDMA;
    if (raf & RAF::EVDO) raf |= RAF::EVDO;
    if (raf & RAF::LTE) raf |= RAF::LTE;
    if (raf & RAF::NR) raf |= RAF::NR;

    return raf;
}

V1_0::PreferredNetworkType getNetworkTypeFromRaf(hidl_bitfield<V1_4::RadioAccessFamily> raf) {
    raf = getAdjustedRaf(raf);
    switch (raf) {
        case RAF::GSM | RAF::WCDMA:
            return V1_0::PreferredNetworkType::GSM_WCDMA_AUTO;
        case RAF::GSM:
            return V1_0::PreferredNetworkType::GSM_ONLY;
        case RAF::WCDMA:
            return V1_0::PreferredNetworkType::WCDMA;
        case (RAF::CDMA | RAF::EVDO):
            return V1_0::PreferredNetworkType::CDMA_EVDO_AUTO;
        case (RAF::LTE | RAF::CDMA | RAF::EVDO):
            return V1_0::PreferredNetworkType::LTE_CDMA_EVDO;
        case (RAF::LTE | RAF::GSM | RAF::WCDMA):
            return V1_0::PreferredNetworkType::LTE_GSM_WCDMA;
        case (RAF::LTE | RAF::CDMA | RAF::EVDO | RAF::GSM | RAF::WCDMA):
            return V1_0::PreferredNetworkType::LTE_CMDA_EVDO_GSM_WCDMA;  // CDMA typo
        case RAF::LTE:
            return V1_0::PreferredNetworkType::LTE_ONLY;
        case (RAF::LTE | RAF::WCDMA):
            return V1_0::PreferredNetworkType::LTE_WCDMA;
        case RAF::CDMA:
            return V1_0::PreferredNetworkType::CDMA_ONLY;
        case RAF::EVDO:
            return V1_0::PreferredNetworkType::EVDO_ONLY;
        case (RAF::GSM | RAF::WCDMA | RAF::CDMA | RAF::EVDO):
            return V1_0::PreferredNetworkType::GSM_WCDMA_CDMA_EVDO_AUTO;
        case static_cast<int>(RAF::E::TD_SCDMA):
            return V1_0::PreferredNetworkType::TD_SCDMA_ONLY;
        case (RAF::E::TD_SCDMA | RAF::WCDMA):
            return V1_0::PreferredNetworkType::TD_SCDMA_WCDMA;
        case (RAF::LTE | RAF::E::TD_SCDMA):
            return V1_0::PreferredNetworkType::TD_SCDMA_LTE;
        case (RAF::E::TD_SCDMA | RAF::GSM):
            return V1_0::PreferredNetworkType::TD_SCDMA_GSM;
        case (RAF::LTE | RAF::E::TD_SCDMA | RAF::GSM):
            return V1_0::PreferredNetworkType::TD_SCDMA_GSM_LTE;
        case (RAF::E::TD_SCDMA | RAF::GSM | RAF::WCDMA):
            return V1_0::PreferredNetworkType::TD_SCDMA_GSM_WCDMA;
        case (RAF::LTE | RAF::E::TD_SCDMA | RAF::WCDMA):
            return V1_0::PreferredNetworkType::TD_SCDMA_WCDMA_LTE;
        case (RAF::LTE | RAF::E::TD_SCDMA | RAF::GSM | RAF::WCDMA):
            return V1_0::PreferredNetworkType::TD_SCDMA_GSM_WCDMA_LTE;
        case (RAF::E::TD_SCDMA | RAF::CDMA | RAF::EVDO | RAF::GSM | RAF::WCDMA):
            return V1_0::PreferredNetworkType::TD_SCDMA_GSM_WCDMA_CDMA_EVDO_AUTO;
        case (RAF::LTE | RAF::E::TD_SCDMA | RAF::CDMA | RAF::EVDO | RAF::GSM | RAF::WCDMA):
            return V1_0::PreferredNetworkType::TD_SCDMA_LTE_CDMA_EVDO_GSM_WCDMA;
        case static_cast<int>(RAF::NR):
            return V1_0::PreferredNetworkType(23);  //  NR_ONLY
        case (RAF::NR | RAF::LTE):
            return V1_0::PreferredNetworkType(24);  //  NR_LTE
        case (RAF::NR | RAF::LTE | RAF::CDMA | RAF::EVDO):
            return V1_0::PreferredNetworkType(25);  //  NR_LTE_CDMA_EVDO
        case (RAF::NR | RAF::LTE | RAF::GSM | RAF::WCDMA):
            return V1_0::PreferredNetworkType(26);  //  NR_LTE_GSM_WCDMA
        case (RAF::NR | RAF::LTE | RAF::CDMA | RAF::EVDO | RAF::GSM | RAF::WCDMA):
            return V1_0::PreferredNetworkType(27);  //  NR_LTE_CDMA_EVDO_GSM_WCDMA
        case (RAF::NR | RAF::LTE | RAF::WCDMA):
            return V1_0::PreferredNetworkType(28);  //  NR_LTE_WCDMA
        case (RAF::NR | RAF::LTE | RAF::E::TD_SCDMA):
            return V1_0::PreferredNetworkType(29);  //  NR_LTE_TDSCDMA
        case (RAF::NR | RAF::LTE | RAF::E::TD_SCDMA | RAF::GSM):
            return V1_0::PreferredNetworkType(30);  //  NR_LTE_TDSCDMA_GSM
        case (RAF::NR | RAF::LTE | RAF::E::TD_SCDMA | RAF::WCDMA):
            return V1_0::PreferredNetworkType(31);  //  NR_LTE_TDSCDMA_WCDMA
        case (RAF::NR | RAF::LTE | RAF::E::TD_SCDMA | RAF::GSM | RAF::WCDMA):
            return V1_0::PreferredNetworkType(32);  //  NR_LTE_TDSCDMA_GSM_WCDMA
        case (RAF::NR | RAF::LTE | RAF::E::TD_SCDMA | RAF::CDMA | RAF::EVDO | RAF::GSM |
              RAF::WCDMA):
            return V1_0::PreferredNetworkType(33);  //  NR_LTE_TDSCDMA_CDMA_EVDO_GSM_WCDMA
        default:
            return V1_0::PreferredNetworkType::WCDMA;
    }
}

hidl_bitfield<V1_4::RadioAccessFamily> getRafFromNetworkType(V1_0::PreferredNetworkType type) {
    switch (type) {
        case V1_0::PreferredNetworkType::GSM_WCDMA_AUTO:
            return RAF::GSM | RAF::WCDMA;
        case V1_0::PreferredNetworkType::GSM_ONLY:
            return RAF::GSM;
        case V1_0::PreferredNetworkType::WCDMA:
            return RAF::WCDMA;
        case V1_0::PreferredNetworkType::CDMA_EVDO_AUTO:
            return (RAF::CDMA | RAF::EVDO);
        case V1_0::PreferredNetworkType::LTE_CDMA_EVDO:
            return (RAF::LTE | RAF::CDMA | RAF::EVDO);
        case V1_0::PreferredNetworkType::LTE_GSM_WCDMA:
            return (RAF::LTE | RAF::GSM | RAF::WCDMA);
        case V1_0::PreferredNetworkType::LTE_CMDA_EVDO_GSM_WCDMA:
            return (RAF::LTE | RAF::CDMA | RAF::EVDO | RAF::GSM | RAF::WCDMA);
        case V1_0::PreferredNetworkType::LTE_ONLY:
            return RAF::LTE;
        case V1_0::PreferredNetworkType::LTE_WCDMA:
            return (RAF::LTE | RAF::WCDMA);
        case V1_0::PreferredNetworkType::CDMA_ONLY:
            return RAF::CDMA;
        case V1_0::PreferredNetworkType::EVDO_ONLY:
            return RAF::EVDO;
        case V1_0::PreferredNetworkType::GSM_WCDMA_CDMA_EVDO_AUTO:
            return (RAF::GSM | RAF::WCDMA | RAF::CDMA | RAF::EVDO);
        case V1_0::PreferredNetworkType::TD_SCDMA_ONLY:
            return static_cast<int>(RAF::E::TD_SCDMA);
        case V1_0::PreferredNetworkType::TD_SCDMA_WCDMA:
            return (RAF::E::TD_SCDMA | RAF::WCDMA);
        case V1_0::PreferredNetworkType::TD_SCDMA_LTE:
            return (RAF::LTE | RAF::E::TD_SCDMA);
        case V1_0::PreferredNetworkType::TD_SCDMA_GSM:
            return (RAF::E::TD_SCDMA | RAF::GSM);
        case V1_0::PreferredNetworkType::TD_SCDMA_GSM_LTE:
            return (RAF::LTE | RAF::E::TD_SCDMA | RAF::GSM);
        case V1_0::PreferredNetworkType::TD_SCDMA_GSM_WCDMA:
            return (RAF::E::TD_SCDMA | RAF::GSM | RAF::WCDMA);
        case V1_0::PreferredNetworkType::TD_SCDMA_WCDMA_LTE:
            return (RAF::LTE | RAF::E::TD_SCDMA | RAF::WCDMA);
        case V1_0::PreferredNetworkType::TD_SCDMA_GSM_WCDMA_LTE:
            return (RAF::LTE | RAF::E::TD_SCDMA | RAF::GSM | RAF::WCDMA);
        case V1_0::PreferredNetworkType::TD_SCDMA_GSM_WCDMA_CDMA_EVDO_AUTO:
            return (RAF::E::TD_SCDMA | RAF::CDMA | RAF::EVDO | RAF::GSM | RAF::WCDMA);
        case V1_0::PreferredNetworkType::TD_SCDMA_LTE_CDMA_EVDO_GSM_WCDMA:
            return (RAF::LTE | RAF::E::TD_SCDMA | RAF::CDMA | RAF::EVDO | RAF::GSM | RAF::WCDMA);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch"
        case V1_0::PreferredNetworkType(23):  //  NR_ONLY
            return static_cast<int>(RAF::NR);
        case V1_0::PreferredNetworkType(24):  //  NR_LTE
            return (RAF::NR | RAF::LTE);
        case V1_0::PreferredNetworkType(25):  //  NR_LTE_CDMA_EVDO
            return (RAF::NR | RAF::LTE | RAF::CDMA | RAF::EVDO);
        case V1_0::PreferredNetworkType(26):  //  NR_LTE_GSM_WCDMA
            return (RAF::NR | RAF::LTE | RAF::GSM | RAF::WCDMA);
        case V1_0::PreferredNetworkType(27):  //  NR_LTE_CDMA_EVDO_GSM_WCDMA
            return (RAF::NR | RAF::LTE | RAF::CDMA | RAF::EVDO | RAF::GSM | RAF::WCDMA);
        case V1_0::PreferredNetworkType(28):  //  NR_LTE_WCDMA
            return (RAF::NR | RAF::LTE | RAF::WCDMA);
        case V1_0::PreferredNetworkType(29):  //  NR_LTE_TDSCDMA
            return (RAF::NR | RAF::LTE | RAF::E::TD_SCDMA);
        case V1_0::PreferredNetworkType(30):  //  NR_LTE_TDSCDMA_GSM
            return (RAF::NR | RAF::LTE | RAF::E::TD_SCDMA | RAF::GSM);
        case V1_0::PreferredNetworkType(31):  //  NR_LTE_TDSCDMA_WCDMA
            return (RAF::NR | RAF::LTE | RAF::E::TD_SCDMA | RAF::WCDMA);
        case V1_0::PreferredNetworkType(32):  //  NR_LTE_TDSCDMA_GSM_WCDMA
            return (RAF::NR | RAF::LTE | RAF::E::TD_SCDMA | RAF::GSM | RAF::WCDMA);
        case V1_0::PreferredNetworkType(33):  //  NR_LTE_TDSCDMA_CDMA_EVDO_GSM_WCDMA
            return (RAF::NR | RAF::LTE | RAF::E::TD_SCDMA | RAF::CDMA | RAF::EVDO | RAF::GSM |
                    RAF::WCDMA);
#pragma GCC diagnostic pop
        default:
            return {};  // unknown
    }
}

}  // namespace android::hardware::radio::compat
