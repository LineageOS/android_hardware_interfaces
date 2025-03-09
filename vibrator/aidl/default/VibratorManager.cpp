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

#include "vibrator-impl/VibratorManager.h"
#include "vibrator-impl/VibrationSession.h"

#include <aidl/android/hardware/vibrator/BnVibratorCallback.h>

#include <android-base/logging.h>
#include <thread>

namespace aidl {
namespace android {
namespace hardware {
namespace vibrator {

static constexpr int32_t kDefaultVibratorId = 1;

class VibratorCallback : public BnVibratorCallback {
  public:
    VibratorCallback(const std::function<void()>& callback) : mCallback(callback) {}
    ndk::ScopedAStatus onComplete() override {
        mCallback();
        return ndk::ScopedAStatus::ok();
    }

  private:
    std::function<void()> mCallback;
};

ndk::ScopedAStatus VibratorManager::getCapabilities(int32_t* _aidl_return) {
    LOG(VERBOSE) << "Vibrator manager reporting capabilities";
    std::lock_guard lock(mMutex);
    if (mCapabilities == 0) {
        int32_t version;
        if (!getInterfaceVersion(&version).isOk()) {
            return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_ILLEGAL_STATE));
        }
        mCapabilities = IVibratorManager::CAP_SYNC | IVibratorManager::CAP_PREPARE_ON |
                        IVibratorManager::CAP_PREPARE_PERFORM |
                        IVibratorManager::CAP_PREPARE_COMPOSE |
                        IVibratorManager::CAP_MIXED_TRIGGER_ON |
                        IVibratorManager::CAP_MIXED_TRIGGER_PERFORM |
                        IVibratorManager::CAP_MIXED_TRIGGER_COMPOSE |
                        IVibratorManager::CAP_TRIGGER_CALLBACK;

        if (version >= 3) {
            mCapabilities |= IVibratorManager::CAP_START_SESSIONS;
        }
    }

    *_aidl_return = mCapabilities;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus VibratorManager::getVibratorIds(std::vector<int32_t>* _aidl_return) {
    LOG(VERBOSE) << "Vibrator manager getting vibrator ids";
    *_aidl_return = {kDefaultVibratorId};
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus VibratorManager::getVibrator(int32_t vibratorId,
                                                std::shared_ptr<IVibrator>* _aidl_return) {
    LOG(VERBOSE) << "Vibrator manager getting vibrator " << vibratorId;
    if (vibratorId == kDefaultVibratorId) {
        *_aidl_return = mDefaultVibrator;
        return ndk::ScopedAStatus::ok();
    } else {
        *_aidl_return = nullptr;
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
}

ndk::ScopedAStatus VibratorManager::prepareSynced(const std::vector<int32_t>& vibratorIds) {
    LOG(VERBOSE) << "Vibrator Manager prepare synced";
    if (vibratorIds.size() != 1 || vibratorIds[0] != kDefaultVibratorId) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
    std::lock_guard lock(mMutex);
    if (mIsPreparing) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    mIsPreparing = true;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus VibratorManager::triggerSynced(
        const std::shared_ptr<IVibratorCallback>& callback) {
    LOG(VERBOSE) << "Vibrator Manager trigger synced";
    std::lock_guard lock(mMutex);
    if (!mIsPreparing) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    std::thread([callback] {
        if (callback != nullptr) {
            LOG(VERBOSE) << "Notifying perform complete";
            callback->onComplete();
        }
    }).detach();
    mIsPreparing = false;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus VibratorManager::cancelSynced() {
    LOG(VERBOSE) << "Vibrator Manager cancel synced";
    std::lock_guard lock(mMutex);
    mIsPreparing = false;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus VibratorManager::startSession(const std::vector<int32_t>& vibratorIds,
                                                 const VibrationSessionConfig&,
                                                 const std::shared_ptr<IVibratorCallback>& callback,
                                                 std::shared_ptr<IVibrationSession>* _aidl_return) {
    LOG(VERBOSE) << "Vibrator Manager start session";
    *_aidl_return = nullptr;
    int32_t capabilities = 0;
    if (!getCapabilities(&capabilities).isOk()) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    if ((capabilities & IVibratorManager::CAP_START_SESSIONS) == 0) {
        return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
    }
    if (vibratorIds.size() != 1 || vibratorIds[0] != kDefaultVibratorId) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
    std::lock_guard lock(mMutex);
    if (mIsPreparing || mSession) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    mSessionCallback = callback;
    mSession = ndk::SharedRefBase::make<VibrationSession>(this->ref<VibratorManager>());
    *_aidl_return = static_cast<std::shared_ptr<IVibrationSession>>(mSession);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus VibratorManager::clearSessions() {
    LOG(VERBOSE) << "Vibrator Manager clear sessions";
    abortSession();
    return ndk::ScopedAStatus::ok();
}

void VibratorManager::abortSession() {
    std::shared_ptr<IVibrationSession> session;
    {
        std::lock_guard lock(mMutex);
        session = mSession;
    }
    if (session) {
        mDefaultVibrator->off();
        clearSession(session);
    }
}

void VibratorManager::closeSession(int32_t delayMs) {
    std::shared_ptr<IVibrationSession> session;
    {
        std::lock_guard lock(mMutex);
        if (mIsClosingSession) {
            // Already closing session, ignore this.
            return;
        }
        session = mSession;
        mIsClosingSession = true;
    }
    if (session) {
        auto callback = ndk::SharedRefBase::make<VibratorCallback>(
                [session, delayMs, sharedThis = this->ref<VibratorManager>()] {
                    LOG(VERBOSE) << "Closing session after vibrator became idle";
                    usleep(delayMs * 1000);

                    if (sharedThis) {
                        sharedThis->clearSession(session);
                    }
                });
        mDefaultVibrator->setGlobalVibrationCallback(callback);
    }
}

void VibratorManager::clearSession(const std::shared_ptr<IVibrationSession>& session) {
    std::lock_guard lock(mMutex);
    if (mSession != session) {
        // Probably a delayed call from an old session that was already cleared, ignore it.
        return;
    }
    std::shared_ptr<IVibratorCallback> callback = mSessionCallback;
    mSession = nullptr;
    mSessionCallback = nullptr;  // make sure any delayed call will not trigger this again.
    mIsClosingSession = false;
    if (callback) {
        std::thread([callback] {
            LOG(VERBOSE) << "Notifying session complete";
            if (!callback->onComplete().isOk()) {
                LOG(ERROR) << "Failed to call onComplete";
            }
        }).detach();
    }
}

}  // namespace vibrator
}  // namespace hardware
}  // namespace android
}  // namespace aidl
