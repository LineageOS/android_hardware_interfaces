/*
 * Copyright 2016 The Android Open Source Project
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

#include <atomic>
#include <cstring>  // for strerror
#include <type_traits>
#include <unordered_set>
#include <vector>

#include <android/hardware/graphics/composer/2.1/IComposer.h>
#include <composer-hal/2.1/ComposerHal.h>
#define HWC2_INCLUDE_STRINGIFICATION
#define HWC2_USE_CPP11
#include <hardware/hwcomposer2.h>
#undef HWC2_INCLUDE_STRINGIFICATION
#undef HWC2_USE_CPP11
#include <log/log.h>

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_1 {
namespace passthrough {

namespace detail {

using android::hardware::graphics::common::V1_0::ColorMode;
using android::hardware::graphics::common::V1_0::ColorTransform;
using android::hardware::graphics::common::V1_0::Dataspace;
using android::hardware::graphics::common::V1_0::Hdr;
using android::hardware::graphics::common::V1_0::PixelFormat;
using android::hardware::graphics::common::V1_0::Transform;

// HwcHalImpl implements V2_*::hal::ComposerHal on top of hwcomposer2
template <typename Hal>
class HwcHalImpl : public Hal {
   public:
    virtual ~HwcHalImpl() {
        if (mDevice) {
            hwc2_close(mDevice);
        }
    }

    bool initWithModule(const hw_module_t* module) {
        hwc2_device_t* device;
        int result = hwc2_open(module, &device);
        if (result) {
            ALOGE("failed to open hwcomposer2 device: %s", strerror(-result));
            return false;
        }

        return initWithDevice(std::move(device), true);
    }

    bool initWithDevice(hwc2_device_t* device, bool requireReliablePresentFence) {
        // we own the device from this point on
        mDevice = device;

        initCapabilities();
        if (requireReliablePresentFence &&
            hasCapability(HWC2_CAPABILITY_PRESENT_FENCE_IS_NOT_RELIABLE)) {
            ALOGE("present fence must be reliable");
            mDevice->common.close(&mDevice->common);
            mDevice = nullptr;
            return false;
        }

        if (!initDispatch()) {
            mDevice->common.close(&mDevice->common);
            mDevice = nullptr;
            return false;
        }

        return true;
    }

    bool hasCapability(hwc2_capability_t capability) override {
        return (mCapabilities.count(capability) > 0);
    }

    std::string dumpDebugInfo() override {
        uint32_t len = 0;
        mDispatch.dump(mDevice, &len, nullptr);

        std::vector<char> buf(len + 1);
        mDispatch.dump(mDevice, &len, buf.data());
        buf.resize(len + 1);
        buf[len] = '\0';

        return buf.data();
    }

    void registerEventCallback(hal::ComposerHal::EventCallback* callback) override {
        mEventCallback = callback;

        mDispatch.registerCallback(mDevice, HWC2_CALLBACK_HOTPLUG, this,
                                   reinterpret_cast<hwc2_function_pointer_t>(hotplugHook));
        mDispatch.registerCallback(mDevice, HWC2_CALLBACK_REFRESH, this,
                                   reinterpret_cast<hwc2_function_pointer_t>(refreshHook));
        mDispatch.registerCallback(mDevice, HWC2_CALLBACK_VSYNC, this,
                                   reinterpret_cast<hwc2_function_pointer_t>(vsyncHook));
    }

    void unregisterEventCallback() override {
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

        mEventCallback = nullptr;
    }

    uint32_t getMaxVirtualDisplayCount() override {
        return mDispatch.getMaxVirtualDisplayCount(mDevice);
    }

    Error createVirtualDisplay(uint32_t width, uint32_t height, PixelFormat* format,
                               Display* outDisplay) override {
        int32_t hwc_format = static_cast<int32_t>(*format);
        int32_t err =
            mDispatch.createVirtualDisplay(mDevice, width, height, &hwc_format, outDisplay);
        *format = static_cast<PixelFormat>(hwc_format);

        return static_cast<Error>(err);
    }

    Error destroyVirtualDisplay(Display display) override {
        int32_t err = mDispatch.destroyVirtualDisplay(mDevice, display);
        return static_cast<Error>(err);
    }

    Error createLayer(Display display, Layer* outLayer) override {
        int32_t err = mDispatch.createLayer(mDevice, display, outLayer);
        return static_cast<Error>(err);
    }

    Error destroyLayer(Display display, Layer layer) override {
        int32_t err = mDispatch.destroyLayer(mDevice, display, layer);
        onLayerDestroyed(display, layer);
        return static_cast<Error>(err);
    }

    Error getActiveConfig(Display display, Config* outConfig) override {
        int32_t err = mDispatch.getActiveConfig(mDevice, display, outConfig);
        return static_cast<Error>(err);
    }

    Error getClientTargetSupport(Display display, uint32_t width, uint32_t height,
                                 PixelFormat format, Dataspace dataspace) override {
        int32_t err = mDispatch.getClientTargetSupport(mDevice, display, width, height,
                                                       static_cast<int32_t>(format),
                                                       static_cast<int32_t>(dataspace));
        return static_cast<Error>(err);
    }

    Error getColorModes(Display display, hidl_vec<ColorMode>* outModes) override {
        uint32_t count = 0;
        int32_t err = mDispatch.getColorModes(mDevice, display, &count, nullptr);
        if (err != HWC2_ERROR_NONE) {
            return static_cast<Error>(err);
        }

        outModes->resize(count);
        err = mDispatch.getColorModes(
            mDevice, display, &count,
            reinterpret_cast<std::underlying_type<ColorMode>::type*>(outModes->data()));
        if (err != HWC2_ERROR_NONE) {
            *outModes = hidl_vec<ColorMode>();
            return static_cast<Error>(err);
        }

        return Error::NONE;
    }

    Error getDisplayAttribute(Display display, Config config, IComposerClient::Attribute attribute,
                              int32_t* outValue) override {
        int32_t err = mDispatch.getDisplayAttribute(mDevice, display, config,
                                                    static_cast<int32_t>(attribute), outValue);
        return static_cast<Error>(err);
    }

    Error getDisplayConfigs(Display display, hidl_vec<Config>* outConfigs) override {
        uint32_t count = 0;
        int32_t err = mDispatch.getDisplayConfigs(mDevice, display, &count, nullptr);
        if (err != HWC2_ERROR_NONE) {
            return static_cast<Error>(err);
        }

        outConfigs->resize(count);
        err = mDispatch.getDisplayConfigs(mDevice, display, &count, outConfigs->data());
        if (err != HWC2_ERROR_NONE) {
            *outConfigs = hidl_vec<Config>();
            return static_cast<Error>(err);
        }

        return Error::NONE;
    }

    Error getDisplayName(Display display, hidl_string* outName) override {
        uint32_t count = 0;
        int32_t err = mDispatch.getDisplayName(mDevice, display, &count, nullptr);
        if (err != HWC2_ERROR_NONE) {
            return static_cast<Error>(err);
        }

        std::vector<char> buf(count + 1);
        err = mDispatch.getDisplayName(mDevice, display, &count, buf.data());
        if (err != HWC2_ERROR_NONE) {
            return static_cast<Error>(err);
        }
        buf.resize(count + 1);
        buf[count] = '\0';

        *outName = buf.data();

        return Error::NONE;
    }

    Error getDisplayType(Display display, IComposerClient::DisplayType* outType) override {
        int32_t hwc_type = HWC2_DISPLAY_TYPE_INVALID;
        int32_t err = mDispatch.getDisplayType(mDevice, display, &hwc_type);
        *outType = static_cast<IComposerClient::DisplayType>(hwc_type);

        return static_cast<Error>(err);
    }

    Error getDozeSupport(Display display, bool* outSupport) override {
        int32_t hwc_support = 0;
        int32_t err = mDispatch.getDozeSupport(mDevice, display, &hwc_support);
        *outSupport = hwc_support;

        return static_cast<Error>(err);
    }

    Error getHdrCapabilities(Display display, hidl_vec<Hdr>* outTypes, float* outMaxLuminance,
                             float* outMaxAverageLuminance, float* outMinLuminance) override {
        uint32_t count = 0;
        int32_t err =
            mDispatch.getHdrCapabilities(mDevice, display, &count, nullptr, outMaxLuminance,
                                         outMaxAverageLuminance, outMinLuminance);
        if (err != HWC2_ERROR_NONE) {
            return static_cast<Error>(err);
        }

        outTypes->resize(count);
        err = mDispatch.getHdrCapabilities(
            mDevice, display, &count,
            reinterpret_cast<std::underlying_type<Hdr>::type*>(outTypes->data()), outMaxLuminance,
            outMaxAverageLuminance, outMinLuminance);
        if (err != HWC2_ERROR_NONE) {
            *outTypes = hidl_vec<Hdr>();
            return static_cast<Error>(err);
        }

        return Error::NONE;
    }

    Error setActiveConfig(Display display, Config config) override {
        int32_t err = mDispatch.setActiveConfig(mDevice, display, config);
        return static_cast<Error>(err);
    }

    Error setColorMode(Display display, ColorMode mode) override {
        int32_t err = mDispatch.setColorMode(mDevice, display, static_cast<int32_t>(mode));
        return static_cast<Error>(err);
    }

    Error setPowerMode(Display display, IComposerClient::PowerMode mode) override {
        int32_t err = mDispatch.setPowerMode(mDevice, display, static_cast<int32_t>(mode));
        return static_cast<Error>(err);
    }

    Error setVsyncEnabled(Display display, IComposerClient::Vsync enabled) override {
        int32_t err = mDispatch.setVsyncEnabled(mDevice, display, static_cast<int32_t>(enabled));
        return static_cast<Error>(err);
    }

    Error setColorTransform(Display display, const float* matrix, int32_t hint) override {
        int32_t err = mDispatch.setColorTransform(mDevice, display, matrix, hint);
        return static_cast<Error>(err);
    }

    Error setClientTarget(Display display, buffer_handle_t target, int32_t acquireFence,
                          int32_t dataspace, const std::vector<hwc_rect_t>& damage) override {
        hwc_region region = {damage.size(), damage.data()};
        int32_t err =
            mDispatch.setClientTarget(mDevice, display, target, acquireFence, dataspace, region);
        return static_cast<Error>(err);
    }

    Error setOutputBuffer(Display display, buffer_handle_t buffer, int32_t releaseFence) override {
        int32_t err = mDispatch.setOutputBuffer(mDevice, display, buffer, releaseFence);
        // unlike in setClientTarget, releaseFence is owned by us
        if (err == HWC2_ERROR_NONE && releaseFence >= 0) {
            close(releaseFence);
        }

        return static_cast<Error>(err);
    }

    Error validateDisplay(Display display, std::vector<Layer>* outChangedLayers,
                          std::vector<IComposerClient::Composition>* outCompositionTypes,
                          uint32_t* outDisplayRequestMask, std::vector<Layer>* outRequestedLayers,
                          std::vector<uint32_t>* outRequestMasks) override {
        onBeforeValidateDisplay(display);
        uint32_t typesCount = 0;
        uint32_t reqsCount = 0;
        int32_t err = mDispatch.validateDisplay(mDevice, display, &typesCount, &reqsCount);

        if (err != HWC2_ERROR_NONE && err != HWC2_ERROR_HAS_CHANGES) {
            return static_cast<Error>(err);
        }

        err = getChangedCompositionTypes(display, &typesCount, nullptr, nullptr);
        if (err != HWC2_ERROR_NONE) {
            return static_cast<Error>(err);
        }

        std::vector<Layer> changedLayers(typesCount);
        std::vector<IComposerClient::Composition> compositionTypes(typesCount);
        err = getChangedCompositionTypes(display, &typesCount, changedLayers.data(),
                                         compositionTypes.data());
        if (err != HWC2_ERROR_NONE) {
            return static_cast<Error>(err);
        }

        int32_t displayReqs = 0;
        err = mDispatch.getDisplayRequests(mDevice, display, &displayReqs, &reqsCount, nullptr,
                                           nullptr);
        if (err != HWC2_ERROR_NONE) {
            return static_cast<Error>(err);
        }

        std::vector<Layer> requestedLayers(reqsCount);
        std::vector<uint32_t> requestMasks(reqsCount);
        err = mDispatch.getDisplayRequests(mDevice, display, &displayReqs, &reqsCount,
                                           requestedLayers.data(),
                                           reinterpret_cast<int32_t*>(requestMasks.data()));
        if (err != HWC2_ERROR_NONE) {
            return static_cast<Error>(err);
        }

        *outChangedLayers = std::move(changedLayers);
        *outCompositionTypes = std::move(compositionTypes);
        *outDisplayRequestMask = displayReqs;
        *outRequestedLayers = std::move(requestedLayers);
        *outRequestMasks = std::move(requestMasks);

        return static_cast<Error>(err);
    }

    Error acceptDisplayChanges(Display display) override {
        int32_t err = mDispatch.acceptDisplayChanges(mDevice, display);
        return static_cast<Error>(err);
    }

    Error presentDisplay(Display display, int32_t* outPresentFence, std::vector<Layer>* outLayers,
                         std::vector<int32_t>* outReleaseFences) override {
        *outPresentFence = -1;
        int32_t err = mDispatch.presentDisplay(mDevice, display, outPresentFence);
        if (err != HWC2_ERROR_NONE) {
            return static_cast<Error>(err);
        }

        uint32_t count = 0;
        err = mDispatch.getReleaseFences(mDevice, display, &count, nullptr, nullptr);
        if (err != HWC2_ERROR_NONE) {
            ALOGW("failed to get release fences");
            return Error::NONE;
        }

        outLayers->resize(count);
        outReleaseFences->resize(count);
        err = mDispatch.getReleaseFences(mDevice, display, &count, outLayers->data(),
                                         outReleaseFences->data());
        if (err != HWC2_ERROR_NONE) {
            ALOGW("failed to get release fences");
            outLayers->clear();
            outReleaseFences->clear();
            return Error::NONE;
        }

        return static_cast<Error>(err);
    }

    Error setLayerCursorPosition(Display display, Layer layer, int32_t x, int32_t y) override {
        int32_t err = mDispatch.setCursorPosition(mDevice, display, layer, x, y);
        return static_cast<Error>(err);
    }

    Error setLayerBuffer(Display display, Layer layer, buffer_handle_t buffer,
                         int32_t acquireFence) override {
        int32_t err = mDispatch.setLayerBuffer(mDevice, display, layer, buffer, acquireFence);
        return static_cast<Error>(err);
    }

    Error setLayerSurfaceDamage(Display display, Layer layer,
                                const std::vector<hwc_rect_t>& damage) override {
        hwc_region region = {damage.size(), damage.data()};
        int32_t err = mDispatch.setLayerSurfaceDamage(mDevice, display, layer, region);
        return static_cast<Error>(err);
    }

    Error setLayerBlendMode(Display display, Layer layer, int32_t mode) override {
        int32_t err = mDispatch.setLayerBlendMode(mDevice, display, layer, mode);
        return static_cast<Error>(err);
    }

    Error setLayerColor(Display display, Layer layer, IComposerClient::Color color) override {
        hwc_color_t hwc_color{color.r, color.g, color.b, color.a};
        int32_t err = mDispatch.setLayerColor(mDevice, display, layer, hwc_color);
        return static_cast<Error>(err);
    }

    Error setLayerCompositionType(Display display, Layer layer, int32_t type) override {
        int32_t err = mDispatch.setLayerCompositionType(mDevice, display, layer, type);
        return static_cast<Error>(err);
    }

    Error setLayerDataspace(Display display, Layer layer, int32_t dataspace) override {
        int32_t err = mDispatch.setLayerDataspace(mDevice, display, layer, dataspace);
        return static_cast<Error>(err);
    }

    Error setLayerDisplayFrame(Display display, Layer layer, const hwc_rect_t& frame) override {
        int32_t err = mDispatch.setLayerDisplayFrame(mDevice, display, layer, frame);
        return static_cast<Error>(err);
    }

    Error setLayerPlaneAlpha(Display display, Layer layer, float alpha) override {
        int32_t err = mDispatch.setLayerPlaneAlpha(mDevice, display, layer, alpha);
        return static_cast<Error>(err);
    }

    Error setLayerSidebandStream(Display display, Layer layer, buffer_handle_t stream) override {
        int32_t err = mDispatch.setLayerSidebandStream(mDevice, display, layer, stream);
        return static_cast<Error>(err);
    }

    Error setLayerSourceCrop(Display display, Layer layer, const hwc_frect_t& crop) override {
        int32_t err = mDispatch.setLayerSourceCrop(mDevice, display, layer, crop);
        return static_cast<Error>(err);
    }

    Error setLayerTransform(Display display, Layer layer, int32_t transform) override {
        int32_t err = mDispatch.setLayerTransform(mDevice, display, layer, transform);
        return static_cast<Error>(err);
    }

    Error setLayerVisibleRegion(Display display, Layer layer,
                                const std::vector<hwc_rect_t>& visible) override {
        hwc_region_t region = {visible.size(), visible.data()};
        int32_t err = mDispatch.setLayerVisibleRegion(mDevice, display, layer, region);
        return static_cast<Error>(err);
    }

    Error setLayerZOrder(Display display, Layer layer, uint32_t z) override {
        int32_t err = mDispatch.setLayerZOrder(mDevice, display, layer, z);
        return static_cast<Error>(err);
    }

   protected:
    virtual void initCapabilities() {
        uint32_t count = 0;
        mDevice->getCapabilities(mDevice, &count, nullptr);

        std::vector<int32_t> caps(count);
        mDevice->getCapabilities(mDevice, &count, caps.data());
        caps.resize(count);

        mCapabilities.reserve(count);
        for (auto cap : caps) {
            mCapabilities.insert(static_cast<hwc2_capability_t>(cap));
        }
    }

    template <typename T>
    bool initDispatch(hwc2_function_descriptor_t desc, T* outPfn) {
        auto pfn = mDevice->getFunction(mDevice, desc);
        if (pfn) {
            *outPfn = reinterpret_cast<T>(pfn);
            return true;
        } else {
            ALOGE("failed to get hwcomposer2 function %d", desc);
            return false;
        }
    }

    virtual bool initDispatch() {
        if (!initDispatch(HWC2_FUNCTION_ACCEPT_DISPLAY_CHANGES, &mDispatch.acceptDisplayChanges) ||
            !initDispatch(HWC2_FUNCTION_CREATE_LAYER, &mDispatch.createLayer) ||
            !initDispatch(HWC2_FUNCTION_CREATE_VIRTUAL_DISPLAY, &mDispatch.createVirtualDisplay) ||
            !initDispatch(HWC2_FUNCTION_DESTROY_LAYER, &mDispatch.destroyLayer) ||
            !initDispatch(HWC2_FUNCTION_DESTROY_VIRTUAL_DISPLAY,
                          &mDispatch.destroyVirtualDisplay) ||
            !initDispatch(HWC2_FUNCTION_DUMP, &mDispatch.dump) ||
            !initDispatch(HWC2_FUNCTION_GET_ACTIVE_CONFIG, &mDispatch.getActiveConfig) ||
            !initDispatch(HWC2_FUNCTION_GET_CHANGED_COMPOSITION_TYPES,
                          &mDispatch.getChangedCompositionTypes) ||
            !initDispatch(HWC2_FUNCTION_GET_CLIENT_TARGET_SUPPORT,
                          &mDispatch.getClientTargetSupport) ||
            !initDispatch(HWC2_FUNCTION_GET_COLOR_MODES, &mDispatch.getColorModes) ||
            !initDispatch(HWC2_FUNCTION_GET_DISPLAY_ATTRIBUTE, &mDispatch.getDisplayAttribute) ||
            !initDispatch(HWC2_FUNCTION_GET_DISPLAY_CONFIGS, &mDispatch.getDisplayConfigs) ||
            !initDispatch(HWC2_FUNCTION_GET_DISPLAY_NAME, &mDispatch.getDisplayName) ||
            !initDispatch(HWC2_FUNCTION_GET_DISPLAY_REQUESTS, &mDispatch.getDisplayRequests) ||
            !initDispatch(HWC2_FUNCTION_GET_DISPLAY_TYPE, &mDispatch.getDisplayType) ||
            !initDispatch(HWC2_FUNCTION_GET_DOZE_SUPPORT, &mDispatch.getDozeSupport) ||
            !initDispatch(HWC2_FUNCTION_GET_HDR_CAPABILITIES, &mDispatch.getHdrCapabilities) ||
            !initDispatch(HWC2_FUNCTION_GET_MAX_VIRTUAL_DISPLAY_COUNT,
                          &mDispatch.getMaxVirtualDisplayCount) ||
            !initDispatch(HWC2_FUNCTION_GET_RELEASE_FENCES, &mDispatch.getReleaseFences) ||
            !initDispatch(HWC2_FUNCTION_PRESENT_DISPLAY, &mDispatch.presentDisplay) ||
            !initDispatch(HWC2_FUNCTION_REGISTER_CALLBACK, &mDispatch.registerCallback) ||
            !initDispatch(HWC2_FUNCTION_SET_ACTIVE_CONFIG, &mDispatch.setActiveConfig) ||
            !initDispatch(HWC2_FUNCTION_SET_CLIENT_TARGET, &mDispatch.setClientTarget) ||
            !initDispatch(HWC2_FUNCTION_SET_COLOR_MODE, &mDispatch.setColorMode) ||
            !initDispatch(HWC2_FUNCTION_SET_COLOR_TRANSFORM, &mDispatch.setColorTransform) ||
            !initDispatch(HWC2_FUNCTION_SET_CURSOR_POSITION, &mDispatch.setCursorPosition) ||
            !initDispatch(HWC2_FUNCTION_SET_LAYER_BLEND_MODE, &mDispatch.setLayerBlendMode) ||
            !initDispatch(HWC2_FUNCTION_SET_LAYER_BUFFER, &mDispatch.setLayerBuffer) ||
            !initDispatch(HWC2_FUNCTION_SET_LAYER_COLOR, &mDispatch.setLayerColor) ||
            !initDispatch(HWC2_FUNCTION_SET_LAYER_COMPOSITION_TYPE,
                          &mDispatch.setLayerCompositionType) ||
            !initDispatch(HWC2_FUNCTION_SET_LAYER_DATASPACE, &mDispatch.setLayerDataspace) ||
            !initDispatch(HWC2_FUNCTION_SET_LAYER_DISPLAY_FRAME, &mDispatch.setLayerDisplayFrame) ||
            !initDispatch(HWC2_FUNCTION_SET_LAYER_PLANE_ALPHA, &mDispatch.setLayerPlaneAlpha)) {
            return false;
        }

        if (hasCapability(HWC2_CAPABILITY_SIDEBAND_STREAM)) {
            if (!initDispatch(HWC2_FUNCTION_SET_LAYER_SIDEBAND_STREAM,
                              &mDispatch.setLayerSidebandStream)) {
                return false;
            }
        }

        if (!initDispatch(HWC2_FUNCTION_SET_LAYER_SOURCE_CROP, &mDispatch.setLayerSourceCrop) ||
            !initDispatch(HWC2_FUNCTION_SET_LAYER_SURFACE_DAMAGE,
                          &mDispatch.setLayerSurfaceDamage) ||
            !initDispatch(HWC2_FUNCTION_SET_LAYER_TRANSFORM, &mDispatch.setLayerTransform) ||
            !initDispatch(HWC2_FUNCTION_SET_LAYER_VISIBLE_REGION,
                          &mDispatch.setLayerVisibleRegion) ||
            !initDispatch(HWC2_FUNCTION_SET_LAYER_Z_ORDER, &mDispatch.setLayerZOrder) ||
            !initDispatch(HWC2_FUNCTION_SET_OUTPUT_BUFFER, &mDispatch.setOutputBuffer) ||
            !initDispatch(HWC2_FUNCTION_SET_POWER_MODE, &mDispatch.setPowerMode) ||
            !initDispatch(HWC2_FUNCTION_SET_VSYNC_ENABLED, &mDispatch.setVsyncEnabled) ||
            !initDispatch(HWC2_FUNCTION_VALIDATE_DISPLAY, &mDispatch.validateDisplay)) {
            return false;
        }

        return true;
    }

    virtual int32_t getChangedCompositionTypes(Display display, uint32_t* outTypesCount,
                                               Layer* outChangedLayers,
                                               IComposerClient::Composition* outCompositionTypes) {
        return getChangedCompositionTypesInternal(display, outTypesCount, outChangedLayers,
                                                  outCompositionTypes);
    }
    virtual void onLayerDestroyed(Display /* display */, Layer /* layer */) {}
    virtual void onBeforeValidateDisplay(Display /* display */) {}

    static void hotplugHook(hwc2_callback_data_t callbackData, hwc2_display_t display,
                            int32_t connected) {
        auto hal = static_cast<HwcHalImpl*>(callbackData);
        hal->mEventCallback->onHotplug(display,
                                       static_cast<IComposerCallback::Connection>(connected));
    }

    static void refreshHook(hwc2_callback_data_t callbackData, hwc2_display_t display) {
        auto hal = static_cast<HwcHalImpl*>(callbackData);
        hal->mEventCallback->onRefresh(display);
    }

    static void vsyncHook(hwc2_callback_data_t callbackData, hwc2_display_t display,
                          int64_t timestamp) {
        auto hal = static_cast<HwcHalImpl*>(callbackData);
        hal->mEventCallback->onVsync(display, timestamp);
    }

    int32_t getChangedCompositionTypesInternal(Display display, uint32_t* outTypesCount,
                                               Layer* outChangedLayers,
                                               IComposerClient::Composition* outCompositionTypes) {
        return mDispatch.getChangedCompositionTypes(
                mDevice, display, outTypesCount, outChangedLayers,
                reinterpret_cast<std::underlying_type<IComposerClient::Composition>::type*>(
                        outCompositionTypes));
    }

    hwc2_device_t* mDevice = nullptr;

    std::unordered_set<hwc2_capability_t> mCapabilities;

    struct {
        HWC2_PFN_ACCEPT_DISPLAY_CHANGES acceptDisplayChanges;
        HWC2_PFN_CREATE_LAYER createLayer;
        HWC2_PFN_CREATE_VIRTUAL_DISPLAY createVirtualDisplay;
        HWC2_PFN_DESTROY_LAYER destroyLayer;
        HWC2_PFN_DESTROY_VIRTUAL_DISPLAY destroyVirtualDisplay;
        HWC2_PFN_DUMP dump;
        HWC2_PFN_GET_ACTIVE_CONFIG getActiveConfig;
        HWC2_PFN_GET_CHANGED_COMPOSITION_TYPES getChangedCompositionTypes;
        HWC2_PFN_GET_CLIENT_TARGET_SUPPORT getClientTargetSupport;
        HWC2_PFN_GET_COLOR_MODES getColorModes;
        HWC2_PFN_GET_DISPLAY_ATTRIBUTE getDisplayAttribute;
        HWC2_PFN_GET_DISPLAY_CONFIGS getDisplayConfigs;
        HWC2_PFN_GET_DISPLAY_NAME getDisplayName;
        HWC2_PFN_GET_DISPLAY_REQUESTS getDisplayRequests;
        HWC2_PFN_GET_DISPLAY_TYPE getDisplayType;
        HWC2_PFN_GET_DOZE_SUPPORT getDozeSupport;
        HWC2_PFN_GET_HDR_CAPABILITIES getHdrCapabilities;
        HWC2_PFN_GET_MAX_VIRTUAL_DISPLAY_COUNT getMaxVirtualDisplayCount;
        HWC2_PFN_GET_RELEASE_FENCES getReleaseFences;
        HWC2_PFN_PRESENT_DISPLAY presentDisplay;
        HWC2_PFN_REGISTER_CALLBACK registerCallback;
        HWC2_PFN_SET_ACTIVE_CONFIG setActiveConfig;
        HWC2_PFN_SET_CLIENT_TARGET setClientTarget;
        HWC2_PFN_SET_COLOR_MODE setColorMode;
        HWC2_PFN_SET_COLOR_TRANSFORM setColorTransform;
        HWC2_PFN_SET_CURSOR_POSITION setCursorPosition;
        HWC2_PFN_SET_LAYER_BLEND_MODE setLayerBlendMode;
        HWC2_PFN_SET_LAYER_BUFFER setLayerBuffer;
        HWC2_PFN_SET_LAYER_COLOR setLayerColor;
        HWC2_PFN_SET_LAYER_COMPOSITION_TYPE setLayerCompositionType;
        HWC2_PFN_SET_LAYER_DATASPACE setLayerDataspace;
        HWC2_PFN_SET_LAYER_DISPLAY_FRAME setLayerDisplayFrame;
        HWC2_PFN_SET_LAYER_PLANE_ALPHA setLayerPlaneAlpha;
        HWC2_PFN_SET_LAYER_SIDEBAND_STREAM setLayerSidebandStream;
        HWC2_PFN_SET_LAYER_SOURCE_CROP setLayerSourceCrop;
        HWC2_PFN_SET_LAYER_SURFACE_DAMAGE setLayerSurfaceDamage;
        HWC2_PFN_SET_LAYER_TRANSFORM setLayerTransform;
        HWC2_PFN_SET_LAYER_VISIBLE_REGION setLayerVisibleRegion;
        HWC2_PFN_SET_LAYER_Z_ORDER setLayerZOrder;
        HWC2_PFN_SET_OUTPUT_BUFFER setOutputBuffer;
        HWC2_PFN_SET_POWER_MODE setPowerMode;
        HWC2_PFN_SET_VSYNC_ENABLED setVsyncEnabled;
        HWC2_PFN_VALIDATE_DISPLAY validateDisplay;
    } mDispatch = {};

    hal::ComposerHal::EventCallback* mEventCallback = nullptr;
};

}  // namespace detail

using HwcHal = detail::HwcHalImpl<hal::ComposerHal>;

}  // namespace passthrough
}  // namespace V2_1
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android
