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
#define LOG_TAG "android.hardware.drm@1.0-impl"

#include <utils/Log.h>

#include "DrmFactory.h"
#include "DrmPlugin.h"
#include "TypeConvert.h"

namespace android {
namespace hardware {
namespace drm {
namespace V1_0 {
namespace implementation {

DrmFactory::DrmFactory() :
    trebleLoader("/vendor/lib/hw", "createDrmFactory"),
    legacyLoader("/vendor/lib/mediadrm", "createDrmFactory") {
}

// Methods from ::android::hardware::drm::V1_0::IDrmFactory follow.
Return<bool> DrmFactory::isCryptoSchemeSupported(
        const hidl_array<uint8_t, 16>& uuid) {
    return isCryptoSchemeSupported(trebleLoader, uuid) ||
            isCryptoSchemeSupported(legacyLoader, uuid);
}

Return<bool> DrmFactory::isContentTypeSupported (
        const hidl_string& mimeType) {
    return isContentTypeSupported<PluginLoader, hidl_string>(trebleLoader, mimeType) ||
            isContentTypeSupported<LegacyLoader, String8>(legacyLoader, mimeType);
}

    Return<void> DrmFactory::createPlugin(const hidl_array<uint8_t, 16>& uuid,
            const hidl_string& appPackageName, createPlugin_cb _hidl_cb) {
        sp<IDrmPlugin> plugin = createTreblePlugin(uuid, appPackageName);
    if (plugin == nullptr) {
        plugin = createLegacyPlugin(uuid);
    }
    _hidl_cb(plugin != nullptr ? Status::OK : Status::ERROR_DRM_CANNOT_HANDLE, plugin);
    return Void();
}

sp<IDrmPlugin> DrmFactory::createTreblePlugin(const hidl_array<uint8_t, 16>& uuid,
        const hidl_string& appPackageName) {
    sp<IDrmPlugin> plugin;
    for (size_t i = 0; i < trebleLoader.factoryCount(); i++) {
        Return<void> hResult = trebleLoader.getFactory(i)->createPlugin(uuid,
                appPackageName, [&](Status status, const sp<IDrmPlugin>& hPlugin) {
                    if (status == Status::OK) {
                        plugin = hPlugin;
                    }
                }
            );
        if (plugin != nullptr) {
            return plugin;
        }
    }
    return nullptr;
}

sp<IDrmPlugin> DrmFactory::createLegacyPlugin(const hidl_array<uint8_t, 16>& uuid) {
    android::DrmPlugin *legacyPlugin = nullptr;
    for (size_t i = 0; i < legacyLoader.factoryCount(); i++) {
        legacyLoader.getFactory(i)->createDrmPlugin(uuid.data(), &legacyPlugin);
        if (legacyPlugin) {
            return new DrmPlugin(legacyPlugin);
        }
    }
    return nullptr;
}

IDrmFactory* HIDL_FETCH_IDrmFactory(const char* /* name */) {
    return new DrmFactory();
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace drm
}  // namespace hardware
}  // namespace android
