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
#define LOG_TAG "FakeUserHal"

#include "FakeUserHal.h"

#include "UserHalHelper.h"

#include <VehicleUtils.h>
#include <utils/Log.h>
#include <utils/SystemClock.h>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace fake {

namespace {

using ::aidl::android::hardware::automotive::vehicle::CreateUserResponse;
using ::aidl::android::hardware::automotive::vehicle::CreateUserStatus;
using ::aidl::android::hardware::automotive::vehicle::InitialUserInfoResponse;
using ::aidl::android::hardware::automotive::vehicle::InitialUserInfoResponseAction;
using ::aidl::android::hardware::automotive::vehicle::StatusCode;
using ::aidl::android::hardware::automotive::vehicle::SwitchUserMessageType;
using ::aidl::android::hardware::automotive::vehicle::SwitchUserResponse;
using ::aidl::android::hardware::automotive::vehicle::SwitchUserStatus;
using ::aidl::android::hardware::automotive::vehicle::VehicleProperty;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropValue;
using ::android::base::Error;
using ::android::base::Result;

constexpr int32_t INITIAL_USER_INFO = toInt(VehicleProperty::INITIAL_USER_INFO);
constexpr int32_t SWITCH_USER = toInt(VehicleProperty::SWITCH_USER);
constexpr int32_t CREATE_USER = toInt(VehicleProperty::CREATE_USER);
constexpr int32_t REMOVE_USER = toInt(VehicleProperty::REMOVE_USER);
constexpr int32_t USER_IDENTIFICATION_ASSOCIATION =
        toInt(VehicleProperty::USER_IDENTIFICATION_ASSOCIATION);

VhalResult<int32_t> getRequestId(const VehiclePropValue& value) {
    if (value.value.int32Values.size() < 1) {
        return StatusError(StatusCode::INVALID_ARG)
               << "no int32Values on property: " << value.toString();
    }
    return value.value.int32Values[0];
}

VhalResult<SwitchUserMessageType> getSwitchUserMessageType(const VehiclePropValue& value) {
    if (value.value.int32Values.size() < 2) {
        return StatusError(StatusCode::INVALID_ARG)
               << "missing switch user message type on property: " << value.toString();
    }
    auto result = user_hal_helper::verifyAndCast<SwitchUserMessageType>(value.value.int32Values[1]);
    if (!result.ok()) {
        return StatusError(StatusCode::INVALID_ARG) << result.error().message();
    }
    return result.value();
}

}  // namespace

bool FakeUserHal::isSupported(int32_t prop) {
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

FakeUserHal::ValueResultType FakeUserHal::onSetProperty(const VehiclePropValue& value) {
    ALOGV("onSetProperty(): %s", value.toString().c_str());

    switch (value.prop) {
        case INITIAL_USER_INFO:
            return onSetInitialUserInfoResponse(value);
        case SWITCH_USER:
            return onSetSwitchUserResponse(value);
        case CREATE_USER:
            return onSetCreateUserResponse(value);
        case REMOVE_USER:
            ALOGI("REMOVE_USER is FYI only, nothing to do...");
            return nullptr;
        case USER_IDENTIFICATION_ASSOCIATION:
            return onSetUserIdentificationAssociation(value);
        default:
            return StatusError(StatusCode::INVALID_ARG)
                   << "Unsupported property: " << value.toString();
    }
}

FakeUserHal::ValueResultType FakeUserHal::onGetProperty(const VehiclePropValue& value) const {
    ALOGV("onGetProperty(%s)", value.toString().c_str());
    switch (value.prop) {
        case INITIAL_USER_INFO:
        case SWITCH_USER:
        case CREATE_USER:
        case REMOVE_USER:
            ALOGE("onGetProperty(): %d is only supported on SET", value.prop);
            return StatusError(StatusCode::INVALID_ARG) << "only supported on SET";
        case USER_IDENTIFICATION_ASSOCIATION:
            return onGetUserIdentificationAssociation(value);
        default:
            ALOGE("onGetProperty(): %d is not supported", value.prop);
            return StatusError(StatusCode::INVALID_ARG) << "not supported by User HAL";
    }
}

FakeUserHal::ValueResultType FakeUserHal::onGetUserIdentificationAssociation(
        const VehiclePropValue& value) const {
    std::scoped_lock<std::mutex> lockGuard(mLock);

    if (mSetUserIdentificationAssociationResponseFromCmd == nullptr) {
        return defaultUserIdentificationAssociation(value);
    }
    ALOGI("get(USER_IDENTIFICATION_ASSOCIATION): returning %s",
          mSetUserIdentificationAssociationResponseFromCmd->toString().c_str());
    auto newValue = mValuePool->obtain(*mSetUserIdentificationAssociationResponseFromCmd);
    auto requestId = getRequestId(value);
    if (requestId.ok()) {
        // Must use the same requestId
        newValue->value.int32Values[0] = *requestId;
    } else {
        ALOGE("get(USER_IDENTIFICATION_ASSOCIATION): no requestId on %s", value.toString().c_str());
        return requestId.error();
    }
    return newValue;
}

FakeUserHal::ValueResultType FakeUserHal::onSetInitialUserInfoResponse(
        const VehiclePropValue& value) {
    std::scoped_lock<std::mutex> lockGuard(mLock);

    auto requestId = getRequestId(value);
    if (!requestId.ok()) {
        ALOGE("Failed to get requestId on set(INITIAL_USER_INFO): %s",
              requestId.error().message().c_str());
        return requestId.error();
    }

    if (value.areaId != 0) {
        ALOGD("set(INITIAL_USER_INFO) called from lshal; storing it: %s", value.toString().c_str());
        mInitialUserResponseFromCmd = mValuePool->obtain(value);
        return nullptr;
    }

    ALOGD("set(INITIAL_USER_INFO) called from Android: %s", value.toString().c_str());
    if (mInitialUserResponseFromCmd != nullptr) {
        ALOGI("replying INITIAL_USER_INFO with lshal value: %s",
              mInitialUserResponseFromCmd->toString().c_str());
        return sendUserHalResponse(std::move(mInitialUserResponseFromCmd), *requestId);
    }

    // Returns default response
    auto updatedValue = user_hal_helper::toVehiclePropValue(
            *mValuePool, InitialUserInfoResponse{
                                 .requestId = *requestId,
                                 .action = InitialUserInfoResponseAction::DEFAULT,
                         });
    ALOGI("no lshal response; replying with InitialUserInfoResponseAction::DEFAULT: %s",
          updatedValue->toString().c_str());
    return updatedValue;
}

FakeUserHal::ValueResultType FakeUserHal::onSetSwitchUserResponse(const VehiclePropValue& value) {
    std::scoped_lock<std::mutex> lockGuard(mLock);

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
                  value.toString().c_str());
            return mValuePool->obtain(value);
        }
        // Otherwise, we store it
        ALOGD("set(SWITCH_USER) called from lshal; storing it: %s", value.toString().c_str());
        mSwitchUserResponseFromCmd = mValuePool->obtain(value);
        return nullptr;
    }
    ALOGD("set(SWITCH_USER) called from Android: %s", value.toString().c_str());

    if (mSwitchUserResponseFromCmd != nullptr) {
        ALOGI("replying SWITCH_USER with lshal value:  %s",
              mSwitchUserResponseFromCmd->toString().c_str());
        return sendUserHalResponse(std::move(mSwitchUserResponseFromCmd), *requestId);
    }

    if (*messageType == SwitchUserMessageType::LEGACY_ANDROID_SWITCH ||
        *messageType == SwitchUserMessageType::ANDROID_POST_SWITCH) {
        ALOGI("request is %s; ignoring it", toString(*messageType).c_str());
        return nullptr;
    }

    // Returns default response
    auto updatedValue = user_hal_helper::toVehiclePropValue(
            *mValuePool, SwitchUserResponse{
                                 .requestId = *requestId,
                                 .messageType = SwitchUserMessageType::VEHICLE_RESPONSE,
                                 .status = SwitchUserStatus::SUCCESS,
                         });
    ALOGI("no lshal response; replying with VEHICLE_RESPONSE / SUCCESS: %s",
          updatedValue->toString().c_str());
    return updatedValue;
}

FakeUserHal::ValueResultType FakeUserHal::onSetCreateUserResponse(const VehiclePropValue& value) {
    std::scoped_lock<std::mutex> lockGuard(mLock);

    auto requestId = getRequestId(value);
    if (!requestId.ok()) {
        ALOGE("Failed to get requestId on set(CREATE_USER): %s",
              requestId.error().message().c_str());
        return requestId.error();
    }

    if (value.areaId != 0) {
        ALOGD("set(CREATE_USER) called from lshal; storing it: %s", value.toString().c_str());
        mCreateUserResponseFromCmd = mValuePool->obtain(value);
        return nullptr;
    }
    ALOGD("set(CREATE_USER) called from Android: %s", value.toString().c_str());

    if (mCreateUserResponseFromCmd != nullptr) {
        ALOGI("replying CREATE_USER with lshal value:  %s",
              mCreateUserResponseFromCmd->toString().c_str());
        return sendUserHalResponse(std::move(mCreateUserResponseFromCmd), *requestId);
    }

    // Returns default response
    auto updatedValue = user_hal_helper::toVehiclePropValue(
            *mValuePool, CreateUserResponse{
                                 .requestId = *requestId,
                                 .status = CreateUserStatus::SUCCESS,
                         });
    ALOGI("no lshal response; replying with SUCCESS: %s", updatedValue->toString().c_str());
    return updatedValue;
}

FakeUserHal::ValueResultType FakeUserHal::onSetUserIdentificationAssociation(
        const VehiclePropValue& value) {
    std::scoped_lock<std::mutex> lockGuard(mLock);

    auto requestId = getRequestId(value);
    if (!requestId.ok()) {
        ALOGE("Failed to get requestId on set(USER_IDENTIFICATION_ASSOCIATION): %s",
              requestId.error().message().c_str());
        return requestId.error();
    }

    if (value.areaId != 0) {
        ALOGD("set(USER_IDENTIFICATION_ASSOCIATION) called from lshal; storing it: %s",
              value.toString().c_str());
        mSetUserIdentificationAssociationResponseFromCmd = mValuePool->obtain(value);
        return nullptr;
    }
    ALOGD("set(USER_IDENTIFICATION_ASSOCIATION) called from Android: %s", value.toString().c_str());

    if (mSetUserIdentificationAssociationResponseFromCmd != nullptr) {
        ALOGI("replying USER_IDENTIFICATION_ASSOCIATION with lshal value:  %s",
              mSetUserIdentificationAssociationResponseFromCmd->toString().c_str());
        // Not moving response so it can be used on GET requests
        auto copy = mValuePool->obtain(*mSetUserIdentificationAssociationResponseFromCmd);
        return sendUserHalResponse(std::move(copy), *requestId);
    }
    // Returns default response
    return defaultUserIdentificationAssociation(value);
}

FakeUserHal::ValueResultType FakeUserHal::defaultUserIdentificationAssociation(
        const VehiclePropValue& request) {
    // TODO(b/159498909): return a response with NOT_ASSOCIATED_ANY_USER for all requested types
    ALOGE("no lshal response for %s; replying with NOT_AVAILABLE", request.toString().c_str());
    return StatusError(StatusCode::NOT_AVAILABLE) << "not set by lshal";
}

FakeUserHal::ValueResultType FakeUserHal::sendUserHalResponse(
        VehiclePropValuePool::RecyclableType response, int32_t requestId) {
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
                  response->toString().c_str());
            return StatusError(StatusCode::NOT_AVAILABLE)
                   << "not generating a property change event because of lshal prop: "
                   << response->toString();
        default:
            ALOGE("invalid action on lshal response: %s", response->toString().c_str());
            return StatusError(StatusCode::INTERNAL_ERROR)
                   << "invalid action on lshal response: " << response->toString();
    }

    // Update area ID to 0 since this is a global property (and the area ID was only set to emulate
    // the request id behavior).
    response->areaId = 0;
    ALOGD("updating property to: %s", response->toString().c_str());
    return response;
}

std::string FakeUserHal::showDumpHelp() const {
    return fmt::format("{}: dumps state used for user management\n", kUserHalDumpOption);
}

std::string FakeUserHal::dump() const {
    std::scoped_lock<std::mutex> lockGuard(mLock);

    std::string info;
    if (mInitialUserResponseFromCmd != nullptr) {
        info += fmt::format("InitialUserInfo response: {}\n",
                            mInitialUserResponseFromCmd->toString());
    } else {
        info += "No InitialUserInfo response\n";
    }
    if (mSwitchUserResponseFromCmd != nullptr) {
        info += fmt::format("SwitchUser response: {}\n", mSwitchUserResponseFromCmd->toString());
    } else {
        info += "No SwitchUser response\n";
    }
    if (mCreateUserResponseFromCmd != nullptr) {
        info += fmt::format("CreateUser response: {}\n", mCreateUserResponseFromCmd->toString());
    } else {
        info += "No CreateUser response\n";
    }
    if (mSetUserIdentificationAssociationResponseFromCmd != nullptr) {
        info += fmt::format("SetUserIdentificationAssociation response: {}\n",
                            mSetUserIdentificationAssociationResponseFromCmd->toString());
    } else {
        info += "No SetUserIdentificationAssociation response\n";
    }
    return info;
}

}  // namespace fake
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
