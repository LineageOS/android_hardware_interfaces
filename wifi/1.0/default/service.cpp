/*
 * Copyright (C) 2016 The Android Open Source Project
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
#include <hidl/IServiceManager.h>
#include <hwbinder/IPCThreadState.h>
#include <hwbinder/ProcessState.h>
#include <utils/Looper.h>
#include <utils/StrongPointer.h>

#include "wifi.h"

using android::hardware::hidl_version;
using android::hardware::IPCThreadState;
using android::hardware::ProcessState;
using android::Looper;

namespace {
int OnBinderReadReady(int /*fd*/, int /*events*/, void* /*data*/) {
  IPCThreadState::self()->handlePolledCommands();
  return 1;  // continue receiving events
}
}

int main(int /*argc*/, char** argv) {
  android::base::InitLogging(argv,
                             android::base::LogdLogger(android::base::SYSTEM));
  LOG(INFO) << "wifi_hal_legacy is starting up...";

  // Setup binder
  int binder_fd = -1;
  ProcessState::self()->setThreadPoolMaxThreadCount(0);
  CHECK_EQ(IPCThreadState::self()->setupPolling(&binder_fd), android::NO_ERROR)
      << "Failed to initialize binder polling";
  CHECK_GE(binder_fd, 0) << "Invalid binder FD: " << binder_fd;

  // Setup looper
  android::sp<Looper> looper = Looper::prepare(0 /* no options */);
  CHECK(looper->addFd(
      binder_fd, 0, Looper::EVENT_INPUT, OnBinderReadReady, nullptr))
      << "Failed to watch binder FD";

  // Setup hwbinder service
  android::sp<android::hardware::wifi::V1_0::IWifi> service =
      new android::hardware::wifi::V1_0::implementation::Wifi();
  CHECK_EQ(service->registerAsService("wifi"), android::NO_ERROR)
      << "Failed to register wifi HAL";

  // Loop
  while (looper->pollAll(-1) != Looper::POLL_ERROR) {
    // Keep polling until failure.
  }

  LOG(INFO) << "wifi_hal_legacy is terminating...";
  return 0;
}
