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

#include "BluetoothChannelSounding.h"

#include "BluetoothChannelSoundingSession.h"

namespace aidl::android::hardware::bluetooth::ranging::impl {

BluetoothChannelSounding::BluetoothChannelSounding() {}
BluetoothChannelSounding::~BluetoothChannelSounding() {}

ndk::ScopedAStatus BluetoothChannelSounding::getVendorSpecificData(
    std::optional<
        std::vector<std::optional<VendorSpecificData>>>* /*_aidl_return*/) {
  return ::ndk::ScopedAStatus::ok();
}
ndk::ScopedAStatus BluetoothChannelSounding::getSupportedSessionTypes(
    std::optional<std::vector<SessionType>>* _aidl_return) {
  std::vector<SessionType> supported_session_types = {};
  *_aidl_return = supported_session_types;
  return ::ndk::ScopedAStatus::ok();
}
ndk::ScopedAStatus BluetoothChannelSounding::getMaxSupportedCsSecurityLevel(
    CsSecurityLevel* _aidl_return) {
  CsSecurityLevel security_level = CsSecurityLevel::NOT_SUPPORTED;
  *_aidl_return = security_level;
  return ::ndk::ScopedAStatus::ok();
}
ndk::ScopedAStatus BluetoothChannelSounding::openSession(
    const BluetoothChannelSoundingParameters& /*in_params*/,
    const std::shared_ptr<IBluetoothChannelSoundingSessionCallback>&
        in_callback,
    std::shared_ptr<IBluetoothChannelSoundingSession>* _aidl_return) {
  if (in_callback == nullptr) {
    return ndk::ScopedAStatus::fromExceptionCodeWithMessage(
        EX_ILLEGAL_ARGUMENT, "Invalid nullptr callback");
  }
  std::shared_ptr<BluetoothChannelSoundingSession> session = nullptr;
  session = ndk::SharedRefBase::make<BluetoothChannelSoundingSession>(
      in_callback, Reason::LOCAL_STACK_REQUEST);
  *_aidl_return = session;
  return ::ndk::ScopedAStatus::ok();
}
}  // namespace aidl::android::hardware::bluetooth::ranging::impl
