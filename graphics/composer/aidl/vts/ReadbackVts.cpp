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

namespace aidl::android::hardware::graphics::composer3::vts {

const std::vector<ColorMode> ReadbackHelper::colorModes = {ColorMode::SRGB, ColorMode::DISPLAY_P3};
const std::vector<Dataspace> ReadbackHelper::dataspaces = {common::Dataspace::SRGB,
                                                           common::Dataspace::DISPLAY_P3};

void TestLayer::write(ComposerClientWriter& writer) {
    writer.setLayerDisplayFrame(mDisplay, mLayer, mDisplayFrame);
    writer.setLayerSourceCrop(mDisplay, mLayer, mSourceCrop);
    writer.setLayerZOrder(mDisplay, mLayer, mZOrder);
    writer.setLayerSurfaceDamage(mDisplay, mLayer, mSurfaceDamage);
    writer.setLayerTransform(mDisplay, mLayer, mTransform);
    writer.setLayerPlaneAlpha(mDisplay, mLayer, mAlpha);
    writer.setLayerBlendMode(mDisplay, mLayer, mBlendMode);
    writer.setLayerBrightness(mDisplay, mLayer, mBrightness);
    writer.setLayerDataspace(mDisplay, mLayer, mDataspace);
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
    layerSettings.sourceDataspace = static_cast<::android::ui::Dataspace>(mDataspace);

    return layerSettings;
}

int32_t ReadbackHelper::GetBytesPerPixel(common::PixelFormat pixelFormat) {
    switch (pixelFormat) {
        case common::PixelFormat::RGBA_8888:
            return 4;
        case common::PixelFormat::RGB_888:
            return 3;
        default:
            return -1;
    }
}

void ReadbackHelper::fillBuffer(uint32_t width, uint32_t height, uint32_t stride, void* bufferData,
                                common::PixelFormat pixelFormat,
                                std::vector<Color> desiredPixelColors) {
    ASSERT_TRUE(pixelFormat == common::PixelFormat::RGB_888 ||
                pixelFormat == common::PixelFormat::RGBA_8888);
    int32_t bytesPerPixel = GetBytesPerPixel(pixelFormat);
    ASSERT_NE(-1, bytesPerPixel);
    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            auto pixel = row * static_cast<int32_t>(width) + col;
            Color srcColor = desiredPixelColors[static_cast<size_t>(pixel)];

            int offset = (row * static_cast<int32_t>(stride) + col) * bytesPerPixel;
            uint8_t* pixelColor = (uint8_t*)bufferData + offset;
            pixelColor[0] = static_cast<uint8_t>(std::round(255.0f * srcColor.r));
            pixelColor[1] = static_cast<uint8_t>(std::round(255.0f * srcColor.g));
            pixelColor[2] = static_cast<uint8_t>(std::round(255.0f * srcColor.b));

            if (bytesPerPixel == 4) {
                pixelColor[3] = static_cast<uint8_t>(std::round(255.0f * srcColor.a));
            }
        }
    }
}

void ReadbackHelper::clearColors(std::vector<Color>& expectedColors, int32_t width, int32_t height,
                                 int32_t displayWidth) {
    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            int pixel = row * displayWidth + col;
            expectedColors[static_cast<size_t>(pixel)] = BLACK;
        }
    }
}

void ReadbackHelper::fillColorsArea(std::vector<Color>& expectedColors, int32_t stride, Rect area,
                                    Color color) {
    for (int row = area.top; row < area.bottom; row++) {
        for (int col = area.left; col < area.right; col++) {
            int pixel = row * stride + col;
            expectedColors[static_cast<size_t>(pixel)] = color;
        }
    }
}

bool ReadbackHelper::readbackSupported(const common::PixelFormat& pixelFormat,
                                       const common::Dataspace& dataspace) {
    if (pixelFormat != common::PixelFormat::RGB_888 &&
        pixelFormat != common::PixelFormat::RGBA_8888) {
        return false;
    }
    if (std::find(dataspaces.begin(), dataspaces.end(), dataspace) == dataspaces.end()) {
        return false;
    }
    return true;
}

void ReadbackHelper::compareColorBuffers(const std::vector<Color>& expectedColors, void* bufferData,
                                         const uint32_t stride, const uint32_t width,
                                         const uint32_t height, common::PixelFormat pixelFormat) {
    const int32_t bytesPerPixel = ReadbackHelper::GetBytesPerPixel(pixelFormat);
    ASSERT_NE(-1, bytesPerPixel);
    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            auto pixel = row * static_cast<int32_t>(width) + col;
            int offset = (row * static_cast<int32_t>(stride) + col) * bytesPerPixel;
            uint8_t* pixelColor = (uint8_t*)bufferData + offset;
            const Color expectedColor = expectedColors[static_cast<size_t>(pixel)];
            ASSERT_EQ(std::round(255.0f * expectedColor.r), pixelColor[0]);
            ASSERT_EQ(std::round(255.0f * expectedColor.g), pixelColor[1]);
            ASSERT_EQ(std::round(255.0f * expectedColor.b), pixelColor[2]);
        }
    }
}

void ReadbackHelper::compareColorBuffers(void* expectedBuffer, void* actualBuffer,
                                         const uint32_t stride, const uint32_t width,
                                         const uint32_t height, common::PixelFormat pixelFormat) {
    const int32_t bytesPerPixel = ReadbackHelper::GetBytesPerPixel(pixelFormat);
    ASSERT_NE(-1, bytesPerPixel);
    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            int offset = (row * static_cast<int32_t>(stride) + col) * bytesPerPixel;
            uint8_t* expectedColor = (uint8_t*)expectedBuffer + offset;
            uint8_t* actualColor = (uint8_t*)actualBuffer + offset;
            ASSERT_EQ(expectedColor[0], actualColor[0]);
            ASSERT_EQ(expectedColor[1], actualColor[1]);
            ASSERT_EQ(expectedColor[2], actualColor[2]);
        }
    }
}

ReadbackBuffer::ReadbackBuffer(int64_t display, const std::shared_ptr<VtsComposerClient>& client,
                               int32_t width, int32_t height, common::PixelFormat pixelFormat,
                               common::Dataspace dataspace)
    : mComposerClient(client) {
    mDisplay = display;

    mPixelFormat = pixelFormat;
    mDataspace = dataspace;

    mWidth = static_cast<uint32_t>(width);
    mHeight = static_cast<uint32_t>(height);
    mLayerCount = 1;
    mUsage = static_cast<uint64_t>(static_cast<uint64_t>(common::BufferUsage::CPU_READ_OFTEN) |
                                   static_cast<uint64_t>(common::BufferUsage::GPU_TEXTURE));

    mAccessRegion.top = 0;
    mAccessRegion.left = 0;
    mAccessRegion.right = static_cast<int32_t>(width);
    mAccessRegion.bottom = static_cast<int32_t>(height);
}

::android::sp<::android::GraphicBuffer> ReadbackBuffer::allocateBuffer() {
    return ::android::sp<::android::GraphicBuffer>::make(
            mWidth, mHeight, static_cast<::android::PixelFormat>(mPixelFormat), mLayerCount, mUsage,
            "ReadbackBuffer");
}

void ReadbackBuffer::setReadbackBuffer() {
    mGraphicBuffer = allocateBuffer();
    ASSERT_NE(nullptr, mGraphicBuffer);
    ASSERT_EQ(::android::OK, mGraphicBuffer->initCheck());
    const auto& bufferHandle = mGraphicBuffer->handle;
    ::ndk::ScopedFileDescriptor fence = ::ndk::ScopedFileDescriptor(-1);
    EXPECT_TRUE(mComposerClient->setReadbackBuffer(mDisplay, bufferHandle, fence).isOk());
}

void ReadbackBuffer::checkReadbackBuffer(const std::vector<Color>& expectedColors) {
    ASSERT_NE(nullptr, mGraphicBuffer);
    // lock buffer for reading
    const auto& [fenceStatus, bufferFence] = mComposerClient->getReadbackBufferFence(mDisplay);
    EXPECT_TRUE(fenceStatus.isOk());

    int bytesPerPixel = -1;
    int bytesPerStride = -1;
    void* bufData = nullptr;

    auto status = mGraphicBuffer->lockAsync(mUsage, mAccessRegion, &bufData, dup(bufferFence.get()),
                                            &bytesPerPixel, &bytesPerStride);
    EXPECT_EQ(::android::OK, status);
    ASSERT_TRUE(mPixelFormat == PixelFormat::RGB_888 || mPixelFormat == PixelFormat::RGBA_8888);
    const uint32_t stride = (bytesPerPixel > 0 && bytesPerStride > 0)
                                    ? static_cast<uint32_t>(bytesPerStride / bytesPerPixel)
                                    : mGraphicBuffer->getStride();
    ReadbackHelper::compareColorBuffers(expectedColors, bufData, stride, mWidth, mHeight,
                                        mPixelFormat);
    status = mGraphicBuffer->unlock();
    EXPECT_EQ(::android::OK, status);
}

::android::sp<::android::GraphicBuffer> ReadbackBuffer::getBuffer() {
    const auto& [fenceStatus, bufferFence] = mComposerClient->getReadbackBufferFence(mDisplay);
    EXPECT_TRUE(fenceStatus.isOk());
    if (bufferFence.get() != -1) {
        sync_wait(bufferFence.get(), -1);
    }
    return mGraphicBuffer;
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
    EXPECT_EQ(::android::OK, status);
    ASSERT_NO_FATAL_FAILURE(ReadbackHelper::fillBuffer(mWidth, mHeight, stride, bufData,
                                                       mPixelFormat, expectedColors));

    const auto unlockStatus = mGraphicBuffer->unlockAsync(&mFillFence);
    ASSERT_EQ(::android::OK, unlockStatus);
}

void TestBufferLayer::setBuffer(std::vector<Color> colors) {
    mGraphicBuffer = allocateBuffer();
    ASSERT_NE(nullptr, mGraphicBuffer);
    ASSERT_EQ(::android::OK, mGraphicBuffer->initCheck());
    ASSERT_NO_FATAL_FAILURE(fillBuffer(colors));
}

::android::sp<::android::GraphicBuffer> TestBufferLayer::allocateBuffer() {
    return ::android::sp<::android::GraphicBuffer>::make(
            mWidth, mHeight, static_cast<::android::PixelFormat>(mPixelFormat), mLayerCount, mUsage,
            "TestBufferLayer");
}

void TestBufferLayer::setToClientComposition(ComposerClientWriter& writer) {
    writer.setLayerCompositionType(mDisplay, mLayer, Composition::CLIENT);
}

}  // namespace aidl::android::hardware::graphics::composer3::vts
