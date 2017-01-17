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

#include <hardware/gralloc.h>
#include <hardware/gralloc1.h>
#include <log/log.h>

#include "Hwc.h"
#include "HwcClient.h"
#include "IComposerCommandBuffer.h"

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_1 {
namespace implementation {

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
        if (!handle) {
            return true;
        }

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

private:
    bool mInitialized;

    // Some existing gralloc drivers do not support retaining more than once,
    // when we are in passthrough mode.
#ifdef BINDERIZED
    bool openGralloc()
    {
        const hw_module_t* module = nullptr;
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
        }
        native_handle_close(handle);
        native_handle_delete(const_cast<native_handle_t*>(handle));
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

} // anonymous namespace

BufferClone::BufferClone()
    : mHandle(nullptr)
{
}

BufferClone::BufferClone(BufferClone&& other)
{
    mHandle = other.mHandle;
    other.mHandle = nullptr;
}

BufferClone& BufferClone::operator=(buffer_handle_t handle)
{
    clear();
    mHandle = handle;
    return *this;
}

BufferClone::~BufferClone()
{
    clear();
}

void BufferClone::clear()
{
    if (mHandle) {
        sHandleImporter.freeBuffer(mHandle);
    }
}

HwcClient::HwcClient(HwcHal& hal)
    : mHal(hal), mReader(*this), mWriter(kWriterInitialSize)
{
    if (!sHandleImporter.initialize()) {
        LOG_ALWAYS_FATAL("failed to initialize handle importer");
    }
}

HwcClient::~HwcClient()
{
    mHal.enableCallback(false);
    mHal.removeClient();

    // no need to grab the mutex as any in-flight hwbinder call should keep
    // the client alive
    for (const auto& dpy : mDisplayData) {
        if (!dpy.second.Layers.empty()) {
            ALOGW("client destroyed with valid layers");
        }
        for (const auto& ly : dpy.second.Layers) {
            mHal.destroyLayer(dpy.first, ly.first);
        }

        if (dpy.second.IsVirtual) {
            ALOGW("client destroyed with valid virtual display");
            mHal.destroyVirtualDisplay(dpy.first);
        }
    }

    mDisplayData.clear();

    sHandleImporter.cleanup();
}

void HwcClient::onHotplug(Display display,
        IComposerCallback::Connection connected)
{
    {
        std::lock_guard<std::mutex> lock(mDisplayDataMutex);

        if (connected == IComposerCallback::Connection::CONNECTED) {
            mDisplayData.emplace(display, DisplayData(false));
        } else if (connected == IComposerCallback::Connection::DISCONNECTED) {
            mDisplayData.erase(display);
        }
    }

    mCallback->onHotplug(display, connected);
}

void HwcClient::onRefresh(Display display)
{
    mCallback->onRefresh(display);
}

void HwcClient::onVsync(Display display, int64_t timestamp)
{
    mCallback->onVsync(display, timestamp);
}

Return<void> HwcClient::registerCallback(const sp<IComposerCallback>& callback)
{
    // no locking as we require this function to be called only once
    mCallback = callback;
    mHal.enableCallback(callback != nullptr);

    return Void();
}

Return<uint32_t> HwcClient::getMaxVirtualDisplayCount()
{
    return mHal.getMaxVirtualDisplayCount();
}

Return<void> HwcClient::createVirtualDisplay(uint32_t width, uint32_t height,
        PixelFormat formatHint, uint32_t outputBufferSlotCount,
        createVirtualDisplay_cb hidl_cb)
{
    Display display = 0;
    Error err = mHal.createVirtualDisplay(width, height,
            &formatHint, &display);
    if (err == Error::NONE) {
        std::lock_guard<std::mutex> lock(mDisplayDataMutex);

        auto dpy = mDisplayData.emplace(display, DisplayData(true)).first;
        dpy->second.OutputBuffers.resize(outputBufferSlotCount);
    }

    hidl_cb(err, display, formatHint);
    return Void();
}

Return<Error> HwcClient::destroyVirtualDisplay(Display display)
{
    Error err = mHal.destroyVirtualDisplay(display);
    if (err == Error::NONE) {
        std::lock_guard<std::mutex> lock(mDisplayDataMutex);

        mDisplayData.erase(display);
    }

    return err;
}

Return<void> HwcClient::createLayer(Display display, uint32_t bufferSlotCount,
        createLayer_cb hidl_cb)
{
    Layer layer = 0;
    Error err = mHal.createLayer(display, &layer);
    if (err == Error::NONE) {
        std::lock_guard<std::mutex> lock(mDisplayDataMutex);

        auto dpy = mDisplayData.find(display);
        auto ly = dpy->second.Layers.emplace(layer, LayerBuffers()).first;
        ly->second.Buffers.resize(bufferSlotCount);
    }

    hidl_cb(err, layer);
    return Void();
}

Return<Error> HwcClient::destroyLayer(Display display, Layer layer)
{
    Error err = mHal.destroyLayer(display, layer);
    if (err == Error::NONE) {
        std::lock_guard<std::mutex> lock(mDisplayDataMutex);

        auto dpy = mDisplayData.find(display);
        dpy->second.Layers.erase(layer);
    }

    return err;
}

Return<void> HwcClient::getActiveConfig(Display display,
        getActiveConfig_cb hidl_cb)
{
    Config config = 0;
    Error err = mHal.getActiveConfig(display, &config);

    hidl_cb(err, config);
    return Void();
}

Return<Error> HwcClient::getClientTargetSupport(Display display,
        uint32_t width, uint32_t height,
        PixelFormat format, Dataspace dataspace)
{
    Error err = mHal.getClientTargetSupport(display,
            width, height, format, dataspace);
    return err;
}

Return<void> HwcClient::getColorModes(Display display, getColorModes_cb hidl_cb)
{
    hidl_vec<ColorMode> modes;
    Error err = mHal.getColorModes(display, &modes);

    hidl_cb(err, modes);
    return Void();
}

Return<void> HwcClient::getDisplayAttribute(Display display,
        Config config, Attribute attribute,
        getDisplayAttribute_cb hidl_cb)
{
    int32_t value = 0;
    Error err = mHal.getDisplayAttribute(display, config, attribute, &value);

    hidl_cb(err, value);
    return Void();
}

Return<void> HwcClient::getDisplayConfigs(Display display,
        getDisplayConfigs_cb hidl_cb)
{
    hidl_vec<Config> configs;
    Error err = mHal.getDisplayConfigs(display, &configs);

    hidl_cb(err, configs);
    return Void();
}

Return<void> HwcClient::getDisplayName(Display display,
        getDisplayName_cb hidl_cb)
{
    hidl_string name;
    Error err = mHal.getDisplayName(display, &name);

    hidl_cb(err, name);
    return Void();
}

Return<void> HwcClient::getDisplayType(Display display,
        getDisplayType_cb hidl_cb)
{
    DisplayType type = DisplayType::INVALID;
    Error err = mHal.getDisplayType(display, &type);

    hidl_cb(err, type);
    return Void();
}

Return<void> HwcClient::getDozeSupport(Display display,
        getDozeSupport_cb hidl_cb)
{
    bool support = false;
    Error err = mHal.getDozeSupport(display, &support);

    hidl_cb(err, support);
    return Void();
}

Return<void> HwcClient::getHdrCapabilities(Display display,
        getHdrCapabilities_cb hidl_cb)
{
    hidl_vec<Hdr> types;
    float max_lumi = 0.0f;
    float max_avg_lumi = 0.0f;
    float min_lumi = 0.0f;
    Error err = mHal.getHdrCapabilities(display, &types,
            &max_lumi, &max_avg_lumi, &min_lumi);

    hidl_cb(err, types, max_lumi, max_avg_lumi, min_lumi);
    return Void();
}

Return<Error> HwcClient::setClientTargetSlotCount(Display display,
        uint32_t clientTargetSlotCount)
{
    std::lock_guard<std::mutex> lock(mDisplayDataMutex);

    auto dpy = mDisplayData.find(display);
    if (dpy == mDisplayData.end()) {
        return Error::BAD_DISPLAY;
    }

    dpy->second.ClientTargets.resize(clientTargetSlotCount);

    return Error::NONE;
}

Return<Error> HwcClient::setActiveConfig(Display display, Config config)
{
    Error err = mHal.setActiveConfig(display, config);
    return err;
}

Return<Error> HwcClient::setColorMode(Display display, ColorMode mode)
{
    Error err = mHal.setColorMode(display, mode);
    return err;
}

Return<Error> HwcClient::setPowerMode(Display display, PowerMode mode)
{
    Error err = mHal.setPowerMode(display, mode);
    return err;
}

Return<Error> HwcClient::setVsyncEnabled(Display display, Vsync enabled)
{
    Error err = mHal.setVsyncEnabled(display, enabled);
    return err;
}

Return<Error> HwcClient::setInputCommandQueue(
        const MQDescriptorSync<uint32_t>& descriptor)
{
    std::lock_guard<std::mutex> lock(mCommandMutex);
    return mReader.setMQDescriptor(descriptor) ?
        Error::NONE : Error::NO_RESOURCES;
}

Return<void> HwcClient::getOutputCommandQueue(
        getOutputCommandQueue_cb hidl_cb)
{
    // no locking as we require this function to be called inside
    // executeCommands_cb

    auto outDescriptor = mWriter.getMQDescriptor();
    if (outDescriptor) {
        hidl_cb(Error::NONE, *outDescriptor);
    } else {
        hidl_cb(Error::NO_RESOURCES, CommandQueueType::Descriptor());
    }

    return Void();
}

Return<void> HwcClient::executeCommands(uint32_t inLength,
        const hidl_vec<hidl_handle>& inHandles,
        executeCommands_cb hidl_cb)
{
    std::lock_guard<std::mutex> lock(mCommandMutex);

    bool outChanged = false;
    uint32_t outLength = 0;
    hidl_vec<hidl_handle> outHandles;

    if (!mReader.readQueue(inLength, inHandles)) {
        hidl_cb(Error::BAD_PARAMETER, outChanged, outLength, outHandles);
        return Void();
    }

    Error err = mReader.parse();
    if (err == Error::NONE &&
            !mWriter.writeQueue(&outChanged, &outLength, &outHandles)) {
        err = Error::NO_RESOURCES;
    }

    hidl_cb(err, outChanged, outLength, outHandles);

    mReader.reset();
    mWriter.reset();

    return Void();
}

HwcClient::CommandReader::CommandReader(HwcClient& client)
    : mClient(client), mHal(client.mHal), mWriter(client.mWriter)
{
}

Error HwcClient::CommandReader::parse()
{
    IComposerClient::Command command;
    uint16_t length = 0;

    while (!isEmpty()) {
        if (!beginCommand(&command, &length)) {
            break;
        }

        bool parsed = false;
        switch (command) {
        case IComposerClient::Command::SELECT_DISPLAY:
            parsed = parseSelectDisplay(length);
            break;
        case IComposerClient::Command::SELECT_LAYER:
            parsed = parseSelectLayer(length);
            break;
        case IComposerClient::Command::SET_COLOR_TRANSFORM:
            parsed = parseSetColorTransform(length);
            break;
        case IComposerClient::Command::SET_CLIENT_TARGET:
            parsed = parseSetClientTarget(length);
            break;
        case IComposerClient::Command::SET_OUTPUT_BUFFER:
            parsed = parseSetOutputBuffer(length);
            break;
        case IComposerClient::Command::VALIDATE_DISPLAY:
            parsed = parseValidateDisplay(length);
            break;
        case IComposerClient::Command::ACCEPT_DISPLAY_CHANGES:
            parsed = parseAcceptDisplayChanges(length);
            break;
        case IComposerClient::Command::PRESENT_DISPLAY:
            parsed = parsePresentDisplay(length);
            break;
        case IComposerClient::Command::SET_LAYER_CURSOR_POSITION:
            parsed = parseSetLayerCursorPosition(length);
            break;
        case IComposerClient::Command::SET_LAYER_BUFFER:
            parsed = parseSetLayerBuffer(length);
            break;
        case IComposerClient::Command::SET_LAYER_SURFACE_DAMAGE:
            parsed = parseSetLayerSurfaceDamage(length);
            break;
        case IComposerClient::Command::SET_LAYER_BLEND_MODE:
            parsed = parseSetLayerBlendMode(length);
            break;
        case IComposerClient::Command::SET_LAYER_COLOR:
            parsed = parseSetLayerColor(length);
            break;
        case IComposerClient::Command::SET_LAYER_COMPOSITION_TYPE:
            parsed = parseSetLayerCompositionType(length);
            break;
        case IComposerClient::Command::SET_LAYER_DATASPACE:
            parsed = parseSetLayerDataspace(length);
            break;
        case IComposerClient::Command::SET_LAYER_DISPLAY_FRAME:
            parsed = parseSetLayerDisplayFrame(length);
            break;
        case IComposerClient::Command::SET_LAYER_PLANE_ALPHA:
            parsed = parseSetLayerPlaneAlpha(length);
            break;
        case IComposerClient::Command::SET_LAYER_SIDEBAND_STREAM:
            parsed = parseSetLayerSidebandStream(length);
            break;
        case IComposerClient::Command::SET_LAYER_SOURCE_CROP:
            parsed = parseSetLayerSourceCrop(length);
            break;
        case IComposerClient::Command::SET_LAYER_TRANSFORM:
            parsed = parseSetLayerTransform(length);
            break;
        case IComposerClient::Command::SET_LAYER_VISIBLE_REGION:
            parsed = parseSetLayerVisibleRegion(length);
            break;
        case IComposerClient::Command::SET_LAYER_Z_ORDER:
            parsed = parseSetLayerZOrder(length);
            break;
        default:
            parsed = false;
            break;
        }

        endCommand();

        if (!parsed) {
            ALOGE("failed to parse command 0x%x, length %" PRIu16,
                    command, length);
            break;
        }
    }

    return (isEmpty()) ? Error::NONE : Error::BAD_PARAMETER;
}

bool HwcClient::CommandReader::parseSelectDisplay(uint16_t length)
{
    if (length != CommandWriterBase::kSelectDisplayLength) {
        return false;
    }

    mDisplay = read64();
    mWriter.selectDisplay(mDisplay);

    return true;
}

bool HwcClient::CommandReader::parseSelectLayer(uint16_t length)
{
    if (length != CommandWriterBase::kSelectLayerLength) {
        return false;
    }

    mLayer = read64();

    return true;
}

bool HwcClient::CommandReader::parseSetColorTransform(uint16_t length)
{
    if (length != CommandWriterBase::kSetColorTransformLength) {
        return false;
    }

    float matrix[16];
    for (int i = 0; i < 16; i++) {
        matrix[i] = readFloat();
    }
    auto transform = readSigned();

    auto err = mHal.setColorTransform(mDisplay, matrix, transform);
    if (err != Error::NONE) {
        mWriter.setError(getCommandLoc(), err);
    }

    return true;
}

bool HwcClient::CommandReader::parseSetClientTarget(uint16_t length)
{
    // 4 parameters followed by N rectangles
    if ((length - 4) % 4 != 0) {
        return false;
    }

    bool useCache = false;
    auto slot = read();
    auto clientTarget = readHandle(&useCache);
    auto fence = readFence();
    auto dataspace = readSigned();
    auto damage = readRegion((length - 4) / 4);

    auto err = lookupBuffer(BufferCache::CLIENT_TARGETS,
            slot, useCache, clientTarget);
    if (err == Error::NONE) {
        err = mHal.setClientTarget(mDisplay, clientTarget, fence,
                dataspace, damage);
    }
    if (err != Error::NONE) {
        close(fence);
        mWriter.setError(getCommandLoc(), err);
    }

    return true;
}

bool HwcClient::CommandReader::parseSetOutputBuffer(uint16_t length)
{
    if (length != CommandWriterBase::kSetOutputBufferLength) {
        return false;
    }

    bool useCache = false;
    auto slot = read();
    auto outputBuffer = readHandle(&useCache);
    auto fence = readFence();

    auto err = lookupBuffer(BufferCache::OUTPUT_BUFFERS,
            slot, useCache, outputBuffer);
    if (err == Error::NONE) {
        err = mHal.setOutputBuffer(mDisplay, outputBuffer, fence);
    }
    if (err != Error::NONE) {
        close(fence);
        mWriter.setError(getCommandLoc(), err);
    }

    return true;
}

bool HwcClient::CommandReader::parseValidateDisplay(uint16_t length)
{
    if (length != CommandWriterBase::kValidateDisplayLength) {
        return false;
    }

    std::vector<Layer> changedLayers;
    std::vector<IComposerClient::Composition> compositionTypes;
    uint32_t displayRequestMask = 0x0;
    std::vector<Layer> requestedLayers;
    std::vector<uint32_t> requestMasks;

    auto err = mHal.validateDisplay(mDisplay, &changedLayers,
            &compositionTypes, &displayRequestMask,
            &requestedLayers, &requestMasks);
    if (err == Error::NONE) {
        mWriter.setChangedCompositionTypes(changedLayers,
                compositionTypes);
        mWriter.setDisplayRequests(displayRequestMask,
                requestedLayers, requestMasks);
    } else {
        mWriter.setError(getCommandLoc(), err);
    }

    return true;
}

bool HwcClient::CommandReader::parseAcceptDisplayChanges(uint16_t length)
{
    if (length != CommandWriterBase::kAcceptDisplayChangesLength) {
        return false;
    }

    auto err = mHal.acceptDisplayChanges(mDisplay);
    if (err != Error::NONE) {
        mWriter.setError(getCommandLoc(), err);
    }

    return true;
}

bool HwcClient::CommandReader::parsePresentDisplay(uint16_t length)
{
    if (length != CommandWriterBase::kPresentDisplayLength) {
        return false;
    }

    int presentFence = -1;
    std::vector<Layer> layers;
    std::vector<int> fences;
    auto err = mHal.presentDisplay(mDisplay, &presentFence, &layers, &fences);
    if (err == Error::NONE) {
        mWriter.setPresentFence(presentFence);
        mWriter.setReleaseFences(layers, fences);
    } else {
        mWriter.setError(getCommandLoc(), err);
    }

    return true;
}

bool HwcClient::CommandReader::parseSetLayerCursorPosition(uint16_t length)
{
    if (length != CommandWriterBase::kSetLayerCursorPositionLength) {
        return false;
    }

    auto err = mHal.setLayerCursorPosition(mDisplay, mLayer,
            readSigned(), readSigned());
    if (err != Error::NONE) {
        mWriter.setError(getCommandLoc(), err);
    }

    return true;
}

bool HwcClient::CommandReader::parseSetLayerBuffer(uint16_t length)
{
    if (length != CommandWriterBase::kSetLayerBufferLength) {
        return false;
    }

    bool useCache = false;
    auto slot = read();
    auto buffer = readHandle(&useCache);
    auto fence = readFence();

    auto err = lookupBuffer(BufferCache::LAYER_BUFFERS,
            slot, useCache, buffer);
    if (err == Error::NONE) {
        err = mHal.setLayerBuffer(mDisplay, mLayer, buffer, fence);
    }
    if (err != Error::NONE) {
        close(fence);
        mWriter.setError(getCommandLoc(), err);
    }

    return true;
}

bool HwcClient::CommandReader::parseSetLayerSurfaceDamage(uint16_t length)
{
    // N rectangles
    if (length % 4 != 0) {
        return false;
    }

    auto damage = readRegion(length / 4);
    auto err = mHal.setLayerSurfaceDamage(mDisplay, mLayer, damage);
    if (err != Error::NONE) {
        mWriter.setError(getCommandLoc(), err);
    }

    return true;
}

bool HwcClient::CommandReader::parseSetLayerBlendMode(uint16_t length)
{
    if (length != CommandWriterBase::kSetLayerBlendModeLength) {
        return false;
    }

    auto err = mHal.setLayerBlendMode(mDisplay, mLayer, readSigned());
    if (err != Error::NONE) {
        mWriter.setError(getCommandLoc(), err);
    }

    return true;
}

bool HwcClient::CommandReader::parseSetLayerColor(uint16_t length)
{
    if (length != CommandWriterBase::kSetLayerColorLength) {
        return false;
    }

    auto err = mHal.setLayerColor(mDisplay, mLayer, readColor());
    if (err != Error::NONE) {
        mWriter.setError(getCommandLoc(), err);
    }

    return true;
}

bool HwcClient::CommandReader::parseSetLayerCompositionType(uint16_t length)
{
    if (length != CommandWriterBase::kSetLayerCompositionTypeLength) {
        return false;
    }

    auto err = mHal.setLayerCompositionType(mDisplay, mLayer, readSigned());
    if (err != Error::NONE) {
        mWriter.setError(getCommandLoc(), err);
    }

    return true;
}

bool HwcClient::CommandReader::parseSetLayerDataspace(uint16_t length)
{
    if (length != CommandWriterBase::kSetLayerDataspaceLength) {
        return false;
    }

    auto err = mHal.setLayerDataspace(mDisplay, mLayer, readSigned());
    if (err != Error::NONE) {
        mWriter.setError(getCommandLoc(), err);
    }

    return true;
}

bool HwcClient::CommandReader::parseSetLayerDisplayFrame(uint16_t length)
{
    if (length != CommandWriterBase::kSetLayerDisplayFrameLength) {
        return false;
    }

    auto err = mHal.setLayerDisplayFrame(mDisplay, mLayer, readRect());
    if (err != Error::NONE) {
        mWriter.setError(getCommandLoc(), err);
    }

    return true;
}

bool HwcClient::CommandReader::parseSetLayerPlaneAlpha(uint16_t length)
{
    if (length != CommandWriterBase::kSetLayerPlaneAlphaLength) {
        return false;
    }

    auto err = mHal.setLayerPlaneAlpha(mDisplay, mLayer, readFloat());
    if (err != Error::NONE) {
        mWriter.setError(getCommandLoc(), err);
    }

    return true;
}

bool HwcClient::CommandReader::parseSetLayerSidebandStream(uint16_t length)
{
    if (length != CommandWriterBase::kSetLayerSidebandStreamLength) {
        return false;
    }

    auto stream = readHandle();

    auto err = lookupLayerSidebandStream(stream);
    if (err == Error::NONE) {
        err = mHal.setLayerSidebandStream(mDisplay, mLayer, stream);
    }
    if (err != Error::NONE) {
        mWriter.setError(getCommandLoc(), err);
    }

    return true;
}

bool HwcClient::CommandReader::parseSetLayerSourceCrop(uint16_t length)
{
    if (length != CommandWriterBase::kSetLayerSourceCropLength) {
        return false;
    }

    auto err = mHal.setLayerSourceCrop(mDisplay, mLayer, readFRect());
    if (err != Error::NONE) {
        mWriter.setError(getCommandLoc(), err);
    }

    return true;
}

bool HwcClient::CommandReader::parseSetLayerTransform(uint16_t length)
{
    if (length != CommandWriterBase::kSetLayerTransformLength) {
        return false;
    }

    auto err = mHal.setLayerTransform(mDisplay, mLayer, readSigned());
    if (err != Error::NONE) {
        mWriter.setError(getCommandLoc(), err);
    }

    return true;
}

bool HwcClient::CommandReader::parseSetLayerVisibleRegion(uint16_t length)
{
    // N rectangles
    if (length % 4 != 0) {
        return false;
    }

    auto region = readRegion(length / 4);
    auto err = mHal.setLayerVisibleRegion(mDisplay, mLayer, region);
    if (err != Error::NONE) {
        mWriter.setError(getCommandLoc(), err);
    }

    return true;
}

bool HwcClient::CommandReader::parseSetLayerZOrder(uint16_t length)
{
    if (length != CommandWriterBase::kSetLayerZOrderLength) {
        return false;
    }

    auto err = mHal.setLayerZOrder(mDisplay, mLayer, read());
    if (err != Error::NONE) {
        mWriter.setError(getCommandLoc(), err);
    }

    return true;
}

hwc_rect_t HwcClient::CommandReader::readRect()
{
    return hwc_rect_t{
        readSigned(),
        readSigned(),
        readSigned(),
        readSigned(),
    };
}

std::vector<hwc_rect_t> HwcClient::CommandReader::readRegion(size_t count)
{
    std::vector<hwc_rect_t> region;
    region.reserve(count);
    while (count > 0) {
        region.emplace_back(readRect());
        count--;
    }

    return region;
}

hwc_frect_t HwcClient::CommandReader::readFRect()
{
    return hwc_frect_t{
        readFloat(),
        readFloat(),
        readFloat(),
        readFloat(),
    };
}

Error HwcClient::CommandReader::lookupBuffer(BufferCache cache, uint32_t slot,
        bool useCache, buffer_handle_t& handle)
{
    std::lock_guard<std::mutex> lock(mClient.mDisplayDataMutex);

    auto dpy = mClient.mDisplayData.find(mDisplay);
    if (dpy == mClient.mDisplayData.end()) {
        return Error::BAD_DISPLAY;
    }

    BufferClone* clone = nullptr;
    switch (cache) {
    case BufferCache::CLIENT_TARGETS:
        if (slot < dpy->second.ClientTargets.size()) {
            clone = &dpy->second.ClientTargets[slot];
        }
        break;
    case BufferCache::OUTPUT_BUFFERS:
        if (slot < dpy->second.OutputBuffers.size()) {
            clone = &dpy->second.OutputBuffers[slot];
        }
        break;
    case BufferCache::LAYER_BUFFERS:
        {
            auto ly = dpy->second.Layers.find(mLayer);
            if (ly == dpy->second.Layers.end()) {
                return Error::BAD_LAYER;
            }
            if (slot < ly->second.Buffers.size()) {
                clone = &ly->second.Buffers[slot];
            }
        }
        break;
    case BufferCache::LAYER_SIDEBAND_STREAMS:
        {
            auto ly = dpy->second.Layers.find(mLayer);
            if (ly == dpy->second.Layers.end()) {
                return Error::BAD_LAYER;
            }
            if (slot == 0) {
                clone = &ly->second.SidebandStream;
            }
        }
        break;
    default:
        break;
    }

    if (!clone) {
        ALOGW("invalid buffer slot");
        return Error::BAD_PARAMETER;
    }

    // use or update cache
    if (useCache) {
        handle = *clone;
    } else {
        if (!sHandleImporter.importBuffer(handle)) {
            return Error::NO_RESOURCES;
        }

        *clone = handle;
    }

    return Error::NONE;
}

} // namespace implementation
} // namespace V2_1
} // namespace composer
} // namespace graphics
} // namespace hardware
} // namespace android
