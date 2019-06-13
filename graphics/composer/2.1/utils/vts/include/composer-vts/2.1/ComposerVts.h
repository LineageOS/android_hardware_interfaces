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

#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <android/hardware/graphics/composer/2.1/IComposer.h>
#include <composer-command-buffer/2.1/ComposerCommandBuffer.h>
#include <composer-vts/2.1/TestCommandReader.h>
#include <mapper-vts/2.0/MapperVts.h>
#include <mapper-vts/3.0/MapperVts.h>
#include <utils/StrongPointer.h>

#include "gtest/gtest.h"

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_1 {
namespace vts {

using android::hardware::graphics::common::V1_0::ColorMode;
using android::hardware::graphics::common::V1_0::Dataspace;
using android::hardware::graphics::common::V1_0::Hdr;
using android::hardware::graphics::common::V1_0::PixelFormat;
using IMapper2 = android::hardware::graphics::mapper::V2_0::IMapper;
using IMapper3 = android::hardware::graphics::mapper::V3_0::IMapper;
using Gralloc2 = android::hardware::graphics::mapper::V2_0::vts::Gralloc;
using Gralloc3 = android::hardware::graphics::mapper::V3_0::vts::Gralloc;

class ComposerClient;

// A wrapper to IComposer.
class Composer {
   public:
    Composer();
    explicit Composer(const std::string& name);

    sp<IComposer> getRaw() const;

    // Returns true when the composer supports the specified capability.
    bool hasCapability(IComposer::Capability capability) const;

    std::vector<IComposer::Capability> getCapabilities();
    std::string dumpDebugInfo();
    std::unique_ptr<ComposerClient> createClient();

   protected:
    explicit Composer(const sp<IComposer>& composer);

   private:
    const sp<IComposer> mComposer;

    std::unordered_set<IComposer::Capability> mCapabilities;
};

// A wrapper to IComposerClient.
class ComposerClient {
   public:
    explicit ComposerClient(const sp<IComposerClient>& client);
    ~ComposerClient();

    sp<IComposerClient> getRaw() const;

    void registerCallback(const sp<IComposerCallback>& callback);
    uint32_t getMaxVirtualDisplayCount();

    Display createVirtualDisplay(uint32_t width, uint32_t height, PixelFormat formatHint,
                                 uint32_t outputBufferSlotCount, PixelFormat* outFormat);
    void destroyVirtualDisplay(Display display);

    Layer createLayer(Display display, uint32_t bufferSlotCount);
    void destroyLayer(Display display, Layer layer);

    Config getActiveConfig(Display display);
    bool getClientTargetSupport(Display display, uint32_t width, uint32_t height,
                                PixelFormat format, Dataspace dataspace);
    std::vector<ColorMode> getColorModes(Display display);
    int32_t getDisplayAttribute(Display display, Config config,
                                IComposerClient::Attribute attribute);
    std::vector<Config> getDisplayConfigs(Display display);
    std::string getDisplayName(Display display);
    IComposerClient::DisplayType getDisplayType(Display display);
    bool getDozeSupport(Display display);
    std::vector<Hdr> getHdrCapabilities(Display display, float* outMaxLuminance,
                                        float* outMaxAverageLuminance, float* outMinLuminance);

    void setClientTargetSlotCount(Display display, uint32_t clientTargetSlotCount);
    void setActiveConfig(Display display, Config config);
    void setColorMode(Display display, ColorMode mode);
    void setPowerMode(Display display, IComposerClient::PowerMode mode);
    void setVsyncEnabled(Display display, bool enabled);

    void execute(TestCommandReader* reader, CommandWriterBase* writer);

   protected:
    // Keep track of all virtual displays and layers.  When a test fails with
    // ASSERT_*, the destructor will clean up the resources for the test.
    struct DisplayResource {
        DisplayResource(bool isVirtual_) : isVirtual(isVirtual_) {}

        bool isVirtual;
        std::unordered_set<Layer> layers;
    };
    std::unordered_map<Display, DisplayResource> mDisplayResources;

   private:
    const sp<IComposerClient> mClient;
};

class AccessRegion {
  public:
    int32_t left;
    int32_t top;
    int32_t width;
    int32_t height;
};

class Gralloc {
  public:
    explicit Gralloc();

    const native_handle_t* allocate(uint32_t width, uint32_t height, uint32_t layerCount,
                                    PixelFormat format, uint64_t usage, bool import = true,
                                    uint32_t* outStride = nullptr);

    void* lock(const native_handle_t* bufferHandle, uint64_t cpuUsage,
               const AccessRegion& accessRegionRect, int acquireFence);

    int unlock(const native_handle_t* bufferHandle);

    void freeBuffer(const native_handle_t* bufferHandle);

  protected:
    std::shared_ptr<Gralloc2> mGralloc2 = nullptr;
    std::shared_ptr<Gralloc3> mGralloc3 = nullptr;
};

}  // namespace vts
}  // namespace V2_1
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android
