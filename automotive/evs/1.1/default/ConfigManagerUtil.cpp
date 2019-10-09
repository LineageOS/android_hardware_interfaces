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

#include "ConfigManagerUtil.h"

#include <string>
#include <sstream>
#include <linux/videodev2.h>

#include <log/log.h>
#include <system/graphics-base-v1.0.h>


bool ConfigManagerUtil::convertToEvsCameraParam(const string &id,
                                                CameraParam &camParam) {
    string trimmed = ConfigManagerUtil::trimString(id);
    bool success = true;

    if (!trimmed.compare("BRIGHTNESS")) {
        camParam =  CameraParam::BRIGHTNESS;
    } else if (!trimmed.compare("CONTRAST")) {
        camParam =  CameraParam::CONTRAST;
    } else if (!trimmed.compare("AUTOGAIN")) {
        camParam =  CameraParam::AUTOGAIN;
    } else if (!trimmed.compare("GAIN")) {
        camParam =  CameraParam::GAIN;
    } else if (!trimmed.compare("AUTO_WHITE_BALANCE")) {
        camParam =  CameraParam::AUTO_WHITE_BALANCE;
    } else if (!trimmed.compare("WHITE_BALANCE_TEMPERATURE")) {
        camParam =  CameraParam::WHITE_BALANCE_TEMPERATURE;
    } else if (!trimmed.compare("SHARPNESS")) {
        camParam =  CameraParam::SHARPNESS;
    } else if (!trimmed.compare("AUTO_EXPOSURE")) {
        camParam =  CameraParam::AUTO_EXPOSURE;
    } else if (!trimmed.compare("ABSOLUTE_EXPOSURE")) {
        camParam =  CameraParam::ABSOLUTE_EXPOSURE;
    } else if (!trimmed.compare("ABSOLUTE_FOCUS")) {
        camParam =  CameraParam::ABSOLUTE_FOCUS;
    } else if (!trimmed.compare("AUTO_FOCUS")) {
        camParam =  CameraParam::AUTO_FOCUS;
    } else if (!trimmed.compare("ABSOLUTE_ZOOM")) {
        camParam =  CameraParam::ABSOLUTE_ZOOM;
    } else {
        success = false;
    }

    return success;
}


bool ConfigManagerUtil::convertToPixelFormat(const string &format,
                                             int32_t &pixFormat) {
    string trimmed = ConfigManagerUtil::trimString(format);
    bool success = true;

    if (!trimmed.compare("RGBA_8888")) {
        pixFormat =  HAL_PIXEL_FORMAT_RGBA_8888;
    } else if (!trimmed.compare("YCRCB_420_SP")) {
        pixFormat =  HAL_PIXEL_FORMAT_YCRCB_420_SP;
    } else if (!trimmed.compare("YCBCR_422_I")) {
        pixFormat =  HAL_PIXEL_FORMAT_YCBCR_422_I;
    } else {
        success = false;
    }

    return success;
}


bool ConfigManagerUtil::convertToMetadataTag(const char *name,
                                             camera_metadata_tag &aTag) {
    if (!strcmp(name, "LENS_DISTORTION")) {
        aTag =  ANDROID_LENS_DISTORTION;
    } else if (!strcmp(name, "LENS_INTRINSIC_CALIBRATION")) {
        aTag =  ANDROID_LENS_INTRINSIC_CALIBRATION;
    } else if (!strcmp(name, "LENS_POSE_ROTATION")) {
        aTag =  ANDROID_LENS_POSE_ROTATION;
    } else if (!strcmp(name, "LENS_POSE_TRANSLATION")) {
        aTag =  ANDROID_LENS_POSE_TRANSLATION;
    } else if (!strcmp(name, "REQUEST_AVAILABLE_CAPABILITIES")) {
        aTag =  ANDROID_REQUEST_AVAILABLE_CAPABILITIES;
    } else if (!strcmp(name, "LOGICAL_MULTI_CAMERA_PHYSICAL_IDS")) {
        aTag =  ANDROID_LOGICAL_MULTI_CAMERA_PHYSICAL_IDS;
    } else {
        return false;
    }

    return true;
}


bool ConfigManagerUtil::convertToCameraCapability(
    const char *name,
    camera_metadata_enum_android_request_available_capabilities_t &cap) {

    if (!strcmp(name, "DEPTH_OUTPUT")) {
        cap = ANDROID_REQUEST_AVAILABLE_CAPABILITIES_DEPTH_OUTPUT;
    } else if (!strcmp(name, "LOGICAL_MULTI_CAMERA")) {
        cap = ANDROID_REQUEST_AVAILABLE_CAPABILITIES_LOGICAL_MULTI_CAMERA;
    } else if (!strcmp(name, "MONOCHROME")) {
        cap = ANDROID_REQUEST_AVAILABLE_CAPABILITIES_MONOCHROME;
    } else if (!strcmp(name, "SECURE_IMAGE_DATA")) {
        cap = ANDROID_REQUEST_AVAILABLE_CAPABILITIES_SECURE_IMAGE_DATA;
    } else {
        return false;
    }

    return true;
}


float *ConfigManagerUtil::convertFloatArray(const char *sz, const char *vals,
                                            size_t &count, const char delimiter) {
    string size_string(sz);
    string value_string(vals);

    count = stoi(size_string);
    float *result = new float[count];
    stringstream values(value_string);

    int32_t idx = 0;
    string token;
    while (getline(values, token, delimiter)) {
        result[idx++] = stof(token);
    }

    return result;
}


string ConfigManagerUtil::trimString(const string &src, const string &ws) {
    const auto s = src.find_first_not_of(ws);
    if (s == string::npos) {
        return "";
    }

    const auto e = src.find_last_not_of(ws);
    const auto r = e - s + 1;

    return src.substr(s, r);
}

