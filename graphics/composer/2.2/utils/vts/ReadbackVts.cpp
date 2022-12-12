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
#include <composer-vts/2.2/RenderEngineVts.h>
#include "renderengine/ExternalTexture.h"
#include "renderengine/impl/ExternalTexture.h"

using ::android::status_t;

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_2 {
namespace vts {

void TestLayer::write(const std::shared_ptr<CommandWriterBase>& writer) {
    writer->selectLayer(mLayer);
    writer->setLayerDisplayFrame(mDisplayFrame);
    writer->setLayerSourceCrop(mSourceCrop);
    writer->setLayerZOrder(mZOrder);
    writer->setLayerSurfaceDamage(mSurfaceDamage);
    writer->setLayerTransform(mTransform);
    writer->setLayerPlaneAlpha(mAlpha);
    writer->setLayerBlendMode(mBlendMode);
}

const std::vector<ColorMode> ReadbackHelper::colorModes = {ColorMode::SRGB, ColorMode::DISPLAY_P3};
const std::vector<Dataspace> ReadbackHelper::dataspaces = {Dataspace::V0_SRGB,
                                                           Dataspace::DISPLAY_P3};

std::string ReadbackHelper::getColorModeString(ColorMode mode) {
    switch (mode) {
        case ColorMode::SRGB:
            return std::string("SRGB");
        case ColorMode::DISPLAY_P3:
            return std::string("DISPLAY_P3");
        default:
            return std::string("Unsupported color mode for readback");
    }
}

std::string ReadbackHelper::getDataspaceString(Dataspace dataspace) {
    switch (dataspace) {
        case Dataspace::V0_SRGB:
            return std::string("V0_SRGB");
        case Dataspace::DISPLAY_P3:
            return std::string("DISPLAY_P3");
        case Dataspace::UNKNOWN:
            return std::string("UNKNOWN");
        default:
            return std::string("Unsupported dataspace for readback");
    }
}

Dataspace ReadbackHelper::getDataspaceForColorMode(ColorMode mode) {
    switch (mode) {
        case ColorMode::DISPLAY_P3:
            return Dataspace::DISPLAY_P3;
        case ColorMode::SRGB:
        default:
            return Dataspace::UNKNOWN;
    }
}

LayerSettings TestLayer::toRenderEngineLayerSettings() {
    LayerSettings layerSettings;

    layerSettings.alpha = half(mAlpha);
    layerSettings.disableBlending = mBlendMode == IComposerClient::BlendMode::NONE;
    layerSettings.geometry.boundaries = FloatRect(
            static_cast<float>(mDisplayFrame.left), static_cast<float>(mDisplayFrame.top),
            static_cast<float>(mDisplayFrame.right), static_cast<float>(mDisplayFrame.bottom));

    const mat4 translation = mat4::translate(
            vec4((mTransform & Transform::FLIP_H ? -mDisplayFrame.right : 0.0f),
                 (mTransform & Transform::FLIP_V ? -mDisplayFrame.bottom : 0.0f), 0.0f, 1.0f));

    const mat4 scale = mat4::scale(vec4(mTransform & Transform::FLIP_H ? -1.0f : 1.0f,
                                        mTransform & Transform::FLIP_V ? -1.0f : 1.0f, 1.0f, 1.0f));

    layerSettings.geometry.positionTransform = scale * translation;

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

void ReadbackHelper::fillBufferAndGetFence(const sp<GraphicBuffer>& graphicBuffer,
                                           IComposerClient::Color desiredColor, int* fillFence) {
    ASSERT_NE(nullptr, fillFence);
    std::vector<IComposerClient::Color> desiredColors(
            static_cast<size_t>(graphicBuffer->getWidth() * graphicBuffer->getHeight()));
    ::android::Rect bounds = graphicBuffer->getBounds();
    fillColorsArea(desiredColors, static_cast<int32_t>(graphicBuffer->getWidth()),
                   {bounds.left, bounds.top, bounds.right, bounds.bottom}, desiredColor);
    ASSERT_NO_FATAL_FAILURE(fillBufferAndGetFence(graphicBuffer, desiredColors, fillFence));
}

void ReadbackHelper::fillBufferAndGetFence(const sp<GraphicBuffer>& graphicBuffer,
                                           const std::vector<IComposerClient::Color>& desiredColors,
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
    ReadbackHelper::fillBuffer(graphicBuffer->getWidth(), graphicBuffer->getHeight(), stride,
                               bufData, static_cast<PixelFormat>(graphicBuffer->getPixelFormat()),
                               desiredColors);
    status = graphicBuffer->unlockAsync(fillFence);
    ASSERT_EQ(::android::OK, status);
}

void ReadbackHelper::fillBuffer(int32_t width, int32_t height, uint32_t stride, void* bufferData,
                                PixelFormat pixelFormat,
                                std::vector<IComposerClient::Color> desiredPixelColors) {
    ASSERT_TRUE(pixelFormat == PixelFormat::RGB_888 || pixelFormat == PixelFormat::RGBA_8888);
    int32_t bytesPerPixel = GetBytesPerPixel(pixelFormat);
    ASSERT_NE(-1, bytesPerPixel);
    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            int pixel = row * width + col;
            IComposerClient::Color desiredColor = desiredPixelColors[pixel];

            int offset = (row * stride + col) * bytesPerPixel;
            uint8_t* pixelColor = (uint8_t*)bufferData + offset;
            pixelColor[0] = desiredColor.r;
            pixelColor[1] = desiredColor.g;
            pixelColor[2] = desiredColor.b;

            if (bytesPerPixel == 4) {
                pixelColor[3] = desiredColor.a;
            }
        }
    }
}

void ReadbackHelper::clearColors(std::vector<IComposerClient::Color>& expectedColors, int32_t width,
                                 int32_t height, int32_t displayWidth) {
    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            int pixel = row * displayWidth + col;
            expectedColors[pixel] = BLACK;
        }
    }
}

void ReadbackHelper::fillColorsArea(std::vector<IComposerClient::Color>& expectedColors,
                                    int32_t stride, IComposerClient::Rect area,
                                    IComposerClient::Color color) {
    for (int row = area.top; row < area.bottom; row++) {
        for (int col = area.left; col < area.right; col++) {
            int pixel = row * stride + col;
            expectedColors[pixel] = color;
        }
    }
}

bool ReadbackHelper::readbackSupported(PixelFormat pixelFormat, Dataspace dataspace, Error error) {
    if (error != Error::NONE) {
        return false;
    }
    return readbackSupported(pixelFormat, dataspace);
}

bool ReadbackHelper::readbackSupported(PixelFormat pixelFormat, Dataspace dataspace) {
    if (pixelFormat != PixelFormat::RGB_888 && pixelFormat != PixelFormat::RGBA_8888) {
        return false;
    }
    if (std::find(dataspaces.begin(), dataspaces.end(), dataspace) == dataspaces.end()) {
        return false;
    }
    return true;
}

void ReadbackHelper::createReadbackBuffer(uint32_t width, uint32_t height, PixelFormat pixelFormat,
                                          Dataspace dataspace, sp<GraphicBuffer>* graphicBuffer) {
    ASSERT_NE(nullptr, graphicBuffer);
    if (!readbackSupported(pixelFormat, dataspace)) {
        *graphicBuffer = nullptr;
    }
    android::PixelFormat bufferFormat = static_cast<android::PixelFormat>(pixelFormat);
    uint32_t layerCount = 1;
    uint64_t usage = static_cast<uint64_t>(static_cast<uint64_t>(BufferUsage::CPU_READ_OFTEN) |
                                           static_cast<uint64_t>(BufferUsage::GPU_TEXTURE));
    *graphicBuffer = sp<GraphicBuffer>::make(width, height, bufferFormat, layerCount, usage,
                                             "ReadbackBuffer");
    ASSERT_NE(nullptr, *graphicBuffer);
    ASSERT_EQ(::android::OK, (*graphicBuffer)->initCheck());
}

void ReadbackHelper::compareColorToBuffer(IComposerClient::Color expectedColor,
                                          const sp<GraphicBuffer>& graphicBuffer, int32_t fence) {
    std::vector<IComposerClient::Color> expectedColors(
            static_cast<size_t>(graphicBuffer->getWidth() * graphicBuffer->getHeight()));
    ::android::Rect bounds = graphicBuffer->getBounds();
    fillColorsArea(expectedColors, static_cast<int32_t>(graphicBuffer->getWidth()),
                   {bounds.left, bounds.top, bounds.right, bounds.bottom}, expectedColor);
    compareColorsToBuffer(expectedColors, graphicBuffer, fence);
}

void ReadbackHelper::compareColorsToBuffer(std::vector<IComposerClient::Color>& expectedColors,
                                           const sp<GraphicBuffer>& graphicBuffer, int32_t fence) {
    ASSERT_TRUE(graphicBuffer->getPixelFormat() == ::android::PIXEL_FORMAT_RGB_888 ||
                graphicBuffer->getPixelFormat() == ::android::PIXEL_FORMAT_RGBA_8888);

    int bytesPerPixel = -1;
    int bytesPerStride = -1;
    void* bufData = nullptr;
    status_t status = graphicBuffer->lockAsync(GRALLOC_USAGE_SW_READ_OFTEN, &bufData, fence,
                                               &bytesPerPixel, &bytesPerStride);
    ASSERT_EQ(::android::OK, status);

    const uint32_t stride = (bytesPerPixel > 0 && bytesPerStride > 0)
                                    ? static_cast<uint32_t>(bytesPerStride / bytesPerPixel)
                                    : graphicBuffer->getStride();

    if (bytesPerPixel == -1) {
        PixelFormat pixelFormat = static_cast<PixelFormat>(graphicBuffer->getPixelFormat());
        bytesPerPixel = ReadbackHelper::GetBytesPerPixel(pixelFormat);
    }
    ASSERT_NE(-1, bytesPerPixel);
    for (int row = 0; row < graphicBuffer->getHeight(); row++) {
        for (int col = 0; col < graphicBuffer->getWidth(); col++) {
            int pixel = row * static_cast<int32_t>(graphicBuffer->getWidth()) + col;
            int offset = (row * static_cast<int32_t>(stride) + col) * bytesPerPixel;
            uint8_t* pixelColor = (uint8_t*)bufData + offset;
            const IComposerClient::Color expectedColor = expectedColors[static_cast<size_t>(pixel)];
            ASSERT_EQ(std::round(255.0f * expectedColor.r), pixelColor[0]);
            ASSERT_EQ(std::round(255.0f * expectedColor.g), pixelColor[1]);
            ASSERT_EQ(std::round(255.0f * expectedColor.b), pixelColor[2]);
        }
    }

    status = graphicBuffer->unlock();
    ASSERT_EQ(::android::OK, status);
}

ReadbackBuffer::ReadbackBuffer(Display display, const std::shared_ptr<ComposerClient>& client,
                               uint32_t width, uint32_t height, PixelFormat pixelFormat) {
    mDisplay = display;
    mComposerClient = client;
    mWidth = width;
    mHeight = height;
    mPixelFormat = pixelFormat;
    mLayerCount = 1;
    mUsage = static_cast<uint64_t>(BufferUsage::CPU_READ_OFTEN | BufferUsage::GPU_TEXTURE);
}

void ReadbackBuffer::setReadbackBuffer() {
    mGraphicBuffer = sp<GraphicBuffer>::make(mWidth, mHeight,
                                             static_cast<::android::PixelFormat>(mPixelFormat),
                                             mLayerCount, mUsage, "ReadbackBuffer");
    ASSERT_NE(nullptr, mGraphicBuffer);
    ASSERT_EQ(::android::OK, mGraphicBuffer->initCheck());
    mComposerClient->setReadbackBuffer(mDisplay, mGraphicBuffer->handle, -1 /* fence */);
}

void ReadbackBuffer::checkReadbackBuffer(std::vector<IComposerClient::Color> expectedColors) {
    // lock buffer for reading
    int32_t fenceHandle;
    ASSERT_NO_FATAL_FAILURE(mComposerClient->getReadbackBufferFence(mDisplay, &fenceHandle));
    ReadbackHelper::compareColorsToBuffer(expectedColors, mGraphicBuffer, fenceHandle);
}

void TestColorLayer::write(const std::shared_ptr<CommandWriterBase>& writer) {
    TestLayer::write(writer);
    writer->setLayerCompositionType(IComposerClient::Composition::SOLID_COLOR);
    writer->setLayerColor(mColor);
}

LayerSettings TestColorLayer::toRenderEngineLayerSettings() {
    LayerSettings layerSettings = TestLayer::toRenderEngineLayerSettings();

    layerSettings.source.solidColor =
            half3(static_cast<half>(mColor.r) / 255.0, static_cast<half>(mColor.g) / 255.0,
                  static_cast<half>(mColor.b) / 255.0);
    layerSettings.alpha = mAlpha * (static_cast<half>(mColor.a) / 255.0);
    return layerSettings;
}

TestBufferLayer::TestBufferLayer(const std::shared_ptr<ComposerClient>& client,
                                 const std::shared_ptr<Gralloc>& gralloc,
                                 TestRenderEngine& renderEngine, Display display, int32_t width,
                                 int32_t height, PixelFormat format,
                                 IComposerClient::Composition composition)
    : TestLayer{client, display}, mRenderEngine(renderEngine) {
    mGralloc = gralloc;
    mComposition = composition;
    mWidth = width;
    mHeight = height;
    mLayerCount = 1;
    mFormat = format;
    mUsage = static_cast<uint64_t>(BufferUsage::CPU_READ_OFTEN | BufferUsage::CPU_WRITE_OFTEN |
                                   BufferUsage::COMPOSER_OVERLAY | BufferUsage::GPU_TEXTURE);

    mAccessRegion.top = 0;
    mAccessRegion.left = 0;
    mAccessRegion.width = width;
    mAccessRegion.height = height;

    setSourceCrop({0, 0, (float)width, (float)height});
}

void TestBufferLayer::write(const std::shared_ptr<CommandWriterBase>& writer) {
    TestLayer::write(writer);
    writer->setLayerCompositionType(mComposition);
    writer->setLayerVisibleRegion(std::vector<IComposerClient::Rect>(1, mDisplayFrame));
    if (mBufferHandle != nullptr) writer->setLayerBuffer(0, mBufferHandle->get(), mFillFence);
}

LayerSettings TestBufferLayer::toRenderEngineLayerSettings() {
    LayerSettings layerSettings = TestLayer::toRenderEngineLayerSettings();
    layerSettings.source.buffer.buffer = std::make_shared<renderengine::impl::ExternalTexture>(
            new GraphicBuffer(mBufferHandle->get(), GraphicBuffer::CLONE_HANDLE, mWidth, mHeight,
                              static_cast<int32_t>(mFormat), 1, mUsage, mStride),
            mRenderEngine.getInternalRenderEngine(),
            renderengine::impl::ExternalTexture::Usage::READABLE);

    layerSettings.source.buffer.usePremultipliedAlpha =
            mBlendMode == IComposerClient::BlendMode::PREMULTIPLIED;

    const float scaleX = (mSourceCrop.right - mSourceCrop.left) / (mWidth);
    const float scaleY = (mSourceCrop.bottom - mSourceCrop.top) / (mHeight);
    const float translateX = mSourceCrop.left / (mWidth);
    const float translateY = mSourceCrop.top / (mHeight);

    layerSettings.source.buffer.textureTransform =
            mat4::translate(vec4(translateX, translateY, 0, 1)) *
            mat4::scale(vec4(scaleX, scaleY, 1.0, 1.0));

    return layerSettings;
}

void TestBufferLayer::fillBuffer(std::vector<IComposerClient::Color> expectedColors) {
    void* bufData = mGralloc->lock(mBufferHandle->get(), mUsage, mAccessRegion, -1);
    ASSERT_NO_FATAL_FAILURE(
            ReadbackHelper::fillBuffer(mWidth, mHeight, mStride, bufData, mFormat, expectedColors));
    mFillFence = mGralloc->unlock(mBufferHandle->get());
    if (mFillFence != -1) {
        sync_wait(mFillFence, -1);
        close(mFillFence);
    }
}

void TestBufferLayer::setBuffer(std::vector<IComposerClient::Color> colors) {
    mBufferHandle.reset(new Gralloc::NativeHandleWrapper(
            mGralloc->allocate(mWidth, mHeight, mLayerCount, mFormat, mUsage,
                               /*import*/ true, &mStride)));
    ASSERT_NE(nullptr, mBufferHandle->get());
    ASSERT_NO_FATAL_FAILURE(fillBuffer(colors));
    ASSERT_NE(false, mGralloc->validateBufferSize(mBufferHandle->get(), mWidth, mHeight,
                                                  mLayerCount, mFormat, mUsage, mStride));
}

void TestBufferLayer::setDataspace(Dataspace dataspace,
                                   const std::shared_ptr<CommandWriterBase>& writer) {
    writer->selectLayer(mLayer);
    writer->setLayerDataspace(dataspace);
}

void TestBufferLayer::setToClientComposition(const std::shared_ptr<CommandWriterBase>& writer) {
    writer->selectLayer(mLayer);
    writer->setLayerCompositionType(IComposerClient::Composition::CLIENT);
}

}  // namespace vts
}  // namespace V2_2
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android
