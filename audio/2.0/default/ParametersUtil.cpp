/*
 * Copyright (C) 2016 The Android Open Source Project
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

#include "ParametersUtil.h"

namespace android {
namespace hardware {
namespace audio {
namespace V2_0 {
namespace implementation {

Result ParametersUtil::getParam(const char* name, bool* value) {
    String8 halValue;
    Result retval = getParam(name, &halValue);
    *value = false;
    if (retval == Result::OK) {
        *value = !(halValue == AudioParameter::valueOff);
    }
    return retval;
}

Result ParametersUtil::getParam(const char* name, int* value) {
    const String8 halName(name);
    AudioParameter keys;
    keys.addKey(halName);
    std::unique_ptr<AudioParameter> params = getParams(keys);
    status_t halStatus = params->getInt(halName, *value);
    return halStatus == OK ? Result::OK : Result::INVALID_ARGUMENTS;
}

Result ParametersUtil::getParam(const char* name, String8* value) {
    const String8 halName(name);
    AudioParameter keys;
    keys.addKey(halName);
    std::unique_ptr<AudioParameter> params = getParams(keys);
    status_t halStatus = params->get(halName, *value);
    return halStatus == OK ? Result::OK : Result::INVALID_ARGUMENTS;
}

void ParametersUtil::getParametersImpl(
        const hidl_vec<hidl_string>& keys,
        std::function<void(Result retval, const hidl_vec<ParameterValue>& parameters)> cb)  {
    AudioParameter halKeys;
    for (size_t i = 0; i < keys.size(); ++i) {
        halKeys.addKey(String8(keys[i].c_str()));
    }
    std::unique_ptr<AudioParameter> halValues = getParams(halKeys);
    Result retval(Result::INVALID_ARGUMENTS);
    hidl_vec<ParameterValue> result;
    if (halValues->size() > 0) {
        result.resize(halValues->size());
        String8 halKey, halValue;
        for (size_t i = 0; i < halValues->size(); ++i) {
            status_t status = halValues->getAt(i, halKey, halValue);
            if (status != OK) {
                result.resize(0);
                break;
            }
            result[i].key = halKey.string();
            result[i].value = halValue.string();
        }
        if (result.size() != 0) {
            retval = Result::OK;
        }
    }
    cb(retval, result);
}

std::unique_ptr<AudioParameter> ParametersUtil::getParams(const AudioParameter& keys) {
    String8 paramsAndValues;
    char *halValues = halGetParameters(keys.keysToString().string());
    if (halValues != NULL) {
        paramsAndValues.setTo(halValues);
        free(halValues);
    } else {
        paramsAndValues.clear();
    }
    return std::unique_ptr<AudioParameter>(new AudioParameter(paramsAndValues));
}

Result ParametersUtil::setParam(const char* name, bool value) {
    AudioParameter param;
    param.add(String8(name), String8(value ? AudioParameter::valueOn : AudioParameter::valueOff));
    return setParams(param);
}

Result ParametersUtil::setParam(const char* name, int value) {
    AudioParameter param;
    param.addInt(String8(name), value);
    return setParams(param);
}

Result ParametersUtil::setParam(const char* name, const char* value) {
    AudioParameter param;
    param.add(String8(name), String8(value));
    return setParams(param);
}

Result ParametersUtil::setParametersImpl(const hidl_vec<ParameterValue>& parameters)  {
    AudioParameter params;
    for (size_t i = 0; i < parameters.size(); ++i) {
        params.add(String8(parameters[i].key.c_str()), String8(parameters[i].value.c_str()));
    }
    return setParams(params);
}

Result ParametersUtil::setParams(const AudioParameter& param) {
    int halStatus = halSetParameters(param.toString().string());
    if (halStatus == OK)
        return Result::OK;
    else if (halStatus == -ENOSYS)
        return Result::INVALID_STATE;
    else
        return Result::INVALID_ARGUMENTS;
}

}  // namespace implementation
}  // namespace V2_0
}  // namespace audio
}  // namespace hardware
}  // namespace android
