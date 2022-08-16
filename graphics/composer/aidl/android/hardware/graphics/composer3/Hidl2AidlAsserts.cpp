/**
 * Copyright (c) 2021, The Android Open Source Project
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

#include "aidl/android/hardware/graphics/common/BlendMode.h"
#include "aidl/android/hardware/graphics/common/FRect.h"
#include "aidl/android/hardware/graphics/common/Rect.h"
#include "aidl/android/hardware/graphics/composer3/Capability.h"
#include "aidl/android/hardware/graphics/composer3/ClientTargetProperty.h"
#include "aidl/android/hardware/graphics/composer3/Color.h"
#include "aidl/android/hardware/graphics/composer3/Composition.h"
#include "aidl/android/hardware/graphics/composer3/ContentType.h"
#include "aidl/android/hardware/graphics/composer3/DisplayAttribute.h"
#include "aidl/android/hardware/graphics/composer3/DisplayCapability.h"
#include "aidl/android/hardware/graphics/composer3/DisplayConnectionType.h"
#include "aidl/android/hardware/graphics/composer3/FormatColorComponent.h"
#include "aidl/android/hardware/graphics/composer3/IComposer.h"
#include "aidl/android/hardware/graphics/composer3/PerFrameMetadata.h"
#include "aidl/android/hardware/graphics/composer3/PerFrameMetadataBlob.h"
#include "aidl/android/hardware/graphics/composer3/PerFrameMetadataKey.h"
#include "aidl/android/hardware/graphics/composer3/PowerMode.h"
#include "aidl/android/hardware/graphics/composer3/VsyncPeriodChangeConstraints.h"
#include "aidl/android/hardware/graphics/composer3/VsyncPeriodChangeTimeline.h"
#include "android/hardware/graphics/composer/2.1/IComposer.h"
#include "android/hardware/graphics/composer/2.1/IComposerCallback.h"
#include "android/hardware/graphics/composer/2.1/IComposerClient.h"
#include "android/hardware/graphics/composer/2.2/IComposerClient.h"
#include "android/hardware/graphics/composer/2.3/IComposerClient.h"
#include "android/hardware/graphics/composer/2.4/IComposerClient.h"
#include "android/hardware/graphics/composer/2.4/types.h"

namespace android::h2a {

static_assert(
        aidl::android::hardware::graphics::composer3::IComposer::EX_NO_RESOURCES ==
        static_cast<int32_t>(::android::hardware::graphics::composer::V2_4::Error::NO_RESOURCES));

static_assert(
        aidl::android::hardware::graphics::composer3::IComposerClient::EX_BAD_CONFIG ==
        static_cast<int32_t>(::android::hardware::graphics::composer::V2_4::Error::BAD_CONFIG));
static_assert(
        aidl::android::hardware::graphics::composer3::IComposerClient::EX_BAD_DISPLAY ==
        static_cast<int32_t>(::android::hardware::graphics::composer::V2_4::Error::BAD_DISPLAY));
static_assert(
        aidl::android::hardware::graphics::composer3::IComposerClient::EX_BAD_LAYER ==
        static_cast<int32_t>(::android::hardware::graphics::composer::V2_4::Error::BAD_LAYER));
static_assert(
        aidl::android::hardware::graphics::composer3::IComposerClient::EX_BAD_PARAMETER ==
        static_cast<int32_t>(::android::hardware::graphics::composer::V2_4::Error::BAD_PARAMETER));
static_assert(
        aidl::android::hardware::graphics::composer3::IComposerClient::EX_NO_RESOURCES ==
        static_cast<int32_t>(::android::hardware::graphics::composer::V2_4::Error::NO_RESOURCES));
static_assert(
        aidl::android::hardware::graphics::composer3::IComposerClient::EX_NOT_VALIDATED ==
        static_cast<int32_t>(::android::hardware::graphics::composer::V2_4::Error::NOT_VALIDATED));
static_assert(
        aidl::android::hardware::graphics::composer3::IComposerClient::EX_UNSUPPORTED ==
        static_cast<int32_t>(::android::hardware::graphics::composer::V2_4::Error::UNSUPPORTED));
static_assert(
        aidl::android::hardware::graphics::composer3::IComposerClient::EX_SEAMLESS_NOT_ALLOWED ==
        static_cast<int32_t>(
                ::android::hardware::graphics::composer::V2_4::Error::SEAMLESS_NOT_ALLOWED));
static_assert(
        aidl::android::hardware::graphics::composer3::IComposerClient::EX_SEAMLESS_NOT_POSSIBLE ==
        static_cast<int32_t>(
                ::android::hardware::graphics::composer::V2_4::Error::SEAMLESS_NOT_POSSIBLE));

static_assert(
        aidl::android::hardware::graphics::composer3::Capability::INVALID ==
        static_cast<aidl::android::hardware::graphics::composer3::Capability>(
                ::android::hardware::graphics::composer::V2_1::IComposer::Capability::INVALID));
static_assert(aidl::android::hardware::graphics::composer3::Capability::SIDEBAND_STREAM ==
              static_cast<aidl::android::hardware::graphics::composer3::Capability>(
                      ::android::hardware::graphics::composer::V2_1::IComposer::Capability::
                              SIDEBAND_STREAM));
static_assert(
        aidl::android::hardware::graphics::composer3::Capability::SKIP_CLIENT_COLOR_TRANSFORM ==
        static_cast<aidl::android::hardware::graphics::composer3::Capability>(
                ::android::hardware::graphics::composer::V2_1::IComposer::Capability::
                        SKIP_CLIENT_COLOR_TRANSFORM));
static_assert(
        aidl::android::hardware::graphics::composer3::Capability::PRESENT_FENCE_IS_NOT_RELIABLE ==
        static_cast<aidl::android::hardware::graphics::composer3::Capability>(
                ::android::hardware::graphics::composer::V2_1::IComposer::Capability::
                        PRESENT_FENCE_IS_NOT_RELIABLE));
// HWC2_CAPABILITY_SKIP_VALIDATE was never defined for HIDL, so we just hardcode its value
static_assert(aidl::android::hardware::graphics::composer3::Capability::SKIP_VALIDATE ==
              static_cast<aidl::android::hardware::graphics::composer3::Capability>(4));

static_assert(aidl::android::hardware::graphics::composer3::DisplayRequest::LayerRequest::
                      CLEAR_CLIENT_TARGET ==
              static_cast<int>(::android::hardware::graphics::composer::V2_1::IComposerClient::
                                       LayerRequest::CLEAR_CLIENT_TARGET));

static_assert(aidl::android::hardware::graphics::common::BlendMode::INVALID ==
              static_cast<aidl::android::hardware::graphics::common::BlendMode>(
                      ::android::hardware::graphics::composer::V2_1::IComposerClient::BlendMode::
                              INVALID));
static_assert(
        aidl::android::hardware::graphics::common::BlendMode::NONE ==
        static_cast<aidl::android::hardware::graphics::common::BlendMode>(
                ::android::hardware::graphics::composer::V2_1::IComposerClient::BlendMode::NONE));
static_assert(aidl::android::hardware::graphics::common::BlendMode::PREMULTIPLIED ==
              static_cast<aidl::android::hardware::graphics::common::BlendMode>(
                      ::android::hardware::graphics::composer::V2_1::IComposerClient::BlendMode::
                              PREMULTIPLIED));
static_assert(aidl::android::hardware::graphics::common::BlendMode::COVERAGE ==
              static_cast<aidl::android::hardware::graphics::common::BlendMode>(
                      ::android::hardware::graphics::composer::V2_1::IComposerClient::BlendMode::
                              COVERAGE));

static_assert(aidl::android::hardware::graphics::composer3::Composition::INVALID ==
              static_cast<aidl::android::hardware::graphics::composer3::Composition>(
                      ::android::hardware::graphics::composer::V2_1::IComposerClient::Composition::
                              INVALID));
static_assert(aidl::android::hardware::graphics::composer3::Composition::CLIENT ==
              static_cast<aidl::android::hardware::graphics::composer3::Composition>(
                      ::android::hardware::graphics::composer::V2_1::IComposerClient::Composition::
                              CLIENT));
static_assert(aidl::android::hardware::graphics::composer3::Composition::DEVICE ==
              static_cast<aidl::android::hardware::graphics::composer3::Composition>(
                      ::android::hardware::graphics::composer::V2_1::IComposerClient::Composition::
                              DEVICE));
static_assert(aidl::android::hardware::graphics::composer3::Composition::SOLID_COLOR ==
              static_cast<aidl::android::hardware::graphics::composer3::Composition>(
                      ::android::hardware::graphics::composer::V2_1::IComposerClient::Composition::
                              SOLID_COLOR));
static_assert(aidl::android::hardware::graphics::composer3::Composition::CURSOR ==
              static_cast<aidl::android::hardware::graphics::composer3::Composition>(
                      ::android::hardware::graphics::composer::V2_1::IComposerClient::Composition::
                              CURSOR));
static_assert(aidl::android::hardware::graphics::composer3::Composition::SIDEBAND ==
              static_cast<aidl::android::hardware::graphics::composer3::Composition>(
                      ::android::hardware::graphics::composer::V2_1::IComposerClient::Composition::
                              SIDEBAND));

static_assert(aidl::android::hardware::graphics::composer3::DisplayRequest::FLIP_CLIENT_TARGET ==
              static_cast<int>(::android::hardware::graphics::composer::V2_1::IComposerClient::
                                       DisplayRequest::FLIP_CLIENT_TARGET));
static_assert(aidl::android::hardware::graphics::composer3::DisplayRequest::
                      WRITE_CLIENT_TARGET_TO_OUTPUT ==
              static_cast<int>(::android::hardware::graphics::composer::V2_1::IComposerClient::
                                       DisplayRequest::WRITE_CLIENT_TARGET_TO_OUTPUT));

static_assert(
        aidl::android::hardware::graphics::composer3::PowerMode::OFF ==
        static_cast<aidl::android::hardware::graphics::composer3::PowerMode>(
                ::android::hardware::graphics::composer::V2_2::IComposerClient::PowerMode::OFF));
static_assert(
        aidl::android::hardware::graphics::composer3::PowerMode::DOZE ==
        static_cast<aidl::android::hardware::graphics::composer3::PowerMode>(
                ::android::hardware::graphics::composer::V2_2::IComposerClient::PowerMode::DOZE));
static_assert(aidl::android::hardware::graphics::composer3::PowerMode::DOZE_SUSPEND ==
              static_cast<aidl::android::hardware::graphics::composer3::PowerMode>(
                      ::android::hardware::graphics::composer::V2_2::IComposerClient::PowerMode::
                              DOZE_SUSPEND));
static_assert(
        aidl::android::hardware::graphics::composer3::PowerMode::ON ==
        static_cast<aidl::android::hardware::graphics::composer3::PowerMode>(
                ::android::hardware::graphics::composer::V2_2::IComposerClient::PowerMode::ON));
static_assert(aidl::android::hardware::graphics::composer3::PowerMode::ON_SUSPEND ==
              static_cast<aidl::android::hardware::graphics::composer3::PowerMode>(
                      ::android::hardware::graphics::composer::V2_2::IComposerClient::PowerMode::
                              ON_SUSPEND));

static_assert(aidl::android::hardware::graphics::composer3::DisplayCapability::INVALID ==
              static_cast<aidl::android::hardware::graphics::composer3::DisplayCapability>(
                      ::android::hardware::graphics::composer::V2_4::IComposerClient::
                              DisplayCapability::INVALID));
static_assert(aidl::android::hardware::graphics::composer3::DisplayCapability::
                      SKIP_CLIENT_COLOR_TRANSFORM ==
              static_cast<aidl::android::hardware::graphics::composer3::DisplayCapability>(
                      ::android::hardware::graphics::composer::V2_4::IComposerClient::
                              DisplayCapability::SKIP_CLIENT_COLOR_TRANSFORM));
static_assert(aidl::android::hardware::graphics::composer3::DisplayCapability::DOZE ==
              static_cast<aidl::android::hardware::graphics::composer3::DisplayCapability>(
                      ::android::hardware::graphics::composer::V2_4::IComposerClient::
                              DisplayCapability::DOZE));
static_assert(aidl::android::hardware::graphics::composer3::DisplayCapability::BRIGHTNESS ==
              static_cast<aidl::android::hardware::graphics::composer3::DisplayCapability>(
                      ::android::hardware::graphics::composer::V2_4::IComposerClient::
                              DisplayCapability::BRIGHTNESS));
static_assert(aidl::android::hardware::graphics::composer3::DisplayCapability::PROTECTED_CONTENTS ==
              static_cast<aidl::android::hardware::graphics::composer3::DisplayCapability>(
                      ::android::hardware::graphics::composer::V2_4::IComposerClient::
                              DisplayCapability::PROTECTED_CONTENTS));
static_assert(
        aidl::android::hardware::graphics::composer3::DisplayCapability::AUTO_LOW_LATENCY_MODE ==
        static_cast<aidl::android::hardware::graphics::composer3::DisplayCapability>(
                ::android::hardware::graphics::composer::V2_4::IComposerClient::DisplayCapability::
                        AUTO_LOW_LATENCY_MODE));

static_assert(aidl::android::hardware::graphics::composer3::DisplayAttribute::INVALID ==
              static_cast<aidl::android::hardware::graphics::composer3::DisplayAttribute>(
                      ::android::hardware::graphics::composer::V2_4::IComposerClient::Attribute::
                              INVALID));
static_assert(
        aidl::android::hardware::graphics::composer3::DisplayAttribute::WIDTH ==
        static_cast<aidl::android::hardware::graphics::composer3::DisplayAttribute>(
                ::android::hardware::graphics::composer::V2_4::IComposerClient::Attribute::WIDTH));
static_assert(
        aidl::android::hardware::graphics::composer3::DisplayAttribute::HEIGHT ==
        static_cast<aidl::android::hardware::graphics::composer3::DisplayAttribute>(
                ::android::hardware::graphics::composer::V2_4::IComposerClient::Attribute::HEIGHT));
static_assert(aidl::android::hardware::graphics::composer3::DisplayAttribute::VSYNC_PERIOD ==
              static_cast<aidl::android::hardware::graphics::composer3::DisplayAttribute>(
                      ::android::hardware::graphics::composer::V2_4::IComposerClient::Attribute::
                              VSYNC_PERIOD));
static_assert(
        aidl::android::hardware::graphics::composer3::DisplayAttribute::DPI_X ==
        static_cast<aidl::android::hardware::graphics::composer3::DisplayAttribute>(
                ::android::hardware::graphics::composer::V2_4::IComposerClient::Attribute::DPI_X));
static_assert(
        aidl::android::hardware::graphics::composer3::DisplayAttribute::DPI_Y ==
        static_cast<aidl::android::hardware::graphics::composer3::DisplayAttribute>(
                ::android::hardware::graphics::composer::V2_4::IComposerClient::Attribute::DPI_Y));
static_assert(aidl::android::hardware::graphics::composer3::DisplayAttribute::CONFIG_GROUP ==
              static_cast<aidl::android::hardware::graphics::composer3::DisplayAttribute>(
                      ::android::hardware::graphics::composer::V2_4::IComposerClient::Attribute::
                              CONFIG_GROUP));

static_assert(aidl::android::hardware::graphics::composer3::DisplayConnectionType::INTERNAL ==
              static_cast<aidl::android::hardware::graphics::composer3::DisplayConnectionType>(
                      ::android::hardware::graphics::composer::V2_4::IComposerClient::
                              DisplayConnectionType::INTERNAL));
static_assert(aidl::android::hardware::graphics::composer3::DisplayConnectionType::EXTERNAL ==
              static_cast<aidl::android::hardware::graphics::composer3::DisplayConnectionType>(
                      ::android::hardware::graphics::composer::V2_4::IComposerClient::
                              DisplayConnectionType::EXTERNAL));

static_assert(
        aidl::android::hardware::graphics::composer3::PerFrameMetadataKey::DISPLAY_RED_PRIMARY_X ==
        static_cast<aidl::android::hardware::graphics::composer3::PerFrameMetadataKey>(
                ::android::hardware::graphics::composer::V2_3::IComposerClient::
                        PerFrameMetadataKey::DISPLAY_RED_PRIMARY_X));
static_assert(
        aidl::android::hardware::graphics::composer3::PerFrameMetadataKey::DISPLAY_RED_PRIMARY_Y ==
        static_cast<aidl::android::hardware::graphics::composer3::PerFrameMetadataKey>(
                ::android::hardware::graphics::composer::V2_3::IComposerClient::
                        PerFrameMetadataKey::DISPLAY_RED_PRIMARY_Y));
static_assert(aidl::android::hardware::graphics::composer3::PerFrameMetadataKey::
                      DISPLAY_GREEN_PRIMARY_X ==
              static_cast<aidl::android::hardware::graphics::composer3::PerFrameMetadataKey>(
                      ::android::hardware::graphics::composer::V2_3::IComposerClient::
                              PerFrameMetadataKey::DISPLAY_GREEN_PRIMARY_X));
static_assert(aidl::android::hardware::graphics::composer3::PerFrameMetadataKey::
                      DISPLAY_GREEN_PRIMARY_Y ==
              static_cast<aidl::android::hardware::graphics::composer3::PerFrameMetadataKey>(
                      ::android::hardware::graphics::composer::V2_3::IComposerClient::
                              PerFrameMetadataKey::DISPLAY_GREEN_PRIMARY_Y));
static_assert(
        aidl::android::hardware::graphics::composer3::PerFrameMetadataKey::DISPLAY_BLUE_PRIMARY_X ==
        static_cast<aidl::android::hardware::graphics::composer3::PerFrameMetadataKey>(
                ::android::hardware::graphics::composer::V2_3::IComposerClient::
                        PerFrameMetadataKey::DISPLAY_BLUE_PRIMARY_X));
static_assert(
        aidl::android::hardware::graphics::composer3::PerFrameMetadataKey::DISPLAY_BLUE_PRIMARY_Y ==
        static_cast<aidl::android::hardware::graphics::composer3::PerFrameMetadataKey>(
                ::android::hardware::graphics::composer::V2_3::IComposerClient::
                        PerFrameMetadataKey::DISPLAY_BLUE_PRIMARY_Y));
static_assert(aidl::android::hardware::graphics::composer3::PerFrameMetadataKey::WHITE_POINT_X ==
              static_cast<aidl::android::hardware::graphics::composer3::PerFrameMetadataKey>(
                      ::android::hardware::graphics::composer::V2_3::IComposerClient::
                              PerFrameMetadataKey::WHITE_POINT_X));
static_assert(aidl::android::hardware::graphics::composer3::PerFrameMetadataKey::WHITE_POINT_Y ==
              static_cast<aidl::android::hardware::graphics::composer3::PerFrameMetadataKey>(
                      ::android::hardware::graphics::composer::V2_3::IComposerClient::
                              PerFrameMetadataKey::WHITE_POINT_Y));
static_assert(aidl::android::hardware::graphics::composer3::PerFrameMetadataKey::MAX_LUMINANCE ==
              static_cast<aidl::android::hardware::graphics::composer3::PerFrameMetadataKey>(
                      ::android::hardware::graphics::composer::V2_3::IComposerClient::
                              PerFrameMetadataKey::MAX_LUMINANCE));
static_assert(aidl::android::hardware::graphics::composer3::PerFrameMetadataKey::MIN_LUMINANCE ==
              static_cast<aidl::android::hardware::graphics::composer3::PerFrameMetadataKey>(
                      ::android::hardware::graphics::composer::V2_3::IComposerClient::
                              PerFrameMetadataKey::MIN_LUMINANCE));
static_assert(aidl::android::hardware::graphics::composer3::PerFrameMetadataKey::
                      MAX_CONTENT_LIGHT_LEVEL ==
              static_cast<aidl::android::hardware::graphics::composer3::PerFrameMetadataKey>(
                      ::android::hardware::graphics::composer::V2_3::IComposerClient::
                              PerFrameMetadataKey::MAX_CONTENT_LIGHT_LEVEL));
static_assert(aidl::android::hardware::graphics::composer3::PerFrameMetadataKey::
                      MAX_FRAME_AVERAGE_LIGHT_LEVEL ==
              static_cast<aidl::android::hardware::graphics::composer3::PerFrameMetadataKey>(
                      ::android::hardware::graphics::composer::V2_3::IComposerClient::
                              PerFrameMetadataKey::MAX_FRAME_AVERAGE_LIGHT_LEVEL));
static_assert(aidl::android::hardware::graphics::composer3::PerFrameMetadataKey::HDR10_PLUS_SEI ==
              static_cast<aidl::android::hardware::graphics::composer3::PerFrameMetadataKey>(
                      ::android::hardware::graphics::composer::V2_3::IComposerClient::
                              PerFrameMetadataKey::HDR10_PLUS_SEI));

static_assert(
        aidl::android::hardware::graphics::composer3::FormatColorComponent::FORMAT_COMPONENT_0 ==
        static_cast<aidl::android::hardware::graphics::composer3::FormatColorComponent>(
                ::android::hardware::graphics::composer::V2_3::IComposerClient::
                        FormatColorComponent::FORMAT_COMPONENT_0));
static_assert(
        aidl::android::hardware::graphics::composer3::FormatColorComponent::FORMAT_COMPONENT_1 ==
        static_cast<aidl::android::hardware::graphics::composer3::FormatColorComponent>(
                ::android::hardware::graphics::composer::V2_3::IComposerClient::
                        FormatColorComponent::FORMAT_COMPONENT_1));
static_assert(
        aidl::android::hardware::graphics::composer3::FormatColorComponent::FORMAT_COMPONENT_2 ==
        static_cast<aidl::android::hardware::graphics::composer3::FormatColorComponent>(
                ::android::hardware::graphics::composer::V2_3::IComposerClient::
                        FormatColorComponent::FORMAT_COMPONENT_2));
static_assert(
        aidl::android::hardware::graphics::composer3::FormatColorComponent::FORMAT_COMPONENT_3 ==
        static_cast<aidl::android::hardware::graphics::composer3::FormatColorComponent>(
                ::android::hardware::graphics::composer::V2_3::IComposerClient::
                        FormatColorComponent::FORMAT_COMPONENT_3));

static_assert(
        aidl::android::hardware::graphics::composer3::ContentType::NONE ==
        static_cast<aidl::android::hardware::graphics::composer3::ContentType>(
                ::android::hardware::graphics::composer::V2_4::IComposerClient::ContentType::NONE));
static_assert(aidl::android::hardware::graphics::composer3::ContentType::GRAPHICS ==
              static_cast<aidl::android::hardware::graphics::composer3::ContentType>(
                      ::android::hardware::graphics::composer::V2_4::IComposerClient::ContentType::
                              GRAPHICS));
static_assert(aidl::android::hardware::graphics::composer3::ContentType::PHOTO ==
              static_cast<aidl::android::hardware::graphics::composer3::ContentType>(
                      ::android::hardware::graphics::composer::V2_4::IComposerClient::ContentType::
                              PHOTO));
static_assert(aidl::android::hardware::graphics::composer3::ContentType::CINEMA ==
              static_cast<aidl::android::hardware::graphics::composer3::ContentType>(
                      ::android::hardware::graphics::composer::V2_4::IComposerClient::ContentType::
                              CINEMA));
static_assert(
        aidl::android::hardware::graphics::composer3::ContentType::GAME ==
        static_cast<aidl::android::hardware::graphics::composer3::ContentType>(
                ::android::hardware::graphics::composer::V2_4::IComposerClient::ContentType::GAME));

static_assert(
        aidl::android::hardware::graphics::composer3::PresentOrValidate::Result::Presented ==
        static_cast<aidl::android::hardware::graphics::composer3::PresentOrValidate::Result>(1));
static_assert(
        aidl::android::hardware::graphics::composer3::PresentOrValidate::Result::Validated ==
        static_cast<aidl::android::hardware::graphics::composer3::PresentOrValidate::Result>(0));

}  // namespace android::h2a
