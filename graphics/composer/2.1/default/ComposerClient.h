/*
 * Copyright 2016 The Android Open Source Project
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

#ifndef ANDROID_HARDWARE_GRAPHICS_COMPOSER_V2_1_COMPOSER_CLIENT_H
#define ANDROID_HARDWARE_GRAPHICS_COMPOSER_V2_1_COMPOSER_CLIENT_H

#include <mutex>
#include <unordered_map>
#include <vector>

#include <composer-command-buffer/2.1/ComposerCommandBuffer.h>
#include <composer-hal/2.1/ComposerCommandEngine.h>
#include <composer-hal/2.1/ComposerHal.h>
#include <composer-hal/2.1/ComposerResources.h>
#include <hardware/hwcomposer2.h>

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_1 {
namespace implementation {

using namespace hal;

class ComposerClient : public IComposerClient {
public:
    ComposerClient(ComposerHal& hal);
    virtual ~ComposerClient();

    void initialize();

    void onHotplug(Display display, IComposerCallback::Connection connected);
    void onRefresh(Display display);
    void onVsync(Display display, int64_t timestamp);

    // IComposerClient interface
    Return<void> registerCallback(
            const sp<IComposerCallback>& callback) override;
    Return<uint32_t> getMaxVirtualDisplayCount() override;
    Return<void> createVirtualDisplay(uint32_t width, uint32_t height,
            PixelFormat formatHint, uint32_t outputBufferSlotCount,
            createVirtualDisplay_cb hidl_cb) override;
    Return<Error> destroyVirtualDisplay(Display display) override;
    Return<void> createLayer(Display display, uint32_t bufferSlotCount,
            createLayer_cb hidl_cb) override;
    Return<Error> destroyLayer(Display display, Layer layer) override;
    Return<void> getActiveConfig(Display display,
            getActiveConfig_cb hidl_cb) override;
    Return<Error> getClientTargetSupport(Display display,
            uint32_t width, uint32_t height,
            PixelFormat format, Dataspace dataspace) override;
    Return<void> getColorModes(Display display,
            getColorModes_cb hidl_cb) override;
    Return<void> getDisplayAttribute(Display display,
            Config config, Attribute attribute,
            getDisplayAttribute_cb hidl_cb) override;
    Return<void> getDisplayConfigs(Display display,
            getDisplayConfigs_cb hidl_cb) override;
    Return<void> getDisplayName(Display display,
            getDisplayName_cb hidl_cb) override;
    Return<void> getDisplayType(Display display,
            getDisplayType_cb hidl_cb) override;
    Return<void> getDozeSupport(Display display,
            getDozeSupport_cb hidl_cb) override;
    Return<void> getHdrCapabilities(Display display,
            getHdrCapabilities_cb hidl_cb) override;
    Return<Error> setActiveConfig(Display display, Config config) override;
    Return<Error> setColorMode(Display display, ColorMode mode) override;
    Return<Error> setPowerMode(Display display, PowerMode mode) override;
    Return<Error> setVsyncEnabled(Display display, Vsync enabled) override;
    Return<Error> setClientTargetSlotCount(Display display,
            uint32_t clientTargetSlotCount) override;
    Return<Error> setInputCommandQueue(
            const MQDescriptorSync<uint32_t>& descriptor) override;
    Return<void> getOutputCommandQueue(
            getOutputCommandQueue_cb hidl_cb) override;
    Return<void> executeCommands(uint32_t inLength,
            const hidl_vec<hidl_handle>& inHandles,
            executeCommands_cb hidl_cb) override;

protected:
    virtual std::unique_ptr<ComposerResources> createResources();
    virtual std::unique_ptr<ComposerCommandEngine> createCommandEngine();

    void destroyResources();

    ComposerHal& mHal;

    std::unique_ptr<ComposerResources> mResources;

    std::mutex mCommandEngineMutex;
    std::unique_ptr<ComposerCommandEngine> mCommandEngine;

    sp<IComposerCallback> mCallback;
};

} // namespace implementation
} // namespace V2_1
} // namespace composer
} // namespace graphics
} // namespace hardware
} // namespace android

#endif  // ANDROID_HARDWARE_GRAPHICS_COMPOSER_V2_1_COMPOSER_CLIENT_H
