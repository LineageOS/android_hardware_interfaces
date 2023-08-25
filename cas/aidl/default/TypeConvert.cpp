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

#define LOG_TAG "android.hardware.cas-TypeConvert"

#include <aidl/android/hardware/cas/Status.h>
#include <utils/Log.h>

#include "TypeConvert.h"

namespace aidl {
namespace android {
namespace hardware {
namespace cas {

ScopedAStatus toStatus(status_t legacyStatus) {
    int status;
    switch (legacyStatus) {
        case OK:
            return ndk::ScopedAStatus::ok();
        case ERROR_CAS_NO_LICENSE:
            status = Status::ERROR_CAS_NO_LICENSE;
            break;
        case ERROR_CAS_LICENSE_EXPIRED:
            status = Status::ERROR_CAS_LICENSE_EXPIRED;
            break;
        case ERROR_CAS_SESSION_NOT_OPENED:
            status = Status::ERROR_CAS_SESSION_NOT_OPENED;
            break;
        case ERROR_CAS_CANNOT_HANDLE:
            status = Status::ERROR_CAS_CANNOT_HANDLE;
            break;
        case ERROR_CAS_TAMPER_DETECTED:
            status = Status::ERROR_CAS_INVALID_STATE;
            break;
        case BAD_VALUE:
            status = Status::BAD_VALUE;
            break;
        case ERROR_CAS_NOT_PROVISIONED:
            status = Status::ERROR_CAS_NOT_PROVISIONED;
            break;
        case ERROR_CAS_RESOURCE_BUSY:
            status = Status::ERROR_CAS_RESOURCE_BUSY;
            break;
        case ERROR_CAS_INSUFFICIENT_OUTPUT_PROTECTION:
            status = Status::ERROR_CAS_INSUFFICIENT_OUTPUT_PROTECTION;
            break;
        case ERROR_CAS_DEVICE_REVOKED:
            status = Status::ERROR_CAS_DEVICE_REVOKED;
            break;
        case ERROR_CAS_DECRYPT_UNIT_NOT_INITIALIZED:
            status = Status::ERROR_CAS_DECRYPT_UNIT_NOT_INITIALIZED;
            break;
        case ERROR_CAS_DECRYPT:
            status = Status::ERROR_CAS_DECRYPT;
            break;
        case ERROR_CAS_NEED_ACTIVATION:
            status = Status::ERROR_CAS_NEED_ACTIVATION;
            break;
        case ERROR_CAS_NEED_PAIRING:
            status = Status::ERROR_CAS_NEED_PAIRING;
            break;
        case ERROR_CAS_NO_CARD:
            status = Status::ERROR_CAS_NO_CARD;
            break;
        case ERROR_CAS_CARD_MUTE:
            status = Status::ERROR_CAS_CARD_MUTE;
            break;
        case ERROR_CAS_CARD_INVALID:
            status = Status::ERROR_CAS_CARD_INVALID;
            break;
        case ERROR_CAS_BLACKOUT:
            status = Status::ERROR_CAS_BLACKOUT;
            break;
        default:
            ALOGW("Unable to convert legacy status: %d, defaulting to UNKNOWN", legacyStatus);
            status = Status::ERROR_CAS_UNKNOWN;
            break;
    }
    return ScopedAStatus::fromServiceSpecificError(status);
}

String8 sessionIdToString(const std::vector<uint8_t>& sessionId) {
    String8 result;
    for (auto it = sessionId.begin(); it != sessionId.end(); it++) {
        result.appendFormat("%02x ", *it);
    }
    if (result.empty()) {
        result.append("(null)");
    }
    return result;
}

}  // namespace cas
}  // namespace hardware
}  // namespace android
}  // namespace aidl
