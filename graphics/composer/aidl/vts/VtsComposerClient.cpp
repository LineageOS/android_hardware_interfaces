/**
 * Copyright (c) 2022, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "VtsComposerClient.h"
#include <aidlcommonsupport/NativeHandle.h>
#include <android-base/logging.h>
#include <log/log_main.h>

#undef LOG_TAG
#define LOG_TAG "VtsComposerClient"

using namespace std::chrono_literals;

namespace aidl::android::hardware::graphics::composer3::vts {

VtsComposerClient::VtsComposerClient(const std::string& name) {
    SpAIBinder binder(AServiceManager_waitForService(name.c_str()));
    ALOGE_IF(binder == nullptr, "Could not initialize the service binder");
    if (binder != nullptr) {
        mComposer = IComposer::fromBinder(binder);
        ALOGE_IF(mComposer == nullptr, "Failed to acquire the composer from the binder");
    }
}

ScopedAStatus VtsComposerClient::createClient() {
    if (mComposer == nullptr) {
        ALOGE("IComposer not initialized");
        return ScopedAStatus::fromServiceSpecificError(IComposerClient::INVALID_CONFIGURATION);
    }
    auto status = mComposer->createClient(&mComposerClient);
    if (!status.isOk() || mComposerClient == nullptr) {
        ALOGE("Failed to create client for IComposerClient with %s",
              status.getDescription().c_str());
        return status;
    }
    mComposerCallback = SharedRefBase::make<GraphicsComposerCallback>();
    if (mComposerCallback == nullptr) {
        ALOGE("Unable to create ComposerCallback");
        return ScopedAStatus::fromServiceSpecificError(IComposerClient::INVALID_CONFIGURATION);
    }
    return mComposerClient->registerCallback(mComposerCallback);
}

bool VtsComposerClient::tearDown() {
    return verifyComposerCallbackParams() && destroyAllLayers();
}

std::pair<ScopedAStatus, VirtualDisplay> VtsComposerClient::createVirtualDisplay(
        int32_t width, int32_t height, PixelFormat pixelFormat, int32_t bufferSlotCount) {
    VirtualDisplay outVirtualDisplay;
    auto status = mComposerClient->createVirtualDisplay(width, height, pixelFormat, bufferSlotCount,
                                                        &outVirtualDisplay);
    if (!status.isOk()) {
        return {std::move(status), outVirtualDisplay};
    }
    return {addDisplayToDisplayResources(outVirtualDisplay.display, /*isVirtual*/ true),
            outVirtualDisplay};
}

ScopedAStatus VtsComposerClient::destroyVirtualDisplay(int64_t display) {
    auto status = mComposerClient->destroyVirtualDisplay(display);
    if (!status.isOk()) {
        return status;
    }
    mDisplayResources.erase(display);
    return status;
}

std::pair<ScopedAStatus, int64_t> VtsComposerClient::createLayer(int64_t display,
                                                                 int32_t bufferSlotCount) {
    int64_t outLayer;
    auto status = mComposerClient->createLayer(display, bufferSlotCount, &outLayer);

    if (!status.isOk()) {
        return {std::move(status), outLayer};
    }
    return {addLayerToDisplayResources(display, outLayer), outLayer};
}

ScopedAStatus VtsComposerClient::destroyLayer(int64_t display, int64_t layer) {
    auto status = mComposerClient->destroyLayer(display, layer);

    if (!status.isOk()) {
        return status;
    }
    removeLayerFromDisplayResources(display, layer);
    return status;
}

std::pair<ScopedAStatus, int32_t> VtsComposerClient::getActiveConfig(int64_t display) {
    int32_t outConfig;
    return {mComposerClient->getActiveConfig(display, &outConfig), outConfig};
}

ScopedAStatus VtsComposerClient::setActiveConfig(VtsDisplay* vtsDisplay, int32_t config) {
    auto status = mComposerClient->setActiveConfig(vtsDisplay->getDisplayId(), config);
    if (!status.isOk()) {
        return status;
    }
    return updateDisplayProperties(vtsDisplay, config);
}

std::pair<ScopedAStatus, int32_t> VtsComposerClient::getDisplayAttribute(
        int64_t display, int32_t config, DisplayAttribute displayAttribute) {
    int32_t outDisplayAttribute;
    return {mComposerClient->getDisplayAttribute(display, config, displayAttribute,
                                                 &outDisplayAttribute),
            outDisplayAttribute};
}

ScopedAStatus VtsComposerClient::setPowerMode(int64_t display, PowerMode powerMode) {
    return mComposerClient->setPowerMode(display, powerMode);
}

ScopedAStatus VtsComposerClient::setVsync(int64_t display, bool enable) {
    return mComposerClient->setVsyncEnabled(display, enable);
}

void VtsComposerClient::setVsyncAllowed(bool isAllowed) {
    mComposerCallback->setVsyncAllowed(isAllowed);
}

std::pair<ScopedAStatus, std::vector<float>> VtsComposerClient::getDataspaceSaturationMatrix(
        Dataspace dataspace) {
    std::vector<float> outMatrix;
    return {mComposerClient->getDataspaceSaturationMatrix(dataspace, &outMatrix), outMatrix};
}

std::pair<ScopedAStatus, std::vector<CommandResultPayload>> VtsComposerClient::executeCommands(
        const std::vector<DisplayCommand>& commands) {
    std::vector<CommandResultPayload> outResultPayload;
    return {mComposerClient->executeCommands(commands, &outResultPayload),
            std::move(outResultPayload)};
}

std::optional<VsyncPeriodChangeTimeline> VtsComposerClient::takeLastVsyncPeriodChangeTimeline() {
    return mComposerCallback->takeLastVsyncPeriodChangeTimeline();
}

ScopedAStatus VtsComposerClient::setContentType(int64_t display, ContentType contentType) {
    return mComposerClient->setContentType(display, contentType);
}

std::pair<ScopedAStatus, VsyncPeriodChangeTimeline>
VtsComposerClient::setActiveConfigWithConstraints(VtsDisplay* vtsDisplay, int32_t config,
                                                  const VsyncPeriodChangeConstraints& constraints) {
    VsyncPeriodChangeTimeline outTimeline;
    auto status = mComposerClient->setActiveConfigWithConstraints(
            vtsDisplay->getDisplayId(), config, constraints, &outTimeline);
    if (!status.isOk()) {
        return {std::move(status), outTimeline};
    }
    return {updateDisplayProperties(vtsDisplay, config), outTimeline};
}

std::pair<ScopedAStatus, std::vector<DisplayCapability>> VtsComposerClient::getDisplayCapabilities(
        int64_t display) {
    std::vector<DisplayCapability> outCapabilities;
    return {mComposerClient->getDisplayCapabilities(display, &outCapabilities), outCapabilities};
}

ScopedAStatus VtsComposerClient::dumpDebugInfo() {
    int pipefds[2];
    if (pipe(pipefds) < 0) {
        return ScopedAStatus::fromServiceSpecificError(IComposer::EX_NO_RESOURCES);
    }

    const auto status = mComposer->dump(pipefds[1], /*args*/ nullptr, /*numArgs*/ 0);
    close(pipefds[0]);
    close(pipefds[1]);
    return ScopedAStatus::fromStatus(status);
}

std::pair<ScopedAStatus, DisplayIdentification> VtsComposerClient::getDisplayIdentificationData(
        int64_t display) {
    DisplayIdentification outDisplayIdentification;
    return {mComposerClient->getDisplayIdentificationData(display, &outDisplayIdentification),
            outDisplayIdentification};
}

std::pair<ScopedAStatus, HdrCapabilities> VtsComposerClient::getHdrCapabilities(int64_t display) {
    HdrCapabilities outHdrCapabilities;
    return {mComposerClient->getHdrCapabilities(display, &outHdrCapabilities), outHdrCapabilities};
}

std::pair<ScopedAStatus, std::vector<PerFrameMetadataKey>>
VtsComposerClient::getPerFrameMetadataKeys(int64_t display) {
    std::vector<PerFrameMetadataKey> outPerFrameMetadataKeys;
    return {mComposerClient->getPerFrameMetadataKeys(display, &outPerFrameMetadataKeys),
            outPerFrameMetadataKeys};
}

std::pair<ScopedAStatus, ReadbackBufferAttributes> VtsComposerClient::getReadbackBufferAttributes(
        int64_t display) {
    ReadbackBufferAttributes outReadbackBufferAttributes;
    return {mComposerClient->getReadbackBufferAttributes(display, &outReadbackBufferAttributes),
            outReadbackBufferAttributes};
}

ScopedAStatus VtsComposerClient::setReadbackBuffer(int64_t display, const native_handle_t* buffer,
                                                   const ScopedFileDescriptor& releaseFence) {
    return mComposerClient->setReadbackBuffer(display, ::android::dupToAidl(buffer), releaseFence);
}

std::pair<ScopedAStatus, ScopedFileDescriptor> VtsComposerClient::getReadbackBufferFence(
        int64_t display) {
    ScopedFileDescriptor outReleaseFence;
    return {mComposerClient->getReadbackBufferFence(display, &outReleaseFence),
            std::move(outReleaseFence)};
}

std::pair<ScopedAStatus, std::vector<ColorMode>> VtsComposerClient::getColorModes(int64_t display) {
    std::vector<ColorMode> outColorModes;
    return {mComposerClient->getColorModes(display, &outColorModes), outColorModes};
}

std::pair<ScopedAStatus, std::vector<RenderIntent>> VtsComposerClient::getRenderIntents(
        int64_t display, ColorMode colorMode) {
    std::vector<RenderIntent> outRenderIntents;
    return {mComposerClient->getRenderIntents(display, colorMode, &outRenderIntents),
            outRenderIntents};
}

ScopedAStatus VtsComposerClient::setColorMode(int64_t display, ColorMode colorMode,
                                              RenderIntent renderIntent) {
    return mComposerClient->setColorMode(display, colorMode, renderIntent);
}

std::pair<ScopedAStatus, DisplayContentSamplingAttributes>
VtsComposerClient::getDisplayedContentSamplingAttributes(int64_t display) {
    DisplayContentSamplingAttributes outAttributes;
    return {mComposerClient->getDisplayedContentSamplingAttributes(display, &outAttributes),
            outAttributes};
}

ScopedAStatus VtsComposerClient::setDisplayedContentSamplingEnabled(
        int64_t display, bool isEnabled, FormatColorComponent formatColorComponent,
        int64_t maxFrames) {
    return mComposerClient->setDisplayedContentSamplingEnabled(display, isEnabled,
                                                               formatColorComponent, maxFrames);
}

std::pair<ScopedAStatus, DisplayContentSample> VtsComposerClient::getDisplayedContentSample(
        int64_t display, int64_t maxFrames, int64_t timestamp) {
    DisplayContentSample outDisplayContentSample;
    return {mComposerClient->getDisplayedContentSample(display, maxFrames, timestamp,
                                                       &outDisplayContentSample),
            outDisplayContentSample};
}

std::pair<ScopedAStatus, DisplayConnectionType> VtsComposerClient::getDisplayConnectionType(
        int64_t display) {
    DisplayConnectionType outDisplayConnectionType;
    return {mComposerClient->getDisplayConnectionType(display, &outDisplayConnectionType),
            outDisplayConnectionType};
}

std::pair<ScopedAStatus, std::vector<int32_t>> VtsComposerClient::getDisplayConfigs(
        int64_t display) {
    std::vector<int32_t> outConfigs;
    return {mComposerClient->getDisplayConfigs(display, &outConfigs), outConfigs};
}

std::pair<ScopedAStatus, int32_t> VtsComposerClient::getDisplayVsyncPeriod(int64_t display) {
    int32_t outVsyncPeriodNanos;
    return {mComposerClient->getDisplayVsyncPeriod(display, &outVsyncPeriodNanos),
            outVsyncPeriodNanos};
}

ScopedAStatus VtsComposerClient::setAutoLowLatencyMode(int64_t display, bool isEnabled) {
    return mComposerClient->setAutoLowLatencyMode(display, isEnabled);
}

std::pair<ScopedAStatus, std::vector<ContentType>> VtsComposerClient::getSupportedContentTypes(
        int64_t display) {
    std::vector<ContentType> outContentTypes;
    return {mComposerClient->getSupportedContentTypes(display, &outContentTypes), outContentTypes};
}

std::pair<ScopedAStatus, std::optional<DisplayDecorationSupport>>
VtsComposerClient::getDisplayDecorationSupport(int64_t display) {
    std::optional<DisplayDecorationSupport> outSupport;
    return {mComposerClient->getDisplayDecorationSupport(display, &outSupport), outSupport};
}

std::pair<ScopedAStatus, int32_t> VtsComposerClient::getMaxVirtualDisplayCount() {
    int32_t outMaxVirtualDisplayCount;
    return {mComposerClient->getMaxVirtualDisplayCount(&outMaxVirtualDisplayCount),
            outMaxVirtualDisplayCount};
}

std::pair<ScopedAStatus, std::string> VtsComposerClient::getDisplayName(int64_t display) {
    std::string outDisplayName;
    return {mComposerClient->getDisplayName(display, &outDisplayName), outDisplayName};
}

ScopedAStatus VtsComposerClient::setClientTargetSlotCount(int64_t display,
                                                          int32_t bufferSlotCount) {
    return mComposerClient->setClientTargetSlotCount(display, bufferSlotCount);
}

std::pair<ScopedAStatus, std::vector<Capability>> VtsComposerClient::getCapabilities() {
    std::vector<Capability> outCapabilities;
    return {mComposer->getCapabilities(&outCapabilities), outCapabilities};
}

ScopedAStatus VtsComposerClient::setBootDisplayConfig(int64_t display, int32_t config) {
    return mComposerClient->setBootDisplayConfig(display, config);
}

ScopedAStatus VtsComposerClient::clearBootDisplayConfig(int64_t display) {
    return mComposerClient->clearBootDisplayConfig(display);
}

std::pair<ScopedAStatus, int32_t> VtsComposerClient::getPreferredBootDisplayConfig(
        int64_t display) {
    int32_t outConfig;
    return {mComposerClient->getPreferredBootDisplayConfig(display, &outConfig), outConfig};
}

std::pair<ScopedAStatus, common::Transform> VtsComposerClient::getDisplayPhysicalOrientation(
        int64_t display) {
    common::Transform outDisplayOrientation;
    return {mComposerClient->getDisplayPhysicalOrientation(display, &outDisplayOrientation),
            outDisplayOrientation};
}

ScopedAStatus VtsComposerClient::setIdleTimerEnabled(int64_t display, int32_t timeoutMs) {
    return mComposerClient->setIdleTimerEnabled(display, timeoutMs);
}

int32_t VtsComposerClient::getVsyncIdleCount() {
    return mComposerCallback->getVsyncIdleCount();
}

int64_t VtsComposerClient::getVsyncIdleTime() {
    return mComposerCallback->getVsyncIdleTime();
}

int64_t VtsComposerClient::getInvalidDisplayId() {
    // returns an invalid display id (one that has not been registered to a
    // display. Currently assuming that a device will never have close to
    // std::numeric_limit<uint64_t>::max() displays registered while running tests
    int64_t id = std::numeric_limits<int64_t>::max();
    std::vector<int64_t> displays = mComposerCallback->getDisplays();
    while (id > 0) {
        if (std::none_of(displays.begin(), displays.end(),
                         [id](const auto& display) { return id == display; })) {
            return id;
        }
        id--;
    }

    // Although 0 could be an invalid display, a return value of 0
    // from getInvalidDisplayId means all other ids are in use, a condition which
    // we are assuming a device will never have
    EXPECT_NE(0, id);
    return id;
}

std::pair<ScopedAStatus, std::vector<VtsDisplay>> VtsComposerClient::getDisplays() {
    while (true) {
        // Sleep for a small period of time to allow all built-in displays
        // to post hotplug events
        std::this_thread::sleep_for(5ms);
        std::vector<int64_t> displays = mComposerCallback->getDisplays();
        if (displays.empty()) {
            continue;
        }

        std::vector<VtsDisplay> vtsDisplays;
        vtsDisplays.reserve(displays.size());
        for (int64_t display : displays) {
            auto vtsDisplay = VtsDisplay{display};
            auto configs = getDisplayConfigs(display);
            if (!configs.first.isOk()) {
                ALOGE("Unable to get the displays for test, failed to get the configs "
                      "for display %" PRId64,
                      display);
                return {std::move(configs.first), vtsDisplays};
            }
            for (int config : configs.second) {
                auto status = addDisplayConfig(&vtsDisplay, config);
                if (!status.isOk()) {
                    ALOGE("Unable to get the displays for test, failed to add config "
                          "for display %" PRId64,
                          display);
                    return {std::move(status), vtsDisplays};
                }
            }

            auto config = getActiveConfig(display);
            if (!config.first.isOk()) {
                ALOGE("Unable to get the displays for test, failed to get active config "
                      "for display %" PRId64, display);
                return {std::move(config.first), vtsDisplays};
            }

            auto status = updateDisplayProperties(&vtsDisplay, config.second);
            if (!status.isOk()) {
                ALOGE("Unable to get the displays for test, "
                      "failed to update the properties "
                      "for display %" PRId64,
                      display);
                return {std::move(status), vtsDisplays};
            }

            vtsDisplays.emplace_back(vtsDisplay);
            addDisplayToDisplayResources(display, /*isVirtual*/ false);
        }

        return {ScopedAStatus::ok(), vtsDisplays};
    }
}

ScopedAStatus VtsComposerClient::addDisplayConfig(VtsDisplay* vtsDisplay, int32_t config) {
    const auto width =
            getDisplayAttribute(vtsDisplay->getDisplayId(), config, DisplayAttribute::WIDTH);
    const auto height =
            getDisplayAttribute(vtsDisplay->getDisplayId(), config, DisplayAttribute::HEIGHT);
    const auto vsyncPeriod =
            getDisplayAttribute(vtsDisplay->getDisplayId(), config, DisplayAttribute::VSYNC_PERIOD);
    const auto configGroup =
            getDisplayAttribute(vtsDisplay->getDisplayId(), config, DisplayAttribute::CONFIG_GROUP);
    if (width.first.isOk() && height.first.isOk() && vsyncPeriod.first.isOk() &&
        configGroup.first.isOk()) {
        vtsDisplay->addDisplayConfig(config, {vsyncPeriod.second, configGroup.second});
        return ScopedAStatus::ok();
    }

    LOG(ERROR) << "Failed to update display property for width: " << width.first.isOk()
               << ", height: " << height.first.isOk() << ", vsync: " << vsyncPeriod.first.isOk()
               << ", config: " << configGroup.first.isOk();
    return ScopedAStatus::fromServiceSpecificError(IComposerClient::EX_BAD_CONFIG);
}

ScopedAStatus VtsComposerClient::updateDisplayProperties(VtsDisplay* vtsDisplay, int32_t config) {
    const auto width =
            getDisplayAttribute(vtsDisplay->getDisplayId(), config, DisplayAttribute::WIDTH);
    const auto height =
            getDisplayAttribute(vtsDisplay->getDisplayId(), config, DisplayAttribute::HEIGHT);
    if (width.first.isOk() && height.first.isOk()) {
        vtsDisplay->setDimensions(width.second, height.second);
        return ScopedAStatus::ok();
    }

    LOG(ERROR) << "Failed to update display property for width: " << width.first.isOk()
               << ", height: " << height.first.isOk();
    return ScopedAStatus::fromServiceSpecificError(IComposerClient::EX_BAD_CONFIG);
}

ScopedAStatus VtsComposerClient::addDisplayToDisplayResources(int64_t display, bool isVirtual) {
    if (mDisplayResources.insert({display, DisplayResource(isVirtual)}).second) {
        return ScopedAStatus::ok();
    }

    ALOGE("Duplicate display id %" PRId64, display);
    return ScopedAStatus::fromServiceSpecificError(IComposerClient::EX_BAD_DISPLAY);
}

ScopedAStatus VtsComposerClient::addLayerToDisplayResources(int64_t display, int64_t layer) {
    auto resource = mDisplayResources.find(display);
    if (resource == mDisplayResources.end()) {
        resource = mDisplayResources.insert({display, DisplayResource(false)}).first;
    }

    if (!resource->second.layers.insert(layer).second) {
        ALOGE("Duplicate layer id %" PRId64, layer);
        return ScopedAStatus::fromServiceSpecificError(IComposerClient::EX_BAD_LAYER);
    }
    return ScopedAStatus::ok();
}

void VtsComposerClient::removeLayerFromDisplayResources(int64_t display, int64_t layer) {
    auto resource = mDisplayResources.find(display);
    if (resource != mDisplayResources.end()) {
        resource->second.layers.erase(layer);
    }
}

bool VtsComposerClient::verifyComposerCallbackParams() {
    bool isValid = true;
    if (mComposerCallback != nullptr) {
        if (mComposerCallback->getInvalidHotplugCount() != 0) {
            ALOGE("Invalid hotplug count");
            isValid = false;
        }
        if (mComposerCallback->getInvalidRefreshCount() != 0) {
            ALOGE("Invalid refresh count");
            isValid = false;
        }
        if (mComposerCallback->getInvalidVsyncCount() != 0) {
            ALOGE("Invalid vsync count");
            isValid = false;
        }
        if (mComposerCallback->getInvalidVsyncPeriodChangeCount() != 0) {
            ALOGE("Invalid vsync period change count");
            isValid = false;
        }
        if (mComposerCallback->getInvalidSeamlessPossibleCount() != 0) {
            ALOGE("Invalid seamless possible count");
            isValid = false;
        }
    }
    return isValid;
}

bool VtsComposerClient::destroyAllLayers() {
    std::unordered_map<int64_t, DisplayResource> physicalDisplays;
    while (!mDisplayResources.empty()) {
        const auto& it = mDisplayResources.begin();
        const auto& [display, resource] = *it;

        while (!resource.layers.empty()) {
            auto layer = *resource.layers.begin();
            const auto status = destroyLayer(display, layer);
            if (!status.isOk()) {
                ALOGE("Unable to destroy all the layers, failed at layer %" PRId64 " with error %s",
                      layer, status.getDescription().c_str());
                return false;
            }
        }

        if (resource.isVirtual) {
            const auto status = destroyVirtualDisplay(display);
            if (!status.isOk()) {
                ALOGE("Unable to destroy the display %" PRId64 " failed with error %s", display,
                      status.getDescription().c_str());
                return false;
            }
        } else {
            auto extractIter = mDisplayResources.extract(it);
            physicalDisplays.insert(std::move(extractIter));
        }
    }
    mDisplayResources.swap(physicalDisplays);
    mDisplayResources.clear();
    return true;
}
}  // namespace aidl::android::hardware::graphics::composer3::vts
