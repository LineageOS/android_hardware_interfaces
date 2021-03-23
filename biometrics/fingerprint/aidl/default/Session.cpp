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

#include "Session.h"

#include <android-base/logging.h>

#include "CancellationSignal.h"

namespace aidl::android::hardware::biometrics::fingerprint {

Session::Session(int sensorId, int userId, std::shared_ptr<ISessionCallback> cb,
                 FakeFingerprintEngine* engine, WorkerThread* worker)
    : mSensorId(sensorId),
      mUserId(userId),
      mCb(std::move(cb)),
      mEngine(engine),
      mWorker(worker),
      mScheduledState(SessionState::IDLING),
      mCurrentState(SessionState::IDLING) {
    CHECK_GE(mSensorId, 0);
    CHECK_GE(mUserId, 0);
    CHECK(mEngine);
    CHECK(mWorker);
    CHECK(mCb);
}

void Session::scheduleStateOrCrash(SessionState state) {
    CHECK(mScheduledState == SessionState::IDLING);
    CHECK(mCurrentState == SessionState::IDLING);
    mScheduledState = state;
}

void Session::enterStateOrCrash(int cookie, SessionState state) {
    CHECK(mScheduledState == state);
    mCurrentState = state;
    mScheduledState = SessionState::IDLING;
    mCb->onStateChanged(cookie, mCurrentState);
}

void Session::enterIdling(int cookie) {
    mCurrentState = SessionState::IDLING;
    mCb->onStateChanged(cookie, mCurrentState);
}

bool Session::isClosed() {
    return mCurrentState == SessionState::CLOSED;
}

ndk::ScopedAStatus Session::generateChallenge(int32_t cookie) {
    LOG(INFO) << "generateChallenge";
    scheduleStateOrCrash(SessionState::GENERATING_CHALLENGE);

    mWorker->schedule(Callable::from([this, cookie] {
        enterStateOrCrash(cookie, SessionState::GENERATING_CHALLENGE);
        mEngine->generateChallengeImpl(mCb.get());
        enterIdling(cookie);
    }));

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::revokeChallenge(int32_t cookie, int64_t challenge) {
    LOG(INFO) << "revokeChallenge";
    scheduleStateOrCrash(SessionState::REVOKING_CHALLENGE);

    mWorker->schedule(Callable::from([this, cookie, challenge] {
        enterStateOrCrash(cookie, SessionState::REVOKING_CHALLENGE);
        mEngine->revokeChallengeImpl(mCb.get(), challenge);
        enterIdling(cookie);
    }));

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::enroll(int32_t cookie, const keymaster::HardwareAuthToken& hat,
                                   std::shared_ptr<common::ICancellationSignal>* out) {
    LOG(INFO) << "enroll";
    scheduleStateOrCrash(SessionState::ENROLLING);

    std::promise<void> cancellationPromise;
    auto cancFuture = cancellationPromise.get_future();

    mWorker->schedule(Callable::from([this, cookie, hat, cancFuture = std::move(cancFuture)] {
        enterStateOrCrash(cookie, SessionState::ENROLLING);
        if (shouldCancel(cancFuture)) {
            mCb->onError(Error::CANCELED, 0 /* vendorCode */);
        } else {
            mEngine->enrollImpl(mCb.get(), hat);
        }
        enterIdling(cookie);
    }));

    *out = SharedRefBase::make<CancellationSignal>(std::move(cancellationPromise));
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::authenticate(int32_t cookie, int64_t operationId,
                                         std::shared_ptr<common::ICancellationSignal>* out) {
    LOG(INFO) << "authenticate";
    scheduleStateOrCrash(SessionState::AUTHENTICATING);

    std::promise<void> cancPromise;
    auto cancFuture = cancPromise.get_future();

    mWorker->schedule(
            Callable::from([this, cookie, operationId, cancFuture = std::move(cancFuture)] {
                enterStateOrCrash(cookie, SessionState::AUTHENTICATING);
                if (shouldCancel(cancFuture)) {
                    mCb->onError(Error::CANCELED, 0 /* vendorCode */);
                } else {
                    mEngine->authenticateImpl(mCb.get(), operationId);
                }
                enterIdling(cookie);
            }));

    *out = SharedRefBase::make<CancellationSignal>(std::move(cancPromise));
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::detectInteraction(int32_t cookie,
                                              std::shared_ptr<common::ICancellationSignal>* out) {
    LOG(INFO) << "detectInteraction";
    scheduleStateOrCrash(SessionState::DETECTING_INTERACTION);

    std::promise<void> cancellationPromise;
    auto cancFuture = cancellationPromise.get_future();

    mWorker->schedule(Callable::from([this, cookie, cancFuture = std::move(cancFuture)] {
        enterStateOrCrash(cookie, SessionState::DETECTING_INTERACTION);
        if (shouldCancel(cancFuture)) {
            mCb->onError(Error::CANCELED, 0 /* vendorCode */);
        } else {
            mEngine->detectInteractionImpl(mCb.get());
        }
        enterIdling(cookie);
    }));

    *out = SharedRefBase::make<CancellationSignal>(std::move(cancellationPromise));
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::enumerateEnrollments(int32_t cookie) {
    LOG(INFO) << "enumerateEnrollments";
    scheduleStateOrCrash(SessionState::ENUMERATING_ENROLLMENTS);

    mWorker->schedule(Callable::from([this, cookie] {
        enterStateOrCrash(cookie, SessionState::ENUMERATING_ENROLLMENTS);
        mEngine->enumerateEnrollmentsImpl(mCb.get());
        enterIdling(cookie);
    }));

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::removeEnrollments(int32_t cookie,
                                              const std::vector<int32_t>& enrollmentIds) {
    LOG(INFO) << "removeEnrollments";
    scheduleStateOrCrash(SessionState::REMOVING_ENROLLMENTS);

    mWorker->schedule(Callable::from([this, cookie, enrollmentIds] {
        enterStateOrCrash(cookie, SessionState::REMOVING_ENROLLMENTS);
        mEngine->removeEnrollmentsImpl(mCb.get(), enrollmentIds);
        enterIdling(cookie);
    }));

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::getAuthenticatorId(int32_t cookie) {
    LOG(INFO) << "getAuthenticatorId";
    scheduleStateOrCrash(SessionState::GETTING_AUTHENTICATOR_ID);

    mWorker->schedule(Callable::from([this, cookie] {
        enterStateOrCrash(cookie, SessionState::GETTING_AUTHENTICATOR_ID);
        mEngine->getAuthenticatorIdImpl(mCb.get());
        enterIdling(cookie);
    }));

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::invalidateAuthenticatorId(int32_t cookie) {
    LOG(INFO) << "invalidateAuthenticatorId";
    scheduleStateOrCrash(SessionState::INVALIDATING_AUTHENTICATOR_ID);

    mWorker->schedule(Callable::from([this, cookie] {
        enterStateOrCrash(cookie, SessionState::INVALIDATING_AUTHENTICATOR_ID);
        mEngine->invalidateAuthenticatorIdImpl(mCb.get());
        enterIdling(cookie);
    }));

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::resetLockout(int32_t cookie, const keymaster::HardwareAuthToken& hat) {
    LOG(INFO) << "resetLockout";
    scheduleStateOrCrash(SessionState::RESETTING_LOCKOUT);

    mWorker->schedule(Callable::from([this, cookie, hat] {
        enterStateOrCrash(cookie, SessionState::RESETTING_LOCKOUT);
        mEngine->resetLockoutImpl(mCb.get(), hat);
        enterIdling(cookie);
    }));

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::close(int32_t /*cookie*/) {
    LOG(INFO) << "close";
    CHECK(mCurrentState == SessionState::IDLING) << "Can't close a non-idling session. Crashing.";
    mCurrentState = SessionState::CLOSED;
    mCb->onSessionClosed();
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::onPointerDown(int32_t /*pointerId*/, int32_t /*x*/, int32_t /*y*/,
                                          float /*minor*/, float /*major*/) {
    LOG(INFO) << "onPointerDown";
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::onPointerUp(int32_t /*pointerId*/) {
    LOG(INFO) << "onPointerUp";
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::onUiReady() {
    LOG(INFO) << "onUiReady";
    return ndk::ScopedAStatus::ok();
}

}  // namespace aidl::android::hardware::biometrics::fingerprint
