/*
 * Copyright (C) 2021 The Android Open Source Project
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

#ifndef android_hardware_automotive_vehicle_aidl_impl_vhal_include_RecurrentTimer_H_
#define android_hardware_automotive_vehicle_aidl_impl_vhal_include_RecurrentTimer_H_

#include <android-base/thread_annotations.h>

#include <utils/Looper.h>
#include <atomic>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>
#include <vector>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {

// Forward declaration
class RecurrentMessageHandler;

// A thread-safe recurrent timer.
class RecurrentTimer final {
  public:
    // The class for the function that would be called recurrently.
    using Callback = std::function<void()>;

    RecurrentTimer();

    ~RecurrentTimer();

    // Registers a recurrent callback for a given interval.
    // Registering the same callback twice will override the interval provided before.
    void registerTimerCallback(int64_t intervalInNanos, std::shared_ptr<Callback> callback);

    // Unregisters a previously registered recurrent callback.
    void unregisterTimerCallback(std::shared_ptr<Callback> callback);

  private:
    friend class RecurrentMessageHandler;

    // For unit test
    friend class RecurrentTimerTest;

    struct CallbackInfo {
        std::shared_ptr<Callback> callback;
        int64_t intervalInNanos;
        int64_t nextTimeInNanos;
    };

    android::sp<Looper> mLooper;
    android::sp<RecurrentMessageHandler> mHandler;

    std::atomic<bool> mStopRequested = false;
    std::atomic<int> mCallbackId = 0;
    std::mutex mLock;
    std::thread mThread;
    std::unordered_map<std::shared_ptr<Callback>, int> mIdByCallback GUARDED_BY(mLock);
    std::unordered_map<int, std::unique_ptr<CallbackInfo>> mCallbackInfoById GUARDED_BY(mLock);

    void handleMessage(const android::Message& message) EXCLUDES(mLock);
    int getCallbackIdLocked(std::shared_ptr<Callback> callback) REQUIRES(mLock);
};

class RecurrentMessageHandler final : public android::MessageHandler {
  public:
    RecurrentMessageHandler(RecurrentTimer* timer) { mTimer = timer; }
    void handleMessage(const android::Message& message) override;

  private:
    RecurrentTimer* mTimer;
};

}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android

#endif  // android_hardware_automotive_vehicle_aidl_impl_vhal_include_RecurrentTimer_H_
