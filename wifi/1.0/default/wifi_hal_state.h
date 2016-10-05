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

#ifndef WIFI_HAL_LEGACY_WIFI_HAL_STATE_H_
#define WIFI_HAL_LEGACY_WIFI_HAL_STATE_H_

#include <functional>

#include <android-base/macros.h>
#include <hardware_legacy/wifi_hal.h>
#include <utils/Looper.h>

namespace android {
namespace hardware {
namespace wifi {

/**
 * Class that stores common state and functionality shared between HAL services.
 */
class WifiHalState {
 public:
  WifiHalState(sp<Looper>& looper);

  /** Post a task to be executed on the main thread */
  void PostTask(const std::function<void()>& callback);

  wifi_hal_fn func_table_;
  /** opaque handle from vendor for use while HAL is running */
  wifi_handle hal_handle_;

  enum class RunState {
    STOPPED,
    STARTED,
    STOPPING
  };

  RunState run_state_;

 private:
  sp<Looper> looper_;

  DISALLOW_COPY_AND_ASSIGN(WifiHalState);
};

}  // namespace wifi
}  // namespace hardware
}  // namespace android

#endif  // WIFI_HAL_LEGACY_WIFI_HAL_STATE_H_
