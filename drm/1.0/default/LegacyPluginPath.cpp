/*
 * Copyright (C) 2017 The Android Open Source Project
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

#include "LegacyPluginPath.h"

#include <unistd.h>

#include <cutils/properties.h>

namespace android {
namespace hardware {
namespace drm {
namespace V1_0 {
namespace implementation {

// 64-bit DRM depends on OEM libraries that aren't
// provided for all devices. If the drm hal service
// is running as 64-bit use the 64-bit libs, otherwise
// use the 32-bit libs.
const char* getDrmPluginPath() {
#if defined(__LP64__)
    return "/vendor/lib64/mediadrm";
#else
    return "/vendor/lib/mediadrm";
#endif
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace drm
}  // namespace hardware
}  // namespace android
