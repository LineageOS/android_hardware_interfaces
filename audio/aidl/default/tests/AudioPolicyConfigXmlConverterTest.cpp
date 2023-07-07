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

// #include <memory>
// #include <string>
// #include <vector>

#include <android-base/macros.h>
#include <gtest/gtest.h>
#define LOG_TAG "AudioPolicyConfigXmlConverterTest"
#include <log/log.h>

#include <core-impl/AudioPolicyConfigXmlConverter.h>
#include <media/AidlConversionCppNdk.h>

using aidl::android::hardware::audio::core::internal::AudioPolicyConfigXmlConverter;
using aidl::android::media::audio::common::AudioFormatDescription;

namespace {

void ValidateAudioFormatDescription(const AudioFormatDescription& format) {
    auto conv = ::aidl::android::aidl2legacy_AudioFormatDescription_audio_format_t(format);
    ASSERT_TRUE(conv.ok()) << format.toString();
}

}  // namespace

TEST(AudioPolicyConfigXmlConverterTest, DefaultSurroundSoundConfigIsValid) {
    auto config = AudioPolicyConfigXmlConverter::getDefaultSurroundSoundConfig();
    for (const auto& family : config.formatFamilies) {
        EXPECT_NO_FATAL_FAILURE(ValidateAudioFormatDescription(family.primaryFormat));
        SCOPED_TRACE(family.primaryFormat.toString());
        for (const auto& sub : family.subFormats) {
            EXPECT_NO_FATAL_FAILURE(ValidateAudioFormatDescription(sub));
        }
    }
}
