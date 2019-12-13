/*
 * Copyright (C) 2019 The Android Open Source Project
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

#include <gtest/gtest.h>

#include <unistd.h>
#include <string>
#include "utility/ValidateXml.h"

static const std::vector<const char*> locations = {"/odm/etc", "/vendor/etc", "/system/etc"};
static const std::string config = "audio_policy_engine_configuration.xml";
static const std::string schema =
        std::string(XSD_DIR) + "/audio_policy_engine_configuration_V1_0.xsd";

/**
 * @brief TEST to ensure the audio policy engine configuration file is validating schemas.
 * Note: this configuration file is not mandatory, an hardcoded fallback is provided, so
 * it does not fail if not found.
 */
TEST(ValidateConfiguration, audioPolicyEngineConfiguration) {
    RecordProperty("description",
                   "Verify that the audio policy engine configuration file "
                   "is valid according to the schemas");
    EXPECT_VALID_XML_MULTIPLE_LOCATIONS(config.c_str(), locations, schema.c_str());
}
