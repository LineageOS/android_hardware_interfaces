/*
 * Copyright 2019 The Android Open Source Project
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

#include <composer-vts/2.2/ReadbackVts.h>
#include <math/half.h>
#include <math/vec3.h>
#include <renderengine/RenderEngine.h>
#include <ui/GraphicBuffer.h>
#include <ui/GraphicBufferAllocator.h>
#include <ui/PixelFormat.h>
#include <ui/Rect.h>
#include <ui/Region.h>

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_2 {
namespace vts {

using mapper::V2_1::IMapper;
using renderengine::DisplaySettings;
using renderengine::RenderEngineCreationArgs;
using vts::Gralloc;

class TestRenderEngine {
  public:
    static constexpr uint32_t sMaxFrameBufferAcquireBuffers = 2;

    TestRenderEngine(const RenderEngineCreationArgs& args);
    ~TestRenderEngine() = default;

    void setRenderLayers(std::vector<std::shared_ptr<TestLayer>> layers);
    void initGraphicBuffer(uint32_t width, uint32_t height, uint32_t layerCount, uint64_t usage);
    void setDisplaySettings(DisplaySettings& displaySettings) {
        mDisplaySettings = displaySettings;
    };
    void drawLayers();
    void checkColorBuffer(std::vector<V2_2::IComposerClient::Color>& expectedColors);

  private:
    common::V1_1::PixelFormat mFormat;
    std::vector<renderengine::LayerSettings> mCompositionLayers;
    std::unique_ptr<renderengine::RenderEngine> mRenderEngine;
    std::vector<renderengine::LayerSettings> mRenderLayers;
    sp<GraphicBuffer> mGraphicBuffer;
    DisplaySettings mDisplaySettings;
};

}  // namespace vts
}  // namespace V2_2
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android
