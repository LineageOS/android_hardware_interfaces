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

#pragma once

#ifndef LOG_TAG
#warning "HwcHal.h included without LOG_TAG"
#endif

#include <type_traits>

#include <composer-hal/2.4/ComposerHal.h>
#include <composer-passthrough/2.3/HwcHal.h>

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_4 {
namespace passthrough {

namespace detail {

using common::V1_1::RenderIntent;
using common::V1_2::ColorMode;
using common::V1_2::Dataspace;
using common::V1_2::Hdr;
using common::V1_2::PixelFormat;
using V2_1::Display;
using V2_1::Error;

// HwcHalImpl implements V2_*::hal::ComposerHal on top of hwcomposer2
template <typename Hal>
class HwcHalImpl : public V2_3::passthrough::detail::HwcHalImpl<Hal> {
  public:
    Error getDisplayCapabilities_2_4(
            Display display,
            std::vector<IComposerClient::DisplayCapability>* outCapabilities) override {
        std::vector<V2_3::IComposerClient::DisplayCapability> capabilities;
        Error error = BaseType2_3::getDisplayCapabilities(display, &capabilities);
        if (error != Error::NONE) {
            return error;
        }
        outCapabilities->clear();
        outCapabilities->reserve(capabilities.size());
        for (auto capability : capabilities) {
            outCapabilities->push_back(static_cast<IComposerClient::DisplayCapability>(capability));
        }
        return Error::NONE;
    }

    Error getDisplayConnectionType(Display display,
                                   IComposerClient::DisplayConnectionType* outType) override {
        if (!mDispatch.getDisplayConnectionType) {
            return Error::UNSUPPORTED;
        }

        uint32_t type = HWC2_DISPLAY_CONNECTION_TYPE_INTERNAL;
        int32_t error = mDispatch.getDisplayConnectionType(mDevice, display, &type);
        *outType = static_cast<IComposerClient::DisplayConnectionType>(type);
        return static_cast<Error>(error);
    }

  protected:
    bool initDispatch() override {
        if (!BaseType2_3::initDispatch()) {
            return false;
        }

        this->initOptionalDispatch(HWC2_FUNCTION_GET_DISPLAY_CONNECTION_TYPE,
                                   &mDispatch.getDisplayConnectionType);
        return true;
    }

  private:
    struct {
        HWC2_PFN_GET_DISPLAY_CONNECTION_TYPE getDisplayConnectionType;
    } mDispatch = {};

    using BaseType2_1 = V2_1::passthrough::detail::HwcHalImpl<Hal>;
    using BaseType2_3 = V2_3::passthrough::detail::HwcHalImpl<Hal>;
    using BaseType2_1::mDevice;
};

}  // namespace detail

using HwcHal = detail::HwcHalImpl<hal::ComposerHal>;

}  // namespace passthrough
}  // namespace V2_4
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android
