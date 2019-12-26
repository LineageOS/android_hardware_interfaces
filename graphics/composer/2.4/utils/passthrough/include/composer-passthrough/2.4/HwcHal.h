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
#include <hardware/hwcomposer_defs.h>

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

        BaseType2_1::mDispatch.registerCallback(
                mDevice, HWC2_CALLBACK_HOTPLUG, this,
                reinterpret_cast<hwc2_function_pointer_t>(hotplugHook));
        BaseType2_1::mDispatch.registerCallback(
                mDevice, HWC2_CALLBACK_REFRESH, this,
                reinterpret_cast<hwc2_function_pointer_t>(refreshHook));
        BaseType2_1::mDispatch.registerCallback(
                mDevice, HWC2_CALLBACK_VSYNC, this,
                reinterpret_cast<hwc2_function_pointer_t>(vsyncHook));
        BaseType2_1::mDispatch.registerCallback(
                mDevice, HWC2_CALLBACK_VSYNC_2_4, this,
                reinterpret_cast<hwc2_function_pointer_t>(vsync_2_4_Hook));

        BaseType2_1::mDispatch.registerCallback(
                mDevice, HWC2_CALLBACK_VSYNC_PERIOD_TIMING_CHANGED, this,
                reinterpret_cast<hwc2_function_pointer_t>(vsyncPeriodTimingChangedHook));
    }

    void unregisterEventCallback_2_4() override {
        // we assume the callback functions
        //
        //  - can be unregistered
        //  - can be in-flight
        //  - will never be called afterward
        //
        // which is likely incorrect
        BaseType2_1::mDispatch.registerCallback(mDevice, HWC2_CALLBACK_HOTPLUG, this, nullptr);
        BaseType2_1::mDispatch.registerCallback(mDevice, HWC2_CALLBACK_REFRESH, this, nullptr);
        BaseType2_1::mDispatch.registerCallback(mDevice, HWC2_CALLBACK_VSYNC, this, nullptr);
        BaseType2_1::mDispatch.registerCallback(mDevice, HWC2_CALLBACK_VSYNC_2_4, this, nullptr);
        BaseType2_1::mDispatch.registerCallback(mDevice, HWC2_CALLBACK_VSYNC_PERIOD_TIMING_CHANGED,
                                                this, nullptr);

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

    Error getDisplayAttribute_2_4(Display display, Config config,
                                  IComposerClient::Attribute attribute,
                                  int32_t* outValue) override {
        int32_t err = BaseType2_1::mDispatch.getDisplayAttribute(
                mDevice, display, config, static_cast<int32_t>(attribute), outValue);
        if (err != HWC2_ERROR_NONE && *outValue == -1) {
            // Convert the error from hwcomposer2 to IComposerClient definition
            return Error::BAD_PARAMETER;
        }
        return static_cast<Error>(err);
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

    Error setActiveConfigWithConstraints(
            Display display, Config config,
            const IComposerClient::VsyncPeriodChangeConstraints& vsyncPeriodChangeConstraints,
            VsyncPeriodChangeTimeline* timeline) override {
        if (!mDispatch.setActiveConfigWithConstraints) {
            return Error::UNSUPPORTED;
        }

        hwc_vsync_period_change_constraints_t vsync_period_change_constraints;
        vsync_period_change_constraints.desiredTimeNanos =
                vsyncPeriodChangeConstraints.desiredTimeNanos;
        vsync_period_change_constraints.seamlessRequired =
                vsyncPeriodChangeConstraints.seamlessRequired;

        hwc_vsync_period_change_timeline_t out_timeline;
        int32_t error = mDispatch.setActiveConfigWithConstraints(
                mDevice, display, config, &vsync_period_change_constraints, &out_timeline);
        if (error != HWC2_ERROR_NONE) {
            return static_cast<Error>(error);
        }
        timeline->newVsyncAppliedTimeNanos = out_timeline.newVsyncAppliedTimeNanos;
        timeline->refreshRequired = out_timeline.refreshRequired;
        timeline->refreshTimeNanos = out_timeline.refreshTimeNanos;
        return Error::NONE;
    }

    Error setAutoLowLatencyMode(Display display, bool on) override {
        if (!mDispatch.setAutoLowLatencyMode) {
            return Error::UNSUPPORTED;
        }

        int32_t error = mDispatch.setAutoLowLatencyMode(mDevice, display, on);
        if (error != HWC2_ERROR_NONE) {
            return static_cast<Error>(error);
        }
        return Error::NONE;
    }

    Error getSupportedContentTypes(
            Display display,
            std::vector<IComposerClient::ContentType>* outSupportedContentTypes) override {
        if (!mDispatch.getSupportedContentTypes) {
            return Error::UNSUPPORTED;
        }

        uint32_t count = 0;
        int32_t error = mDispatch.getSupportedContentTypes(mDevice, display, &count, nullptr);
        if (error != HWC2_ERROR_NONE) {
            return static_cast<Error>(error);
        }

        outSupportedContentTypes->resize(count);

        error = mDispatch.getSupportedContentTypes(
                mDevice, display, &count,
                reinterpret_cast<std::underlying_type<IComposerClient::ContentType>::type*>(
                        outSupportedContentTypes->data()));
        if (error != HWC2_ERROR_NONE) {
            *outSupportedContentTypes = std::vector<IComposerClient::ContentType>();
            return static_cast<Error>(error);
        }
        return Error::NONE;
    }

    Error setContentType(Display display, IComposerClient::ContentType contentType) override {
        if (!mDispatch.setContentType) {
            return Error::UNSUPPORTED;
        }

        int32_t error =
                mDispatch.setContentType(mDevice, display, static_cast<int32_t>(contentType));
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
        this->initOptionalDispatch(HWC2_FUNCTION_GET_DISPLAY_VSYNC_PERIOD,
                                   &mDispatch.getDisplayVsyncPeriod);
        this->initOptionalDispatch(HWC2_FUNCTION_SET_ACTIVE_CONFIG_WITH_CONSTRAINTS,
                                   &mDispatch.setActiveConfigWithConstraints);
        this->initOptionalDispatch(HWC2_FUNCTION_SET_AUTO_LOW_LATENCY_MODE,
                                   &mDispatch.setAutoLowLatencyMode);
        this->initOptionalDispatch(HWC2_FUNCTION_GET_SUPPORTED_CONTENT_TYPES,
                                   &mDispatch.getSupportedContentTypes);
        this->initOptionalDispatch(HWC2_FUNCTION_SET_CONTENT_TYPE, &mDispatch.setContentType);
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

    static void vsyncPeriodTimingChangedHook(hwc2_callback_data_t callbackData,
                                             hwc2_display_t display,
                                             hwc_vsync_period_change_timeline_t* updated_timeline) {
        auto hal = static_cast<HwcHalImpl*>(callbackData);
        VsyncPeriodChangeTimeline timeline;
        timeline.newVsyncAppliedTimeNanos = updated_timeline->newVsyncAppliedTimeNanos;
        timeline.refreshRequired = updated_timeline->refreshRequired;
        timeline.refreshTimeNanos = updated_timeline->refreshTimeNanos;
        hal->mEventCallback_2_4->onVsyncPeriodTimingChanged(display, timeline);
    }

  private:
    struct {
        HWC2_PFN_GET_DISPLAY_CONNECTION_TYPE getDisplayConnectionType;
        HWC2_PFN_GET_DISPLAY_VSYNC_PERIOD getDisplayVsyncPeriod;
        HWC2_PFN_SET_ACTIVE_CONFIG_WITH_CONSTRAINTS setActiveConfigWithConstraints;
        HWC2_PFN_SET_AUTO_LOW_LATENCY_MODE setAutoLowLatencyMode;
        HWC2_PFN_GET_SUPPORTED_CONTENT_TYPES getSupportedContentTypes;
        HWC2_PFN_SET_CONTENT_TYPE setContentType;
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
