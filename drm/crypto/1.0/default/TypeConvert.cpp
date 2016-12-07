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

#include "TypeConvert.h"

namespace android {
namespace hardware {
namespace drm {
namespace crypto {
namespace V1_0 {
namespace implementation {

Status toStatus(status_t legacyStatus) {
    Status status;
    switch(legacyStatus) {
    case android::ERROR_DRM_NO_LICENSE:
        status = Status::ERROR_DRM_NO_LICENSE;
        break;
    case android::ERROR_DRM_LICENSE_EXPIRED:
        status = Status::ERROR_DRM_LICENSE_EXPIRED;
        break;
    case android::ERROR_DRM_RESOURCE_BUSY:
        status = Status::ERROR_DRM_RESOURCE_BUSY;
        break;
    case android::ERROR_DRM_INSUFFICIENT_OUTPUT_PROTECTION:
        status = Status::ERROR_DRM_INSUFFICIENT_OUTPUT_PROTECTION;
        break;
    case android::ERROR_DRM_SESSION_NOT_OPENED:
        status = Status::ERROR_DRM_SESSION_NOT_OPENED;
        break;
    case android::ERROR_DRM_CANNOT_HANDLE:
    case android::BAD_VALUE:
        status = Status::ERROR_DRM_CANNOT_HANDLE;
        break;
    default:
        ALOGW("Unable to convert legacy status: %d, defaulting to UNKNOWN",
                legacyStatus);
        status = Status::ERROR_UNKNOWN_CRYPTO_EXCEPTION;
        break;
    }
    return status;
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace crypto
}  // namespace drm
}  // namespace hardware
}  // namespace android
