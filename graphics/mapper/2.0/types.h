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

#ifndef ANDROID_HARDWARE_GRAPHICS_MAPPER_V2_0_TYPES_H
#define ANDROID_HARDWARE_GRAPHICS_MAPPER_V2_0_TYPES_H

#include <type_traits>

#include <android/hardware/graphics/allocator/2.0/types.h>

namespace android {
namespace hardware {
namespace graphics {
namespace mapper {
namespace V2_0 {

using android::hardware::graphics::allocator::V2_0::Error;
using android::hardware::graphics::allocator::V2_0::PixelFormat;
using android::hardware::graphics::allocator::V2_0::ProducerUsage;
using android::hardware::graphics::allocator::V2_0::ConsumerUsage;

/*
 * Structures for describing flexible YUVA/RGBA formats for consumption by
 * applications. Such flexible formats contain a plane for each component
 * (e.g.  red, green, blue), where each plane is laid out in a grid-like
 * pattern occupying unique byte addresses and with consistent byte offsets
 * between neighboring pixels.
 *
 * The FlexLayout structure is used with any pixel format that can be
 * represented by it, such as:
 *
 *  - PixelFormat::YCbCr_*_888
 *  - PixelFormat::FLEX_RGB*_888
 *  - PixelFormat::RGB[AX]_888[8],BGRA_8888,RGB_888
 *  - PixelFormat::YV12,Y8,Y16,YCbCr_422_SP/I,YCrCb_420_SP
 *  - even implementation defined formats that can be represented by the
 *    structures
 *
 * Vertical increment (aka. row increment or stride) describes the distance in
 * bytes from the first pixel of one row to the first pixel of the next row
 * (below) for the component plane. This can be negative.
 *
 * Horizontal increment (aka. column or pixel increment) describes the
 * distance in bytes from one pixel to the next pixel (to the right) on the
 * same row for the component plane. This can be negative.
 *
 * Each plane can be subsampled either vertically or horizontally by a
 * power-of-two factor.
 *
 * The bit-depth of each component can be arbitrary, as long as the pixels are
 * laid out on whole bytes, in native byte-order, using the most significant
 * bits of each unit.
 */

enum class FlexComponent : int32_t {
    Y          = 1 << 0,  /* luma */
    Cb         = 1 << 1,  /* chroma blue */
    Cr         = 1 << 2,  /* chroma red */

    R          = 1 << 10, /* red */
    G          = 1 << 11, /* green */
    B          = 1 << 12, /* blue */

    A          = 1 << 30, /* alpha */
};

inline FlexComponent operator|(FlexComponent lhs, FlexComponent rhs)
{
    return static_cast<FlexComponent>(static_cast<int32_t>(lhs) |
                                      static_cast<int32_t>(rhs));
}

inline FlexComponent& operator|=(FlexComponent &lhs, FlexComponent rhs)
{
    lhs = static_cast<FlexComponent>(static_cast<int32_t>(lhs) |
                                     static_cast<int32_t>(rhs));
    return lhs;
}

enum class FlexFormat : int32_t {
    /* not a flexible format */
    INVALID    = 0x0,

    Y          = static_cast<int32_t>(FlexComponent::Y),
    YCbCr      = static_cast<int32_t>(FlexComponent::Y) |
                 static_cast<int32_t>(FlexComponent::Cb) |
                 static_cast<int32_t>(FlexComponent::Cr),
    YCbCrA     = static_cast<int32_t>(YCbCr) |
                 static_cast<int32_t>(FlexComponent::A),
    RGB        = static_cast<int32_t>(FlexComponent::R) |
                 static_cast<int32_t>(FlexComponent::G) |
                 static_cast<int32_t>(FlexComponent::B),
    RGBA       = static_cast<int32_t>(RGB) |
                 static_cast<int32_t>(FlexComponent::A),
};

struct FlexPlane {
    /* pointer to the first byte of the top-left pixel of the plane. */
    uint8_t *topLeft;

    FlexComponent component;

    /*
     * bits allocated for the component in each pixel. Must be a positive
     * multiple of 8.
     */
    int32_t bitsPerComponent;

    /*
     * number of the most significant bits used in the format for this
     * component. Must be between 1 and bits_per_component, inclusive.
     */
    int32_t bitsUsed;

    /* horizontal increment */
    int32_t hIncrement;
    /* vertical increment */
    int32_t vIncrement;

    /* horizontal subsampling. Must be a positive power of 2. */
    int32_t hSubsampling;
    /* vertical subsampling. Must be a positive power of 2. */
    int32_t vSubsampling;
};
static_assert(std::is_pod<FlexPlane>::value, "FlexPlane is not POD");

struct FlexLayout {
    /* the kind of flexible format */
    FlexFormat format;

    /* number of planes; 0 for FLEX_FORMAT_INVALID */
    uint32_t numPlanes;

    /*
     * a plane for each component; ordered in increasing component value order.
     * E.g. FLEX_FORMAT_RGBA maps 0 -> R, 1 -> G, etc.
     * Can be NULL for FLEX_FORMAT_INVALID
     */
    FlexPlane* planes;
};
static_assert(std::is_pod<FlexLayout>::value, "FlexLayout is not POD");

typedef uint64_t BackingStore;

} // namespace V2_0
} // namespace mapper
} // namespace graphics
} // namespace hardware
} // namespace android

#endif /* ANDROID_HARDWARE_GRAPHICS_MAPPER_V2_0_TYPES_H */
