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

#include <mutex>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <hardware/gralloc.h>
#include <hardware/gralloc1.h>
#include <hardware/hwcomposer2.h>
#include <log/log.h>

#include "Hwc.h"

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

namespace {

class HandleImporter {
public:
    HandleImporter() : mInitialized(false) {}

    bool initialize()
    {
        // allow only one client
        if (mInitialized) {
            return false;
        }

        if (!openGralloc()) {
            return false;
        }

        mInitialized = true;
        return true;
    }

    void cleanup()
    {
        if (!mInitialized) {
            return;
        }

        closeGralloc();
        mInitialized = false;
    }

    // In IComposer, any buffer_handle_t is owned by the caller and we need to
    // make a clone for hwcomposer2.  We also need to translate empty handle
    // to nullptr.  This function does that, in-place.
    bool importBuffer(buffer_handle_t& handle)
    {
        if (!handle->numFds && !handle->numInts) {
            handle = nullptr;
            return true;
        }

        buffer_handle_t clone = cloneBuffer(handle);
        if (!clone) {
            return false;
        }

        handle = clone;
        return true;
    }

    void freeBuffer(buffer_handle_t handle)
    {
        if (!handle) {
            return;
        }

        releaseBuffer(handle);
    }

    bool importFence(const native_handle_t* handle, int& fd)
    {
        if (handle->numFds == 0) {
            fd = -1;
        } else if (handle->numFds == 1) {
            fd = dup(handle->data[0]);
            if (fd < 0) {
                ALOGE("failed to dup fence fd %d", handle->data[0]);
                return false;
            }
        } else {
            ALOGE("invalid fence handle with %d file descriptors",
                    handle->numFds);
            return false;
        }

        return true;
    }

    void closeFence(int fd)
    {
        if (fd >= 0) {
            close(fd);
        }
    }

private:
    bool mInitialized;

    // Some existing gralloc drivers do not support retaining more than once,
    // when we are in passthrough mode.
#ifdef BINDERIZED
    bool openGralloc()
    {
        const hw_module_t* module;
        int err = hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &module);
        if (err) {
            ALOGE("failed to get gralloc module");
            return false;
        }

        uint8_t major = (module->module_api_version >> 8) & 0xff;
        if (major > 1) {
            ALOGE("unknown gralloc module major version %d", major);
            return false;
        }

        if (major == 1) {
            err = gralloc1_open(module, &mDevice);
            if (err) {
                ALOGE("failed to open gralloc1 device");
                return false;
            }

            mRetain = reinterpret_cast<GRALLOC1_PFN_RETAIN>(
                    mDevice->getFunction(mDevice, GRALLOC1_FUNCTION_RETAIN));
            mRelease = reinterpret_cast<GRALLOC1_PFN_RELEASE>(
                    mDevice->getFunction(mDevice, GRALLOC1_FUNCTION_RELEASE));
            if (!mRetain || !mRelease) {
                ALOGE("invalid gralloc1 device");
                gralloc1_close(mDevice);
                return false;
            }
        } else {
            mModule = reinterpret_cast<const gralloc_module_t*>(module);
        }

        return true;
    }

    void closeGralloc()
    {
        if (mDevice) {
            gralloc1_close(mDevice);
        }
    }

    buffer_handle_t cloneBuffer(buffer_handle_t handle)
    {
        native_handle_t* clone = native_handle_clone(handle);
        if (!clone) {
            ALOGE("failed to clone buffer %p", handle);
            return nullptr;
        }

        bool err;
        if (mDevice) {
            err = (mRetain(mDevice, clone) != GRALLOC1_ERROR_NONE);
        } else {
            err = (mModule->registerBuffer(mModule, clone) != 0);
        }

        if (err) {
            ALOGE("failed to retain/register buffer %p", clone);
            native_handle_close(clone);
            native_handle_delete(clone);
            return nullptr;
        }

        return clone;
    }

    void releaseBuffer(buffer_handle_t handle)
    {
        if (mDevice) {
            mRelease(mDevice, handle);
        } else {
            mModule->unregisterBuffer(mModule, handle);
            native_handle_close(handle);
            native_handle_delete(const_cast<native_handle_t*>(handle));
        }
    }

    // gralloc1
    gralloc1_device_t* mDevice;
    GRALLOC1_PFN_RETAIN mRetain;
    GRALLOC1_PFN_RELEASE mRelease;

    // gralloc0
    const gralloc_module_t* mModule;
#else
    bool openGralloc() { return true; }
    void closeGralloc() {}
    buffer_handle_t cloneBuffer(buffer_handle_t handle) { return handle; }
    void releaseBuffer(buffer_handle_t) {}
#endif
};

HandleImporter sHandleImporter;

class BufferClone {
public:
    BufferClone() : mHandle(nullptr) {}

    BufferClone(BufferClone&& other)
    {
        mHandle = other.mHandle;
        other.mHandle = nullptr;
    }

    BufferClone(const BufferClone& other) = delete;
    BufferClone& operator=(const BufferClone& other) = delete;

    BufferClone& operator=(buffer_handle_t handle)
    {
        clear();
        mHandle = handle;
        return *this;
    }

    ~BufferClone()
    {
        clear();
    }

private:
    void clear()
    {
        if (mHandle) {
            sHandleImporter.freeBuffer(mHandle);
        }
    }

    buffer_handle_t mHandle;
};

} // anonymous namespace

class HwcHal : public IComposer {
public:
    HwcHal(const hw_module_t* module);
    virtual ~HwcHal();

    // IComposer interface
    Return<void> getCapabilities(getCapabilities_cb hidl_cb) override;
    Return<void> dumpDebugInfo(dumpDebugInfo_cb hidl_cb) override;
    Return<void> registerCallback(const sp<IComposerCallback>& callback) override;
    Return<uint32_t> getMaxVirtualDisplayCount() override;
    Return<void> createVirtualDisplay(uint32_t width, uint32_t height,
            PixelFormat formatHint, createVirtualDisplay_cb hidl_cb) override;
    Return<Error> destroyVirtualDisplay(Display display) override;
    Return<Error> acceptDisplayChanges(Display display) override;
    Return<void> createLayer(Display display,
            createLayer_cb hidl_cb) override;
    Return<Error> destroyLayer(Display display, Layer layer) override;
    Return<void> getActiveConfig(Display display,
            getActiveConfig_cb hidl_cb) override;
    Return<void> getChangedCompositionTypes(Display display,
            getChangedCompositionTypes_cb hidl_cb) override;
    Return<Error> getClientTargetSupport(Display display,
            uint32_t width, uint32_t height,
            PixelFormat format, Dataspace dataspace) override;
    Return<void> getColorModes(Display display,
            getColorModes_cb hidl_cb) override;
    Return<void> getDisplayAttribute(Display display,
            Config config, Attribute attribute,
            getDisplayAttribute_cb hidl_cb) override;
    Return<void> getDisplayConfigs(Display display,
            getDisplayConfigs_cb hidl_cb) override;
    Return<void> getDisplayName(Display display,
            getDisplayName_cb hidl_cb) override;
    Return<void> getDisplayRequests(Display display,
            getDisplayRequests_cb hidl_cb) override;
    Return<void> getDisplayType(Display display,
            getDisplayType_cb hidl_cb) override;
    Return<void> getDozeSupport(Display display,
            getDozeSupport_cb hidl_cb) override;
    Return<void> getHdrCapabilities(Display display,
            getHdrCapabilities_cb hidl_cb) override;
    Return<void> getReleaseFences(Display display,
            getReleaseFences_cb hidl_cb) override;
    Return<void> presentDisplay(Display display,
            presentDisplay_cb hidl_cb) override;
    Return<Error> setActiveConfig(Display display, Config config) override;
    Return<Error> setClientTarget(Display display,
            const native_handle_t* target,
            const native_handle_t* acquireFence,
            Dataspace dataspace, const hidl_vec<Rect>& damage) override;
    Return<Error> setColorMode(Display display, ColorMode mode) override;
    Return<Error> setColorTransform(Display display,
            const hidl_vec<float>& matrix, ColorTransform hint) override;
    Return<Error> setOutputBuffer(Display display,
            const native_handle_t* buffer,
            const native_handle_t* releaseFence) override;
    Return<Error> setPowerMode(Display display, PowerMode mode) override;
    Return<Error> setVsyncEnabled(Display display, Vsync enabled) override;
    Return<void> validateDisplay(Display display,
            validateDisplay_cb hidl_cb) override;
    Return<Error> setCursorPosition(Display display,
            Layer layer, int32_t x, int32_t y) override;
    Return<Error> setLayerBuffer(Display display,
            Layer layer, const native_handle_t* buffer,
            const native_handle_t* acquireFence) override;
    Return<Error> setLayerSurfaceDamage(Display display,
            Layer layer, const hidl_vec<Rect>& damage) override;
    Return<Error> setLayerBlendMode(Display display,
            Layer layer, BlendMode mode) override;
    Return<Error> setLayerColor(Display display,
            Layer layer, const Color& color) override;
    Return<Error> setLayerCompositionType(Display display,
            Layer layer, Composition type) override;
    Return<Error> setLayerDataspace(Display display,
            Layer layer, Dataspace dataspace) override;
    Return<Error> setLayerDisplayFrame(Display display,
            Layer layer, const Rect& frame) override;
    Return<Error> setLayerPlaneAlpha(Display display,
            Layer layer, float alpha) override;
    Return<Error> setLayerSidebandStream(Display display,
            Layer layer, const native_handle_t* stream) override;
    Return<Error> setLayerSourceCrop(Display display,
            Layer layer, const FRect& crop) override;
    Return<Error> setLayerTransform(Display display,
            Layer layer, Transform transform) override;
    Return<Error> setLayerVisibleRegion(Display display,
            Layer layer, const hidl_vec<Rect>& visible) override;
    Return<Error> setLayerZOrder(Display display,
            Layer layer, uint32_t z) override;

private:
    void initCapabilities();

    template<typename T>
    void initDispatch(T& func, hwc2_function_descriptor_t desc);
    void initDispatch();

    bool hasCapability(Capability capability) const;

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

    // cloned buffers for a display
    struct DisplayBuffers {
        BufferClone ClientTarget;
        BufferClone OutputBuffer;

        std::unordered_map<Layer, BufferClone> LayerBuffers;
        std::unordered_map<Layer, BufferClone> LayerSidebandStreams;
    };

    std::mutex mCallbackMutex;
    sp<IComposerCallback> mCallback;

    std::mutex mDisplayMutex;
    std::unordered_map<Display, DisplayBuffers> mDisplays;
};

HwcHal::HwcHal(const hw_module_t* module)
    : mDevice(nullptr), mDispatch()
{
    if (!sHandleImporter.initialize()) {
        LOG_ALWAYS_FATAL("failed to initialize handle importer");
    }

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
    mDisplays.clear();
    sHandleImporter.cleanup();
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

void HwcHal::hotplugHook(hwc2_callback_data_t callbackData,
        hwc2_display_t display, int32_t connected)
{
    auto hal = reinterpret_cast<HwcHal*>(callbackData);

    {
        std::lock_guard<std::mutex> lock(hal->mDisplayMutex);

        if (connected == HWC2_CONNECTION_CONNECTED) {
            hal->mDisplays.emplace(display, DisplayBuffers());
        } else if (connected == HWC2_CONNECTION_DISCONNECTED) {
            hal->mDisplays.erase(display);
        }
    }

    hal->mCallback->onHotplug(display,
            static_cast<IComposerCallback::Connection>(connected));
}

void HwcHal::refreshHook(hwc2_callback_data_t callbackData,
        hwc2_display_t display)
{
    auto hal = reinterpret_cast<HwcHal*>(callbackData);
    hal->mCallback->onRefresh(display);
}

void HwcHal::vsyncHook(hwc2_callback_data_t callbackData,
        hwc2_display_t display, int64_t timestamp)
{
    auto hal = reinterpret_cast<HwcHal*>(callbackData);
    hal->mCallback->onVsync(display, timestamp);
}

Return<void> HwcHal::registerCallback(const sp<IComposerCallback>& callback)
{
    std::lock_guard<std::mutex> lock(mCallbackMutex);

    mCallback = callback;

    mDispatch.registerCallback(mDevice, HWC2_CALLBACK_HOTPLUG, this,
            reinterpret_cast<hwc2_function_pointer_t>(hotplugHook));
    mDispatch.registerCallback(mDevice, HWC2_CALLBACK_REFRESH, this,
            reinterpret_cast<hwc2_function_pointer_t>(refreshHook));
    mDispatch.registerCallback(mDevice, HWC2_CALLBACK_VSYNC, this,
            reinterpret_cast<hwc2_function_pointer_t>(vsyncHook));

    return Void();
}

Return<uint32_t> HwcHal::getMaxVirtualDisplayCount()
{
    return mDispatch.getMaxVirtualDisplayCount(mDevice);
}

Return<void> HwcHal::createVirtualDisplay(uint32_t width, uint32_t height,
        PixelFormat formatHint, createVirtualDisplay_cb hidl_cb)
{
    int32_t format = static_cast<int32_t>(formatHint);
    hwc2_display_t display;
    auto error = mDispatch.createVirtualDisplay(mDevice, width, height,
            &format, &display);
    if (error == HWC2_ERROR_NONE) {
        std::lock_guard<std::mutex> lock(mDisplayMutex);

        mDisplays.emplace(display, DisplayBuffers());
    }

    hidl_cb(static_cast<Error>(error), display,
            static_cast<PixelFormat>(format));

    return Void();
}

Return<Error> HwcHal::destroyVirtualDisplay(Display display)
{
    auto error = mDispatch.destroyVirtualDisplay(mDevice, display);
    if (error == HWC2_ERROR_NONE) {
        std::lock_guard<std::mutex> lock(mDisplayMutex);

        mDisplays.erase(display);
    }

    return static_cast<Error>(error);
}

Return<Error> HwcHal::acceptDisplayChanges(Display display)
{
    auto error = mDispatch.acceptDisplayChanges(mDevice, display);
    return static_cast<Error>(error);
}

Return<void> HwcHal::createLayer(Display display, createLayer_cb hidl_cb)
{
    hwc2_layer_t layer;
    auto error = mDispatch.createLayer(mDevice, display, &layer);

    hidl_cb(static_cast<Error>(error), layer);

    return Void();
}

Return<Error> HwcHal::destroyLayer(Display display, Layer layer)
{
    auto error = mDispatch.destroyLayer(mDevice, display, layer);
    if (error == HWC2_ERROR_NONE) {
        std::lock_guard<std::mutex> lock(mDisplayMutex);

        auto dpy = mDisplays.find(display);
        dpy->second.LayerBuffers.erase(layer);
        dpy->second.LayerSidebandStreams.erase(layer);
    }

    return static_cast<Error>(error);
}

Return<void> HwcHal::getActiveConfig(Display display,
        getActiveConfig_cb hidl_cb)
{
    hwc2_config_t config;
    auto error = mDispatch.getActiveConfig(mDevice, display, &config);

    hidl_cb(static_cast<Error>(error), config);

    return Void();
}

Return<void> HwcHal::getChangedCompositionTypes(Display display,
        getChangedCompositionTypes_cb hidl_cb)
{
    uint32_t count = 0;
    auto error = mDispatch.getChangedCompositionTypes(mDevice, display,
            &count, nullptr, nullptr);
    if (error != HWC2_ERROR_NONE) {
        count = 0;
    }

    std::vector<hwc2_layer_t> layers(count);
    std::vector<Composition> types(count);
    error = mDispatch.getChangedCompositionTypes(mDevice, display,
            &count, layers.data(),
            reinterpret_cast<std::underlying_type<Composition>::type*>(
                types.data()));
    if (error != HWC2_ERROR_NONE) {
        count = 0;
    }
    layers.resize(count);
    types.resize(count);

    hidl_vec<Layer> layers_reply;
    layers_reply.setToExternal(layers.data(), layers.size());

    hidl_vec<Composition> types_reply;
    types_reply.setToExternal(types.data(), types.size());

    hidl_cb(static_cast<Error>(error), layers_reply, types_reply);

    return Void();
}

Return<Error> HwcHal::getClientTargetSupport(Display display,
        uint32_t width, uint32_t height,
        PixelFormat format, Dataspace dataspace)
{
    auto error = mDispatch.getClientTargetSupport(mDevice, display,
            width, height, static_cast<int32_t>(format),
            static_cast<int32_t>(dataspace));
    return static_cast<Error>(error);
}

Return<void> HwcHal::getColorModes(Display display, getColorModes_cb hidl_cb)
{
    uint32_t count = 0;
    auto error = mDispatch.getColorModes(mDevice, display, &count, nullptr);
    if (error != HWC2_ERROR_NONE) {
        count = 0;
    }

    std::vector<ColorMode> modes(count);
    error = mDispatch.getColorModes(mDevice, display, &count,
            reinterpret_cast<std::underlying_type<ColorMode>::type*>(
                modes.data()));
    if (error != HWC2_ERROR_NONE) {
        count = 0;
    }
    modes.resize(count);

    hidl_vec<ColorMode> modes_reply;
    modes_reply.setToExternal(modes.data(), modes.size());
    hidl_cb(static_cast<Error>(error), modes_reply);

    return Void();
}

Return<void> HwcHal::getDisplayAttribute(Display display,
        Config config, Attribute attribute,
        getDisplayAttribute_cb hidl_cb)
{
    int32_t value;
    auto error = mDispatch.getDisplayAttribute(mDevice, display, config,
            static_cast<int32_t>(attribute), &value);

    hidl_cb(static_cast<Error>(error), value);

    return Void();
}

Return<void> HwcHal::getDisplayConfigs(Display display,
        getDisplayConfigs_cb hidl_cb)
{
    uint32_t count = 0;
    auto error = mDispatch.getDisplayConfigs(mDevice, display,
            &count, nullptr);
    if (error != HWC2_ERROR_NONE) {
        count = 0;
    }

    std::vector<hwc2_config_t> configs(count);
    error = mDispatch.getDisplayConfigs(mDevice, display,
            &count, configs.data());
    if (error != HWC2_ERROR_NONE) {
        count = 0;
    }
    configs.resize(count);

    hidl_vec<Config> configs_reply;
    configs_reply.setToExternal(configs.data(), configs.size());
    hidl_cb(static_cast<Error>(error), configs_reply);

    return Void();
}

Return<void> HwcHal::getDisplayName(Display display,
        getDisplayName_cb hidl_cb)
{
    uint32_t count = 0;
    auto error = mDispatch.getDisplayName(mDevice, display, &count, nullptr);
    if (error != HWC2_ERROR_NONE) {
        count = 0;
    }

    std::vector<char> name(count + 1);
    error = mDispatch.getDisplayName(mDevice, display, &count, name.data());
    if (error != HWC2_ERROR_NONE) {
        count = 0;
    }
    name.resize(count + 1);
    name[count] = '\0';

    hidl_string name_reply;
    name_reply.setToExternal(name.data(), count);
    hidl_cb(static_cast<Error>(error), name_reply);

    return Void();
}

Return<void> HwcHal::getDisplayRequests(Display display,
        getDisplayRequests_cb hidl_cb)
{
    int32_t display_reqs;
    uint32_t count = 0;
    auto error = mDispatch.getDisplayRequests(mDevice, display,
            &display_reqs, &count, nullptr, nullptr);
    if (error != HWC2_ERROR_NONE) {
        count = 0;
    }

    std::vector<hwc2_layer_t> layers(count);
    std::vector<int32_t> layer_reqs(count);
    error = mDispatch.getDisplayRequests(mDevice, display,
            &display_reqs, &count, layers.data(), layer_reqs.data());
    if (error != HWC2_ERROR_NONE) {
        count = 0;
    }
    layers.resize(count);
    layer_reqs.resize(count);

    hidl_vec<Layer> layers_reply;
    layers_reply.setToExternal(layers.data(), layers.size());

    hidl_vec<uint32_t> layer_reqs_reply;
    layer_reqs_reply.setToExternal(
            reinterpret_cast<uint32_t*>(layer_reqs.data()),
            layer_reqs.size());

    hidl_cb(static_cast<Error>(error), display_reqs,
            layers_reply, layer_reqs_reply);

    return Void();
}

Return<void> HwcHal::getDisplayType(Display display,
        getDisplayType_cb hidl_cb)
{
    int32_t type;
    auto error = mDispatch.getDisplayType(mDevice, display, &type);

    hidl_cb(static_cast<Error>(error), static_cast<DisplayType>(type));

    return Void();
}

Return<void> HwcHal::getDozeSupport(Display display,
        getDozeSupport_cb hidl_cb)
{
    int32_t support;
    auto error = mDispatch.getDozeSupport(mDevice, display, &support);

    hidl_cb(static_cast<Error>(error), support);

    return Void();
}

Return<void> HwcHal::getHdrCapabilities(Display display,
        getHdrCapabilities_cb hidl_cb)
{
    float max_lumi, max_avg_lumi, min_lumi;
    uint32_t count = 0;
    auto error = mDispatch.getHdrCapabilities(mDevice, display,
            &count, nullptr, &max_lumi, &max_avg_lumi, &min_lumi);
    if (error != HWC2_ERROR_NONE) {
        count = 0;
    }

    std::vector<Hdr> types(count);
    error = mDispatch.getHdrCapabilities(mDevice, display, &count,
            reinterpret_cast<std::underlying_type<Hdr>::type*>(types.data()),
            &max_lumi, &max_avg_lumi, &min_lumi);
    if (error != HWC2_ERROR_NONE) {
        count = 0;
    }
    types.resize(count);

    hidl_vec<Hdr> types_reply;
    types_reply.setToExternal(types.data(), types.size());
    hidl_cb(static_cast<Error>(error), types_reply,
            max_lumi, max_avg_lumi, min_lumi);

    return Void();
}

Return<void> HwcHal::getReleaseFences(Display display,
        getReleaseFences_cb hidl_cb)
{
    uint32_t count = 0;
    auto error = mDispatch.getReleaseFences(mDevice, display,
            &count, nullptr, nullptr);
    if (error != HWC2_ERROR_NONE) {
        count = 0;
    }

    std::vector<hwc2_layer_t> layers(count);
    std::vector<int32_t> fences(count);
    error = mDispatch.getReleaseFences(mDevice, display,
            &count, layers.data(), fences.data());
    if (error != HWC2_ERROR_NONE) {
        count = 0;
    }
    layers.resize(count);
    fences.resize(count);

    // filter out layers with release fence -1
    std::vector<hwc2_layer_t> filtered_layers;
    std::vector<int> filtered_fences;
    for (size_t i = 0; i < layers.size(); i++) {
        if (fences[i] >= 0) {
            filtered_layers.push_back(layers[i]);
            filtered_fences.push_back(fences[i]);
        }
    }

    hidl_vec<Layer> layers_reply;
    native_handle_t* fences_reply =
        native_handle_create(filtered_fences.size(), 0);
    if (fences_reply) {
        layers_reply.setToExternal(filtered_layers.data(),
                filtered_layers.size());
        memcpy(fences_reply->data, filtered_fences.data(),
                sizeof(int) * filtered_fences.size());

        hidl_cb(static_cast<Error>(error), layers_reply, fences_reply);

        native_handle_close(fences_reply);
        native_handle_delete(fences_reply);
    } else {
        NATIVE_HANDLE_DECLARE_STORAGE(fences_storage, 0, 0);
        fences_reply = native_handle_init(fences_storage, 0, 0);

        hidl_cb(Error::NO_RESOURCES, layers_reply, fences_reply);

        for (auto fence : filtered_fences) {
            close(fence);
        }
    }

    return Void();
}

Return<void> HwcHal::presentDisplay(Display display,
        presentDisplay_cb hidl_cb)
{
    int32_t fence = -1;
    auto error = mDispatch.presentDisplay(mDevice, display, &fence);

    NATIVE_HANDLE_DECLARE_STORAGE(fence_storage, 1, 0);
    native_handle_t* fence_reply;
    if (fence >= 0) {
        fence_reply = native_handle_init(fence_storage, 1, 0);
        fence_reply->data[0] = fence;
    } else {
        fence_reply = native_handle_init(fence_storage, 0, 0);
    }

    hidl_cb(static_cast<Error>(error), fence_reply);

    if (fence >= 0) {
        close(fence);
    }

    return Void();
}

Return<Error> HwcHal::setActiveConfig(Display display, Config config)
{
    auto error = mDispatch.setActiveConfig(mDevice, display, config);
    return static_cast<Error>(error);
}

Return<Error> HwcHal::setClientTarget(Display display,
        const native_handle_t* target,
        const native_handle_t* acquireFence,
        Dataspace dataspace, const hidl_vec<Rect>& damage)
{
    if (!sHandleImporter.importBuffer(target)) {
        return Error::NO_RESOURCES;
    }

    int32_t fence;
    if (!sHandleImporter.importFence(acquireFence, fence)) {
        sHandleImporter.freeBuffer(target);
        return Error::NO_RESOURCES;
    }

    hwc_region_t damage_region = { damage.size(),
        reinterpret_cast<const hwc_rect_t*>(&damage[0]) };

    int32_t error = mDispatch.setClientTarget(mDevice, display,
            target, fence, static_cast<int32_t>(dataspace),
            damage_region);
    if (error == HWC2_ERROR_NONE) {
        std::lock_guard<std::mutex> lock(mDisplayMutex);

        auto dpy = mDisplays.find(display);
        dpy->second.ClientTarget = target;
    } else {
        sHandleImporter.freeBuffer(target);
        sHandleImporter.closeFence(fence);
    }

    return static_cast<Error>(error);
}

Return<Error> HwcHal::setColorMode(Display display, ColorMode mode)
{
    auto error = mDispatch.setColorMode(mDevice, display,
            static_cast<int32_t>(mode));
    return static_cast<Error>(error);
}

Return<Error> HwcHal::setColorTransform(Display display,
        const hidl_vec<float>& matrix, ColorTransform hint)
{
    auto error = mDispatch.setColorTransform(mDevice, display,
            &matrix[0], static_cast<int32_t>(hint));
    return static_cast<Error>(error);
}

Return<Error> HwcHal::setOutputBuffer(Display display,
        const native_handle_t* buffer,
        const native_handle_t* releaseFence)
{
    if (!sHandleImporter.importBuffer(buffer)) {
        return Error::NO_RESOURCES;
    }

    int32_t fence;
    if (!sHandleImporter.importFence(releaseFence, fence)) {
        sHandleImporter.freeBuffer(buffer);
        return Error::NO_RESOURCES;
    }

    int32_t error = mDispatch.setOutputBuffer(mDevice,
            display, buffer, fence);
    if (error == HWC2_ERROR_NONE) {
        std::lock_guard<std::mutex> lock(mDisplayMutex);

        auto dpy = mDisplays.find(display);
        dpy->second.OutputBuffer = buffer;
    } else {
        sHandleImporter.freeBuffer(buffer);
    }

    // unlike in setClientTarget, fence is owned by us and is always closed
    sHandleImporter.closeFence(fence);

    return static_cast<Error>(error);
}

Return<Error> HwcHal::setPowerMode(Display display, PowerMode mode)
{
    auto error = mDispatch.setPowerMode(mDevice, display,
            static_cast<int32_t>(mode));
    return static_cast<Error>(error);
}

Return<Error> HwcHal::setVsyncEnabled(Display display,
        Vsync enabled)
{
    auto error = mDispatch.setVsyncEnabled(mDevice, display,
            static_cast<int32_t>(enabled));
    return static_cast<Error>(error);
}

Return<void> HwcHal::validateDisplay(Display display,
        validateDisplay_cb hidl_cb)
{
    uint32_t types_count = 0;
    uint32_t reqs_count = 0;
    auto error = mDispatch.validateDisplay(mDevice, display,
            &types_count, &reqs_count);

    hidl_cb(static_cast<Error>(error), types_count, reqs_count);

    return Void();
}

Return<Error> HwcHal::setCursorPosition(Display display,
        Layer layer, int32_t x, int32_t y)
{
    auto error = mDispatch.setCursorPosition(mDevice, display, layer, x, y);
    return static_cast<Error>(error);
}

Return<Error> HwcHal::setLayerBuffer(Display display,
        Layer layer, const native_handle_t* buffer,
        const native_handle_t* acquireFence)
{
    if (!sHandleImporter.importBuffer(buffer)) {
        return Error::NO_RESOURCES;
    }

    int32_t fence;
    if (!sHandleImporter.importFence(acquireFence, fence)) {
        sHandleImporter.freeBuffer(buffer);
        return Error::NO_RESOURCES;
    }

    int32_t error = mDispatch.setLayerBuffer(mDevice,
            display, layer, buffer, fence);
    if (error == HWC2_ERROR_NONE) {
        std::lock_guard<std::mutex> lock(mDisplayMutex);

        auto dpy = mDisplays.find(display);
        dpy->second.LayerBuffers[layer] = buffer;
    } else {
        sHandleImporter.freeBuffer(buffer);
        sHandleImporter.closeFence(fence);
    }

    return static_cast<Error>(error);
}

Return<Error> HwcHal::setLayerSurfaceDamage(Display display,
        Layer layer, const hidl_vec<Rect>& damage)
{
    hwc_region_t damage_region = { damage.size(),
        reinterpret_cast<const hwc_rect_t*>(&damage[0]) };

    auto error = mDispatch.setLayerSurfaceDamage(mDevice, display, layer,
            damage_region);
    return static_cast<Error>(error);
}

Return<Error> HwcHal::setLayerBlendMode(Display display,
        Layer layer, BlendMode mode)
{
    auto error = mDispatch.setLayerBlendMode(mDevice, display, layer,
            static_cast<int32_t>(mode));
    return static_cast<Error>(error);
}

Return<Error> HwcHal::setLayerColor(Display display,
        Layer layer, const Color& color)
{
    hwc_color_t hwc_color{color.r, color.g, color.b, color.a};
    auto error = mDispatch.setLayerColor(mDevice, display, layer, hwc_color);
    return static_cast<Error>(error);
}

Return<Error> HwcHal::setLayerCompositionType(Display display,
        Layer layer, Composition type)
{
    auto error = mDispatch.setLayerCompositionType(mDevice, display, layer,
            static_cast<int32_t>(type));
    return static_cast<Error>(error);
}

Return<Error> HwcHal::setLayerDataspace(Display display,
        Layer layer, Dataspace dataspace)
{
    auto error = mDispatch.setLayerDataspace(mDevice, display, layer,
            static_cast<int32_t>(dataspace));
    return static_cast<Error>(error);
}

Return<Error> HwcHal::setLayerDisplayFrame(Display display,
        Layer layer, const Rect& frame)
{
    hwc_rect_t hwc_frame{frame.left, frame.top, frame.right, frame.bottom};
    auto error = mDispatch.setLayerDisplayFrame(mDevice, display, layer,
            hwc_frame);
    return static_cast<Error>(error);
}

Return<Error> HwcHal::setLayerPlaneAlpha(Display display,
        Layer layer, float alpha)
{
    auto error = mDispatch.setLayerPlaneAlpha(mDevice, display, layer, alpha);
    return static_cast<Error>(error);
}

Return<Error> HwcHal::setLayerSidebandStream(Display display,
        Layer layer, const native_handle_t* stream)
{
    if (!sHandleImporter.importBuffer(stream)) {
        return Error::NO_RESOURCES;
    }

    int32_t error = mDispatch.setLayerSidebandStream(mDevice,
            display, layer, stream);
    if (error == HWC2_ERROR_NONE) {
        std::lock_guard<std::mutex> lock(mDisplayMutex);

        auto dpy = mDisplays.find(display);
        dpy->second.LayerSidebandStreams[layer] = stream;
    } else {
        sHandleImporter.freeBuffer(stream);
    }

    return static_cast<Error>(error);
}

Return<Error> HwcHal::setLayerSourceCrop(Display display,
        Layer layer, const FRect& crop)
{
    hwc_frect_t hwc_crop{crop.left, crop.top, crop.right, crop.bottom};
    auto error = mDispatch.setLayerSourceCrop(mDevice, display, layer,
            hwc_crop);
    return static_cast<Error>(error);
}

Return<Error> HwcHal::setLayerTransform(Display display,
        Layer layer, Transform transform)
{
    auto error = mDispatch.setLayerTransform(mDevice, display, layer,
            static_cast<int32_t>(transform));
    return static_cast<Error>(error);
}

Return<Error> HwcHal::setLayerVisibleRegion(Display display,
        Layer layer, const hidl_vec<Rect>& visible)
{
    hwc_region_t visible_region = { visible.size(),
        reinterpret_cast<const hwc_rect_t*>(&visible[0]) };

    auto error = mDispatch.setLayerVisibleRegion(mDevice, display, layer,
            visible_region);
    return static_cast<Error>(error);
}

Return<Error> HwcHal::setLayerZOrder(Display display,
        Layer layer, uint32_t z)
{
    auto error = mDispatch.setLayerZOrder(mDevice, display, layer, z);
    return static_cast<Error>(error);
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
