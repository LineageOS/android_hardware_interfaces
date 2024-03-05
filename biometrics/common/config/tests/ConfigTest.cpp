/*
 * Copyright (C) 2024 The Android Open Source Project
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

#include "config/Config.h"

#define LOG_TAG "ConfigTest"
#include <android-base/logging.h>

// using namespace ::testing::Eq;
using namespace testing;

#define SP_DEFAULT_astring "astringSP"
#define SP_DEFAULT_aint32 32
#define SP_DEFAULT_aint64 64
#define SP_DEFAULT_abool false
#define SP_DEFAULT_avector \
    { 1, 2, 3 }
namespace aidl::android::hardware::biometrics {
namespace TestHalProperties {
OptString val_astring = SP_DEFAULT_astring;
OptInt32 val_aint32 = SP_DEFAULT_aint32;
OptInt64 val_aint64 = SP_DEFAULT_aint64;
OptBool val_abool = SP_DEFAULT_abool;
OptIntVec val_avector = SP_DEFAULT_avector;

OptString astring() {
    return val_astring;
}
bool astring(const OptString& v) {
    val_astring = v;
    return true;
}
OptInt32 aint32() {
    return val_aint32;
}
bool aint32(const OptInt32& v) {
    val_aint32 = v;
    return true;
}
OptInt64 aint64() {
    return val_aint64;
}
bool aint64(const OptInt64& v) {
    val_aint64 = v;
    return true;
}
OptBool abool() {
    return val_abool;
}
bool abool(const OptBool& v) {
    val_abool = v;
    return true;
}
OptIntVec avector() {
    return val_avector;
}
bool avector(const OptIntVec& v) {
    val_avector = v;
    return true;
}
}  // namespace TestHalProperties
using namespace TestHalProperties;
#define AIDL_DEFAULT_astring "astringAIDL"
#define AIDL_DEFAULT_aint32 "320"
#define AIDL_DEFAULT_aint64 "640"
#define AIDL_DEFAULT_abool "true"
#define AIDL_DEFAULT_avector "10,20,30"
#define CREATE_GETTER_SETTER_WRAPPER(_NAME_, _T_)           \
    ConfigValue _NAME_##Getter() {                          \
        return TestHalProperties::_NAME_();                 \
    }                                                       \
    bool _NAME_##Setter(const ConfigValue& v) {             \
        return TestHalProperties::_NAME_(std::get<_T_>(v)); \
    }
CREATE_GETTER_SETTER_WRAPPER(astring, OptString)
CREATE_GETTER_SETTER_WRAPPER(aint32, OptInt32)
CREATE_GETTER_SETTER_WRAPPER(aint64, OptInt64)
CREATE_GETTER_SETTER_WRAPPER(abool, OptBool)
CREATE_GETTER_SETTER_WRAPPER(avector, std::vector<OptInt32>)

// Name,Getter, Setter, Parser and default value
#define NGS(_NAME_) #_NAME_, _NAME_##Getter, _NAME_##Setter
static Config::Data configData[] = {
        {NGS(astring), &Config::parseString, AIDL_DEFAULT_astring},
        {NGS(aint32), &Config::parseInt32, AIDL_DEFAULT_aint32},
        {NGS(aint64), &Config::parseInt64, AIDL_DEFAULT_aint64},
        {NGS(abool), &Config::parseBool, AIDL_DEFAULT_abool},
        {NGS(avector), &Config::parseIntVec, AIDL_DEFAULT_avector},
};

class TestConfig : public Config {
    Config::Data* getConfigData(int* size) {
        *size = sizeof(configData) / sizeof(configData[0]);
        return configData;
    }
};

class ConfigTest : public ::testing::Test {
  protected:
    void SetUp() override { cfg.init(); }
    void TearDown() override {}

    void switch2aidl() { cfg.setParam("astring", "astring"); }

    TestConfig cfg;
};

TEST_F(ConfigTest, parseInt32) {
    std::int32_t defval = 5678;
    struct {
        std::string strval;
        std::int32_t expval;
    } values[] = {
            {"1234", 1234},
            {"0", 0},
            {"", defval},
            {"xyz", defval},
    };
    for (int i = 0; i < sizeof(values) / sizeof(values[0]); i++) {
        ASSERT_EQ((std::get<OptInt32>(cfg.parseInt32(values[i].strval))).value_or(defval),
                  values[i].expval);
    }
}

TEST_F(ConfigTest, parseInt64) {
    std::int64_t defval = 5678;
    struct {
        std::string strval;
        std::int64_t expval;
    } values[] = {
            {"1234", 1234},  {"12345678909876", 12345678909876}, {"0", 0}, {"", defval},
            {"xyz", defval},
    };
    for (int i = 0; i < sizeof(values) / sizeof(values[0]); i++) {
        ASSERT_EQ((std::get<OptInt64>(cfg.parseInt64(values[i].strval))).value_or(defval),
                  values[i].expval);
    }
}

TEST_F(ConfigTest, parseBool) {
    bool defval = true;
    struct {
        std::string strval;
        bool expval;
    } values[] = {
            {"false", false},
            {"true", true},
            {"", defval},
            {"xyz", defval},
    };
    for (int i = 0; i < sizeof(values) / sizeof(values[0]); i++) {
        ASSERT_EQ((std::get<OptBool>(cfg.parseBool(values[i].strval))).value_or(defval),
                  values[i].expval);
    }
}

TEST_F(ConfigTest, parseIntVec) {
    std::vector<std::optional<int>> defval = {};
    struct {
        std::string strval;
        std::vector<std::optional<int>> expval;
    } values[] = {
            {"1", {1}}, {"1,2,3", {1, 2, 3}}, {"1,2,b", defval}, {"", defval}, {"xyz", defval},
    };
    for (int i = 0; i < sizeof(values) / sizeof(values[0]); i++) {
        ASSERT_EQ(std::get<OptIntVec>(cfg.parseIntVec(values[i].strval)), values[i].expval);
    }
}

TEST_F(ConfigTest, getters_sp) {
    ASSERT_EQ(cfg.get<std::string>("astring"), val_astring);
    ASSERT_EQ(cfg.get<std::int32_t>("aint32"), val_aint32);
    ASSERT_EQ(cfg.get<std::int64_t>("aint64"), val_aint64);
    ASSERT_EQ(cfg.get<bool>("abool"), val_abool);
    OptIntVec exp{val_avector};
    EXPECT_EQ(cfg.getopt<OptIntVec>("avector"), exp);
}

TEST_F(ConfigTest, setters_sp) {
    std::string val_astring_new("astringNew");
    ASSERT_TRUE(cfg.set<std::string>("astring", val_astring_new));
    ASSERT_EQ(cfg.get<std::string>("astring"), val_astring_new);

    std::int32_t val_aint32_new = val_aint32.value() + 100;
    ASSERT_TRUE(cfg.set<std::int32_t>("aint32", val_aint32_new));
    ASSERT_EQ(cfg.get<std::int32_t>("aint32"), val_aint32_new);

    std::int64_t val_aint64_new = val_aint64.value() + 200;
    ASSERT_TRUE(cfg.set<std::int64_t>("aint64", val_aint64_new));
    ASSERT_EQ(cfg.get<std::int64_t>("aint64"), val_aint64_new);

    bool val_abool_new = !val_abool.value();
    ASSERT_TRUE(cfg.set<bool>("abool", val_abool_new));
    ASSERT_EQ(cfg.get<bool>("abool"), val_abool_new);

    OptIntVec val_avector_new{100, 200};
    ASSERT_TRUE(cfg.setopt<OptIntVec>("avector", val_avector_new));
    EXPECT_EQ(cfg.getopt<OptIntVec>("avector"), val_avector_new);
}

TEST_F(ConfigTest, setters_sp_null) {
    val_astring = std::nullopt;
    ASSERT_EQ(cfg.get<std::string>("astring"),
              (std::get<OptString>(cfg.parseString(AIDL_DEFAULT_astring))).value());
}

TEST_F(ConfigTest, getters_aidl) {
    cfg.setParam("astring", "astringAIDL");
    ASSERT_EQ(cfg.get<std::string>("astring"),
              (std::get<OptString>(cfg.parseString(AIDL_DEFAULT_astring))).value());
    ASSERT_EQ(cfg.get<std::int32_t>("aint32"),
              (std::get<OptInt32>(cfg.parseInt32(AIDL_DEFAULT_aint32))).value());
    ASSERT_EQ(cfg.get<std::int64_t>("aint64"),
              (std::get<OptInt64>(cfg.parseInt64(AIDL_DEFAULT_aint64))).value());
    ASSERT_EQ(cfg.get<bool>("abool"),
              (std::get<OptBool>(cfg.parseBool(AIDL_DEFAULT_abool))).value());
    OptIntVec exp{std::get<OptIntVec>(cfg.parseIntVec(AIDL_DEFAULT_avector))};
    EXPECT_EQ(cfg.getopt<OptIntVec>("avector"), exp);
}

TEST_F(ConfigTest, setters_aidl) {
    std::string val_astring_new("astringNewAidl");
    ASSERT_TRUE(cfg.set<std::string>("astring", val_astring_new));
    ASSERT_EQ(cfg.get<std::string>("astring"), val_astring_new);

    std::int32_t val_aint32_new = val_aint32.value() + 1000;
    ASSERT_TRUE(cfg.set<std::int32_t>("aint32", val_aint32_new));
    ASSERT_EQ(cfg.get<std::int32_t>("aint32"), val_aint32_new);

    std::int64_t val_aint64_new = val_aint64.value() + 2000;
    ASSERT_TRUE(cfg.set<std::int64_t>("aint64", val_aint64_new));
    ASSERT_EQ(cfg.get<std::int64_t>("aint64"), val_aint64_new);

    bool val_abool_new = !val_abool.value();
    ASSERT_TRUE(cfg.set<bool>("abool", val_abool_new));
    ASSERT_EQ(cfg.get<bool>("abool"), val_abool_new);

    OptIntVec val_avector_new{1000, 2000};
    ASSERT_TRUE(cfg.setopt<OptIntVec>("avector", val_avector_new));
    EXPECT_EQ(cfg.getopt<OptIntVec>("avector"), val_avector_new);
}

TEST_F(ConfigTest, setParam) {
    ASSERT_TRUE(cfg.setParam("aint32", "789"));
    ASSERT_EQ(cfg.get<std::int32_t>("aint32"), 789);
    ASSERT_TRUE(cfg.setParam("avector", "7,8,9,10"));
    OptIntVec val_avector_new{7, 8, 9, 10};
    EXPECT_EQ(cfg.getopt<OptIntVec>("avector"), val_avector_new);
    ASSERT_FALSE(cfg.setParam("unknown", "any"));
}
}  // namespace aidl::android::hardware::biometrics
