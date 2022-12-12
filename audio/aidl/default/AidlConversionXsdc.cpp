/*
 * Copyright (C) 2023 The Android Open Source Project
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

#define LOG_TAG "AHAL_AidlXsdc"
#include <android-base/logging.h>
#include <error/expected_utils.h>
#include <media/AidlConversionCppNdk.h>
#include <media/TypeConverter.h>

#include "core-impl/AidlConversionXsdc.h"

using aidl::android::media::audio::common::AudioFormatDescription;

namespace xsd = android::audio::policy::configuration;

namespace aidl::android::hardware::audio::core::internal {

ConversionResult<AudioFormatDescription> xsdc2aidl_AudioFormatDescription(const std::string& xsdc) {
    return legacy2aidl_audio_format_t_AudioFormatDescription(::android::formatFromString(xsdc));
}

ConversionResult<SurroundSoundConfig::SurroundFormatFamily> xsdc2aidl_SurroundFormatFamily(
        const ::xsd::SurroundFormats::Format& xsdc) {
    SurroundSoundConfig::SurroundFormatFamily aidl;
    aidl.primaryFormat = VALUE_OR_RETURN(xsdc2aidl_AudioFormatDescription(xsdc.getName()));
    if (xsdc.hasSubformats()) {
        aidl.subFormats = VALUE_OR_RETURN(convertContainer<std::vector<AudioFormatDescription>>(
                xsdc.getSubformats(), xsdc2aidl_AudioFormatDescription));
    }
    return aidl;
}

ConversionResult<SurroundSoundConfig> xsdc2aidl_SurroundSoundConfig(
        const ::xsd::SurroundSound& xsdc) {
    SurroundSoundConfig aidl;
    if (!xsdc.hasFormats() || !xsdc.getFirstFormats()->hasFormat()) return aidl;
    aidl.formatFamilies = VALUE_OR_RETURN(
            convertContainer<std::vector<SurroundSoundConfig::SurroundFormatFamily>>(
                    xsdc.getFirstFormats()->getFormat(), xsdc2aidl_SurroundFormatFamily));
    return aidl;
}

}  // namespace aidl::android::hardware::audio::core::internal
