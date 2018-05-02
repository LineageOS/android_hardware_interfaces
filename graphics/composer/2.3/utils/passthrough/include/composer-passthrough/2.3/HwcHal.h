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

#include <composer-hal/2.3/ComposerHal.h>
#include <composer-passthrough/2.2/HwcHal.h>

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_3 {
namespace passthrough {

namespace detail {

using V2_1::Display;
using V2_1::Error;

// HwcHalImpl implements V2_*::hal::ComposerHal on top of hwcomposer2
template <typename Hal>
class HwcHalImpl : public V2_2::passthrough::detail::HwcHalImpl<Hal> {
   public:
    Error getDisplayIdentificationData(Display display, uint8_t* outPort,
                                       std::vector<uint8_t>* outData) override {
        if (!mDispatch.getDisplayIdentificationData) {
            return Error::UNSUPPORTED;
        }

        uint32_t size = 0;
        int32_t error =
            mDispatch.getDisplayIdentificationData(mDevice, display, outPort, &size, nullptr);
        if (error != HWC2_ERROR_NONE) {
            return static_cast<Error>(error);
        }

        std::vector<uint8_t> data(size);
        error =
            mDispatch.getDisplayIdentificationData(mDevice, display, outPort, &size, data.data());
        if (error != HWC2_ERROR_NONE) {
            return static_cast<Error>(error);
        }

        data.resize(size);
        *outData = std::move(data);
        return Error::NONE;
    }

   protected:
    bool initDispatch() override {
        if (!BaseType2_2::initDispatch()) {
            return false;
        }

        this->initOptionalDispatch(HWC2_FUNCTION_GET_DISPLAY_IDENTIFICATION_DATA,
                                   &mDispatch.getDisplayIdentificationData);
        return true;
    }

   private:
    struct {
        HWC2_PFN_GET_DISPLAY_IDENTIFICATION_DATA getDisplayIdentificationData;
    } mDispatch = {};

    using BaseType2_2 = V2_2::passthrough::detail::HwcHalImpl<Hal>;
    using BaseType2_1 = V2_1::passthrough::detail::HwcHalImpl<Hal>;
    using BaseType2_1::mDevice;
};

}  // namespace detail

using HwcHal = detail::HwcHalImpl<hal::ComposerHal>;

}  // namespace passthrough
}  // namespace V2_3
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android
