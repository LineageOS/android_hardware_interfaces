/*
 * Copyright (C) 2024 The Android Open Source Project
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

#pragma once

#include <aidl/android/hardware/broadcastradio/BnTunerCallback.h>
#include <aidl/android/hardware/broadcastradio/ConfigFlag.h>
#include <aidl/android/hardware/broadcastradio/IBroadcastRadio.h>
#include <aidl/android/hardware/broadcastradio/ProgramInfo.h>
#include <aidl/android/hardware/broadcastradio/ProgramListChunk.h>
#include <aidl/android/hardware/broadcastradio/ProgramSelector.h>
#include <aidl/android/hardware/broadcastradio/Result.h>
#include <aidl/android/hardware/broadcastradio/VendorKeyValue.h>

#include <android-base/thread_annotations.h>

#include <broadcastradio-utils-aidl/Utils.h>

#include <condition_variable>

namespace aidl::android::hardware::broadcastradio {

namespace {
using ::ndk::ScopedAStatus;

}  // namespace

class MockBroadcastRadioCallback final : public BnTunerCallback {
  public:
    explicit MockBroadcastRadioCallback();
    ScopedAStatus onTuneFailed(Result result, const ProgramSelector& selector) override;
    ScopedAStatus onCurrentProgramInfoChanged(const ProgramInfo& info) override;
    ScopedAStatus onProgramListUpdated(const ProgramListChunk& chunk) override;
    ScopedAStatus onParametersUpdated(const std::vector<VendorKeyValue>& parameters) override;
    ScopedAStatus onAntennaStateChange(bool connected) override;
    ScopedAStatus onConfigFlagUpdated(ConfigFlag in_flag, bool in_value) override;

    bool waitOnCurrentProgramInfoChangedCallback();
    bool waitProgramReady();
    bool isTunerFailed();
    void reset();

    ProgramInfo getCurrentProgramInfo();
    utils::ProgramInfoSet getProgramList();

  private:
    class CallbackFlag final {
      public:
        CallbackFlag(int timeoutMs) { mTimeoutMs = timeoutMs; }
        /**
         * Notify that the callback is called.
         */
        void notify() {
            std::unique_lock<std::mutex> lock(mMutex);
            mCalled = true;
            lock.unlock();
            mCv.notify_all();
        };

        /**
         * Wait for the timeout passed into the constructor.
         */
        bool wait() {
            std::unique_lock<std::mutex> lock(mMutex);
            return mCv.wait_for(lock, std::chrono::milliseconds(mTimeoutMs),
                                [this] { return mCalled; });
        };

        /**
         * Reset the callback to not called.
         */
        void reset() {
            std::unique_lock<std::mutex> lock(mMutex);
            mCalled = false;
        }

      private:
        std::mutex mMutex;
        bool mCalled GUARDED_BY(mMutex) = false;
        std::condition_variable mCv;
        int mTimeoutMs;
    };

    std::mutex mLock;
    bool mAntennaConnectionState GUARDED_BY(mLock);
    bool tunerFailed GUARDED_BY(mLock) = false;
    ProgramInfo mCurrentProgramInfo GUARDED_BY(mLock);
    utils::ProgramInfoSet mProgramList GUARDED_BY(mLock);
    CallbackFlag mOnCurrentProgramInfoChangedFlag = CallbackFlag(IBroadcastRadio::TUNER_TIMEOUT_MS);
    CallbackFlag mOnProgramListReadyFlag = CallbackFlag(IBroadcastRadio::LIST_COMPLETE_TIMEOUT_MS);
};

}  // namespace aidl::android::hardware::broadcastradio
