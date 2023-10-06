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

#include "FakeFingerprintEngineUdfps.h"

#include <android-base/logging.h>

#include <fingerprint.sysprop.h>

#include "util/CancellationSignal.h"
#include "util/Util.h"

#undef LOG_TAG
#define LOG_TAG "FingerprintVirtualHalUdfps"

using namespace ::android::fingerprint::virt;

namespace aidl::android::hardware::biometrics::fingerprint {

FakeFingerprintEngineUdfps::FakeFingerprintEngineUdfps()
    : FakeFingerprintEngine(), mWorkMode(WorkMode::kIdle), mPointerDownTime(0), mUiReadyTime(0) {}

SensorLocation FakeFingerprintEngineUdfps::defaultSensorLocation() {
    return {0 /* displayId (not used) */, defaultSensorLocationX /* sensorLocationX */,
            defaultSensorLocationY /* sensorLocationY */, defaultSensorRadius /* sensorRadius */,
            "" /* display */};
}

ndk::ScopedAStatus FakeFingerprintEngineUdfps::onPointerDownImpl(int32_t /*pointerId*/,
                                                                 int32_t /*x*/, int32_t /*y*/,
                                                                 float /*minor*/, float /*major*/) {
    BEGIN_OP(0);
    // verify whetehr touch coordinates/area matching sensor location ?
    mPointerDownTime = Util::getSystemNanoTime();
    if (FingerprintHalProperties::control_illumination().value_or(false)) {
        fingerDownAction();
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus FakeFingerprintEngineUdfps::onPointerUpImpl(int32_t /*pointerId*/) {
    BEGIN_OP(0);
    mUiReadyTime = 0;
    mPointerDownTime = 0;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus FakeFingerprintEngineUdfps::onUiReadyImpl() {
    BEGIN_OP(0);

    if (Util::hasElapsed(mPointerDownTime, uiReadyTimeoutInMs * 100)) {
        LOG(ERROR) << "onUiReady() arrives too late after onPointerDown()";
    } else {
        fingerDownAction();
    }
    return ndk::ScopedAStatus::ok();
}

void FakeFingerprintEngineUdfps::fingerDownAction() {
    switch (mWorkMode) {
        case WorkMode::kAuthenticate:
            onAuthenticateFingerDown();
            break;
        case WorkMode::kEnroll:
            onEnrollFingerDown();
            break;
        case WorkMode::kDetectInteract:
            onDetectInteractFingerDown();
            break;
        default:
            LOG(WARNING) << "unexpected call: onUiReady()";
            break;
    }

    mUiReadyTime = 0;
    mPointerDownTime = 0;
}

void FakeFingerprintEngineUdfps::onAuthenticateFingerDown() {
    FakeFingerprintEngine::authenticateImpl(mCb, mOperationId, mCancelVec[0]);
}

void FakeFingerprintEngineUdfps::onEnrollFingerDown() {
    // Any use case to emulate display touch for each capture during enrollment?
    FakeFingerprintEngine::enrollImpl(mCb, mHat, mCancelVec[0]);
}

void FakeFingerprintEngineUdfps::onDetectInteractFingerDown() {
    FakeFingerprintEngine::detectInteractionImpl(mCb, mCancelVec[0]);
}

void FakeFingerprintEngineUdfps::enrollImpl(ISessionCallback* cb,
                                            const keymaster::HardwareAuthToken& hat,
                                            const std::future<void>& cancel) {
    updateContext(WorkMode::kEnroll, cb, const_cast<std::future<void>&>(cancel), 0, hat);
}

void FakeFingerprintEngineUdfps::authenticateImpl(ISessionCallback* cb, int64_t operationId,
                                                  const std::future<void>& cancel) {
    updateContext(WorkMode::kAuthenticate, cb, const_cast<std::future<void>&>(cancel), operationId,
                  keymaster::HardwareAuthToken());
}

void FakeFingerprintEngineUdfps::detectInteractionImpl(ISessionCallback* cb,
                                                       const std::future<void>& cancel) {
    updateContext(WorkMode::kDetectInteract, cb, const_cast<std::future<void>&>(cancel), 0,
                  keymaster::HardwareAuthToken());
}

void FakeFingerprintEngineUdfps::updateContext(WorkMode mode, ISessionCallback* cb,
                                               std::future<void>& cancel, int64_t operationId,
                                               const keymaster::HardwareAuthToken& hat) {
    mPointerDownTime = 0;
    mUiReadyTime = 0;
    mCancelVec.clear();

    mCancelVec.push_back(std::move(cancel));
    mWorkMode = mode;
    mCb = cb;
    mOperationId = operationId;
    mHat = hat;
}

}  // namespace aidl::android::hardware::biometrics::fingerprint
