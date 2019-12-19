/*
 * Copyright (C) 2019 The Android Open Source Project
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

#include <memory>
#include <vector>

#include <log/log.h>

#include <vhal_v2_0/VehicleUtils.h>

#include "ProtoMessageConverter.h"

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace V2_0 {

namespace impl {

namespace proto_msg_converter {

// If protobuf class PROTO_VALUE has value in field PROTO_VARNAME,
// then casting the value by CAST and copying it to VHAL_TYPE_VALUE->VHAL_TYPE_VARNAME
#define CHECK_CAST_COPY_PROTOBUF_VAR_TO_VHAL_TYPE(PROTO_VALUE, PROTO_VARNAME, VHAL_TYPE_VALUE, \
                                                  VHAL_TYPE_VARNAME, CAST)                     \
    if (PROTO_VALUE.has_##PROTO_VARNAME()) {                                                   \
        (VHAL_TYPE_VALUE)->VHAL_TYPE_VARNAME = CAST(PROTO_VALUE.PROTO_VARNAME());              \
    }

// Copying the vector PROTO_VECNAME of protobuf class PROTO_VALUE to
// VHAL_TYPE_VALUE->VHAL_TYPE_VECNAME, every element of PROTO_VECNAME
// is casted by CAST
#define CAST_COPY_PROTOBUF_VEC_TO_VHAL_TYPE(PROTO_VALUE, PROTO_VECNAME, VHAL_TYPE_VALUE, \
                                            VHAL_TYPE_VECNAME, CAST)                     \
    do {                                                                                 \
        (VHAL_TYPE_VALUE)->VHAL_TYPE_VECNAME.resize(PROTO_VALUE.PROTO_VECNAME##_size()); \
        size_t idx = 0;                                                                  \
        for (auto& value : PROTO_VALUE.PROTO_VECNAME()) {                                \
            VHAL_TYPE_VALUE->VHAL_TYPE_VECNAME[idx++] = CAST(value);                     \
        }                                                                                \
    } while (0)

// If protobuf message has value in field PROTO_VARNAME,
// then copying it to VHAL_TYPE_VALUE->VHAL_TYPE_VARNAME
#define CHECK_COPY_PROTOBUF_VAR_TO_VHAL_TYPE(PROTO_VALUE, PROTO_VARNAME, VHAL_TYPE_VALUE, \
                                             VHAL_TYPE_VARNAME)                           \
    CHECK_CAST_COPY_PROTOBUF_VAR_TO_VHAL_TYPE(                                            \
            PROTO_VALUE, PROTO_VARNAME, VHAL_TYPE_VALUE, VHAL_TYPE_VARNAME, /*NO CAST*/)

// Copying the vector PROTO_VECNAME of protobuf class PROTO_VALUE to
// VHAL_TYPE_VALUE->VHAL_TYPE_VECNAME
#define COPY_PROTOBUF_VEC_TO_VHAL_TYPE(PROTO_VALUE, PROTO_VECNAME, VHAL_TYPE_VALUE, \
                                       VHAL_TYPE_VECNAME)                           \
    CAST_COPY_PROTOBUF_VEC_TO_VHAL_TYPE(                                            \
            PROTO_VALUE, PROTO_VECNAME, VHAL_TYPE_VALUE, VHAL_TYPE_VECNAME, /*NO CAST*/)

void toProto(vhal_proto::VehiclePropConfig* protoCfg, const VehiclePropConfig& cfg) {
    protoCfg->set_prop(cfg.prop);
    protoCfg->set_access(toInt(cfg.access));
    protoCfg->set_change_mode(toInt(cfg.changeMode));
    protoCfg->set_value_type(toInt(getPropType(cfg.prop)));

    for (auto& configElement : cfg.configArray) {
        protoCfg->add_config_array(configElement);
    }

    if (cfg.configString.size() > 0) {
        protoCfg->set_config_string(cfg.configString.c_str(), cfg.configString.size());
    }

    protoCfg->clear_area_configs();
    for (auto& areaConfig : cfg.areaConfigs) {
        auto* protoACfg = protoCfg->add_area_configs();
        protoACfg->set_area_id(areaConfig.areaId);

        switch (getPropType(cfg.prop)) {
            case VehiclePropertyType::STRING:
            case VehiclePropertyType::BOOLEAN:
            case VehiclePropertyType::INT32_VEC:
            case VehiclePropertyType::INT64_VEC:
            case VehiclePropertyType::FLOAT_VEC:
            case VehiclePropertyType::BYTES:
            case VehiclePropertyType::MIXED:
                // Do nothing.  These types don't have min/max values
                break;
            case VehiclePropertyType::INT64:
                protoACfg->set_min_int64_value(areaConfig.minInt64Value);
                protoACfg->set_max_int64_value(areaConfig.maxInt64Value);
                break;
            case VehiclePropertyType::FLOAT:
                protoACfg->set_min_float_value(areaConfig.minFloatValue);
                protoACfg->set_max_float_value(areaConfig.maxFloatValue);
                break;
            case VehiclePropertyType::INT32:
                protoACfg->set_min_int32_value(areaConfig.minInt32Value);
                protoACfg->set_max_int32_value(areaConfig.maxInt32Value);
                break;
            default:
                ALOGW("%s: Unknown property type:  0x%x", __func__, toInt(getPropType(cfg.prop)));
                break;
        }
    }

    protoCfg->set_min_sample_rate(cfg.minSampleRate);
    protoCfg->set_max_sample_rate(cfg.maxSampleRate);
}

void fromProto(VehiclePropConfig* cfg, const vhal_proto::VehiclePropConfig& protoCfg) {
    CHECK_COPY_PROTOBUF_VAR_TO_VHAL_TYPE(protoCfg, prop, cfg, prop);
    CHECK_CAST_COPY_PROTOBUF_VAR_TO_VHAL_TYPE(protoCfg, access, cfg, access,
                                              static_cast<VehiclePropertyAccess>);
    CHECK_CAST_COPY_PROTOBUF_VAR_TO_VHAL_TYPE(protoCfg, change_mode, cfg, changeMode,
                                              static_cast<VehiclePropertyChangeMode>);
    COPY_PROTOBUF_VEC_TO_VHAL_TYPE(protoCfg, config_array, cfg, configArray);
    CHECK_COPY_PROTOBUF_VAR_TO_VHAL_TYPE(protoCfg, config_string, cfg, configString);

    auto cast_to_acfg = [](const vhal_proto::VehicleAreaConfig& protoAcfg) {
        VehicleAreaConfig acfg;
        CHECK_COPY_PROTOBUF_VAR_TO_VHAL_TYPE(protoAcfg, area_id, &acfg, areaId);
        CHECK_COPY_PROTOBUF_VAR_TO_VHAL_TYPE(protoAcfg, min_int32_value, &acfg, minInt32Value);
        CHECK_COPY_PROTOBUF_VAR_TO_VHAL_TYPE(protoAcfg, max_int32_value, &acfg, maxInt32Value);
        CHECK_COPY_PROTOBUF_VAR_TO_VHAL_TYPE(protoAcfg, min_int64_value, &acfg, minInt64Value);
        CHECK_COPY_PROTOBUF_VAR_TO_VHAL_TYPE(protoAcfg, max_int64_value, &acfg, maxInt64Value);
        CHECK_COPY_PROTOBUF_VAR_TO_VHAL_TYPE(protoAcfg, min_float_value, &acfg, minFloatValue);
        CHECK_COPY_PROTOBUF_VAR_TO_VHAL_TYPE(protoAcfg, max_float_value, &acfg, maxFloatValue);
        return acfg;
    };

    CAST_COPY_PROTOBUF_VEC_TO_VHAL_TYPE(protoCfg, area_configs, cfg, areaConfigs, cast_to_acfg);

    CHECK_COPY_PROTOBUF_VAR_TO_VHAL_TYPE(protoCfg, min_sample_rate, cfg, minSampleRate);
    CHECK_COPY_PROTOBUF_VAR_TO_VHAL_TYPE(protoCfg, max_sample_rate, cfg, maxSampleRate);
}

void toProto(vhal_proto::VehiclePropValue* protoVal, const VehiclePropValue& val) {
    protoVal->set_prop(val.prop);
    protoVal->set_value_type(toInt(getPropType(val.prop)));
    protoVal->set_timestamp(val.timestamp);
    protoVal->set_status((vhal_proto::VehiclePropStatus)(val.status));
    protoVal->set_area_id(val.areaId);

    // Copy value data if it is set.
    //  - for bytes and strings, this is indicated by size > 0
    //  - for int32, int64, and float, copy the values if vectors have data
    if (val.value.stringValue.size() > 0) {
        protoVal->set_string_value(val.value.stringValue.c_str(), val.value.stringValue.size());
    }

    if (val.value.bytes.size() > 0) {
        protoVal->set_bytes_value(val.value.bytes.data(), val.value.bytes.size());
    }

    for (auto& int32Value : val.value.int32Values) {
        protoVal->add_int32_values(int32Value);
    }

    for (auto& int64Value : val.value.int64Values) {
        protoVal->add_int64_values(int64Value);
    }

    for (auto& floatValue : val.value.floatValues) {
        protoVal->add_float_values(floatValue);
    }
}

void fromProto(VehiclePropValue* val, const vhal_proto::VehiclePropValue& protoVal) {
    CHECK_COPY_PROTOBUF_VAR_TO_VHAL_TYPE(protoVal, prop, val, prop);
    CHECK_COPY_PROTOBUF_VAR_TO_VHAL_TYPE(protoVal, timestamp, val, timestamp);
    CHECK_CAST_COPY_PROTOBUF_VAR_TO_VHAL_TYPE(protoVal, status, val, status,
                                              static_cast<VehiclePropertyStatus>);
    CHECK_COPY_PROTOBUF_VAR_TO_VHAL_TYPE(protoVal, area_id, val, areaId);

    // Copy value data
    CHECK_COPY_PROTOBUF_VAR_TO_VHAL_TYPE(protoVal, string_value, val, value.stringValue);

    auto cast_proto_bytes_to_vec = [](auto&& bytes) {
        return std::vector<uint8_t>(bytes.begin(), bytes.end());
    };
    CHECK_CAST_COPY_PROTOBUF_VAR_TO_VHAL_TYPE(protoVal, bytes_value, val, value.bytes,
                                              cast_proto_bytes_to_vec);

    COPY_PROTOBUF_VEC_TO_VHAL_TYPE(protoVal, int32_values, val, value.int32Values);
    COPY_PROTOBUF_VEC_TO_VHAL_TYPE(protoVal, int64_values, val, value.int64Values);
    COPY_PROTOBUF_VEC_TO_VHAL_TYPE(protoVal, float_values, val, value.floatValues);
}

#undef COPY_PROTOBUF_VEC_TO_VHAL_TYPE
#undef CHECK_COPY_PROTOBUF_VAR_TO_VHAL_TYPE
#undef CAST_COPY_PROTOBUF_VEC_TO_VHAL_TYPE
#undef CHECK_CAST_COPY_PROTOBUF_VAR_TO_VHAL_TYPE

}  // namespace proto_msg_converter

}  // namespace impl

}  // namespace V2_0
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
