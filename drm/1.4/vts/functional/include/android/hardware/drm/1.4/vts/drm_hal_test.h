/*
 * Copyright (C) 2021 The Android Open Source Project
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

#ifndef DRM_HAL_TEST_V1_4_H
#define DRM_HAL_TEST_V1_4_H

#include <android/hardware/drm/1.0/IDrmPlugin.h>
#include <android/hardware/drm/1.3/IDrmFactory.h>
#include <android/hardware/drm/1.4/ICryptoFactory.h>
#include <android/hardware/drm/1.4/ICryptoPlugin.h>
#include <android/hardware/drm/1.4/IDrmFactory.h>
#include <android/hardware/drm/1.4/IDrmPlugin.h>
#include <gtest/gtest.h>
#include <hidl/HidlSupport.h>
#include <hidl/ServiceManagement.h>
#include <log/log.h>

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

#include "drm_hal_vendor_module_api.h"
#include "drm_vts_helper.h"
#include "vendor_modules.h"
#include "VtsHalHidlTargetCallbackBase.h"

#include "android/hardware/drm/1.2/vts/drm_hal_common.h"

namespace android {
namespace hardware {
namespace drm {
namespace V1_4 {
namespace vts {

namespace drm = ::android::hardware::drm;
using android::hardware::hidl_array;
using android::hardware::hidl_string;
using V1_0::SessionId;
using V1_1::SecurityLevel;

using drm_vts::DrmHalTestParam;

class DrmHalTest : public drm::V1_2::vts::DrmHalTest {
public:
  using drm::V1_2::vts::DrmHalTest::DrmHalTest;
  static const char* const kVideoMp4;
  static const char* const kAudioMp4;
  static const uint32_t kSecLevelMin = static_cast<uint32_t>(SecurityLevel::SW_SECURE_CRYPTO);
  static const uint32_t kSecLevelMax = static_cast<uint32_t>(SecurityLevel::HW_SECURE_ALL);
  static const uint32_t kSecLevelDefault;

protected:
  sp<V1_4::IDrmPlugin> DrmPluginV1_4() const;
  sp<V1_0::ICryptoPlugin> CryptoPlugin(const SessionId& sid);
  SessionId OpenSession(uint32_t level);

private:
  void DoProvisioning();
};

}  // namespace vts
}  // namespace V1_4
}  // namespace drm
}  // namespace hardware
}  // namespace android

#endif  // DRM_HAL_TEST_V1_4_H
