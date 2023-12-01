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

#include <aidl/android/hardware/bluetooth/finder/BnBluetoothFinder.h>

#include <vector>

namespace aidl::android::hardware::bluetooth::finder::impl {

using ::aidl::android::hardware::bluetooth::finder::BnBluetoothFinder;
using ::aidl::android::hardware::bluetooth::finder::Eid;

class BluetoothFinder : public BnBluetoothFinder {
 public:
  BluetoothFinder() = default;

  ::ndk::ScopedAStatus sendEids(const ::std::vector<Eid>& keys) override;
  ::ndk::ScopedAStatus setPoweredOffFinderMode(bool enable) override;
  ::ndk::ScopedAStatus getPoweredOffFinderMode(bool* _aidl_return) override;

 private:
  bool pof_enabled_;
  std::vector<Eid> keys_;
};

}  // namespace aidl::android::hardware::bluetooth::finder::impl
