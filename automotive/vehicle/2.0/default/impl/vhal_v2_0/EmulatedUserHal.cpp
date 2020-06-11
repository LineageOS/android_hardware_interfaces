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

#include <cutils/log.h>
#include <utils/SystemClock.h>

#include "EmulatedUserHal.h"

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace V2_0 {

namespace impl {

constexpr int INITIAL_USER_INFO = static_cast<int>(VehicleProperty::INITIAL_USER_INFO);
constexpr int SWITCH_USER = static_cast<int>(VehicleProperty::SWITCH_USER);
constexpr int CREATE_USER = static_cast<int>(VehicleProperty::CREATE_USER);
constexpr int REMOVE_USER = static_cast<int>(VehicleProperty::REMOVE_USER);
constexpr int USER_IDENTIFICATION_ASSOCIATION =
        static_cast<int>(VehicleProperty::USER_IDENTIFICATION_ASSOCIATION);

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

android::base::Result<std::unique_ptr<VehiclePropValue>> EmulatedUserHal::onSetProperty(
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
            return android::base::Error((int)StatusCode::INVALID_ARG)
                   << "Unsupported property: " << toString(value);
    }
}

android::base::Result<std::unique_ptr<VehiclePropValue>> EmulatedUserHal::onGetProperty(
        int32_t prop) {
    ALOGV("onGetProperty(%d)", prop);
    switch (prop) {
        case INITIAL_USER_INFO:
        case SWITCH_USER:
        case CREATE_USER:
        case REMOVE_USER:
            ALOGE("onGetProperty(): %d is only supported on SET", prop);
            return android::base::Error(static_cast<int>(StatusCode::INVALID_ARG))
                   << "only supported on SET";
        case USER_IDENTIFICATION_ASSOCIATION:
            if (mSetUserIdentificationAssociationResponseFromCmd != nullptr) {
                ALOGI("onGetProperty(%d): returning %s", prop,
                      toString(*mSetUserIdentificationAssociationResponseFromCmd).c_str());
                auto value = std::unique_ptr<VehiclePropValue>(
                        new VehiclePropValue(*mSetUserIdentificationAssociationResponseFromCmd));
                return value;
            }
            ALOGE("onGetProperty(%d): USER_IDENTIFICATION_ASSOCIATION not set by lshal", prop);
            return android::base::Error(static_cast<int>(StatusCode::NOT_AVAILABLE))
                   << "not set by lshal";
        default:
            ALOGE("onGetProperty(): %d is not supported", prop);
            return android::base::Error(static_cast<int>(StatusCode::INVALID_ARG))
                   << "not supported by User HAL";
    }
}

android::base::Result<std::unique_ptr<VehiclePropValue>>
EmulatedUserHal::onSetInitialUserInfoResponse(const VehiclePropValue& value) {
    if (value.value.int32Values.size() == 0) {
        ALOGE("set(INITIAL_USER_INFO): no int32values, ignoring it: %s", toString(value).c_str());
        return android::base::Error((int)StatusCode::INVALID_ARG)
               << "no int32values on " << toString(value);
    }

    if (value.areaId != 0) {
        ALOGD("set(INITIAL_USER_INFO) called from lshal; storing it: %s", toString(value).c_str());
        mInitialUserResponseFromCmd.reset(new VehiclePropValue(value));
        return {};
    }

    ALOGD("set(INITIAL_USER_INFO) called from Android: %s", toString(value).c_str());

    int32_t requestId = value.value.int32Values[0];
    if (mInitialUserResponseFromCmd != nullptr) {
        ALOGI("replying INITIAL_USER_INFO with lshal value:  %s",
              toString(*mInitialUserResponseFromCmd).c_str());
        return sendUserHalResponse(std::move(mInitialUserResponseFromCmd), requestId);
    }

    // Returns default response
    auto updatedValue = std::unique_ptr<VehiclePropValue>(new VehiclePropValue);
    updatedValue->prop = INITIAL_USER_INFO;
    updatedValue->timestamp = elapsedRealtimeNano();
    updatedValue->value.int32Values.resize(2);
    updatedValue->value.int32Values[0] = requestId;
    updatedValue->value.int32Values[1] = (int32_t)InitialUserInfoResponseAction::DEFAULT;

    ALOGI("no lshal response; replying with InitialUserInfoResponseAction::DEFAULT: %s",
          toString(*updatedValue).c_str());

    return updatedValue;
}

android::base::Result<std::unique_ptr<VehiclePropValue>> EmulatedUserHal::onSetSwitchUserResponse(
        const VehiclePropValue& value) {
    if (value.value.int32Values.size() == 0) {
        ALOGE("set(SWITCH_USER): no int32values, ignoring it: %s", toString(value).c_str());
        return android::base::Error((int)StatusCode::INVALID_ARG)
               << "no int32values on " << toString(value);
    }

    if (value.areaId != 0) {
        ALOGD("set(SWITCH_USER) called from lshal; storing it: %s", toString(value).c_str());
        mSwitchUserResponseFromCmd.reset(new VehiclePropValue(value));
        return {};
    }
    ALOGD("set(SWITCH_USER) called from Android: %s", toString(value).c_str());

    int32_t requestId = value.value.int32Values[0];
    if (mSwitchUserResponseFromCmd != nullptr) {
        ALOGI("replying SWITCH_USER with lshal value:  %s",
              toString(*mSwitchUserResponseFromCmd).c_str());
        return sendUserHalResponse(std::move(mSwitchUserResponseFromCmd), requestId);
    }

    if (value.value.int32Values.size() > 1) {
        auto messageType = static_cast<SwitchUserMessageType>(value.value.int32Values[1]);
        switch (messageType) {
            case SwitchUserMessageType::LEGACY_ANDROID_SWITCH:
                ALOGI("request is LEGACY_ANDROID_SWITCH; ignoring it");
                return {};
            case SwitchUserMessageType::ANDROID_POST_SWITCH:
                ALOGI("request is ANDROID_POST_SWITCH; ignoring it");
                return {};
            default:
                break;
        }
    }

    // Returns default response
    auto updatedValue = std::unique_ptr<VehiclePropValue>(new VehiclePropValue);
    updatedValue->prop = SWITCH_USER;
    updatedValue->timestamp = elapsedRealtimeNano();
    updatedValue->value.int32Values.resize(3);
    updatedValue->value.int32Values[0] = requestId;
    updatedValue->value.int32Values[1] = (int32_t)SwitchUserMessageType::VEHICLE_RESPONSE;
    updatedValue->value.int32Values[2] = (int32_t)SwitchUserStatus::SUCCESS;

    ALOGI("no lshal response; replying with VEHICLE_RESPONSE / SUCCESS: %s",
          toString(*updatedValue).c_str());

    return updatedValue;
}

android::base::Result<std::unique_ptr<VehiclePropValue>> EmulatedUserHal::onSetCreateUserResponse(
        const VehiclePropValue& value) {
    if (value.value.int32Values.size() == 0) {
        ALOGE("set(CREATE_USER): no int32values, ignoring it: %s", toString(value).c_str());
        return android::base::Error(static_cast<int>(StatusCode::INVALID_ARG))
               << "no int32values on " << toString(value);
    }

    if (value.areaId != 0) {
        ALOGD("set(CREATE_USER) called from lshal; storing it: %s", toString(value).c_str());
        mCreateUserResponseFromCmd.reset(new VehiclePropValue(value));
        return {};
    }
    ALOGD("set(CREATE_USER) called from Android: %s", toString(value).c_str());

    int32_t requestId = value.value.int32Values[0];
    if (mCreateUserResponseFromCmd != nullptr) {
        ALOGI("replying CREATE_USER with lshal value:  %s",
              toString(*mCreateUserResponseFromCmd).c_str());
        return sendUserHalResponse(std::move(mCreateUserResponseFromCmd), requestId);
    }

    // Returns default response
    auto updatedValue = std::unique_ptr<VehiclePropValue>(new VehiclePropValue);
    updatedValue->prop = CREATE_USER;
    updatedValue->timestamp = elapsedRealtimeNano();
    updatedValue->value.int32Values.resize(2);
    updatedValue->value.int32Values[0] = requestId;
    updatedValue->value.int32Values[1] = (int32_t)CreateUserStatus::SUCCESS;

    ALOGI("no lshal response; replying with SUCCESS: %s", toString(*updatedValue).c_str());

    return updatedValue;
}

android::base::Result<std::unique_ptr<VehiclePropValue>>
EmulatedUserHal::onSetUserIdentificationAssociation(const VehiclePropValue& value) {
    if (value.value.int32Values.size() == 0) {
        ALOGE("set(USER_IDENTIFICATION_ASSOCIATION): no int32values, ignoring it: %s",
              toString(value).c_str());
        return android::base::Error(static_cast<int>(StatusCode::INVALID_ARG))
               << "no int32values on " << toString(value);
    }

    if (value.areaId != 0) {
        ALOGD("set(USER_IDENTIFICATION_ASSOCIATION) called from lshal; storing it: %s",
              toString(value).c_str());
        mSetUserIdentificationAssociationResponseFromCmd.reset(new VehiclePropValue(value));
        return {};
    }
    ALOGD("set(USER_IDENTIFICATION_ASSOCIATION) called from Android: %s", toString(value).c_str());

    int32_t requestId = value.value.int32Values[0];
    if (mSetUserIdentificationAssociationResponseFromCmd != nullptr) {
        ALOGI("replying USER_IDENTIFICATION_ASSOCIATION with lshal value:  %s",
              toString(*mSetUserIdentificationAssociationResponseFromCmd).c_str());
        // Not moving response so it can be used on GET requests
        auto copy = std::unique_ptr<VehiclePropValue>(
                new VehiclePropValue(*mSetUserIdentificationAssociationResponseFromCmd));
        return sendUserHalResponse(std::move(copy), requestId);
    }

    // Returns default response
    auto updatedValue = std::unique_ptr<VehiclePropValue>(new VehiclePropValue);
    updatedValue->prop = USER_IDENTIFICATION_ASSOCIATION;
    updatedValue->timestamp = elapsedRealtimeNano();
    updatedValue->value.int32Values.resize(1);
    updatedValue->value.int32Values[0] = requestId;
    updatedValue->value.stringValue = "Response not set by LSHAL";

    ALOGI("no lshal response; replying with an error message: %s", toString(*updatedValue).c_str());

    return updatedValue;
}

android::base::Result<std::unique_ptr<VehiclePropValue>> EmulatedUserHal::sendUserHalResponse(
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
            return android::base::Error((int)StatusCode::NOT_AVAILABLE)
                   << "not generating a property change event because of lshal prop: "
                   << toString(*response);
        default:
            ALOGE("invalid action on lshal response: %s", toString(*response).c_str());
            return android::base::Error((int)StatusCode::INTERNAL_ERROR)
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
