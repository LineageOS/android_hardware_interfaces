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

#include "wifi_feature_flags.h"

namespace {
#ifdef WIFI_HIDL_FEATURE_AWARE
static const bool wifiHidlFeatureAware = true;
#else
static const bool wifiHidlFeatureAware = false;
#endif  // WIFI_HIDL_FEATURE_AWARE
#ifdef WIFI_HIDL_FEATURE_DUAL_INTERFACE
static const bool wifiHidlFeatureDualInterface = true;
#else
static const bool wifiHidlFeatureDualInterface = false;
#endif  // WIFI_HIDL_FEATURE_DUAL_INTERFACE
#ifdef WIFI_HIDL_FEATURE_DISABLE_AP
static const bool wifiHidlFeatureDisableAp = true;
#else
static const bool wifiHidlFeatureDisableAp = false;
#endif  // WIFI_HIDL_FEATURE_DISABLE_AP

}  // namespace

namespace android {
namespace hardware {
namespace wifi {
namespace V1_2 {
namespace implementation {
namespace feature_flags {

WifiFeatureFlags::WifiFeatureFlags() {}
bool WifiFeatureFlags::isAwareSupported() { return wifiHidlFeatureAware; }
bool WifiFeatureFlags::isDualInterfaceSupported() {
    return wifiHidlFeatureDualInterface;
}
bool WifiFeatureFlags::isApDisabled() {
  return wifiHidlFeatureDisableAp;
}

}  // namespace feature_flags
}  // namespace implementation
}  // namespace V1_2
}  // namespace wifi
}  // namespace hardware
}  // namespace android
