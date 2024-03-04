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

#pragma once

#include <android-base/logging.h>
#include <cstdint>
#include <iostream>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <variant>
#include <vector>

namespace aidl::android::hardware::biometrics {

using OptBool = std::optional<bool>;
using OptInt32 = std::optional<std::int32_t>;
using OptInt64 = std::optional<std::int64_t>;
using OptString = std::optional<std::string>;
using OptIntVec = std::vector<std::optional<std::int32_t>>;

using ConfigValue = std::variant<OptBool, OptInt32, OptInt64, OptString, OptIntVec>;

class Config {
  public:
    struct Data {
        std::string name;
        ConfigValue (*getter)();
        bool (*setter)(const ConfigValue&);
        ConfigValue (Config::*parser)(const std::string&);
        std::string defaultValue;
        ConfigValue value;
    };
    enum class ConfigSourceType { SOURCE_SYSPROP, SOURCE_AIDL, SOURCE_FILE };

  public:
    Config();
    virtual ~Config() = default;

    template <typename T>
    T get(const std::string& name) {
        CHECK(mMap.count(name) > 0) << " biometric/config get invalid name: " << name;
        std::optional<T> optval = std::get<std::optional<T>>(getInternal(name));
        if (!optval) optval = std::get<std::optional<T>>(getDefault(name));
        if (optval) return optval.value();
        return T();
    }
    template <typename T>
    bool set(const std::string& name, const T& val) {
        CHECK(mMap.count(name) > 0) << " biometric/config set invalid name: " << name;
        std::optional<T> aval(val);
        ConfigValue cval(aval);
        return setInternal(name, cval);
    }
    template <typename T>
    T getopt(const std::string& name) {
        CHECK(mMap.count(name) > 0) << " biometric/config get invalid name: " << name;
        return std::get<T>(getInternal(name));
    }
    template <typename T>
    bool setopt(const std::string& name, const T& val) {
        CHECK(mMap.count(name) > 0) << " biometric/config set invalid name: " << name;
        ConfigValue cval(val);
        return setInternal(name, cval);
    }

    void init();

    virtual Config::Data* getConfigData(int* size) = 0;
    bool setParam(const std::string& name, const std::string& value);

    ConfigValue parseBool(const std::string& value);
    ConfigValue parseString(const std::string& name);
    ConfigValue parseInt32(const std::string& value);
    ConfigValue parseInt64(const std::string& value);
    ConfigValue parseIntVec(const std::string& value);

  protected:
    void setConfig(const std::string& name, const Config::Data& value) { mMap[name] = value; }

  private:
    ConfigValue getInternal(const std::string& name);
    bool setInternal(const std::string& name, const ConfigValue& val);
    ConfigValue getDefault(const std::string& name);

    Config::ConfigSourceType mSource;
    std::map<std::string, Config::Data> mMap;
};

}  // namespace aidl::android::hardware::biometrics
