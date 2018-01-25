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

#include <android/hardware/graphics/mapper/2.0/IMapper.h>
#include <log/log.h>

#include "ComposerClient.h"

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_1 {
namespace implementation {

ComposerClient::ComposerClient(ComposerHal& hal)
    : mHal(hal), mWriter(kWriterInitialSize)
{
}

ComposerClient::~ComposerClient()
{
    ALOGD("destroying composer client");

    mHal.enableCallback(false);
    destroyResources();
    mHal.removeClient();

    ALOGD("removed composer client");
}

void ComposerClient::destroyResources()
{
    // We want to call hwc2_close here (and move hwc2_open to the
    // constructor), with the assumption that hwc2_close would
    //
    //  - clean up all resources owned by the client
    //  - make sure all displays are blank (since there is no layer)
    //
    // But since SF used to crash at this point, different hwcomposer2
    // implementations behave differently on hwc2_close.  Our only portable
    // choice really is to abort().  But that is not an option anymore
    // because we might also have VTS or VR as clients that can come and go.
    //
    // Below we manually clean all resources (layers and virtual
    // displays), and perform a presentDisplay afterwards.
    mResources->clear([this](Display display, bool isVirtual, const std::vector<Layer> layers) {
        ALOGW("destroying client resources for display %" PRIu64, display);

        for (auto layer : layers) {
            mHal.destroyLayer(display, layer);
        }

        if (isVirtual) {
            mHal.destroyVirtualDisplay(display);
        } else {
            ALOGW("performing a final presentDisplay");

            std::vector<Layer> changedLayers;
            std::vector<IComposerClient::Composition> compositionTypes;
            uint32_t displayRequestMask = 0;
            std::vector<Layer> requestedLayers;
            std::vector<uint32_t> requestMasks;
            mHal.validateDisplay(display, &changedLayers, &compositionTypes, &displayRequestMask,
                                 &requestedLayers, &requestMasks);

            mHal.acceptDisplayChanges(display);

            int32_t presentFence = -1;
            std::vector<Layer> releasedLayers;
            std::vector<int32_t> releaseFences;
            mHal.presentDisplay(display, &presentFence, &releasedLayers, &releaseFences);
            if (presentFence >= 0) {
                close(presentFence);
            }
            for (auto fence : releaseFences) {
                if (fence >= 0) {
                    close(fence);
                }
            }
        }
    });

    mResources.reset();
}

void ComposerClient::initialize()
{
    mResources = createResources();
    if (!mResources) {
        LOG_ALWAYS_FATAL("failed to create resources");
    }

    mReader = createCommandReader();
}

void ComposerClient::onHotplug(Display display,
        IComposerCallback::Connection connected)
{
    if (connected == IComposerCallback::Connection::CONNECTED) {
        mResources->addPhysicalDisplay(display);
    } else if (connected == IComposerCallback::Connection::DISCONNECTED) {
        mResources->removeDisplay(display);
    }

    auto ret = mCallback->onHotplug(display, connected);
    ALOGE_IF(!ret.isOk(), "failed to send onHotplug: %s",
            ret.description().c_str());
}

void ComposerClient::onRefresh(Display display)
{
    auto ret = mCallback->onRefresh(display);
    ALOGE_IF(!ret.isOk(), "failed to send onRefresh: %s",
            ret.description().c_str());
}

void ComposerClient::onVsync(Display display, int64_t timestamp)
{
    auto ret = mCallback->onVsync(display, timestamp);
    ALOGE_IF(!ret.isOk(), "failed to send onVsync: %s",
            ret.description().c_str());
}

Return<void> ComposerClient::registerCallback(
        const sp<IComposerCallback>& callback)
{
    // no locking as we require this function to be called only once
    mCallback = callback;
    mHal.enableCallback(callback != nullptr);

    return Void();
}

Return<uint32_t> ComposerClient::getMaxVirtualDisplayCount()
{
    return mHal.getMaxVirtualDisplayCount();
}

Return<void> ComposerClient::createVirtualDisplay(uint32_t width,
        uint32_t height, PixelFormat formatHint, uint32_t outputBufferSlotCount,
        createVirtualDisplay_cb hidl_cb)
{
    Display display = 0;
    Error err = mHal.createVirtualDisplay(width, height,
            &formatHint, &display);
    if (err == Error::NONE) {
        mResources->addVirtualDisplay(display, outputBufferSlotCount);
    }

    hidl_cb(err, display, formatHint);
    return Void();
}

Return<Error> ComposerClient::destroyVirtualDisplay(Display display)
{
    Error err = mHal.destroyVirtualDisplay(display);
    if (err == Error::NONE) {
        mResources->removeDisplay(display);
    }

    return err;
}

Return<void> ComposerClient::createLayer(Display display,
        uint32_t bufferSlotCount, createLayer_cb hidl_cb)
{
    Layer layer = 0;
    Error err = mHal.createLayer(display, &layer);
    if (err == Error::NONE) {
        err = mResources->addLayer(display, layer, bufferSlotCount);
        if (err != Error::NONE) {
            // The display entry may have already been removed by onHotplug.
            // Note: We do not destroy the layer on this error as the hotplug
            // disconnect invalidates the display id. The implementation should
            // ensure all layers for the display are destroyed.
        }
    }

    hidl_cb(err, layer);
    return Void();
}

Return<Error> ComposerClient::destroyLayer(Display display, Layer layer)
{
    Error err = mHal.destroyLayer(display, layer);
    if (err == Error::NONE) {
        mResources->removeLayer(display, layer);
    }

    return err;
}

Return<void> ComposerClient::getActiveConfig(Display display,
        getActiveConfig_cb hidl_cb)
{
    Config config = 0;
    Error err = mHal.getActiveConfig(display, &config);

    hidl_cb(err, config);
    return Void();
}

Return<Error> ComposerClient::getClientTargetSupport(Display display,
        uint32_t width, uint32_t height,
        PixelFormat format, Dataspace dataspace)
{
    Error err = mHal.getClientTargetSupport(display,
            width, height, format, dataspace);
    return err;
}

Return<void> ComposerClient::getColorModes(Display display,
          getColorModes_cb hidl_cb)
{
    hidl_vec<ColorMode> modes;
    Error err = mHal.getColorModes(display, &modes);

    hidl_cb(err, modes);
    return Void();
}

Return<void> ComposerClient::getDisplayAttribute(Display display,
        Config config, Attribute attribute,
        getDisplayAttribute_cb hidl_cb)
{
    int32_t value = 0;
    Error err = mHal.getDisplayAttribute(display, config, attribute, &value);

    hidl_cb(err, value);
    return Void();
}

Return<void> ComposerClient::getDisplayConfigs(Display display,
        getDisplayConfigs_cb hidl_cb)
{
    hidl_vec<Config> configs;
    Error err = mHal.getDisplayConfigs(display, &configs);

    hidl_cb(err, configs);
    return Void();
}

Return<void> ComposerClient::getDisplayName(Display display,
        getDisplayName_cb hidl_cb)
{
    hidl_string name;
    Error err = mHal.getDisplayName(display, &name);

    hidl_cb(err, name);
    return Void();
}

Return<void> ComposerClient::getDisplayType(Display display,
        getDisplayType_cb hidl_cb)
{
    DisplayType type = DisplayType::INVALID;
    Error err = mHal.getDisplayType(display, &type);

    hidl_cb(err, type);
    return Void();
}

Return<void> ComposerClient::getDozeSupport(Display display,
        getDozeSupport_cb hidl_cb)
{
    bool support = false;
    Error err = mHal.getDozeSupport(display, &support);

    hidl_cb(err, support);
    return Void();
}

Return<void> ComposerClient::getHdrCapabilities(Display display,
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

Return<Error> ComposerClient::setClientTargetSlotCount(Display display,
        uint32_t clientTargetSlotCount)
{
    return mResources->setDisplayClientTargetCacheSize(display, clientTargetSlotCount);
}

Return<Error> ComposerClient::setActiveConfig(Display display, Config config)
{
    Error err = mHal.setActiveConfig(display, config);
    return err;
}

Return<Error> ComposerClient::setColorMode(Display display, ColorMode mode)
{
    Error err = mHal.setColorMode(display, mode);
    return err;
}

Return<Error> ComposerClient::setPowerMode(Display display, PowerMode mode)
{
    Error err = mHal.setPowerMode(display, mode);
    return err;
}

Return<Error> ComposerClient::setVsyncEnabled(Display display, Vsync enabled)
{
    Error err = mHal.setVsyncEnabled(display, enabled);
    return err;
}

Return<Error> ComposerClient::setInputCommandQueue(
        const MQDescriptorSync<uint32_t>& descriptor)
{
    std::lock_guard<std::mutex> lock(mCommandMutex);
    return mReader->setMQDescriptor(descriptor) ?
        Error::NONE : Error::NO_RESOURCES;
}

Return<void> ComposerClient::getOutputCommandQueue(
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

Return<void> ComposerClient::executeCommands(uint32_t inLength,
        const hidl_vec<hidl_handle>& inHandles,
        executeCommands_cb hidl_cb)
{
    std::lock_guard<std::mutex> lock(mCommandMutex);

    bool outChanged = false;
    uint32_t outLength = 0;
    hidl_vec<hidl_handle> outHandles;

    if (!mReader->readQueue(inLength, inHandles)) {
        hidl_cb(Error::BAD_PARAMETER, outChanged, outLength, outHandles);
        return Void();
    }

    Error err = mReader->parse();
    if (err == Error::NONE &&
            !mWriter.writeQueue(&outChanged, &outLength, &outHandles)) {
        err = Error::NO_RESOURCES;
    }

    hidl_cb(err, outChanged, outLength, outHandles);

    mReader->reset();
    mWriter.reset();

    return Void();
}

std::unique_ptr<ComposerResources> ComposerClient::createResources()
{
    return ComposerResources::create();
}

std::unique_ptr<ComposerClient::CommandReader>
ComposerClient::createCommandReader()
{
    return std::unique_ptr<ComposerClient::CommandReader>(
        new CommandReader(*this));
}

ComposerClient::CommandReader::CommandReader(ComposerClient& client)
    : mHal(client.mHal), mResources(client.mResources.get()), mWriter(client.mWriter)
{
}

ComposerClient::CommandReader::~CommandReader()
{
}

Error ComposerClient::CommandReader::parse()
{
    IComposerClient::Command command;
    uint16_t length = 0;

    while (!isEmpty()) {
        if (!beginCommand(&command, &length)) {
            break;
        }

        bool parsed = parseCommand(command, length);
        endCommand();

        if (!parsed) {
            ALOGE("failed to parse command 0x%x, length %" PRIu16,
                    command, length);
            break;
        }
    }

    return (isEmpty()) ? Error::NONE : Error::BAD_PARAMETER;
}

bool ComposerClient::CommandReader::parseCommand(
        IComposerClient::Command command, uint16_t length) {
    switch (command) {
    case IComposerClient::Command::SELECT_DISPLAY:
        return parseSelectDisplay(length);
    case IComposerClient::Command::SELECT_LAYER:
        return parseSelectLayer(length);
    case IComposerClient::Command::SET_COLOR_TRANSFORM:
        return parseSetColorTransform(length);
    case IComposerClient::Command::SET_CLIENT_TARGET:
        return parseSetClientTarget(length);
    case IComposerClient::Command::SET_OUTPUT_BUFFER:
        return parseSetOutputBuffer(length);
    case IComposerClient::Command::VALIDATE_DISPLAY:
        return parseValidateDisplay(length);
    case IComposerClient::Command::PRESENT_OR_VALIDATE_DISPLAY:
        return parsePresentOrValidateDisplay(length);
    case IComposerClient::Command::ACCEPT_DISPLAY_CHANGES:
        return parseAcceptDisplayChanges(length);
    case IComposerClient::Command::PRESENT_DISPLAY:
        return parsePresentDisplay(length);
    case IComposerClient::Command::SET_LAYER_CURSOR_POSITION:
        return parseSetLayerCursorPosition(length);
    case IComposerClient::Command::SET_LAYER_BUFFER:
        return parseSetLayerBuffer(length);
    case IComposerClient::Command::SET_LAYER_SURFACE_DAMAGE:
        return parseSetLayerSurfaceDamage(length);
    case IComposerClient::Command::SET_LAYER_BLEND_MODE:
        return parseSetLayerBlendMode(length);
    case IComposerClient::Command::SET_LAYER_COLOR:
        return parseSetLayerColor(length);
    case IComposerClient::Command::SET_LAYER_COMPOSITION_TYPE:
        return parseSetLayerCompositionType(length);
    case IComposerClient::Command::SET_LAYER_DATASPACE:
        return parseSetLayerDataspace(length);
    case IComposerClient::Command::SET_LAYER_DISPLAY_FRAME:
        return parseSetLayerDisplayFrame(length);
    case IComposerClient::Command::SET_LAYER_PLANE_ALPHA:
        return parseSetLayerPlaneAlpha(length);
    case IComposerClient::Command::SET_LAYER_SIDEBAND_STREAM:
        return parseSetLayerSidebandStream(length);
    case IComposerClient::Command::SET_LAYER_SOURCE_CROP:
        return parseSetLayerSourceCrop(length);
    case IComposerClient::Command::SET_LAYER_TRANSFORM:
        return parseSetLayerTransform(length);
    case IComposerClient::Command::SET_LAYER_VISIBLE_REGION:
        return parseSetLayerVisibleRegion(length);
    case IComposerClient::Command::SET_LAYER_Z_ORDER:
        return parseSetLayerZOrder(length);
    default:
        return false;
    }
}

bool ComposerClient::CommandReader::parseSelectDisplay(uint16_t length)
{
    if (length != CommandWriterBase::kSelectDisplayLength) {
        return false;
    }

    mDisplay = read64();
    mWriter.selectDisplay(mDisplay);

    return true;
}

bool ComposerClient::CommandReader::parseSelectLayer(uint16_t length)
{
    if (length != CommandWriterBase::kSelectLayerLength) {
        return false;
    }

    mLayer = read64();

    return true;
}

bool ComposerClient::CommandReader::parseSetColorTransform(uint16_t length)
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

bool ComposerClient::CommandReader::parseSetClientTarget(uint16_t length)
{
    // 4 parameters followed by N rectangles
    if ((length - 4) % 4 != 0) {
        return false;
    }

    bool useCache = false;
    auto slot = read();
    auto rawHandle = readHandle(&useCache);
    auto fence = readFence();
    auto dataspace = readSigned();
    auto damage = readRegion((length - 4) / 4);
    bool closeFence = true;

    const native_handle_t* clientTarget;
    ComposerResources::ReplacedBufferHandle replacedClientTarget;
    auto err = mResources->getDisplayClientTarget(mDisplay,
            slot, useCache, rawHandle, &clientTarget, &replacedClientTarget);
    if (err == Error::NONE) {
        err = mHal.setClientTarget(mDisplay, clientTarget, fence,
                dataspace, damage);
        if (err == Error::NONE) {
            closeFence = false;
        }
    }
    if (closeFence) {
        close(fence);
    }
    if (err != Error::NONE) {
        mWriter.setError(getCommandLoc(), err);
    }

    return true;
}

bool ComposerClient::CommandReader::parseSetOutputBuffer(uint16_t length)
{
    if (length != CommandWriterBase::kSetOutputBufferLength) {
        return false;
    }

    bool useCache = false;
    auto slot = read();
    auto rawhandle = readHandle(&useCache);
    auto fence = readFence();
    bool closeFence = true;

    const native_handle_t* outputBuffer;
    ComposerResources::ReplacedBufferHandle replacedOutputBuffer;
    auto err = mResources->getDisplayOutputBuffer(mDisplay,
            slot, useCache, rawhandle, &outputBuffer, &replacedOutputBuffer);
    if (err == Error::NONE) {
        err = mHal.setOutputBuffer(mDisplay, outputBuffer, fence);
        if (err == Error::NONE) {
            closeFence = false;
        }
    }
    if (closeFence) {
        close(fence);
    }
    if (err != Error::NONE) {
        mWriter.setError(getCommandLoc(), err);
    }

    return true;
}

bool ComposerClient::CommandReader::parseValidateDisplay(uint16_t length)
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

bool ComposerClient::CommandReader::parsePresentOrValidateDisplay(uint16_t length)
{
    if (length != CommandWriterBase::kPresentOrValidateDisplayLength) {
        return false;
    }

    // First try to Present as is.
    if (mHal.hasCapability(HWC2_CAPABILITY_SKIP_VALIDATE)) {
        int presentFence = -1;
        std::vector<Layer> layers;
        std::vector<int> fences;
        auto err = mHal.presentDisplay(mDisplay, &presentFence, &layers, &fences);
        if (err == Error::NONE) {
            mWriter.setPresentOrValidateResult(1);
            mWriter.setPresentFence(presentFence);
            mWriter.setReleaseFences(layers, fences);
            return true;
        }
    }

    // Present has failed. We need to fallback to validate
    std::vector<Layer> changedLayers;
    std::vector<IComposerClient::Composition> compositionTypes;
    uint32_t displayRequestMask = 0x0;
    std::vector<Layer> requestedLayers;
    std::vector<uint32_t> requestMasks;

    auto err = mHal.validateDisplay(mDisplay, &changedLayers, &compositionTypes,
                                    &displayRequestMask, &requestedLayers, &requestMasks);
    if (err == Error::NONE) {
        mWriter.setPresentOrValidateResult(0);
        mWriter.setChangedCompositionTypes(changedLayers,
                                           compositionTypes);
        mWriter.setDisplayRequests(displayRequestMask,
                                   requestedLayers, requestMasks);
    } else {
        mWriter.setError(getCommandLoc(), err);
    }

    return true;
}

bool ComposerClient::CommandReader::parseAcceptDisplayChanges(uint16_t length)
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

bool ComposerClient::CommandReader::parsePresentDisplay(uint16_t length)
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

bool ComposerClient::CommandReader::parseSetLayerCursorPosition(uint16_t length)
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

bool ComposerClient::CommandReader::parseSetLayerBuffer(uint16_t length)
{
    if (length != CommandWriterBase::kSetLayerBufferLength) {
        return false;
    }

    bool useCache = false;
    auto slot = read();
    auto rawHandle = readHandle(&useCache);
    auto fence = readFence();
    bool closeFence = true;

    const native_handle_t* buffer;
    ComposerResources::ReplacedBufferHandle replacedBuffer;
    auto err = mResources->getLayerBuffer(mDisplay, mLayer,
            slot, useCache, rawHandle, &buffer, &replacedBuffer);
    if (err == Error::NONE) {
        err = mHal.setLayerBuffer(mDisplay, mLayer, buffer, fence);
        if (err == Error::NONE) {
            closeFence = false;
        }
    }
    if (closeFence) {
        close(fence);
    }
    if (err != Error::NONE) {
        mWriter.setError(getCommandLoc(), err);
    }

    return true;
}

bool ComposerClient::CommandReader::parseSetLayerSurfaceDamage(uint16_t length)
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

bool ComposerClient::CommandReader::parseSetLayerBlendMode(uint16_t length)
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

bool ComposerClient::CommandReader::parseSetLayerColor(uint16_t length)
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

bool ComposerClient::CommandReader::parseSetLayerCompositionType(
        uint16_t length)
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

bool ComposerClient::CommandReader::parseSetLayerDataspace(uint16_t length)
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

bool ComposerClient::CommandReader::parseSetLayerDisplayFrame(uint16_t length)
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

bool ComposerClient::CommandReader::parseSetLayerPlaneAlpha(uint16_t length)
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

bool ComposerClient::CommandReader::parseSetLayerSidebandStream(uint16_t length)
{
    if (length != CommandWriterBase::kSetLayerSidebandStreamLength) {
        return false;
    }

    auto rawHandle = readHandle();

    const native_handle_t* stream;
    ComposerResources::ReplacedStreamHandle replacedStream;
    auto err = mResources->getLayerSidebandStream(mDisplay, mLayer,
            rawHandle, &stream, &replacedStream);
    if (err == Error::NONE) {
        err = mHal.setLayerSidebandStream(mDisplay, mLayer, stream);
    }
    if (err != Error::NONE) {
        mWriter.setError(getCommandLoc(), err);
    }

    return true;
}

bool ComposerClient::CommandReader::parseSetLayerSourceCrop(uint16_t length)
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

bool ComposerClient::CommandReader::parseSetLayerTransform(uint16_t length)
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

bool ComposerClient::CommandReader::parseSetLayerVisibleRegion(uint16_t length)
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

bool ComposerClient::CommandReader::parseSetLayerZOrder(uint16_t length)
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

hwc_rect_t ComposerClient::CommandReader::readRect()
{
    return hwc_rect_t{
        readSigned(),
        readSigned(),
        readSigned(),
        readSigned(),
    };
}

std::vector<hwc_rect_t> ComposerClient::CommandReader::readRegion(size_t count)
{
    std::vector<hwc_rect_t> region;
    region.reserve(count);
    while (count > 0) {
        region.emplace_back(readRect());
        count--;
    }

    return region;
}

hwc_frect_t ComposerClient::CommandReader::readFRect()
{
    return hwc_frect_t{
        readFloat(),
        readFloat(),
        readFloat(),
        readFloat(),
    };
}

} // namespace implementation
} // namespace V2_1
} // namespace composer
} // namespace graphics
} // namespace hardware
} // namespace android
