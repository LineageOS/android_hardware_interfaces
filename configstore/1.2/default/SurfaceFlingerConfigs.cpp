/*
 * Copyright (C) 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.1 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.1
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "SurfaceFlingerConfigs.h"

#include <android/hardware/configstore/1.1/types.h>
#include <android/hardware/configstore/1.2/types.h>
#include <android/hardware/graphics/common/1.1/types.h>
#include <log/log.h>

namespace android {
namespace hardware {
namespace configstore {
namespace V1_2 {
namespace implementation {

using ::android::hardware::graphics::common::V1_1::PixelFormat;
using ::android::hardware::graphics::common::V1_2::Dataspace;

// ::android::hardware::configstore::V1_0::ISurfaceFlingerConfigs implementation.
Return<void> SurfaceFlingerConfigs::vsyncEventPhaseOffsetNs(vsyncEventPhaseOffsetNs_cb _hidl_cb) {
#ifdef VSYNC_EVENT_PHASE_OFFSET_NS
    _hidl_cb({true, VSYNC_EVENT_PHASE_OFFSET_NS});
#else
    _hidl_cb({false, 0});
#endif
    return Void();
}

Return<void> SurfaceFlingerConfigs::vsyncSfEventPhaseOffsetNs(vsyncEventPhaseOffsetNs_cb _hidl_cb) {
#ifdef SF_VSYNC_EVENT_PHASE_OFFSET_NS
    _hidl_cb({true, SF_VSYNC_EVENT_PHASE_OFFSET_NS});
#else
    _hidl_cb({false, 0});
#endif
    return Void();
}

Return<void> SurfaceFlingerConfigs::useContextPriority(useContextPriority_cb _hidl_cb) {
#ifdef USE_CONTEXT_PRIORITY
    _hidl_cb({true, USE_CONTEXT_PRIORITY});
#else
    _hidl_cb({false, false});
#endif
    return Void();
}

Return<void> SurfaceFlingerConfigs::maxFrameBufferAcquiredBuffers(
    maxFrameBufferAcquiredBuffers_cb _hidl_cb) {
#ifdef NUM_FRAMEBUFFER_SURFACE_BUFFERS
    _hidl_cb({true, NUM_FRAMEBUFFER_SURFACE_BUFFERS});
#else
    _hidl_cb({false, 0});
#endif
    return Void();
}

Return<void> SurfaceFlingerConfigs::hasWideColorDisplay(hasWideColorDisplay_cb _hidl_cb) {
    bool value = false;
#ifdef HAS_WIDE_COLOR_DISPLAY
    value = true;
#endif
    _hidl_cb({true, value});
    return Void();
}

Return<void> SurfaceFlingerConfigs::hasSyncFramework(hasSyncFramework_cb _hidl_cb) {
    bool value = true;
#ifdef RUNNING_WITHOUT_SYNC_FRAMEWORK
    value = false;
#endif
    _hidl_cb({true, value});
    return Void();
}

Return<void> SurfaceFlingerConfigs::hasHDRDisplay(hasHDRDisplay_cb _hidl_cb) {
    bool value = false;
#ifdef HAS_HDR_DISPLAY
    value = true;
#endif
    _hidl_cb({true, value});
    return Void();
}

Return<void> SurfaceFlingerConfigs::presentTimeOffsetFromVSyncNs(
    presentTimeOffsetFromVSyncNs_cb _hidl_cb) {
#ifdef PRESENT_TIME_OFFSET_FROM_VSYNC_NS
    _hidl_cb({true, PRESENT_TIME_OFFSET_FROM_VSYNC_NS});
#else
    _hidl_cb({false, 0});
#endif
    return Void();
}

Return<void> SurfaceFlingerConfigs::useHwcForRGBtoYUV(useHwcForRGBtoYUV_cb _hidl_cb) {
    bool value = false;
#ifdef FORCE_HWC_COPY_FOR_VIRTUAL_DISPLAYS
    value = true;
#endif
    _hidl_cb({true, value});
    return Void();
}

Return<void> SurfaceFlingerConfigs::maxVirtualDisplaySize(maxVirtualDisplaySize_cb _hidl_cb) {
    uint64_t maxSize = 0;
#ifdef MAX_VIRTUAL_DISPLAY_DIMENSION
    maxSize = MAX_VIRTUAL_DISPLAY_DIMENSION;
    _hidl_cb({true, maxSize});
#else
    _hidl_cb({false, maxSize});
#endif
    return Void();
}

Return<void> SurfaceFlingerConfigs::useVrFlinger(useVrFlinger_cb _hidl_cb) {
    bool value = false;
    bool specified = false;
#ifdef USE_VR_FLINGER
    value = true;
    specified = true;
#endif
    _hidl_cb({specified, value});
    return Void();
}

Return<void> SurfaceFlingerConfigs::startGraphicsAllocatorService(
    startGraphicsAllocatorService_cb _hidl_cb) {
    bool value = false;
#ifdef START_GRAPHICS_ALLOCATOR_SERVICE
    value = true;
#endif
    _hidl_cb({true, value});
    return Void();
}

// ::android::hardware::configstore::V1_1::ISurfaceFlingerConfigs implementation.

#ifdef PRIMARY_DISPLAY_ORIENTATION
static_assert(PRIMARY_DISPLAY_ORIENTATION == 0 || PRIMARY_DISPLAY_ORIENTATION == 90 ||
                  PRIMARY_DISPLAY_ORIENTATION == 180 || PRIMARY_DISPLAY_ORIENTATION == 270,
              "Primary display orientation must be 0/90/180/270");
#endif

Return<void> SurfaceFlingerConfigs::primaryDisplayOrientation(
    primaryDisplayOrientation_cb _hidl_cb) {
    using ::android::hardware::configstore::V1_1::DisplayOrientation;

    bool specified = false;
    DisplayOrientation value = DisplayOrientation::ORIENTATION_0;

    int orientation = 0;
#ifdef PRIMARY_DISPLAY_ORIENTATION
    specified = true;
    orientation = PRIMARY_DISPLAY_ORIENTATION;
#endif

    switch (orientation) {
        case 0: {
            value = DisplayOrientation::ORIENTATION_0;
            break;
        }
        case 90: {
            value = DisplayOrientation::ORIENTATION_90;
            break;
        }
        case 180: {
            value = DisplayOrientation::ORIENTATION_180;
            break;
        }
        case 270: {
            value = DisplayOrientation::ORIENTATION_270;
            break;
        }
        default: {
            // statically checked above -> memory corruption
            LOG_ALWAYS_FATAL("Invalid orientation %d", orientation);
        }
    }

    _hidl_cb({specified, value});
    return Void();
}

// ::android::hardware::configstore::V1_2::ISurfaceFlingerConfigs implementation.
Return<void> SurfaceFlingerConfigs::useColorManagement(useColorManagement_cb _hidl_cb) {
#if defined(USE_COLOR_MANAGEMENT) || defined(HAS_WIDE_COLOR_DISPLAY) || defined(HAS_HDR_DISPLAY)
    _hidl_cb({true, true});
#else
    _hidl_cb({true, false});
#endif
    return Void();
}

#ifdef DEFAULT_COMPOSITION_DATA_SPACE
static_assert(DEFAULT_COMPOSITION_DATA_SPACE != 0,
              "Default composition data space must not be UNKNOWN");
#endif

#ifdef WCG_COMPOSITION_DATA_SPACE
static_assert(WCG_COMPOSITION_DATA_SPACE != 0,
              "Wide color gamut composition data space must not be UNKNOWN");
#endif

Return<void> SurfaceFlingerConfigs::getCompositionPreference(getCompositionPreference_cb _hidl_cb) {
    Dataspace defaultDataspace = Dataspace::V0_SRGB;
    PixelFormat defaultPixelFormat = PixelFormat::RGBA_8888;

#ifdef DEFAULT_COMPOSITION_DATA_SPACE
    defaultDataspace = static_cast<Dataspace>(DEFAULT_COMPOSITION_DATA_SPACE);
#endif

#ifdef DEFAULT_COMPOSITION_PIXEL_FORMAT
    defaultPixelFormat = static_cast<PixelFormat>(DEFAULT_COMPOSITION_PIXEL_FORMAT);
#endif

    Dataspace wideColorGamutDataspace = Dataspace::V0_SRGB;
    PixelFormat wideColorGamutPixelFormat = PixelFormat::RGBA_8888;

#ifdef WCG_COMPOSITION_DATA_SPACE
    wideColorGamutDataspace = static_cast<Dataspace>(WCG_COMPOSITION_DATA_SPACE);
#endif

#ifdef WCG_COMPOSITION_PIXEL_FORMAT
    wideColorGamutPixelFormat = static_cast<PixelFormat>(WCG_COMPOSITION_PIXEL_FORMAT);
#endif

    _hidl_cb(defaultDataspace, defaultPixelFormat, wideColorGamutDataspace,
             wideColorGamutPixelFormat);
    return Void();
}

Return<void> SurfaceFlingerConfigs::getDisplayNativePrimaries(getDisplayNativePrimaries_cb _hidl_cb) {
    DisplayPrimaries primaries;
    // The default XYZ is sRGB gamut in CIE1931 color space
#ifdef TARGET_DISPLAY_PRIMARY_RED_X
    primaries.red.X = TARGET_DISPLAY_PRIMARY_RED_X;
#else
    primaries.red.X = 0.4123;
#endif

#ifdef TARGET_DISPLAY_PRIMARY_RED_Y
    primaries.red.Y = TARGET_DISPLAY_PRIMARY_RED_Y;
#else
    primaries.red.Y = 0.2126;
#endif

#ifdef TARGET_DISPLAY_PRIMARY_RED_Z
    primaries.red.Z = TARGET_DISPLAY_PRIMARY_RED_Z;
#else
    primaries.red.Z = 0.0193;
#endif

#ifdef TARGET_DISPLAY_PRIMARY_GREEN_X
    primaries.green.X = TARGET_DISPLAY_PRIMARY_GREEN_X;
#else
    primaries.green.X = 0.3576;
#endif

#ifdef TARGET_DISPLAY_PRIMARY_GREEN_Y
    primaries.green.Y = TARGET_DISPLAY_PRIMARY_GREEN_Y;
#else
    primaries.green.Y = 0.7152;
#endif

#ifdef TARGET_DISPLAY_PRIMARY_GREEN_Z
    primaries.green.Z = TARGET_DISPLAY_PRIMARY_GREEN_Z;
#else
    primaries.green.Z = 0.1192;
#endif

#ifdef TARGET_DISPLAY_PRIMARY_BLUE_X
    primaries.blue.X = TARGET_DISPLAY_PRIMARY_BLUE_X;
#else
    primaries.blue.X = 0.1805;
#endif

#ifdef TARGET_DISPLAY_PRIMARY_BLUE_Y
    primaries.blue.Y = TARGET_DISPLAY_PRIMARY_BLUE_Y;
#else
    primaries.blue.Y = 0.0722;
#endif

#ifdef TARGET_DISPLAY_PRIMARY_BLUE_Z
    primaries.blue.Z = TARGET_DISPLAY_PRIMARY_BLUE_Z;
#else
    primaries.blue.Z = 0.9506;
#endif

#ifdef TARGET_DISPLAY_PRIMARY_WHITE_X
    primaries.white.X = TARGET_DISPLAY_PRIMARY_WHITE_X;
#else
    primaries.white.X = 0.9505;
#endif

#ifdef TARGET_DISPLAY_PRIMARY_WHITE_Y
    primaries.white.Y = TARGET_DISPLAY_PRIMARY_WHITE_Y;
#else
    primaries.white.Y = 1.0000;
#endif

#ifdef TARGET_DISPLAY_PRIMARY_WHITE_Z
    primaries.white.Z = TARGET_DISPLAY_PRIMARY_WHITE_Z;
#else
    primaries.white.Z = 1.0891;
#endif

    _hidl_cb(primaries);
    return Void();
}

}  // namespace implementation
}  // namespace V1_2
}  // namespace configstore
}  // namespace hardware
}  // namespace android
