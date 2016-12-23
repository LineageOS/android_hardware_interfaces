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

#ifndef ANDROID_HARDWARE_GRAPHICS_COMPOSER_V2_1_HWC_H
#define ANDROID_HARDWARE_GRAPHICS_COMPOSER_V2_1_HWC_H

#include <mutex>
#include <unordered_set>
#include <vector>

#include <android/hardware/graphics/composer/2.1/IComposer.h>
#include <hardware/hwcomposer2.h>

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_1 {
namespace implementation {

using android::hardware::graphics::common::V1_0::PixelFormat;
using android::hardware::graphics::common::V1_0::Transform;
using android::hardware::graphics::common::V1_0::Dataspace;
using android::hardware::graphics::common::V1_0::ColorMode;
using android::hardware::graphics::common::V1_0::ColorTransform;
using android::hardware::graphics::common::V1_0::Hdr;

class HwcClient;

class HwcHal : public IComposer {
public:
    HwcHal(const hw_module_t* module);
    virtual ~HwcHal();

    // IComposer interface
    Return<void> getCapabilities(getCapabilities_cb hidl_cb) override;
    Return<void> dumpDebugInfo(dumpDebugInfo_cb hidl_cb) override;
    Return<void> createClient(createClient_cb hidl_cb) override;

    bool hasCapability(Capability capability) const;

    void removeClient();

    void enableCallback(bool enable);

    uint32_t getMaxVirtualDisplayCount();
    Error createVirtualDisplay(uint32_t width, uint32_t height,
        PixelFormat* format, Display* outDisplay);
    Error destroyVirtualDisplay(Display display);

    Error createLayer(Display display, Layer* outLayer);
    Error destroyLayer(Display display, Layer layer);

    Error getActiveConfig(Display display, Config* outConfig);
    Error getClientTargetSupport(Display display,
            uint32_t width, uint32_t height,
            PixelFormat format, Dataspace dataspace);
    Error getColorModes(Display display, hidl_vec<ColorMode>* outModes);
    Error getDisplayAttribute(Display display, Config config,
            IComposerClient::Attribute attribute, int32_t* outValue);
    Error getDisplayConfigs(Display display, hidl_vec<Config>* outConfigs);
    Error getDisplayName(Display display, hidl_string* outName);
    Error getDisplayType(Display display,
            IComposerClient::DisplayType* outType);
    Error getDozeSupport(Display display, bool* outSupport);
    Error getHdrCapabilities(Display display, hidl_vec<Hdr>* outTypes,
            float* outMaxLuminance, float* outMaxAverageLuminance,
            float* outMinLuminance);

    Error setActiveConfig(Display display, Config config);
    Error setColorMode(Display display, ColorMode mode);
    Error setPowerMode(Display display, IComposerClient::PowerMode mode);
    Error setVsyncEnabled(Display display, IComposerClient::Vsync enabled);

    Error setColorTransform(Display display, const float* matrix,
            int32_t hint);
    Error setClientTarget(Display display, buffer_handle_t target,
            int32_t acquireFence, int32_t dataspace,
            const std::vector<hwc_rect_t>& damage);
    Error setOutputBuffer(Display display, buffer_handle_t buffer,
            int32_t releaseFence);
    Error validateDisplay(Display display,
            std::vector<Layer>* outChangedLayers,
            std::vector<IComposerClient::Composition>* outCompositionTypes,
            uint32_t* outDisplayRequestMask,
            std::vector<Layer>* outRequestedLayers,
            std::vector<uint32_t>* outRequestMasks);
    Error acceptDisplayChanges(Display display);
    Error presentDisplay(Display display, int32_t* outPresentFence,
            std::vector<Layer>* outLayers,
            std::vector<int32_t>* outReleaseFences);

    Error setLayerCursorPosition(Display display, Layer layer,
            int32_t x, int32_t y);
    Error setLayerBuffer(Display display, Layer layer,
            buffer_handle_t buffer, int32_t acquireFence);
    Error setLayerSurfaceDamage(Display display, Layer layer,
            const std::vector<hwc_rect_t>& damage);
    Error setLayerBlendMode(Display display, Layer layer, int32_t mode);
    Error setLayerColor(Display display, Layer layer,
            IComposerClient::Color color);
    Error setLayerCompositionType(Display display, Layer layer,
            int32_t type);
    Error setLayerDataspace(Display display, Layer layer,
            int32_t dataspace);
    Error setLayerDisplayFrame(Display display, Layer layer,
            const hwc_rect_t& frame);
    Error setLayerPlaneAlpha(Display display, Layer layer, float alpha);
    Error setLayerSidebandStream(Display display, Layer layer,
            buffer_handle_t stream);
    Error setLayerSourceCrop(Display display, Layer layer,
            const hwc_frect_t& crop);
    Error setLayerTransform(Display display, Layer layer,
            int32_t transform);
    Error setLayerVisibleRegion(Display display, Layer layer,
            const std::vector<hwc_rect_t>& visible);
    Error setLayerZOrder(Display display, Layer layer, uint32_t z);

private:
    void initCapabilities();

    template<typename T>
    void initDispatch(hwc2_function_descriptor_t desc, T* outPfn);
    void initDispatch();

    sp<HwcClient> getClient();

    static void hotplugHook(hwc2_callback_data_t callbackData,
        hwc2_display_t display, int32_t connected);
    static void refreshHook(hwc2_callback_data_t callbackData,
        hwc2_display_t display);
    static void vsyncHook(hwc2_callback_data_t callbackData,
        hwc2_display_t display, int64_t timestamp);

    hwc2_device_t* mDevice;

    std::unordered_set<Capability> mCapabilities;

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
    } mDispatch;

    std::mutex mClientMutex;
    wp<HwcClient> mClient;
};

extern "C" IComposer* HIDL_FETCH_IComposer(const char* name);

} // namespace implementation
} // namespace V2_1
} // namespace composer
} // namespace graphics
} // namespace hardware
} // namespace android

#endif  // ANDROID_HARDWARE_GRAPHICS_COMPOSER_V2_1_HWC_H
