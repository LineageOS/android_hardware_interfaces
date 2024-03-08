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

#pragma once

#include <condition_variable>
#include <memory>
#include <mutex>

#include <android-base/properties.h>
#include <android/binder_auto_utils.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

#include <android-base/logging.h>

class AudioHalBinderServiceUtil {
  public:
    ndk::SpAIBinder connectToService(const std::string& serviceName) {
        mServiceName = serviceName;
        mBinder = ndk::SpAIBinder(AServiceManager_waitForService(serviceName.c_str()));
        if (mBinder == nullptr) {
            LOG(ERROR) << "Failed to get service " << serviceName;
        } else {
            LOG(DEBUG) << "Succeeded to get service " << serviceName;
        }
        return mBinder;
    }

    ndk::SpAIBinder restartService(
            std::chrono::milliseconds timeoutMs = std::chrono::milliseconds(3000)) {
        if (!stopService(timeoutMs)) {
            return {};
        }
        return connectToService(mServiceName);
    }

  private:
    class AidlDeathRecipient {
      public:
        explicit AidlDeathRecipient(const ndk::SpAIBinder& binder)
            : binder(binder), recipient(AIBinder_DeathRecipient_new(&binderDiedCallbackAidl)) {}

        binder_status_t linkToDeath() {
            return AIBinder_linkToDeath(binder.get(), recipient.get(), this);
        }

        bool waitForFired(std::chrono::milliseconds timeoutMs) {
            std::unique_lock<std::mutex> lock(mutex);
            return condition.wait_for(lock, timeoutMs, [this]() { return fired; });
        }

      private:
        const ndk::SpAIBinder binder;
        const ndk::ScopedAIBinder_DeathRecipient recipient;
        std::mutex mutex;
        std::condition_variable condition;
        bool fired = false;

        void binderDied() {
            std::unique_lock<std::mutex> lock(mutex);
            fired = true;
            condition.notify_one();
        };

        static void binderDiedCallbackAidl(void* cookie) {
            AidlDeathRecipient* self = static_cast<AidlDeathRecipient*>(cookie);
            self->binderDied();
        }
    };

    bool stopService(std::chrono::milliseconds timeoutMs) {
        AidlDeathRecipient deathHandler(mBinder);
        if (STATUS_OK != deathHandler.linkToDeath()) {
            LOG(ERROR) << "linkToDeath failed";
            return false;
        }
        if (!android::base::SetProperty("sys.audio.restart.hal", "1")) {
            LOG(ERROR) << "SetProperty failed";
            return false;
        }
        if (!deathHandler.waitForFired(timeoutMs)) {
            LOG(ERROR) << "Timeout wait for death of " << mServiceName;
            return false;
        }
        return true;
    }

    std::string mServiceName;
    ndk::SpAIBinder mBinder;
};
