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
#include <aidl/android/hardware/graphics/composer3/Luts.h>
#include <android-base/unique_fd.h>
#include <android/hardware/graphics/composer3/ComposerClientReader.h>
#include <android/hardware/graphics/composer3/ComposerClientWriter.h>
#include <renderengine/RenderEngine.h>
#include <ui/GraphicBuffer.h>
#include <ui/PixelFormat.h>
#include <memory>
#include "ComposerClientWrapper.h"
#include "RenderEngine.h"
#include "TestLayer.h"

using aidl::android::hardware::graphics::composer3::Luts;

namespace aidl::android::hardware::graphics::composer3::libhwc_aidl_test {

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

class TestColorLayer : public TestLayer {
  public:
    TestColorLayer(const std::shared_ptr<ComposerClientWrapper>& client, int64_t display,
                   ComposerClientWriter& writer)
        : TestLayer{client, display, writer} {}

    void write(ComposerClientWriter& writer) override;

    LayerSettings toRenderEngineLayerSettings() override;

    void setColor(Color color) { mColor = color; }

  private:
    Color mColor = WHITE;
};

class TestBufferLayer : public TestLayer {
  public:
    TestBufferLayer(const std::shared_ptr<ComposerClientWrapper>& client,
                    TestRenderEngine& renderEngine, int64_t display, uint32_t width,
                    uint32_t height, common::PixelFormat format, ComposerClientWriter& writer,
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

struct DisplayProperties {
    DisplayProperties(int64_t displayId, std::vector<ColorMode> testColorModes,
                      std::unique_ptr<TestRenderEngine> testRenderEngine,
                      ::android::renderengine::DisplaySettings clientCompositionDisplaySettings,
                      common::PixelFormat pixelFormat, common::Dataspace dataspace)
        : testColorModes(testColorModes),
          pixelFormat(pixelFormat),
          dataspace(dataspace),
          testRenderEngine(std::move(testRenderEngine)),
          clientCompositionDisplaySettings(std::move(clientCompositionDisplaySettings)),
          writer(displayId),
          reader(displayId) {}

    std::vector<ColorMode> testColorModes = {};
    common::PixelFormat pixelFormat = common::PixelFormat::UNSPECIFIED;
    common::Dataspace dataspace = common::Dataspace::UNKNOWN;
    std::unique_ptr<TestRenderEngine> testRenderEngine = nullptr;
    ::android::renderengine::DisplaySettings clientCompositionDisplaySettings = {};
    ComposerClientWriter writer;
    ComposerClientReader reader;
};

class ReadbackHelper {
  public:
    static DisplayProperties setupDisplayProperty(
            const DisplayWrapper& display,
            const std::shared_ptr<ComposerClientWrapper>& composerClient);

    static std::string getColorModeString(ColorMode mode);

    static std::string getDataspaceString(Dataspace dataspace);

    static Dataspace getDataspaceForColorMode(ColorMode mode);

    static int32_t GetBitsPerChannel(PixelFormat pixelFormat);
    static int32_t GetTolerance(int32_t bitsPerChannel);
    static int32_t GetAlphaBits(PixelFormat pixelFormat);

    static void fillBuffer(uint32_t width, uint32_t height, uint32_t stride, int32_t bytesPerPixel,
                           void* bufferData, PixelFormat pixelFormat,
                           std::vector<Color> desiredPixelColors);

    static void clearColors(std::vector<Color>& expectedColors, int32_t width, int32_t height,
                            int32_t displayWidth);

    static void fillColorsArea(std::vector<Color>& expectedColors, int32_t stride, Rect area,
                               Color color);

    static bool readbackSupported(const PixelFormat& pixelFormat, const Dataspace& dataspace);

    static const std::vector<ColorMode> colorModes;
    static const std::vector<Dataspace> dataspaces;

    static void compareColorBuffers(const std::vector<Color>& expectedColors, void* bufferData,
                                    const uint32_t stride, int32_t bytesPerPixel,
                                    const uint32_t width, const uint32_t height,
                                    PixelFormat pixelFormat);
    static void compareColorBuffers(void* expectedBuffer, void* actualBuffer, const uint32_t stride,
                                    int32_t bytesPerPixel, const uint32_t width,
                                    const uint32_t height, PixelFormat pixelFormat);
};

class ReadbackBuffer {
  public:
    ReadbackBuffer(int64_t display, const std::shared_ptr<ComposerClientWrapper>& client,
                   int32_t width, int32_t height, common::PixelFormat pixelFormat,
                   common::Dataspace dataspace);

    void setReadbackBuffer();

    void checkReadbackBuffer(const std::vector<Color>& expectedColors, bool saveImage = false);

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
    std::shared_ptr<ComposerClientWrapper> mComposerClient;
    ::android::Rect mAccessRegion;
    native_handle_t mBufferHandle;

  private:
    ::android::sp<::android::GraphicBuffer> allocateBuffer();
};

}  // namespace aidl::android::hardware::graphics::composer3::libhwc_aidl_test
