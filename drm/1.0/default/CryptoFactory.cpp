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

#include <utils/Log.h>

#include "CryptoFactory.h"
#include "CryptoPlugin.h"
#include "TypeConvert.h"

namespace android {
namespace hardware {
namespace drm {
namespace V1_0 {
namespace implementation {

CryptoFactory::CryptoFactory() :
    trebleLoader("/vendor/lib/hw", "createCryptoFactory"),
    legacyLoader("/vendor/lib/mediadrm", "createCryptoFactory") {
}

// Methods from ::android::hardware::drm::V1_0::ICryptoFactory follow.
Return<bool> CryptoFactory::isCryptoSchemeSupported(
        const hidl_array<uint8_t, 16>& uuid) {
    return isCryptoSchemeSupported(trebleLoader, uuid) ||
            isCryptoSchemeSupported(legacyLoader, uuid);
}

Return<void> CryptoFactory::createPlugin(const hidl_array<uint8_t, 16>& uuid,
        const hidl_vec<uint8_t>& initData, createPlugin_cb _hidl_cb) {
    sp<ICryptoPlugin> plugin = createTreblePlugin(uuid, initData);
    if (plugin == nullptr) {
        plugin = createLegacyPlugin(uuid, initData);
    }
    _hidl_cb(plugin != nullptr ? Status::OK : Status::ERROR_DRM_CANNOT_HANDLE, plugin);
    return Void();
}

sp<ICryptoPlugin> CryptoFactory::createTreblePlugin(const hidl_array<uint8_t, 16>& uuid,
        const hidl_vec<uint8_t>& initData) {
    sp<ICryptoPlugin> plugin;
    for (size_t i = 0; i < trebleLoader.factoryCount(); i++) {
        Return<void> hResult = trebleLoader.getFactory(i)->createPlugin(uuid, initData,
                [&](Status status, const sp<ICryptoPlugin>& hPlugin) {
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

sp<ICryptoPlugin> CryptoFactory::createLegacyPlugin(const hidl_array<uint8_t, 16>& uuid,
        const hidl_vec<uint8_t>& initData) {
    android::CryptoPlugin *legacyPlugin = nullptr;
    for (size_t i = 0; i < legacyLoader.factoryCount(); i++) {
        legacyLoader.getFactory(i)->createPlugin(uuid.data(),
                initData.data(), initData.size(), &legacyPlugin);
        if (legacyPlugin) {
            return new CryptoPlugin(legacyPlugin);
        }
    }
    return nullptr;
}


ICryptoFactory* HIDL_FETCH_ICryptoFactory(const char* /* name */) {
    return new CryptoFactory();
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace drm
}  // namespace hardware
}  // namespace android
