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

#include "structs.h"

#include "commonStructs.h"

#include "collections.h"

#include <android-base/logging.h>

namespace android::hardware::radio::compat {

namespace aidl = ::aidl::android::hardware::radio::network;
using ::aidl::android::hardware::radio::AccessNetwork;
using ::aidl::android::hardware::radio::RadioTechnology;

aidl::RadioBandMode toAidl(V1_0::RadioBandMode mode) {
    return aidl::RadioBandMode(mode);
}

aidl::GeranBands toAidl(V1_1::GeranBands band) {
    return aidl::GeranBands(band);
}

V1_1::GeranBands toHidl(aidl::GeranBands band) {
    return V1_1::GeranBands(band);
}

aidl::UtranBands toAidl(V1_5::UtranBands band) {
    return aidl::UtranBands(band);
}

V1_5::UtranBands toHidl(aidl::UtranBands band) {
    return V1_5::UtranBands(band);
}

aidl::EutranBands toAidl(V1_5::EutranBands band) {
    return aidl::EutranBands(band);
}

V1_5::EutranBands toHidl(aidl::EutranBands band) {
    return V1_5::EutranBands(band);
}

aidl::NgranBands toAidl(V1_5::NgranBands band) {
    return aidl::NgranBands(band);
}

V1_5::NgranBands toHidl(aidl::NgranBands band) {
    return V1_5::NgranBands(band);
}

V1_5::SignalThresholdInfo toHidl(const aidl::SignalThresholdInfo& info) {
    return {
            .signalMeasurement = static_cast<V1_5::SignalMeasurementType>(info.signalMeasurement),
            .hysteresisMs = info.hysteresisMs,
            .hysteresisDb = info.hysteresisDb,
            .thresholds = info.thresholds,
            .isEnabled = info.isEnabled,
    };
}

static aidl::RadioAccessSpecifierBands toAidl(const V1_5::RadioAccessSpecifier::Bands& bands) {
    using Discr = V1_5::RadioAccessSpecifier::Bands::hidl_discriminator;
    const auto discr = bands.getDiscriminator();

    if (discr == Discr::geranBands) return toAidl(bands.geranBands());
    if (discr == Discr::utranBands) return toAidl(bands.utranBands());
    if (discr == Discr::eutranBands) return toAidl(bands.eutranBands());
    if (discr == Discr::ngranBands) return toAidl(bands.ngranBands());

    return {};
}

static V1_5::RadioAccessSpecifier::Bands toHidl(const aidl::RadioAccessSpecifierBands& bands) {
    V1_5::RadioAccessSpecifier::Bands hidl;
    using Tag = aidl::RadioAccessSpecifierBands::Tag;

    if (bands.getTag() == Tag::geranBands) hidl.geranBands(toHidl(bands.get<Tag::geranBands>()));
    if (bands.getTag() == Tag::utranBands) hidl.utranBands(toHidl(bands.get<Tag::utranBands>()));
    if (bands.getTag() == Tag::eutranBands) hidl.eutranBands(toHidl(bands.get<Tag::eutranBands>()));
    if (bands.getTag() == Tag::ngranBands) hidl.ngranBands(toHidl(bands.get<Tag::ngranBands>()));

    return hidl;
}

AccessNetwork fromRadioAccessNetwork(V1_5::RadioAccessNetworks ran) {
    switch (ran) {
        case V1_5::RadioAccessNetworks::UNKNOWN:
            return AccessNetwork::UNKNOWN;
        case V1_5::RadioAccessNetworks::GERAN:
            return AccessNetwork::GERAN;
        case V1_5::RadioAccessNetworks::UTRAN:
            return AccessNetwork::UTRAN;
        case V1_5::RadioAccessNetworks::EUTRAN:
            return AccessNetwork::EUTRAN;
        case V1_5::RadioAccessNetworks::CDMA2000:
            return AccessNetwork::CDMA2000;
        case V1_5::RadioAccessNetworks::NGRAN:
            return AccessNetwork::NGRAN;
        default:
            return AccessNetwork::UNKNOWN;
    }
}

aidl::RadioAccessSpecifier toAidl(const V1_5::RadioAccessSpecifier& spec) {
    return {
            .accessNetwork = fromRadioAccessNetwork(spec.radioAccessNetwork),
            .bands = toAidl(spec.bands),
            .channels = spec.channels,
    };
}

V1_5::RadioAccessNetworks toRadioAccessNetworks(AccessNetwork val) {
    switch (val) {
        case AccessNetwork::UNKNOWN:
            return V1_5::RadioAccessNetworks::UNKNOWN;
        case AccessNetwork::GERAN:
            return V1_5::RadioAccessNetworks::GERAN;
        case AccessNetwork::UTRAN:
            return V1_5::RadioAccessNetworks::UTRAN;
        case AccessNetwork::EUTRAN:
            return V1_5::RadioAccessNetworks::EUTRAN;
        case AccessNetwork::CDMA2000:
            return V1_5::RadioAccessNetworks::CDMA2000;
        case AccessNetwork::NGRAN:
            return V1_5::RadioAccessNetworks::NGRAN;
        case AccessNetwork::IWLAN:
        default:
            return V1_5::RadioAccessNetworks::UNKNOWN;
    }
}

V1_5::RadioAccessSpecifier toHidl(const aidl::RadioAccessSpecifier& spec) {
    return {
            .radioAccessNetwork = toRadioAccessNetworks(spec.accessNetwork),
            .bands = toHidl(spec.bands),
            .channels = spec.channels,
    };
}

V1_5::NetworkScanRequest toHidl(const aidl::NetworkScanRequest& req) {
    return {
            .type = static_cast<V1_1::ScanType>(req.type),
            .interval = req.interval,
            .specifiers = toHidl(req.specifiers),
            .maxSearchTime = req.maxSearchTime,
            .incrementalResults = req.incrementalResults,
            .incrementalResultsPeriodicity = req.incrementalResultsPeriodicity,
            .mccMncs = toHidl(req.mccMncs),
    };
}

static aidl::OperatorInfo toAidl(const V1_2::CellIdentityOperatorNames& names) {
    return {
            .alphaLong = names.alphaLong,
            .alphaShort = names.alphaShort,
            .operatorNumeric = "",
            .status = aidl::OperatorInfo::STATUS_UNKNOWN,
    };
}

static aidl::CellIdentityGsm toAidl(const V1_5::CellIdentityGsm& ci) {
    return {
            .mcc = ci.base.base.mcc,
            .mnc = ci.base.base.mnc,
            .lac = ci.base.base.lac,
            .cid = ci.base.base.cid,
            .arfcn = ci.base.base.arfcn,
            .bsic = static_cast<int8_t>(ci.base.base.bsic),
            .operatorNames = toAidl(ci.base.operatorNames),
            .additionalPlmns = toAidl(ci.additionalPlmns),
    };
}

aidl::ClosedSubscriberGroupInfo toAidl(const V1_5::ClosedSubscriberGroupInfo& info) {
    return {
            .csgIndication = info.csgIndication,
            .homeNodebName = info.homeNodebName,
            .csgIdentity = info.csgIdentity,
    };
}

static std::optional<aidl::ClosedSubscriberGroupInfo> toAidl(const V1_5::OptionalCsgInfo& opt) {
    using descr = V1_5::OptionalCsgInfo::hidl_discriminator;
    if (opt.getDiscriminator() == descr::noinit) return std::nullopt;
    return toAidl(opt.csgInfo());
}

static aidl::CellIdentityWcdma toAidl(const V1_5::CellIdentityWcdma& ci) {
    return {
            .mcc = ci.base.base.mcc,
            .mnc = ci.base.base.mnc,
            .lac = ci.base.base.lac,
            .cid = ci.base.base.cid,
            .psc = ci.base.base.psc,
            .uarfcn = ci.base.base.uarfcn,
            .operatorNames = toAidl(ci.base.operatorNames),
            .additionalPlmns = toAidl(ci.additionalPlmns),
            .csgInfo = toAidl(ci.optionalCsgInfo),
    };
}

static aidl::CellIdentityTdscdma toAidl(const V1_5::CellIdentityTdscdma& ci) {
    return {
            .mcc = ci.base.base.mcc,
            .mnc = ci.base.base.mnc,
            .lac = ci.base.base.lac,
            .cid = ci.base.base.cid,
            .cpid = ci.base.base.cpid,
            .uarfcn = ci.base.uarfcn,
            .operatorNames = toAidl(ci.base.operatorNames),
            .additionalPlmns = toAidl(ci.additionalPlmns),
            .csgInfo = toAidl(ci.optionalCsgInfo),
    };
}

static aidl::CellIdentityCdma toAidl(const V1_2::CellIdentityCdma& ci) {
    return {
            .networkId = ci.base.networkId,
            .systemId = ci.base.systemId,
            .baseStationId = ci.base.baseStationId,
            .longitude = ci.base.longitude,
            .latitude = ci.base.latitude,
            .operatorNames = toAidl(ci.operatorNames),
    };
}

static aidl::CellIdentityLte toAidl(const V1_5::CellIdentityLte& ci) {
    return {
            .mcc = ci.base.base.mcc,
            .mnc = ci.base.base.mnc,
            .ci = ci.base.base.ci,
            .pci = ci.base.base.pci,
            .tac = ci.base.base.tac,
            .earfcn = ci.base.base.earfcn,
            .operatorNames = toAidl(ci.base.operatorNames),
            .bandwidth = ci.base.bandwidth,
            .additionalPlmns = toAidl(ci.additionalPlmns),
            .csgInfo = toAidl(ci.optionalCsgInfo),
            .bands = toAidl(ci.bands),
    };
}

static aidl::CellIdentityNr toAidl(const V1_5::CellIdentityNr& ci) {
    return {
            .mcc = ci.base.mcc,
            .mnc = ci.base.mnc,
            .nci = static_cast<int64_t>(ci.base.nci),
            .pci = static_cast<int32_t>(ci.base.pci),
            .tac = ci.base.tac,
            .nrarfcn = ci.base.nrarfcn,
            .operatorNames = toAidl(ci.base.operatorNames),
            .additionalPlmns = toAidl(ci.additionalPlmns),
            .bands = toAidl(ci.bands),
    };
}

aidl::CellIdentity toAidl(const V1_5::CellIdentity& ci) {
    using Discr = V1_5::CellIdentity::hidl_discriminator;
    const auto discr = ci.getDiscriminator();

    if (discr == Discr::gsm) return toAidl(ci.gsm());
    if (discr == Discr::wcdma) return toAidl(ci.wcdma());
    if (discr == Discr::tdscdma) return toAidl(ci.tdscdma());
    if (discr == Discr::cdma) return toAidl(ci.cdma());
    if (discr == Discr::lte) return toAidl(ci.lte());
    if (discr == Discr::nr) return toAidl(ci.nr());

    return {};
}

static std::optional<aidl::BarringTypeSpecificInfo>  //
toAidl(const V1_5::BarringInfo::BarringTypeSpecificInfo& opt) {
    using discr = V1_5::BarringInfo::BarringTypeSpecificInfo::hidl_discriminator;
    if (opt.getDiscriminator() == discr::noinit) return std::nullopt;

    const auto& info = opt.conditional();
    return aidl::BarringTypeSpecificInfo{
            .factor = info.factor,
            .timeSeconds = info.timeSeconds,
            .isBarred = info.isBarred,
    };
}

aidl::BarringInfo toAidl(const V1_5::BarringInfo& info) {
    return {
            .serviceType = static_cast<int32_t>(info.serviceType),
            .barringType = static_cast<int32_t>(info.barringType),
            .barringTypeSpecificInfo = toAidl(info.barringTypeSpecificInfo),
    };
}

static aidl::GsmSignalStrength toAidl(const V1_0::GsmSignalStrength& sig) {
    return {
            .signalStrength = static_cast<int32_t>(sig.signalStrength),
            .bitErrorRate = static_cast<int32_t>(sig.bitErrorRate),
            .timingAdvance = sig.timingAdvance,
    };
}

static aidl::CellInfoGsm toAidl(const V1_5::CellInfoGsm& info) {
    return {
            .cellIdentityGsm = toAidl(info.cellIdentityGsm),
            .signalStrengthGsm = toAidl(info.signalStrengthGsm),
    };
}

static aidl::WcdmaSignalStrength toAidl(const V1_2::WcdmaSignalStrength& sig) {
    return {
            .signalStrength = sig.base.signalStrength,
            .bitErrorRate = sig.base.bitErrorRate,
            .rscp = static_cast<int32_t>(sig.rscp),
            .ecno = static_cast<int32_t>(sig.ecno),
    };
}

static aidl::CellInfoWcdma toAidl(const V1_5::CellInfoWcdma& info) {
    return {
            .cellIdentityWcdma = toAidl(info.cellIdentityWcdma),
            .signalStrengthWcdma = toAidl(info.signalStrengthWcdma),
    };
}

static aidl::TdscdmaSignalStrength toAidl(const V1_2::TdscdmaSignalStrength& sig) {
    return {
            .signalStrength = static_cast<int32_t>(sig.signalStrength),
            .bitErrorRate = static_cast<int32_t>(sig.bitErrorRate),
            .rscp = static_cast<int32_t>(sig.rscp),
    };
}

static aidl::CellInfoTdscdma toAidl(const V1_5::CellInfoTdscdma& info) {
    return {
            .cellIdentityTdscdma = toAidl(info.cellIdentityTdscdma),
            .signalStrengthTdscdma = toAidl(info.signalStrengthTdscdma),
    };
}

static aidl::LteSignalStrength toAidl(const V1_6::LteSignalStrength& sig) {
    return {
            .signalStrength = static_cast<int32_t>(sig.base.signalStrength),
            .rsrp = static_cast<int32_t>(sig.base.rsrp),
            .rsrq = static_cast<int32_t>(sig.base.rsrq),
            .rssnr = sig.base.rssnr,
            .cqi = static_cast<int32_t>(sig.base.cqi),
            .timingAdvance = static_cast<int32_t>(sig.base.timingAdvance),
            .cqiTableIndex = static_cast<int32_t>(sig.cqiTableIndex),
    };
}

static aidl::LteSignalStrength toAidl(const V1_0::LteSignalStrength& sig) {
    return toAidl({sig, 0});
}

static aidl::CellInfoLte toAidl(const V1_5::CellInfoLte& info) {
    return {
            .cellIdentityLte = toAidl(info.cellIdentityLte),
            .signalStrengthLte = toAidl(info.signalStrengthLte),
    };
}

static aidl::CellInfoLte toAidl(const V1_6::CellInfoLte& info) {
    return {
            .cellIdentityLte = toAidl(info.cellIdentityLte),
            .signalStrengthLte = toAidl(info.signalStrengthLte),
    };
}

static aidl::NrSignalStrength toAidl(const V1_6::NrSignalStrength& sig) {
    return {
            .ssRsrp = sig.base.ssRsrp,
            .ssRsrq = sig.base.ssRsrq,
            .ssSinr = sig.base.ssSinr,
            .csiRsrp = sig.base.csiRsrp,
            .csiRsrq = sig.base.csiRsrq,
            .csiSinr = sig.base.csiSinr,
            .csiCqiTableIndex = static_cast<int32_t>(sig.csiCqiTableIndex),
            .csiCqiReport = sig.csiCqiReport,
    };
}

static aidl::NrSignalStrength toAidl(const V1_4::NrSignalStrength& sig) {
    return toAidl({sig, 0, 0});
}

static aidl::CellInfoNr toAidl(const V1_5::CellInfoNr& info) {
    return {
            .cellIdentityNr = toAidl(info.cellIdentityNr),
            .signalStrengthNr = toAidl(info.signalStrengthNr),
    };
}

static aidl::CellInfoNr toAidl(const V1_6::CellInfoNr& info) {
    return {
            .cellIdentityNr = toAidl(info.cellIdentityNr),
            .signalStrengthNr = toAidl(info.signalStrengthNr),
    };
}

static aidl::CdmaSignalStrength toAidl(const V1_0::CdmaSignalStrength& sig) {
    return {
            .dbm = static_cast<int32_t>(sig.dbm),
            .ecio = static_cast<int32_t>(sig.ecio),
    };
}

static aidl::EvdoSignalStrength toAidl(const V1_0::EvdoSignalStrength& sig) {
    return {
            .dbm = static_cast<int32_t>(sig.dbm),
            .ecio = static_cast<int32_t>(sig.ecio),
            .signalNoiseRatio = static_cast<int32_t>(sig.signalNoiseRatio),
    };
}

static aidl::CellInfoCdma toAidl(const V1_2::CellInfoCdma& info) {
    return {
            .cellIdentityCdma = toAidl(info.cellIdentityCdma),
            .signalStrengthCdma = toAidl(info.signalStrengthCdma),
            .signalStrengthEvdo = toAidl(info.signalStrengthEvdo),
    };
}

static aidl::CellInfoRatSpecificInfo toAidl(const V1_5::CellInfo::CellInfoRatSpecificInfo& ci) {
    using Discr = V1_5::CellInfo::CellInfoRatSpecificInfo::hidl_discriminator;
    const auto discr = ci.getDiscriminator();

    if (discr == Discr::gsm) return toAidl(ci.gsm());
    if (discr == Discr::wcdma) return toAidl(ci.wcdma());
    if (discr == Discr::tdscdma) return toAidl(ci.tdscdma());
    if (discr == Discr::lte) return toAidl(ci.lte());
    if (discr == Discr::nr) return toAidl(ci.nr());
    if (discr == Discr::cdma) return toAidl(ci.cdma());

    return {};
}

static aidl::CellInfoRatSpecificInfo toAidl(const V1_6::CellInfo::CellInfoRatSpecificInfo& ci) {
    using Discr = V1_6::CellInfo::CellInfoRatSpecificInfo::hidl_discriminator;
    const auto discr = ci.getDiscriminator();

    if (discr == Discr::gsm) return toAidl(ci.gsm());
    if (discr == Discr::wcdma) return toAidl(ci.wcdma());
    if (discr == Discr::tdscdma) return toAidl(ci.tdscdma());
    if (discr == Discr::lte) return toAidl(ci.lte());
    if (discr == Discr::nr) return toAidl(ci.nr());
    if (discr == Discr::cdma) return toAidl(ci.cdma());

    return {};
}

aidl::CellInfo toAidl(const V1_5::CellInfo& info) {
    return {
            .registered = info.registered,
            // ignored: timeStampType and timeStamp
            .connectionStatus = aidl::CellConnectionStatus(info.connectionStatus),
            .ratSpecificInfo = toAidl(info.ratSpecificInfo),
    };
}

aidl::CellInfo toAidl(const V1_6::CellInfo& info) {
    return {
            .registered = info.registered,
            .connectionStatus = aidl::CellConnectionStatus(info.connectionStatus),
            .ratSpecificInfo = toAidl(info.ratSpecificInfo),
    };
}

aidl::LinkCapacityEstimate toAidl(const V1_2::LinkCapacityEstimate& e) {
    return {
            .downlinkCapacityKbps = static_cast<int32_t>(e.downlinkCapacityKbps),
            .uplinkCapacityKbps = static_cast<int32_t>(e.uplinkCapacityKbps),
    };
}

aidl::LinkCapacityEstimate toAidl(const V1_6::LinkCapacityEstimate& e) {
    return {
            .downlinkCapacityKbps = static_cast<int32_t>(e.downlinkCapacityKbps),
            .uplinkCapacityKbps = static_cast<int32_t>(e.uplinkCapacityKbps),
            .secondaryDownlinkCapacityKbps = static_cast<int32_t>(e.secondaryDownlinkCapacityKbps),
            .secondaryUplinkCapacityKbps = static_cast<int32_t>(e.secondaryUplinkCapacityKbps),
    };
}

static aidl::PhysicalChannelConfigBand toAidl(const V1_6::PhysicalChannelConfig::Band& band) {
    using Discr = V1_6::PhysicalChannelConfig::Band::hidl_discriminator;
    const auto discr = band.getDiscriminator();

    if (discr == Discr::geranBand) return aidl::GeranBands(band.geranBand());
    if (discr == Discr::utranBand) return aidl::UtranBands(band.utranBand());
    if (discr == Discr::eutranBand) return aidl::EutranBands(band.eutranBand());
    if (discr == Discr::ngranBand) return aidl::NgranBands(band.ngranBand());

    return {};
}

aidl::PhysicalChannelConfig toAidl(const V1_4::PhysicalChannelConfig& cfg) {
    int32_t downlinkChannelNumber = 0;
    // ignored rfInfo.range
    using Discr = V1_4::RadioFrequencyInfo::hidl_discriminator;
    if (cfg.rfInfo.getDiscriminator() == Discr::channelNumber) {
        downlinkChannelNumber = cfg.rfInfo.channelNumber();
    }

    return {
            .status = aidl::CellConnectionStatus(cfg.base.status),
            .rat = RadioTechnology(cfg.rat),
            .downlinkChannelNumber = downlinkChannelNumber,
            .cellBandwidthDownlinkKhz = cfg.base.cellBandwidthDownlink,
            .contextIds = cfg.contextIds,
            .physicalCellId = static_cast<int32_t>(cfg.physicalCellId),
    };
}

aidl::PhysicalChannelConfig toAidl(const V1_6::PhysicalChannelConfig& cfg) {
    return {
            .status = aidl::CellConnectionStatus(cfg.status),
            .rat = RadioTechnology(cfg.rat),
            .downlinkChannelNumber = cfg.downlinkChannelNumber,
            .uplinkChannelNumber = cfg.uplinkChannelNumber,
            .cellBandwidthDownlinkKhz = cfg.cellBandwidthDownlinkKhz,
            .cellBandwidthUplinkKhz = cfg.cellBandwidthUplinkKhz,
            .contextIds = cfg.contextIds,
            .physicalCellId = static_cast<int32_t>(cfg.physicalCellId),
            .band = toAidl(cfg.band),
    };
}

aidl::SignalStrength toAidl(const V1_4::SignalStrength& sig) {
    return {
            .gsm = toAidl(sig.gsm),
            .cdma = toAidl(sig.cdma),
            .evdo = toAidl(sig.evdo),
            .lte = toAidl(sig.lte),
            .tdscdma = toAidl(sig.tdscdma),
            .wcdma = toAidl(sig.wcdma),
            .nr = toAidl(sig.nr),
    };
}

aidl::SignalStrength toAidl(const V1_6::SignalStrength& sig) {
    return {
            .gsm = toAidl(sig.gsm),
            .cdma = toAidl(sig.cdma),
            .evdo = toAidl(sig.evdo),
            .lte = toAidl(sig.lte),
            .tdscdma = toAidl(sig.tdscdma),
            .wcdma = toAidl(sig.wcdma),
            .nr = toAidl(sig.nr),
    };
}

aidl::NetworkScanResult toAidl(const V1_5::NetworkScanResult& res) {
    return {
            .status = static_cast<int32_t>(res.status),
            .error = toAidl(res.error),
            .networkInfos = toAidl(res.networkInfos),
    };
}

aidl::NetworkScanResult toAidl(const V1_6::NetworkScanResult& res) {
    return {
            .status = static_cast<int32_t>(res.status),
            .error = toAidl(res.error),
            .networkInfos = toAidl(res.networkInfos),
    };
}

aidl::SuppSvcNotification toAidl(const V1_0::SuppSvcNotification& svc) {
    return {
            .isMT = svc.isMT,
            .code = svc.code,
            .index = svc.index,
            .type = svc.type,
            .number = svc.number,
    };
}

aidl::OperatorInfo toAidl(const V1_0::OperatorInfo& info) {
    return {
            .alphaLong = info.alphaLong,
            .alphaShort = info.alphaShort,
            .operatorNumeric = info.operatorNumeric,
            .status = static_cast<int32_t>(info.status),
    };
}

static aidl::Cdma2000RegistrationInfo  //
toAidl(const V1_5::RegStateResult::AccessTechnologySpecificInfo::Cdma2000RegistrationInfo& info) {
    return {
            .cssSupported = info.cssSupported,
            .roamingIndicator = info.roamingIndicator,
            .systemIsInPrl = static_cast<int32_t>(info.systemIsInPrl),
            .defaultRoamingIndicator = info.defaultRoamingIndicator,
    };
}

static aidl::LteVopsInfo toAidl(const V1_4::LteVopsInfo& info) {
    return {
            .isVopsSupported = info.isVopsSupported,
            .isEmcBearerSupported = info.isEmcBearerSupported,
    };
}

static aidl::NrIndicators toAidl(const V1_4::NrIndicators& info) {
    return {
            .isEndcAvailable = info.isEndcAvailable,
            .isDcNrRestricted = info.isDcNrRestricted,
            .isNrAvailable = info.isNrAvailable,
    };
}

static aidl::EutranRegistrationInfo  //
toAidl(const V1_5::RegStateResult::AccessTechnologySpecificInfo::EutranRegistrationInfo& info) {
    return {
            .lteVopsInfo = toAidl(info.lteVopsInfo),
            .nrIndicators = toAidl(info.nrIndicators),
    };
}

static aidl::NrVopsInfo toAidl(const V1_6::NrVopsInfo& info) {
    return {
            .vopsSupported = static_cast<int8_t>(info.vopsSupported),
            .emcSupported = static_cast<int8_t>(info.emcSupported),
            .emfSupported = static_cast<int8_t>(info.emfSupported),
    };
}

static aidl::AccessTechnologySpecificInfo  //
toAidl(const V1_5::RegStateResult::AccessTechnologySpecificInfo& info) {
    using Discr = V1_5::RegStateResult::AccessTechnologySpecificInfo::hidl_discriminator;
    const auto discr = info.getDiscriminator();

    if (discr == Discr::cdmaInfo) return toAidl(info.cdmaInfo());
    if (discr == Discr::eutranInfo) return toAidl(info.eutranInfo());

    return {};
}

static aidl::AccessTechnologySpecificInfo  //
toAidl(const V1_6::RegStateResult::AccessTechnologySpecificInfo& info) {
    using Discr = V1_6::RegStateResult::AccessTechnologySpecificInfo::hidl_discriminator;
    const auto discr = info.getDiscriminator();

    if (discr == Discr::cdmaInfo) return toAidl(info.cdmaInfo());
    if (discr == Discr::eutranInfo) return toAidl(info.eutranInfo());
    if (discr == Discr::ngranNrVopsInfo) return toAidl(info.ngranNrVopsInfo());
    if (discr == Discr::geranDtmSupported) {
        using T = aidl::AccessTechnologySpecificInfo;
        return T::make<T::Tag::geranDtmSupported>(info.geranDtmSupported());
    }

    return {};
}

aidl::RegStateResult toAidl(const V1_5::RegStateResult& res) {
    return {
            .regState = aidl::RegState(res.regState),
            .rat = RadioTechnology(res.rat),
            .reasonForDenial = aidl::RegistrationFailCause(res.reasonForDenial),
            .cellIdentity = toAidl(res.cellIdentity),
            .registeredPlmn = res.registeredPlmn,
            .accessTechnologySpecificInfo = toAidl(res.accessTechnologySpecificInfo),
    };
}

aidl::RegStateResult toAidl(const V1_6::RegStateResult& res) {
    return {
            .regState = aidl::RegState(res.regState),
            .rat = RadioTechnology(res.rat),
            .reasonForDenial = aidl::RegistrationFailCause(res.reasonForDenial),
            .cellIdentity = toAidl(res.cellIdentity),
            .registeredPlmn = res.registeredPlmn,
            .accessTechnologySpecificInfo = toAidl(res.accessTechnologySpecificInfo),
    };
}

aidl::LceDataInfo toAidl(const V1_0::LceDataInfo& info) {
    return {
            .lastHopCapacityKbps = static_cast<int32_t>(info.lastHopCapacityKbps),
            .confidenceLevel = static_cast<int8_t>(info.confidenceLevel),
            .lceSuspended = info.lceSuspended,
    };
}

}  // namespace android::hardware::radio::compat
