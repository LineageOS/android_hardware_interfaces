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

#include "UserHalHelper.h"

#include <VehicleUtils.h>
#include <aidl/android/hardware/automotive/vehicle/InitialUserInfoResponseAction.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace fake {
namespace user_hal_helper {

namespace {

using ::testing::Eq;
using ::testing::Gt;
using ::testing::IsNull;
using ::testing::NotNull;
using ::testing::Pointee;

using ::aidl::android::hardware::automotive::vehicle::CreateUserRequest;
using ::aidl::android::hardware::automotive::vehicle::CreateUserResponse;
using ::aidl::android::hardware::automotive::vehicle::CreateUserStatus;
using ::aidl::android::hardware::automotive::vehicle::InitialUserInfoRequest;
using ::aidl::android::hardware::automotive::vehicle::InitialUserInfoRequestType;
using ::aidl::android::hardware::automotive::vehicle::InitialUserInfoResponse;
using ::aidl::android::hardware::automotive::vehicle::InitialUserInfoResponseAction;
using ::aidl::android::hardware::automotive::vehicle::RemoveUserRequest;
using ::aidl::android::hardware::automotive::vehicle::SwitchUserMessageType;
using ::aidl::android::hardware::automotive::vehicle::SwitchUserRequest;
using ::aidl::android::hardware::automotive::vehicle::SwitchUserResponse;
using ::aidl::android::hardware::automotive::vehicle::SwitchUserStatus;
using ::aidl::android::hardware::automotive::vehicle::UserIdentificationAssociationSetValue;
using ::aidl::android::hardware::automotive::vehicle::UserIdentificationAssociationType;
using ::aidl::android::hardware::automotive::vehicle::UserIdentificationAssociationValue;
using ::aidl::android::hardware::automotive::vehicle::UserIdentificationGetRequest;
using ::aidl::android::hardware::automotive::vehicle::UserIdentificationResponse;
using ::aidl::android::hardware::automotive::vehicle::UserIdentificationSetRequest;
using ::aidl::android::hardware::automotive::vehicle::UserInfo;
using ::aidl::android::hardware::automotive::vehicle::VehicleProperty;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropValue;

constexpr int32_t INITIAL_USER_INFO = toInt(VehicleProperty::INITIAL_USER_INFO);
constexpr int32_t SWITCH_USER = toInt(VehicleProperty::SWITCH_USER);
constexpr int32_t CREATE_USER = toInt(VehicleProperty::CREATE_USER);
constexpr int32_t REMOVE_USER = toInt(VehicleProperty::REMOVE_USER);
constexpr int32_t USER_IDENTIFICATION_ASSOCIATION =
        toInt(VehicleProperty::USER_IDENTIFICATION_ASSOCIATION);

constexpr int32_t FIRST_BOOT_AFTER_OTA = toInt(InitialUserInfoRequestType::FIRST_BOOT_AFTER_OTA);
constexpr int32_t LEGACY_ANDROID_SWITCH = toInt(SwitchUserMessageType::LEGACY_ANDROID_SWITCH);
constexpr int32_t VEHICLE_REQUEST = toInt(SwitchUserMessageType::VEHICLE_REQUEST);

constexpr int32_t NONE_USER = 0;
constexpr int32_t GUEST_USER = toInt(UserInfo::USER_FLAG_GUEST);
constexpr int32_t SYSTEM_USER = toInt(UserInfo::USER_FLAG_SYSTEM);
constexpr int32_t SYSTEM_ADMIN_USER = SYSTEM_USER | toInt(UserInfo::USER_FLAG_ADMIN);
// 0x1111 is not a valid UserFlags combination.
constexpr int32_t INVALID_USER_FLAG = 0x1111;

constexpr int32_t USER_ID_ASSOC_KEY_FOB = toInt(UserIdentificationAssociationType::KEY_FOB);
constexpr int32_t USER_ID_ASSOC_CUSTOM_1 = toInt(UserIdentificationAssociationType::CUSTOM_1);

constexpr int32_t USER_ID_ASSOC_SET_CURRENT_USER =
        toInt(UserIdentificationAssociationSetValue::ASSOCIATE_CURRENT_USER);
constexpr int32_t USER_ID_ASSOC_UNSET_CURRENT_USER =
        toInt(UserIdentificationAssociationSetValue::DISASSOCIATE_CURRENT_USER);

constexpr int32_t USER_ID_ASSOC_CURRENT_USER =
        toInt(UserIdentificationAssociationValue::ASSOCIATED_CURRENT_USER);
constexpr int32_t USER_ID_ASSOC_NO_USER =
        toInt(UserIdentificationAssociationValue::NOT_ASSOCIATED_ANY_USER);

}  // namespace

TEST(UserHalHelperTest, TestToInitialUserInfoRequest) {
    VehiclePropValue propValue{
            .prop = INITIAL_USER_INFO,
            .value = {.int32Values = {23, FIRST_BOOT_AFTER_OTA, 10, NONE_USER, 2, 0, SYSTEM_USER,
                                      10, NONE_USER}},
    };
    InitialUserInfoRequest expected{
            .requestId = 23,
            .requestType = InitialUserInfoRequestType::FIRST_BOOT_AFTER_OTA,
            .usersInfo = {{10, 0}, 2, {{0, SYSTEM_USER}, {10, 0}}},
    };

    auto actual = toInitialUserInfoRequest(propValue);

    ASSERT_TRUE(actual.ok()) << actual.error().message();
    EXPECT_THAT(actual.value(), Eq(expected));
}

TEST(UserHalHelperTest, TestToInitialUserInfoRequestUserFlagsBitCombination) {
    // SYSTEM_ADMIN_USER is two UserFlags combined.
    VehiclePropValue propValue{
            .prop = INITIAL_USER_INFO,
            .value = {.int32Values = {23, FIRST_BOOT_AFTER_OTA, 10, NONE_USER, 2, 0,
                                      SYSTEM_ADMIN_USER, 10, NONE_USER}},
    };
    InitialUserInfoRequest expected{
            .requestId = 23,
            .requestType = InitialUserInfoRequestType::FIRST_BOOT_AFTER_OTA,
            .usersInfo = {{10, 0}, 2, {{0, SYSTEM_ADMIN_USER}, {10, 0}}},
    };

    auto actual = toInitialUserInfoRequest(propValue);

    ASSERT_TRUE(actual.ok()) << actual.error().message();
    EXPECT_THAT(actual.value(), Eq(expected));
}

TEST(UserHalHelperTest, TestToInitialUserInfoRequestUserInvalidUserFlag) {
    VehiclePropValue propValue{
            .prop = INITIAL_USER_INFO,
            .value = {.int32Values = {23, FIRST_BOOT_AFTER_OTA, 10, NONE_USER, 2, 0,
                                      INVALID_USER_FLAG, 10, NONE_USER}},
    };

    auto actual = toInitialUserInfoRequest(propValue);

    EXPECT_FALSE(actual.ok()) << "No error returned on invalid user flags";
}

TEST(UserHalHelperTest, TestFailsToInitialUserInfoRequestWithMismatchingPropType) {
    VehiclePropValue propValue{
            .prop = INT32_MAX,
            .value = {.int32Values = {23, FIRST_BOOT_AFTER_OTA, 10, NONE_USER, 2, 0, SYSTEM_USER,
                                      10, NONE_USER}},
    };

    auto actual = toInitialUserInfoRequest(propValue);

    EXPECT_FALSE(actual.ok()) << "No error returned on mismatching property type";
}

TEST(UserHalHelperTest, TestFailsToInitialUserInfoRequestWithInvalidRequestType) {
    VehiclePropValue propValue{
            .prop = INITIAL_USER_INFO,
            .value = {.int32Values = {23, INT32_MAX, 10, NONE_USER, 2, 0, SYSTEM_USER, 10,
                                      NONE_USER}},
    };

    auto actual = toInitialUserInfoRequest(propValue);

    EXPECT_FALSE(actual.ok()) << "No error returned on invalid request type";
}

TEST(UserHalHelperTest, TestFailsToInitialUserInfoRequestWithInvalidUserFlag) {
    VehiclePropValue propValue{
            .prop = INITIAL_USER_INFO,
            .value = {.int32Values = {23, FIRST_BOOT_AFTER_OTA, 10, NONE_USER, 2, 0, SYSTEM_USER,
                                      10, INT32_MAX}},
    };

    auto actual = toInitialUserInfoRequest(propValue);

    EXPECT_FALSE(actual.ok()) << "No error returned on invalid user flags";
}

TEST(UserHalHelperTest, TestFailsToInitialUserInfoRequestWithIncompleteUsersInfo) {
    VehiclePropValue propValueMissingSecondUserInfo{
            .prop = INITIAL_USER_INFO,
            .value = {.int32Values = {23, FIRST_BOOT_AFTER_OTA, 10, NONE_USER, 2, 0,
                                      SYSTEM_USER /*Missing 2nd UserInfo*/}},
    };

    auto actual = toInitialUserInfoRequest(propValueMissingSecondUserInfo);

    EXPECT_FALSE(actual.ok()) << "No error returned on missing second user info";

    VehiclePropValue propValueMissingUsersInfo{
            .prop = INITIAL_USER_INFO,
            .value = {.int32Values = {23, FIRST_BOOT_AFTER_OTA, /*Missing UsersInfo*/}},
    };

    actual = toInitialUserInfoRequest(propValueMissingUsersInfo);

    EXPECT_FALSE(actual.ok()) << "No error returned on missing users info";
}

TEST(UserHalHelperTest, TestToSwitchUserRequest) {
    VehiclePropValue propValue{
            .prop = SWITCH_USER,
            .value = {.int32Values = {23, LEGACY_ANDROID_SWITCH, 0, SYSTEM_USER, 10, NONE_USER, 2,
                                      0, SYSTEM_USER, 10, NONE_USER}},
    };
    SwitchUserRequest expected{
            .requestId = 23,
            .messageType = SwitchUserMessageType::LEGACY_ANDROID_SWITCH,
            .targetUser = {0, SYSTEM_USER},
            .usersInfo = {{10, 0}, 2, {{0, SYSTEM_USER}, {10, 0}}},
    };

    auto actual = toSwitchUserRequest(propValue);

    ASSERT_TRUE(actual.ok()) << actual.error().message();
    EXPECT_THAT(actual.value(), Eq(expected));
}

TEST(UserHalHelperTest, TestFailsToSwitchUserRequestWithMismatchingPropType) {
    VehiclePropValue propValue{
            .prop = INITIAL_USER_INFO,
            .value = {.int32Values = {23, LEGACY_ANDROID_SWITCH, 0, SYSTEM_USER, 10, NONE_USER, 2,
                                      0, SYSTEM_USER, 10, NONE_USER}},
    };

    auto actual = toSwitchUserRequest(propValue);

    EXPECT_FALSE(actual.ok()) << "No error returned on mismatching property type";
}

TEST(UserHalHelperTest, TestFailsToSwitchUserRequestWithInvalidMessageType) {
    VehiclePropValue propValueIncompatibleMessageType{
            .prop = SWITCH_USER,
            .value = {.int32Values = {23, VEHICLE_REQUEST, 0, SYSTEM_USER, 10, NONE_USER, 2, 0,
                                      SYSTEM_USER, 10, NONE_USER}},
    };

    auto actual = toSwitchUserRequest(propValueIncompatibleMessageType);

    EXPECT_FALSE(actual.ok()) << "No error returned on incompatible message type";

    VehiclePropValue propValueInvalidMessageType{
            .prop = SWITCH_USER,
            .value = {.int32Values = {23, INT32_MAX, 0, SYSTEM_USER, 10, NONE_USER, 2, 0,
                                      SYSTEM_USER, 10, NONE_USER}},
    };

    actual = toSwitchUserRequest(propValueInvalidMessageType);

    EXPECT_FALSE(actual.ok()) << "No error returned on invalid message type";
}

TEST(UserHalHelperTest, TestFailsToSwitchUserRequestWithIncompleteUsersInfo) {
    VehiclePropValue propValueMissingSecondUserInfo{
            .prop = SWITCH_USER,
            .value = {.int32Values = {23, LEGACY_ANDROID_SWITCH, 0, SYSTEM_USER, 10, NONE_USER, 2,
                                      0, SYSTEM_USER,
                                      /*Missing 2nd UserInfo*/}},
    };

    auto actual = toSwitchUserRequest(propValueMissingSecondUserInfo);

    EXPECT_FALSE(actual.ok()) << "No error returned on missing second user info";

    VehiclePropValue propValueMissingUsersInfo{
            .prop = SWITCH_USER,
            .value = {.int32Values = {23, LEGACY_ANDROID_SWITCH, 0, SYSTEM_USER,
                                      /*Missing UsersInfo*/}},
    };

    actual = toSwitchUserRequest(propValueMissingUsersInfo);

    EXPECT_FALSE(actual.ok()) << "No error returned on missing users info";

    VehiclePropValue propValueMissingTargetUser{
            .prop = SWITCH_USER,
            .value = {.int32Values = {23, LEGACY_ANDROID_SWITCH, /*Missing target UserInfo*/}},
    };

    actual = toSwitchUserRequest(propValueMissingTargetUser);

    EXPECT_FALSE(actual.ok()) << "No error returned on missing target user info";
}

TEST(UserHalHelperTest, TestToCreateUserRequest) {
    VehiclePropValue propValue{
            .prop = CREATE_USER,
            .value = {.int32Values = {23, 11, GUEST_USER, 10, NONE_USER, 2, 0, SYSTEM_USER, 10,
                                      NONE_USER},
                      .stringValue = "Guest11"},
    };
    CreateUserRequest expected{
            .requestId = 23,
            .newUserInfo = {11, GUEST_USER},
            .newUserName = "Guest11",
            .usersInfo = {{10, 0}, 2, {{0, SYSTEM_USER}, {10, 0}}},
    };

    auto actual = toCreateUserRequest(propValue);

    ASSERT_TRUE(actual.ok()) << actual.error().message();
    EXPECT_THAT(actual.value(), Eq(expected));
}

TEST(UserHalHelperTest, TestFailsToCreateUserRequestWithMismatchingPropType) {
    VehiclePropValue propValue{
            .prop = INITIAL_USER_INFO,
            .value = {.int32Values = {23, 11, GUEST_USER, 10, NONE_USER, 2, 0, SYSTEM_USER, 10,
                                      NONE_USER},
                      .stringValue = "Guest11"},
    };

    auto actual = toCreateUserRequest(propValue);

    EXPECT_FALSE(actual.ok()) << "No error returned on mismatching property type";
}

TEST(UserHalHelperTest, TestFailsToCreateUserRequestWithIncompleteUsersInfo) {
    VehiclePropValue propValueMissingSecondUserInfo{
            .prop = CREATE_USER,
            .value = {.int32Values = {23, 11, GUEST_USER, 10, NONE_USER, 2, 0,
                                      SYSTEM_USER /*Missing 2nd UserInfo*/},
                      .stringValue = "Guest11"},
    };

    auto actual = toCreateUserRequest(propValueMissingSecondUserInfo);

    EXPECT_FALSE(actual.ok()) << "No error returned on missing second user info";

    VehiclePropValue propValueMissingUsersInfo{
            .prop = CREATE_USER,
            .value = {.int32Values = {23, 11, GUEST_USER, /*Missing UsersInfo*/},
                      .stringValue = "Guest11"},
    };

    actual = toCreateUserRequest(propValueMissingUsersInfo);

    EXPECT_FALSE(actual.ok()) << "No error returned on missing users info";

    VehiclePropValue propValueMissingCreateUserInfo{
            .prop = CREATE_USER,
            .value = {.int32Values = {23, /*Missing create UserInfo*/}, .stringValue = "Guest11"},
    };

    actual = toCreateUserRequest(propValueMissingCreateUserInfo);

    EXPECT_FALSE(actual.ok()) << "No error returned on missing create user info";
}

TEST(UserHalHelperTest, TestToRemoveUserRequest) {
    VehiclePropValue propValue{
            .prop = REMOVE_USER,
            .value = {.int32Values = {23, 10, NONE_USER, 10, NONE_USER, 2, 0, SYSTEM_USER, 10,
                                      NONE_USER}},
    };
    RemoveUserRequest expected{
            .requestId = 23,
            .removedUserInfo = {10, 0},
            .usersInfo = {{10, 0}, 2, {{0, SYSTEM_USER}, {10, 0}}},
    };

    auto actual = toRemoveUserRequest(propValue);

    ASSERT_TRUE(actual.ok()) << actual.error().message();
    EXPECT_THAT(actual.value(), Eq(expected));
}

TEST(UserHalHelperTest, TestFailsToRemoveUserRequestWithMismatchingPropType) {
    VehiclePropValue propValue{
            .prop = INITIAL_USER_INFO,
            .value = {.int32Values = {23, 10, NONE_USER, 10, NONE_USER, 2, 0, SYSTEM_USER, 10,
                                      NONE_USER}},
    };

    auto actual = toRemoveUserRequest(propValue);

    EXPECT_FALSE(actual.ok()) << "No error returned on mismatching property type";
}

TEST(UserHalHelperTest, TestFailsToRemoveUserRequestWithIncompleteUsersInfo) {
    VehiclePropValue propValueMissingSecondUserInfo{
            .prop = REMOVE_USER,
            .value = {.int32Values = {23, 10, NONE_USER, 10, NONE_USER, 2, 0,
                                      SYSTEM_USER /*Missing 2nd UserInfo*/}},
    };

    auto actual = toRemoveUserRequest(propValueMissingSecondUserInfo);

    EXPECT_FALSE(actual.ok()) << "No error returned on missing second user info";

    VehiclePropValue propValueMissingUsersInfo{
            .prop = REMOVE_USER,
            .value = {.int32Values = {23, 10, NONE_USER, /*Missing UsersInfo*/}},
    };

    actual = toRemoveUserRequest(propValueMissingUsersInfo);

    EXPECT_FALSE(actual.ok()) << "No error returned on missing users info";

    VehiclePropValue propValueMissingRemoveUserInfo{
            .prop = REMOVE_USER,
            .value = {.int32Values = {23, /*Missing remove UserInfo*/}},
    };

    actual = toRemoveUserRequest(propValueMissingRemoveUserInfo);

    EXPECT_FALSE(actual.ok()) << "No error returned on missing remove user info";
}

TEST(UserHalHelperTest, TestFailsToUserIdentificationGetRequest) {
    VehiclePropValue propValue{
            .prop = USER_IDENTIFICATION_ASSOCIATION,
            .value = {.int32Values = {23, 10, NONE_USER, 2, USER_ID_ASSOC_KEY_FOB,
                                      USER_ID_ASSOC_CUSTOM_1}},
    };
    UserIdentificationGetRequest expected{
            .requestId = 23,
            .userInfo = {10, 0},
            .numberAssociationTypes = 2,
            .associationTypes = {UserIdentificationAssociationType::KEY_FOB,
                                 UserIdentificationAssociationType::CUSTOM_1},
    };

    auto actual = toUserIdentificationGetRequest(propValue);

    ASSERT_TRUE(actual.ok()) << actual.error().message();
    EXPECT_THAT(actual.value(), Eq(expected));
}

TEST(UserHalHelperTest, TestFailsToUserIdentificationGetRequestWithMismatchingPropType) {
    VehiclePropValue propValue{
            .prop = INITIAL_USER_INFO,
            .value = {.int32Values = {23, 10, NONE_USER, 2, USER_ID_ASSOC_KEY_FOB,
                                      USER_ID_ASSOC_CUSTOM_1}},
    };

    auto actual = toUserIdentificationGetRequest(propValue);

    EXPECT_FALSE(actual.ok()) << "No error returned on mismatching property type";
}

TEST(UserHalHelperTest, TestFailsToUserIdentificationGetRequestWithInvalidAssociationTypes) {
    VehiclePropValue propValue{
            .prop = USER_IDENTIFICATION_ASSOCIATION,
            .value = {.int32Values = {23, 10, NONE_USER, 1, INT32_MAX}},
    };

    auto actual = toUserIdentificationGetRequest(propValue);

    EXPECT_FALSE(actual.ok()) << "No error returned on invalid association type";
}

TEST(UserHalHelperTest, TestFailsToUserIdentificationGetRequestWithIncompleteAssociationTypes) {
    VehiclePropValue propValueMissingSecondAssociationType{
            .prop = USER_IDENTIFICATION_ASSOCIATION,
            .value = {.int32Values = {23, 10, NONE_USER, 2,
                                      USER_ID_ASSOC_KEY_FOB /*Missing 2nd association type*/}},
    };

    auto actual = toUserIdentificationGetRequest(propValueMissingSecondAssociationType);

    EXPECT_FALSE(actual.ok()) << "No error returned on missing second association type";

    VehiclePropValue propValueMissingNumberAssociationTypes{
            .prop = USER_IDENTIFICATION_ASSOCIATION,
            .value = {.int32Values = {23, 10, NONE_USER, /*Missing number association types*/}},
    };

    actual = toUserIdentificationGetRequest(propValueMissingNumberAssociationTypes);

    EXPECT_FALSE(actual.ok()) << "No error returned on missing number association types";
}

TEST(UserHalHelperTest, TestFailsToUserIdentificationGetRequestWithMissingUserInfo) {
    VehiclePropValue propValue{
            .prop = USER_IDENTIFICATION_ASSOCIATION,
            .value = {.int32Values = {23, /*Missing user info*/}},
    };

    auto actual = toUserIdentificationGetRequest(propValue);

    EXPECT_FALSE(actual.ok()) << "No error returned on missing UserInfo";
}

TEST(UserHalHelperTest, TestToUserIdentificationSetRequest) {
    VehiclePropValue propValue{
            .prop = USER_IDENTIFICATION_ASSOCIATION,
            .value = {.int32Values = {23, 10, NONE_USER, 2, USER_ID_ASSOC_KEY_FOB,
                                      USER_ID_ASSOC_SET_CURRENT_USER, USER_ID_ASSOC_CUSTOM_1,
                                      USER_ID_ASSOC_UNSET_CURRENT_USER}},
    };
    UserIdentificationSetRequest expected{
            .requestId = 23,
            .userInfo = {10, 0},
            .numberAssociations = 2,
            .associations = {{UserIdentificationAssociationType::KEY_FOB,
                              UserIdentificationAssociationSetValue::ASSOCIATE_CURRENT_USER},
                             {UserIdentificationAssociationType::CUSTOM_1,
                              UserIdentificationAssociationSetValue::DISASSOCIATE_CURRENT_USER}},
    };

    auto actual = toUserIdentificationSetRequest(propValue);

    ASSERT_TRUE(actual.ok()) << actual.error().message();
    EXPECT_THAT(actual.value(), Eq(expected));
}

TEST(UserHalHelperTest, TestFailsToUserIdentificationSetRequestWithMismatchingPropType) {
    VehiclePropValue propValue{
            .prop = INITIAL_USER_INFO,
            .value = {.int32Values = {23, 10, NONE_USER, 2, USER_ID_ASSOC_KEY_FOB,
                                      USER_ID_ASSOC_SET_CURRENT_USER, USER_ID_ASSOC_CUSTOM_1,
                                      USER_ID_ASSOC_UNSET_CURRENT_USER}},
    };

    auto actual = toUserIdentificationSetRequest(propValue);

    EXPECT_FALSE(actual.ok()) << "No error returned on mismatching property type";
}

TEST(UserHalHelperTest, TestFailsToUserIdentificationSetRequestWithInvalidAssociations) {
    VehiclePropValue propValueInvalidAssociationType{
            .prop = USER_IDENTIFICATION_ASSOCIATION,
            .value = {.int32Values = {23, 10, NONE_USER, 1, INT32_MAX,
                                      USER_ID_ASSOC_SET_CURRENT_USER}},
    };

    auto actual = toUserIdentificationSetRequest(propValueInvalidAssociationType);

    EXPECT_FALSE(actual.ok()) << "No error returned on invalid association type";

    VehiclePropValue propValueInvalidAssociationValue{
            .prop = USER_IDENTIFICATION_ASSOCIATION,
            .value = {.int32Values = {23, 10, NONE_USER, USER_ID_ASSOC_KEY_FOB, INT32_MAX}},
    };

    actual = toUserIdentificationSetRequest(propValueInvalidAssociationValue);

    EXPECT_FALSE(actual.ok()) << "No error returned on missing number association types";
}

TEST(UserHalHelperTest, TestFailsToUserIdentificationSetRequestWithIncompleteAssociations) {
    VehiclePropValue propValueMissingSecondAssociationType{
            .prop = USER_IDENTIFICATION_ASSOCIATION,
            .value = {.int32Values = {23, 10, NONE_USER, 2, USER_ID_ASSOC_KEY_FOB,
                                      USER_ID_ASSOC_SET_CURRENT_USER,
                                      /*Missing 2nd association*/}},
    };

    auto actual = toUserIdentificationSetRequest(propValueMissingSecondAssociationType);

    EXPECT_FALSE(actual.ok()) << "No error returned on missing second association type";

    VehiclePropValue propValueMissingNumberAssociationTypes{
            .prop = USER_IDENTIFICATION_ASSOCIATION,
            .value = {.int32Values = {23, 10, NONE_USER, /*Missing number associations*/}},
    };

    actual = toUserIdentificationSetRequest(propValueMissingNumberAssociationTypes);

    EXPECT_FALSE(actual.ok()) << "No error returned on missing number association types";
}

TEST(UserHalHelperTest, TestFailsToUserIdentificationSetRequestWithMissingUserInfo) {
    VehiclePropValue propValue{
            .prop = USER_IDENTIFICATION_ASSOCIATION,
            .value = {.int32Values = {23, /*Missing user info*/}},
    };

    auto actual = toUserIdentificationSetRequest(propValue);

    EXPECT_FALSE(actual.ok()) << "No error returned on missing UserInfo";
}

TEST(UserHalHelperTest, TestSwitchUserRequestToVehiclePropValue) {
    SwitchUserRequest request{
            .requestId = 23,
            .messageType = SwitchUserMessageType::VEHICLE_REQUEST,
            .targetUser = {11, GUEST_USER},
    };
    VehiclePropValue expected{
            .prop = SWITCH_USER,
            .value = {.int32Values = {23,
                                      static_cast<int32_t>(SwitchUserMessageType::VEHICLE_REQUEST),
                                      11}},
    };

    VehiclePropValuePool pool;
    auto result = toVehiclePropValue(pool, request);

    ASSERT_TRUE(result.ok());
    EXPECT_THAT(result.value()->timestamp, Gt(0));
    // Don't rely on real timestamp in tests as the expected and actual objects won't have the same
    // timestamps. Thus remove the timestamps before comparing them.
    result.value()->timestamp = 0;
    EXPECT_THAT(result.value(), Pointee(Eq(expected)));
}

TEST(UserHalHelperTest, TestFailsSwitchUserRequestToVehiclePropValueWithIncompatibleMessageType) {
    SwitchUserRequest request{
            .requestId = 23,
            .messageType = SwitchUserMessageType::VEHICLE_RESPONSE,
            .targetUser = {11, GUEST_USER},
    };

    VehiclePropValuePool pool;
    auto result = toVehiclePropValue(pool, request);

    EXPECT_FALSE(result.ok());
}

TEST(UserHalHelperTest, TestInitialUserInfoResponseToVehiclePropValue) {
    InitialUserInfoResponse response{
            .requestId = 23,
            .action = InitialUserInfoResponseAction::CREATE,
            .userToSwitchOrCreate = {11, GUEST_USER},
            .userLocales = "en-US,pt-BR",
            .userNameToCreate = "Owner",
    };
    VehiclePropValue expected{
            .prop = INITIAL_USER_INFO,
            .value = {.int32Values = {23,
                                      static_cast<int32_t>(InitialUserInfoResponseAction::CREATE),
                                      11, GUEST_USER},
                      .stringValue = "en-US,pt-BR||Owner"},
    };

    VehiclePropValuePool pool;
    auto actual = toVehiclePropValue(pool, response);

    ASSERT_THAT(actual, NotNull());
    EXPECT_THAT(actual->timestamp, Gt(0));
    actual->timestamp = 0;
    EXPECT_THAT(actual, Pointee(Eq(expected)));
}

TEST(UserHalHelperTest, TestSwitchUserResponseToVehiclePropValue) {
    SwitchUserResponse response{
            .requestId = 23,
            .messageType = SwitchUserMessageType::VEHICLE_RESPONSE,
            .status = SwitchUserStatus::FAILURE,
            .errorMessage = "random error",
    };
    VehiclePropValue expected{
            .prop = SWITCH_USER,
            .value = {.int32Values = {23,
                                      static_cast<int32_t>(SwitchUserMessageType::VEHICLE_RESPONSE),
                                      static_cast<int32_t>(SwitchUserStatus::FAILURE)},
                      .stringValue = "random error"},
    };

    VehiclePropValuePool pool;
    auto actual = toVehiclePropValue(pool, response);

    ASSERT_THAT(actual, NotNull());
    EXPECT_THAT(actual->timestamp, Gt(0));
    actual->timestamp = 0;
    EXPECT_THAT(actual, Pointee(Eq(expected)));
}

TEST(UserHalHelperTest, TestCreateUserResponseToVehiclePropValue) {
    CreateUserResponse response{
            .requestId = 23,
            .status = CreateUserStatus::FAILURE,
            .errorMessage = "random error",
    };
    VehiclePropValue expected{
            .prop = CREATE_USER,
            .value = {.int32Values = {23, static_cast<int32_t>(CreateUserStatus::FAILURE)},
                      .stringValue = "random error"},
    };

    VehiclePropValuePool pool;
    auto actual = toVehiclePropValue(pool, response);

    ASSERT_THAT(actual, NotNull());
    EXPECT_THAT(actual->timestamp, Gt(0));
    actual->timestamp = 0;
    EXPECT_THAT(actual, Pointee(Eq(expected)));
}

TEST(UserHalHelperTest, TestUserIdentificationResponseToVehiclePropValue) {
    UserIdentificationResponse response{
            .requestId = 23,
            .numberAssociation = 2,
            .associations = {{UserIdentificationAssociationType::KEY_FOB,
                              UserIdentificationAssociationValue::ASSOCIATED_CURRENT_USER},
                             {UserIdentificationAssociationType::CUSTOM_1,
                              UserIdentificationAssociationValue::NOT_ASSOCIATED_ANY_USER}},
            .errorMessage = "random error",
    };
    VehiclePropValue expected{
            .prop = USER_IDENTIFICATION_ASSOCIATION,
            .value = {.int32Values = {23, 2, USER_ID_ASSOC_KEY_FOB, USER_ID_ASSOC_CURRENT_USER,
                                      USER_ID_ASSOC_CUSTOM_1, USER_ID_ASSOC_NO_USER},
                      .stringValue = "random error"},
    };

    VehiclePropValuePool pool;
    auto actual = toVehiclePropValue(pool, response);

    ASSERT_THAT(actual, NotNull());
    EXPECT_THAT(actual->timestamp, Gt(0));
    actual->timestamp = 0;
    EXPECT_THAT(actual, Pointee(Eq(expected)));
}

}  // namespace user_hal_helper
}  // namespace fake
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
