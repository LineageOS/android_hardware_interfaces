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

#define LOG_TAG "VirtualHalConfig"

#include "config/Config.h"
#include <android-base/logging.h>
#include <android-base/parseint.h>
#include "../../util/include/util/Util.h"

using ::android::base::ParseInt;

namespace aidl::android::hardware::biometrics {

Config::Config() : mSource(Config::ConfigSourceType::SOURCE_SYSPROP) {}

ConfigValue Config::parseBool(const std::string& value) {
    OptBool res;
    if (value == "true")
        res.emplace(true);
    else if (value == "false")
        res.emplace(false);
    else
        LOG(ERROR) << "ERROR: invalid bool " << value;
    return res;
}

ConfigValue Config::parseString(const std::string& value) {
    OptString res;
    if (!value.empty()) res.emplace(value);
    return res;
}

ConfigValue Config::parseInt32(const std::string& value) {
    OptInt32 res;
    if (!value.empty()) {
        std::int32_t val;
        if (ParseInt(value, &val)) res.emplace(val);
    }
    return res;
}

ConfigValue Config::parseInt64(const std::string& value) {
    OptInt64 res;
    if (!value.empty()) {
        std::int64_t val = std::strtoull(value.c_str(), nullptr, 10);
        if (val != 0LL or (val == 0LL && value == "0")) {
            res.emplace(val);
        }
    }
    return res;
}

ConfigValue Config::parseIntVec(const std::string& value) {
    OptIntVec res;
    for (auto& i : Util::parseIntSequence(value)) {
        res.push_back(i);
    }
    return res;
}

void Config::init() {
    LOG(INFO) << "calling init()";
    int len = 0;
    Config::Data* pd = getConfigData(&len);
    for (int i = 0; i < len; i++) {
        LOG(INFO) << "init():" << pd->name;
        pd->value = (this->*(pd->parser))(pd->defaultValue);
        setConfig(pd->name, *pd);
        ++pd;
    }
}

bool Config::setParam(const std::string& name, const std::string& value) {
    auto it = mMap.find(name);
    if (it == mMap.end()) {
        LOG(ERROR) << "ERROR: setParam unknown config name " << name;
        return false;
    }
    LOG(INFO) << "setParam name=" << name << "=" << value;

    it->second.value = (this->*(it->second.parser))(value);

    mSource = ConfigSourceType::SOURCE_AIDL;

    return true;
}

ConfigValue Config::getInternal(const std::string& name) {
    ConfigValue res;

    auto data = mMap[name];
    switch (mSource) {
        case ConfigSourceType::SOURCE_SYSPROP:
            res = data.getter();
            break;
        case ConfigSourceType::SOURCE_AIDL:
            res = data.value;
            break;
        case ConfigSourceType::SOURCE_FILE:
            LOG(WARNING) << "Unsupported";
            break;
        default:
            LOG(ERROR) << " wrong srouce type " << (int)mSource;
            break;
    }

    return res;
}

ConfigValue Config::getDefault(const std::string& name) {
    return mMap[name].value;
}

bool Config::setInternal(const std::string& name, const ConfigValue& val) {
    bool res = false;
    auto data = mMap[name];

    switch (mSource) {
        case ConfigSourceType::SOURCE_SYSPROP:
            res = data.setter(val);
            break;
        case ConfigSourceType::SOURCE_AIDL:
            data.value = val;
            res = true;
            break;
        case ConfigSourceType::SOURCE_FILE:
            LOG(WARNING) << "Unsupported";
            break;
        default:
            LOG(ERROR) << " wrong srouce type " << (int)mSource;
            break;
    }

    return res;
}
}  // namespace aidl::android::hardware::biometrics
