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

#include "Fingerprint.h"
#include "Session.h"

namespace aidl::android::hardware::biometrics::fingerprint {

const int kSensorId = 0;
const common::SensorStrength kSensorStrength = common::SensorStrength::STRONG;
const int kMaxEnrollmentsPerUser = 5;
const FingerprintSensorType kSensorType = FingerprintSensorType::REAR;

ndk::ScopedAStatus Fingerprint::getSensorProps(std::vector<SensorProps>* return_val) {
    *return_val = std::vector<SensorProps>();
    common::CommonProps commonProps = {kSensorId,
            kSensorStrength,
            kMaxEnrollmentsPerUser};
    SensorProps props = {commonProps,
            kSensorType};
    return_val->push_back(props);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Fingerprint::createSession(int32_t /*sensorId*/, int32_t /*userId*/,
                                              const std::shared_ptr<ISessionCallback>& cb,
                                              std::shared_ptr<ISession>* return_val) {
    *return_val = SharedRefBase::make<Session>(cb);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Fingerprint::setResetLockoutCallback(
        const std::shared_ptr<IResetLockoutCallback>& /*cb*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Fingerprint::generateChallenge(
        int32_t /*sensorId*/, int32_t /*userId*/, int32_t /*timeoutSec*/,
        const std::shared_ptr<IGenerateChallengeCallback>& /*cb*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Fingerprint::revokeChallenge(
        int32_t /*sensorId*/, int32_t /*userId*/,
        const std::shared_ptr<IRevokeChallengeCallback>& /*cb*/) {
    return ndk::ScopedAStatus::ok();
}

}  // namespace aidl::android::hardware::biometrics::fingerprint
