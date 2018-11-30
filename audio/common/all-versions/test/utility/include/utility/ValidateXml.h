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

#ifndef ANDROID_HARDWARE_AUDIO_COMMON_TEST_UTILITY_VALIDATE_XML_H
#define ANDROID_HARDWARE_AUDIO_COMMON_TEST_UTILITY_VALIDATE_XML_H

#include <gtest/gtest.h>

namespace android {
namespace hardware {
namespace audio {
namespace common {
namespace test {
namespace utility {

/** Validate the provided XmlFile with the provided xsdFile.
 * Intended to use with ASSERT_PRED_FORMAT2 as such:
 *   ASSERT_PRED_FORMAT2(validateXml, pathToXml, pathToXsd);
 * See ASSERT_VALID_XML for a helper macro.
 */
::testing::AssertionResult validateXml(const char* xmlFilePathExpr, const char* xsdFilePathExpr,
                                       const char* xmlFilePath, const char* xsdFilePath);

/** Helper gtest ASSERT to test XML validity against an XSD. */
#define ASSERT_VALID_XML(xmlFilePath, xsdFilePath)                                      \
    ASSERT_PRED_FORMAT2(::android::hardware::audio::common::test::utility::validateXml, \
                        xmlFilePath, xsdFilePath)

/** Helper gtest EXPECT to test XML validity against an XSD. */
#define EXPECT_VALID_XML(xmlFilePath, xsdFilePath)                                      \
    EXPECT_PRED_FORMAT2(::android::hardware::audio::common::test::utility::validateXml, \
                        xmlFilePath, xsdFilePath)

/** Validate an XML according to an xsd.
 * All file named xmlFileName in each xmlFileLocations folder must be valid if present.
 * @tparam atLeastOneRequired If true, at least one file has to be found.
 *                           If false, no found file is a success.
 */
template <bool atLeastOneRequired = true>
::testing::AssertionResult validateXmlMultipleLocations(
    const char* xmlFileNameExpr, const char* xmlFileLocationsExpr, const char* xsdFilePathExpr,
    const char* xmlFileName, std::vector<const char*> xmlFileLocations, const char* xsdFilePath);

/** ASSERT that all found XML are valid according to an xsd. */
#define ASSERT_VALID_XML_MULTIPLE_LOCATIONS(xmlFileName, xmlFileLocations, xsdFilePath)         \
    ASSERT_PRED_FORMAT3(                                                                        \
        ::android::hardware::audio::common::test::utility::validateXmlMultipleLocations<false>, \
        xmlFileName, xmlFileLocations, xsdFilePath)

/** EXPECT that all found XML are valid according to an xsd. */
#define EXPECT_VALID_XML_MULTIPLE_LOCATIONS(xmlFileName, xmlFileLocations, xsdFilePath)         \
    EXPECT_PRED_FORMAT3(                                                                        \
        ::android::hardware::audio::common::test::utility::validateXmlMultipleLocations<false>, \
        xmlFileName, xmlFileLocations, xsdFilePath)

/** ASSERT that all found XML are valid according to an xsd. At least one must be found. */
#define ASSERT_ONE_VALID_XML_MULTIPLE_LOCATIONS(xmlFileName, xmlFileLocations, xsdFilePath)    \
    ASSERT_PRED_FORMAT3(                                                                       \
        ::android::hardware::audio::common::test::utility::validateXmlMultipleLocations<true>, \
        xmlFileName, xmlFileLocations, xsdFilePath)

/** EXPECT that all found XML are valid according to an xsd. At least one must be found. */
#define EXPECT_ONE_VALID_XML_MULTIPLE_LOCATIONS(xmlFileName, xmlFileLocations, xsdFilePath)    \
    EXPECT_PRED_FORMAT3(                                                                       \
        ::android::hardware::audio::common::test::utility::validateXmlMultipleLocations<true>, \
        xmlFileName, xmlFileLocations, xsdFilePath)

}  // namespace utility
}  // namespace test
}  // namespace common
}  // namespace audio
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_AUDIO_COMMON_TEST_UTILITY_VALIDATE_XML_H
