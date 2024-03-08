/*
 * Copyright (C) 2023 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "BluetoothFinder.h"

namespace aidl::android::hardware::bluetooth::finder::impl {

::ndk::ScopedAStatus BluetoothFinder::sendEids(const ::std::vector<Eid>& keys) {
  keys_.insert(keys_.end(), keys.begin(), keys.end());
  return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus BluetoothFinder::setPoweredOffFinderMode(bool enable) {
  pof_enabled_ = enable;
  return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus BluetoothFinder::getPoweredOffFinderMode(
    bool* _aidl_return) {
  *_aidl_return = pof_enabled_;
  return ::ndk::ScopedAStatus::ok();
}

}  // namespace aidl::android::hardware::bluetooth::finder::impl
