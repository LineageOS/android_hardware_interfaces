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

#include <VtsCoreUtil.h>

#include <aidl/android/hardware/wifi/IWifi.h>
#include <aidl/android/hardware/wifi/IWifiChip.h>
#include <android/binder_manager.h>
#include <wifi_system/interface_tool.h>

using aidl::android::hardware::wifi::IfaceConcurrencyType;
using aidl::android::hardware::wifi::IWifi;
using aidl::android::hardware::wifi::IWifiApIface;
using aidl::android::hardware::wifi::IWifiChip;
using aidl::android::hardware::wifi::IWifiNanIface;
using aidl::android::hardware::wifi::IWifiStaIface;
using aidl::android::hardware::wifi::WifiStatusCode;

// Helper functions to obtain references to the various AIDL interface objects.
std::shared_ptr<IWifi> getWifi(const char* instance_name);
std::shared_ptr<IWifiChip> getWifiChip(const char* instance_name);
std::shared_ptr<IWifiStaIface> getWifiStaIface(const char* instance_name);
std::shared_ptr<IWifiNanIface> getWifiNanIface(const char* instance_name);
std::shared_ptr<IWifiApIface> getWifiApIface(const char* instance_name);
std::shared_ptr<IWifiApIface> getBridgedWifiApIface(const char* instance_name);
std::shared_ptr<IWifiApIface> getBridgedWifiApIface(std::shared_ptr<IWifiChip> wifi_chip);
// Configure the chip in a mode to support the creation of the provided iface type.
bool configureChipToSupportConcurrencyType(const std::shared_ptr<IWifiChip>& wifi_chip,
                                           IfaceConcurrencyType type, int* configured_mode_id);
// Check whether the chip supports the creation of the provided iface type.
bool doesChipSupportConcurrencyType(const std::shared_ptr<IWifiChip>& wifi_chip,
                                    IfaceConcurrencyType type);
// Used to trigger IWifi.stop() at the end of every test.
void stopWifiService(const char* instance_name);
int32_t getChipFeatureSet(const std::shared_ptr<IWifiChip>& wifi_chip);
bool checkStatusCode(ndk::ScopedAStatus* status, WifiStatusCode expected_code);
bool isAidlServiceAvailable(const char* instance_name);
