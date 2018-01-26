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
    : mHal(hal)
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

    mCommandEngine = createCommandEngine();
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
    std::lock_guard<std::mutex> lock(mCommandEngineMutex);
    return mCommandEngine->setInputMQDescriptor(descriptor) ?
        Error::NONE : Error::NO_RESOURCES;
}

Return<void> ComposerClient::getOutputCommandQueue(
        getOutputCommandQueue_cb hidl_cb)
{
    // no locking as we require this function to be called inside
    // executeCommands_cb

    auto outDescriptor = mCommandEngine->getOutputMQDescriptor();
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
    std::lock_guard<std::mutex> lock(mCommandEngineMutex);

    bool outChanged = false;
    uint32_t outLength = 0;
    hidl_vec<hidl_handle> outHandles;
    Error error = mCommandEngine->execute(inLength, inHandles,
            &outChanged, &outLength, &outHandles);

    hidl_cb(error, outChanged, outLength, outHandles);

    mCommandEngine->reset();

    return Void();
}

std::unique_ptr<ComposerResources> ComposerClient::createResources()
{
    return ComposerResources::create();
}

std::unique_ptr<ComposerCommandEngine>
ComposerClient::createCommandEngine()
{
    return std::make_unique<ComposerCommandEngine>(&mHal, mResources.get());
}

} // namespace implementation
} // namespace V2_1
} // namespace composer
} // namespace graphics
} // namespace hardware
} // namespace android
