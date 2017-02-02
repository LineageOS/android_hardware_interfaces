/*
 * Copyright (C) 2016 The Android Open Source Project
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

#define LOG_TAG "automotive.vehicle@2.0-impl"

#include "AccessControlConfigParser.h"
#include "VehicleUtils.h"

#include <fstream>
#include <iostream>
#include <sstream>

#include <log/log.h>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace V2_0 {

AccessControlConfigParser::AccessControlConfigParser(
        const std::vector<int32_t>& properties) {
    // Property Id in the config file doesn't include information about
    // type and area. So we want to create a map from these kind of
    // *stripped* properties to the whole VehicleProperty.
    // We also want to filter out ACL to the properties that supported
    // by concrete Vehicle HAL implementation.
    for (auto prop : properties) {
        auto strippedProp = prop
                            & ~toInt(VehiclePropertyType::MASK)
                            & ~toInt(VehicleArea::MASK);
        mStrippedToVehiclePropertyMap.emplace(strippedProp, prop);
    }
}

bool AccessControlConfigParser::parseFromStream(
        std::istream* stream, PropertyAclMap* propertyAclMap) {
    std::list<std::string> tokens;
    std::string line;
    int lineNo = 0;
    bool warnings = false;
    for (;std::getline(*stream, line); lineNo++) {
        split(line, &tokens);
        if (!processTokens(&tokens, propertyAclMap)) {
            warnings = true;
            ALOGW("Failed to parse line %d : %s", lineNo, line.c_str());
        }
    }
    return !warnings;
}


bool AccessControlConfigParser::processTokens(std::list<std::string>* tokens,
                                              PropertyAclMap* propertyAclMap) {
    std::string token = readNextToken(tokens);
    if (token.empty() || token[0] == '#') {   // Ignore comment.
        return true;
    }

    if (token == "Set") {
        std::string alias = readNextToken(tokens);
        std::string strUid = readNextToken(tokens);
        if (alias.empty() || strUid.empty()) {
            ALOGW("Expected alias and UID must be specified");
            return false;
        }
        int uid;
        if (!parseInt(strUid.c_str(), &uid)) {
            ALOGW("Invalid UID: %d", uid);
        }
        mUidMap.emplace(std::move(alias), uid);
    } else if (token.size() > 2 && token[1] == ':') {
        VehiclePropertyGroup propGroup;
        if (!parsePropertyGroup(token[0], &propGroup)) {
            return false;
        }
        std::string strUid = readNextToken(tokens);
        std::string strAccess = readNextToken(tokens);
        if (strUid.empty() || strAccess.empty()) {
            ALOGW("Expected UID and access for property: %s",
                  token.c_str());
        }


        PropertyAcl acl;
        if (parsePropertyId(token.substr(2), propGroup, &acl.propId)
            && parseUid(strUid, &acl.uid)
            && parseAccess(strAccess, &acl.access)) {
            propertyAclMap->emplace(acl.propId, std::move(acl));
        } else {
            return false;
        }
    } else {
        ALOGW("Unexpected token: %s", token.c_str());
        return false;
    }

    return true;
}

bool AccessControlConfigParser::parsePropertyGroup(
        char group, VehiclePropertyGroup* outPropertyGroup) const {
    switch (group) {
        case 'S':  // Fall through.
        case 's':
            *outPropertyGroup = VehiclePropertyGroup::SYSTEM;
            break;
        case 'V':  // Fall through.
        case 'v':
            *outPropertyGroup = VehiclePropertyGroup::VENDOR;
            break;
        default:
            ALOGW("Unexpected group: %c", group);
            return false;
    }
    return true;
}

bool AccessControlConfigParser::parsePropertyId(
        const std::string& strPropId,
        VehiclePropertyGroup propertyGroup,
        int32_t* outVehicleProperty) const {
    int32_t propId;
    if (!parseInt(strPropId.c_str(), &propId)) {
        ALOGW("Failed to convert property id to integer: %s",
              strPropId.c_str());
        return false;
    }
    propId |= static_cast<int>(propertyGroup);
    auto it = mStrippedToVehiclePropertyMap.find(propId);
    if (it == mStrippedToVehiclePropertyMap.end()) {
        ALOGW("Property Id not found or not supported: 0x%x", propId);
        return false;
    }
    *outVehicleProperty = it->second;
    return true;
}

bool AccessControlConfigParser::parseInt(const char* strValue,
                                         int* outIntValue) {
    char* end;
    long num = std::strtol(strValue, &end, 0 /* auto detect base */);
    bool success = *end == 0 && errno != ERANGE;
    if (success) {
        *outIntValue = static_cast<int>(num);
    }

    return success;
}

bool AccessControlConfigParser::parseUid(const std::string& strUid,
                                         unsigned* outUid) const {
    auto element = mUidMap.find(strUid);
    if (element != mUidMap.end()) {
        *outUid = element->second;
    } else {
        int val;
        if (!parseInt(strUid.c_str(), &val)) {
            ALOGW("Failed to convert UID '%s' to integer", strUid.c_str());
            return false;
        }
        *outUid = static_cast<unsigned>(val);
    }
    return true;
}

bool AccessControlConfigParser::parseAccess(
        const std::string& strAccess, VehiclePropertyAccess* outAccess) const {
    if (strAccess.size() == 0 || strAccess.size() > 2) {
        ALOGW("Unknown access mode '%s'", strAccess.c_str());
        return false;
    }
    int32_t access = static_cast<int32_t>(VehiclePropertyAccess::NONE);
    for (char c : strAccess) {
        if (c == 'R' || c == 'r') {
            access |= VehiclePropertyAccess::READ;
        } else if (c == 'W' || c == 'w') {
            access |= VehiclePropertyAccess::WRITE;
        } else {
            ALOGW("Unknown access mode: %c", c);
            return false;
        }
    }
    *outAccess = static_cast<VehiclePropertyAccess>(access);
    return true;
}

void AccessControlConfigParser::split(const std::string& line,
                                      std::list<std::string>* outTokens) {
    outTokens->clear();
    std::istringstream iss(line);

    while (!iss.eof()) {
        std::string token;
        iss >> token;
        outTokens->push_back(std::move(token));
    }
}

std::string AccessControlConfigParser::readNextToken(
        std::list<std::string>* tokens) const {
    if (tokens->empty()) {
        return "";
    }

    std::string token = tokens->front();
    tokens->pop_front();
    return token;
}

}  // namespace V2_0
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
