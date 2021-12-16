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

#include <aidl/android/hardware/graphics/composer3/CommandResultPayload.h>
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

class ComposerClientReader {
  public:
    ~ComposerClientReader() { resetData(); }

    // Parse and execute commands from the command queue.  The commands are
    // actually return values from the server and will be saved in ReturnData.
    void parse(const std::vector<CommandResultPayload>& results) {
        resetData();

        for (const auto& result : results) {
            switch (result.getTag()) {
                case CommandResultPayload::Tag::error:
                    parseSetError(result.get<CommandResultPayload::Tag::error>());
                    break;
                case CommandResultPayload::Tag::changedCompositionTypes:
                    parseSetChangedCompositionTypes(
                            result.get<CommandResultPayload::Tag::changedCompositionTypes>());
                    break;
                case CommandResultPayload::Tag::displayRequest:
                    parseSetDisplayRequests(
                            result.get<CommandResultPayload::Tag::displayRequest>());
                    break;
                case CommandResultPayload::Tag::presentFence:
                    parseSetPresentFence(result.get<CommandResultPayload::Tag::presentFence>());
                    break;
                case CommandResultPayload::Tag::releaseFences:
                    parseSetReleaseFences(result.get<CommandResultPayload::Tag::releaseFences>());
                    break;
                case CommandResultPayload::Tag::presentOrValidateResult:
                    parseSetPresentOrValidateDisplayResult(
                            result.get<CommandResultPayload::Tag::presentOrValidateResult>());
                    break;
                case CommandResultPayload::Tag::clientTargetProperty:
                    parseSetClientTargetProperty(
                            result.get<CommandResultPayload::Tag::clientTargetProperty>());
                    break;
            }
        }
    }

    std::vector<CommandError> takeErrors() { return std::move(mErrors); }

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

    void parseSetError(const CommandError& error) { mErrors.emplace_back(error); }

    void parseSetChangedCompositionTypes(const ChangedCompositionTypes& changedCompositionTypes) {
        auto& data = mReturnData[changedCompositionTypes.display];

        data.changedLayers.reserve(changedCompositionTypes.layers.size());
        data.compositionTypes.reserve(changedCompositionTypes.layers.size());
        for (const auto& layer : changedCompositionTypes.layers) {
            data.changedLayers.push_back(layer.layer);
            data.compositionTypes.push_back(layer.composition);
        }
    }

    void parseSetDisplayRequests(const DisplayRequest& displayRequest) {
        auto& data = mReturnData[displayRequest.display];

        data.displayRequests = displayRequest.mask;
        data.requestedLayers.reserve(displayRequest.layerRequests.size());
        data.requestMasks.reserve(displayRequest.layerRequests.size());
        for (const auto& layerRequest : displayRequest.layerRequests) {
            data.requestedLayers.push_back(layerRequest.layer);
            data.requestMasks.push_back(layerRequest.mask);
        }
    }

    void parseSetPresentFence(const PresentFence& presentFence) {
        auto& data = mReturnData[presentFence.display];
        if (data.presentFence >= 0) {
            close(data.presentFence);
        }
        data.presentFence = dup(presentFence.fence.get());
    }

    void parseSetReleaseFences(const ReleaseFences& releaseFences) {
        auto& data = mReturnData[releaseFences.display];
        data.releasedLayers.reserve(releaseFences.layers.size());
        data.releaseFences.reserve(releaseFences.layers.size());
        for (const auto& layer : releaseFences.layers) {
            data.releasedLayers.push_back(layer.layer);
            data.releaseFences.push_back(dup(layer.fence.get()));
        }
    }

    void parseSetPresentOrValidateDisplayResult(const PresentOrValidate& presentOrValidate) {
        auto& data = mReturnData[presentOrValidate.display];
        data.presentOrValidateState =
                presentOrValidate.result == PresentOrValidate::Result::Presented ? 1 : 0;
    }

    void parseSetClientTargetProperty(const ClientTargetPropertyWithNits& clientTargetProperty) {
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

    std::vector<CommandError> mErrors;
    std::unordered_map<int64_t, ReturnData> mReturnData;
};

}  // namespace aidl::android::hardware::graphics::composer3
