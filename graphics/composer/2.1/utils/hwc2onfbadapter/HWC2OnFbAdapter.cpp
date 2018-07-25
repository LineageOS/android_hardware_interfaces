/*
 * Copyright 2017 The Android Open Source Project
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

#define LOG_TAG "HWC2OnFbAdapter"

//#define LOG_NDEBUG 0

#include "hwc2onfbadapter/HWC2OnFbAdapter.h"

#include <algorithm>
#include <type_traits>

#include <inttypes.h>
#include <time.h>
#include <sys/prctl.h>
#include <unistd.h> // for close

#include <hardware/fb.h>
#include <log/log.h>
#include <sync/sync.h>

using namespace HWC2;

namespace android {

namespace {

void dumpHook(hwc2_device_t* device, uint32_t* outSize, char* outBuffer) {
    auto& adapter = HWC2OnFbAdapter::cast(device);
    if (outBuffer) {
        *outSize = adapter.getDebugString().copy(outBuffer, *outSize);
    } else {
        adapter.updateDebugString();
        *outSize = adapter.getDebugString().size();
    }
}

int32_t registerCallbackHook(hwc2_device_t* device, int32_t descriptor,
                             hwc2_callback_data_t callbackData, hwc2_function_pointer_t pointer) {
    auto& adapter = HWC2OnFbAdapter::cast(device);
    switch (descriptor) {
        case HWC2_CALLBACK_HOTPLUG:
            if (pointer) {
                reinterpret_cast<HWC2_PFN_HOTPLUG>(pointer)(callbackData, adapter.getDisplayId(),
                                                            HWC2_CONNECTION_CONNECTED);
            }
            break;
        case HWC2_CALLBACK_REFRESH:
            break;
        case HWC2_CALLBACK_VSYNC:
            adapter.setVsyncCallback(reinterpret_cast<HWC2_PFN_VSYNC>(pointer), callbackData);
            break;
        default:
            return HWC2_ERROR_BAD_PARAMETER;
    }

    return HWC2_ERROR_NONE;
}

uint32_t getMaxVirtualDisplayCountHook(hwc2_device_t* /*device*/) {
    return 0;
}

int32_t createVirtualDisplayHook(hwc2_device_t* /*device*/, uint32_t /*width*/, uint32_t /*height*/,
                                 int32_t* /*format*/, hwc2_display_t* /*outDisplay*/) {
    return HWC2_ERROR_NO_RESOURCES;
}

int32_t destroyVirtualDisplayHook(hwc2_device_t* /*device*/, hwc2_display_t /*display*/) {
    return HWC2_ERROR_BAD_DISPLAY;
}

int32_t setOutputBufferHook(hwc2_device_t* /*device*/, hwc2_display_t /*display*/,
                            buffer_handle_t /*buffer*/, int32_t /*releaseFence*/) {
    return HWC2_ERROR_BAD_DISPLAY;
}

int32_t getDisplayNameHook(hwc2_device_t* device, hwc2_display_t display, uint32_t* outSize,
                           char* outName) {
    auto& adapter = HWC2OnFbAdapter::cast(device);
    if (adapter.getDisplayId() != display) {
        return HWC2_ERROR_BAD_DISPLAY;
    }

    const auto& info = adapter.getInfo();
    if (outName) {
        *outSize = info.name.copy(outName, *outSize);
    } else {
        *outSize = info.name.size();
    }

    return HWC2_ERROR_NONE;
}

int32_t getDisplayTypeHook(hwc2_device_t* device, hwc2_display_t display, int32_t* outType) {
    auto& adapter = HWC2OnFbAdapter::cast(device);
    if (adapter.getDisplayId() != display) {
        return HWC2_ERROR_BAD_DISPLAY;
    }

    *outType = HWC2_DISPLAY_TYPE_PHYSICAL;
    return HWC2_ERROR_NONE;
}

int32_t getDozeSupportHook(hwc2_device_t* device, hwc2_display_t display, int32_t* outSupport) {
    auto& adapter = HWC2OnFbAdapter::cast(device);
    if (adapter.getDisplayId() != display) {
        return HWC2_ERROR_BAD_DISPLAY;
    }

    *outSupport = 0;
    return HWC2_ERROR_NONE;
}

int32_t getHdrCapabilitiesHook(hwc2_device_t* device, hwc2_display_t display, uint32_t* outNumTypes,
                               int32_t* /*outTypes*/, float* /*outMaxLuminance*/,
                               float* /*outMaxAverageLuminance*/, float* /*outMinLuminance*/) {
    auto& adapter = HWC2OnFbAdapter::cast(device);
    if (adapter.getDisplayId() != display) {
        return HWC2_ERROR_BAD_DISPLAY;
    }

    *outNumTypes = 0;
    return HWC2_ERROR_NONE;
}

int32_t setPowerModeHook(hwc2_device_t* device, hwc2_display_t display, int32_t /*mode*/) {
    auto& adapter = HWC2OnFbAdapter::cast(device);
    if (adapter.getDisplayId() != display) {
        return HWC2_ERROR_BAD_DISPLAY;
    }

    // pretend that it works
    return HWC2_ERROR_NONE;
}

int32_t setVsyncEnabledHook(hwc2_device_t* device, hwc2_display_t display, int32_t enabled) {
    auto& adapter = HWC2OnFbAdapter::cast(device);
    if (adapter.getDisplayId() != display) {
        return HWC2_ERROR_BAD_DISPLAY;
    }

    adapter.enableVsync(enabled == HWC2_VSYNC_ENABLE);
    return HWC2_ERROR_NONE;
}

int32_t getColorModesHook(hwc2_device_t* device, hwc2_display_t display, uint32_t* outNumModes,
                          int32_t* outModes) {
    auto& adapter = HWC2OnFbAdapter::cast(device);
    if (adapter.getDisplayId() != display) {
        return HWC2_ERROR_BAD_DISPLAY;
    }

    if (outModes) {
        if (*outNumModes > 0) {
            outModes[0] = HAL_COLOR_MODE_NATIVE;
            *outNumModes = 1;
        }
    } else {
        *outNumModes = 1;
    }

    return HWC2_ERROR_NONE;
}

int32_t setColorModeHook(hwc2_device_t* device, hwc2_display_t display, int32_t mode) {
    auto& adapter = HWC2OnFbAdapter::cast(device);
    if (adapter.getDisplayId() != display) {
        return HWC2_ERROR_BAD_DISPLAY;
    }
    if (mode != HAL_COLOR_MODE_NATIVE) {
        return HWC2_ERROR_BAD_PARAMETER;
    }

    return HWC2_ERROR_NONE;
}

int32_t setColorTransformHook(hwc2_device_t* device, hwc2_display_t display,
                              const float* /*matrix*/, int32_t /*hint*/) {
    auto& adapter = HWC2OnFbAdapter::cast(device);
    if (adapter.getDisplayId() != display) {
        return HWC2_ERROR_BAD_DISPLAY;
    }

    // we always force client composition
    adapter.setState(HWC2OnFbAdapter::State::MODIFIED);
    return HWC2_ERROR_NONE;
}

int32_t getClientTargetSupportHook(hwc2_device_t* device, hwc2_display_t display, uint32_t width,
                                   uint32_t height, int32_t format, int32_t dataspace) {
    auto& adapter = HWC2OnFbAdapter::cast(device);
    if (adapter.getDisplayId() != display) {
        return HWC2_ERROR_BAD_DISPLAY;
    }
    if (dataspace != HAL_DATASPACE_UNKNOWN) {
        return HWC2_ERROR_UNSUPPORTED;
    }

    const auto& info = adapter.getInfo();
    return (info.width == width && info.height == height && info.format == format)
            ? HWC2_ERROR_NONE
            : HWC2_ERROR_UNSUPPORTED;
}

int32_t setClientTargetHook(hwc2_device_t* device, hwc2_display_t display, buffer_handle_t target,
                            int32_t acquireFence, int32_t dataspace, hwc_region_t /*damage*/) {
    if (acquireFence >= 0) {
        sync_wait(acquireFence, -1);
        close(acquireFence);
    }

    auto& adapter = HWC2OnFbAdapter::cast(device);
    if (adapter.getDisplayId() != display) {
        return HWC2_ERROR_BAD_DISPLAY;
    }
    if (dataspace != HAL_DATASPACE_UNKNOWN) {
        return HWC2_ERROR_BAD_PARAMETER;
    }

    // no state change
    adapter.setBuffer(target);
    return HWC2_ERROR_NONE;
}

int32_t getDisplayConfigsHook(hwc2_device_t* device, hwc2_display_t display,
                              uint32_t* outNumConfigs, hwc2_config_t* outConfigs) {
    auto& adapter = HWC2OnFbAdapter::cast(device);
    if (adapter.getDisplayId() != display) {
        return HWC2_ERROR_BAD_DISPLAY;
    }

    if (outConfigs) {
        if (*outNumConfigs > 0) {
            outConfigs[0] = adapter.getConfigId();
            *outNumConfigs = 1;
        }
    } else {
        *outNumConfigs = 1;
    }

    return HWC2_ERROR_NONE;
}

int32_t getDisplayAttributeHook(hwc2_device_t* device, hwc2_display_t display, hwc2_config_t config,
                                int32_t attribute, int32_t* outValue) {
    auto& adapter = HWC2OnFbAdapter::cast(device);
    if (adapter.getDisplayId() != display) {
        return HWC2_ERROR_BAD_DISPLAY;
    }
    if (adapter.getConfigId() != config) {
        return HWC2_ERROR_BAD_CONFIG;
    }

    const auto& info = adapter.getInfo();
    switch (attribute) {
        case HWC2_ATTRIBUTE_WIDTH:
            *outValue = int32_t(info.width);
            break;
        case HWC2_ATTRIBUTE_HEIGHT:
            *outValue = int32_t(info.height);
            break;
        case HWC2_ATTRIBUTE_VSYNC_PERIOD:
            *outValue = int32_t(info.vsync_period_ns);
            break;
        case HWC2_ATTRIBUTE_DPI_X:
            *outValue = int32_t(info.xdpi_scaled);
            break;
        case HWC2_ATTRIBUTE_DPI_Y:
            *outValue = int32_t(info.ydpi_scaled);
            break;
        default:
            return HWC2_ERROR_BAD_PARAMETER;
    }

    return HWC2_ERROR_NONE;
}

int32_t getActiveConfigHook(hwc2_device_t* device, hwc2_display_t display,
                            hwc2_config_t* outConfig) {
    auto& adapter = HWC2OnFbAdapter::cast(device);
    if (adapter.getDisplayId() != display) {
        return HWC2_ERROR_BAD_DISPLAY;
    }

    *outConfig = adapter.getConfigId();
    return HWC2_ERROR_NONE;
}

int32_t setActiveConfigHook(hwc2_device_t* device, hwc2_display_t display, hwc2_config_t config) {
    auto& adapter = HWC2OnFbAdapter::cast(device);
    if (adapter.getDisplayId() != display) {
        return HWC2_ERROR_BAD_DISPLAY;
    }
    if (adapter.getConfigId() != config) {
        return HWC2_ERROR_BAD_CONFIG;
    }

    return HWC2_ERROR_NONE;
}

int32_t validateDisplayHook(hwc2_device_t* device, hwc2_display_t display, uint32_t* outNumTypes,
                            uint32_t* outNumRequests) {
    auto& adapter = HWC2OnFbAdapter::cast(device);
    if (adapter.getDisplayId() != display) {
        return HWC2_ERROR_BAD_DISPLAY;
    }

    const auto& dirtyLayers = adapter.getDirtyLayers();
    *outNumTypes = dirtyLayers.size();
    *outNumRequests = 0;

    if (*outNumTypes > 0) {
        adapter.setState(HWC2OnFbAdapter::State::VALIDATED_WITH_CHANGES);
        return HWC2_ERROR_HAS_CHANGES;
    } else {
        adapter.setState(HWC2OnFbAdapter::State::VALIDATED);
        return HWC2_ERROR_NONE;
    }
}

int32_t getChangedCompositionTypesHook(hwc2_device_t* device, hwc2_display_t display,
                                       uint32_t* outNumElements, hwc2_layer_t* outLayers,
                                       int32_t* outTypes) {
    auto& adapter = HWC2OnFbAdapter::cast(device);
    if (adapter.getDisplayId() != display) {
        return HWC2_ERROR_BAD_DISPLAY;
    }
    if (adapter.getState() == HWC2OnFbAdapter::State::MODIFIED) {
        return HWC2_ERROR_NOT_VALIDATED;
    }

    // request client composition for all layers
    const auto& dirtyLayers = adapter.getDirtyLayers();
    if (outLayers && outTypes) {
        *outNumElements = std::min(*outNumElements, uint32_t(dirtyLayers.size()));
        auto iter = dirtyLayers.cbegin();
        for (uint32_t i = 0; i < *outNumElements; i++) {
            outLayers[i] = *iter++;
            outTypes[i] = HWC2_COMPOSITION_CLIENT;
        }
    } else {
        *outNumElements = dirtyLayers.size();
    }

    return HWC2_ERROR_NONE;
}

int32_t getDisplayRequestsHook(hwc2_device_t* device, hwc2_display_t display,
                               int32_t* outDisplayRequests, uint32_t* outNumElements,
                               hwc2_layer_t* /*outLayers*/, int32_t* /*outLayerRequests*/) {
    auto& adapter = HWC2OnFbAdapter::cast(device);
    if (adapter.getDisplayId() != display) {
        return HWC2_ERROR_BAD_DISPLAY;
    }
    if (adapter.getState() == HWC2OnFbAdapter::State::MODIFIED) {
        return HWC2_ERROR_NOT_VALIDATED;
    }

    *outDisplayRequests = 0;
    *outNumElements = 0;
    return HWC2_ERROR_NONE;
}

int32_t acceptDisplayChangesHook(hwc2_device_t* device, hwc2_display_t display) {
    auto& adapter = HWC2OnFbAdapter::cast(device);
    if (adapter.getDisplayId() != display) {
        return HWC2_ERROR_BAD_DISPLAY;
    }
    if (adapter.getState() == HWC2OnFbAdapter::State::MODIFIED) {
        return HWC2_ERROR_NOT_VALIDATED;
    }

    adapter.clearDirtyLayers();
    adapter.setState(HWC2OnFbAdapter::State::VALIDATED);
    return HWC2_ERROR_NONE;
}

int32_t presentDisplayHook(hwc2_device_t* device, hwc2_display_t display,
                           int32_t* outPresentFence) {
    auto& adapter = HWC2OnFbAdapter::cast(device);
    if (adapter.getDisplayId() != display) {
        return HWC2_ERROR_BAD_DISPLAY;
    }
    if (adapter.getState() != HWC2OnFbAdapter::State::VALIDATED) {
        return HWC2_ERROR_NOT_VALIDATED;
    }

    adapter.postBuffer();
    *outPresentFence = -1;

    return HWC2_ERROR_NONE;
}

int32_t getReleaseFencesHook(hwc2_device_t* device, hwc2_display_t display,
                             uint32_t* outNumElements, hwc2_layer_t* /*outLayers*/,
                             int32_t* /*outFences*/) {
    auto& adapter = HWC2OnFbAdapter::cast(device);
    if (adapter.getDisplayId() != display) {
        return HWC2_ERROR_BAD_DISPLAY;
    }

    *outNumElements = 0;
    return HWC2_ERROR_NONE;
}

int32_t createLayerHook(hwc2_device_t* device, hwc2_display_t display, hwc2_layer_t* outLayer) {
    auto& adapter = HWC2OnFbAdapter::cast(device);
    if (adapter.getDisplayId() != display) {
        return HWC2_ERROR_BAD_DISPLAY;
    }

    *outLayer = adapter.addLayer();
    adapter.setState(HWC2OnFbAdapter::State::MODIFIED);
    return HWC2_ERROR_NONE;
}

int32_t destroyLayerHook(hwc2_device_t* device, hwc2_display_t display, hwc2_layer_t layer) {
    auto& adapter = HWC2OnFbAdapter::cast(device);
    if (adapter.getDisplayId() != display) {
        return HWC2_ERROR_BAD_DISPLAY;
    }

    if (adapter.removeLayer(layer)) {
        adapter.setState(HWC2OnFbAdapter::State::MODIFIED);
        return HWC2_ERROR_NONE;
    } else {
        return HWC2_ERROR_BAD_LAYER;
    }
}

int32_t setCursorPositionHook(hwc2_device_t* device, hwc2_display_t display, hwc2_layer_t /*layer*/,
                              int32_t /*x*/, int32_t /*y*/) {
    auto& adapter = HWC2OnFbAdapter::cast(device);
    if (adapter.getDisplayId() != display) {
        return HWC2_ERROR_BAD_DISPLAY;
    }

    // always an error
    return HWC2_ERROR_BAD_LAYER;
}

int32_t setLayerBufferHook(hwc2_device_t* device, hwc2_display_t display, hwc2_layer_t layer,
                           buffer_handle_t /*buffer*/, int32_t acquireFence) {
    if (acquireFence >= 0) {
        sync_wait(acquireFence, -1);
        close(acquireFence);
    }

    auto& adapter = HWC2OnFbAdapter::cast(device);
    if (adapter.getDisplayId() != display) {
        return HWC2_ERROR_BAD_DISPLAY;
    }
    if (!adapter.hasLayer(layer)) {
        return HWC2_ERROR_BAD_LAYER;
    }

    // no state change
    return HWC2_ERROR_NONE;
}

int32_t setLayerSurfaceDamageHook(hwc2_device_t* device, hwc2_display_t display, hwc2_layer_t layer,
                                  hwc_region_t /*damage*/) {
    auto& adapter = HWC2OnFbAdapter::cast(device);
    if (adapter.getDisplayId() != display) {
        return HWC2_ERROR_BAD_DISPLAY;
    }
    if (!adapter.hasLayer(layer)) {
        return HWC2_ERROR_BAD_LAYER;
    }

    // no state change
    return HWC2_ERROR_NONE;
}

int32_t setLayerCompositionTypeHook(hwc2_device_t* device, hwc2_display_t display,
                                    hwc2_layer_t layer, int32_t type) {
    auto& adapter = HWC2OnFbAdapter::cast(device);
    if (adapter.getDisplayId() != display) {
        return HWC2_ERROR_BAD_DISPLAY;
    }
    if (!adapter.markLayerDirty(layer, type != HWC2_COMPOSITION_CLIENT)) {
        return HWC2_ERROR_BAD_LAYER;
    }

    adapter.setState(HWC2OnFbAdapter::State::MODIFIED);
    return HWC2_ERROR_NONE;
}

template <typename... Args>
int32_t setLayerStateHook(hwc2_device_t* device, hwc2_display_t display, hwc2_layer_t layer,
                          Args... /*args*/) {
    auto& adapter = HWC2OnFbAdapter::cast(device);
    if (adapter.getDisplayId() != display) {
        return HWC2_ERROR_BAD_DISPLAY;
    }
    if (!adapter.hasLayer(layer)) {
        return HWC2_ERROR_BAD_LAYER;
    }

    adapter.setState(HWC2OnFbAdapter::State::MODIFIED);
    return HWC2_ERROR_NONE;
}

template <typename PFN, typename T>
static hwc2_function_pointer_t asFP(T function) {
    static_assert(std::is_same<PFN, T>::value, "Incompatible function pointer");
    return reinterpret_cast<hwc2_function_pointer_t>(function);
}

hwc2_function_pointer_t getFunctionHook(hwc2_device_t* /*device*/, int32_t descriptor) {
    switch (descriptor) {
        // global functions
        case HWC2_FUNCTION_DUMP:
            return asFP<HWC2_PFN_DUMP>(dumpHook);
        case HWC2_FUNCTION_REGISTER_CALLBACK:
            return asFP<HWC2_PFN_REGISTER_CALLBACK>(registerCallbackHook);

        // virtual display functions
        case HWC2_FUNCTION_GET_MAX_VIRTUAL_DISPLAY_COUNT:
            return asFP<HWC2_PFN_GET_MAX_VIRTUAL_DISPLAY_COUNT>(getMaxVirtualDisplayCountHook);
        case HWC2_FUNCTION_CREATE_VIRTUAL_DISPLAY:
            return asFP<HWC2_PFN_CREATE_VIRTUAL_DISPLAY>(createVirtualDisplayHook);
        case HWC2_FUNCTION_DESTROY_VIRTUAL_DISPLAY:
            return asFP<HWC2_PFN_DESTROY_VIRTUAL_DISPLAY>(destroyVirtualDisplayHook);
        case HWC2_FUNCTION_SET_OUTPUT_BUFFER:
            return asFP<HWC2_PFN_SET_OUTPUT_BUFFER>(setOutputBufferHook);

        // display functions
        case HWC2_FUNCTION_GET_DISPLAY_NAME:
            return asFP<HWC2_PFN_GET_DISPLAY_NAME>(getDisplayNameHook);
        case HWC2_FUNCTION_GET_DISPLAY_TYPE:
            return asFP<HWC2_PFN_GET_DISPLAY_TYPE>(getDisplayTypeHook);
        case HWC2_FUNCTION_GET_DOZE_SUPPORT:
            return asFP<HWC2_PFN_GET_DOZE_SUPPORT>(getDozeSupportHook);
        case HWC2_FUNCTION_GET_HDR_CAPABILITIES:
            return asFP<HWC2_PFN_GET_HDR_CAPABILITIES>(getHdrCapabilitiesHook);
        case HWC2_FUNCTION_SET_POWER_MODE:
            return asFP<HWC2_PFN_SET_POWER_MODE>(setPowerModeHook);
        case HWC2_FUNCTION_SET_VSYNC_ENABLED:
            return asFP<HWC2_PFN_SET_VSYNC_ENABLED>(setVsyncEnabledHook);
        case HWC2_FUNCTION_GET_COLOR_MODES:
            return asFP<HWC2_PFN_GET_COLOR_MODES>(getColorModesHook);
        case HWC2_FUNCTION_SET_COLOR_MODE:
            return asFP<HWC2_PFN_SET_COLOR_MODE>(setColorModeHook);
        case HWC2_FUNCTION_SET_COLOR_TRANSFORM:
            return asFP<HWC2_PFN_SET_COLOR_TRANSFORM>(setColorTransformHook);
        case HWC2_FUNCTION_GET_CLIENT_TARGET_SUPPORT:
            return asFP<HWC2_PFN_GET_CLIENT_TARGET_SUPPORT>(getClientTargetSupportHook);
        case HWC2_FUNCTION_SET_CLIENT_TARGET:
            return asFP<HWC2_PFN_SET_CLIENT_TARGET>(setClientTargetHook);

        // config functions
        case HWC2_FUNCTION_GET_DISPLAY_CONFIGS:
            return asFP<HWC2_PFN_GET_DISPLAY_CONFIGS>(getDisplayConfigsHook);
        case HWC2_FUNCTION_GET_DISPLAY_ATTRIBUTE:
            return asFP<HWC2_PFN_GET_DISPLAY_ATTRIBUTE>(getDisplayAttributeHook);
        case HWC2_FUNCTION_GET_ACTIVE_CONFIG:
            return asFP<HWC2_PFN_GET_ACTIVE_CONFIG>(getActiveConfigHook);
        case HWC2_FUNCTION_SET_ACTIVE_CONFIG:
            return asFP<HWC2_PFN_SET_ACTIVE_CONFIG>(setActiveConfigHook);

        // validate/present functions
        case HWC2_FUNCTION_VALIDATE_DISPLAY:
            return asFP<HWC2_PFN_VALIDATE_DISPLAY>(validateDisplayHook);
        case HWC2_FUNCTION_GET_CHANGED_COMPOSITION_TYPES:
            return asFP<HWC2_PFN_GET_CHANGED_COMPOSITION_TYPES>(getChangedCompositionTypesHook);
        case HWC2_FUNCTION_GET_DISPLAY_REQUESTS:
            return asFP<HWC2_PFN_GET_DISPLAY_REQUESTS>(getDisplayRequestsHook);
        case HWC2_FUNCTION_ACCEPT_DISPLAY_CHANGES:
            return asFP<HWC2_PFN_ACCEPT_DISPLAY_CHANGES>(acceptDisplayChangesHook);
        case HWC2_FUNCTION_PRESENT_DISPLAY:
            return asFP<HWC2_PFN_PRESENT_DISPLAY>(presentDisplayHook);
        case HWC2_FUNCTION_GET_RELEASE_FENCES:
            return asFP<HWC2_PFN_GET_RELEASE_FENCES>(getReleaseFencesHook);

        // layer create/destroy
        case HWC2_FUNCTION_CREATE_LAYER:
            return asFP<HWC2_PFN_CREATE_LAYER>(createLayerHook);
        case HWC2_FUNCTION_DESTROY_LAYER:
            return asFP<HWC2_PFN_DESTROY_LAYER>(destroyLayerHook);

        // layer functions; validateDisplay not required
        case HWC2_FUNCTION_SET_CURSOR_POSITION:
            return asFP<HWC2_PFN_SET_CURSOR_POSITION>(setCursorPositionHook);
        case HWC2_FUNCTION_SET_LAYER_BUFFER:
            return asFP<HWC2_PFN_SET_LAYER_BUFFER>(setLayerBufferHook);
        case HWC2_FUNCTION_SET_LAYER_SURFACE_DAMAGE:
            return asFP<HWC2_PFN_SET_LAYER_SURFACE_DAMAGE>(setLayerSurfaceDamageHook);

        // layer state functions; validateDisplay required
        case HWC2_FUNCTION_SET_LAYER_COMPOSITION_TYPE:
            return asFP<HWC2_PFN_SET_LAYER_COMPOSITION_TYPE>(setLayerCompositionTypeHook);
        case HWC2_FUNCTION_SET_LAYER_BLEND_MODE:
            return asFP<HWC2_PFN_SET_LAYER_BLEND_MODE>(setLayerStateHook<int32_t>);
        case HWC2_FUNCTION_SET_LAYER_COLOR:
            return asFP<HWC2_PFN_SET_LAYER_COLOR>(setLayerStateHook<hwc_color_t>);
        case HWC2_FUNCTION_SET_LAYER_DATASPACE:
            return asFP<HWC2_PFN_SET_LAYER_DATASPACE>(setLayerStateHook<int32_t>);
        case HWC2_FUNCTION_SET_LAYER_DISPLAY_FRAME:
            return asFP<HWC2_PFN_SET_LAYER_DISPLAY_FRAME>(setLayerStateHook<hwc_rect_t>);
        case HWC2_FUNCTION_SET_LAYER_PLANE_ALPHA:
            return asFP<HWC2_PFN_SET_LAYER_PLANE_ALPHA>(setLayerStateHook<float>);
        case HWC2_FUNCTION_SET_LAYER_SIDEBAND_STREAM:
            return asFP<HWC2_PFN_SET_LAYER_SIDEBAND_STREAM>(setLayerStateHook<buffer_handle_t>);
        case HWC2_FUNCTION_SET_LAYER_SOURCE_CROP:
            return asFP<HWC2_PFN_SET_LAYER_SOURCE_CROP>(setLayerStateHook<hwc_frect_t>);
        case HWC2_FUNCTION_SET_LAYER_TRANSFORM:
            return asFP<HWC2_PFN_SET_LAYER_TRANSFORM>(setLayerStateHook<int32_t>);
        case HWC2_FUNCTION_SET_LAYER_VISIBLE_REGION:
            return asFP<HWC2_PFN_SET_LAYER_VISIBLE_REGION>(setLayerStateHook<hwc_region_t>);
        case HWC2_FUNCTION_SET_LAYER_Z_ORDER:
            return asFP<HWC2_PFN_SET_LAYER_Z_ORDER>(setLayerStateHook<uint32_t>);

        default:
            ALOGE("unknown function descriptor %d", descriptor);
            return nullptr;
    }
}

void getCapabilitiesHook(hwc2_device_t* device, uint32_t* outCount,
                         int32_t* outCapabilities) {
    auto& adapter = HWC2OnFbAdapter::cast(device);
    adapter.getCapabilities(outCount, outCapabilities);
}

int closeHook(hw_device_t* device) {
    auto& adapter = HWC2OnFbAdapter::cast(device);
    adapter.close();
    return 0;
}

} // anonymous namespace

HWC2OnFbAdapter::HWC2OnFbAdapter(framebuffer_device_t* fbDevice)
      : hwc2_device_t(), mFbDevice(fbDevice) {
    common.close = closeHook;
    hwc2_device::getCapabilities = getCapabilitiesHook;
    hwc2_device::getFunction = getFunctionHook;

    mFbInfo.name = "fbdev";
    mFbInfo.width = mFbDevice->width;
    mFbInfo.height = mFbDevice->height;
    mFbInfo.format = mFbDevice->format;
    mFbInfo.vsync_period_ns = int(1e9 / mFbDevice->fps);
    mFbInfo.xdpi_scaled = int(mFbDevice->xdpi * 1000.0f);
    mFbInfo.ydpi_scaled = int(mFbDevice->ydpi * 1000.0f);

    // Present fences aren't supported, always indicate PresentFenceIsNotReliable
    // for FB devices
    mCapabilities.insert(Capability::PresentFenceIsNotReliable);

    mVsyncThread.start(0, mFbInfo.vsync_period_ns);
}

HWC2OnFbAdapter& HWC2OnFbAdapter::cast(hw_device_t* device) {
    return *reinterpret_cast<HWC2OnFbAdapter*>(device);
}

HWC2OnFbAdapter& HWC2OnFbAdapter::cast(hwc2_device_t* device) {
    return *reinterpret_cast<HWC2OnFbAdapter*>(device);
}

hwc2_display_t HWC2OnFbAdapter::getDisplayId() {
    return 0;
}

hwc2_config_t HWC2OnFbAdapter::getConfigId() {
    return 0;
}

void HWC2OnFbAdapter::close() {
    mVsyncThread.stop();
    framebuffer_close(mFbDevice);
}

const HWC2OnFbAdapter::Info& HWC2OnFbAdapter::getInfo() const {
    return mFbInfo;
}

void HWC2OnFbAdapter::updateDebugString() {
    if (mFbDevice->common.version >= 1 && mFbDevice->dump) {
        char buffer[4096];
        mFbDevice->dump(mFbDevice, buffer, sizeof(buffer));
        buffer[sizeof(buffer) - 1] = '\0';

        mDebugString = buffer;
    }
}

const std::string& HWC2OnFbAdapter::getDebugString() const {
    return mDebugString;
}

void HWC2OnFbAdapter::setState(State state) {
    mState = state;
}

HWC2OnFbAdapter::State HWC2OnFbAdapter::getState() const {
    return mState;
}

hwc2_layer_t HWC2OnFbAdapter::addLayer() {
    hwc2_layer_t id = ++mNextLayerId;

    mLayers.insert(id);
    mDirtyLayers.insert(id);

    return id;
}

bool HWC2OnFbAdapter::removeLayer(hwc2_layer_t layer) {
    mDirtyLayers.erase(layer);
    return mLayers.erase(layer);
}

bool HWC2OnFbAdapter::hasLayer(hwc2_layer_t layer) const {
    return mLayers.count(layer) > 0;
}

bool HWC2OnFbAdapter::markLayerDirty(hwc2_layer_t layer, bool dirty) {
    if (mLayers.count(layer) == 0) {
        return false;
    }

    if (dirty) {
        mDirtyLayers.insert(layer);
    } else {
        mDirtyLayers.erase(layer);
    }

    return true;
}

const std::unordered_set<hwc2_layer_t>& HWC2OnFbAdapter::getDirtyLayers() const {
    return mDirtyLayers;
}

void HWC2OnFbAdapter::clearDirtyLayers() {
    mDirtyLayers.clear();
}

/*
 * For each frame, SurfaceFlinger
 *
 *  - peforms GLES composition
 *  - calls eglSwapBuffers
 *  - calls setClientTarget, which maps to setBuffer below
 *  - calls presentDisplay, which maps to postBuffer below
 *
 * setBuffer should be a good place to call compositionComplete.
 *
 * As for post, it
 *
 *  - schedules the buffer for presentation on the next vsync
 *  - locks the buffer and blocks all other users trying to lock it
 *
 * It does not give us a way to return a present fence, and we need to live
 * with that.  The implication is that, when we are double-buffered,
 * SurfaceFlinger assumes the front buffer is available for rendering again
 * immediately after the back buffer is posted.  The locking semantics
 * hopefully are strong enough that the rendering will be blocked.
 */
void HWC2OnFbAdapter::setBuffer(buffer_handle_t buffer) {
    if (mFbDevice->compositionComplete) {
        mFbDevice->compositionComplete(mFbDevice);
    }
    mBuffer = buffer;
}

bool HWC2OnFbAdapter::postBuffer() {
    int error = 0;
    if (mBuffer) {
        error = mFbDevice->post(mFbDevice, mBuffer);
    }

    return error == 0;
}

void HWC2OnFbAdapter::setVsyncCallback(HWC2_PFN_VSYNC callback, hwc2_callback_data_t data) {
    mVsyncThread.setCallback(callback, data);
}

void HWC2OnFbAdapter::enableVsync(bool enable) {
    mVsyncThread.enableCallback(enable);
}

void HWC2OnFbAdapter::getCapabilities(uint32_t* outCount,
                                      int32_t* outCapabilities) {
    if (outCapabilities == nullptr) {
        *outCount = mCapabilities.size();
        return;
    }

    auto capabilityIter = mCapabilities.cbegin();
    for (size_t written = 0; written < *outCount; ++written) {
        if (capabilityIter == mCapabilities.cend()) {
            return;
        }
        outCapabilities[written] = static_cast<int32_t>(*capabilityIter);
        ++capabilityIter;
    }
}

int64_t HWC2OnFbAdapter::VsyncThread::now() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    return int64_t(ts.tv_sec) * 1'000'000'000 + ts.tv_nsec;
}

bool HWC2OnFbAdapter::VsyncThread::sleepUntil(int64_t t) {
    struct timespec ts;
    ts.tv_sec = t / 1'000'000'000;
    ts.tv_nsec = t % 1'000'000'000;

    while (true) {
        int error = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ts, nullptr);
        if (error) {
            if (error == EINTR) {
                continue;
            }
            return false;
        } else {
            return true;
        }
    }
}

void HWC2OnFbAdapter::VsyncThread::start(int64_t firstVsync, int64_t period) {
    mNextVsync = firstVsync;
    mPeriod = period;
    mStarted = true;
    mThread = std::thread(&VsyncThread::vsyncLoop, this);
}

void HWC2OnFbAdapter::VsyncThread::stop() {
    {
        std::lock_guard<std::mutex> lock(mMutex);
        mStarted = false;
    }
    mCondition.notify_all();
    mThread.join();
}

void HWC2OnFbAdapter::VsyncThread::setCallback(HWC2_PFN_VSYNC callback, hwc2_callback_data_t data) {
    std::lock_guard<std::mutex> lock(mMutex);
    mCallback = callback;
    mCallbackData = data;
}

void HWC2OnFbAdapter::VsyncThread::enableCallback(bool enable) {
    {
        std::lock_guard<std::mutex> lock(mMutex);
        mCallbackEnabled = enable;
    }
    mCondition.notify_all();
}

void HWC2OnFbAdapter::VsyncThread::vsyncLoop() {
    prctl(PR_SET_NAME, "VsyncThread", 0, 0, 0);

    std::unique_lock<std::mutex> lock(mMutex);
    if (!mStarted) {
        return;
    }

    while (true) {
        if (!mCallbackEnabled) {
            mCondition.wait(lock, [this] { return mCallbackEnabled || !mStarted; });
            if (!mStarted) {
                break;
            }
        }

        lock.unlock();

        // adjust mNextVsync if necessary
        int64_t t = now();
        if (mNextVsync < t) {
            int64_t n = (t - mNextVsync + mPeriod - 1) / mPeriod;
            mNextVsync += mPeriod * n;
        }
        bool fire = sleepUntil(mNextVsync);

        lock.lock();

        if (fire) {
            ALOGV("VsyncThread(%" PRId64 ")", mNextVsync);
            if (mCallback) {
                mCallback(mCallbackData, getDisplayId(), mNextVsync);
            }
            mNextVsync += mPeriod;
        }
    }
}

} // namespace android
