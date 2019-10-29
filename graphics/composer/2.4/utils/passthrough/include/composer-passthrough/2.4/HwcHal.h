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

#include <android/hardware/graphics/composer/2.4/IComposerClient.h>
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
using V2_1::Config;
using V2_1::Display;
using V2_4::Error;

// HwcHalImpl implements V2_*::hal::ComposerHal on top of hwcomposer2
template <typename Hal>
class HwcHalImpl : public V2_3::passthrough::detail::HwcHalImpl<Hal> {
  public:
    void registerEventCallback_2_4(hal::ComposerHal::EventCallback_2_4* callback) override {
        mEventCallback_2_4 = callback;

        mDispatch.registerCallback(mDevice, HWC2_CALLBACK_HOTPLUG, this,
                                   reinterpret_cast<hwc2_function_pointer_t>(hotplugHook));
        mDispatch.registerCallback(mDevice, HWC2_CALLBACK_REFRESH, this,
                                   reinterpret_cast<hwc2_function_pointer_t>(refreshHook));
        mDispatch.registerCallback(mDevice, HWC2_CALLBACK_VSYNC, this,
                                   reinterpret_cast<hwc2_function_pointer_t>(vsyncHook));
        mDispatch.registerCallback(mDevice, HWC2_CALLBACK_VSYNC_2_4, this,
                                   reinterpret_cast<hwc2_function_pointer_t>(vsync_2_4_Hook));
    }

    void unregisterEventCallback_2_4() override {
        // we assume the callback functions
        //
        //  - can be unregistered
        //  - can be in-flight
        //  - will never be called afterward
        //
        // which is likely incorrect
        mDispatch.registerCallback(mDevice, HWC2_CALLBACK_HOTPLUG, this, nullptr);
        mDispatch.registerCallback(mDevice, HWC2_CALLBACK_REFRESH, this, nullptr);
        mDispatch.registerCallback(mDevice, HWC2_CALLBACK_VSYNC, this, nullptr);
        mDispatch.registerCallback(mDevice, HWC2_CALLBACK_VSYNC_2_4, this, nullptr);

        mEventCallback_2_4 = nullptr;
    }

    Error getDisplayCapabilities_2_4(
            Display display,
            std::vector<IComposerClient::DisplayCapability>* outCapabilities) override {
        std::vector<V2_3::IComposerClient::DisplayCapability> capabilities;
        V2_3::Error error_2_3 = BaseType2_3::getDisplayCapabilities(display, &capabilities);
        Error error = static_cast<Error>(error_2_3);
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

    Error getSupportedDisplayVsyncPeriods(Display display, Config config,
                                          std::vector<VsyncPeriodNanos>* outVsyncPeriods) override {
        if (!mDispatch.getSupportedDisplayVsyncPeriods) {
            return Error::UNSUPPORTED;
        }

        uint32_t count = 0;
        int32_t error = mDispatch.getSupportedDisplayVsyncPeriods(mDevice, display, config, &count,
                                                                  nullptr);
        if (error != HWC2_ERROR_NONE) {
            return static_cast<Error>(error);
        }
        outVsyncPeriods->resize(count);
        error = mDispatch.getSupportedDisplayVsyncPeriods(mDevice, display, config, &count,
                                                          outVsyncPeriods->data());
        if (error != HWC2_ERROR_NONE) {
            *outVsyncPeriods = std::vector<VsyncPeriodNanos>();
            return static_cast<Error>(error);
        }
        return Error::NONE;
    }

    Error getDisplayVsyncPeriod(Display display, VsyncPeriodNanos* outVsyncPeriod) override {
        if (!mDispatch.getDisplayVsyncPeriod) {
            return Error::UNSUPPORTED;
        }

        int32_t error = mDispatch.getDisplayVsyncPeriod(mDevice, display, outVsyncPeriod);
        if (error != HWC2_ERROR_NONE) {
            return static_cast<Error>(error);
        }
        return Error::NONE;
    }

    Error setActiveConfigAndVsyncPeriod(
            Display display, Config config, VsyncPeriodNanos vsyncPeriodNanos,
            const IComposerClient::VsyncPeriodChangeConstraints& vsyncPeriodChangeConstraints,
            int64_t* outNewVsyncAppliedTime) override {
        if (!mDispatch.setActiveConfigAndVsyncPeriod) {
            return Error::UNSUPPORTED;
        }

        hwc_vsync_period_change_constraints_t vsync_period_change_constraints;
        vsync_period_change_constraints.desiredTimeNanos =
                vsyncPeriodChangeConstraints.desiredTimeNanos;
        vsync_period_change_constraints.seamlessRequired =
                vsyncPeriodChangeConstraints.seamlessRequired;

        int32_t error = mDispatch.setActiveConfigAndVsyncPeriod(
                mDevice, display, config, vsyncPeriodNanos, &vsync_period_change_constraints,
                outNewVsyncAppliedTime);
        if (error != HWC2_ERROR_NONE) {
            return static_cast<Error>(error);
        }
        return Error::NONE;
    }

  protected:
    bool initDispatch() override {
        if (!BaseType2_3::initDispatch()) {
            return false;
        }

        this->initOptionalDispatch(HWC2_FUNCTION_GET_DISPLAY_CONNECTION_TYPE,
                                   &mDispatch.getDisplayConnectionType);
        this->initOptionalDispatch(HWC2_FUNCTION_REGISTER_CALLBACK, &mDispatch.registerCallback);
        this->initOptionalDispatch(HWC2_FUNCTION_GET_SUPPORTED_DISPLAY_VSYNC_PERIODS,
                                   &mDispatch.getSupportedDisplayVsyncPeriods);
        this->initOptionalDispatch(HWC2_FUNCTION_GET_DISPLAY_VSYNC_PERIOD,
                                   &mDispatch.getDisplayVsyncPeriod);
        this->initOptionalDispatch(HWC2_FUNCTION_SET_ACTIVE_CONFIG_AND_VSYNC_PERIOD,
                                   &mDispatch.setActiveConfigAndVsyncPeriod);
        return true;
    }

    static void hotplugHook(hwc2_callback_data_t callbackData, hwc2_display_t display,
                            int32_t connected) {
        auto hal = static_cast<HwcHalImpl*>(callbackData);
        hal->mEventCallback_2_4->onHotplug(display,
                                           static_cast<IComposerCallback::Connection>(connected));
    }

    static void refreshHook(hwc2_callback_data_t callbackData, hwc2_display_t display) {
        auto hal = static_cast<HwcHalImpl*>(callbackData);
        hal->mEventCallback_2_4->onRefresh(display);
    }

    static void vsyncHook(hwc2_callback_data_t callbackData, hwc2_display_t display,
                          int64_t timestamp) {
        auto hal = static_cast<HwcHalImpl*>(callbackData);
        hal->mEventCallback_2_4->onVsync(display, timestamp);
    }

    static void vsync_2_4_Hook(hwc2_callback_data_t callbackData, hwc2_display_t display,
                               int64_t timestamp, hwc2_vsync_period_t vsyncPeriodNanos) {
        auto hal = static_cast<HwcHalImpl*>(callbackData);
        hal->mEventCallback_2_4->onVsync_2_4(display, timestamp, vsyncPeriodNanos);
    }

  private:
    struct {
        HWC2_PFN_GET_DISPLAY_CONNECTION_TYPE getDisplayConnectionType;
        HWC2_PFN_REGISTER_CALLBACK registerCallback;
        HWC2_PFN_GET_SUPPORTED_DISPLAY_VSYNC_PERIODS getSupportedDisplayVsyncPeriods;
        HWC2_PFN_GET_DISPLAY_VSYNC_PERIOD getDisplayVsyncPeriod;
        HWC2_PFN_SET_ACTIVE_CONFIG_AND_VSYNC_PERIOD setActiveConfigAndVsyncPeriod;
    } mDispatch = {};

    hal::ComposerHal::EventCallback_2_4* mEventCallback_2_4 = nullptr;

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
