/**
 * Copyright (c) 2025, The Android Open Source Project
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

#include "TestLayer.h"

namespace aidl::android::hardware::graphics::composer3::libhwc_aidl_test {

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
    Luts luts{
            .pfd = ::ndk::ScopedFileDescriptor(dup(mLuts.pfd.get())),
            .offsets = mLuts.offsets,
            .lutProperties = mLuts.lutProperties,
    };
    writer.setLayerLuts(mDisplay, mLayer, luts);
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
    if (mLuts.pfd.get() >= 0 && mLuts.offsets) {
        std::vector<int32_t> dimensions;
        std::vector<int32_t> sizes;
        std::vector<int32_t> keys;
        dimensions.reserve(mLuts.lutProperties.size());
        sizes.reserve(mLuts.lutProperties.size());
        keys.reserve(mLuts.lutProperties.size());

        for (auto& l : mLuts.lutProperties) {
            dimensions.emplace_back(static_cast<int32_t>(l.dimension));
            sizes.emplace_back(static_cast<int32_t>(l.size));
            keys.emplace_back(static_cast<int32_t>(l.samplingKeys[0]));
        }

        layerSettings.luts = std::make_shared<::android::gui::DisplayLuts>(
                ::android::base::unique_fd(dup(mLuts.pfd.get())), *mLuts.offsets, dimensions, sizes,
                keys);
    }

    return layerSettings;
}

}  // namespace aidl::android::hardware::graphics::composer3::libhwc_aidl_test