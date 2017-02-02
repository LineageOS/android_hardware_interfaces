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

#ifndef android_hardware_automotive_vehicle_V2_0_AccessControlConfigParser_H_
#define android_hardware_automotive_vehicle_V2_0_AccessControlConfigParser_H_

#include <string>
#include <vector>
#include <unordered_map>
#include <list>

#include <android/hardware/automotive/vehicle/2.0/types.h>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace V2_0 {

struct PropertyAcl {
    int32_t propId;
    unsigned uid;
    VehiclePropertyAccess access;
};

using PropertyAclMap = std::unordered_multimap<int32_t, PropertyAcl>;

/**
 * Parser for per-property access control in vehicle HAL.
 *
 * It supports the following format:
 *   Set ALIAS_NAME UID
 *   {S,V}:0x0305   {ALIAS_NAME,UID}   {R,W,RW}
 *
 * ALIAS_NAME is just an alias for UID
 * S - for system properties (VehiclePropertyGroup::SYSTEM)
 * V - for vendor properties (VehiclePropertyGroup::VENDOR)
 *
 * Example:
 *
 *   Set AID_AUDIO  1004
 *   Set AID_MY_APP     10022
 *
 *   S:0x0305   AID_AUDIO   RW
 *   S:0x0305   10021       R
 *   V:0x0101   AID_MY_APP  R
 */
class AccessControlConfigParser {
public:
    /**
     * Creates an instance of AccessControlConfigParser
     *
     * @param properties - properties supported by HAL implementation
     */
    AccessControlConfigParser(const std::vector<int32_t>& properties);

    /**
     * Parses config content from given stream and writes results to
     * propertyAclMap.
     */
    bool parseFromStream(std::istream* stream, PropertyAclMap* propertyAclMap);

private:
    bool processTokens(std::list<std::string>* tokens,
                       PropertyAclMap* propertyAclMap);

    bool parsePropertyGroup(char group,
                            VehiclePropertyGroup* outPropertyGroup) const;

    bool parsePropertyId(const std::string& strPropId,
                                VehiclePropertyGroup propertyGroup,
                                int32_t* outVehicleProperty) const;

    bool parseUid(const std::string& strUid, unsigned* outUid) const;

    bool parseAccess(const std::string& strAccess,
                     VehiclePropertyAccess* outAccess) const;


    std::string readNextToken(std::list<std::string>* tokens) const;

    static bool parseInt(const char* strValue, int* outIntValue);
    static void split(const std::string& line,
                      std::list<std::string>* outTokens);

private:
    std::unordered_map<std::string, unsigned> mUidMap {};  // Contains UID
    // aliases.

    // Map property ids w/o TYPE and AREA to VehicleProperty.
    std::unordered_map<int32_t, int32_t> mStrippedToVehiclePropertyMap;
};

}  // namespace V2_0
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android

#endif // android_hardware_automotive_vehicle_V2_0_AccessControlConfigParser_H_
