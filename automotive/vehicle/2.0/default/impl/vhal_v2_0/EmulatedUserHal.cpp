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
#define LOG_TAG "EmulatedUserHal"

#include "EmulatedUserHal.h"

#include <cutils/log.h>
#include <utils/SystemClock.h>

#include "UserHalHelper.h"

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace V2_0 {

namespace impl {

namespace {

using android::base::Error;
using android::base::Result;

constexpr int32_t INITIAL_USER_INFO = static_cast<int32_t>(VehicleProperty::INITIAL_USER_INFO);
constexpr int32_t SWITCH_USER = static_cast<int32_t>(VehicleProperty::SWITCH_USER);
constexpr int32_t CREATE_USER = static_cast<int32_t>(VehicleProperty::CREATE_USER);
constexpr int32_t REMOVE_USER = static_cast<int32_t>(VehicleProperty::REMOVE_USER);
constexpr int32_t USER_IDENTIFICATION_ASSOCIATION =
        static_cast<int32_t>(VehicleProperty::USER_IDENTIFICATION_ASSOCIATION);

Result<int32_t> getRequestId(const VehiclePropValue& value) {
    if (value.value.int32Values.size() < 1) {
        return Error(static_cast<int>(StatusCode::INVALID_ARG))
               << "no int32values on " << toString(value);
    }
    return value.value.int32Values[0];
}

Result<SwitchUserMessageType> getSwitchUserMessageType(const VehiclePropValue& value) {
    if (value.value.int32Values.size() < 2) {
        return Error(static_cast<int>(StatusCode::INVALID_ARG))
               << "missing switch user message type " << toString(value);
    }
    return user_hal_helper::verifyAndCast<SwitchUserMessageType>(value.value.int32Values[1]);
}

}  // namespace

bool EmulatedUserHal::isSupported(int32_t prop) {
    switch (prop) {
        case INITIAL_USER_INFO:
        case SWITCH_USER:
        case CREATE_USER:
        case REMOVE_USER:
        case USER_IDENTIFICATION_ASSOCIATION:
            return true;
        default:
            return false;
    }
}

Result<std::unique_ptr<VehiclePropValue>> EmulatedUserHal::onSetProperty(
        const VehiclePropValue& value) {
    ALOGV("onSetProperty(): %s", toString(value).c_str());

    switch (value.prop) {
        case INITIAL_USER_INFO:
            return onSetInitialUserInfoResponse(value);
        case SWITCH_USER:
            return onSetSwitchUserResponse(value);
        case CREATE_USER:
            return onSetCreateUserResponse(value);
        case REMOVE_USER:
            ALOGI("REMOVE_USER is FYI only, nothing to do...");
            return {};
        case USER_IDENTIFICATION_ASSOCIATION:
            return onSetUserIdentificationAssociation(value);
        default:
            return Error(static_cast<int>(StatusCode::INVALID_ARG))
                   << "Unsupported property: " << toString(value);
    }
}

Result<std::unique_ptr<VehiclePropValue>> EmulatedUserHal::onGetProperty(
        const VehiclePropValue& value) {
    ALOGV("onGetProperty(%s)", toString(value).c_str());
    switch (value.prop) {
        case INITIAL_USER_INFO:
        case SWITCH_USER:
        case CREATE_USER:
        case REMOVE_USER:
            ALOGE("onGetProperty(): %d is only supported on SET", value.prop);
            return Error(static_cast<int>(StatusCode::INVALID_ARG)) << "only supported on SET";
        case USER_IDENTIFICATION_ASSOCIATION:
            return onGetUserIdentificationAssociation(value);
        default:
            ALOGE("onGetProperty(): %d is not supported", value.prop);
            return Error(static_cast<int>(StatusCode::INVALID_ARG)) << "not supported by User HAL";
    }
}

Result<std::unique_ptr<VehiclePropValue>> EmulatedUserHal::onGetUserIdentificationAssociation(
        const VehiclePropValue& value) {
    if (mSetUserIdentificationAssociationResponseFromCmd == nullptr) {
        return defaultUserIdentificationAssociation(value);
    }
    ALOGI("get(USER_IDENTIFICATION_ASSOCIATION): returning %s",
          toString(*mSetUserIdentificationAssociationResponseFromCmd).c_str());
    auto newValue = std::unique_ptr<VehiclePropValue>(
            new VehiclePropValue(*mSetUserIdentificationAssociationResponseFromCmd));
    auto requestId = getRequestId(value);
    if (requestId.ok()) {
        // Must use the same requestId
        newValue->value.int32Values[0] = *requestId;
    } else {
        ALOGE("get(USER_IDENTIFICATION_ASSOCIATION): no requestId on %s", toString(value).c_str());
    }
    return newValue;
}

Result<std::unique_ptr<VehiclePropValue>> EmulatedUserHal::onSetInitialUserInfoResponse(
        const VehiclePropValue& value) {
    auto requestId = getRequestId(value);
    if (!requestId.ok()) {
        ALOGE("Failed to get requestId on set(INITIAL_USER_INFO): %s",
              requestId.error().message().c_str());
        return requestId.error();
    }

    if (value.areaId != 0) {
        ALOGD("set(INITIAL_USER_INFO) called from lshal; storing it: %s", toString(value).c_str());
        mInitialUserResponseFromCmd.reset(new VehiclePropValue(value));
        return {};
    }

    ALOGD("set(INITIAL_USER_INFO) called from Android: %s", toString(value).c_str());
    if (mInitialUserResponseFromCmd != nullptr) {
        ALOGI("replying INITIAL_USER_INFO with lshal value:  %s",
              toString(*mInitialUserResponseFromCmd).c_str());
        return sendUserHalResponse(std::move(mInitialUserResponseFromCmd), *requestId);
    }

    // Returns default response
    auto updatedValue = user_hal_helper::toVehiclePropValue(InitialUserInfoResponse{
            .requestId = *requestId,
            .action = InitialUserInfoResponseAction::DEFAULT,
    });
    ALOGI("no lshal response; replying with InitialUserInfoResponseAction::DEFAULT: %s",
          toString(*updatedValue).c_str());
    return updatedValue;
}

Result<std::unique_ptr<VehiclePropValue>> EmulatedUserHal::onSetSwitchUserResponse(
        const VehiclePropValue& value) {
    auto requestId = getRequestId(value);
    if (!requestId.ok()) {
        ALOGE("Failed to get requestId on set(SWITCH_USER): %s",
              requestId.error().message().c_str());
        return requestId.error();
    }

    auto messageType = getSwitchUserMessageType(value);
    if (!messageType.ok()) {
        ALOGE("Failed to get messageType on set(SWITCH_USER): %s",
              messageType.error().message().c_str());
        return messageType.error();
    }

    if (value.areaId != 0) {
        if (*messageType == SwitchUserMessageType::VEHICLE_REQUEST) {
            // User HAL can also request a user switch, so we need to check it first
            ALOGD("set(SWITCH_USER) called from lshal to emulate a vehicle request: %s",
                  toString(value).c_str());
            return std::unique_ptr<VehiclePropValue>(new VehiclePropValue(value));
        }
        // Otherwise, we store it
        ALOGD("set(SWITCH_USER) called from lshal; storing it: %s", toString(value).c_str());
        mSwitchUserResponseFromCmd.reset(new VehiclePropValue(value));
        return {};
    }
    ALOGD("set(SWITCH_USER) called from Android: %s", toString(value).c_str());

    if (mSwitchUserResponseFromCmd != nullptr) {
        ALOGI("replying SWITCH_USER with lshal value:  %s",
              toString(*mSwitchUserResponseFromCmd).c_str());
        return sendUserHalResponse(std::move(mSwitchUserResponseFromCmd), *requestId);
    }

    if (*messageType == SwitchUserMessageType::LEGACY_ANDROID_SWITCH ||
        *messageType == SwitchUserMessageType::ANDROID_POST_SWITCH) {
        ALOGI("request is %s; ignoring it", toString(*messageType).c_str());
        return {};
    }

    // Returns default response
    auto updatedValue = user_hal_helper::toVehiclePropValue(SwitchUserResponse{
            .requestId = *requestId,
            .messageType = SwitchUserMessageType::VEHICLE_RESPONSE,
            .status = SwitchUserStatus::SUCCESS,
    });
    ALOGI("no lshal response; replying with VEHICLE_RESPONSE / SUCCESS: %s",
          toString(*updatedValue).c_str());
    return updatedValue;
}

Result<std::unique_ptr<VehiclePropValue>> EmulatedUserHal::onSetCreateUserResponse(
        const VehiclePropValue& value) {
    auto requestId = getRequestId(value);
    if (!requestId.ok()) {
        ALOGE("Failed to get requestId on set(CREATE_USER): %s",
              requestId.error().message().c_str());
        return requestId.error();
    }

    if (value.areaId != 0) {
        ALOGD("set(CREATE_USER) called from lshal; storing it: %s", toString(value).c_str());
        mCreateUserResponseFromCmd.reset(new VehiclePropValue(value));
        return {};
    }
    ALOGD("set(CREATE_USER) called from Android: %s", toString(value).c_str());

    if (mCreateUserResponseFromCmd != nullptr) {
        ALOGI("replying CREATE_USER with lshal value:  %s",
              toString(*mCreateUserResponseFromCmd).c_str());
        return sendUserHalResponse(std::move(mCreateUserResponseFromCmd), *requestId);
    }

    // Returns default response
    auto updatedValue = user_hal_helper::toVehiclePropValue(CreateUserResponse{
            .requestId = *requestId,
            .status = CreateUserStatus::SUCCESS,
    });
    ALOGI("no lshal response; replying with SUCCESS: %s", toString(*updatedValue).c_str());
    return updatedValue;
}

Result<std::unique_ptr<VehiclePropValue>> EmulatedUserHal::onSetUserIdentificationAssociation(
        const VehiclePropValue& value) {
    auto requestId = getRequestId(value);
    if (!requestId.ok()) {
        ALOGE("Failed to get requestId on set(USER_IDENTIFICATION_ASSOCIATION): %s",
              requestId.error().message().c_str());
        return requestId.error();
    }

    if (value.areaId != 0) {
        ALOGD("set(USER_IDENTIFICATION_ASSOCIATION) called from lshal; storing it: %s",
              toString(value).c_str());
        mSetUserIdentificationAssociationResponseFromCmd.reset(new VehiclePropValue(value));
        return {};
    }
    ALOGD("set(USER_IDENTIFICATION_ASSOCIATION) called from Android: %s", toString(value).c_str());

    if (mSetUserIdentificationAssociationResponseFromCmd != nullptr) {
        ALOGI("replying USER_IDENTIFICATION_ASSOCIATION with lshal value:  %s",
              toString(*mSetUserIdentificationAssociationResponseFromCmd).c_str());
        // Not moving response so it can be used on GET requests
        auto copy = std::unique_ptr<VehiclePropValue>(
                new VehiclePropValue(*mSetUserIdentificationAssociationResponseFromCmd));
        return sendUserHalResponse(std::move(copy), *requestId);
    }
    // Returns default response
    return defaultUserIdentificationAssociation(value);
}

Result<std::unique_ptr<VehiclePropValue>> EmulatedUserHal::defaultUserIdentificationAssociation(
        const VehiclePropValue& request) {
    // TODO(b/159498909): return a response with NOT_ASSOCIATED_ANY_USER for all requested types
    ALOGE("no lshal response for %s; replying with NOT_AVAILABLE", toString(request).c_str());
    return Error(static_cast<int>(StatusCode::NOT_AVAILABLE)) << "not set by lshal";
}

Result<std::unique_ptr<VehiclePropValue>> EmulatedUserHal::sendUserHalResponse(
        std::unique_ptr<VehiclePropValue> response, int32_t requestId) {
    switch (response->areaId) {
        case 1:
            ALOGD("returning response with right request id");
            response->value.int32Values[0] = requestId;
            break;
        case 2:
            ALOGD("returning response with wrong request id");
            response->value.int32Values[0] = -requestId;
            break;
        case 3:
            ALOGD("not generating a property change event because of lshal prop: %s",
                  toString(*response).c_str());
            return Error(static_cast<int>(StatusCode::NOT_AVAILABLE))
                   << "not generating a property change event because of lshal prop: "
                   << toString(*response);
        default:
            ALOGE("invalid action on lshal response: %s", toString(*response).c_str());
            return Error(static_cast<int>(StatusCode::INTERNAL_ERROR))
                   << "invalid action on lshal response: " << toString(*response);
    }

    ALOGD("updating property to: %s", toString(*response).c_str());
    return response;
}

void EmulatedUserHal::showDumpHelp(int fd) {
    dprintf(fd, "%s: dumps state used for user management\n", kUserHalDumpOption);
}

void EmulatedUserHal::dump(int fd, std::string indent) {
    if (mInitialUserResponseFromCmd != nullptr) {
        dprintf(fd, "%sInitialUserInfo response: %s\n", indent.c_str(),
                toString(*mInitialUserResponseFromCmd).c_str());
    } else {
        dprintf(fd, "%sNo InitialUserInfo response\n", indent.c_str());
    }
    if (mSwitchUserResponseFromCmd != nullptr) {
        dprintf(fd, "%sSwitchUser response: %s\n", indent.c_str(),
                toString(*mSwitchUserResponseFromCmd).c_str());
    } else {
        dprintf(fd, "%sNo SwitchUser response\n", indent.c_str());
    }
    if (mCreateUserResponseFromCmd != nullptr) {
        dprintf(fd, "%sCreateUser response: %s\n", indent.c_str(),
                toString(*mCreateUserResponseFromCmd).c_str());
    } else {
        dprintf(fd, "%sNo CreateUser response\n", indent.c_str());
    }
    if (mSetUserIdentificationAssociationResponseFromCmd != nullptr) {
        dprintf(fd, "%sSetUserIdentificationAssociation response: %s\n", indent.c_str(),
                toString(*mSetUserIdentificationAssociationResponseFromCmd).c_str());
    } else {
        dprintf(fd, "%sNo SetUserIdentificationAssociation response\n", indent.c_str());
    }
}

}  // namespace impl

}  // namespace V2_0
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
