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

#include <string>
#include <vector>

#include <android-base/strings.h>
#include <vintf/fcm_exclude.h>

namespace android::vintf::details {

// The predicate to VintfObject::checkMissingHalsInMatrices.
bool ShouldCheckMissingHalsInFcm(const std::string& package) {
    using std::placeholders::_1;

    static std::vector<std::string> included_prefixes{
            // Other AOSP HALs (e.g. android.frameworks.*) are not added because only framework
            // matrix is checked.
            "android.hardware.",
    };

    static std::vector<std::string> excluded_prefixes{
            // Packages without top level interfaces (including types-only packages) are exempted.
            "android.hardware.camera.device@",
            "android.hardware.gnss.measurement_corrections@1.",
            "android.hardware.graphics.bufferqueue@",

            // Test packages are exempted.
            "android.hardware.tests.",
    };

    static std::vector<std::string> excluded_exact{
            // Packages without top level interfaces (including types-only packages) are exempted.
            // HIDL
            "android.hardware.cas.native@1.0",
            "android.hardware.gnss.visibility_control@1.0",
            "android.hardware.media.bufferpool@1.0",
            "android.hardware.media.bufferpool@2.0",
            "android.hardware.radio.config@1.2",
            // AIDL
            "android.hardware.biometrics.common",
            "android.hardware.common",
            "android.hardware.common.fmq",
            "android.hardware.graphics.common",
            "android.hardware.keymaster",

            // Fastboot HAL is only used by recovery. Recovery is owned by OEM. Framework
            // does not depend on this HAL, hence it is not declared in any manifests or matrices.
            "android.hardware.fastboot@1.0",
            "android.hardware.fastboot@1.1",

            // Deprecated HALs.
            // HIDL
            // TODO(b/171260360) Remove when HAL definition is removed
            "android.hardware.audio.effect@2.0",
            "android.hardware.audio@2.0",
            // Health 1.0 HAL is deprecated. The top level interface are deleted.
            "android.hardware.health@1.0",
            // TODO(b/171260670) Remove when HAL definition is removed
            "android.hardware.nfc@1.0",
            // TODO(b/171260715) Remove when HAL definition is removed
            "android.hardware.radio.deprecated@1.0",
    };

    auto package_has_prefix = [&](const std::string& prefix) {
        return android::base::StartsWith(package, prefix);
    };

    // Only check packageAndVersions that are in the include list and not in the exclude list.
    if (!std::any_of(included_prefixes.begin(), included_prefixes.end(), package_has_prefix)) {
        return false;
    }

    if (std::find(excluded_exact.begin(), excluded_exact.end(), package) != excluded_exact.end()) {
        return false;
    }

    return !std::any_of(excluded_prefixes.begin(), excluded_prefixes.end(), package_has_prefix);
}

}  // namespace android::vintf::details
