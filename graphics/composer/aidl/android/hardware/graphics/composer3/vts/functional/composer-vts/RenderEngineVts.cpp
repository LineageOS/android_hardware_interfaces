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

#include "include/RenderEngineVts.h"

namespace aidl::android::hardware::graphics::composer3::vts {

using ::android::hardware::graphics::mapper::V2_1::IMapper;
using ::android::renderengine::DisplaySettings;
using ::android::renderengine::LayerSettings;
using ::android::renderengine::RenderEngineCreationArgs;

TestRenderEngine::TestRenderEngine(const RenderEngineCreationArgs& args) {
    mFormat = static_cast<common::PixelFormat>(args.pixelFormat);
    mRenderEngine = ::android::renderengine::RenderEngine::create(args);
}

TestRenderEngine::~TestRenderEngine() {
    mRenderEngine.release();
}

void TestRenderEngine::setRenderLayers(std::vector<std::shared_ptr<TestLayer>> layers) {
    sort(layers.begin(), layers.end(),
         [](const std::shared_ptr<TestLayer>& lhs, const std::shared_ptr<TestLayer>& rhs) -> bool {
             return lhs->getZOrder() < rhs->getZOrder();
         });

    if (!mCompositionLayers.empty()) {
        mCompositionLayers.clear();
    }
    for (auto& layer : layers) {
        LayerSettings settings = layer->toRenderEngineLayerSettings();
        mCompositionLayers.push_back(settings);
    }
}

void TestRenderEngine::initGraphicBuffer(uint32_t width, uint32_t height, uint32_t layerCount,
                                         uint64_t usage) {
    mGraphicBuffer = ::android::sp<::android::GraphicBuffer>::make(
            width, height, static_cast<int32_t>(mFormat), layerCount, usage);
}

void TestRenderEngine::drawLayers() {
    ::android::base::unique_fd bufferFence;

    std::vector<::android::renderengine::LayerSettings> compositionLayers;
    compositionLayers.reserve(mCompositionLayers.size());
    std::transform(mCompositionLayers.begin(), mCompositionLayers.end(),
                   std::back_insert_iterator(compositionLayers),
                   [](::android::renderengine::LayerSettings& settings)
                           -> ::android::renderengine::LayerSettings { return settings; });
    auto texture = std::make_shared<::android::renderengine::ExternalTexture>(
            mGraphicBuffer, *mRenderEngine,
            ::android::renderengine::ExternalTexture::Usage::WRITEABLE);
    auto [status, readyFence] = mRenderEngine
                                        ->drawLayers(mDisplaySettings, compositionLayers, texture,
                                                     true, std::move(bufferFence))
                                        .get();
    int fd = readyFence.release();
    if (fd != -1) {
        ASSERT_EQ(0, sync_wait(fd, -1));
        ASSERT_EQ(0, close(fd));
    }
}

void TestRenderEngine::checkColorBuffer(std::vector<Color>& expectedColors) {
    void* bufferData;
    ASSERT_EQ(0,
              mGraphicBuffer->lock(static_cast<uint32_t>(mGraphicBuffer->getUsage()), &bufferData));
    ReadbackHelper::compareColorBuffers(
            expectedColors, bufferData, static_cast<int32_t>(mGraphicBuffer->getStride()),
            mGraphicBuffer->getWidth(), mGraphicBuffer->getHeight(), mFormat);
    ASSERT_EQ(::android::OK, mGraphicBuffer->unlock());
}

}  // namespace aidl::android::hardware::graphics::composer3::vts
