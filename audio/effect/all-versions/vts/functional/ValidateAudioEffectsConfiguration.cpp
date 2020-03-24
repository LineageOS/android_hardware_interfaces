/*
 * Copyright (C) 2018 The Android Open Source Project
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

#include <unistd.h>
#include <iterator>

#include <media/EffectsConfig.h>
// clang-format off
#include PATH(android/hardware/audio/effect/FILE_VERSION/IEffectsFactory.h)
// clang-format on

#include <gtest/gtest.h>
#include <hidl/ServiceManagement.h>

#include "utility/ValidateXml.h"

// Stringify the argument.
#define QUOTE(x) #x
#define STRINGIFY(x) QUOTE(x)

TEST(CheckConfig, audioEffectsConfigurationValidation) {
    RecordProperty("description",
                   "Verify that the effects configuration file is valid according to the schema");
    using namespace android::effectsConfig;
    if (android::hardware::getAllHalInstanceNames(
                ::android::hardware::audio::effect::CPP_VERSION::IEffectsFactory::descriptor)
                .size() == 0) {
        GTEST_SKIP() << "No Effects HAL version " STRINGIFY(CPP_VERSION) " on this device";
    }

    std::vector<const char*> locations(std::begin(DEFAULT_LOCATIONS), std::end(DEFAULT_LOCATIONS));
    const char* xsd = "/data/local/tmp/audio_effects_conf_" STRINGIFY(CPP_VERSION) ".xsd";
#if MAJOR_VERSION == 2
    // In V2, audio effect XML is not required. .conf is still allowed though deprecated
    EXPECT_VALID_XML_MULTIPLE_LOCATIONS(DEFAULT_NAME, locations, xsd);
#elif MAJOR_VERSION >= 4
    // Starting with V4, audio effect XML is required
    EXPECT_ONE_VALID_XML_MULTIPLE_LOCATIONS(DEFAULT_NAME, locations, xsd);
#endif
}
