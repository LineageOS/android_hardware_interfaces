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

#define LOG_TAG "automotive.vehicle@2.0-watchdog"

#include <common/include/vhal_v2_0/WatchdogClient.h>

#include <android/binder_manager.h>
#include <android/hardware/automotive/vehicle/2.0/types.h>

using aidl::android::automotive::watchdog::ICarWatchdog;
using aidl::android::automotive::watchdog::TimeoutLength;

namespace {

enum { WHAT_CHECK_ALIVE = 1 };

}  // namespace

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace V2_0 {

WatchdogClient::WatchdogClient(const sp<Looper>& handlerLooper, VehicleHalManager* vhalManager)
    : mHandlerLooper(handlerLooper), mVhalManager(vhalManager), mCurrentSessionId(-1) {
    mMessageHandler = new MessageHandlerImpl(this);
}

ndk::ScopedAStatus WatchdogClient::checkIfAlive(int32_t sessionId, TimeoutLength /*timeout*/) {
    mHandlerLooper->removeMessages(mMessageHandler, WHAT_CHECK_ALIVE);
    {
        Mutex::Autolock lock(mMutex);
        mCurrentSessionId = sessionId;
    }
    mHandlerLooper->sendMessage(mMessageHandler, Message(WHAT_CHECK_ALIVE));
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus WatchdogClient::prepareProcessTermination() {
    return ndk::ScopedAStatus::ok();
}

bool WatchdogClient::initialize() {
    ndk::SpAIBinder binder(
            AServiceManager_getService("android.automotive.watchdog.ICarWatchdog/default"));
    if (binder.get() == nullptr) {
        ALOGE("Failed to get carwatchdog daemon");
        return false;
    }
    std::shared_ptr<ICarWatchdog> server = ICarWatchdog::fromBinder(binder);
    if (server == nullptr) {
        ALOGE("Failed to connect to carwatchdog daemon");
        return false;
    }
    mWatchdogServer = server;

    binder = this->asBinder();
    if (binder.get() == nullptr) {
        ALOGE("Failed to get car watchdog client binder object");
        return false;
    }
    std::shared_ptr<ICarWatchdogClient> client = ICarWatchdogClient::fromBinder(binder);
    if (client == nullptr) {
        ALOGE("Failed to get ICarWatchdogClient from binder");
        return false;
    }
    mTestClient = client;
    mWatchdogServer->registerClient(client, TimeoutLength::TIMEOUT_NORMAL);
    ALOGI("Successfully registered the client to car watchdog server");
    return true;
}

void WatchdogClient::respondToWatchdog() {
    if (mWatchdogServer == nullptr) {
        ALOGW("Cannot respond to car watchdog daemon: car watchdog daemon is not connected");
        return;
    }
    int sessionId;
    {
        Mutex::Autolock lock(mMutex);
        sessionId = mCurrentSessionId;
    }
    if (isClientHealthy()) {
        ndk::ScopedAStatus status = mWatchdogServer->tellClientAlive(mTestClient, sessionId);
        if (!status.isOk()) {
            ALOGE("Failed to call tellClientAlive(session id = %d): %d", sessionId,
                  status.getStatus());
            return;
        }
    }
}

bool WatchdogClient::isClientHealthy() const {
    // We consider that default vehicle HAL is healthy if we can get PERF_VEHICLE_SPEED value.
    StatusCode status = StatusCode::TRY_AGAIN;
    VehiclePropValue propValue = {.prop = (int32_t)VehicleProperty::PERF_VEHICLE_SPEED};
    while (status == StatusCode::TRY_AGAIN) {
        mVhalManager->get(propValue,
                          [&propValue, &status](StatusCode s, const VehiclePropValue& v) {
                              status = s;
                              if (s == StatusCode::OK) {
                                  propValue = v;
                              }
                          });
    }
    return status == StatusCode::OK;
}

WatchdogClient::MessageHandlerImpl::MessageHandlerImpl(WatchdogClient* client) : mClient(client) {}

void WatchdogClient::MessageHandlerImpl::handleMessage(const Message& message) {
    switch (message.what) {
        case WHAT_CHECK_ALIVE:
            mClient->respondToWatchdog();
            break;
        default:
            ALOGW("Unknown message: %d", message.what);
    }
}

}  // namespace V2_0
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
