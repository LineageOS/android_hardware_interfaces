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

#include <renderengine/RenderEngine.h>
#include "ComposerClientWrapper.h"

namespace aidl::android::hardware::graphics::composer3::libhwc_aidl_test {

using ::android::renderengine::LayerSettings;

class TestLayer {
  public:
    TestLayer(ComposerClientWrapper& client, int64_t display, ComposerClientWriter& writer)
        : mDisplay(display) {
        auto [status, layer] = client.createLayer(display, kBufferSlotCount, &writer);
        EXPECT_TRUE(status.isOk());
        mLayer = layer;

        OverlayProperties properties;
        std::tie(status, properties) = client.getOverlaySupport();
        mLutsSupported = status.isOk() && properties.lutProperties.has_value();
    }

    // ComposerClient will take care of destroying layers, no need to explicitly
    // call destroyLayers here
    virtual ~TestLayer() {};

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
    void setLuts(Luts luts) { mLuts = std::move(luts); }

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
    Luts mLuts;

    bool mLutsSupported = true;
};

}  // namespace aidl::android::hardware::graphics::composer3::libhwc_aidl_test