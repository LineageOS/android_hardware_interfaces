/*
 * Copyright (C) 2018 The Android Open Source Project
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

#include <VtsHalHidlTargetTestBase.h>
#include <android/hardware/graphics/composer/2.2/IComposer.h>
#include <android/hardware/graphics/composer/2.2/IComposerClient.h>
#include <composer-command-buffer/2.2/ComposerCommandBuffer.h>
#include <composer-vts/2.1/ComposerVts.h>
#include <utils/StrongPointer.h>

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_2 {
namespace vts {

using android::hardware::graphics::common::V1_0::Hdr;
using android::hardware::graphics::common::V1_1::ColorMode;
using android::hardware::graphics::common::V1_1::Dataspace;
using android::hardware::graphics::common::V1_1::PixelFormat;
using android::hardware::graphics::common::V1_1::RenderIntent;
using android::hardware::graphics::composer::V2_2::IComposer;
using android::hardware::graphics::composer::V2_2::IComposerClient;

class ComposerClient_v2_2;

// Only thing I need for Composer_v2_2 is to create a v2_2 ComposerClient
// Everything else is the same
class Composer_v2_2 : public V2_1::vts::Composer {
   public:
    Composer_v2_2() : V2_1::vts::Composer(){};
    explicit Composer_v2_2(const std::string& name) : V2_1::vts::Composer(name){};

    std::unique_ptr<ComposerClient_v2_2> createClient_v2_2();
};

// A wrapper to IComposerClient.
class ComposerClient_v2_2
    : public android::hardware::graphics::composer::V2_1::vts::ComposerClient {
   public:
    ComposerClient_v2_2(const sp<IComposerClient>& client)
        : V2_1::vts::ComposerClient(client), mClient_v2_2(client){};

    sp<V2_2::IComposerClient> getRaw() const;

    void execute_v2_2(V2_1::vts::TestCommandReader* reader, V2_2::CommandWriterBase* writer);

    std::vector<IComposerClient::PerFrameMetadataKey> getPerFrameMetadataKeys(Display display);

    Display createVirtualDisplay_2_2(uint32_t width, uint32_t height, PixelFormat formatHint,
                                     uint32_t outputBufferSlotCount, PixelFormat* outFormat);
    bool getClientTargetSupport_2_2(Display display, uint32_t width, uint32_t height,
                                    PixelFormat format, Dataspace dataspace);
    void setPowerMode_2_2(Display display, V2_2::IComposerClient::PowerMode mode);
    void setReadbackBuffer(Display display, const native_handle_t* buffer, int32_t releaseFence);
    void getReadbackBufferAttributes(Display display, PixelFormat* outPixelFormat,
                                     Dataspace* outDataspace);
    void getReadbackBufferFence(Display display, int32_t* outFence);

    std::vector<ColorMode> getColorModes(Display display);
    std::vector<RenderIntent> getRenderIntents(Display display, ColorMode mode);
    void setColorMode(Display display, ColorMode mode, RenderIntent intent);

    std::array<float, 16> getDataspaceSaturationMatrix(Dataspace dataspace);

   private:
    sp<V2_2::IComposerClient> mClient_v2_2;
};

}  // namespace vts
}  // namespace V2_2
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android
