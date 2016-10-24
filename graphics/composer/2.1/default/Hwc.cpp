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

#define LOG_TAG "HwcPassthrough"

#include <type_traits>

#include <log/log.h>

#include "Hwc.h"
#include "HwcClient.h"

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_1 {
namespace implementation {

HwcHal::HwcHal(const hw_module_t* module)
    : mDevice(nullptr), mDispatch()
{
    int status = hwc2_open(module, &mDevice);
    if (status) {
        LOG_ALWAYS_FATAL("failed to open hwcomposer2 device: %s",
                strerror(-status));
    }

    initCapabilities();
    initDispatch();
}

HwcHal::~HwcHal()
{
    hwc2_close(mDevice);
}

void HwcHal::initCapabilities()
{
    uint32_t count = 0;
    mDevice->getCapabilities(mDevice, &count, nullptr);

    std::vector<Capability> caps(count);
    mDevice->getCapabilities(mDevice, &count, reinterpret_cast<
              std::underlying_type<Capability>::type*>(caps.data()));
    caps.resize(count);

    mCapabilities.insert(caps.cbegin(), caps.cend());
}

template<typename T>
void HwcHal::initDispatch(T& func, hwc2_function_descriptor_t desc)
{
    auto pfn = mDevice->getFunction(mDevice, desc);
    if (!pfn) {
        LOG_ALWAYS_FATAL("failed to get hwcomposer2 function %d", desc);
    }

    func = reinterpret_cast<T>(pfn);
}

void HwcHal::initDispatch()
{
    initDispatch(mDispatch.acceptDisplayChanges,
            HWC2_FUNCTION_ACCEPT_DISPLAY_CHANGES);
    initDispatch(mDispatch.createLayer, HWC2_FUNCTION_CREATE_LAYER);
    initDispatch(mDispatch.createVirtualDisplay,
            HWC2_FUNCTION_CREATE_VIRTUAL_DISPLAY);
    initDispatch(mDispatch.destroyLayer, HWC2_FUNCTION_DESTROY_LAYER);
    initDispatch(mDispatch.destroyVirtualDisplay,
            HWC2_FUNCTION_DESTROY_VIRTUAL_DISPLAY);
    initDispatch(mDispatch.dump, HWC2_FUNCTION_DUMP);
    initDispatch(mDispatch.getActiveConfig, HWC2_FUNCTION_GET_ACTIVE_CONFIG);
    initDispatch(mDispatch.getChangedCompositionTypes,
            HWC2_FUNCTION_GET_CHANGED_COMPOSITION_TYPES);
    initDispatch(mDispatch.getClientTargetSupport,
            HWC2_FUNCTION_GET_CLIENT_TARGET_SUPPORT);
    initDispatch(mDispatch.getColorModes, HWC2_FUNCTION_GET_COLOR_MODES);
    initDispatch(mDispatch.getDisplayAttribute,
            HWC2_FUNCTION_GET_DISPLAY_ATTRIBUTE);
    initDispatch(mDispatch.getDisplayConfigs,
            HWC2_FUNCTION_GET_DISPLAY_CONFIGS);
    initDispatch(mDispatch.getDisplayName, HWC2_FUNCTION_GET_DISPLAY_NAME);
    initDispatch(mDispatch.getDisplayRequests,
            HWC2_FUNCTION_GET_DISPLAY_REQUESTS);
    initDispatch(mDispatch.getDisplayType, HWC2_FUNCTION_GET_DISPLAY_TYPE);
    initDispatch(mDispatch.getDozeSupport, HWC2_FUNCTION_GET_DOZE_SUPPORT);
    initDispatch(mDispatch.getHdrCapabilities,
            HWC2_FUNCTION_GET_HDR_CAPABILITIES);
    initDispatch(mDispatch.getMaxVirtualDisplayCount,
            HWC2_FUNCTION_GET_MAX_VIRTUAL_DISPLAY_COUNT);
    initDispatch(mDispatch.getReleaseFences,
            HWC2_FUNCTION_GET_RELEASE_FENCES);
    initDispatch(mDispatch.presentDisplay, HWC2_FUNCTION_PRESENT_DISPLAY);
    initDispatch(mDispatch.registerCallback, HWC2_FUNCTION_REGISTER_CALLBACK);
    initDispatch(mDispatch.setActiveConfig, HWC2_FUNCTION_SET_ACTIVE_CONFIG);
    initDispatch(mDispatch.setClientTarget, HWC2_FUNCTION_SET_CLIENT_TARGET);
    initDispatch(mDispatch.setColorMode, HWC2_FUNCTION_SET_COLOR_MODE);
    initDispatch(mDispatch.setColorTransform,
            HWC2_FUNCTION_SET_COLOR_TRANSFORM);
    initDispatch(mDispatch.setCursorPosition,
            HWC2_FUNCTION_SET_CURSOR_POSITION);
    initDispatch(mDispatch.setLayerBlendMode,
            HWC2_FUNCTION_SET_LAYER_BLEND_MODE);
    initDispatch(mDispatch.setLayerBuffer, HWC2_FUNCTION_SET_LAYER_BUFFER);
    initDispatch(mDispatch.setLayerColor, HWC2_FUNCTION_SET_LAYER_COLOR);
    initDispatch(mDispatch.setLayerCompositionType,
            HWC2_FUNCTION_SET_LAYER_COMPOSITION_TYPE);
    initDispatch(mDispatch.setLayerDataspace,
            HWC2_FUNCTION_SET_LAYER_DATASPACE);
    initDispatch(mDispatch.setLayerDisplayFrame,
            HWC2_FUNCTION_SET_LAYER_DISPLAY_FRAME);
    initDispatch(mDispatch.setLayerPlaneAlpha,
            HWC2_FUNCTION_SET_LAYER_PLANE_ALPHA);

    if (hasCapability(Capability::SIDEBAND_STREAM)) {
        initDispatch(mDispatch.setLayerSidebandStream,
                HWC2_FUNCTION_SET_LAYER_SIDEBAND_STREAM);
    }

    initDispatch(mDispatch.setLayerSourceCrop,
            HWC2_FUNCTION_SET_LAYER_SOURCE_CROP);
    initDispatch(mDispatch.setLayerSurfaceDamage,
            HWC2_FUNCTION_SET_LAYER_SURFACE_DAMAGE);
    initDispatch(mDispatch.setLayerTransform,
            HWC2_FUNCTION_SET_LAYER_TRANSFORM);
    initDispatch(mDispatch.setLayerVisibleRegion,
            HWC2_FUNCTION_SET_LAYER_VISIBLE_REGION);
    initDispatch(mDispatch.setLayerZOrder, HWC2_FUNCTION_SET_LAYER_Z_ORDER);
    initDispatch(mDispatch.setOutputBuffer, HWC2_FUNCTION_SET_OUTPUT_BUFFER);
    initDispatch(mDispatch.setPowerMode, HWC2_FUNCTION_SET_POWER_MODE);
    initDispatch(mDispatch.setVsyncEnabled, HWC2_FUNCTION_SET_VSYNC_ENABLED);
    initDispatch(mDispatch.validateDisplay, HWC2_FUNCTION_VALIDATE_DISPLAY);
}

bool HwcHal::hasCapability(Capability capability) const
{
    return (mCapabilities.count(capability) > 0);
}

Return<void> HwcHal::getCapabilities(getCapabilities_cb hidl_cb)
{
    std::vector<Capability> caps(
            mCapabilities.cbegin(), mCapabilities.cend());

    hidl_vec<Capability> caps_reply;
    caps_reply.setToExternal(caps.data(), caps.size());
    hidl_cb(caps_reply);

    return Void();
}

Return<void> HwcHal::dumpDebugInfo(dumpDebugInfo_cb hidl_cb)
{
    uint32_t len;
    mDispatch.dump(mDevice, &len, nullptr);

    std::vector<char> buf(len + 1);
    mDispatch.dump(mDevice, &len, buf.data());
    buf.resize(len + 1);
    buf[len] = '\0';

    hidl_string buf_reply;
    buf_reply.setToExternal(buf.data(), len);
    hidl_cb(buf_reply);

    return Void();
}

Return<void> HwcHal::createClient(createClient_cb hidl_cb)
{
    Error err = Error::NONE;
    sp<HwcClient> client;

    {
        std::lock_guard<std::mutex> lock(mClientMutex);

        // only one client is allowed
        if (mClient == nullptr) {
            client = new HwcClient(*this);
            mClient = client;
        } else {
            err = Error::NO_RESOURCES;
        }
    }

    hidl_cb(err, client);

    return Void();
}

sp<HwcClient> HwcHal::getClient()
{
    std::lock_guard<std::mutex> lock(mClientMutex);
    return (mClient != nullptr) ? mClient.promote() : nullptr;
}

void HwcHal::removeClient()
{
    std::lock_guard<std::mutex> lock(mClientMutex);
    mClient = nullptr;
}

void HwcHal::hotplugHook(hwc2_callback_data_t callbackData,
        hwc2_display_t display, int32_t connected)
{
    auto hal = reinterpret_cast<HwcHal*>(callbackData);
    auto client = hal->getClient();
    if (client != nullptr) {
        client->onHotplug(display,
                static_cast<IComposerCallback::Connection>(connected));
    }
}

void HwcHal::refreshHook(hwc2_callback_data_t callbackData,
        hwc2_display_t display)
{
    auto hal = reinterpret_cast<HwcHal*>(callbackData);
    auto client = hal->getClient();
    if (client != nullptr) {
        client->onRefresh(display);
    }
}

void HwcHal::vsyncHook(hwc2_callback_data_t callbackData,
        hwc2_display_t display, int64_t timestamp)
{
    auto hal = reinterpret_cast<HwcHal*>(callbackData);
    auto client = hal->getClient();
    if (client != nullptr) {
        client->onVsync(display, timestamp);
    }
}

void HwcHal::enableCallback(bool enable)
{
    if (enable) {
        mDispatch.registerCallback(mDevice, HWC2_CALLBACK_HOTPLUG, this,
                reinterpret_cast<hwc2_function_pointer_t>(hotplugHook));
        mDispatch.registerCallback(mDevice, HWC2_CALLBACK_REFRESH, this,
                reinterpret_cast<hwc2_function_pointer_t>(refreshHook));
        mDispatch.registerCallback(mDevice, HWC2_CALLBACK_VSYNC, this,
                reinterpret_cast<hwc2_function_pointer_t>(vsyncHook));
    } else {
        mDispatch.registerCallback(mDevice, HWC2_CALLBACK_HOTPLUG, this,
                nullptr);
        mDispatch.registerCallback(mDevice, HWC2_CALLBACK_REFRESH, this,
                nullptr);
        mDispatch.registerCallback(mDevice, HWC2_CALLBACK_VSYNC, this,
                nullptr);
    }
}

uint32_t HwcHal::getMaxVirtualDisplayCount()
{
    return mDispatch.getMaxVirtualDisplayCount(mDevice);
}

Error HwcHal::createVirtualDisplay(uint32_t width, uint32_t height,
    PixelFormat& format, Display& display)
{
    int32_t hwc_format = static_cast<int32_t>(format);
    int32_t err = mDispatch.createVirtualDisplay(mDevice, width, height,
            &hwc_format, &display);
    format = static_cast<PixelFormat>(hwc_format);

    return static_cast<Error>(err);
}

Error HwcHal::destroyVirtualDisplay(Display display)
{
    int32_t err = mDispatch.destroyVirtualDisplay(mDevice, display);
    return static_cast<Error>(err);
}

Error HwcHal::createLayer(Display display, Layer& layer)
{
    int32_t err = mDispatch.createLayer(mDevice, display, &layer);
    return static_cast<Error>(err);
}

Error HwcHal::destroyLayer(Display display, Layer layer)
{
    int32_t err = mDispatch.destroyLayer(mDevice, display, layer);
    return static_cast<Error>(err);
}

Error HwcHal::getActiveConfig(Display display, Config& config)
{
    int32_t err = mDispatch.getActiveConfig(mDevice, display, &config);
    return static_cast<Error>(err);
}

Error HwcHal::getClientTargetSupport(Display display,
        uint32_t width, uint32_t height,
        PixelFormat format, Dataspace dataspace)
{
    int32_t err = mDispatch.getClientTargetSupport(mDevice, display,
            width, height, static_cast<int32_t>(format),
            static_cast<int32_t>(dataspace));
    return static_cast<Error>(err);
}

Error HwcHal::getColorModes(Display display, hidl_vec<ColorMode>& modes)
{
    uint32_t count = 0;
    int32_t err = mDispatch.getColorModes(mDevice, display, &count, nullptr);
    if (err != HWC2_ERROR_NONE) {
        return static_cast<Error>(err);
    }

    modes.resize(count);
    err = mDispatch.getColorModes(mDevice, display, &count,
            reinterpret_cast<std::underlying_type<ColorMode>::type*>(
                modes.data()));
    if (err != HWC2_ERROR_NONE) {
        modes = hidl_vec<ColorMode>();
        return static_cast<Error>(err);
    }

    return Error::NONE;
}

Error HwcHal::getDisplayAttribute(Display display, Config config,
        IComposerClient::Attribute attribute, int32_t& value)
{
    int32_t err = mDispatch.getDisplayAttribute(mDevice, display, config,
            static_cast<int32_t>(attribute), &value);
    return static_cast<Error>(err);
}

Error HwcHal::getDisplayConfigs(Display display, hidl_vec<Config>& configs)
{
    uint32_t count = 0;
    int32_t err = mDispatch.getDisplayConfigs(mDevice, display,
            &count, nullptr);
    if (err != HWC2_ERROR_NONE) {
        return static_cast<Error>(err);
    }

    configs.resize(count);
    err = mDispatch.getDisplayConfigs(mDevice, display,
            &count, configs.data());
    if (err != HWC2_ERROR_NONE) {
        configs = hidl_vec<Config>();
        return static_cast<Error>(err);
    }

    return Error::NONE;
}

Error HwcHal::getDisplayName(Display display, hidl_string& name)
{
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

    name = buf.data();

    return Error::NONE;
}

Error HwcHal::getDisplayType(Display display, IComposerClient::DisplayType& type)
{
    int32_t hwc_type = HWC2_DISPLAY_TYPE_INVALID;
    int32_t err = mDispatch.getDisplayType(mDevice, display, &hwc_type);
    type = static_cast<IComposerClient::DisplayType>(hwc_type);

    return static_cast<Error>(err);
}

Error HwcHal::getDozeSupport(Display display, bool& support)
{
    int32_t hwc_support = 0;
    int32_t err = mDispatch.getDozeSupport(mDevice, display, &hwc_support);
    support = hwc_support;

    return static_cast<Error>(err);
}

Error HwcHal::getHdrCapabilities(Display display, hidl_vec<Hdr>& types,
        float& maxLuminance, float& maxAverageLuminance, float& minLuminance)
{
    uint32_t count = 0;
    int32_t err = mDispatch.getHdrCapabilities(mDevice, display, &count,
            nullptr, &maxLuminance, &maxAverageLuminance, &minLuminance);
    if (err != HWC2_ERROR_NONE) {
        return static_cast<Error>(err);
    }

    types.resize(count);
    err = mDispatch.getHdrCapabilities(mDevice, display, &count,
            reinterpret_cast<std::underlying_type<Hdr>::type*>(types.data()),
            &maxLuminance, &maxAverageLuminance, &minLuminance);
    if (err != HWC2_ERROR_NONE) {
        return static_cast<Error>(err);
    }

    return Error::NONE;
}

Error HwcHal::setActiveConfig(Display display, Config config)
{
    int32_t err = mDispatch.setActiveConfig(mDevice, display, config);
    return static_cast<Error>(err);
}

Error HwcHal::setColorMode(Display display, ColorMode mode)
{
    int32_t err = mDispatch.setColorMode(mDevice, display,
            static_cast<int32_t>(mode));
    return static_cast<Error>(err);
}

Error HwcHal::setPowerMode(Display display, IComposerClient::PowerMode mode)
{
    int32_t err = mDispatch.setPowerMode(mDevice, display,
            static_cast<int32_t>(mode));
    return static_cast<Error>(err);
}

Error HwcHal::setVsyncEnabled(Display display, IComposerClient::Vsync enabled)
{
    int32_t err = mDispatch.setVsyncEnabled(mDevice, display,
            static_cast<int32_t>(enabled));
    return static_cast<Error>(err);
}

Error HwcHal::setColorTransform(Display display, const float* matrix,
        int32_t hint)
{
    int32_t err = mDispatch.setColorTransform(mDevice, display, matrix, hint);
    return static_cast<Error>(err);
}

Error HwcHal::setClientTarget(Display display, buffer_handle_t target,
        int32_t acquireFence, int32_t dataspace,
        const std::vector<hwc_rect_t>& damage)
{
    hwc_region region = { damage.size(), damage.data() };
    int32_t err = mDispatch.setClientTarget(mDevice, display, target,
            acquireFence, dataspace, region);
    return static_cast<Error>(err);
}

Error HwcHal::setOutputBuffer(Display display, buffer_handle_t buffer,
        int32_t releaseFence)
{
    int32_t err = mDispatch.setOutputBuffer(mDevice, display, buffer,
            releaseFence);
    // unlike in setClientTarget, releaseFence is owned by us
    if (err == HWC2_ERROR_NONE && releaseFence >= 0) {
        close(releaseFence);
    }

    return static_cast<Error>(err);
}

Error HwcHal::validateDisplay(Display display,
        std::vector<Layer>& changedLayers,
        std::vector<IComposerClient::Composition>& compositionTypes,
        uint32_t& displayRequestMask,
        std::vector<Layer>& requestedLayers,
        std::vector<uint32_t>& requestMasks)
{
    uint32_t types_count = 0;
    uint32_t reqs_count = 0;
    int32_t err = mDispatch.validateDisplay(mDevice, display,
            &types_count, &reqs_count);
    if (err != HWC2_ERROR_NONE && err != HWC2_ERROR_HAS_CHANGES) {
        return static_cast<Error>(err);
    }

    err = mDispatch.getChangedCompositionTypes(mDevice, display,
            &types_count, nullptr, nullptr);
    if (err != HWC2_ERROR_NONE) {
        return static_cast<Error>(err);
    }

    changedLayers.resize(types_count);
    compositionTypes.resize(types_count);
    err = mDispatch.getChangedCompositionTypes(mDevice, display,
            &types_count, changedLayers.data(),
            reinterpret_cast<
            std::underlying_type<IComposerClient::Composition>::type*>(
                compositionTypes.data()));
    if (err != HWC2_ERROR_NONE) {
        changedLayers.clear();
        compositionTypes.clear();
        return static_cast<Error>(err);
    }

    int32_t display_reqs = 0;
    err = mDispatch.getDisplayRequests(mDevice, display, &display_reqs,
            &reqs_count, nullptr, nullptr);
    if (err != HWC2_ERROR_NONE) {
        changedLayers.clear();
        compositionTypes.clear();
        return static_cast<Error>(err);
    }

    requestedLayers.resize(reqs_count);
    requestMasks.resize(reqs_count);
    err = mDispatch.getDisplayRequests(mDevice, display, &display_reqs,
            &reqs_count, requestedLayers.data(),
            reinterpret_cast<int32_t*>(requestMasks.data()));
    if (err != HWC2_ERROR_NONE) {
        changedLayers.clear();
        compositionTypes.clear();

        requestedLayers.clear();
        requestMasks.clear();
        return static_cast<Error>(err);
    }

    displayRequestMask = display_reqs;

    return static_cast<Error>(err);
}

Error HwcHal::acceptDisplayChanges(Display display)
{
    int32_t err = mDispatch.acceptDisplayChanges(mDevice, display);
    return static_cast<Error>(err);
}

Error HwcHal::presentDisplay(Display display, int32_t& presentFence,
        std::vector<Layer>& layers, std::vector<int32_t>& releaseFences)
{
    presentFence = -1;
    int32_t err = mDispatch.presentDisplay(mDevice, display, &presentFence);
    if (err != HWC2_ERROR_NONE) {
        return static_cast<Error>(err);
    }

    uint32_t count = 0;
    err = mDispatch.getReleaseFences(mDevice, display, &count,
            nullptr, nullptr);
    if (err != HWC2_ERROR_NONE) {
        ALOGW("failed to get release fences");
        return Error::NONE;
    }

    layers.resize(count);
    releaseFences.resize(count);
    err = mDispatch.getReleaseFences(mDevice, display, &count,
            layers.data(), releaseFences.data());
    if (err != HWC2_ERROR_NONE) {
        ALOGW("failed to get release fences");
        layers.clear();
        releaseFences.clear();
        return Error::NONE;
    }

    return static_cast<Error>(err);
}

Error HwcHal::setLayerCursorPosition(Display display, Layer layer,
        int32_t x, int32_t y)
{
    int32_t err = mDispatch.setCursorPosition(mDevice, display, layer, x, y);
    return static_cast<Error>(err);
}

Error HwcHal::setLayerBuffer(Display display, Layer layer,
        buffer_handle_t buffer, int32_t acquireFence)
{
    int32_t err = mDispatch.setLayerBuffer(mDevice, display, layer,
            buffer, acquireFence);
    return static_cast<Error>(err);
}

Error HwcHal::setLayerSurfaceDamage(Display display, Layer layer,
        const std::vector<hwc_rect_t>& damage)
{
    hwc_region region = { damage.size(), damage.data() };
    int32_t err = mDispatch.setLayerSurfaceDamage(mDevice, display, layer,
            region);
    return static_cast<Error>(err);
}

Error HwcHal::setLayerBlendMode(Display display, Layer layer, int32_t mode)
{
    int32_t err = mDispatch.setLayerBlendMode(mDevice, display, layer, mode);
    return static_cast<Error>(err);
}

Error HwcHal::setLayerColor(Display display, Layer layer,
        IComposerClient::Color color)
{
    hwc_color_t hwc_color{color.r, color.g, color.b, color.a};
    int32_t err = mDispatch.setLayerColor(mDevice, display, layer, hwc_color);
    return static_cast<Error>(err);
}

Error HwcHal::setLayerCompositionType(Display display, Layer layer,
        int32_t type)
{
    int32_t err = mDispatch.setLayerCompositionType(mDevice, display, layer,
            type);
    return static_cast<Error>(err);
}

Error HwcHal::setLayerDataspace(Display display, Layer layer,
        int32_t dataspace)
{
    int32_t err = mDispatch.setLayerDataspace(mDevice, display, layer,
            dataspace);
    return static_cast<Error>(err);
}

Error HwcHal::setLayerDisplayFrame(Display display, Layer layer,
        const hwc_rect_t& frame)
{
    int32_t err = mDispatch.setLayerDisplayFrame(mDevice, display, layer,
            frame);
    return static_cast<Error>(err);
}

Error HwcHal::setLayerPlaneAlpha(Display display, Layer layer, float alpha)
{
    int32_t err = mDispatch.setLayerPlaneAlpha(mDevice, display, layer,
            alpha);
    return static_cast<Error>(err);
}

Error HwcHal::setLayerSidebandStream(Display display, Layer layer,
        buffer_handle_t stream)
{
    int32_t err = mDispatch.setLayerSidebandStream(mDevice, display, layer,
            stream);
    return static_cast<Error>(err);
}

Error HwcHal::setLayerSourceCrop(Display display, Layer layer,
        const hwc_frect_t& crop)
{
    int32_t err = mDispatch.setLayerSourceCrop(mDevice, display, layer, crop);
    return static_cast<Error>(err);
}

Error HwcHal::setLayerTransform(Display display, Layer layer,
        int32_t transform)
{
    int32_t err = mDispatch.setLayerTransform(mDevice, display, layer,
            transform);
    return static_cast<Error>(err);
}

Error HwcHal::setLayerVisibleRegion(Display display, Layer layer,
        const std::vector<hwc_rect_t>& visible)
{
    hwc_region_t region = { visible.size(), visible.data() };
    int32_t err = mDispatch.setLayerVisibleRegion(mDevice, display, layer,
            region);
    return static_cast<Error>(err);
}

Error HwcHal::setLayerZOrder(Display display, Layer layer, uint32_t z)
{
    int32_t err = mDispatch.setLayerZOrder(mDevice, display, layer, z);
    return static_cast<Error>(err);
}

IComposer* HIDL_FETCH_IComposer(const char*)
{
    const hw_module_t* module;
    int err = hw_get_module(HWC_HARDWARE_MODULE_ID, &module);
    if (err) {
        ALOGE("failed to get hwcomposer module");
        return nullptr;
    }

    return new HwcHal(module);
}

} // namespace implementation
} // namespace V2_1
} // namespace composer
} // namespace graphics
} // namespace hardware
} // namespace android
