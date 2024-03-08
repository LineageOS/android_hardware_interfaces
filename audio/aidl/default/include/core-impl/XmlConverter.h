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

#pragma once

#include <optional>
#include <string>
#include <unordered_map>

#include <media/AidlConversionUtil.h>
#include <system/audio_config.h>

namespace aidl::android::hardware::audio::core::internal {

template <typename T>
class XmlConverter {
  public:
    XmlConverter(const std::string& configFilePath,
                 std::function<std::optional<T>(const char*)> readXmlConfig)
        : XmlConverter(configFilePath,
                       ::android::audio_is_readable_configuration_file(configFilePath.c_str()),
                       readXmlConfig) {}

    const ::android::status_t& getStatus() const { return mStatus; }

    const std::string& getError() const { return mErrorMessage; }

    const std::optional<T>& getXsdcConfig() const { return mXsdcConfig; }

  private:
    XmlConverter(const std::string& configFilePath, const bool& isReadableConfigFile,
                 const std::function<std::optional<T>(const char*)>& readXmlConfig)
        : mXsdcConfig{isReadableConfigFile ? readXmlConfig(configFilePath.c_str()) : std::nullopt},
          mStatus(mXsdcConfig ? ::android::OK : ::android::NO_INIT),
          mErrorMessage(generateError(configFilePath, isReadableConfigFile, mStatus)) {}

    static std::string generateError(const std::string& configFilePath,
                                     const bool& isReadableConfigFile,
                                     const ::android::status_t& status) {
        std::string errorMessage;
        if (status != ::android::OK) {
            if (configFilePath.empty()) {
                errorMessage = "No audio configuration files found";
            } else if (!isReadableConfigFile) {
                errorMessage = std::string("Could not read requested XML config file: \"")
                                       .append(configFilePath)
                                       .append("\"");
            } else {
                errorMessage = std::string("Invalid XML config file: \"")
                                       .append(configFilePath)
                                       .append("\"");
            }
        }
        return errorMessage;
    }

    const std::optional<T> mXsdcConfig;
    const ::android::status_t mStatus;
    const std::string mErrorMessage;
};

/**
 * Converts a vector of an xsd wrapper type to a flat vector of the
 * corresponding AIDL type.
 *
 * Wrapper types are used in order to have well-formed xIncludes. In the
 * example below, Modules is the wrapper type for Module.
 *     <Modules>
 *         <Module> ... </Module>
 *         <Module> ... </Module>
 *     </Modules>
 */
template <typename W, typename X, typename A>
static ConversionResult<std::vector<A>> convertWrappedCollectionToAidl(
        const std::vector<W>& xsdcWrapperTypeVec,
        std::function<const std::vector<X>&(const W&)> getInnerTypeVec,
        std::function<ConversionResult<A>(const X&)> convertToAidl) {
    std::vector<A> resultAidlTypeVec;
    if (!xsdcWrapperTypeVec.empty()) {
        /*
         * xsdcWrapperTypeVec likely only contains one element; that is, it's
         * likely that all the inner types that we need to convert are inside of
         * xsdcWrapperTypeVec[0].
         */
        resultAidlTypeVec.reserve(getInnerTypeVec(xsdcWrapperTypeVec[0]).size());
        for (const W& xsdcWrapperType : xsdcWrapperTypeVec) {
            for (const X& xsdcType : getInnerTypeVec(xsdcWrapperType)) {
                resultAidlTypeVec.push_back(VALUE_OR_FATAL(convertToAidl(xsdcType)));
            }
        }
    }
    return resultAidlTypeVec;
}

template <typename X, typename A>
static ConversionResult<std::vector<A>> convertCollectionToAidl(
        const std::vector<X>& xsdcTypeVec,
        std::function<ConversionResult<A>(const X&)> convertToAidl) {
    std::vector<A> resultAidlTypeVec;
    resultAidlTypeVec.reserve(xsdcTypeVec.size());
    for (const X& xsdcType : xsdcTypeVec) {
        resultAidlTypeVec.push_back(VALUE_OR_FATAL(convertToAidl(xsdcType)));
    }
    return resultAidlTypeVec;
}

/**
 * Generates a map of xsd references, keyed by reference name, given a
 * vector of wrapper types for the reference.
 *
 * Wrapper types are used in order to have well-formed xIncludes. In the
 * example below, Wrapper is the wrapper type for Reference.
 *     <Wrapper>
 *         <Reference> ... </Reference>
 *         <Reference> ... </Reference>
 *     </Wrapper>
 */
template <typename W, typename R>
std::unordered_map<std::string, R> generateReferenceMap(const std::vector<W>& xsdcWrapperTypeVec) {
    std::unordered_map<std::string, R> resultMap;
    if (!xsdcWrapperTypeVec.empty()) {
        /*
         * xsdcWrapperTypeVec likely only contains one element; that is, it's
         * likely that all the inner types that we need to convert are inside of
         * xsdcWrapperTypeVec[0].
         */
        resultMap.reserve(xsdcWrapperTypeVec[0].getReference().size());
        for (const W& xsdcWrapperType : xsdcWrapperTypeVec) {
            for (const R& xsdcReference : xsdcWrapperType.getReference()) {
                resultMap.insert({xsdcReference.getName(), xsdcReference});
            }
        }
    }
    return resultMap;
}
}  // namespace aidl::android::hardware::audio::core::internal
