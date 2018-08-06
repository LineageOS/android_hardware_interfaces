/*
 * Copyright (C) 2017 The Android Open Source Project
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
#include <string>

#include "utility/ValidateXml.h"

TEST(CheckConfig, audioPolicyConfigurationValidation) {
    RecordProperty("description",
                   "Verify that the audio policy configuration file "
                   "is valid according to the schema");

    std::vector<const char*> locations = {"/odm/etc", "/vendor/etc", "/system/etc"};
    EXPECT_ONE_VALID_XML_MULTIPLE_LOCATIONS("audio_policy_configuration.xml", locations,
                                            "/data/local/tmp/audio_policy_configuration.xsd");
}
