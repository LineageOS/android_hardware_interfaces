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

#define LOG_TAG "AHAL_EffectConfig"
#include <android-base/logging.h>

#include "effectFactory-impl/EffectConfig.h"

using aidl::android::media::audio::common::AudioUuid;

namespace aidl::android::hardware::audio::effect {

EffectConfig::EffectConfig(const std::string& file) {
    tinyxml2::XMLDocument doc;
    doc.LoadFile(file.c_str());
    LOG(DEBUG) << __func__ << " loading " << file;
    // parse the xml file into maps
    if (doc.Error()) {
        LOG(ERROR) << __func__ << " tinyxml2 failed to load " << file
                   << " error: " << doc.ErrorStr();
        return;
    }

    auto registerFailure = [&](bool result) { mSkippedElements += result ? 0 : 1; };

    for (auto& xmlConfig : getChildren(doc, "audio_effects_conf")) {
        // Parse library
        for (auto& xmlLibraries : getChildren(xmlConfig, "libraries")) {
            for (auto& xmlLibrary : getChildren(xmlLibraries, "library")) {
                registerFailure(parseLibrary(xmlLibrary));
            }
        }

        // Parse effects
        for (auto& xmlEffects : getChildren(xmlConfig, "effects")) {
            for (auto& xmlEffect : getChildren(xmlEffects)) {
                registerFailure(parseEffect(xmlEffect));
            }
        }

        // Parse pre processing chains
        for (auto& xmlPreprocess : getChildren(xmlConfig, "preprocess")) {
            for (auto& xmlStream : getChildren(xmlPreprocess, "stream")) {
                registerFailure(parseStream(xmlStream));
            }
        }

        // Parse post processing chains
        for (auto& xmlPostprocess : getChildren(xmlConfig, "postprocess")) {
            for (auto& xmlStream : getChildren(xmlPostprocess, "stream")) {
                registerFailure(parseStream(xmlStream));
            }
        }
    }
    LOG(DEBUG) << __func__ << " successfully parsed " << file << ", skipping " << mSkippedElements
               << " element(s)";
}

std::vector<std::reference_wrapper<const tinyxml2::XMLElement>> EffectConfig::getChildren(
        const tinyxml2::XMLNode& node, const char* childTag) {
    std::vector<std::reference_wrapper<const tinyxml2::XMLElement>> children;
    for (auto* child = node.FirstChildElement(childTag); child != nullptr;
         child = child->NextSiblingElement(childTag)) {
        children.emplace_back(*child);
    }
    return children;
}

bool EffectConfig::resolveLibrary(const std::string& path, std::string* resolvedPath) {
    for (auto* libraryDirectory : kEffectLibPath) {
        std::string candidatePath = std::string(libraryDirectory) + '/' + path;
        if (access(candidatePath.c_str(), R_OK) == 0) {
            *resolvedPath = std::move(candidatePath);
            return true;
        }
    }
    return false;
}

bool EffectConfig::parseLibrary(const tinyxml2::XMLElement& xml) {
    const char* name = xml.Attribute("name");
    RETURN_VALUE_IF(!name, false, "noNameAttribute");
    const char* path = xml.Attribute("path");
    RETURN_VALUE_IF(!path, false, "noPathAttribute");

    std::string resolvedPath;
    if (!resolveLibrary(path, &resolvedPath)) {
        LOG(ERROR) << __func__ << " can't find " << path;
        return false;
    }
    mLibraryMap[name] = resolvedPath;
    LOG(DEBUG) << __func__ << " " << name << " : " << resolvedPath;
    return true;
}

bool EffectConfig::parseEffect(const tinyxml2::XMLElement& xml) {
    struct EffectLibraries effectLibraries;
    std::vector<LibraryUuid> libraryUuids;
    std::string name = xml.Attribute("name");
    RETURN_VALUE_IF(name == "", false, "effectsNoName");

    LOG(DEBUG) << __func__ << dump(xml);
    struct LibraryUuid libraryUuid;
    if (std::strcmp(xml.Name(), "effectProxy") == 0) {
        // proxy lib and uuid
        RETURN_VALUE_IF(!parseLibraryUuid(xml, libraryUuid, true), false, "parseProxyLibFailed");
        effectLibraries.proxyLibrary = libraryUuid;
        // proxy effect libs and UUID
        auto xmlProxyLib = xml.FirstChildElement();
        RETURN_VALUE_IF(!xmlProxyLib, false, "noLibForProxy");
        while (xmlProxyLib) {
            struct LibraryUuid tempLibraryUuid;
            RETURN_VALUE_IF(!parseLibraryUuid(*xmlProxyLib, tempLibraryUuid), false,
                            "parseEffectLibFailed");
            libraryUuids.push_back(std::move(tempLibraryUuid));
            xmlProxyLib = xmlProxyLib->NextSiblingElement();
        }
    } else {
        // expect only one library if not proxy
        RETURN_VALUE_IF(!parseLibraryUuid(xml, libraryUuid), false, "parseEffectLibFailed");
        libraryUuids.push_back(std::move(libraryUuid));
    }

    effectLibraries.libraries = std::move(libraryUuids);
    mEffectsMap[name] = std::move(effectLibraries);
    return true;
}

bool EffectConfig::parseStream(const tinyxml2::XMLElement& xml) {
    LOG(DEBUG) << __func__ << dump(xml);
    const char* type = xml.Attribute("type");
    RETURN_VALUE_IF(!type, false, "noTypeInProcess");
    RETURN_VALUE_IF(0 != mProcessingMap.count(type), false, "duplicateType");

    for (auto& apply : getChildren(xml, "apply")) {
        const char* name = apply.get().Attribute("effect");
        RETURN_VALUE_IF(!name, false, "noEffectAttribute");
        mProcessingMap[type].push_back(name);
        LOG(DEBUG) << __func__ << " " << type << " : " << name;
    }
    return true;
}

bool EffectConfig::parseLibraryUuid(const tinyxml2::XMLElement& xml,
                                    struct LibraryUuid& libraryUuid, bool isProxy) {
    // Retrieve library name only if not effectProxy element
    if (!isProxy) {
        const char* name = xml.Attribute("library");
        RETURN_VALUE_IF(!name, false, "noLibraryAttribute");
        libraryUuid.name = name;
    }

    const char* uuid = xml.Attribute("uuid");
    RETURN_VALUE_IF(!uuid, false, "noUuidAttribute");
    RETURN_VALUE_IF(!stringToUuid(uuid, &libraryUuid.uuid), false, "invalidUuidAttribute");

    LOG(DEBUG) << __func__ << (isProxy ? " proxy " : libraryUuid.name) << " : "
               << libraryUuid.uuid.toString();
    return true;
}

const char* EffectConfig::dump(const tinyxml2::XMLElement& element,
                               tinyxml2::XMLPrinter&& printer) const {
    element.Accept(&printer);
    return printer.CStr();
}

}  // namespace aidl::android::hardware::audio::effect
