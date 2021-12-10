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
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <inttypes.h>
#include <string.h>

#include <aidl/android/hardware/graphics/common/BlendMode.h>
#include <aidl/android/hardware/graphics/composer3/ClientTargetProperty.h>
#include <aidl/android/hardware/graphics/composer3/Color.h>
#include <aidl/android/hardware/graphics/composer3/Composition.h>
#include <aidl/android/hardware/graphics/composer3/FloatColor.h>
#include <aidl/android/hardware/graphics/composer3/HandleIndex.h>
#include <aidl/android/hardware/graphics/composer3/IComposer.h>
#include <aidl/android/hardware/graphics/composer3/IComposerClient.h>
#include <aidl/android/hardware/graphics/composer3/PerFrameMetadata.h>
#include <aidl/android/hardware/graphics/composer3/PerFrameMetadataBlob.h>

#include <aidl/android/hardware/graphics/composer3/command/CommandPayload.h>
#include <aidl/android/hardware/graphics/composer3/command/CommandResultPayload.h>

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

// This class helps build a command queue.  Note that all sizes/lengths are in
// units of uint32_t's.
class CommandWriterBase {
  public:
    CommandWriterBase() { reset(); }

    virtual ~CommandWriterBase() { reset(); }

    void reset() {
        mDisplayCommand.reset();
        mLayerCommand.reset();
        mCommands.clear();
        mCommandsResults.clear();
    }

    void setError(int32_t index, int32_t errorCode) {
        command::Error error;
        error.commandIndex = index;
        error.errorCode = errorCode;
        mCommandsResults.emplace_back(std::move(error));
    }

    void setPresentOrValidateResult(int64_t display, command::PresentOrValidate::Result result) {
        command::PresentOrValidate presentOrValidate;
        presentOrValidate.display = display;
        presentOrValidate.result = result;
        mCommandsResults.emplace_back(std::move(presentOrValidate));
    }

    void setChangedCompositionTypes(int64_t display, const std::vector<int64_t>& layers,
                                    const std::vector<Composition>& types) {
        command::ChangedCompositionTypes changedCompositionTypes;
        changedCompositionTypes.display = display;
        changedCompositionTypes.layers.reserve(layers.size());
        for (int i = 0; i < layers.size(); i++) {
            auto layer = command::ChangedCompositionTypes::Layer{.layer = layers[i],
                                                                 .composition = types[i]};
            changedCompositionTypes.layers.emplace_back(std::move(layer));
        }
        mCommandsResults.emplace_back(std::move(changedCompositionTypes));
    }

    void setDisplayRequests(int64_t display, int32_t displayRequestMask,
                            const std::vector<int64_t>& layers,
                            const std::vector<int32_t>& layerRequestMasks) {
        command::DisplayRequest displayRequest;
        displayRequest.display = display;
        displayRequest.mask = displayRequestMask;
        displayRequest.layerRequests.reserve(layers.size());
        for (int i = 0; i < layers.size(); i++) {
            auto layerRequest = command::DisplayRequest::LayerRequest{.layer = layers[i],
                                                                      .mask = layerRequestMasks[i]};
            displayRequest.layerRequests.emplace_back(std::move(layerRequest));
        }
        mCommandsResults.emplace_back(std::move(displayRequest));
    }

    void setPresentFence(int64_t display, ::ndk::ScopedFileDescriptor presentFence) {
        if (presentFence.get() >= 0) {
            command::PresentFence presentFenceCommand;
            presentFenceCommand.fence = std::move(presentFence);
            presentFenceCommand.display = display;
            mCommandsResults.emplace_back(std::move(presentFenceCommand));
        } else {
            ALOGW("%s: invalid present fence %d", __func__, presentFence.get());
        }
    }

    void setReleaseFences(int64_t display, const std::vector<int64_t>& layers,
                          std::vector<::ndk::ScopedFileDescriptor> releaseFences) {
        command::ReleaseFences releaseFencesCommand;
        releaseFencesCommand.display = display;
        for (int i = 0; i < layers.size(); i++) {
            if (releaseFences[i].get() >= 0) {
                command::ReleaseFences::Layer layer;
                layer.layer = layers[i];
                layer.fence = std::move(releaseFences[i]);
                releaseFencesCommand.layers.emplace_back(std::move(layer));
            } else {
                ALOGW("%s: invalid release fence %d", __func__, releaseFences[i].get());
            }
        }
        mCommandsResults.emplace_back(std::move(releaseFencesCommand));
    }

    void setClientTargetProperty(int64_t display, const ClientTargetProperty& clientTargetProperty,
                                 float whitePointNits) {
        command::ClientTargetPropertyWithNits clientTargetPropertyWithNits;
        clientTargetPropertyWithNits.display = display;
        clientTargetPropertyWithNits.clientTargetProperty = clientTargetProperty;
        clientTargetPropertyWithNits.whitePointNits = whitePointNits;
        mCommandsResults.emplace_back(std::move(clientTargetPropertyWithNits));
    }

    void setColorTransform(int64_t display, const float* matrix, ColorTransform hint) {
        command::ColorTransformPayload colorTransformPayload;
        colorTransformPayload.matrix.assign(matrix, matrix + 16);
        colorTransformPayload.hint = hint;
        getDisplayCommand(display).colorTransform.emplace(std::move(colorTransformPayload));
    }

    void setClientTarget(int64_t display, uint32_t slot, const native_handle_t* target,
                         int acquireFence, Dataspace dataspace, const std::vector<Rect>& damage) {
        command::ClientTarget clientTargetCommand;
        clientTargetCommand.buffer = getBuffer(slot, target, acquireFence);
        clientTargetCommand.dataspace = dataspace;
        clientTargetCommand.damage.assign(damage.begin(), damage.end());
        getDisplayCommand(display).clientTarget.emplace(std::move(clientTargetCommand));
    }

    void setOutputBuffer(int64_t display, uint32_t slot, const native_handle_t* buffer,
                         int releaseFence) {
        getDisplayCommand(display).virtualDisplayOutputBuffer.emplace(
                getBuffer(slot, buffer, releaseFence));
    }

    void validateDisplay(int64_t display) { getDisplayCommand(display).validateDisplay = true; }

    void presentOrvalidateDisplay(int64_t display) {
        getDisplayCommand(display).presentOrValidateDisplay = true;
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
        getLayerCommand(display, layer).buffer = getBuffer(slot, buffer, acquireFence);
    }

    void setLayerSurfaceDamage(int64_t display, int64_t layer, const std::vector<Rect>& damage) {
        getLayerCommand(display, layer).damage.emplace(damage.begin(), damage.end());
    }

    void setLayerBlendMode(int64_t display, int64_t layer, BlendMode mode) {
        command::ParcelableBlendMode parcelableBlendMode;
        parcelableBlendMode.blendMode = mode;
        getLayerCommand(display, layer).blendMode.emplace(std::move(parcelableBlendMode));
    }

    void setLayerColor(int64_t display, int64_t layer, Color color) {
        getLayerCommand(display, layer).color.emplace(std::move(color));
    }

    void setLayerCompositionType(int64_t display, int64_t layer, Composition type) {
        command::ParcelableComposition compositionPayload;
        compositionPayload.composition = type;
        getLayerCommand(display, layer).composition.emplace(std::move(compositionPayload));
    }

    void setLayerDataspace(int64_t display, int64_t layer, Dataspace dataspace) {
        command::ParcelableDataspace dataspacePayload;
        dataspacePayload.dataspace = dataspace;
        getLayerCommand(display, layer).dataspace.emplace(std::move(dataspacePayload));
    }

    void setLayerDisplayFrame(int64_t display, int64_t layer, const Rect& frame) {
        getLayerCommand(display, layer).displayFrame.emplace(frame);
    }

    void setLayerPlaneAlpha(int64_t display, int64_t layer, float alpha) {
        command::PlaneAlpha planeAlpha;
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
        command::ParcelableTransform transformPayload;
        transformPayload.transform = transform;
        getLayerCommand(display, layer).transform.emplace(std::move(transformPayload));
    }

    void setLayerVisibleRegion(int64_t display, int64_t layer, const std::vector<Rect>& visible) {
        getLayerCommand(display, layer).visibleRegion.emplace(visible.begin(), visible.end());
    }

    void setLayerZOrder(int64_t display, int64_t layer, uint32_t z) {
        command::ZOrder zorder;
        zorder.z = z;
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

    void setLayerFloatColor(int64_t display, int64_t layer, FloatColor color) {
        getLayerCommand(display, layer).floatColor.emplace(color);
    }

    void setLayerGenericMetadata(int64_t display, int64_t layer, const std::string& key,
                                 const bool mandatory, const std::vector<uint8_t>& value) {
        command::GenericMetadata metadata;
        metadata.key.name = key;
        metadata.key.mandatory = mandatory;
        metadata.value.assign(value.begin(), value.end());
        getLayerCommand(display, layer).genericMetadata.emplace(std::move(metadata));
    }

    void setLayerWhitePointNits(int64_t display, int64_t layer, float whitePointNits) {
        getLayerCommand(display, layer)
                .whitePointNits.emplace(command::WhitePointNits{.nits = whitePointNits});
    }

    const std::vector<command::CommandPayload>& getPendingCommands() {
        if (mLayerCommand.has_value()) {
            mCommands.emplace_back(std::move(*mLayerCommand));
            mLayerCommand.reset();
        }
        if (mDisplayCommand.has_value()) {
            mCommands.emplace_back(std::move(*mDisplayCommand));
            mDisplayCommand.reset();
        }
        return mCommands;
    }

    std::vector<command::CommandResultPayload> getPendingCommandResults() {
        return std::move(mCommandsResults);
    }

  protected:
    command::Buffer getBuffer(int slot, const native_handle_t* bufferHandle, int fence) {
        command::Buffer bufferCommand;
        bufferCommand.slot = slot;
        if (bufferHandle) bufferCommand.handle.emplace(::android::dupToAidl(bufferHandle));
        if (fence > 0) bufferCommand.fence = ::ndk::ScopedFileDescriptor(fence);
        return bufferCommand;
    }

    std::optional<command::DisplayCommand> mDisplayCommand;
    std::optional<command::LayerCommand> mLayerCommand;
    std::vector<command::CommandPayload> mCommands;
    std::vector<command::CommandResultPayload> mCommandsResults;

  private:
    // std::vector<native_handle_t*> mTemporaryHandles;

    command::DisplayCommand& getDisplayCommand(int64_t display) {
        if (!mDisplayCommand.has_value() || mDisplayCommand->display != display) {
            if (mDisplayCommand.has_value()) mCommands.emplace_back(std::move(*mDisplayCommand));
            mDisplayCommand.emplace();
            mDisplayCommand->display = display;
            return *mDisplayCommand;
        }
        return *mDisplayCommand;
    }

    command::LayerCommand& getLayerCommand(int64_t display, int64_t layer) {
        if (!mLayerCommand.has_value() || mLayerCommand->display != display ||
            mLayerCommand->layer != layer) {
            if (mLayerCommand.has_value()) mCommands.emplace_back(std::move(*mLayerCommand));
            mLayerCommand.emplace();
            mLayerCommand->display = display;
            mLayerCommand->layer = layer;
            return *mLayerCommand;
        }
        return *mLayerCommand;
    }
};

class CommandReaderBase {
  public:
    ~CommandReaderBase() { resetData(); }

    // Parse and execute commands from the command queue.  The commands are
    // actually return values from the server and will be saved in ReturnData.
    void parse(const std::vector<command::CommandResultPayload>& results) {
        resetData();

        for (const auto& result : results) {
            switch (result.getTag()) {
                case command::CommandResultPayload::Tag::error:
                    parseSetError(result.get<command::CommandResultPayload::Tag::error>());
                    break;
                case command::CommandResultPayload::Tag::changedCompositionType:
                    parseSetChangedCompositionTypes(
                            result.get<
                                    command::CommandResultPayload::Tag::changedCompositionType>());
                    break;
                case command::CommandResultPayload::Tag::displayRequest:
                    parseSetDisplayRequests(
                            result.get<command::CommandResultPayload::Tag::displayRequest>());
                    break;
                case command::CommandResultPayload::Tag::presentFence:
                    parseSetPresentFence(
                            result.get<command::CommandResultPayload::Tag::presentFence>());
                    break;
                case command::CommandResultPayload::Tag::releaseFences:
                    parseSetReleaseFences(
                            result.get<command::CommandResultPayload::Tag::releaseFences>());
                    break;
                case command::CommandResultPayload::Tag::presentOrValidateResult:
                    parseSetPresentOrValidateDisplayResult(
                            result.get<
                                    command::CommandResultPayload::Tag::presentOrValidateResult>());
                    break;
                case command::CommandResultPayload::Tag::clientTargetProperty:
                    parseSetClientTargetProperty(
                            result.get<command::CommandResultPayload::Tag::clientTargetProperty>());
                    break;
            }
        }
    }

    std::vector<command::Error> takeErrors() { return std::move(mErrors); }

    bool hasChanges(int64_t display, uint32_t* outNumChangedCompositionTypes,
                    uint32_t* outNumLayerRequestMasks) const {
        auto found = mReturnData.find(display);
        if (found == mReturnData.end()) {
            *outNumChangedCompositionTypes = 0;
            *outNumLayerRequestMasks = 0;
            return false;
        }

        const ReturnData& data = found->second;

        *outNumChangedCompositionTypes = static_cast<uint32_t>(data.compositionTypes.size());
        *outNumLayerRequestMasks = static_cast<uint32_t>(data.requestMasks.size());

        return !(data.compositionTypes.empty() && data.requestMasks.empty());
    }

    // Get and clear saved changed composition types.
    void takeChangedCompositionTypes(int64_t display, std::vector<int64_t>* outLayers,
                                     std::vector<Composition>* outTypes) {
        auto found = mReturnData.find(display);
        if (found == mReturnData.end()) {
            outLayers->clear();
            outTypes->clear();
            return;
        }

        ReturnData& data = found->second;

        *outLayers = std::move(data.changedLayers);
        *outTypes = std::move(data.compositionTypes);
    }

    // Get and clear saved display requests.
    void takeDisplayRequests(int64_t display, uint32_t* outDisplayRequestMask,
                             std::vector<int64_t>* outLayers,
                             std::vector<uint32_t>* outLayerRequestMasks) {
        auto found = mReturnData.find(display);
        if (found == mReturnData.end()) {
            *outDisplayRequestMask = 0;
            outLayers->clear();
            outLayerRequestMasks->clear();
            return;
        }

        ReturnData& data = found->second;

        *outDisplayRequestMask = data.displayRequests;
        *outLayers = std::move(data.requestedLayers);
        *outLayerRequestMasks = std::move(data.requestMasks);
    }

    // Get and clear saved release fences.
    void takeReleaseFences(int64_t display, std::vector<int64_t>* outLayers,
                           std::vector<int>* outReleaseFences) {
        auto found = mReturnData.find(display);
        if (found == mReturnData.end()) {
            outLayers->clear();
            outReleaseFences->clear();
            return;
        }

        ReturnData& data = found->second;

        *outLayers = std::move(data.releasedLayers);
        *outReleaseFences = std::move(data.releaseFences);
    }

    // Get and clear saved present fence.
    void takePresentFence(int64_t display, int* outPresentFence) {
        auto found = mReturnData.find(display);
        if (found == mReturnData.end()) {
            *outPresentFence = -1;
            return;
        }

        ReturnData& data = found->second;

        *outPresentFence = data.presentFence;
        data.presentFence = -1;
    }

    // Get what stage succeeded during PresentOrValidate: Present or Validate
    void takePresentOrValidateStage(int64_t display, uint32_t* state) {
        auto found = mReturnData.find(display);
        if (found == mReturnData.end()) {
            *state = static_cast<uint32_t>(-1);
            return;
        }
        ReturnData& data = found->second;
        *state = data.presentOrValidateState;
    }

    // Get the client target properties requested by hardware composer.
    void takeClientTargetProperty(int64_t display, ClientTargetProperty* outClientTargetProperty,
                                  float* outWhitePointNits) {
        auto found = mReturnData.find(display);

        // If not found, return the default values.
        if (found == mReturnData.end()) {
            outClientTargetProperty->pixelFormat = common::PixelFormat::RGBA_8888;
            outClientTargetProperty->dataspace = Dataspace::UNKNOWN;
            *outWhitePointNits = -1.f;
            return;
        }

        ReturnData& data = found->second;
        *outClientTargetProperty = data.clientTargetProperty;
        *outWhitePointNits = data.clientTargetWhitePointNits;
    }

  private:
    void resetData() {
        mErrors.clear();

        for (auto& data : mReturnData) {
            if (data.second.presentFence >= 0) {
                close(data.second.presentFence);
            }
            for (auto fence : data.second.releaseFences) {
                if (fence >= 0) {
                    close(fence);
                }
            }
        }

        mReturnData.clear();
    }

    void parseSetError(const command::Error& error) { mErrors.emplace_back(error); }

    void parseSetChangedCompositionTypes(
            const command::ChangedCompositionTypes& changedCompositionTypes) {
        auto& data = mReturnData[changedCompositionTypes.display];

        data.changedLayers.reserve(changedCompositionTypes.layers.size());
        data.compositionTypes.reserve(changedCompositionTypes.layers.size());
        for (const auto& layer : changedCompositionTypes.layers) {
            data.changedLayers.push_back(layer.layer);
            data.compositionTypes.push_back(layer.composition);
        }
    }

    void parseSetDisplayRequests(const command::DisplayRequest& displayRequest) {
        auto& data = mReturnData[displayRequest.display];

        data.displayRequests = displayRequest.mask;
        data.requestedLayers.reserve(displayRequest.layerRequests.size());
        data.requestMasks.reserve(displayRequest.layerRequests.size());
        for (const auto& layerRequest : displayRequest.layerRequests) {
            data.requestedLayers.push_back(layerRequest.layer);
            data.requestMasks.push_back(layerRequest.mask);
        }
    }

    void parseSetPresentFence(const command::PresentFence& presentFence) {
        auto& data = mReturnData[presentFence.display];
        if (data.presentFence >= 0) {
            close(data.presentFence);
        }
        data.presentFence = dup(presentFence.fence.get());
    }

    void parseSetReleaseFences(const command::ReleaseFences& releaseFences) {
        auto& data = mReturnData[releaseFences.display];
        data.releasedLayers.reserve(releaseFences.layers.size());
        data.releaseFences.reserve(releaseFences.layers.size());
        for (const auto& layer : releaseFences.layers) {
            data.releasedLayers.push_back(layer.layer);
            data.releaseFences.push_back(dup(layer.fence.get()));
        }
    }

    void parseSetPresentOrValidateDisplayResult(
            const command::PresentOrValidate& presentOrValidate) {
        auto& data = mReturnData[presentOrValidate.display];
        data.presentOrValidateState =
                presentOrValidate.result == command::PresentOrValidate::Result::Presented ? 1 : 0;
    }

    void parseSetClientTargetProperty(
            const command::ClientTargetPropertyWithNits& clientTargetProperty) {
        auto& data = mReturnData[clientTargetProperty.display];
        data.clientTargetProperty.pixelFormat =
                clientTargetProperty.clientTargetProperty.pixelFormat;
        data.clientTargetProperty.dataspace = clientTargetProperty.clientTargetProperty.dataspace;
        data.clientTargetWhitePointNits = clientTargetProperty.whitePointNits;
    }

    struct ReturnData {
        int32_t displayRequests = 0;

        std::vector<int64_t> changedLayers;
        std::vector<Composition> compositionTypes;

        std::vector<int64_t> requestedLayers;
        std::vector<uint32_t> requestMasks;

        int presentFence = -1;

        std::vector<int64_t> releasedLayers;
        std::vector<int> releaseFences;

        uint32_t presentOrValidateState;

        ClientTargetProperty clientTargetProperty{common::PixelFormat::RGBA_8888,
                                                  Dataspace::UNKNOWN};
        float clientTargetWhitePointNits = -1.f;
    };

    std::vector<command::Error> mErrors;
    std::unordered_map<int64_t, ReturnData> mReturnData;
};

}  // namespace aidl::android::hardware::graphics::composer3
