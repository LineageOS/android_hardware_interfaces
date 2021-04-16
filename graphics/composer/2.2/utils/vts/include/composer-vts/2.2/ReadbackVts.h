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

#pragma once

#include <android-base/unique_fd.h>
#include <android/hardware/graphics/composer/2.2/IComposerClient.h>
#include <composer-command-buffer/2.2/ComposerCommandBuffer.h>
#include <composer-vts/2.1/GraphicsComposerCallback.h>
#include <composer-vts/2.1/TestCommandReader.h>
#include <composer-vts/2.2/ComposerVts.h>
#include <mapper-vts/2.1/MapperVts.h>
#include <renderengine/RenderEngine.h>

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_2 {
namespace vts {

using android::hardware::hidl_handle;
using common::V1_1::BufferUsage;
using common::V1_1::Dataspace;
using common::V1_1::PixelFormat;
using IMapper2_1 = mapper::V2_1::IMapper;
using Gralloc2_1 = mapper::V2_1::vts::Gralloc;
using renderengine::LayerSettings;
using V2_1::Display;
using V2_1::Layer;
using V2_1::vts::AccessRegion;
using V2_1::vts::TestCommandReader;

static const IComposerClient::Color BLACK = {0, 0, 0, 0xff};
static const IComposerClient::Color RED = {0xff, 0, 0, 0xff};
static const IComposerClient::Color TRANSLUCENT_RED = {0xff, 0, 0, 0x33};
static const IComposerClient::Color GREEN = {0, 0xff, 0, 0xff};
static const IComposerClient::Color BLUE = {0, 0, 0xff, 0xff};

class TestLayer {
  public:
    TestLayer(const std::shared_ptr<ComposerClient>& client, Display display)
        : mLayer(client->createLayer(display, kBufferSlotCount)), mComposerClient(client) {}

    // ComposerClient will take care of destroying layers, no need to explicitly
    // call destroyLayers here
    virtual ~TestLayer(){};

    virtual void write(const std::shared_ptr<CommandWriterBase>& writer);
    virtual LayerSettings toRenderEngineLayerSettings();

    void setDisplayFrame(IComposerClient::Rect frame) { mDisplayFrame = frame; }
    void setSourceCrop(IComposerClient::FRect crop) { mSourceCrop = crop; }
    void setZOrder(uint32_t z) { mZOrder = z; }

    void setSurfaceDamage(std::vector<IComposerClient::Rect> surfaceDamage) {
        mSurfaceDamage = surfaceDamage;
    }

    void setTransform(Transform transform) { mTransform = transform; }
    void setAlpha(float alpha) { mAlpha = alpha; }
    void setBlendMode(IComposerClient::BlendMode blendMode) { mBlendMode = blendMode; }

    static constexpr uint32_t kBufferSlotCount = 64;

    IComposerClient::Rect mDisplayFrame = {0, 0, 0, 0};
    uint32_t mZOrder = 0;
    std::vector<IComposerClient::Rect> mSurfaceDamage;
    Transform mTransform = static_cast<Transform>(0);
    IComposerClient::FRect mSourceCrop = {0, 0, 0, 0};
    float mAlpha = 1.0;
    IComposerClient::BlendMode mBlendMode = IComposerClient::BlendMode::NONE;

  protected:
    Layer mLayer;

  private:
    std::shared_ptr<ComposerClient> const mComposerClient;
};

class TestColorLayer : public TestLayer {
  public:
    TestColorLayer(const std::shared_ptr<ComposerClient>& client, Display display)
        : TestLayer{client, display} {}

    void write(const std::shared_ptr<CommandWriterBase>& writer) override;

    LayerSettings toRenderEngineLayerSettings() override;

    void setColor(IComposerClient::Color color) { mColor = color; }

  private:
    IComposerClient::Color mColor = {0xff, 0xff, 0xff, 0xff};
};

class TestBufferLayer : public TestLayer {
  public:
    TestBufferLayer(
            const std::shared_ptr<ComposerClient>& client, const std::shared_ptr<Gralloc>& gralloc,
            Display display, int32_t width, int32_t height, PixelFormat format,
            IComposerClient::Composition composition = IComposerClient::Composition::DEVICE);

    ~TestBufferLayer();

    void write(const std::shared_ptr<CommandWriterBase>& writer) override;

    LayerSettings toRenderEngineLayerSettings() override;

    void fillBuffer(std::vector<IComposerClient::Color> expectedColors);

    void setBuffer(std::vector<IComposerClient::Color> colors);

    void setDataspace(Dataspace dataspace, const std::shared_ptr<CommandWriterBase>& writer);

    void setToClientComposition(const std::shared_ptr<CommandWriterBase>& writer);

    uint32_t mWidth;
    uint32_t mHeight;
    uint32_t mLayerCount;
    PixelFormat mFormat;
    uint64_t mUsage;
    AccessRegion mAccessRegion;
    uint32_t mStride;

  protected:
    IComposerClient::Composition mComposition;
    std::shared_ptr<Gralloc> mGralloc;
    int32_t mFillFence;
    const native_handle_t* mBufferHandle = nullptr;
};

class ReadbackHelper {
  public:
    static std::string getColorModeString(ColorMode mode);

    static std::string getDataspaceString(Dataspace dataspace);

    static Dataspace getDataspaceForColorMode(ColorMode mode);

    static int32_t GetBytesPerPixel(PixelFormat pixelFormat);

    static void fillBuffer(int32_t width, int32_t height, uint32_t stride, void* bufferData,
                           PixelFormat pixelFormat,
                           std::vector<IComposerClient::Color> desiredPixelColors);

    static void clearColors(std::vector<IComposerClient::Color>& expectedColors, int32_t width,
                            int32_t height, int32_t displayWidth);

    static void fillColorsArea(std::vector<IComposerClient::Color>& expectedColors, int32_t stride,
                               IComposerClient::Rect area, IComposerClient::Color color);

    static bool readbackSupported(const PixelFormat& pixelFormat, const Dataspace& dataspace,
                                  const Error error);

    static const std::vector<ColorMode> colorModes;
    static const std::vector<Dataspace> dataspaces;

    static void compareColorBuffers(std::vector<IComposerClient::Color>& expectedColors,
                                    void* bufferData, const uint32_t stride, const uint32_t width,
                                    const uint32_t height, const PixelFormat pixelFormat);
};

class ReadbackBuffer {
  public:
    ReadbackBuffer(Display display, const std::shared_ptr<ComposerClient>& client,
                   const std::shared_ptr<Gralloc>& gralloc, uint32_t width, uint32_t height,
                   PixelFormat pixelFormat, Dataspace dataspace);
    ~ReadbackBuffer();

    void setReadbackBuffer();

    void checkReadbackBuffer(std::vector<IComposerClient::Color> expectedColors);

  protected:
    uint32_t mWidth;
    uint32_t mHeight;
    uint32_t mLayerCount;
    PixelFormat mFormat;
    uint64_t mUsage;
    AccessRegion mAccessRegion;
    uint32_t mStride;
    const native_handle_t* mBufferHandle = nullptr;
    PixelFormat mPixelFormat;
    Dataspace mDataspace;
    Display mDisplay;
    std::shared_ptr<Gralloc> mGralloc;
    std::shared_ptr<ComposerClient> mComposerClient;
};

}  // namespace vts
}  // namespace V2_2
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android
