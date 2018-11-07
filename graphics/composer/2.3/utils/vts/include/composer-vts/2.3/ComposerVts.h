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
#include <vector>

#include <VtsHalHidlTargetTestBase.h>
#include <android/hardware/graphics/composer/2.3/IComposer.h>
#include <android/hardware/graphics/composer/2.3/IComposerClient.h>
#include <composer-vts/2.2/ComposerVts.h>
#include <utils/StrongPointer.h>

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_3 {
namespace vts {

using common::V1_1::PixelFormat;
using common::V1_1::RenderIntent;
using common::V1_2::ColorMode;
using common::V1_2::Dataspace;
using V2_1::Display;
using V2_1::Error;
using V2_3::IComposer;
using V2_3::IComposerClient;

class ComposerClient;

// A wrapper to IComposer.
class Composer : public V2_2::vts::Composer {
   public:
    Composer();
    explicit Composer(const std::string& name);

    std::unique_ptr<ComposerClient> createClient();

   protected:
    explicit Composer(const sp<IComposer>& composer);

   private:
    const sp<IComposer> mComposer;
};

// A wrapper to IComposerClient.
class ComposerClient : public V2_2::vts::ComposerClient {
   public:
    explicit ComposerClient(const sp<IComposerClient>& client)
        : V2_2::vts::ComposerClient(client), mClient(client) {}

    sp<IComposerClient> getRaw() const;

    bool getDisplayIdentificationData(Display display, uint8_t* outPort,
                                      std::vector<uint8_t>* outData);
    Error getDisplayedContentSamplingAttributes(
        uint64_t display, PixelFormat& format, Dataspace& dataspace,
        hidl_bitfield<IComposerClient::FormatColorComponent>& componentMask);
    Error setDisplayedContentSamplingEnabled(
        uint64_t display, IComposerClient::DisplayedContentSampling enable,
        hidl_bitfield<IComposerClient::FormatColorComponent> componentMask, uint64_t maxFrames);
    Error getDisplayedContentSample(uint64_t display, uint64_t maxFrames, uint64_t timestamp,
                                    uint64_t& frameCount, hidl_vec<uint64_t>& sampleComponent0,
                                    hidl_vec<uint64_t>& sampleComponent1,
                                    hidl_vec<uint64_t>& sampleComponent2,
                                    hidl_vec<uint64_t>& sampleComponent3);

    std::vector<ColorMode> getColorModes_2_3(Display display);

    void setColorMode_2_3(Display display, ColorMode mode, RenderIntent intent);

    std::vector<RenderIntent> getRenderIntents_2_3(Display display, ColorMode mode);

    void getReadbackBufferAttributes_2_3(Display display, PixelFormat* outPixelFormat,
                                         Dataspace* outDataspace);

    bool getClientTargetSupport_2_3(Display display, uint32_t width, uint32_t height,
                                    PixelFormat format, Dataspace dataspace);

   private:
    const sp<IComposerClient> mClient;
};

}  // namespace vts
}  // namespace V2_3
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android
