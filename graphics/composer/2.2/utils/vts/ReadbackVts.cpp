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

void ReadbackHelper::fillBuffer(int32_t width, int32_t height, uint32_t stride, void* bufferData,
                                PixelFormat pixelFormat,
                                std::vector<IComposerClient::Color> desiredPixelColors) {
    ASSERT_TRUE(pixelFormat == PixelFormat::RGB_888 || pixelFormat == PixelFormat::RGBA_8888);
    int32_t bytesPerPixel = GetBytesPerPixel(pixelFormat);
    ASSERT_NE(-1, bytesPerPixel);
    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            int pixel = row * width + col;
            IComposerClient::Color srcColor = desiredPixelColors[pixel];

            int offset = (row * stride + col) * bytesPerPixel;
            uint8_t* pixelColor = (uint8_t*)bufferData + offset;
            pixelColor[0] = srcColor.r;
            pixelColor[1] = srcColor.g;
            pixelColor[2] = srcColor.b;

            if (bytesPerPixel == 4) {
                pixelColor[3] = srcColor.a;
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

bool ReadbackHelper::readbackSupported(const PixelFormat& pixelFormat, const Dataspace& dataspace,
                                       const Error error) {
    if (error != Error::NONE) {
        return false;
    }
    // TODO: add support for RGBA_1010102
    if (pixelFormat != PixelFormat::RGB_888 && pixelFormat != PixelFormat::RGBA_8888) {
        return false;
    }
    if (dataspace != Dataspace::V0_SRGB) {
        return false;
    }
    return true;
}

ReadbackBuffer::ReadbackBuffer(Display display, const std::shared_ptr<ComposerClient>& client,
                               const std::shared_ptr<Gralloc>& gralloc, uint32_t width,
                               uint32_t height, PixelFormat pixelFormat, Dataspace dataspace) {
    mDisplay = display;

    mComposerClient = client;
    mGralloc = gralloc;

    mPixelFormat = pixelFormat;
    mDataspace = dataspace;

    mInfo.width = width;
    mInfo.height = height;
    mInfo.layerCount = 1;
    mInfo.format = mPixelFormat;
    mInfo.usage = static_cast<uint64_t>(BufferUsage::CPU_READ_OFTEN | BufferUsage::GPU_TEXTURE);

    mAccessRegion.top = 0;
    mAccessRegion.left = 0;
    mAccessRegion.width = width;
    mAccessRegion.height = height;
}

ReadbackBuffer::~ReadbackBuffer() {
    if (mBufferHandle != nullptr) {
        mGralloc->freeBuffer(mBufferHandle);
    }
}

void ReadbackBuffer::setReadbackBuffer() {
    if (mBufferHandle != nullptr) {
        mGralloc->freeBuffer(mBufferHandle);
        mBufferHandle = nullptr;
    }
    mBufferHandle = mGralloc->allocate(mInfo, /*import*/ true, &mStride);
    ASSERT_NE(false, mGralloc->validateBufferSize(mBufferHandle, mInfo, mStride));
    ASSERT_NO_FATAL_FAILURE(mComposerClient->setReadbackBuffer(mDisplay, mBufferHandle, -1));
}

void ReadbackBuffer::checkReadbackBuffer(std::vector<IComposerClient::Color> expectedColors) {
    // lock buffer for reading
    int32_t fenceHandle;
    ASSERT_NO_FATAL_FAILURE(mComposerClient->getReadbackBufferFence(mDisplay, &fenceHandle));

    void* bufData = mGralloc->lock(mBufferHandle, mInfo.usage, mAccessRegion, fenceHandle);
    ASSERT_TRUE(mPixelFormat == PixelFormat::RGB_888 || mPixelFormat == PixelFormat::RGBA_8888);
    int32_t bytesPerPixel = ReadbackHelper::GetBytesPerPixel(mPixelFormat);
    ASSERT_NE(-1, bytesPerPixel);
    for (int row = 0; row < mInfo.height; row++) {
        for (int col = 0; col < mInfo.width; col++) {
            int pixel = row * mInfo.width + col;
            int offset = (row * mStride + col) * bytesPerPixel;
            uint8_t* pixelColor = (uint8_t*)bufData + offset;

            ASSERT_EQ(expectedColors[pixel].r, pixelColor[0]);
            ASSERT_EQ(expectedColors[pixel].g, pixelColor[1]);
            ASSERT_EQ(expectedColors[pixel].b, pixelColor[2]);
        }
    }
    int32_t unlockFence = mGralloc->unlock(mBufferHandle);
    if (unlockFence != -1) {
        sync_wait(unlockFence, -1);
        close(unlockFence);
    }
}

void TestColorLayer::write(const std::shared_ptr<CommandWriterBase>& writer) {
    TestLayer::write(writer);
    writer->setLayerCompositionType(IComposerClient::Composition::SOLID_COLOR);
    writer->setLayerColor(mColor);
}

TestBufferLayer::TestBufferLayer(const std::shared_ptr<ComposerClient>& client,
                                 const std::shared_ptr<Gralloc>& gralloc, Display display,
                                 int32_t width, int32_t height, PixelFormat format,
                                 IComposerClient::Composition composition)
    : TestLayer{client, display} {
    mGralloc = gralloc;
    mComposition = composition;
    mInfo.width = width;
    mInfo.height = height;
    mInfo.layerCount = 1;
    mInfo.format = format;
    mInfo.usage = static_cast<uint64_t>(BufferUsage::CPU_READ_OFTEN | BufferUsage::CPU_WRITE_OFTEN |
                                        BufferUsage::COMPOSER_OVERLAY);

    mAccessRegion.top = 0;
    mAccessRegion.left = 0;
    mAccessRegion.width = width;
    mAccessRegion.height = height;

    setSourceCrop({0, 0, (float)width, (float)height});
}

TestBufferLayer::~TestBufferLayer() {
    if (mBufferHandle != nullptr) {
        mGralloc->freeBuffer(mBufferHandle);
    }
}

void TestBufferLayer::write(const std::shared_ptr<CommandWriterBase>& writer) {
    TestLayer::write(writer);
    writer->setLayerCompositionType(mComposition);
    writer->setLayerDataspace(Dataspace::UNKNOWN);
    writer->setLayerVisibleRegion(std::vector<IComposerClient::Rect>(1, mDisplayFrame));
    if (mBufferHandle != nullptr) writer->setLayerBuffer(0, mBufferHandle, mFillFence);
}

void TestBufferLayer::fillBuffer(std::vector<IComposerClient::Color> expectedColors) {
    void* bufData = mGralloc->lock(mBufferHandle, mInfo.usage, mAccessRegion, -1);
    ASSERT_NO_FATAL_FAILURE(ReadbackHelper::fillBuffer(mInfo.width, mInfo.height, mStride, bufData,
                                                       mInfo.format, expectedColors));
    mFillFence = mGralloc->unlock(mBufferHandle);
    if (mFillFence != -1) {
        sync_wait(mFillFence, -1);
        close(mFillFence);
    }
}
void TestBufferLayer::setBuffer(std::vector<IComposerClient::Color> colors) {
    if (mBufferHandle != nullptr) {
        mGralloc->freeBuffer(mBufferHandle);
        mBufferHandle = nullptr;
    }
    mBufferHandle = mGralloc->allocate(mInfo, /*import*/ true, &mStride);
    ASSERT_NE(nullptr, mBufferHandle);
    ASSERT_NO_FATAL_FAILURE(fillBuffer(colors));
    ASSERT_NE(false, mGralloc->validateBufferSize(mBufferHandle, mInfo, mStride));
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
