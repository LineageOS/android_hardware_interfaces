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

#include "Readback.h"
#include <aidl/android/hardware/graphics/common/BufferUsage.h>
#include <renderengine/impl/ExternalTexture.h>
#include "RenderEngine.h"
#include "android-base/stringprintf.h"

namespace aidl::android::hardware::graphics::composer3::libhwc_aidl_test {

namespace {
void saveAsImage(const std::string& prefix, void* bufferData, uint32_t bytesPerPixel,
                 uint32_t stride, uint32_t height, uint32_t width) {
    std::string filename = ::android::base::StringPrintf(
            "/data/local/tmp/%s_%ld.ppm", prefix.c_str(), static_cast<long>(time(nullptr)));
    FILE* file = fopen(filename.c_str(), "wb");
    if (!file) {
        ALOGE("Failed to open file %s for writing", filename.c_str());
        return;
    }

    // PPM header (P6 format - binary RGB)
    // TODO(b/329149798): Add support for 1010102 buffers
    fprintf(file, "P6\n%d %d\n255\n", width, height);

    for (uint32_t y = 0; y < height; y++) {
        std::vector<uint8_t> rowData(width * 3);
        for (uint32_t x = 0; x < width; x++) {
            uint8_t* srcData = static_cast<uint8_t*>(bufferData);
            uint32_t srcOffset = y * stride * bytesPerPixel + x * bytesPerPixel;
            uint32_t dstOffset = x * 3;

            rowData[dstOffset + 0] = srcData[srcOffset + 0];  // R
            rowData[dstOffset + 1] = srcData[srcOffset + 1];  // G
            rowData[dstOffset + 2] = srcData[srcOffset + 2];  // B
        }
        fwrite(rowData.data(), 3, width, file);
    }

    fclose(file);
}

#define ASSERT_APPROX_EQ(val1, val2, error) \
    ASSERT_NEAR(static_cast<double>(val1), static_cast<double>(val2), static_cast<double>(error))
}  // namespace

const std::vector<ColorMode> ReadbackHelper::colorModes = {ColorMode::SRGB, ColorMode::DISPLAY_P3};
const std::vector<Dataspace> ReadbackHelper::dataspaces = {common::Dataspace::SRGB,
                                                           common::Dataspace::DISPLAY_P3};

DisplayProperties ReadbackHelper::setupDisplayProperty(
        const DisplayWrapper& display,
        const std::shared_ptr<ComposerClientWrapper>& composerClient) {
    int64_t displayId = display.getDisplayId();

    // Set testColorModes
    const auto& [status, modes] = composerClient->getColorModes(displayId);
    EXPECT_TRUE(status.isOk());
    if (!status.isOk()) {
        abort();
    }
    std::vector<ColorMode> testColorModes;
    for (ColorMode mode : modes) {
        if (std::find(colorModes.begin(), colorModes.end(), mode) != colorModes.end()) {
            testColorModes.push_back(mode);
        }
    }

    // Set pixelFormat and dataspace
    auto [readbackStatus, readBackBufferAttributes] =
            composerClient->getReadbackBufferAttributes(displayId);
    if (!readbackStatus.isOk()) {
        EXPECT_EQ(readbackStatus.getExceptionCode(), EX_SERVICE_SPECIFIC);
        EXPECT_EQ(readbackStatus.getServiceSpecificError(), IComposerClient::EX_UNSUPPORTED);
    }

    // Set testRenderEngine and clientCompositionDisplaySettings
    EXPECT_TRUE(composerClient->setPowerMode(displayId, PowerMode::ON).isOk());
    const auto format = readbackStatus.isOk() ? readBackBufferAttributes.format
                                              : common::PixelFormat::RGBA_8888;
    std::unique_ptr<TestRenderEngine> testRenderEngine;
    EXPECT_NO_FATAL_FAILURE(
            testRenderEngine = std::unique_ptr<TestRenderEngine>(new TestRenderEngine(
                    ::android::renderengine::RenderEngineCreationArgs::Builder()
                            .setPixelFormat(static_cast<int>(format))
                            .setImageCacheSize(TestRenderEngine::sMaxFrameBufferAcquireBuffers)
                            .setEnableProtectedContext(false)
                            .setPrecacheToneMapperShaderOnly(false)
                            .setContextPriority(
                                    ::android::renderengine::RenderEngine::ContextPriority::High)
                            .build())));

    ::android::renderengine::DisplaySettings clientCompositionDisplaySettings;
    clientCompositionDisplaySettings.physicalDisplay =
            ::android::Rect(display.getDisplayWidth(), display.getDisplayHeight());
    clientCompositionDisplaySettings.clip = clientCompositionDisplaySettings.physicalDisplay;

    testRenderEngine->initGraphicBuffer(
            static_cast<uint32_t>(display.getDisplayWidth()),
            static_cast<uint32_t>(display.getDisplayHeight()),
            /*layerCount*/ 1U,
            static_cast<uint64_t>(static_cast<uint64_t>(common::BufferUsage::CPU_READ_OFTEN) |
                                  static_cast<uint64_t>(common::BufferUsage::CPU_WRITE_OFTEN) |
                                  static_cast<uint64_t>(common::BufferUsage::GPU_RENDER_TARGET)));
    testRenderEngine->setDisplaySettings(clientCompositionDisplaySettings);

    DisplayProperties displayProperties(displayId, testColorModes, std::move(testRenderEngine),
                                        std::move(clientCompositionDisplaySettings),
                                        std::move(readBackBufferAttributes.format),
                                        std::move(readBackBufferAttributes.dataspace));
    return displayProperties;
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
            return Dataspace::SRGB;
        default:
            return Dataspace::UNKNOWN;
    }
}

int32_t ReadbackHelper::GetBitsPerChannel(common::PixelFormat pixelFormat) {
    switch (pixelFormat) {
        case common::PixelFormat::RGBA_1010102:
        case common::PixelFormat::BGRA_1010102:
        case common::PixelFormat::BGRX_1010102:
            return 10;
        case common::PixelFormat::RGBA_8888:
        case common::PixelFormat::RGB_888:
            return 8;
        default:
            return -1;
    }
}

int32_t ReadbackHelper::GetTolerance(int32_t bitsPerChannel) {
    if (bitsPerChannel > 8) {
        return 3;
    } else {
        return 0;
    }
}

int32_t ReadbackHelper::GetAlphaBits(common::PixelFormat pixelFormat) {
    switch (pixelFormat) {
        case common::PixelFormat::RGBA_8888:
            return 8;
        case common::PixelFormat::RGBA_1010102:
        case common::PixelFormat::BGRA_1010102:
        case common::PixelFormat::BGRX_1010102:
            return 2;
        case common::PixelFormat::RGB_888:
            return 0;
        default:
            return -1;
    }
}

void ReadbackHelper::fillBuffer(uint32_t width, uint32_t height, uint32_t stride,
                                int32_t bytesPerPixel, void* bufferData,
                                common::PixelFormat pixelFormat,
                                std::vector<Color> desiredPixelColors) {
    ASSERT_TRUE(pixelFormat == common::PixelFormat::RGB_888 ||
                pixelFormat == common::PixelFormat::RGBA_8888 ||
                pixelFormat == common::PixelFormat::RGBA_1010102 ||
                pixelFormat == common::PixelFormat::BGRA_1010102 ||
                pixelFormat == common::PixelFormat::BGRX_1010102);
    int32_t bitsPerChannel = GetBitsPerChannel(pixelFormat);
    int32_t alphaBits = GetAlphaBits(pixelFormat);
    ASSERT_NE(-1, alphaBits);
    ASSERT_NE(-1, bitsPerChannel);
    ASSERT_NE(-1, bytesPerPixel);

    uint32_t maxValue = (1 << bitsPerChannel) - 1;
    uint32_t maxAlphaValue = (1 << alphaBits) - 1;
    for (uint32_t row = 0; row < height; row++) {
        for (uint32_t col = 0; col < width; col++) {
            auto pixel = row * width + col;
            Color srcColor = desiredPixelColors[static_cast<size_t>(pixel)];

            uint32_t offset = (row * stride + col) * static_cast<uint32_t>(bytesPerPixel);

            uint32_t* pixelStart = (uint32_t*)((uint8_t*)bufferData + offset);

            uint32_t red = static_cast<uint32_t>(std::round(maxValue * srcColor.r));
            uint32_t green = static_cast<uint32_t>(std::round(maxValue * srcColor.g));
            uint32_t blue = static_cast<uint32_t>(std::round(maxValue * srcColor.b));

            // Boo we're not word aligned so special case this.
            if (pixelFormat == common::PixelFormat::RGB_888) {
                uint8_t* pixelColor = (uint8_t*)pixelStart;
                pixelColor[0] = static_cast<uint8_t>(red);
                pixelColor[1] = static_cast<uint8_t>(green);
                pixelColor[2] = static_cast<uint8_t>(blue);
            } else {
                bool bgraSwizzle = pixelFormat == common::PixelFormat::BGRA_1010102 ||
                                   pixelFormat == common::PixelFormat::BGRX_1010102;
                uint32_t alpha = static_cast<uint32_t>(std::round(maxAlphaValue * srcColor.a));
                uint32_t color =
                        (alpha << (32 - alphaBits)) |
                        (blue << (32 - alphaBits - bitsPerChannel * (bgraSwizzle ? 3 : 1))) |
                        (green << (32 - alphaBits - bitsPerChannel * 2)) |
                        (red << (32 - alphaBits - bitsPerChannel * (bgraSwizzle ? 1 : 3)));
                *pixelStart = color;
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
        pixelFormat != common::PixelFormat::RGBA_8888 &&
        pixelFormat != common::PixelFormat::RGBA_1010102 &&
        pixelFormat != common::PixelFormat::BGRA_1010102 &&
        pixelFormat != common::PixelFormat::BGRX_1010102) {
        return false;
    }
    if (std::find(dataspaces.begin(), dataspaces.end(), dataspace) == dataspaces.end()) {
        return false;
    }
    return true;
}

void ReadbackHelper::compareColorBuffers(const std::vector<Color>& expectedColors, void* bufferData,
                                         const uint32_t stride, int32_t bytesPerPixel,
                                         const uint32_t width, const uint32_t height,
                                         common::PixelFormat pixelFormat) {
    int32_t bitsPerChannel = GetBitsPerChannel(pixelFormat);
    int32_t alphaBits = GetAlphaBits(pixelFormat);
    int32_t tolerance = GetTolerance(bitsPerChannel);
    ASSERT_GT(bytesPerPixel, 0);
    ASSERT_NE(-1, alphaBits);
    ASSERT_NE(-1, bitsPerChannel);
    ASSERT_GE(tolerance, 0);
    uint32_t maxValue = (1 << bitsPerChannel) - 1;
    uint32_t maxAlphaValue = (1 << alphaBits) - 1;
    for (uint32_t row = 0; row < height; row++) {
        for (uint32_t col = 0; col < width; col++) {
            auto pixel = row * width + col;
            const Color expectedColor = expectedColors[static_cast<size_t>(pixel)];

            uint32_t offset = (row * stride + col) * static_cast<uint32_t>(bytesPerPixel);
            uint32_t* pixelStart = (uint32_t*)((uint8_t*)bufferData + offset);

            uint32_t expectedRed = static_cast<uint32_t>(std::round(maxValue * expectedColor.r));
            uint32_t expectedGreen = static_cast<uint32_t>(std::round(maxValue * expectedColor.g));
            uint32_t expectedBlue = static_cast<uint32_t>(std::round(maxValue * expectedColor.b));

            // Boo we're not word aligned so special case this.
            if (pixelFormat == common::PixelFormat::RGB_888) {
                uint8_t* pixelColor = (uint8_t*)pixelStart;
                ASSERT_EQ(pixelColor[0], static_cast<uint8_t>(expectedRed))
                        << "Red channel mismatch at (" << row << ", " << col << ")";
                ASSERT_EQ(pixelColor[1], static_cast<uint8_t>(expectedGreen))
                        << "Green channel mismatch at (" << row << ", " << col << ")";
                ASSERT_EQ(pixelColor[2], static_cast<uint8_t>(expectedBlue))
                        << "Blue channel mismatch at (" << row << ", " << col << ")";
            } else {
                uint32_t expectedAlpha =
                        static_cast<uint32_t>(std::round(maxAlphaValue * expectedColor.a));

                bool bgraSwizzle = pixelFormat == common::PixelFormat::BGRA_1010102 ||
                                   pixelFormat == common::PixelFormat::BGRX_1010102;

                uint32_t actualRed =
                        (*pixelStart >> (32 - alphaBits - bitsPerChannel * (bgraSwizzle ? 1 : 3))) &
                        maxValue;
                uint32_t actualGreen =
                        (*pixelStart >> (32 - alphaBits - bitsPerChannel * 2)) & maxValue;
                uint32_t actualBlue =
                        (*pixelStart >> (32 - alphaBits - bitsPerChannel * (bgraSwizzle ? 3 : 1))) &
                        maxValue;
                uint32_t actualAlpha = (*pixelStart >> (32 - alphaBits)) & maxAlphaValue;

                ASSERT_APPROX_EQ(expectedRed, actualRed, tolerance)
                        << "Red channel mismatch at (" << row << ", " << col << ")";
                ASSERT_APPROX_EQ(expectedGreen, actualGreen, tolerance)
                        << "Green channel mismatch at (" << row << ", " << col << ")";
                ASSERT_APPROX_EQ(expectedBlue, actualBlue, tolerance)
                        << "Blue channel mismatch at (" << row << ", " << col << ")";
            }
        }
    }
}

void ReadbackHelper::compareColorBuffers(void* expectedBuffer, void* actualBuffer,
                                         const uint32_t stride, int32_t bytesPerPixel,
                                         const uint32_t width, const uint32_t height,
                                         common::PixelFormat pixelFormat) {
    int32_t bitsPerChannel = GetBitsPerChannel(pixelFormat);
    int32_t alphaBits = GetAlphaBits(pixelFormat);
    int32_t tolerance = GetTolerance(bitsPerChannel);
    ASSERT_GT(bytesPerPixel, 0);
    ASSERT_NE(-1, alphaBits);
    ASSERT_NE(-1, bitsPerChannel);
    ASSERT_GE(tolerance, 0);
    uint32_t maxValue = (1 << bitsPerChannel) - 1;
    uint32_t maxAlphaValue = (1 << alphaBits) - 1;
    for (uint32_t row = 0; row < height; row++) {
        for (uint32_t col = 0; col < width; col++) {
            uint32_t offset = (row * stride + col) * static_cast<uint32_t>(bytesPerPixel);
            uint32_t* expectedStart = (uint32_t*)((uint8_t*)expectedBuffer + offset);
            uint32_t* actualStart = (uint32_t*)((uint8_t*)actualBuffer + offset);

            // Boo we're not word aligned so special case this.
            if (pixelFormat == common::PixelFormat::RGB_888) {
                uint8_t* expectedPixel = (uint8_t*)expectedStart;
                uint8_t* actualPixel = (uint8_t*)actualStart;
                ASSERT_EQ(actualPixel[0], expectedPixel[0])
                        << "Red channel mismatch at (" << row << ", " << col << ")";
                ASSERT_EQ(actualPixel[1], expectedPixel[1])
                        << "Green channel mismatch at (" << row << ", " << col << ")";
                ASSERT_EQ(actualPixel[2], expectedPixel[2])
                        << "Blue channel mismatch at (" << row << ", " << col << ")";
            } else {
                bool bgraSwizzle = pixelFormat == common::PixelFormat::BGRA_1010102 ||
                                   pixelFormat == common::PixelFormat::BGRX_1010102;

                uint32_t expectedRed = (*expectedStart >>
                                        (32 - alphaBits - bitsPerChannel * (bgraSwizzle ? 1 : 3))) &
                                       maxValue;
                uint32_t expectedGreen =
                        (*expectedStart >> (32 - alphaBits - bitsPerChannel * 2)) & maxValue;
                uint32_t expectedBlue =
                        (*expectedStart >>
                         (32 - alphaBits - bitsPerChannel * (bgraSwizzle ? 3 : 1))) &
                        maxValue;
                uint32_t expectedAlpha = (*expectedStart >> (32 - alphaBits)) & maxAlphaValue;

                uint32_t actualRed = (*actualStart >>
                                      (32 - alphaBits - bitsPerChannel * (bgraSwizzle ? 1 : 3))) &
                                     maxValue;
                uint32_t actualGreen =
                        (*actualStart >> (32 - alphaBits - bitsPerChannel * 2)) & maxValue;
                uint32_t actualBlue = (*actualStart >>
                                       (32 - alphaBits - bitsPerChannel * (bgraSwizzle ? 3 : 1))) &
                                      maxValue;
                uint32_t actualAlpha = (*actualStart >> (32 - alphaBits)) & maxAlphaValue;

                ASSERT_APPROX_EQ(expectedRed, actualRed, tolerance)
                        << "Red channel mismatch at (" << row << ", " << col << ")";
                ASSERT_APPROX_EQ(expectedGreen, actualGreen, tolerance)
                        << "Green channel mismatch at (" << row << ", " << col << ")";
                ASSERT_APPROX_EQ(expectedBlue, actualBlue, tolerance)
                        << "Blue channel mismatch at (" << row << ", " << col << ")";
            }
        }
    }
}

ReadbackBuffer::ReadbackBuffer(int64_t display,
                               const std::shared_ptr<ComposerClientWrapper>& client, int32_t width,
                               int32_t height, common::PixelFormat pixelFormat,
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

void ReadbackBuffer::checkReadbackBuffer(const std::vector<Color>& expectedColors, bool saveImage) {
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
    ASSERT_TRUE(mPixelFormat == PixelFormat::RGB_888 || mPixelFormat == PixelFormat::RGBA_8888 ||
                mPixelFormat == PixelFormat::RGBA_1010102 ||
                mPixelFormat == PixelFormat::BGRA_1010102 ||
                mPixelFormat == PixelFormat::BGRX_1010102);
    const uint32_t stride = (bytesPerPixel > 0 && bytesPerStride > 0)
                                    ? static_cast<uint32_t>(bytesPerStride / bytesPerPixel)
                                    : mGraphicBuffer->getStride();
    ReadbackHelper::compareColorBuffers(expectedColors, bufData, stride, bytesPerPixel, mWidth,
                                        mHeight, mPixelFormat);

    // If requested, save the buffer as an image while it's still locked
    if (saveImage) {
        std::string prefix = "readback_display" + std::to_string(mDisplay);
        saveAsImage(prefix, bufData, static_cast<uint32_t>(bytesPerPixel), stride, mHeight, mWidth);
    }

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

TestBufferLayer::TestBufferLayer(ComposerClientWrapper& client, TestRenderEngine& renderEngine,
                                 int64_t display, uint32_t width, uint32_t height,
                                 common::PixelFormat format, ComposerClientWriter& writer,
                                 Composition composition)
    : TestLayer{client, display, writer}, mRenderEngine(renderEngine) {
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
    ASSERT_NO_FATAL_FAILURE(ReadbackHelper::fillBuffer(mWidth, mHeight, stride, bytesPerPixel,
                                                       bufData, mPixelFormat, expectedColors));

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

}  // namespace aidl::android::hardware::graphics::composer3::libhwc_aidl_test
