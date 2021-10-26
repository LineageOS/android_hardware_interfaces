/*
 * Copyright (C) 2020 The Android Open Source Project
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
#define LOG_TAG "UserHalHelper"

#include "UserHalHelper.h"

#include <log/log.h>
#include <utils/SystemClock.h>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace V2_0 {

namespace user_hal_helper {
namespace {

using android::base::Error;
using android::base::Result;

static constexpr const char* kSeparator = "||";
static const size_t kNumFieldsPerUserInfo = 2;
static const size_t kNumFieldsPerSetAssociation = 2;

Result<void> verifyPropValue(const VehiclePropValue& propValue, VehicleProperty vehicleProperty,
                             size_t minInt32Values) {
    auto prop = verifyAndCast<VehicleProperty>(propValue.prop);
    if (!prop.ok()) {
        return Error() << "Invalid vehicle property: " << prop.error();
    }
    if (*prop != vehicleProperty) {
        return Error() << "Mismatching " << toString(vehicleProperty) << " request, received "
                       << toString(*prop) << " property";
    }
    if (propValue.value.int32Values.size() < minInt32Values) {
        return Error() << "Int32Values must have at least " << minInt32Values
                       << " values, received " << propValue.value.int32Values.size();
    }
    return {};
}

Result<void> parseUserInfo(const hidl_vec<int32_t>& int32Values, size_t startPos,
                           UserInfo* userInfo) {
    if (int32Values.size() < startPos + kNumFieldsPerUserInfo) {
        return Error() << "Int32Values must have at least " << startPos + 2 << " values, received "
                       << int32Values.size();
    }
    userInfo->userId = int32Values[startPos];
    int32_t intUserFlags = int32Values[startPos + 1];
    int32_t expectedUserFlags = 0;
    for (const auto& v : hidl_enum_range<UserFlags>()) {
        int32_t intEnumUserFlag = static_cast<int32_t>(v);
        if ((intUserFlags & intEnumUserFlag) != 0) {
            expectedUserFlags |= intEnumUserFlag;
        }
    }
    if (intUserFlags != expectedUserFlags) {
        return Error() << "Invalid user flags: " << intUserFlags << ", must be '|' of UserFlags";
    }
    // intUserFlags is actually not a valid UserFlags enum, instead, it is a 'bit or' of possible
    // multiple UserFlags. However, because the HAL interface was defined incorrectly, we have to
    // cast it to UserFlags here, which is defined behavior because the underlying type for
    // UserFlags is int32_t and our intUserFlags is within the range of int32_t.
    userInfo->flags = static_cast<UserFlags>(intUserFlags);
    return {};
}

Result<void> parseUsersInfo(const hidl_vec<int32_t>& int32Values, size_t startPos,
                            UsersInfo* usersInfo) {
    if (int32Values.size() < startPos + 3) {
        return Error() << "Int32Values must have at least " << startPos + 3 << " values, received "
                       << int32Values.size();
    }
    auto ret = parseUserInfo(int32Values, startPos, &usersInfo->currentUser);
    if (!ret.ok()) {
        return ret;
    }
    usersInfo->numberUsers = int32Values[startPos + 2];
    usersInfo->existingUsers.resize(usersInfo->numberUsers);
    for (size_t i = 0; i < static_cast<size_t>(usersInfo->numberUsers); ++i) {
        ret = parseUserInfo(int32Values, startPos + 3 + (kNumFieldsPerUserInfo * i),
                            &usersInfo->existingUsers[i]);
        if (!ret.ok()) {
            return Error() << "Failed to parse existing user '" << i << "' info: " << ret.error();
        }
    }
    return {};
}

Result<void> parseUserAssociationTypes(
        const hidl_vec<int32_t>& int32Values, size_t startPos, size_t numberAssociationTypes,
        hidl_vec<UserIdentificationAssociationType>* associationTypes) {
    size_t minInt32Values = startPos + numberAssociationTypes;
    if (int32Values.size() < minInt32Values) {
        return Error() << "Int32Values must have at least " << minInt32Values
                       << " values, received " << int32Values.size();
    }
    associationTypes->resize(numberAssociationTypes);
    for (size_t i = 0; i < static_cast<size_t>(numberAssociationTypes); ++i) {
        size_t pos = startPos + i;
        auto type = verifyAndCast<UserIdentificationAssociationType>(int32Values[pos]);
        if (!type.ok()) {
            return Error() << "Invalid association type in query '" << i << "': " << type.error();
        }
        (*associationTypes)[i] = *type;
    }
    return {};
}

Result<void> parseUserAssociations(const hidl_vec<int32_t>& int32Values, size_t startPos,
                                   size_t numberAssociations,
                                   hidl_vec<UserIdentificationSetAssociation>* associations) {
    size_t minInt32Values = startPos + (numberAssociations * kNumFieldsPerSetAssociation);
    if (int32Values.size() < minInt32Values) {
        return Error() << "Int32Values must have at least " << minInt32Values
                       << " values, received " << int32Values.size();
    }
    associations->resize(numberAssociations);
    for (size_t i = 0; i < static_cast<size_t>(numberAssociations); ++i) {
        size_t pos = startPos + (kNumFieldsPerSetAssociation * i);
        auto type = verifyAndCast<UserIdentificationAssociationType>(int32Values[pos]);
        if (!type.ok()) {
            return Error() << "Invalid association type in request '" << i << "': " << type.error();
        }
        (*associations)[i].type = *type;
        auto value = verifyAndCast<UserIdentificationAssociationSetValue>(int32Values[pos + 1]);
        if (!value.ok()) {
            return Error() << "Invalid association set value in request '" << i
                           << "': " << value.error();
        }
        (*associations)[i].value = *value;
    }
    return {};
}

}  // namespace

template <typename T>
Result<T> verifyAndCast(int32_t value) {
    T castValue = static_cast<T>(value);
    for (const auto& v : hidl_enum_range<T>()) {
        if (castValue == v) {
            return castValue;
        }
    }
    return Error() << "Value " << value << " not in enum values";
}

Result<InitialUserInfoRequest> toInitialUserInfoRequest(const VehiclePropValue& propValue) {
    auto ret = verifyPropValue(propValue, VehicleProperty::INITIAL_USER_INFO, 2);
    if (!ret.ok()) {
        return ret.error();
    }
    InitialUserInfoRequest request;
    request.requestId = propValue.value.int32Values[0];
    auto requestType = verifyAndCast<InitialUserInfoRequestType>(propValue.value.int32Values[1]);
    if (!requestType.ok()) {
        return Error() << "Invalid InitialUserInfoRequestType: " << requestType.error();
    }
    request.requestType = *requestType;
    ret = parseUsersInfo(propValue.value.int32Values, 2, &request.usersInfo);
    if (!ret.ok()) {
        return Error() << "Failed to parse users info: " << ret.error();
    }
    return request;
}

Result<SwitchUserRequest> toSwitchUserRequest(const VehiclePropValue& propValue) {
    auto ret = verifyPropValue(propValue, VehicleProperty::SWITCH_USER, 2);
    if (!ret.ok()) {
        return ret.error();
    }
    SwitchUserRequest request;
    auto messageType = verifyAndCast<SwitchUserMessageType>(propValue.value.int32Values[1]);
    if (!messageType.ok()) {
        return Error() << "Invalid SwitchUserMessageType: " << messageType.error();
    }
    if (*messageType != SwitchUserMessageType::LEGACY_ANDROID_SWITCH &&
        *messageType != SwitchUserMessageType::ANDROID_SWITCH &&
        *messageType != SwitchUserMessageType::ANDROID_POST_SWITCH) {
        return Error() << "Invalid " << toString(*messageType)
                       << " message type from Android System";
    }
    request.requestId = propValue.value.int32Values[0];
    request.messageType = *messageType;
    ret = parseUserInfo(propValue.value.int32Values, 2, &request.targetUser);
    if (!ret.ok()) {
        return Error() << "Failed to parse target user info: " << ret.error();
    }
    ret = parseUsersInfo(propValue.value.int32Values, 4, &request.usersInfo);
    if (!ret.ok()) {
        return Error() << "Failed to parse users info: " << ret.error();
    }
    return request;
}

Result<CreateUserRequest> toCreateUserRequest(const VehiclePropValue& propValue) {
    auto ret = verifyPropValue(propValue, VehicleProperty::CREATE_USER, 1);
    if (!ret.ok()) {
        return ret.error();
    }
    CreateUserRequest request;
    request.requestId = propValue.value.int32Values[0];
    ret = parseUserInfo(propValue.value.int32Values, 1, &request.newUserInfo);
    if (!ret.ok()) {
        return Error() << "Failed to parse new user info: " << ret.error();
    }
    request.newUserName = propValue.value.stringValue;
    ret = parseUsersInfo(propValue.value.int32Values, 3, &request.usersInfo);
    if (!ret.ok()) {
        return Error() << "Failed to parse users info: " << ret.error();
    }
    return request;
}

Result<RemoveUserRequest> toRemoveUserRequest(const VehiclePropValue& propValue) {
    auto ret = verifyPropValue(propValue, VehicleProperty::REMOVE_USER, 1);
    if (!ret.ok()) {
        return ret.error();
    }
    RemoveUserRequest request;
    request.requestId = propValue.value.int32Values[0];
    ret = parseUserInfo(propValue.value.int32Values, 1, &request.removedUserInfo);
    if (!ret.ok()) {
        return Error() << "Failed to parse removed user info: " << ret.error();
    }
    ret = parseUsersInfo(propValue.value.int32Values, 3, &request.usersInfo);
    if (!ret.ok()) {
        return Error() << "Failed to parse users info: " << ret.error();
    }
    return request;
}

Result<UserIdentificationGetRequest> toUserIdentificationGetRequest(
        const VehiclePropValue& propValue) {
    auto ret = verifyPropValue(propValue, VehicleProperty::USER_IDENTIFICATION_ASSOCIATION, 4);
    if (!ret.ok()) {
        return ret.error();
    }
    UserIdentificationGetRequest request;
    request.requestId = propValue.value.int32Values[0];
    ret = parseUserInfo(propValue.value.int32Values, 1, &request.userInfo);
    if (!ret.ok()) {
        return Error() << "Failed to parse user info: " << ret.error();
    }
    request.numberAssociationTypes = propValue.value.int32Values[3];
    ret = parseUserAssociationTypes(propValue.value.int32Values, 4, request.numberAssociationTypes,
                                    &request.associationTypes);
    if (!ret.ok()) {
        return Error() << "Failed to parse UserIdentificationAssociationType: " << ret.error();
    }
    return request;
}

Result<UserIdentificationSetRequest> toUserIdentificationSetRequest(
        const VehiclePropValue& propValue) {
    auto ret = verifyPropValue(propValue, VehicleProperty::USER_IDENTIFICATION_ASSOCIATION, 4);
    if (!ret.ok()) {
        return ret.error();
    }
    UserIdentificationSetRequest request;
    request.requestId = propValue.value.int32Values[0];
    ret = parseUserInfo(propValue.value.int32Values, 1, &request.userInfo);
    if (!ret.ok()) {
        return Error() << "Failed to parse user info: " << ret.error();
    }
    request.numberAssociations = propValue.value.int32Values[3];
    ret = parseUserAssociations(propValue.value.int32Values, 4, request.numberAssociations,
                                &request.associations);
    if (!ret.ok()) {
        return Error() << "Failed to parse UserIdentificationSetAssociation: " << ret.error();
    }
    return request;
}

std::unique_ptr<VehiclePropValue> toVehiclePropValue(const SwitchUserRequest& request) {
    if (request.messageType != SwitchUserMessageType::VEHICLE_REQUEST) {
        ALOGE("Invalid %s message type %s from HAL", toString(VehicleProperty::SWITCH_USER).c_str(),
              toString(request.messageType).c_str());
        return nullptr;
    }
    auto propValue = std::unique_ptr<VehiclePropValue>(new VehiclePropValue());
    propValue->prop = static_cast<int32_t>(VehicleProperty::SWITCH_USER);
    propValue->timestamp = elapsedRealtimeNano();
    propValue->value.int32Values.resize(3);
    propValue->value.int32Values[0] = static_cast<int32_t>(request.requestId);
    propValue->value.int32Values[1] = static_cast<int32_t>(request.messageType);
    propValue->value.int32Values[2] = static_cast<int32_t>(request.targetUser.userId);
    return propValue;
}

std::unique_ptr<VehiclePropValue> toVehiclePropValue(const InitialUserInfoResponse& response) {
    auto propValue = std::unique_ptr<VehiclePropValue>(new VehiclePropValue());
    propValue->prop = static_cast<int32_t>(VehicleProperty::INITIAL_USER_INFO);
    propValue->timestamp = elapsedRealtimeNano();
    propValue->value.int32Values.resize(4);
    propValue->value.int32Values[0] = static_cast<int32_t>(response.requestId);
    propValue->value.int32Values[1] = static_cast<int32_t>(response.action);
    propValue->value.int32Values[2] = static_cast<int32_t>(response.userToSwitchOrCreate.userId);
    propValue->value.int32Values[3] = static_cast<int32_t>(response.userToSwitchOrCreate.flags);
    propValue->value.stringValue = std::string(response.userLocales) + std::string(kSeparator) +
                                   std::string(response.userNameToCreate);
    return propValue;
}

std::unique_ptr<VehiclePropValue> toVehiclePropValue(const SwitchUserResponse& response) {
    auto propValue = std::unique_ptr<VehiclePropValue>(new VehiclePropValue());
    propValue->prop = static_cast<int32_t>(VehicleProperty::SWITCH_USER);
    propValue->timestamp = elapsedRealtimeNano();
    propValue->value.int32Values.resize(3);
    propValue->value.int32Values[0] = static_cast<int32_t>(response.requestId);
    propValue->value.int32Values[1] = static_cast<int32_t>(response.messageType);
    propValue->value.int32Values[2] = static_cast<int32_t>(response.status);
    if (response.status == SwitchUserStatus::FAILURE) {
        propValue->value.stringValue = response.errorMessage;
    }
    return propValue;
}

std::unique_ptr<VehiclePropValue> toVehiclePropValue(const CreateUserResponse& response) {
    auto propValue = std::unique_ptr<VehiclePropValue>(new VehiclePropValue());
    propValue->prop = static_cast<int32_t>(VehicleProperty::CREATE_USER);
    propValue->timestamp = elapsedRealtimeNano();
    propValue->value.int32Values.resize(2);
    propValue->value.int32Values[0] = static_cast<int32_t>(response.requestId);
    propValue->value.int32Values[1] = static_cast<int32_t>(response.status);
    if (response.status == CreateUserStatus::FAILURE) {
        propValue->value.stringValue = response.errorMessage;
    }
    return propValue;
}

std::unique_ptr<VehiclePropValue> toVehiclePropValue(const UserIdentificationResponse& response) {
    auto propValue = std::unique_ptr<VehiclePropValue>(new VehiclePropValue());
    propValue->prop = static_cast<int32_t>(VehicleProperty::USER_IDENTIFICATION_ASSOCIATION);
    propValue->timestamp = elapsedRealtimeNano();
    propValue->value.int32Values.resize(2 + (response.numberAssociation * 2));
    propValue->value.int32Values[0] = static_cast<int32_t>(response.requestId);
    propValue->value.int32Values[1] = static_cast<int32_t>(response.numberAssociation);
    for (size_t i = 0; i < static_cast<size_t>(response.numberAssociation); ++i) {
        size_t int32ValuesPos = 2 + (2 * i);
        propValue->value.int32Values[int32ValuesPos] =
                static_cast<int32_t>(response.associations[i].type);
        propValue->value.int32Values[int32ValuesPos + 1] =
                static_cast<int32_t>(response.associations[i].value);
    }
    if (!response.errorMessage.empty()) {
        propValue->value.stringValue = response.errorMessage;
    }
    return propValue;
}

}  // namespace user_hal_helper

}  // namespace V2_0
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
