/**
 * Copyright (c) 2021, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include <math/half.h>
#include <math/vec3.h>
#include <renderengine/ExternalTexture.h>
#include <renderengine/RenderEngine.h>
#include <ui/GraphicBuffer.h>
#include <ui/PixelFormat.h>
#include <ui/Rect.h>
#include <ui/Region.h>
#include "ReadbackVts.h"

namespace aidl::android::hardware::graphics::composer3::vts {

using ::android::renderengine::DisplaySettings;
using ::android::renderengine::ExternalTexture;
using ::android::renderengine::RenderEngineCreationArgs;

class TestRenderEngine {
  public:
    static constexpr uint32_t sMaxFrameBufferAcquireBuffers = 2;

    TestRenderEngine(const RenderEngineCreationArgs& args);
    ~TestRenderEngine();

    void setRenderLayers(std::vector<std::shared_ptr<TestLayer>> layers);
    void initGraphicBuffer(uint32_t width, uint32_t height, uint32_t layerCount, uint64_t usage);
    void setDisplaySettings(DisplaySettings& displaySettings) {
        mDisplaySettings = displaySettings;
    };
    void drawLayers();
    void checkColorBuffer(const std::vector<Color>& expectedColors);
    void checkColorBuffer(const ::android::sp<::android::GraphicBuffer>& buffer);

    ::android::renderengine::RenderEngine& getInternalRenderEngine() { return *mRenderEngine; }

  private:
    common::PixelFormat mFormat;
    std::vector<::android::renderengine::LayerSettings> mCompositionLayers;
    std::unique_ptr<::android::renderengine::RenderEngine> mRenderEngine;
    std::vector<::android::renderengine::LayerSettings> mRenderLayers;
    ::android::sp<::android::GraphicBuffer> mGraphicBuffer;

    DisplaySettings mDisplaySettings;
};

}  // namespace aidl::android::hardware::graphics::composer3::vts
