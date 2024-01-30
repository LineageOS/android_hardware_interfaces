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

#include "RenderEngineVts.h"
#include "renderengine/impl/ExternalTexture.h"

namespace aidl::android::hardware::graphics::composer3::vts {

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
    auto texture = std::make_shared<::android::renderengine::impl::ExternalTexture>(
            mGraphicBuffer, *mRenderEngine,
            ::android::renderengine::impl::ExternalTexture::Usage::WRITEABLE);
    auto result = mRenderEngine
                          ->drawLayers(mDisplaySettings, compositionLayers, texture,
                                       std::move(bufferFence))
                          .get();
    if (result.ok()) {
        result.value()->waitForever(LOG_TAG);
    }
}

void TestRenderEngine::checkColorBuffer(const std::vector<Color>& expectedColors) {
    void* bufferData;
    int32_t bytesPerPixel = -1;
    int32_t bytesPerStride = -1;
    ASSERT_EQ(0, mGraphicBuffer->lock(static_cast<uint32_t>(mGraphicBuffer->getUsage()),
                                      &bufferData, &bytesPerPixel, &bytesPerStride));
    const uint32_t stride = (bytesPerPixel > 0 && bytesPerStride > 0)
                                    ? static_cast<uint32_t>(bytesPerStride / bytesPerPixel)
                                    : mGraphicBuffer->getStride();
    ReadbackHelper::compareColorBuffers(expectedColors, bufferData, stride,
                                        mGraphicBuffer->getWidth(), mGraphicBuffer->getHeight(),
                                        mFormat);
    ASSERT_EQ(::android::OK, mGraphicBuffer->unlock());
}

void TestRenderEngine::checkColorBuffer(const ::android::sp<::android::GraphicBuffer>& buffer) {
    ASSERT_EQ(mGraphicBuffer->getWidth(), buffer->getWidth());
    ASSERT_EQ(mGraphicBuffer->getHeight(), buffer->getHeight());
    void* renderedBufferData;
    int32_t bytesPerPixel = -1;
    int32_t bytesPerStride = -1;
    ASSERT_EQ(0, mGraphicBuffer->lock(static_cast<uint32_t>(mGraphicBuffer->getUsage()),
                                      &renderedBufferData, &bytesPerPixel, &bytesPerStride));
    const uint32_t renderedStride = (bytesPerPixel > 0 && bytesPerStride > 0)
                                            ? static_cast<uint32_t>(bytesPerStride / bytesPerPixel)
                                            : mGraphicBuffer->getStride();

    void* bufferData;
    ASSERT_EQ(0, buffer->lock(static_cast<uint32_t>(buffer->getUsage()), &bufferData,
                              &bytesPerPixel, &bytesPerStride));
    const uint32_t bufferStride = (bytesPerPixel > 0 && bytesPerStride > 0)
                                          ? static_cast<uint32_t>(bytesPerStride / bytesPerPixel)
                                          : buffer->getStride();

    ASSERT_EQ(renderedStride, bufferStride);

    ReadbackHelper::compareColorBuffers(renderedBufferData, bufferData, bufferStride,
                                        mGraphicBuffer->getWidth(), mGraphicBuffer->getHeight(),
                                        mFormat);
    ASSERT_EQ(::android::OK, buffer->unlock());
    ASSERT_EQ(::android::OK, mGraphicBuffer->unlock());
}

}  // namespace aidl::android::hardware::graphics::composer3::vts
