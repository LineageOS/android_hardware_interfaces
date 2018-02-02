/*
 * Copyright 2018 The Android Open Source Project
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

#ifndef LOG_TAG
#warning "HwcHal.h included without LOG_TAG"
#endif

#include <type_traits>

#include <composer-hal/2.2/ComposerHal.h>
#include <composer-passthrough/2.1/HwcHal.h>

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_2 {
namespace passthrough {

namespace detail {

using common::V1_0::Dataspace;
using common::V1_0::PixelFormat;
using V2_1::Display;
using V2_1::Error;
using V2_1::Layer;

// HwcHalImpl implements V2_*::hal::ComposerHal on top of hwcomposer2
template <typename Hal>
class HwcHalImpl : public V2_1::passthrough::detail::HwcHalImpl<Hal> {
   public:
    // XXX when can we return Error::UNSUPPORTED?

    Error getPerFrameMetadataKeys(
        Display display, std::vector<IComposerClient::PerFrameMetadataKey>* outKeys) override {
        if (!mDispatch.getPerFrameMetadataKeys) {
            return Error::UNSUPPORTED;
        }

        uint32_t count = 0;
        int32_t error = mDispatch.getPerFrameMetadataKeys(mDevice, display, &count, nullptr);
        if (error != HWC2_ERROR_NONE) {
            return static_cast<Error>(error);
        }

        std::vector<IComposerClient::PerFrameMetadataKey> keys(count);
        error = mDispatch.getPerFrameMetadataKeys(
            mDevice, display, &count,
            reinterpret_cast<std::underlying_type<IComposerClient::PerFrameMetadataKey>::type*>(
                keys.data()));
        if (error != HWC2_ERROR_NONE) {
            return static_cast<Error>(error);
        }

        keys.resize(count);
        *outKeys = std::move(keys);
        return Error::NONE;
    }

    Error setPerFrameMetadata(
        Display display, const std::vector<IComposerClient::PerFrameMetadata>& metadata) override {
        if (!mDispatch.setPerFrameMetadata) {
            return Error::UNSUPPORTED;
        }

        std::vector<int32_t> keys;
        std::vector<float> values;
        keys.reserve(metadata.size());
        values.reserve(metadata.size());
        for (const auto& m : metadata) {
            keys.push_back(static_cast<int32_t>(m.key));
            values.push_back(m.value);
        }

        int32_t error = mDispatch.setPerFrameMetadata(mDevice, display, metadata.size(),
                                                      keys.data(), values.data());
        return static_cast<Error>(error);
    }

    Error getReadbackBufferAttributes(Display display, PixelFormat* outFormat,
                                      Dataspace* outDataspace) override {
        if (!mDispatch.getReadbackBufferAttributes) {
            return Error::UNSUPPORTED;
        }

        int32_t format = 0;
        int32_t dataspace = 0;
        int32_t error =
            mDispatch.getReadbackBufferAttributes(mDevice, display, &format, &dataspace);
        if (error == HWC2_ERROR_NONE) {
            *outFormat = static_cast<PixelFormat>(format);
            *outDataspace = static_cast<Dataspace>(dataspace);
        }
        return static_cast<Error>(error);
    }

    Error setReadbackBuffer(Display display, const native_handle_t* bufferHandle,
                            base::unique_fd fenceFd) override {
        if (!mDispatch.setReadbackBuffer) {
            return Error::UNSUPPORTED;
        }

        int32_t error =
            mDispatch.setReadbackBuffer(mDevice, display, bufferHandle, fenceFd.release());
        return static_cast<Error>(error);
    }

    Error getReadbackBufferFence(Display display, base::unique_fd* outFenceFd) override {
        if (!mDispatch.getReadbackBufferFence) {
            return Error::UNSUPPORTED;
        }

        int32_t fenceFd = -1;
        int32_t error = mDispatch.getReadbackBufferFence(mDevice, display, &fenceFd);
        outFenceFd->reset(fenceFd);
        return static_cast<Error>(error);
    }

    Error setPowerMode_2_2(Display display, IComposerClient::PowerMode mode) override {
        if (mode == IComposerClient::PowerMode::ON_SUSPEND) {
            return Error::UNSUPPORTED;
        }
        return setPowerMode(display, static_cast<V2_1::IComposerClient::PowerMode>(mode));
    }

    Error setLayerFloatColor(Display display, Layer layer,
                             IComposerClient::FloatColor color) override {
        if (!mDispatch.setLayerFloatColor) {
            return Error::UNSUPPORTED;
        }

        int32_t error = mDispatch.setLayerFloatColor(
            mDevice, display, layer, hwc_float_color{color.r, color.g, color.b, color.a});
        return static_cast<Error>(error);
    }

   protected:
    template <typename T>
    bool initOptionalDispatch(hwc2_function_descriptor_t desc, T* outPfn) {
        auto pfn = mDevice->getFunction(mDevice, desc);
        if (pfn) {
            *outPfn = reinterpret_cast<T>(pfn);
            return true;
        } else {
            return false;
        }
    }

    bool initDispatch() override {
        if (!BaseType2_1::initDispatch()) {
            return false;
        }

        initOptionalDispatch(HWC2_FUNCTION_SET_LAYER_FLOAT_COLOR, &mDispatch.setLayerFloatColor);

        initOptionalDispatch(HWC2_FUNCTION_SET_PER_FRAME_METADATA, &mDispatch.setPerFrameMetadata);
        initOptionalDispatch(HWC2_FUNCTION_GET_PER_FRAME_METADATA_KEYS,
                             &mDispatch.getPerFrameMetadataKeys);

        initOptionalDispatch(HWC2_FUNCTION_SET_READBACK_BUFFER, &mDispatch.setReadbackBuffer);
        initOptionalDispatch(HWC2_FUNCTION_GET_READBACK_BUFFER_ATTRIBUTES,
                             &mDispatch.getReadbackBufferAttributes);
        initOptionalDispatch(HWC2_FUNCTION_GET_READBACK_BUFFER_FENCE,
                             &mDispatch.getReadbackBufferFence);

        return true;
    }

    struct {
        HWC2_PFN_SET_LAYER_FLOAT_COLOR setLayerFloatColor;
        HWC2_PFN_SET_PER_FRAME_METADATA setPerFrameMetadata;
        HWC2_PFN_GET_PER_FRAME_METADATA_KEYS getPerFrameMetadataKeys;
        HWC2_PFN_SET_READBACK_BUFFER setReadbackBuffer;
        HWC2_PFN_GET_READBACK_BUFFER_ATTRIBUTES getReadbackBufferAttributes;
        HWC2_PFN_GET_READBACK_BUFFER_FENCE getReadbackBufferFence;
    } mDispatch = {};

   private:
    using BaseType2_1 = V2_1::passthrough::detail::HwcHalImpl<Hal>;
    using BaseType2_1::mDevice;
    using BaseType2_1::setPowerMode;
};

}  // namespace detail

using HwcHal = detail::HwcHalImpl<hal::ComposerHal>;

}  // namespace passthrough
}  // namespace V2_2
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android
