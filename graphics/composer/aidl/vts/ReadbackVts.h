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

#include <aidl/android/hardware/graphics/composer3/IComposerClient.h>
#include <android-base/unique_fd.h>
#include <android/hardware/graphics/composer3/ComposerClientReader.h>
#include <android/hardware/graphics/composer3/ComposerClientWriter.h>
#include <renderengine/RenderEngine.h>
#include <ui/GraphicBuffer.h>
#include <memory>
#include "GraphicsComposerCallback.h"
#include "VtsComposerClient.h"

namespace aidl::android::hardware::graphics::composer3::vts {

using ::android::renderengine::LayerSettings;
using common::Dataspace;
using common::PixelFormat;

static const Color BLACK = {0.0f, 0.0f, 0.0f, 1.0f};
static const Color RED = {1.0f, 0.0f, 0.0f, 1.0f};
// DIM_RED is 90% dimmed from RED in linear space
// hard-code as value 243 in 8-bit space here, as calculating it requires
// oetf(eotf(value) * .9), which is a complex non-linear transformation
static const Color DIM_RED = {243.f / 255.f, 0.0f, 0.0f, 1.0f};
static const Color TRANSLUCENT_RED = {1.0f, 0.0f, 0.0f, 0.3f};
static const Color GREEN = {0.0f, 1.0f, 0.0f, 1.0f};
static const Color BLUE = {0.0f, 0.0f, 1.0f, 1.0f};
static const Color WHITE = {1.0f, 1.0f, 1.0f, 1.0f};
static const Color LIGHT_RED = {0.5f, 0.0f, 0.0f, 1.0f};
static const Color LIGHT_GREEN = {0.0f, 0.5f, 0.0f, 1.0f};
static const Color LIGHT_BLUE = {0.0f, 0.0f, 0.5f, 1.0f};

class TestRenderEngine;

class TestLayer {
  public:
    TestLayer(const std::shared_ptr<VtsComposerClient>& client, int64_t display)
        : mDisplay(display) {
        const auto& [status, layer] = client->createLayer(display, kBufferSlotCount);
        EXPECT_TRUE(status.isOk());
        mLayer = layer;
    }

    // ComposerClient will take care of destroying layers, no need to explicitly
    // call destroyLayers here
    virtual ~TestLayer(){};

    virtual void write(ComposerClientWriter& writer);
    virtual LayerSettings toRenderEngineLayerSettings();

    void setDisplayFrame(Rect frame) { mDisplayFrame = frame; }
    void setSourceCrop(FRect crop) { mSourceCrop = crop; }
    void setZOrder(uint32_t z) { mZOrder = z; }
    void setWhitePointNits(float whitePointNits) { mWhitePointNits = whitePointNits; }
    void setBrightness(float brightness) { mBrightness = brightness; }

    void setSurfaceDamage(std::vector<Rect> surfaceDamage) {
        mSurfaceDamage = std::move(surfaceDamage);
    }

    void setDataspace(Dataspace dataspace) { mDataspace = dataspace; }

    void setTransform(Transform transform) { mTransform = transform; }
    void setAlpha(float alpha) { mAlpha = alpha; }
    void setBlendMode(BlendMode blendMode) { mBlendMode = blendMode; }

    BlendMode getBlendMode() const { return mBlendMode; }

    uint32_t getZOrder() const { return mZOrder; }

    float getAlpha() const { return mAlpha; }

    int64_t getLayer() const { return mLayer; }

    float getBrightness() const { return mBrightness; }

  protected:
    int64_t mDisplay;
    int64_t mLayer;
    Rect mDisplayFrame = {0, 0, 0, 0};
    float mBrightness = 1.f;
    float mWhitePointNits = -1.f;
    std::vector<Rect> mSurfaceDamage;
    Transform mTransform = static_cast<Transform>(0);
    FRect mSourceCrop = {0, 0, 0, 0};
    static constexpr uint32_t kBufferSlotCount = 64;
    float mAlpha = 1.0;
    BlendMode mBlendMode = BlendMode::NONE;
    uint32_t mZOrder = 0;
    Dataspace mDataspace = Dataspace::UNKNOWN;
};

class TestColorLayer : public TestLayer {
  public:
    TestColorLayer(const std::shared_ptr<VtsComposerClient>& client, int64_t display)
        : TestLayer{client, display} {}

    void write(ComposerClientWriter& writer) override;

    LayerSettings toRenderEngineLayerSettings() override;

    void setColor(Color color) { mColor = color; }

  private:
    Color mColor = WHITE;
};

class TestBufferLayer : public TestLayer {
  public:
    TestBufferLayer(const std::shared_ptr<VtsComposerClient>& client,
                    TestRenderEngine& renderEngine, int64_t display, uint32_t width,
                    uint32_t height, common::PixelFormat format,
                    Composition composition = Composition::DEVICE);

    void write(ComposerClientWriter& writer) override;

    LayerSettings toRenderEngineLayerSettings() override;

    void fillBuffer(std::vector<Color>& expectedColors);

    void setBuffer(std::vector<Color> colors);

    void setToClientComposition(ComposerClientWriter& writer);

    uint32_t getWidth() const { return mWidth; }

    uint32_t getHeight() const { return mHeight; }

    ::android::Rect getAccessRegion() const { return mAccessRegion; }

    uint32_t getLayerCount() const { return mLayerCount; }

  protected:
    Composition mComposition;
    ::android::sp<::android::GraphicBuffer> mGraphicBuffer;
    TestRenderEngine& mRenderEngine;
    int32_t mFillFence;
    uint32_t mWidth;
    uint32_t mHeight;
    uint32_t mLayerCount;
    PixelFormat mPixelFormat;
    uint32_t mUsage;
    ::android::Rect mAccessRegion;

  private:
    ::android::sp<::android::GraphicBuffer> allocateBuffer();
};

class ReadbackHelper {
  public:
    static std::string getColorModeString(ColorMode mode);

    static std::string getDataspaceString(Dataspace dataspace);

    static Dataspace getDataspaceForColorMode(ColorMode mode);

    static int32_t GetBytesPerPixel(PixelFormat pixelFormat);

    static void fillBuffer(uint32_t width, uint32_t height, uint32_t stride, void* bufferData,
                           PixelFormat pixelFormat, std::vector<Color> desiredPixelColors);

    static void clearColors(std::vector<Color>& expectedColors, int32_t width, int32_t height,
                            int32_t displayWidth);

    static void fillColorsArea(std::vector<Color>& expectedColors, int32_t stride, Rect area,
                               Color color);

    static bool readbackSupported(const PixelFormat& pixelFormat, const Dataspace& dataspace);

    static const std::vector<ColorMode> colorModes;
    static const std::vector<Dataspace> dataspaces;

    static void compareColorBuffers(const std::vector<Color>& expectedColors, void* bufferData,
                                    const uint32_t stride, const uint32_t width,
                                    const uint32_t height, PixelFormat pixelFormat);
    static void compareColorBuffers(void* expectedBuffer, void* actualBuffer, const uint32_t stride,
                                    const uint32_t width, const uint32_t height,
                                    PixelFormat pixelFormat);
};

class ReadbackBuffer {
  public:
    ReadbackBuffer(int64_t display, const std::shared_ptr<VtsComposerClient>& client, int32_t width,
                   int32_t height, common::PixelFormat pixelFormat, common::Dataspace dataspace);

    void setReadbackBuffer();

    void checkReadbackBuffer(const std::vector<Color>& expectedColors);

    ::android::sp<::android::GraphicBuffer> getBuffer();

  protected:
    uint32_t mWidth;
    uint32_t mHeight;
    uint32_t mLayerCount;
    uint32_t mUsage;
    PixelFormat mPixelFormat;
    Dataspace mDataspace;
    int64_t mDisplay;
    ::android::sp<::android::GraphicBuffer> mGraphicBuffer;
    std::shared_ptr<VtsComposerClient> mComposerClient;
    ::android::Rect mAccessRegion;
    native_handle_t mBufferHandle;

  private:
    ::android::sp<::android::GraphicBuffer> allocateBuffer();
};

}  // namespace aidl::android::hardware::graphics::composer3::vts
