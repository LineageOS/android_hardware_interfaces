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

#include "android/hardware/graphics/composer3/translate-ndk.h"

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

static_assert(aidl::android::hardware::graphics::composer3::LayerRequest::CLEAR_CLIENT_TARGET ==
              static_cast<aidl::android::hardware::graphics::composer3::LayerRequest>(
                      ::android::hardware::graphics::composer::V2_1::IComposerClient::LayerRequest::
                              CLEAR_CLIENT_TARGET));

static_assert(aidl::android::hardware::graphics::composer3::BlendMode::INVALID ==
              static_cast<aidl::android::hardware::graphics::composer3::BlendMode>(
                      ::android::hardware::graphics::composer::V2_1::IComposerClient::BlendMode::
                              INVALID));
static_assert(
        aidl::android::hardware::graphics::composer3::BlendMode::NONE ==
        static_cast<aidl::android::hardware::graphics::composer3::BlendMode>(
                ::android::hardware::graphics::composer::V2_1::IComposerClient::BlendMode::NONE));
static_assert(aidl::android::hardware::graphics::composer3::BlendMode::PREMULTIPLIED ==
              static_cast<aidl::android::hardware::graphics::composer3::BlendMode>(
                      ::android::hardware::graphics::composer::V2_1::IComposerClient::BlendMode::
                              PREMULTIPLIED));
static_assert(aidl::android::hardware::graphics::composer3::BlendMode::COVERAGE ==
              static_cast<aidl::android::hardware::graphics::composer3::BlendMode>(
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
              static_cast<aidl::android::hardware::graphics::composer3::DisplayRequest>(
                      ::android::hardware::graphics::composer::V2_1::IComposerClient::
                              DisplayRequest::FLIP_CLIENT_TARGET));
static_assert(aidl::android::hardware::graphics::composer3::DisplayRequest::
                      WRITE_CLIENT_TARGET_TO_OUTPUT ==
              static_cast<aidl::android::hardware::graphics::composer3::DisplayRequest>(
                      ::android::hardware::graphics::composer::V2_1::IComposerClient::
                              DisplayRequest::WRITE_CLIENT_TARGET_TO_OUTPUT));

static_assert(aidl::android::hardware::graphics::composer3::HandleIndex::EMPTY ==
              static_cast<aidl::android::hardware::graphics::composer3::HandleIndex>(
                      ::android::hardware::graphics::composer::V2_1::IComposerClient::HandleIndex::
                              EMPTY));
static_assert(aidl::android::hardware::graphics::composer3::HandleIndex::CACHED ==
              static_cast<aidl::android::hardware::graphics::composer3::HandleIndex>(
                      ::android::hardware::graphics::composer::V2_1::IComposerClient::HandleIndex::
                              CACHED));

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

static_assert(aidl::android::hardware::graphics::composer3::Command::LENGTH_MASK ==
              static_cast<aidl::android::hardware::graphics::composer3::Command>(
                      ::android::hardware::graphics::composer::V2_4::IComposerClient::Command::
                              LENGTH_MASK));
static_assert(aidl::android::hardware::graphics::composer3::Command::OPCODE_SHIFT ==
              static_cast<aidl::android::hardware::graphics::composer3::Command>(
                      ::android::hardware::graphics::composer::V2_4::IComposerClient::Command::
                              OPCODE_SHIFT));
static_assert(aidl::android::hardware::graphics::composer3::Command::OPCODE_MASK ==
              static_cast<aidl::android::hardware::graphics::composer3::Command>(
                      ::android::hardware::graphics::composer::V2_4::IComposerClient::Command::
                              OPCODE_MASK));
static_assert(aidl::android::hardware::graphics::composer3::Command::SELECT_DISPLAY ==
              static_cast<aidl::android::hardware::graphics::composer3::Command>(
                      ::android::hardware::graphics::composer::V2_4::IComposerClient::Command::
                              SELECT_DISPLAY));
static_assert(aidl::android::hardware::graphics::composer3::Command::SELECT_LAYER ==
              static_cast<aidl::android::hardware::graphics::composer3::Command>(
                      ::android::hardware::graphics::composer::V2_4::IComposerClient::Command::
                              SELECT_LAYER));
static_assert(aidl::android::hardware::graphics::composer3::Command::SET_ERROR ==
              static_cast<aidl::android::hardware::graphics::composer3::Command>(
                      ::android::hardware::graphics::composer::V2_4::IComposerClient::Command::
                              SET_ERROR));
static_assert(
        aidl::android::hardware::graphics::composer3::Command::SET_CHANGED_COMPOSITION_TYPES ==
        static_cast<aidl::android::hardware::graphics::composer3::Command>(
                ::android::hardware::graphics::composer::V2_4::IComposerClient::Command::
                        SET_CHANGED_COMPOSITION_TYPES));
static_assert(aidl::android::hardware::graphics::composer3::Command::SET_DISPLAY_REQUESTS ==
              static_cast<aidl::android::hardware::graphics::composer3::Command>(
                      ::android::hardware::graphics::composer::V2_4::IComposerClient::Command::
                              SET_DISPLAY_REQUESTS));
static_assert(aidl::android::hardware::graphics::composer3::Command::SET_PRESENT_FENCE ==
              static_cast<aidl::android::hardware::graphics::composer3::Command>(
                      ::android::hardware::graphics::composer::V2_4::IComposerClient::Command::
                              SET_PRESENT_FENCE));
static_assert(aidl::android::hardware::graphics::composer3::Command::SET_RELEASE_FENCES ==
              static_cast<aidl::android::hardware::graphics::composer3::Command>(
                      ::android::hardware::graphics::composer::V2_4::IComposerClient::Command::
                              SET_RELEASE_FENCES));
static_assert(aidl::android::hardware::graphics::composer3::Command::SET_COLOR_TRANSFORM ==
              static_cast<aidl::android::hardware::graphics::composer3::Command>(
                      ::android::hardware::graphics::composer::V2_4::IComposerClient::Command::
                              SET_COLOR_TRANSFORM));
static_assert(aidl::android::hardware::graphics::composer3::Command::SET_CLIENT_TARGET ==
              static_cast<aidl::android::hardware::graphics::composer3::Command>(
                      ::android::hardware::graphics::composer::V2_4::IComposerClient::Command::
                              SET_CLIENT_TARGET));
static_assert(aidl::android::hardware::graphics::composer3::Command::SET_OUTPUT_BUFFER ==
              static_cast<aidl::android::hardware::graphics::composer3::Command>(
                      ::android::hardware::graphics::composer::V2_4::IComposerClient::Command::
                              SET_OUTPUT_BUFFER));
static_assert(aidl::android::hardware::graphics::composer3::Command::VALIDATE_DISPLAY ==
              static_cast<aidl::android::hardware::graphics::composer3::Command>(
                      ::android::hardware::graphics::composer::V2_4::IComposerClient::Command::
                              VALIDATE_DISPLAY));
static_assert(aidl::android::hardware::graphics::composer3::Command::ACCEPT_DISPLAY_CHANGES ==
              static_cast<aidl::android::hardware::graphics::composer3::Command>(
                      ::android::hardware::graphics::composer::V2_4::IComposerClient::Command::
                              ACCEPT_DISPLAY_CHANGES));
static_assert(aidl::android::hardware::graphics::composer3::Command::PRESENT_DISPLAY ==
              static_cast<aidl::android::hardware::graphics::composer3::Command>(
                      ::android::hardware::graphics::composer::V2_4::IComposerClient::Command::
                              PRESENT_DISPLAY));
static_assert(aidl::android::hardware::graphics::composer3::Command::PRESENT_OR_VALIDATE_DISPLAY ==
              static_cast<aidl::android::hardware::graphics::composer3::Command>(
                      ::android::hardware::graphics::composer::V2_4::IComposerClient::Command::
                              PRESENT_OR_VALIDATE_DISPLAY));
static_assert(aidl::android::hardware::graphics::composer3::Command::SET_LAYER_CURSOR_POSITION ==
              static_cast<aidl::android::hardware::graphics::composer3::Command>(
                      ::android::hardware::graphics::composer::V2_4::IComposerClient::Command::
                              SET_LAYER_CURSOR_POSITION));
static_assert(aidl::android::hardware::graphics::composer3::Command::SET_LAYER_BUFFER ==
              static_cast<aidl::android::hardware::graphics::composer3::Command>(
                      ::android::hardware::graphics::composer::V2_4::IComposerClient::Command::
                              SET_LAYER_BUFFER));
static_assert(aidl::android::hardware::graphics::composer3::Command::SET_LAYER_SURFACE_DAMAGE ==
              static_cast<aidl::android::hardware::graphics::composer3::Command>(
                      ::android::hardware::graphics::composer::V2_4::IComposerClient::Command::
                              SET_LAYER_SURFACE_DAMAGE));
static_assert(aidl::android::hardware::graphics::composer3::Command::SET_LAYER_BLEND_MODE ==
              static_cast<aidl::android::hardware::graphics::composer3::Command>(
                      ::android::hardware::graphics::composer::V2_4::IComposerClient::Command::
                              SET_LAYER_BLEND_MODE));
static_assert(aidl::android::hardware::graphics::composer3::Command::SET_LAYER_COLOR ==
              static_cast<aidl::android::hardware::graphics::composer3::Command>(
                      ::android::hardware::graphics::composer::V2_4::IComposerClient::Command::
                              SET_LAYER_COLOR));
static_assert(aidl::android::hardware::graphics::composer3::Command::SET_LAYER_COMPOSITION_TYPE ==
              static_cast<aidl::android::hardware::graphics::composer3::Command>(
                      ::android::hardware::graphics::composer::V2_4::IComposerClient::Command::
                              SET_LAYER_COMPOSITION_TYPE));
static_assert(aidl::android::hardware::graphics::composer3::Command::SET_LAYER_DATASPACE ==
              static_cast<aidl::android::hardware::graphics::composer3::Command>(
                      ::android::hardware::graphics::composer::V2_4::IComposerClient::Command::
                              SET_LAYER_DATASPACE));
static_assert(aidl::android::hardware::graphics::composer3::Command::SET_LAYER_DISPLAY_FRAME ==
              static_cast<aidl::android::hardware::graphics::composer3::Command>(
                      ::android::hardware::graphics::composer::V2_4::IComposerClient::Command::
                              SET_LAYER_DISPLAY_FRAME));
static_assert(aidl::android::hardware::graphics::composer3::Command::SET_LAYER_PLANE_ALPHA ==
              static_cast<aidl::android::hardware::graphics::composer3::Command>(
                      ::android::hardware::graphics::composer::V2_4::IComposerClient::Command::
                              SET_LAYER_PLANE_ALPHA));
static_assert(aidl::android::hardware::graphics::composer3::Command::SET_LAYER_SIDEBAND_STREAM ==
              static_cast<aidl::android::hardware::graphics::composer3::Command>(
                      ::android::hardware::graphics::composer::V2_4::IComposerClient::Command::
                              SET_LAYER_SIDEBAND_STREAM));
static_assert(aidl::android::hardware::graphics::composer3::Command::SET_LAYER_SOURCE_CROP ==
              static_cast<aidl::android::hardware::graphics::composer3::Command>(
                      ::android::hardware::graphics::composer::V2_4::IComposerClient::Command::
                              SET_LAYER_SOURCE_CROP));
static_assert(aidl::android::hardware::graphics::composer3::Command::SET_LAYER_TRANSFORM ==
              static_cast<aidl::android::hardware::graphics::composer3::Command>(
                      ::android::hardware::graphics::composer::V2_4::IComposerClient::Command::
                              SET_LAYER_TRANSFORM));
static_assert(aidl::android::hardware::graphics::composer3::Command::SET_LAYER_VISIBLE_REGION ==
              static_cast<aidl::android::hardware::graphics::composer3::Command>(
                      ::android::hardware::graphics::composer::V2_4::IComposerClient::Command::
                              SET_LAYER_VISIBLE_REGION));
static_assert(aidl::android::hardware::graphics::composer3::Command::SET_LAYER_Z_ORDER ==
              static_cast<aidl::android::hardware::graphics::composer3::Command>(
                      ::android::hardware::graphics::composer::V2_4::IComposerClient::Command::
                              SET_LAYER_Z_ORDER));
static_assert(aidl::android::hardware::graphics::composer3::Command::
                      SET_PRESENT_OR_VALIDATE_DISPLAY_RESULT ==
              static_cast<aidl::android::hardware::graphics::composer3::Command>(
                      ::android::hardware::graphics::composer::V2_4::IComposerClient::Command::
                              SET_PRESENT_OR_VALIDATE_DISPLAY_RESULT));
static_assert(aidl::android::hardware::graphics::composer3::Command::SET_LAYER_PER_FRAME_METADATA ==
              static_cast<aidl::android::hardware::graphics::composer3::Command>(
                      ::android::hardware::graphics::composer::V2_4::IComposerClient::Command::
                              SET_LAYER_PER_FRAME_METADATA));
static_assert(aidl::android::hardware::graphics::composer3::Command::SET_LAYER_FLOAT_COLOR ==
              static_cast<aidl::android::hardware::graphics::composer3::Command>(
                      ::android::hardware::graphics::composer::V2_4::IComposerClient::Command::
                              SET_LAYER_FLOAT_COLOR));
static_assert(aidl::android::hardware::graphics::composer3::Command::SET_LAYER_COLOR_TRANSFORM ==
              static_cast<aidl::android::hardware::graphics::composer3::Command>(
                      ::android::hardware::graphics::composer::V2_4::IComposerClient::Command::
                              SET_LAYER_COLOR_TRANSFORM));
static_assert(
        aidl::android::hardware::graphics::composer3::Command::SET_LAYER_PER_FRAME_METADATA_BLOBS ==
        static_cast<aidl::android::hardware::graphics::composer3::Command>(
                ::android::hardware::graphics::composer::V2_4::IComposerClient::Command::
                        SET_LAYER_PER_FRAME_METADATA_BLOBS));
static_assert(aidl::android::hardware::graphics::composer3::Command::SET_CLIENT_TARGET_PROPERTY ==
              static_cast<aidl::android::hardware::graphics::composer3::Command>(
                      ::android::hardware::graphics::composer::V2_4::IComposerClient::Command::
                              SET_CLIENT_TARGET_PROPERTY));
static_assert(aidl::android::hardware::graphics::composer3::Command::SET_LAYER_GENERIC_METADATA ==
              static_cast<aidl::android::hardware::graphics::composer3::Command>(
                      ::android::hardware::graphics::composer::V2_4::IComposerClient::Command::
                              SET_LAYER_GENERIC_METADATA));

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

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::graphics::composer::V2_4::VsyncPeriodChangeTimeline& in,
        aidl::android::hardware::graphics::composer3::VsyncPeriodChangeTimeline* out) {
    out->newVsyncAppliedTimeNanos = static_cast<int64_t>(in.newVsyncAppliedTimeNanos);
    out->refreshRequired = static_cast<bool>(in.refreshRequired);
    out->refreshTimeNanos = static_cast<int64_t>(in.refreshTimeNanos);
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::graphics::composer::V2_1::IComposerClient::Rect& in,
        aidl::android::hardware::graphics::common::Rect* out) {
    out->left = static_cast<int32_t>(in.left);
    out->top = static_cast<int32_t>(in.top);
    out->right = static_cast<int32_t>(in.right);
    out->bottom = static_cast<int32_t>(in.bottom);
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::graphics::composer::V2_1::IComposerClient::FRect& in,
        aidl::android::hardware::graphics::common::FRect* out) {
    out->left = static_cast<float>(in.left);
    out->top = static_cast<float>(in.top);
    out->right = static_cast<float>(in.right);
    out->bottom = static_cast<float>(in.bottom);
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::graphics::composer::V2_1::IComposerClient::Color& in,
        aidl::android::hardware::graphics::composer3::Color* out) {
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit
    // your needs.
    if (in.r > std::numeric_limits<int8_t>::max() || in.r < 0) {
        return false;
    }
    out->r = static_cast<int8_t>(in.r);
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit
    // your needs.
    if (in.g > std::numeric_limits<int8_t>::max() || in.g < 0) {
        return false;
    }
    out->g = static_cast<int8_t>(in.g);
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit
    // your needs.
    if (in.b > std::numeric_limits<int8_t>::max() || in.b < 0) {
        return false;
    }
    out->b = static_cast<int8_t>(in.b);
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit
    // your needs.
    if (in.a > std::numeric_limits<int8_t>::max() || in.a < 0) {
        return false;
    }
    out->a = static_cast<int8_t>(in.a);
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::graphics::composer::V2_3::IComposerClient::PerFrameMetadata& in,
        aidl::android::hardware::graphics::composer3::PerFrameMetadata* out) {
    out->key =
            static_cast<aidl::android::hardware::graphics::composer3::PerFrameMetadataKey>(in.key);
    out->value = static_cast<float>(in.value);
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::graphics::composer::V2_2::IComposerClient::FloatColor& in,
        aidl::android::hardware::graphics::composer3::FloatColor* out) {
    out->r = static_cast<float>(in.r);
    out->g = static_cast<float>(in.g);
    out->b = static_cast<float>(in.b);
    out->a = static_cast<float>(in.a);
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::graphics::composer::V2_3::IComposerClient::PerFrameMetadataBlob&
                in,
        aidl::android::hardware::graphics::composer3::PerFrameMetadataBlob* out) {
    out->key =
            static_cast<aidl::android::hardware::graphics::composer3::PerFrameMetadataKey>(in.key);
    {
        size_t size = in.blob.size();
        for (size_t i = 0; i < size; i++) {
            // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
            // suit your needs.
            if (in.blob[i] > std::numeric_limits<int8_t>::max() || in.blob[i] < 0) {
                return false;
            }
            out->blob.push_back(static_cast<int8_t>(in.blob[i]));
        }
    }
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::graphics::composer::V2_4::IComposerClient::
                VsyncPeriodChangeConstraints& in,
        aidl::android::hardware::graphics::composer3::VsyncPeriodChangeConstraints* out) {
    out->desiredTimeNanos = static_cast<int64_t>(in.desiredTimeNanos);
    out->seamlessRequired = static_cast<bool>(in.seamlessRequired);
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::graphics::composer::V2_4::IComposerClient::ClientTargetProperty&
                in,
        aidl::android::hardware::graphics::composer3::ClientTargetProperty* out) {
    out->pixelFormat =
            static_cast<aidl::android::hardware::graphics::common::PixelFormat>(in.pixelFormat);
    out->dataspace =
            static_cast<aidl::android::hardware::graphics::common::Dataspace>(in.dataspace);
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::graphics::composer::V2_4::IComposerClient::
                LayerGenericMetadataKey& in,
        aidl::android::hardware::graphics::composer3::LayerGenericMetadataKey* out) {
    out->name = in.name;
    out->mandatory = static_cast<bool>(in.mandatory);
    return true;
}

}  // namespace android::h2a