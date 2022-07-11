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

#include <JsonConfigLoader.h>

#include <AccessForVehicleProperty.h>
#include <ChangeModeForVehicleProperty.h>
#include <PropertyUtils.h>

#ifdef ENABLE_VEHICLE_HAL_TEST_PROPERTIES
#include <TestPropertyUtils.h>
#endif  // ENABLE_VEHICLE_HAL_TEST_PROPERTIES

#include <android-base/strings.h>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {

namespace jsonconfigloader_impl {

using ::aidl::android::hardware::automotive::vehicle::AccessForVehicleProperty;
using ::aidl::android::hardware::automotive::vehicle::ChangeModeForVehicleProperty;
using ::aidl::android::hardware::automotive::vehicle::EvConnectorType;
using ::aidl::android::hardware::automotive::vehicle::EvsServiceState;
using ::aidl::android::hardware::automotive::vehicle::EvsServiceType;
using ::aidl::android::hardware::automotive::vehicle::FuelType;
using ::aidl::android::hardware::automotive::vehicle::GsrComplianceRequirementType;
using ::aidl::android::hardware::automotive::vehicle::RawPropValues;
using ::aidl::android::hardware::automotive::vehicle::VehicleApPowerStateReport;
using ::aidl::android::hardware::automotive::vehicle::VehicleApPowerStateReq;
using ::aidl::android::hardware::automotive::vehicle::VehicleAreaConfig;
using ::aidl::android::hardware::automotive::vehicle::VehicleAreaWindow;
using ::aidl::android::hardware::automotive::vehicle::VehicleGear;
using ::aidl::android::hardware::automotive::vehicle::VehicleHvacFanDirection;
using ::aidl::android::hardware::automotive::vehicle::VehicleIgnitionState;
using ::aidl::android::hardware::automotive::vehicle::VehicleOilLevel;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropConfig;
using ::aidl::android::hardware::automotive::vehicle::VehicleProperty;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropertyAccess;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropertyChangeMode;
using ::aidl::android::hardware::automotive::vehicle::VehicleSeatOccupancyState;
using ::aidl::android::hardware::automotive::vehicle::VehicleTurnSignal;
using ::aidl::android::hardware::automotive::vehicle::VehicleUnit;
using ::aidl::android::hardware::automotive::vehicle::VehicleVendorPermission;

using ::android::base::Error;
using ::android::base::Result;

template <>
Result<int32_t> JsonValueParser::convertValueToType<int32_t>(const std::string& fieldName,
                                                             const Json::Value& value) {
    if (!value.isInt()) {
        return Error() << "The value: " << value << " for field: " << fieldName
                       << " is not in correct type, expect int";
    }
    return static_cast<int32_t>(value.asInt());
}

template <>
Result<float> JsonValueParser::convertValueToType<float>(const std::string& fieldName,
                                                         const Json::Value& value) {
    // isFloat value does not exist, so we use isDouble here.
    if (!value.isDouble()) {
        return Error() << "The value: " << value << " for field: " << fieldName
                       << " is not in correct type, expect float";
    }
    return value.asFloat();
}

template <>
Result<int64_t> JsonValueParser::convertValueToType<int64_t>(const std::string& fieldName,
                                                             const Json::Value& value) {
    if (!value.isInt64()) {
        return Error() << "The value: " << value << " for field: " << fieldName
                       << " is not in correct type, expect int64";
    }
    return static_cast<int64_t>(value.asInt64());
}

template <>
Result<std::string> JsonValueParser::convertValueToType<std::string>(const std::string& fieldName,
                                                                     const Json::Value& value) {
    if (!value.isString()) {
        return Error() << "The value: " << value << " for field: " << fieldName
                       << " is not in correct type, expect string";
    }
    return value.asString();
}

Result<std::string> JsonValueParser::parseStringValue(const std::string& fieldName,
                                                      const Json::Value& value) const {
    return convertValueToType<std::string>(fieldName, value);
}

template <class T>
Result<T> JsonValueParser::parseValue(const std::string& fieldName,
                                      const Json::Value& value) const {
    if (!value.isString()) {
        return convertValueToType<T>(fieldName, value);
    }
    auto maybeTypeAndValue = maybeGetTypeAndValueName(value.asString());
    if (!maybeTypeAndValue.has_value()) {
        return Error() << "Invalid constant value: " << value << " for field: " << fieldName;
    }
    auto constantParseResult = parseConstantValue(maybeTypeAndValue.value());
    if (!constantParseResult.ok()) {
        return constantParseResult.error();
    }
    int constantValue = constantParseResult.value();
    return static_cast<T>(constantValue);
}

template <>
Result<std::string> JsonValueParser::parseValue<std::string>(const std::string& fieldName,
                                                             const Json::Value& value) const {
    return parseStringValue(fieldName, value);
}

template <class T>
Result<std::vector<T>> JsonValueParser::parseArray(const std::string& fieldName,
                                                   const Json::Value& value) const {
    if (!value.isArray()) {
        return Error() << "The value: " << value << " for field: " << fieldName
                       << " is not in correct type, expect array";
    }
    std::vector<T> parsedValues;
    for (unsigned int i = 0; i < value.size(); i++) {
        auto result = parseValue<T>(fieldName, value[i]);
        if (!result.ok()) {
            return result.error();
        }
        parsedValues.push_back(result.value());
    }
    return std::move(parsedValues);
}

std::optional<std::pair<std::string, std::string>> JsonValueParser::maybeGetTypeAndValueName(
        const std::string& jsonFieldValue) const {
    size_t pos = jsonFieldValue.find(DELIMITER);
    if (pos == std::string::npos) {
        return {};
    }
    std::string type = jsonFieldValue.substr(0, pos);
    std::string valueName = jsonFieldValue.substr(pos + DELIMITER.length(), std::string::npos);
    return std::make_pair(type, valueName);
}

Result<int> JsonValueParser::parseConstantValue(const std::pair<std::string, std::string>&) const {
    // TODO(b/238685398): Implement this.
}

template <class T>
bool JsonConfigParser::tryParseJsonValueToVariable(const Json::Value& parentJsonNode,
                                                   const std::string& fieldName,
                                                   bool fieldIsOptional, T* outPtr,
                                                   std::vector<std::string>* errors) {
    if (!parentJsonNode.isObject()) {
        errors->push_back("Node: " + parentJsonNode.toStyledString() + " is not an object");
        return false;
    }
    if (!parentJsonNode.isMember(fieldName)) {
        if (!fieldIsOptional) {
            errors->push_back("Missing required field: " + fieldName +
                              " in node: " + parentJsonNode.toStyledString());
            return false;
        }
        return true;
    }
    auto result = mValueParser.parseValue<T>(fieldName, parentJsonNode[fieldName]);
    if (!result.ok()) {
        errors->push_back(result.error().message());
        return false;
    }
    *outPtr = std::move(result.value());
    return true;
}

template <class T>
bool JsonConfigParser::tryParseJsonArrayToVariable(const Json::Value& parentJsonNode,
                                                   const std::string& fieldName,
                                                   bool fieldIsOptional, std::vector<T>* outPtr,
                                                   std::vector<std::string>* errors) {
    if (!parentJsonNode.isObject()) {
        errors->push_back("Node: " + parentJsonNode.toStyledString() + " is not an object");
        return false;
    }
    if (!parentJsonNode.isMember(fieldName)) {
        if (!fieldIsOptional) {
            errors->push_back("Missing required field: " + fieldName +
                              " in node: " + parentJsonNode.toStyledString());
            return false;
        }
        return true;
    }
    auto result = mValueParser.parseArray<T>(fieldName, parentJsonNode[fieldName]);
    if (!result.ok()) {
        errors->push_back(result.error().message());
        return false;
    }
    *outPtr = std::move(result.value());
    return true;
}

void JsonConfigParser::parsePropValues(const Json::Value& parentJsonNode,
                                       const std::string& fieldName, RawPropValues* outPtr,
                                       std::vector<std::string>* errors) {
    if (!parentJsonNode.isObject()) {
        errors->push_back("Node: " + parentJsonNode.toStyledString() + " is not an object");
        return;
    }
    if (!parentJsonNode.isMember(fieldName)) {
        return;
    }
    const Json::Value& jsonValue = parentJsonNode[fieldName];
    tryParseJsonArrayToVariable(jsonValue, "int32Values", /*optional=*/true, &(outPtr->int32Values),
                                errors);
    tryParseJsonArrayToVariable(jsonValue, "floatValues", /*optional=*/true, &(outPtr->floatValues),
                                errors);
    tryParseJsonArrayToVariable(jsonValue, "int64Values", /*optional=*/true, &(outPtr->int64Values),
                                errors);
    // We don't support "byteValues" yet.
    tryParseJsonValueToVariable(jsonValue, "stringValue", /*optional=*/true, &(outPtr->stringValue),
                                errors);
}

std::optional<ConfigDeclaration> JsonConfigParser::parseEachProperty(
        const Json::Value& propJsonValue, std::vector<std::string>* errors) {
    size_t initialErrorCount = errors->size();
    ConfigDeclaration configDecl = {};
    int32_t propId;

    if (!tryParseJsonValueToVariable(propJsonValue, "property", /*optional=*/false, &propId,
                                     errors)) {
        return std::nullopt;
    }

    configDecl.config.prop = propId;
    std::string propStr = propJsonValue["property"].toStyledString();

    // TODO(b/238685398): Parse access and changeMode.

    tryParseJsonValueToVariable(propJsonValue, "configString", /*optional=*/true,
                                &configDecl.config.configString, errors);

    tryParseJsonArrayToVariable(propJsonValue, "configArray", /*optional=*/true,
                                &configDecl.config.configArray, errors);

    parsePropValues(propJsonValue, "defaultValue", /*optional=*/true, &configDecl.initialValue,
                    errors);

    tryParseJsonValueToVariable(propJsonValue, "minSampleRate", /*optional=*/true,
                                &configDecl.config.minSampleRate, errors);

    tryParseJsonValueToVariable(propJsonValue, "maxSampleRate", /*optional=*/true,
                                &configDecl.config.maxSampleRate, errors);

    // TODO(b/238685398): AreaConfigs

    if (errors->size() != initialErrorCount) {
        return std::nullopt;
    }
    return configDecl;
}

Result<std::vector<ConfigDeclaration>> JsonConfigParser::parseJsonConfig(std::istream& is) {
    Json::CharReaderBuilder builder;
    Json::Value root;
    std::vector<ConfigDeclaration> configs;
    std::string errs;
    if (!Json::parseFromStream(builder, is, &root, &errs)) {
        return Error() << "Failed to parse property config file as JSON, error: " << errs;
    }
    Json::Value properties = root["properties"];
    std::vector<std::string> errors;
    for (unsigned int i = 0; i < properties.size(); i++) {
        if (auto maybeConfig = parseEachProperty(properties[i], &errors); maybeConfig.has_value()) {
            configs.push_back(std::move(maybeConfig.value()));
        }
    }
    if (!errors.empty()) {
        return Error() << android::base::Join(errors, '\n');
    }
    return configs;
}

}  // namespace jsonconfigloader_impl

JsonConfigLoader::JsonConfigLoader() {
    mParser = std::make_unique<jsonconfigloader_impl::JsonConfigParser>();
}

android::base::Result<std::vector<ConfigDeclaration>> JsonConfigLoader::loadPropConfig(
        std::istream& is) {
    return mParser->parseJsonConfig(is);
}

}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
