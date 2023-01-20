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

namespace aidl = ::aidl::android::hardware::radio::data;

V1_5::DataProfileInfo toHidl(const aidl::DataProfileInfo& info) {
    return {
            .profileId = static_cast<V1_0::DataProfileId>(info.profileId),
            .apn = info.apn,
            .protocol = static_cast<V1_4::PdpProtocolType>(info.protocol),
            .roamingProtocol = static_cast<V1_4::PdpProtocolType>(info.roamingProtocol),
            .authType = static_cast<V1_0::ApnAuthType>(info.authType),
            .user = info.user,
            .password = info.password,
            .type = static_cast<V1_0::DataProfileInfoType>(info.type),
            .maxConnsTime = info.maxConnsTime,
            .maxConns = info.maxConns,
            .waitTime = info.waitTime,
            .enabled = info.enabled,
            .supportedApnTypesBitmap = toHidlBitfield<V1_5::ApnTypes>(info.supportedApnTypesBitmap),
            .bearerBitmap = toHidlBitfield<V1_4::RadioAccessFamily>(info.bearerBitmap),
            .mtuV4 = info.mtuV4,
            .mtuV6 = info.mtuV6,
            .preferred = info.preferred,
            .persistent = info.persistent,
    };
}

V1_5::LinkAddress toHidl(const aidl::LinkAddress& addr) {
    return {
            .address = addr.address,
            .properties = addr.addressProperties,
            .deprecationTime = static_cast<uint64_t>(addr.deprecationTime),
            .expirationTime = static_cast<uint64_t>(addr.expirationTime),
    };
}

aidl::SliceInfo toAidl(const V1_6::SliceInfo& info) {
    return {
            .sliceServiceType = static_cast<int8_t>(info.sst),
            .sliceDifferentiator = info.sliceDifferentiator,
            .mappedHplmnSst = static_cast<int8_t>(info.mappedHplmnSst),
            .mappedHplmnSd = info.mappedHplmnSD,
            .status = static_cast<int8_t>(info.status),
    };
}

V1_6::SliceInfo toHidl(const aidl::SliceInfo& info) {
    return {
            .sst = static_cast<V1_6::SliceServiceType>(info.sliceServiceType),
            .sliceDifferentiator = info.sliceDifferentiator,
            .mappedHplmnSst = static_cast<V1_6::SliceServiceType>(info.mappedHplmnSst),
            .mappedHplmnSD = info.mappedHplmnSd,
            .status = static_cast<V1_6::SliceStatus>(info.status),
    };
}

aidl::TrafficDescriptor toAidl(const V1_6::TrafficDescriptor& descr) {
    return {
            .dnn = toAidl(descr.dnn),
            .osAppId = toAidl(descr.osAppId),
    };
}

V1_6::TrafficDescriptor toHidl(const aidl::TrafficDescriptor& descr) {
    return {
            .dnn = toHidl<V1_6::OptionalDnn>(descr.dnn),
            .osAppId = toHidl<V1_6::OptionalOsAppId>(descr.osAppId),
    };
}

aidl::OsAppId toAidl(const V1_6::OsAppId& appId) {
    return {
            .osAppId = appId.osAppId,
    };
}

V1_6::OsAppId toHidl(const aidl::OsAppId& appId) {
    return {
            .osAppId = appId.osAppId,
    };
}

V1_1::KeepaliveRequest toHidl(const aidl::KeepaliveRequest& keep) {
    return {
            .type = static_cast<V1_1::KeepaliveType>(keep.type),
            .sourceAddress = keep.sourceAddress,
            .sourcePort = keep.sourcePort,
            .destinationAddress = keep.destinationAddress,
            .destinationPort = keep.destinationPort,
            .maxKeepaliveIntervalMillis = keep.maxKeepaliveIntervalMillis,
            .cid = keep.cid,
    };
}

static aidl::QosBandwidth toAidl(const V1_6::QosBandwidth& bw) {
    return {
            .maxBitrateKbps = static_cast<int32_t>(bw.maxBitrateKbps),
            .guaranteedBitrateKbps = static_cast<int32_t>(bw.guaranteedBitrateKbps),
    };
}

static aidl::EpsQos toAidl(const V1_6::EpsQos& qos) {
    return {
            .qci = qos.qci,
            .downlink = toAidl(qos.downlink),
            .uplink = toAidl(qos.uplink),
    };
}

static aidl::NrQos toAidl(const V1_6::NrQos& qos) {
    return {
            .fiveQi = qos.fiveQi,
            .downlink = toAidl(qos.downlink),
            .uplink = toAidl(qos.uplink),
            .qfi = static_cast<int8_t>(qos.qfi),
            .averagingWindowMillis = qos.averagingWindowMs,
    };
}

static std::variant<bool, aidl::EpsQos, aidl::NrQos> toAidl(const V1_6::Qos& qos) {
    if (qos.getDiscriminator() == V1_6::Qos::hidl_discriminator::eps) return toAidl(qos.eps());
    if (qos.getDiscriminator() == V1_6::Qos::hidl_discriminator::nr) return toAidl(qos.nr());
    return false;
}

aidl::SetupDataCallResult toAidl(const V1_5::SetupDataCallResult& res) {
    return {
            .cause = aidl::DataCallFailCause(res.cause),
            .suggestedRetryTime = res.suggestedRetryTime,
            .cid = res.cid,
            .active = static_cast<int32_t>(res.active),
            .type = aidl::PdpProtocolType(res.type),
            .ifname = res.ifname,
            .addresses = toAidl(res.addresses),
            .dnses = toAidl(res.dnses),
            .gateways = toAidl(res.gateways),
            .pcscf = toAidl(res.pcscf),
            .mtuV4 = res.mtuV4,
            .mtuV6 = res.mtuV6,
    };
}

aidl::SetupDataCallResult toAidl(const V1_6::SetupDataCallResult& res) {
    return {
            .cause = aidl::DataCallFailCause(res.cause),
            .suggestedRetryTime = res.suggestedRetryTime,
            .cid = res.cid,
            .active = static_cast<int32_t>(res.active),
            .type = aidl::PdpProtocolType(res.type),
            .ifname = res.ifname,
            .addresses = toAidl(res.addresses),
            .dnses = toAidl(res.dnses),
            .gateways = toAidl(res.gateways),
            .pcscf = toAidl(res.pcscf),
            .mtuV4 = res.mtuV4,
            .mtuV6 = res.mtuV6,
            .defaultQos = toAidl(res.defaultQos),
            .qosSessions = toAidl(res.qosSessions),
            .handoverFailureMode = static_cast<int8_t>(res.handoverFailureMode),
            .pduSessionId = res.pduSessionId,
            .sliceInfo = toAidl(res.sliceInfo),
            .trafficDescriptors = toAidl(res.trafficDescriptors),
    };
}

aidl::LinkAddress toAidl(const V1_5::LinkAddress& addr) {
    return {
            .address = addr.address,
            .addressProperties = addr.properties,
            .deprecationTime = static_cast<int64_t>(addr.deprecationTime),
            .expirationTime = static_cast<int64_t>(addr.expirationTime),
    };
}

aidl::QosSession toAidl(const V1_6::QosSession& sess) {
    return {
            .qosSessionId = sess.qosSessionId,
            .qos = toAidl(sess.qos),
            .qosFilters = toAidl(sess.qosFilters),
    };
}

static aidl::PortRange toAidl(const V1_6::PortRange& range) {
    return {
            .start = range.start,
            .end = range.end,
    };
}

static std::optional<aidl::PortRange> toAidl(const V1_6::MaybePort& opt) {
    if (opt.getDiscriminator() == V1_6::MaybePort::hidl_discriminator::noinit) return std::nullopt;
    return toAidl(opt.range());  // can't use MaybeX template - this field is not named "value"
}

aidl::QosFilter toAidl(const V1_6::QosFilter& filter) {
    return {
            .localAddresses = toAidl(filter.localAddresses),
            .remoteAddresses = toAidl(filter.remoteAddresses),
            .localPort = toAidl(filter.localPort),
            .remotePort = toAidl(filter.remotePort),
            .protocol = static_cast<int8_t>(filter.protocol),
            .tos = toAidlVariant(filter.tos),
            .flowLabel = toAidlVariant(filter.flowLabel),
            .spi = toAidlVariant(filter.spi),
            .direction = static_cast<int8_t>(filter.direction),
            .precedence = filter.precedence,
    };
}

aidl::KeepaliveStatus toAidl(const V1_1::KeepaliveStatus& status) {
    return {
            .sessionHandle = status.sessionHandle,
            .code = static_cast<int32_t>(status.code),
    };
}

aidl::PcoDataInfo toAidl(const V1_0::PcoDataInfo& info) {
    return {
            .cid = info.cid,
            .bearerProto = info.bearerProto,
            .pcoId = info.pcoId,
            .contents = info.contents,
    };
}

aidl::SlicingConfig toAidl(const V1_6::SlicingConfig& cfg) {
    return {
            .urspRules = toAidl(cfg.urspRules),
            .sliceInfo = toAidl(cfg.sliceInfo),
    };
}

aidl::UrspRule toAidl(const V1_6::UrspRule& rule) {
    return {
            .precedence = rule.precedence,
            .trafficDescriptors = toAidl(rule.trafficDescriptors),
            .routeSelectionDescriptor = toAidl(rule.routeSelectionDescriptor),
    };
}

static int8_t toAidl(const V1_6::OptionalSscMode& opt) {
    if (opt.getDiscriminator() == V1_6::OptionalSscMode::hidl_discriminator::noinit) {
        return aidl::RouteSelectionDescriptor::SSC_MODE_UNKNOWN;
    }
    return static_cast<int8_t>(opt.value());
}

static aidl::PdpProtocolType toAidl(const V1_6::OptionalPdpProtocolType& opt) {
    using discriminator = V1_6::OptionalPdpProtocolType::hidl_discriminator;
    if (opt.getDiscriminator() == discriminator::noinit) return aidl::PdpProtocolType::UNKNOWN;
    return aidl::PdpProtocolType(opt.value());
}

aidl::RouteSelectionDescriptor toAidl(const V1_6::RouteSelectionDescriptor& descr) {
    return {
            .precedence = static_cast<int8_t>(descr.precedence),
            .sessionType = toAidl(descr.sessionType),
            .sscMode = toAidl(descr.sscMode),
            .sliceInfo = toAidl(descr.sliceInfo),
            .dnn = toAidl(descr.dnn),
    };
}

}  // namespace android::hardware::radio::compat
