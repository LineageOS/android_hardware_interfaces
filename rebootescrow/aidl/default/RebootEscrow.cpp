/*
 * Copyright (C) 2019 The Android Open Source Project
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

#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/unique_fd.h>

#include "HadamardUtils.h"
#include "rebootescrow-impl/RebootEscrow.h"

namespace aidl {
namespace android {
namespace hardware {
namespace rebootescrow {

using ::android::base::unique_fd;

ndk::ScopedAStatus RebootEscrow::storeKey(const std::vector<int8_t>& kek) {
    int rawFd = TEMP_FAILURE_RETRY(::open(REBOOT_ESCROW_DEVICE, O_WRONLY | O_NOFOLLOW | O_CLOEXEC));
    unique_fd fd(rawFd);
    if (fd.get() < 0) {
        LOG(WARNING) << "Could not open reboot escrow device";
        return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
    }

    std::vector<uint8_t> ukek(kek.begin(), kek.end());
    auto encoded = hadamard::EncodeKey(ukek);

    if (!::android::base::WriteFully(fd, encoded.data(), encoded.size())) {
        LOG(WARNING) << "Could not write data fully to character device";
        return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
    }

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RebootEscrow::retrieveKey(std::vector<int8_t>* _aidl_return) {
    int rawFd = TEMP_FAILURE_RETRY(::open(REBOOT_ESCROW_DEVICE, O_RDONLY | O_NOFOLLOW | O_CLOEXEC));
    unique_fd fd(rawFd);
    if (fd.get() < 0) {
        LOG(WARNING) << "Could not open reboot escrow device";
        return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
    }

    std::string encodedString;
    if (!::android::base::ReadFdToString(fd, &encodedString)) {
        LOG(WARNING) << "Could not read device to string";
        return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
    }

    std::vector<uint8_t> encodedBytes(encodedString.begin(), encodedString.end());
    auto keyBytes = hadamard::DecodeKey(encodedBytes);

    std::vector<int8_t> signedKeyBytes(keyBytes.begin(), keyBytes.end());
    *_aidl_return = signedKeyBytes;
    return ndk::ScopedAStatus::ok();
}

}  // namespace rebootescrow
}  // namespace hardware
}  // namespace android
}  // namespace aidl
