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
#include <optional>
#include <unordered_map>
#include <vector>

#include <inttypes.h>
#include <string.h>

#include <aidl/android/hardware/graphics/composer3/ClientTargetProperty.h>
#include <aidl/android/hardware/graphics/composer3/Composition.h>
#include <aidl/android/hardware/graphics/composer3/CommandResultPayload.h>


#include <log/log.h>
#include <sync/sync.h>


using aidl::android::hardware::graphics::common::Dataspace;

namespace aidl::android::hardware::graphics::composer3 {

class ComposerClientReader {
  public:
    explicit ComposerClientReader(std::optional<int64_t> display = {}) : mDisplay(display) {}

    ~ComposerClientReader() { resetData(); }

    ComposerClientReader(ComposerClientReader&&) = default;

    ComposerClientReader(const ComposerClientReader&) = delete;
    ComposerClientReader& operator=(const ComposerClientReader&) = delete;

    // Parse and execute commands from the command queue.  The commands are
    // actually return values from the server and will be saved in ReturnData.
    void parse(std::vector<CommandResultPayload>&& results) {
        resetData();

        for (auto& result : results) {
            switch (result.getTag()) {
                case CommandResultPayload::Tag::error:
                    parseSetError(std::move(result.get<CommandResultPayload::Tag::error>()));
                    break;
                case CommandResultPayload::Tag::changedCompositionTypes:
                    parseSetChangedCompositionTypes(std::move(
                            result.get<CommandResultPayload::Tag::changedCompositionTypes>()));
                    break;
                case CommandResultPayload::Tag::displayRequest:
                    parseSetDisplayRequests(
                            std::move(result.get<CommandResultPayload::Tag::displayRequest>()));
                    break;
                case CommandResultPayload::Tag::presentFence:
                    parseSetPresentFence(
                            std::move(result.get<CommandResultPayload::Tag::presentFence>()));
                    break;
                case CommandResultPayload::Tag::releaseFences:
                    parseSetReleaseFences(
                            std::move(result.get<CommandResultPayload::Tag::releaseFences>()));
                    break;
                case CommandResultPayload::Tag::presentOrValidateResult:
                    parseSetPresentOrValidateDisplayResult(std::move(
                            result.get<CommandResultPayload::Tag::presentOrValidateResult>()));
                    break;
                case CommandResultPayload::Tag::clientTargetProperty:
                    parseSetClientTargetProperty(std::move(
                            result.get<CommandResultPayload::Tag::clientTargetProperty>()));
                    break;
            }
        }
    }

    std::vector<CommandError> takeErrors() { return std::move(mErrors); }

    void hasChanges(int64_t display, uint32_t* outNumChangedCompositionTypes,
                    uint32_t* outNumLayerRequestMasks) const {
        LOG_ALWAYS_FATAL_IF(mDisplay && display != *mDisplay);
        auto found = mReturnData.find(display);
        if (found == mReturnData.end()) {
            *outNumChangedCompositionTypes = 0;
            *outNumLayerRequestMasks = 0;
            return;
        }

        const ReturnData& data = found->second;

        *outNumChangedCompositionTypes = static_cast<uint32_t>(data.changedLayers.size());
        *outNumLayerRequestMasks = static_cast<uint32_t>(data.displayRequests.layerRequests.size());
    }

    // Get and clear saved changed composition types.
    std::vector<ChangedCompositionLayer> takeChangedCompositionTypes(int64_t display) {
        LOG_ALWAYS_FATAL_IF(mDisplay && display != *mDisplay);
        auto found = mReturnData.find(display);
        if (found == mReturnData.end()) {
            return {};
        }

        ReturnData& data = found->second;
        return std::move(data.changedLayers);
    }

    // Get and clear saved display requests.
    DisplayRequest takeDisplayRequests(int64_t display) {
        LOG_ALWAYS_FATAL_IF(mDisplay && display != *mDisplay);
        auto found = mReturnData.find(display);
        if (found == mReturnData.end()) {
            return {};
        }

        ReturnData& data = found->second;
        return std::move(data.displayRequests);
    }

    // Get and clear saved release fences.
    std::vector<ReleaseFences::Layer> takeReleaseFences(int64_t display) {
        LOG_ALWAYS_FATAL_IF(mDisplay && display != *mDisplay);
        auto found = mReturnData.find(display);
        if (found == mReturnData.end()) {
            return {};
        }

        ReturnData& data = found->second;
        return std::move(data.releasedLayers);
    }

    // Get and clear saved present fence.
    ndk::ScopedFileDescriptor takePresentFence(int64_t display) {
        LOG_ALWAYS_FATAL_IF(mDisplay && display != *mDisplay);
        auto found = mReturnData.find(display);
        if (found == mReturnData.end()) {
            return {};
        }

        ReturnData& data = found->second;
        return std::move(data.presentFence);
    }

    // Get what stage succeeded during PresentOrValidate: Present or Validate
    std::optional<PresentOrValidate::Result> takePresentOrValidateStage(int64_t display) {
        LOG_ALWAYS_FATAL_IF(mDisplay && display != *mDisplay);
        auto found = mReturnData.find(display);
        if (found == mReturnData.end()) {
            return std::nullopt;
        }
        ReturnData& data = found->second;
        return data.presentOrValidateState;
    }

    // Get the client target properties requested by hardware composer.
    ClientTargetPropertyWithBrightness takeClientTargetProperty(int64_t display) {
        LOG_ALWAYS_FATAL_IF(mDisplay && display != *mDisplay);
        auto found = mReturnData.find(display);

        // If not found, return the default values.
        if (found == mReturnData.end()) {
            return ClientTargetPropertyWithBrightness{
                    .clientTargetProperty = {common::PixelFormat::RGBA_8888, Dataspace::UNKNOWN},
                    .brightness = 1.f,
            };
        }

        ReturnData& data = found->second;
        return std::move(data.clientTargetProperty);
    }

  private:
    void resetData() {
        mErrors.clear();
        mReturnData.clear();
    }

    void parseSetError(CommandError&& error) { mErrors.emplace_back(error); }

    void parseSetChangedCompositionTypes(ChangedCompositionTypes&& changedCompositionTypes) {
        LOG_ALWAYS_FATAL_IF(mDisplay && changedCompositionTypes.display != *mDisplay);
        auto& data = mReturnData[changedCompositionTypes.display];
        data.changedLayers = std::move(changedCompositionTypes.layers);
    }

    void parseSetDisplayRequests(DisplayRequest&& displayRequest) {
        LOG_ALWAYS_FATAL_IF(mDisplay && displayRequest.display != *mDisplay);
        auto& data = mReturnData[displayRequest.display];
        data.displayRequests = std::move(displayRequest);
    }

    void parseSetPresentFence(PresentFence&& presentFence) {
        LOG_ALWAYS_FATAL_IF(mDisplay && presentFence.display != *mDisplay);
        auto& data = mReturnData[presentFence.display];
        data.presentFence = std::move(presentFence.fence);
    }

    void parseSetReleaseFences(ReleaseFences&& releaseFences) {
        LOG_ALWAYS_FATAL_IF(mDisplay && releaseFences.display != *mDisplay);
        auto& data = mReturnData[releaseFences.display];
        data.releasedLayers = std::move(releaseFences.layers);
    }

    void parseSetPresentOrValidateDisplayResult(const PresentOrValidate&& presentOrValidate) {
        LOG_ALWAYS_FATAL_IF(mDisplay && presentOrValidate.display != *mDisplay);
        auto& data = mReturnData[presentOrValidate.display];
        data.presentOrValidateState = std::move(presentOrValidate.result);
    }

    void parseSetClientTargetProperty(
            const ClientTargetPropertyWithBrightness&& clientTargetProperty) {
        LOG_ALWAYS_FATAL_IF(mDisplay && clientTargetProperty.display != *mDisplay);
        auto& data = mReturnData[clientTargetProperty.display];
        data.clientTargetProperty = std::move(clientTargetProperty);
    }

    struct ReturnData {
        DisplayRequest displayRequests;
        std::vector<ChangedCompositionLayer> changedLayers;
        ndk::ScopedFileDescriptor presentFence;
        std::vector<ReleaseFences::Layer> releasedLayers;
        PresentOrValidate::Result presentOrValidateState;

        ClientTargetPropertyWithBrightness clientTargetProperty = {
                .clientTargetProperty = {common::PixelFormat::RGBA_8888, Dataspace::UNKNOWN},
                .brightness = 1.f,
        };
    };

    std::vector<CommandError> mErrors;
    std::unordered_map<int64_t, ReturnData> mReturnData;
    const std::optional<int64_t> mDisplay;
};

}  // namespace aidl::android::hardware::graphics::composer3
