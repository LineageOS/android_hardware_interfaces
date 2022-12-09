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

#include "ReadbackVts.h"
#include <aidl/android/hardware/graphics/common/BufferUsage.h>
#include "RenderEngineVts.h"
#include "renderengine/ExternalTexture.h"
#include "renderengine/impl/ExternalTexture.h"

using ::android::status_t;

namespace aidl::android::hardware::graphics::composer3::vts {

const std::vector<ColorMode> ReadbackHelper::colorModes = {ColorMode::SRGB, ColorMode::DISPLAY_P3};
const std::vector<Dataspace> ReadbackHelper::dataspaces = {common::Dataspace::SRGB,
                                                           common::Dataspace::DISPLAY_P3};

void TestLayer::write(ComposerClientWriter& writer) {
    ::android::status_t status = ::android::OK;
    ASSERT_EQ(::android::OK, status);

    writer.setLayerDisplayFrame(mDisplay, mLayer, mDisplayFrame);
    writer.setLayerSourceCrop(mDisplay, mLayer, mSourceCrop);
    writer.setLayerZOrder(mDisplay, mLayer, mZOrder);
    writer.setLayerSurfaceDamage(mDisplay, mLayer, mSurfaceDamage);
    writer.setLayerTransform(mDisplay, mLayer, mTransform);
    writer.setLayerPlaneAlpha(mDisplay, mLayer, mAlpha);
    writer.setLayerBlendMode(mDisplay, mLayer, mBlendMode);
    writer.setLayerBrightness(mDisplay, mLayer, mBrightness);
}

bool ReadbackHelper::readbackSupported(const common::PixelFormat& pixelFormat,
                                       const common::Dataspace& dataspace) {
    // TODO: add support for RGBA_1010102
    if (pixelFormat != common::PixelFormat::RGB_888 &&
        pixelFormat != common::PixelFormat::RGBA_8888) {
        return false;
    }
    if (std::find(dataspaces.begin(), dataspaces.end(), dataspace) == dataspaces.end()) {
        return false;
    }
    return true;
}

void ReadbackHelper::createReadbackBuffer(ReadbackBufferAttributes readbackBufferAttributes,
                                          const VtsDisplay& display,
                                          sp<GraphicBuffer>* graphicBuffer) {
    ASSERT_NE(nullptr, graphicBuffer);
    if (!readbackSupported(readbackBufferAttributes.format, readbackBufferAttributes.dataspace)) {
        *graphicBuffer = nullptr;
    }
    uint64_t usage =
            static_cast<uint64_t>(static_cast<uint64_t>(common::BufferUsage::CPU_READ_OFTEN) |
                                  static_cast<uint64_t>(common::BufferUsage::GPU_TEXTURE));

    uint32_t layerCount = 1;
    *graphicBuffer = sp<GraphicBuffer>::make(
            static_cast<uint32_t>(display.getDisplayWidth()),
            static_cast<uint32_t>(display.getDisplayHeight()),
            static_cast<::android::PixelFormat>(readbackBufferAttributes.format), layerCount, usage,
            "ReadbackBuffer");
    ASSERT_NE(nullptr, *graphicBuffer);
    ASSERT_EQ(::android::OK, (*graphicBuffer)->initCheck());
}

std::string ReadbackHelper::getColorModeString(ColorMode mode) {
    switch (mode) {
        case ColorMode::SRGB:
            return {"SRGB"};
        case ColorMode::DISPLAY_P3:
            return {"DISPLAY_P3"};
        default:
            return {"Unsupported color mode for readback"};
    }
}

std::string ReadbackHelper::getDataspaceString(common::Dataspace dataspace) {
    switch (dataspace) {
        case common::Dataspace::SRGB:
            return {"SRGB"};
        case common::Dataspace::DISPLAY_P3:
            return {"DISPLAY_P3"};
        case common::Dataspace::UNKNOWN:
            return {"UNKNOWN"};
        default:
            return {"Unsupported dataspace for readback"};
    }
}

Dataspace ReadbackHelper::getDataspaceForColorMode(ColorMode mode) {
    switch (mode) {
        case ColorMode::DISPLAY_P3:
            return Dataspace::DISPLAY_P3;
        case ColorMode::SRGB:
        default:
            return common::Dataspace::UNKNOWN;
    }
}

LayerSettings TestLayer::toRenderEngineLayerSettings() {
    LayerSettings layerSettings;

    layerSettings.alpha = ::android::half(mAlpha);
    layerSettings.disableBlending = mBlendMode == BlendMode::NONE;
    layerSettings.source.buffer.isOpaque = mBlendMode == BlendMode::NONE;
    layerSettings.geometry.boundaries = ::android::FloatRect(
            static_cast<float>(mDisplayFrame.left), static_cast<float>(mDisplayFrame.top),
            static_cast<float>(mDisplayFrame.right), static_cast<float>(mDisplayFrame.bottom));

    const ::android::mat4 translation = ::android::mat4::translate(::android::vec4(
            (static_cast<uint64_t>(mTransform) & static_cast<uint64_t>(Transform::FLIP_H)
                     ? static_cast<float>(-mDisplayFrame.right)
                     : 0.0f),
            (static_cast<uint64_t>(mTransform) & static_cast<uint64_t>(Transform::FLIP_V)
                     ? static_cast<float>(-mDisplayFrame.bottom)
                     : 0.0f),
            0.0f, 1.0f));

    const ::android::mat4 scale = ::android::mat4::scale(::android::vec4(
            static_cast<uint64_t>(mTransform) & static_cast<uint64_t>(Transform::FLIP_H) ? -1.0f
                                                                                         : 1.0f,
            static_cast<uint64_t>(mTransform) & static_cast<uint64_t>(Transform::FLIP_V) ? -1.0f
                                                                                         : 1.0f,
            1.0f, 1.0f));

    layerSettings.geometry.positionTransform = scale * translation;
    layerSettings.whitePointNits = mWhitePointNits;

    return layerSettings;
}

int32_t ReadbackHelper::GetBytesPerPixel(PixelFormat pixelFormat) {
    switch (pixelFormat) {
        case PixelFormat::RGBA_8888:
            return 4;
        case PixelFormat::RGB_888:
            return 3;
        default:
            return -1;
    }
}

void ReadbackHelper::fillBuffer(uint32_t width, uint32_t height, uint32_t stride, void* bufferData,
                                common::PixelFormat pixelFormat,
                                const std::vector<Color>& desiredColors) {
    ASSERT_TRUE(pixelFormat == common::PixelFormat::RGB_888 ||
                pixelFormat == common::PixelFormat::RGBA_8888);
    int32_t bytesPerPixel = GetBytesPerPixel(pixelFormat);
    ASSERT_NE(-1, bytesPerPixel);
    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            int pixel = row * static_cast<int32_t>(width) + col;
            Color desiredColor = desiredColors[static_cast<size_t>(pixel)];

            int offset = (row * static_cast<int32_t>(stride) + col) * bytesPerPixel;
            uint8_t* pixelColor = (uint8_t*)bufferData + offset;
            pixelColor[0] = static_cast<uint8_t>(std::round(255.0f * desiredColor.r));
            pixelColor[1] = static_cast<uint8_t>(std::round(255.0f * desiredColor.g));
            pixelColor[2] = static_cast<uint8_t>(std::round(255.0f * desiredColor.b));

            if (bytesPerPixel == 4) {
                pixelColor[3] = static_cast<uint8_t>(std::round(255.0f * desiredColor.a));
            }
        }
    }
}

void ReadbackHelper::clearColors(std::vector<Color>& colors, int32_t width, int32_t height,
                                 int32_t displayWidth) {
    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            int pixel = row * displayWidth + col;
            colors[static_cast<size_t>(pixel)] = BLACK;
        }
    }
}

void ReadbackHelper::fillColorsArea(std::vector<Color>& colors, int32_t stride, Rect area,
                                    Color desiredColor) {
    for (int row = area.top; row < area.bottom; row++) {
        for (int col = area.left; col < area.right; col++) {
            int pixel = row * stride + col;
            colors[static_cast<size_t>(pixel)] = desiredColor;
        }
    }
}

void ReadbackHelper::fillBufferAndGetFence(const sp<GraphicBuffer>& graphicBuffer,
                                           Color desiredColor, int* fillFence) {
    ASSERT_NE(nullptr, fillFence);
    std::vector<Color> desiredColors(
            static_cast<size_t>(graphicBuffer->getWidth() * graphicBuffer->getHeight()));
    ::android::Rect bounds = graphicBuffer->getBounds();
    fillColorsArea(desiredColors, static_cast<int32_t>(graphicBuffer->getWidth()),
                   {bounds.left, bounds.top, bounds.right, bounds.bottom}, desiredColor);
    ASSERT_NO_FATAL_FAILURE(fillBufferAndGetFence(graphicBuffer, desiredColors, fillFence));
}

void ReadbackHelper::fillBufferAndGetFence(const sp<GraphicBuffer>& graphicBuffer,
                                           const std::vector<Color>& desiredColors,
                                           int* fillFence) {
    ASSERT_TRUE(graphicBuffer->getPixelFormat() == ::android::PIXEL_FORMAT_RGB_888 ||
                graphicBuffer->getPixelFormat() == ::android::PIXEL_FORMAT_RGBA_8888);
    void* bufData;
    int32_t bytesPerPixel = -1;
    int32_t bytesPerStride = -1;
    status_t status =
            graphicBuffer->lock(GRALLOC_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_SW_WRITE_OFTEN,
                                &bufData, &bytesPerPixel, &bytesPerStride);
    ASSERT_EQ(::android::OK, status);

    const uint32_t stride = (bytesPerPixel > 0 && bytesPerStride > 0)
                                    ? static_cast<uint32_t>(bytesPerStride / bytesPerPixel)
                                    : graphicBuffer->getStride();
    ReadbackHelper::fillBuffer(
            graphicBuffer->getWidth(), graphicBuffer->getHeight(), stride, bufData,
            static_cast<common::PixelFormat>(graphicBuffer->getPixelFormat()), desiredColors);
    status = graphicBuffer->unlockAsync(fillFence);
    ASSERT_EQ(::android::OK, status);
}

void ReadbackHelper::compareColorToBuffer(Color expectedColor,
                                          const sp<GraphicBuffer>& graphicBuffer,
                                          const ndk::ScopedFileDescriptor& fence) {
    std::vector<Color> expectedColors(
            static_cast<size_t>(graphicBuffer->getWidth() * graphicBuffer->getHeight()));
    ::android::Rect bounds = graphicBuffer->getBounds();
    fillColorsArea(expectedColors, static_cast<int32_t>(graphicBuffer->getWidth()),
                   {bounds.left, bounds.top, bounds.right, bounds.bottom}, expectedColor);
    compareColorsToBuffer(expectedColors, graphicBuffer, fence);
}

void ReadbackHelper::compareColorsToBuffer(const std::vector<Color>& expectedColors,
                                           const sp<GraphicBuffer>& graphicBuffer,
                                           const ndk::ScopedFileDescriptor& fence) {
    ASSERT_TRUE(graphicBuffer->getPixelFormat() == ::android::PIXEL_FORMAT_RGB_888 ||
                graphicBuffer->getPixelFormat() == ::android::PIXEL_FORMAT_RGBA_8888);

    int bytesPerPixel = -1;
    int bytesPerStride = -1;
    void* bufData = nullptr;
    status_t status = graphicBuffer->lockAsync(GRALLOC_USAGE_SW_READ_OFTEN, &bufData,
                                               dup(fence.get()), &bytesPerPixel, &bytesPerStride);
    ASSERT_EQ(::android::OK, status);

    const uint32_t stride = (bytesPerPixel > 0 && bytesPerStride > 0)
                                    ? static_cast<uint32_t>(bytesPerStride / bytesPerPixel)
                                    : graphicBuffer->getStride();

    if (bytesPerPixel == -1) {
        bytesPerPixel = ReadbackHelper::GetBytesPerPixel(
                static_cast<PixelFormat>(graphicBuffer->getPixelFormat()));
    }
    ASSERT_NE(-1, bytesPerPixel);
    for (int row = 0; row < graphicBuffer->getHeight(); row++) {
        for (int col = 0; col < graphicBuffer->getWidth(); col++) {
            int pixel = row * static_cast<int32_t>(graphicBuffer->getWidth()) + col;
            int offset = (row * static_cast<int32_t>(stride) + col) * bytesPerPixel;
            uint8_t* pixelColor = (uint8_t*)bufData + offset;
            const Color expectedColor = expectedColors[static_cast<size_t>(pixel)];
            ASSERT_EQ(std::round(255.0f * expectedColor.r), pixelColor[0]);
            ASSERT_EQ(std::round(255.0f * expectedColor.g), pixelColor[1]);
            ASSERT_EQ(std::round(255.0f * expectedColor.b), pixelColor[2]);
        }
    }

    status = graphicBuffer->unlock();
    ASSERT_EQ(::android::OK, status);
}

ReadbackBuffer::ReadbackBuffer(int64_t display, const std::shared_ptr<VtsComposerClient>& client,
                               int32_t width, int32_t height, common::PixelFormat pixelFormat)
    : mComposerClient(client) {
    mDisplay = display;
    mWidth = static_cast<uint32_t>(width);
    mHeight = static_cast<uint32_t>(height);
    mPixelFormat = pixelFormat;
    mLayerCount = 1;
    mUsage = static_cast<uint64_t>(static_cast<uint64_t>(common::BufferUsage::CPU_READ_OFTEN) |
                                   static_cast<uint64_t>(common::BufferUsage::GPU_TEXTURE));
}

void ReadbackBuffer::setReadbackBuffer() {
    mGraphicBuffer = sp<GraphicBuffer>::make(mWidth, mHeight,
                                             static_cast<::android::PixelFormat>(mPixelFormat),
                                             mLayerCount, mUsage, "ReadbackBuffer");
    ASSERT_NE(nullptr, mGraphicBuffer);
    ASSERT_EQ(::android::OK, mGraphicBuffer->initCheck());
    ::ndk::ScopedFileDescriptor noFence = ::ndk::ScopedFileDescriptor(-1);
    const auto& status =
            mComposerClient->setReadbackBuffer(mDisplay, mGraphicBuffer->handle, noFence);
    ASSERT_TRUE(status.isOk());
}

void ReadbackBuffer::checkReadbackBuffer(const std::vector<Color>& expectedColors) {
    // lock buffer for reading
    const auto& [fenceStatus, bufferFence] = mComposerClient->getReadbackBufferFence(mDisplay);
    ASSERT_TRUE(fenceStatus.isOk());
    ReadbackHelper::compareColorsToBuffer(expectedColors, mGraphicBuffer, bufferFence);
}

void TestColorLayer::write(ComposerClientWriter& writer) {
    TestLayer::write(writer);
    writer.setLayerCompositionType(mDisplay, mLayer, Composition::SOLID_COLOR);
    writer.setLayerColor(mDisplay, mLayer, mColor);
}

LayerSettings TestColorLayer::toRenderEngineLayerSettings() {
    LayerSettings layerSettings = TestLayer::toRenderEngineLayerSettings();

    layerSettings.source.solidColor = ::android::half3(mColor.r, mColor.g, mColor.b);
    layerSettings.alpha = mAlpha * mColor.a;
    return layerSettings;
}

TestBufferLayer::TestBufferLayer(const std::shared_ptr<VtsComposerClient>& client,
                                 TestRenderEngine& renderEngine, int64_t display, uint32_t width,
                                 uint32_t height, common::PixelFormat format,
                                 Composition composition)
    : TestLayer{client, display}, mRenderEngine(renderEngine) {
    mComposition = composition;
    mWidth = width;
    mHeight = height;
    mLayerCount = 1;
    mPixelFormat = format;
    mUsage = (static_cast<uint64_t>(common::BufferUsage::CPU_READ_OFTEN) |
              static_cast<uint64_t>(common::BufferUsage::CPU_WRITE_OFTEN) |
              static_cast<uint64_t>(common::BufferUsage::COMPOSER_OVERLAY) |
              static_cast<uint64_t>(common::BufferUsage::GPU_TEXTURE));

    mAccessRegion.top = 0;
    mAccessRegion.left = 0;
    mAccessRegion.right = static_cast<int32_t>(width);
    mAccessRegion.bottom = static_cast<int32_t>(height);

    setSourceCrop({0, 0, (float)width, (float)height});
}

void TestBufferLayer::write(ComposerClientWriter& writer) {
    TestLayer::write(writer);
    writer.setLayerCompositionType(mDisplay, mLayer, mComposition);
    writer.setLayerVisibleRegion(mDisplay, mLayer, std::vector<Rect>(1, mDisplayFrame));
    if (mGraphicBuffer) {
        writer.setLayerBuffer(mDisplay, mLayer, /*slot*/ 0, mGraphicBuffer->handle, mFillFence);
    }
}

LayerSettings TestBufferLayer::toRenderEngineLayerSettings() {
    LayerSettings layerSettings = TestLayer::toRenderEngineLayerSettings();
    layerSettings.source.buffer.buffer =
            std::make_shared<::android::renderengine::impl::ExternalTexture>(
                    mGraphicBuffer, mRenderEngine.getInternalRenderEngine(),
                    ::android::renderengine::impl::ExternalTexture::Usage::READABLE);

    layerSettings.source.buffer.usePremultipliedAlpha = mBlendMode == BlendMode::PREMULTIPLIED;

    const float scaleX = (mSourceCrop.right - mSourceCrop.left) / (static_cast<float>(mWidth));
    const float scaleY = (mSourceCrop.bottom - mSourceCrop.top) / (static_cast<float>(mHeight));
    const float translateX = mSourceCrop.left / (static_cast<float>(mWidth));
    const float translateY = mSourceCrop.top / (static_cast<float>(mHeight));

    layerSettings.source.buffer.textureTransform =
            ::android::mat4::translate(::android::vec4(translateX, translateY, 0.0f, 1.0f)) *
            ::android::mat4::scale(::android::vec4(scaleX, scaleY, 1.0f, 1.0f));

    return layerSettings;
}

void TestBufferLayer::fillBuffer(std::vector<Color>& expectedColors) {
    void* bufData;
    int32_t bytesPerPixel = -1;
    int32_t bytesPerStride = -1;
    auto status = mGraphicBuffer->lock(mUsage, &bufData, &bytesPerPixel, &bytesPerStride);
    const uint32_t stride = (bytesPerPixel > 0 && bytesPerStride > 0)
                                    ? static_cast<uint32_t>(bytesPerStride / bytesPerPixel)
                                    : mGraphicBuffer->getStride();
    ASSERT_EQ(::android::OK, status);
    ReadbackHelper::fillBuffer(mWidth, mHeight, stride, bufData, mPixelFormat, expectedColors);

    const auto unlockStatus = mGraphicBuffer->unlockAsync(&mFillFence);
    ASSERT_EQ(::android::OK, unlockStatus);
}

void TestBufferLayer::setBuffer(std::vector<Color> colors) {
    mGraphicBuffer = allocateBuffer();
    ASSERT_NE(nullptr, mGraphicBuffer);
    ASSERT_EQ(::android::OK, mGraphicBuffer->initCheck());
    fillBuffer(colors);
}

sp<GraphicBuffer> TestBufferLayer::allocateBuffer() {
    return sp<GraphicBuffer>::make(mWidth, mHeight,
                                   static_cast<::android::PixelFormat>(mPixelFormat), mLayerCount,
                                   mUsage, "TestBufferLayer");
}

void TestBufferLayer::setDataspace(common::Dataspace dataspace, ComposerClientWriter& writer) {
    writer.setLayerDataspace(mDisplay, mLayer, dataspace);
}

void TestBufferLayer::setToClientComposition(ComposerClientWriter& writer) {
    writer.setLayerCompositionType(mDisplay, mLayer, Composition::CLIENT);
}

}  // namespace aidl::android::hardware::graphics::composer3::vts
