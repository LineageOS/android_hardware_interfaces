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

#ifndef android_hardware_automotive_vehicle_aidl_impl_default_config_JsonConfigLoader_include_JsonConfigLoader_H_
#define android_hardware_automotive_vehicle_aidl_impl_default_config_JsonConfigLoader_include_JsonConfigLoader_H_

#include <ConfigDeclaration.h>
#include <VehicleHalTypes.h>

#include <android-base/result.h>
#include <json/json.h>
#include <unordered_map>
#include <utility>
#include <vector>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {

// private namespace
namespace jsonconfigloader_impl {

// A class to parse a value field in JSON config file.
// If the field is a string and the field is in the format of "XX::XX", the value will be parsed
// as a constant value in the format of "TYPE::NAME". Otherwise, the field will be return as is
// converted to the expected type.
class JsonValueParser final {
  public:
    JsonValueParser();

    android::base::Result<std::string> parseStringValue(const std::string& fieldName,
                                                        const Json::Value& value) const;

    template <class T>
    android::base::Result<std::vector<T>> parseArray(const std::string& fieldName,
                                                     const Json::Value& value) const;

    template <class T>
    android::base::Result<T> parseValue(const std::string& fieldName,
                                        const Json::Value& value) const;

  private:
    template <class T>
    static android::base::Result<T> convertValueToType(const std::string& fieldName,
                                                       const Json::Value& value);

    std::optional<std::pair<std::string, std::string>> maybeGetTypeAndValueName(
            const std::string& jsonFieldValue) const;
};

// The main class to parse a VHAL config file in JSON format.
class JsonConfigParser {
  public:
    android::base::Result<std::vector<ConfigDeclaration>> parseJsonConfig(std::istream& is);

  private:
    JsonValueParser mValueParser;

    // Parses configuration for each property.
    std::optional<ConfigDeclaration> parseEachProperty(const Json::Value& propJsonValue,
                                                       std::vector<std::string>* errors);
    // Tries to parse a JSON value to a specific type.
    //
    // If fieldIsOptional is True, then if the field specified by "fieldName" does not exist,
    // this method will return true without doing anything, otherwise, it will return false.
    //
    // @param parentJsonNode The parent node of the field you are going to parse.
    // @param fieldName The name for the field.
    // @param fieldIsOptional Whether the field is optional.
    // @param outPtr The pointer to output to if the field exists and parsing succeeded.
    // @param errors The error array to append error to if errors are found.
    // @return true if the field is optional and does not exist or parsed successfully.
    template <class T>
    bool tryParseJsonValueToVariable(const Json::Value& parentJsonNode,
                                     const std::string& fieldName, bool fieldIsOptional, T* outPtr,
                                     std::vector<std::string>* errors);
    // Tries to parse a JSON value to an array of specific type.
    //
    // If fieldIsOptional is True, then if the field specified by "fieldName" does not exist,
    // this method will return true without doing anything, otherwise, it will return false.
    //
    // @param parentJsonNode The parent node of the field you are going to parse.
    // @param fieldName The name for the field.
    // @param fieldIsOptional Whether the field is optional.
    // @param outPtr The pointer to output to if the field exists and parsing succeeded.
    // @param errors The error array to append error to if errors are found.
    // @return true if the field is optional and does not exist or parsed successfully.
    template <class T>
    bool tryParseJsonArrayToVariable(const Json::Value& parentJsonNode,
                                     const std::string& fieldName, bool fieldIsOptional,
                                     std::vector<T>* outPtr, std::vector<std::string>* errors);

    // Parses a JSON field to RawPropValues.
    void parsePropValues(const Json::Value& parentJsonNode, const std::string& fieldName,
                         aidl::android::hardware::automotive::vehicle::RawPropValues* outPtr,
                         std::vector<std::string>* errors);
};

}  // namespace jsonconfigloader_impl

// A class to load vehicle property configs and initial values in JSON format.
class JsonConfigLoader final {
  public:
    JsonConfigLoader();

    // Loads a JSON file stream and parses it to a list of ConfigDeclarations.
    android::base::Result<std::vector<ConfigDeclaration>> loadPropConfig(std::istream& is);

  private:
    std::unique_ptr<jsonconfigloader_impl::JsonConfigParser> mParser;
};

}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android

#endif  // android_hardware_automotive_vehicle_aidl_impl_default_config_JsonConfigLoader_include_JsonConfigLoader_H_
