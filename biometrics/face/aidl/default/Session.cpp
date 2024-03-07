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

#include <android-base/logging.h>

#include "Session.h"

#undef LOG_TAG
#define LOG_TAG "FaceVirtualHalSession"

namespace aidl::android::hardware::biometrics::face {

constexpr size_t MAX_WORKER_QUEUE_SIZE = 5;

Session::Session(std::unique_ptr<FakeFaceEngine> engine, std::shared_ptr<ISessionCallback> cb)
    : mEngine(std::move(engine)), mCb(std::move(cb)), mRandom(std::mt19937::default_seed) {
    mThread = std::make_unique<WorkerThread>(MAX_WORKER_QUEUE_SIZE);
}

ndk::ScopedAStatus Session::generateChallenge() {
    LOG(INFO) << "generateChallenge";
    mThread->schedule(Callable::from([this] { mEngine->generateChallengeImpl(mCb.get()); }));
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::revokeChallenge(int64_t challenge) {
    LOG(INFO) << "revokeChallenge";
    mThread->schedule(Callable::from(
            [this, challenge] { mEngine->revokeChallengeImpl(mCb.get(), challenge); }));
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::getEnrollmentConfig(
        EnrollmentType /*enrollmentType*/, std::vector<EnrollmentStageConfig>* cancellationSignal) {
    *cancellationSignal = {};
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::enroll(
        const keymaster::HardwareAuthToken& hat, EnrollmentType enrollmentType,
        const std::vector<Feature>& features, const std::optional<NativeHandle>& /*previewSurface*/,
        std::shared_ptr<biometrics::common::ICancellationSignal>* cancellationSignal) {
    LOG(INFO) << "enroll";
    std::promise<void> cancellationPromise;
    auto cancFuture = cancellationPromise.get_future();

    mThread->schedule(Callable::from(
            [this, hat, enrollmentType, features, cancFuture = std::move(cancFuture)] {
                mEngine->enrollImpl(mCb.get(), hat, enrollmentType, features, cancFuture);
            }));

    *cancellationSignal = SharedRefBase::make<CancellationSignal>(std::move(cancellationPromise));
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::authenticate(
        int64_t keystoreOperationId,
        std::shared_ptr<common::ICancellationSignal>* cancellationSignal) {
    LOG(INFO) << "authenticate";
    std::promise<void> cancellationPromise;
    auto cancFuture = cancellationPromise.get_future();

    mThread->schedule(
            Callable::from([this, keystoreOperationId, cancFuture = std::move(cancFuture)] {
                mEngine->authenticateImpl(mCb.get(), keystoreOperationId, cancFuture);
            }));

    *cancellationSignal = SharedRefBase::make<CancellationSignal>(std::move(cancellationPromise));
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::detectInteraction(
        std::shared_ptr<common::ICancellationSignal>* cancellationSignal) {
    LOG(INFO) << "detectInteraction";
    std::promise<void> cancellationPromise;
    auto cancFuture = cancellationPromise.get_future();

    mThread->schedule(Callable::from([this, cancFuture = std::move(cancFuture)] {
        mEngine->detectInteractionImpl(mCb.get(), cancFuture);
    }));

    *cancellationSignal = SharedRefBase::make<CancellationSignal>(std::move(cancellationPromise));
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::enumerateEnrollments() {
    LOG(INFO) << "enumerateEnrollments";
    mThread->schedule(Callable::from([this] { mEngine->enumerateEnrollmentsImpl(mCb.get()); }));
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::removeEnrollments(const std::vector<int32_t>& enrollmentIds) {
    LOG(INFO) << "removeEnrollments";
    mThread->schedule(Callable::from(
            [this, enrollmentIds] { mEngine->removeEnrollmentsImpl(mCb.get(), enrollmentIds); }));
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::getFeatures() {
    LOG(INFO) << "getFeatures";
    mThread->schedule(Callable::from([this] { mEngine->getFeaturesImpl(mCb.get()); }));
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::setFeature(const keymaster::HardwareAuthToken& hat, Feature feature,
                                       bool enabled) {
    LOG(INFO) << "setFeature";
    mThread->schedule(Callable::from([this, hat, feature, enabled] {
        mEngine->setFeatureImpl(mCb.get(), hat, feature, enabled);
    }));
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::getAuthenticatorId() {
    LOG(INFO) << "getAuthenticatorId";
    mThread->schedule(Callable::from([this] { mEngine->getAuthenticatorIdImpl(mCb.get()); }));
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::invalidateAuthenticatorId() {
    LOG(INFO) << "invalidateAuthenticatorId";
    mThread->schedule(
            Callable::from([this] { mEngine->invalidateAuthenticatorIdImpl(mCb.get()); }));
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::resetLockout(const keymaster::HardwareAuthToken& hat) {
    LOG(INFO) << "resetLockout";
    mThread->schedule(Callable::from([this, hat] { mEngine->resetLockoutImpl(mCb.get(), hat); }));
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::close() {
    if (mCb) {
        mCb->onSessionClosed();
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::authenticateWithContext(
        int64_t operationId, const common::OperationContext& /*context*/,
        std::shared_ptr<common::ICancellationSignal>* out) {
    return authenticate(operationId, out);
}

ndk::ScopedAStatus Session::enrollWithContext(const keymaster::HardwareAuthToken& hat,
                                              EnrollmentType enrollmentType,
                                              const std::vector<Feature>& features,
                                              const std::optional<NativeHandle>& previewSurface,
                                              const common::OperationContext& /*context*/,
                                              std::shared_ptr<common::ICancellationSignal>* out) {
    return enroll(hat, enrollmentType, features, previewSurface, out);
}

ndk::ScopedAStatus Session::detectInteractionWithContext(
        const common::OperationContext& /*context*/,
        std::shared_ptr<common::ICancellationSignal>* out) {
    return detectInteraction(out);
}

ndk::ScopedAStatus Session::onContextChanged(const common::OperationContext& /*context*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::enrollWithOptions(const FaceEnrollOptions& options,
                                              std::shared_ptr<common::ICancellationSignal>* out) {
    return enroll(options.hardwareAuthToken, options.enrollmentType, options.features,
                  options.nativeHandlePreview, out);
}

}  // namespace aidl::android::hardware::biometrics::face
