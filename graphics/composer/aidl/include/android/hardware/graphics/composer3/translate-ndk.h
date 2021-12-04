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

#pragma once

#include <limits>
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
#include "aidl/android/hardware/graphics/composer3/FloatColor.h"
#include "aidl/android/hardware/graphics/composer3/FormatColorComponent.h"
#include "aidl/android/hardware/graphics/composer3/HandleIndex.h"
#include "aidl/android/hardware/graphics/composer3/IComposer.h"
#include "aidl/android/hardware/graphics/composer3/LayerGenericMetadataKey.h"
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

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::graphics::composer::V2_4::VsyncPeriodChangeTimeline& in,
        aidl::android::hardware::graphics::composer3::VsyncPeriodChangeTimeline* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::graphics::composer::V2_1::IComposerClient::Rect& in,
        aidl::android::hardware::graphics::common::Rect* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::graphics::composer::V2_1::IComposerClient::FRect& in,
        aidl::android::hardware::graphics::common::FRect* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::graphics::composer::V2_1::IComposerClient::Color& in,
        aidl::android::hardware::graphics::composer3::Color* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::graphics::composer::V2_3::IComposerClient::PerFrameMetadata& in,
        aidl::android::hardware::graphics::composer3::PerFrameMetadata* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::graphics::composer::V2_2::IComposerClient::FloatColor& in,
        aidl::android::hardware::graphics::composer3::FloatColor* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::graphics::composer::V2_3::IComposerClient::PerFrameMetadataBlob&
                in,
        aidl::android::hardware::graphics::composer3::PerFrameMetadataBlob* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::graphics::composer::V2_4::IComposerClient::
                VsyncPeriodChangeConstraints& in,
        aidl::android::hardware::graphics::composer3::VsyncPeriodChangeConstraints* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::graphics::composer::V2_4::IComposerClient::ClientTargetProperty&
                in,
        aidl::android::hardware::graphics::composer3::ClientTargetProperty* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::graphics::composer::V2_4::IComposerClient::
                LayerGenericMetadataKey& in,
        aidl::android::hardware::graphics::composer3::LayerGenericMetadataKey* out);

}  // namespace android::h2a
