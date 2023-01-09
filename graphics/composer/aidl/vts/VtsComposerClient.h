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
#pragma once

#include <aidl/android/hardware/graphics/common/BlendMode.h>
#include <aidl/android/hardware/graphics/common/BufferUsage.h>
#include <aidl/android/hardware/graphics/common/FRect.h>
#include <aidl/android/hardware/graphics/common/Rect.h>
#include <aidl/android/hardware/graphics/composer3/Composition.h>
#include <aidl/android/hardware/graphics/composer3/IComposer.h>
#include <android-base/properties.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <android/hardware/graphics/composer3/ComposerClientReader.h>
#include <binder/ProcessState.h>
#include <gtest/gtest.h>
#include <ui/Fence.h>
#include <ui/GraphicBuffer.h>
#include <ui/PixelFormat.h>
#include <algorithm>
#include <numeric>
#include <string>
#include <thread>
#include <unordered_map>
#include "GraphicsComposerCallback.h"

using aidl::android::hardware::graphics::common::Dataspace;
using aidl::android::hardware::graphics::common::DisplayDecorationSupport;
using aidl::android::hardware::graphics::common::FRect;
using aidl::android::hardware::graphics::common::PixelFormat;
using aidl::android::hardware::graphics::common::Rect;
using namespace ::ndk;

namespace aidl::android::hardware::graphics::composer3::vts {

class VtsDisplay;
/**
 * A wrapper to IComposerClient.
 * This wrapper manages the IComposerClient instance and manages the resources for
 * the tests with respect to the IComposerClient calls.
 */
class VtsComposerClient {
  public:
    VtsComposerClient(const std::string& name);

    ScopedAStatus createClient();

    bool tearDown();

    std::pair<ScopedAStatus, VirtualDisplay> createVirtualDisplay(int32_t width, int32_t height,
                                                                  PixelFormat pixelFormat,
                                                                  int32_t bufferSlotCount);

    ScopedAStatus destroyVirtualDisplay(int64_t display);

    std::pair<ScopedAStatus, int64_t> createLayer(int64_t display, int32_t bufferSlotCount);

    ScopedAStatus destroyLayer(int64_t display, int64_t layer);

    std::pair<ScopedAStatus, int32_t> getActiveConfig(int64_t display);

    ScopedAStatus setActiveConfig(VtsDisplay* vtsDisplay, int32_t config);

    std::pair<ScopedAStatus, int32_t> getDisplayAttribute(int64_t display, int32_t config,
                                                          DisplayAttribute displayAttribute);

    ScopedAStatus setPowerMode(int64_t display, PowerMode powerMode);

    ScopedAStatus setVsync(int64_t display, bool enable);

    void setVsyncAllowed(bool isAllowed);

    std::pair<ScopedAStatus, std::vector<float>> getDataspaceSaturationMatrix(Dataspace dataspace);

    std::pair<ScopedAStatus, std::vector<CommandResultPayload>> executeCommands(
            const std::vector<DisplayCommand>& commands);

    std::optional<VsyncPeriodChangeTimeline> takeLastVsyncPeriodChangeTimeline();

    ScopedAStatus setContentType(int64_t display, ContentType contentType);

    std::pair<ScopedAStatus, VsyncPeriodChangeTimeline> setActiveConfigWithConstraints(
            VtsDisplay* vtsDisplay, int32_t config,
            const VsyncPeriodChangeConstraints& constraints);

    std::pair<ScopedAStatus, std::vector<DisplayCapability>> getDisplayCapabilities(
            int64_t display);

    ScopedAStatus dumpDebugInfo();

    std::pair<ScopedAStatus, DisplayIdentification> getDisplayIdentificationData(int64_t display);

    std::pair<ScopedAStatus, HdrCapabilities> getHdrCapabilities(int64_t display);

    std::pair<ScopedAStatus, std::vector<PerFrameMetadataKey>> getPerFrameMetadataKeys(
            int64_t display);

    std::pair<ScopedAStatus, ReadbackBufferAttributes> getReadbackBufferAttributes(int64_t display);

    ScopedAStatus setReadbackBuffer(int64_t display, const native_handle_t* buffer,
                                    const ScopedFileDescriptor& releaseFence);

    std::pair<ScopedAStatus, ScopedFileDescriptor> getReadbackBufferFence(int64_t display);

    std::pair<ScopedAStatus, std::vector<ColorMode>> getColorModes(int64_t display);

    std::pair<ScopedAStatus, std::vector<RenderIntent>> getRenderIntents(int64_t display,
                                                                         ColorMode colorMode);

    ScopedAStatus setColorMode(int64_t display, ColorMode colorMode, RenderIntent renderIntent);

    std::pair<ScopedAStatus, DisplayContentSamplingAttributes>
    getDisplayedContentSamplingAttributes(int64_t display);

    ScopedAStatus setDisplayedContentSamplingEnabled(int64_t display, bool isEnabled,
                                                     FormatColorComponent formatColorComponent,
                                                     int64_t maxFrames);

    std::pair<ScopedAStatus, DisplayContentSample> getDisplayedContentSample(int64_t display,
                                                                             int64_t maxFrames,
                                                                             int64_t timestamp);

    std::pair<ScopedAStatus, DisplayConnectionType> getDisplayConnectionType(int64_t display);

    std::pair<ScopedAStatus, std::vector<int32_t>> getDisplayConfigs(int64_t display);

    std::pair<ScopedAStatus, int32_t> getDisplayVsyncPeriod(int64_t display);

    ScopedAStatus setAutoLowLatencyMode(int64_t display, bool isEnabled);

    std::pair<ScopedAStatus, std::vector<ContentType>> getSupportedContentTypes(int64_t display);

    std::pair<ScopedAStatus, std::optional<DisplayDecorationSupport>> getDisplayDecorationSupport(
            int64_t display);

    std::pair<ScopedAStatus, int32_t> getMaxVirtualDisplayCount();

    std::pair<ScopedAStatus, std::string> getDisplayName(int64_t display);

    ScopedAStatus setClientTargetSlotCount(int64_t display, int32_t bufferSlotCount);

    std::pair<ScopedAStatus, std::vector<Capability>> getCapabilities();

    ScopedAStatus setBootDisplayConfig(int64_t display, int32_t config);

    ScopedAStatus clearBootDisplayConfig(int64_t display);

    std::pair<ScopedAStatus, int32_t> getPreferredBootDisplayConfig(int64_t display);

    std::pair<ScopedAStatus, common::Transform> getDisplayPhysicalOrientation(int64_t display);

    ScopedAStatus setIdleTimerEnabled(int64_t display, int32_t timeoutMs);

    int32_t getVsyncIdleCount();

    int64_t getVsyncIdleTime();

    int64_t getInvalidDisplayId();

    std::pair<ScopedAStatus, std::vector<VtsDisplay>> getDisplays();

  private:
    ScopedAStatus addDisplayConfig(VtsDisplay* vtsDisplay, int32_t config);
    ScopedAStatus updateDisplayProperties(VtsDisplay* vtsDisplay, int32_t config);

    ScopedAStatus addDisplayToDisplayResources(int64_t display, bool isVirtual);

    ScopedAStatus addLayerToDisplayResources(int64_t display, int64_t layer);

    void removeLayerFromDisplayResources(int64_t display, int64_t layer);

    bool destroyAllLayers();

    bool verifyComposerCallbackParams();

    // Keep track of displays and layers. When a test fails/ends,
    // the VtsComposerClient::tearDown should be called from the
    // test tearDown to clean up the resources for the test.
    struct DisplayResource {
        DisplayResource(bool isVirtual_) : isVirtual(isVirtual_) {}

        bool isVirtual;
        std::unordered_set<int64_t> layers;
    };

    std::shared_ptr<IComposer> mComposer;
    std::shared_ptr<IComposerClient> mComposerClient;
    std::shared_ptr<GraphicsComposerCallback> mComposerCallback;
    std::unordered_map<int64_t, DisplayResource> mDisplayResources;
};

class VtsDisplay {
  public:
    VtsDisplay(int64_t displayId) : mDisplayId(displayId), mDisplayWidth(0), mDisplayHeight(0) {}

    int64_t getDisplayId() const { return mDisplayId; }

    FRect getCrop() const {
        return {0, 0, static_cast<float>(mDisplayWidth), static_cast<float>(mDisplayHeight)};
    }

    Rect getFrameRect() const { return {0, 0, mDisplayWidth, mDisplayHeight}; }

    void setDimensions(int32_t displayWidth, int32_t displayHeight) {
        mDisplayWidth = displayWidth;
        mDisplayHeight = displayHeight;
    }

    int32_t getDisplayWidth() const { return mDisplayWidth; }

    int32_t getDisplayHeight() const { return mDisplayHeight; }

    struct DisplayConfig {
        DisplayConfig(int32_t vsyncPeriod_, int32_t configGroup_)
            : vsyncPeriod(vsyncPeriod_), configGroup(configGroup_) {}
        int32_t vsyncPeriod;
        int32_t configGroup;
    };

    void addDisplayConfig(int32_t config, DisplayConfig displayConfig) {
        displayConfigs.insert({config, displayConfig});
    }

    DisplayConfig getDisplayConfig(int32_t config) { return displayConfigs.find(config)->second; }

  private:
    int64_t mDisplayId;
    int32_t mDisplayWidth;
    int32_t mDisplayHeight;
    std::unordered_map<int32_t, DisplayConfig> displayConfigs;
};
}  // namespace aidl::android::hardware::graphics::composer3::vts
