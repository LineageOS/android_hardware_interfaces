/*
 * Copyright 2021 The Android Open Source Project
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

#include <algorithm>
#include <limits>
#include <memory>
#include <vector>

#include <inttypes.h>
#include <string.h>

#include <aidl/android/hardware/graphics/common/BlendMode.h>
#include <aidl/android/hardware/graphics/composer3/Color.h>
#include <aidl/android/hardware/graphics/composer3/Composition.h>
#include <aidl/android/hardware/graphics/composer3/DisplayBrightness.h>
#include <aidl/android/hardware/graphics/composer3/LayerBrightness.h>
#include <aidl/android/hardware/graphics/composer3/LayerLifecycleBatchCommandType.h>
#include <aidl/android/hardware/graphics/composer3/PerFrameMetadata.h>
#include <aidl/android/hardware/graphics/composer3/PerFrameMetadataBlob.h>

#include <aidl/android/hardware/graphics/composer3/DisplayCommand.h>

#include <aidl/android/hardware/graphics/common/ColorTransform.h>
#include <aidl/android/hardware/graphics/common/FRect.h>
#include <aidl/android/hardware/graphics/common/Rect.h>
#include <aidl/android/hardware/graphics/common/Transform.h>

#include <log/log.h>
#include <sync/sync.h>

#include <aidlcommonsupport/NativeHandle.h>

using aidl::android::hardware::graphics::common::BlendMode;
using aidl::android::hardware::graphics::common::ColorTransform;
using aidl::android::hardware::graphics::common::Dataspace;
using aidl::android::hardware::graphics::common::FRect;
using aidl::android::hardware::graphics::common::Rect;
using aidl::android::hardware::graphics::common::Transform;

using namespace aidl::android::hardware::graphics::composer3;

using aidl::android::hardware::common::NativeHandle;

namespace aidl::android::hardware::graphics::composer3 {

class ComposerClientWriter final {
  public:
    static constexpr std::optional<ClockMonotonicTimestamp> kNoTimestamp = std::nullopt;

    explicit ComposerClientWriter(int64_t display) : mDisplay(display) { reset(); }

    ~ComposerClientWriter() { reset(); }

    ComposerClientWriter(ComposerClientWriter&&) = default;

    ComposerClientWriter(const ComposerClientWriter&) = delete;
    ComposerClientWriter& operator=(const ComposerClientWriter&) = delete;

    void setColorTransform(int64_t display, const float* matrix) {
        std::vector<float> matVec;
        matVec.reserve(16);
        matVec.assign(matrix, matrix + 16);
        getDisplayCommand(display).colorTransformMatrix.emplace(std::move(matVec));
    }

    void setDisplayBrightness(int64_t display, float brightness, float brightnessNits) {
        getDisplayCommand(display).brightness.emplace(
                DisplayBrightness{.brightness = brightness, .brightnessNits = brightnessNits});
    }

    void setClientTarget(int64_t display, uint32_t slot, const native_handle_t* target,
                         int acquireFence, Dataspace dataspace, const std::vector<Rect>& damage,
                         float hdrSdrRatio) {
        ClientTarget clientTargetCommand;
        clientTargetCommand.buffer = getBufferCommand(slot, target, acquireFence);
        clientTargetCommand.dataspace = dataspace;
        clientTargetCommand.damage.assign(damage.begin(), damage.end());
        clientTargetCommand.hdrSdrRatio = hdrSdrRatio;
        getDisplayCommand(display).clientTarget.emplace(std::move(clientTargetCommand));
    }

    void setOutputBuffer(int64_t display, uint32_t slot, const native_handle_t* buffer,
                         int releaseFence) {
        getDisplayCommand(display).virtualDisplayOutputBuffer.emplace(
                getBufferCommand(slot, buffer, releaseFence));
    }

    void setLayerLifecycleBatchCommandType(int64_t display, int64_t layer,
                                           LayerLifecycleBatchCommandType cmd) {
        getLayerCommand(display, layer).layerLifecycleBatchCommandType = cmd;
    }

    void setNewBufferSlotCount(int64_t display, int64_t layer, int32_t newBufferSlotToCount) {
        getLayerCommand(display, layer).newBufferSlotCount = newBufferSlotToCount;
    }

    void validateDisplay(int64_t display,
                         std::optional<ClockMonotonicTimestamp> expectedPresentTime,
                         int32_t frameIntervalNs) {
        auto& command = getDisplayCommand(display);
        command.expectedPresentTime = expectedPresentTime;
        command.validateDisplay = true;
        command.frameIntervalNs = frameIntervalNs;
    }

    void presentOrvalidateDisplay(int64_t display,
                                  std::optional<ClockMonotonicTimestamp> expectedPresentTime,
                                  int32_t frameIntervalNs) {
        auto& command = getDisplayCommand(display);
        command.expectedPresentTime = expectedPresentTime;
        command.presentOrValidateDisplay = true;
        command.frameIntervalNs = frameIntervalNs;
    }

    void acceptDisplayChanges(int64_t display) {
        getDisplayCommand(display).acceptDisplayChanges = true;
    }

    void presentDisplay(int64_t display) { getDisplayCommand(display).presentDisplay = true; }

    void setLayerCursorPosition(int64_t display, int64_t layer, int32_t x, int32_t y) {
        common::Point cursorPosition;
        cursorPosition.x = x;
        cursorPosition.y = y;
        getLayerCommand(display, layer).cursorPosition.emplace(std::move(cursorPosition));
    }

    void setLayerBuffer(int64_t display, int64_t layer, uint32_t slot,
                        const native_handle_t* buffer, int acquireFence) {
        getLayerCommand(display, layer).buffer = getBufferCommand(slot, buffer, acquireFence);
    }

    void setLayerBufferWithNewCommand(int64_t display, int64_t layer, uint32_t slot,
                                      const native_handle_t* buffer, int acquireFence) {
        flushLayerCommand();
        getLayerCommand(display, layer).buffer = getBufferCommand(slot, buffer, acquireFence);
        flushLayerCommand();
    }

    void setLayerBufferSlotsToClear(int64_t display, int64_t layer,
                                    const std::vector<uint32_t>& slotsToClear) {
        getLayerCommand(display, layer)
                .bufferSlotsToClear.emplace(slotsToClear.begin(), slotsToClear.end());
    }

    void setLayerSurfaceDamage(int64_t display, int64_t layer, const std::vector<Rect>& damage) {
        getLayerCommand(display, layer).damage.emplace(damage.begin(), damage.end());
    }

    void setLayerBlendMode(int64_t display, int64_t layer, BlendMode mode) {
        ParcelableBlendMode parcelableBlendMode;
        parcelableBlendMode.blendMode = mode;
        getLayerCommand(display, layer).blendMode.emplace(std::move(parcelableBlendMode));
    }

    void setLayerColor(int64_t display, int64_t layer, Color color) {
        getLayerCommand(display, layer).color.emplace(std::move(color));
    }

    void setLayerCompositionType(int64_t display, int64_t layer, Composition type) {
        ParcelableComposition compositionPayload;
        compositionPayload.composition = type;
        getLayerCommand(display, layer).composition.emplace(std::move(compositionPayload));
    }

    void setLayerDataspace(int64_t display, int64_t layer, Dataspace dataspace) {
        ParcelableDataspace dataspacePayload;
        dataspacePayload.dataspace = dataspace;
        getLayerCommand(display, layer).dataspace.emplace(std::move(dataspacePayload));
    }

    void setLayerDisplayFrame(int64_t display, int64_t layer, const Rect& frame) {
        getLayerCommand(display, layer).displayFrame.emplace(frame);
    }

    void setLayerPlaneAlpha(int64_t display, int64_t layer, float alpha) {
        PlaneAlpha planeAlpha;
        planeAlpha.alpha = alpha;
        getLayerCommand(display, layer).planeAlpha.emplace(std::move(planeAlpha));
    }

    void setLayerSidebandStream(int64_t display, int64_t layer, const native_handle_t* stream) {
        NativeHandle handle;
        if (stream) handle = ::android::dupToAidl(stream);
        getLayerCommand(display, layer).sidebandStream.emplace(std::move(handle));
    }

    void setLayerSourceCrop(int64_t display, int64_t layer, const FRect& crop) {
        getLayerCommand(display, layer).sourceCrop.emplace(crop);
    }

    void setLayerTransform(int64_t display, int64_t layer, Transform transform) {
        ParcelableTransform transformPayload;
        transformPayload.transform = transform;
        getLayerCommand(display, layer).transform.emplace(std::move(transformPayload));
    }

    void setLayerVisibleRegion(int64_t display, int64_t layer, const std::vector<Rect>& visible) {
        getLayerCommand(display, layer).visibleRegion.emplace(visible.begin(), visible.end());
    }

    void setLayerZOrder(int64_t display, int64_t layer, uint32_t z) {
        ZOrder zorder;
        zorder.z = static_cast<int32_t>(z);
        getLayerCommand(display, layer).z.emplace(std::move(zorder));
    }

    void setLayerPerFrameMetadata(int64_t display, int64_t layer,
                                  const std::vector<PerFrameMetadata>& metadataVec) {
        getLayerCommand(display, layer)
                .perFrameMetadata.emplace(metadataVec.begin(), metadataVec.end());
    }

    void setLayerColorTransform(int64_t display, int64_t layer, const float* matrix) {
        getLayerCommand(display, layer).colorTransform.emplace(matrix, matrix + 16);
    }

    void setLayerPerFrameMetadataBlobs(int64_t display, int64_t layer,
                                       const std::vector<PerFrameMetadataBlob>& metadata) {
        getLayerCommand(display, layer)
                .perFrameMetadataBlob.emplace(metadata.begin(), metadata.end());
    }

    void setLayerBrightness(int64_t display, int64_t layer, float brightness) {
        getLayerCommand(display, layer)
                .brightness.emplace(LayerBrightness{.brightness = brightness});
    }

    void setLayerBlockingRegion(int64_t display, int64_t layer, const std::vector<Rect>& blocking) {
        getLayerCommand(display, layer).blockingRegion.emplace(blocking.begin(), blocking.end());
    }

    std::vector<DisplayCommand> takePendingCommands() {
        flushLayerCommand();
        flushDisplayCommand();
        std::vector<DisplayCommand> moved = std::move(mCommands);
        mCommands.clear();
        return moved;
    }

  private:
    std::optional<DisplayCommand> mDisplayCommand;
    std::optional<LayerCommand> mLayerCommand;
    std::vector<DisplayCommand> mCommands;
    const int64_t mDisplay;

    Buffer getBufferCommand(uint32_t slot, const native_handle_t* bufferHandle, int fence) {
        Buffer bufferCommand;
        bufferCommand.slot = static_cast<int32_t>(slot);
        if (bufferHandle) bufferCommand.handle.emplace(::android::dupToAidl(bufferHandle));
        if (fence > 0) bufferCommand.fence = ::ndk::ScopedFileDescriptor(fence);
        return bufferCommand;
    }

    void flushLayerCommand() {
        if (mLayerCommand.has_value()) {
            mDisplayCommand->layers.emplace_back(std::move(*mLayerCommand));
            mLayerCommand.reset();
        }
    }

    void flushDisplayCommand() {
        if (mDisplayCommand.has_value()) {
            mCommands.emplace_back(std::move(*mDisplayCommand));
            mDisplayCommand.reset();
        }
    }

    DisplayCommand& getDisplayCommand(int64_t display) {
        if (!mDisplayCommand.has_value() || mDisplayCommand->display != display) {
            LOG_ALWAYS_FATAL_IF(display != mDisplay);
            flushLayerCommand();
            flushDisplayCommand();
            mDisplayCommand.emplace();
            mDisplayCommand->display = display;
        }
        return *mDisplayCommand;
    }

    LayerCommand& getLayerCommand(int64_t display, int64_t layer) {
        getDisplayCommand(display);
        if (!mLayerCommand.has_value() || mLayerCommand->layer != layer) {
            flushLayerCommand();
            mLayerCommand.emplace();
            mLayerCommand->layer = layer;
        }
        return *mLayerCommand;
    }

    void reset() {
        mDisplayCommand.reset();
        mLayerCommand.reset();
        mCommands.clear();
    }
};

}  // namespace aidl::android::hardware::graphics::composer3
