/*
 * Copyright (C) 2021 The Android Open Source Project
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

#include <android-base/file.h>
#include <gtest/gtest.h>

#include <ValidateXml.h>

using ::android::hardware::audio::common::test::utility::validateXml;

const char* XSD_SOURCE =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<xs:schema version=\"2.0\""
        "           elementFormDefault=\"qualified\""
        "           attributeFormDefault=\"unqualified\""
        "           xmlns:xs=\"http://www.w3.org/2001/XMLSchema\">"
        "  <xs:element name=\"audioPolicyConfiguration\">"
        "    <xs:complexType>"
        "      <xs:sequence>"
        "        <xs:element name=\"modules\">"
        "          <xs:complexType>"
        "            <xs:sequence>"
        "              <xs:element name=\"module\" maxOccurs=\"unbounded\">"
        "                <xs:complexType>"
        "                  <xs:attribute name=\"name\" type=\"xs:string\" use=\"required\"/>"
        "                </xs:complexType>"
        "              </xs:element>"
        "            </xs:sequence>"
        "          </xs:complexType>"
        "        </xs:element>"
        "      </xs:sequence>"
        "    </xs:complexType>"
        "  </xs:element>"
        "</xs:schema>";

const char* INVALID_XML_SOURCE =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<audioPolicyKonfiguration />";

const char* VALID_XML_SOURCE =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<audioPolicyConfiguration>"
        "  <modules>"
        "    <module name=\"aaa\" />"
        "    %s"
        "  </modules>"
        "</audioPolicyConfiguration>";

const char* MODULE_SOURCE = "<module name=\"bbb\" />";

const char* XI_INCLUDE = "<xi:include xmlns:xi=\"http://www.w3.org/2001/XInclude\" href=\"%s\" />";

const char* XML_INCLUDED_SOURCE = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>%s";

namespace {

std::string substitute(const char* fmt, const char* param) {
    std::string buffer(static_cast<size_t>(strlen(fmt) + strlen(param)), '\0');
    snprintf(buffer.data(), buffer.size(), fmt, param);
    buffer.resize(strlen(buffer.c_str()));
    return buffer;
}

std::string substitute(const char* fmt, const std::string& s) {
    return substitute(fmt, s.c_str());
}

}  // namespace

TEST(ValidateXml, InvalidXml) {
    TemporaryFile xml;
    ASSERT_TRUE(android::base::WriteStringToFile(INVALID_XML_SOURCE, xml.path)) << strerror(errno);
    TemporaryFile xsd;
    ASSERT_TRUE(android::base::WriteStringToFile(XSD_SOURCE, xsd.path)) << strerror(errno);
    EXPECT_FALSE(validateXml("xml", "xsd", xml.path, xsd.path));
}

TEST(ValidateXml, ValidXml) {
    TemporaryFile xml;
    ASSERT_TRUE(
            android::base::WriteStringToFile(substitute(VALID_XML_SOURCE, MODULE_SOURCE), xml.path))
            << strerror(errno);
    TemporaryFile xsd;
    ASSERT_TRUE(android::base::WriteStringToFile(XSD_SOURCE, xsd.path)) << strerror(errno);
    EXPECT_TRUE(validateXml("xml", "xsd", xml.path, xsd.path));
}

TEST(ValidateXml, IncludeAbsolutePath) {
    TemporaryFile xmlInclude;
    ASSERT_TRUE(android::base::WriteStringToFile(substitute(XML_INCLUDED_SOURCE, MODULE_SOURCE),
                                                 xmlInclude.path))
            << strerror(errno);
    TemporaryFile xml;
    ASSERT_TRUE(android::base::WriteStringToFile(
            substitute(VALID_XML_SOURCE, substitute(XI_INCLUDE, xmlInclude.path)), xml.path))
            << strerror(errno);
    TemporaryFile xsd;
    ASSERT_TRUE(android::base::WriteStringToFile(XSD_SOURCE, xsd.path)) << strerror(errno);
    EXPECT_TRUE(validateXml("xml", "xsd", xml.path, xsd.path));
}

TEST(ValidateXml, IncludeSameDirRelativePath) {
    TemporaryFile xmlInclude;
    ASSERT_TRUE(android::base::WriteStringToFile(substitute(XML_INCLUDED_SOURCE, MODULE_SOURCE),
                                                 xmlInclude.path))
            << strerror(errno);
    TemporaryFile xml;
    ASSERT_EQ(android::base::Dirname(xml.path), android::base::Dirname(xmlInclude.path));
    ASSERT_TRUE(android::base::WriteStringToFile(
            substitute(VALID_XML_SOURCE,
                       substitute(XI_INCLUDE, android::base::Basename(xmlInclude.path))),
            xml.path))
            << strerror(errno);
    TemporaryFile xsd;
    ASSERT_TRUE(android::base::WriteStringToFile(XSD_SOURCE, xsd.path)) << strerror(errno);
    EXPECT_TRUE(validateXml("xml", "xsd", xml.path, xsd.path));
}

TEST(ValidateXml, IncludeSubdirRelativePath) {
    TemporaryDir xmlIncludeDir;
    TemporaryFile xmlInclude(xmlIncludeDir.path);
    ASSERT_TRUE(android::base::WriteStringToFile(substitute(XML_INCLUDED_SOURCE, MODULE_SOURCE),
                                                 xmlInclude.path))
            << strerror(errno);
    TemporaryFile xml;
    ASSERT_EQ(android::base::Dirname(xml.path), android::base::Dirname(xmlIncludeDir.path));
    ASSERT_TRUE(android::base::WriteStringToFile(
            substitute(VALID_XML_SOURCE,
                       substitute(XI_INCLUDE, android::base::Basename(xmlIncludeDir.path) + "/" +
                                                      android::base::Basename(xmlInclude.path))),
            xml.path))
            << strerror(errno);
    TemporaryFile xsd;
    ASSERT_TRUE(android::base::WriteStringToFile(XSD_SOURCE, xsd.path)) << strerror(errno);
    EXPECT_TRUE(validateXml("xml", "xsd", xml.path, xsd.path));
}

TEST(ValidateXml, IncludeParentDirRelativePath) {
    // An XML file from a subdirectory includes a file from the parent directory using '..' syntax.
    TemporaryFile xmlInclude;
    ASSERT_TRUE(android::base::WriteStringToFile(substitute(XML_INCLUDED_SOURCE, MODULE_SOURCE),
                                                 xmlInclude.path))
            << strerror(errno);
    TemporaryDir xmlIncludeDir;
    TemporaryFile xmlParentInclude(xmlIncludeDir.path);
    ASSERT_TRUE(android::base::WriteStringToFile(
            substitute(XML_INCLUDED_SOURCE,
                       substitute(XI_INCLUDE, "../" + android::base::Basename(xmlInclude.path))),
            xmlParentInclude.path))
            << strerror(errno);
    TemporaryFile xml;
    ASSERT_EQ(android::base::Dirname(xml.path), android::base::Dirname(xmlInclude.path));
    ASSERT_EQ(android::base::Dirname(xml.path), android::base::Dirname(xmlIncludeDir.path));
    ASSERT_TRUE(android::base::WriteStringToFile(
            substitute(
                    VALID_XML_SOURCE,
                    substitute(XI_INCLUDE, android::base::Basename(xmlIncludeDir.path) + "/" +
                                                   android::base::Basename(xmlParentInclude.path))),
            xml.path))
            << strerror(errno);
    TemporaryFile xsd;
    ASSERT_TRUE(android::base::WriteStringToFile(XSD_SOURCE, xsd.path)) << strerror(errno);
    EXPECT_TRUE(validateXml("xml", "xsd", xml.path, xsd.path));
}
