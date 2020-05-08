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

#ifndef android_hardware_automotive_vehicle_V2_0_WatchdogClient_H_
#define android_hardware_automotive_vehicle_V2_0_WatchdogClient_H_

#include "VehicleHalManager.h"

#include <aidl/android/automotive/watchdog/BnCarWatchdog.h>
#include <aidl/android/automotive/watchdog/BnCarWatchdogClient.h>
#include <utils/Looper.h>
#include <utils/Mutex.h>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace V2_0 {

class WatchdogClient : public aidl::android::automotive::watchdog::BnCarWatchdogClient {
  public:
    explicit WatchdogClient(const ::android::sp<::android::Looper>& handlerLooper,
                            VehicleHalManager* vhalManager);

    ndk::ScopedAStatus checkIfAlive(
            int32_t sessionId, aidl::android::automotive::watchdog::TimeoutLength timeout) override;
    ndk::ScopedAStatus prepareProcessTermination() override;

    bool initialize();

  private:
    class MessageHandlerImpl : public ::android::MessageHandler {
      public:
        explicit MessageHandlerImpl(WatchdogClient* client);
        void handleMessage(const ::android::Message& message) override;

      private:
        WatchdogClient* mClient;
    };

  private:
    void respondToWatchdog();
    bool isClientHealthy() const;

  private:
    ::android::sp<::android::Looper> mHandlerLooper;
    ::android::sp<MessageHandlerImpl> mMessageHandler;
    std::shared_ptr<aidl::android::automotive::watchdog::ICarWatchdog> mWatchdogServer;
    std::shared_ptr<aidl::android::automotive::watchdog::ICarWatchdogClient> mTestClient;
    VehicleHalManager* mVhalManager;
    ::android::Mutex mMutex;
    int mCurrentSessionId GUARDED_BY(mMutex);
};

}  // namespace V2_0
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android

#endif  // android_hardware_automotive_vehicle_V2_0_WatchdogClient_H_
