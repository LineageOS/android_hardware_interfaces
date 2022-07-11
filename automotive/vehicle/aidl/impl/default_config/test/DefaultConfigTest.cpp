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

#include <DefaultConfig.h>
#include <JsonConfigLoader.h>
#include <VehicleUtils.h>
#include <android-base/file.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <fstream>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {

namespace test {

using ::android::base::Error;
using ::android::base::Result;
using ::testing::UnorderedElementsAreArray;

constexpr char kDefaultPropertiesConfigFile[] = "DefaultProperties.json";

#ifdef ENABLE_VEHICLE_HAL_TEST_PROPERTIES
constexpr char kTestPropertiesConfigFile[] = "TestProperties.json";
constexpr char kVendorClusterTestPropertiesConfigFile[] = "VendorClusterTestProperties.json";
#endif  // ENABLE_VEHICLE_HAL_TEST_PROPERTIES

std::string getTestFilePath(const char* filename) {
    static std::string baseDir = android::base::GetExecutableDirectory();
    return baseDir + "/" + filename;
}

Result<std::vector<ConfigDeclaration>> loadConfig(JsonConfigLoader& loader, const char* path) {
    std::string configPath = getTestFilePath(path);
    std::ifstream ifs(configPath.c_str());
    if (!ifs) {
        return Error() << "couldn't open %s for parsing." << configPath;
    }

    return loader.loadPropConfig(ifs);
}

TEST(DefaultConfigTest, TestloadDefaultProperties) {
    JsonConfigLoader loader;
    auto result = loadConfig(loader, kDefaultPropertiesConfigFile);

    ASSERT_TRUE(result.ok()) << result.error().message();
}

#ifdef ENABLE_VEHICLE_HAL_TEST_PROPERTIES

TEST(DefaultConfigTest, TestloadTestProperties) {
    JsonConfigLoader loader;
    auto result = loadConfig(loader, kTestPropertiesConfigFile);

    ASSERT_TRUE(result.ok()) << result.error().message();
}

TEST(DefaultConfigTest, TestloadVendorClusterTestProperties) {
    JsonConfigLoader loader;
    auto result = loadConfig(loader, kVendorClusterTestPropertiesConfigFile);

    ASSERT_TRUE(result.ok()) << result.error().message();
}

// TODO(b/238685398): Remove this test after we deprecate DefaultConfig.h
TEST(DefaultConfigTest, TestCompatibleWithDefaultConfigHeader) {
    auto configsFromHeaderFile = defaultconfig::getDefaultConfigs();

    std::vector<ConfigDeclaration> configsFromJson;
    JsonConfigLoader loader;
    for (const char* file :
         std::vector<const char*>({kDefaultPropertiesConfigFile, kTestPropertiesConfigFile,
                                   kVendorClusterTestPropertiesConfigFile})) {
        auto result = loadConfig(loader, file);
        ASSERT_TRUE(result.ok()) << result.error().message();
        configsFromJson.insert(configsFromJson.end(), result.value().begin(), result.value().end());
    }

    ASSERT_EQ(configsFromHeaderFile.size(), configsFromJson.size());
    // TODO(b/238685398): Uncomment this once we finish the parser.
    // ASSERT_THAT(configsFromHeaderFile, UnorderedElementsAreArray(configsFromJson));
}

#endif  // ENABLE_VEHICLE_HAL_TEST_PROPERTIES

}  // namespace test

}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
