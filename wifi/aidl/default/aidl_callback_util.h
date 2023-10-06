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

#ifndef AIDL_CALLBACK_UTIL_H_
#define AIDL_CALLBACK_UTIL_H_

#include <android-base/logging.h>

#include <mutex>
#include <set>
#include <unordered_map>

namespace {
std::unordered_map<void* /* callback */, void* /* handler */> callback_handler_map_;
std::mutex callback_handler_lock_;
}

namespace aidl {
namespace android {
namespace hardware {
namespace wifi {
namespace aidl_callback_util {

// Provides a class to manage callbacks for the various AIDL interfaces and
// handle the death of the process hosting each callback.
template <typename CallbackType>
class AidlCallbackHandler {
  public:
    AidlCallbackHandler() {
        death_handler_ = AIBinder_DeathRecipient_new(AidlCallbackHandler::onCallbackDeath);
    }
    ~AidlCallbackHandler() { invalidate(); }

    bool addCallback(const std::shared_ptr<CallbackType>& cb) {
        std::unique_lock<std::mutex> lk(callback_handler_lock_);
        void* cbPtr = reinterpret_cast<void*>(cb->asBinder().get());
        const auto& cbPosition = findCbInSet(cbPtr);
        if (cbPosition != cb_set_.end()) {
            LOG(WARNING) << "Duplicate death notification registration";
            return true;
        }

        if (AIBinder_linkToDeath(cb->asBinder().get(), death_handler_, cbPtr /* cookie */) !=
            STATUS_OK) {
            LOG(ERROR) << "Failed to register death notification";
            return false;
        }

        callback_handler_map_[cbPtr] = reinterpret_cast<void*>(this);
        cb_set_.insert(cb);
        // unique_lock unlocked here
        return true;
    }

    const std::set<std::shared_ptr<CallbackType>>& getCallbacks() {
        std::unique_lock<std::mutex> lk(callback_handler_lock_);
        // unique_lock unlocked here
        return cb_set_;
    }

    void invalidate() {
        std::unique_lock<std::mutex> lk(callback_handler_lock_);
        for (auto cb : cb_set_) {
            void* cookie = reinterpret_cast<void*>(cb->asBinder().get());
            if (AIBinder_unlinkToDeath(cb->asBinder().get(), death_handler_, cookie) != STATUS_OK) {
                LOG(ERROR) << "Failed to deregister death notification";
            }
            if (!removeCbFromHandlerMap(cookie)) {
                LOG(ERROR) << "Failed to remove callback from handler map";
            }
        }
        cb_set_.clear();
        // unique_lock unlocked here
    }

    // Entry point for the death handling logic. AIBinder_DeathRecipient
    // can only call a static function, so use the cookie to find the
    // proper handler and route the request there.
    static void onCallbackDeath(void* cookie) {
        std::unique_lock<std::mutex> lk(callback_handler_lock_);
        auto cbQuery = callback_handler_map_.find(cookie);
        if (cbQuery == callback_handler_map_.end()) {
            LOG(ERROR) << "Invalid death cookie received";
            return;
        }

        AidlCallbackHandler* cbHandler = reinterpret_cast<AidlCallbackHandler*>(cbQuery->second);
        if (cbHandler == nullptr) {
            LOG(ERROR) << "Handler mapping contained an invalid handler";
            return;
        }
        cbHandler->handleCallbackDeath(cbQuery->first);
        // unique_lock unlocked here
    }

  private:
    std::set<std::shared_ptr<CallbackType>> cb_set_;
    AIBinder_DeathRecipient* death_handler_;

    typename std::set<std::shared_ptr<CallbackType>>::iterator findCbInSet(void* cbPtr) {
        const auto& cbPosition = std::find_if(
                cb_set_.begin(), cb_set_.end(), [cbPtr](const std::shared_ptr<CallbackType>& p) {
                    return cbPtr == reinterpret_cast<void*>(p->asBinder().get());
                });
        return cbPosition;
    }

    bool removeCbFromHandlerMap(void* cbPtr) {
        auto cbQuery = callback_handler_map_.find(cbPtr);
        if (cbQuery != callback_handler_map_.end()) {
            callback_handler_map_.erase(cbQuery);
            return true;
        }
        return false;
    }

    void handleCallbackDeath(void* cbPtr) {
        const auto& cbPosition = findCbInSet(cbPtr);
        if (cbPosition == cb_set_.end()) {
            LOG(ERROR) << "Unknown callback death notification received";
            return;
        }
        cb_set_.erase(cbPosition);

        if (!removeCbFromHandlerMap(cbPtr)) {
            LOG(ERROR) << "Callback was not in callback handler map";
        }
    }

    DISALLOW_COPY_AND_ASSIGN(AidlCallbackHandler);
};

}  // namespace aidl_callback_util
}  // namespace wifi
}  // namespace hardware
}  // namespace android
}  // namespace aidl

#endif  // AIDL_CALLBACK_UTIL_H_
