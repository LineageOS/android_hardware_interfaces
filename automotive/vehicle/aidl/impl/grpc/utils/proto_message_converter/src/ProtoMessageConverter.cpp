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

#define LOG_TAG "ProtoMsgConverter"

#include "ProtoMessageConverter.h"

#include <VehicleUtils.h>

#include <memory>
#include <vector>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace proto_msg_converter {

namespace aidl_vehicle = ::aidl::android::hardware::automotive::vehicle;
namespace proto = ::android::hardware::automotive::vehicle::proto;

// Copy the vector PROTO_VECNAME of protobuf class PROTO_VALUE to
// VHAL_TYPE_VALUE->VHAL_TYPE_VECNAME, every element of PROTO_VECNAME is casted by CAST.
#define CAST_COPY_PROTOBUF_VEC_TO_VHAL_TYPE(PROTO_VALUE, PROTO_VECNAME, VHAL_TYPE_VALUE, \
                                            VHAL_TYPE_VECNAME, CAST)                     \
    do {                                                                                 \
        (VHAL_TYPE_VALUE)->VHAL_TYPE_VECNAME.resize(PROTO_VALUE.PROTO_VECNAME##_size()); \
        size_t idx = 0;                                                                  \
        for (auto& value : PROTO_VALUE.PROTO_VECNAME()) {                                \
            VHAL_TYPE_VALUE->VHAL_TYPE_VECNAME[idx++] = CAST(value);                     \
        }                                                                                \
    } while (0)

// Copying the vector PROTO_VECNAME of protobuf class PROTO_VALUE to
// VHAL_TYPE_VALUE->VHAL_TYPE_VECNAME.
#define COPY_PROTOBUF_VEC_TO_VHAL_TYPE(PROTO_VALUE, PROTO_VECNAME, VHAL_TYPE_VALUE, \
                                       VHAL_TYPE_VECNAME)                           \
    CAST_COPY_PROTOBUF_VEC_TO_VHAL_TYPE(                                            \
            PROTO_VALUE, PROTO_VECNAME, VHAL_TYPE_VALUE, VHAL_TYPE_VECNAME, /*NO CAST*/)

void aidlToProto(const aidl_vehicle::VehiclePropConfig& in, proto::VehiclePropConfig* out) {
    out->set_prop(in.prop);
    out->set_access(static_cast<proto::VehiclePropertyAccess>(toInt(in.access)));
    out->set_change_mode(static_cast<proto::VehiclePropertyChangeMode>(toInt(in.changeMode)));
    out->set_config_string(in.configString.c_str(), in.configString.size());
    out->set_min_sample_rate(in.minSampleRate);
    out->set_max_sample_rate(in.maxSampleRate);

    for (auto& configElement : in.configArray) {
        out->add_config_array(configElement);
    }

    out->clear_area_configs();
    for (auto& areaConfig : in.areaConfigs) {
        auto* protoACfg = out->add_area_configs();
        protoACfg->set_area_id(areaConfig.areaId);
        protoACfg->set_access(static_cast<proto::VehiclePropertyAccess>(toInt(areaConfig.access)));
        protoACfg->set_min_int64_value(areaConfig.minInt64Value);
        protoACfg->set_max_int64_value(areaConfig.maxInt64Value);
        protoACfg->set_min_float_value(areaConfig.minFloatValue);
        protoACfg->set_max_float_value(areaConfig.maxFloatValue);
        protoACfg->set_min_int32_value(areaConfig.minInt32Value);
        protoACfg->set_max_int32_value(areaConfig.maxInt32Value);
        if (areaConfig.supportedEnumValues.has_value()) {
            for (auto& supportedEnumValue : areaConfig.supportedEnumValues.value()) {
                protoACfg->add_supported_enum_values(supportedEnumValue);
            }
        }
        protoACfg->set_support_variable_update_rate(areaConfig.supportVariableUpdateRate);
    }
}

void protoToAidl(const proto::VehiclePropConfig& in, aidl_vehicle::VehiclePropConfig* out) {
    out->prop = in.prop();
    out->access = static_cast<aidl_vehicle::VehiclePropertyAccess>(in.access());
    out->changeMode = static_cast<aidl_vehicle::VehiclePropertyChangeMode>(in.change_mode());
    out->configString = in.config_string();
    out->minSampleRate = in.min_sample_rate();
    out->maxSampleRate = in.max_sample_rate();

    COPY_PROTOBUF_VEC_TO_VHAL_TYPE(in, config_array, out, configArray);

    auto cast_to_acfg = [](const proto::VehicleAreaConfig& protoAcfg) {
        auto vehicleAreaConfig = aidl_vehicle::VehicleAreaConfig{
                .areaId = protoAcfg.area_id(),
                .access = static_cast<aidl_vehicle::VehiclePropertyAccess>(protoAcfg.access()),
                .minInt32Value = protoAcfg.min_int32_value(),
                .maxInt32Value = protoAcfg.max_int32_value(),
                .minInt64Value = protoAcfg.min_int64_value(),
                .maxInt64Value = protoAcfg.max_int64_value(),
                .minFloatValue = protoAcfg.min_float_value(),
                .maxFloatValue = protoAcfg.max_float_value(),
                .supportVariableUpdateRate = protoAcfg.support_variable_update_rate(),
        };
        if (protoAcfg.supported_enum_values().size() != 0) {
            vehicleAreaConfig.supportedEnumValues = std::vector<int64_t>();
            COPY_PROTOBUF_VEC_TO_VHAL_TYPE(protoAcfg, supported_enum_values, (&vehicleAreaConfig),
                                           supportedEnumValues.value());
        }

        return vehicleAreaConfig;
    };
    CAST_COPY_PROTOBUF_VEC_TO_VHAL_TYPE(in, area_configs, out, areaConfigs, cast_to_acfg);
}

void aidlToProto(const aidl_vehicle::VehiclePropValue& in, proto::VehiclePropValue* out) {
    out->set_prop(in.prop);
    out->set_timestamp(in.timestamp);
    out->set_status(static_cast<proto::VehiclePropertyStatus>(in.status));
    out->set_area_id(in.areaId);
    out->set_string_value(in.value.stringValue);
    out->set_byte_values(in.value.byteValues.data(), in.value.byteValues.size());

    for (auto& int32Value : in.value.int32Values) {
        out->add_int32_values(int32Value);
    }

    for (auto& int64Value : in.value.int64Values) {
        out->add_int64_values(int64Value);
    }

    for (auto& floatValue : in.value.floatValues) {
        out->add_float_values(floatValue);
    }
}

void protoToAidl(const proto::VehiclePropValue& in, aidl_vehicle::VehiclePropValue* out) {
    out->prop = in.prop();
    out->timestamp = in.timestamp();
    out->status = static_cast<aidl_vehicle::VehiclePropertyStatus>(in.status());
    out->areaId = in.area_id();
    out->value.stringValue = in.string_value();
    for (const char& byte : in.byte_values()) {
        out->value.byteValues.push_back(byte);
    }

    COPY_PROTOBUF_VEC_TO_VHAL_TYPE(in, int32_values, out, value.int32Values);
    COPY_PROTOBUF_VEC_TO_VHAL_TYPE(in, int64_values, out, value.int64Values);
    COPY_PROTOBUF_VEC_TO_VHAL_TYPE(in, float_values, out, value.floatValues);
}

#undef COPY_PROTOBUF_VEC_TO_VHAL_TYPE
#undef CAST_COPY_PROTOBUF_VEC_TO_VHAL_TYPE

}  // namespace proto_msg_converter
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
