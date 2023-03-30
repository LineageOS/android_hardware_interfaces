/*
 * Copyright (C) 2023 The Android Open Source Project
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

#include <android-base/logging.h>
#include <android-base/parsedouble.h>
#include <android-base/parseint.h>
#include <linux/videodev2.h>

#include <sstream>
#include <string>

#include <system/graphics-base-v1.0.h>

using ::aidl::android::hardware::automotive::evs::CameraParam;
using ::aidl::android::hardware::graphics::common::PixelFormat;

bool ConfigManagerUtil::convertToEvsCameraParam(const std::string& id, CameraParam& camParam) {
    std::string trimmed = ConfigManagerUtil::trimString(id);
    bool success = true;

    if (!trimmed.compare("BRIGHTNESS")) {
        camParam = CameraParam::BRIGHTNESS;
    } else if (!trimmed.compare("CONTRAST")) {
        camParam = CameraParam::CONTRAST;
    } else if (!trimmed.compare("AUTOGAIN")) {
        camParam = CameraParam::AUTOGAIN;
    } else if (!trimmed.compare("GAIN")) {
        camParam = CameraParam::GAIN;
    } else if (!trimmed.compare("AUTO_WHITE_BALANCE")) {
        camParam = CameraParam::AUTO_WHITE_BALANCE;
    } else if (!trimmed.compare("WHITE_BALANCE_TEMPERATURE")) {
        camParam = CameraParam::WHITE_BALANCE_TEMPERATURE;
    } else if (!trimmed.compare("SHARPNESS")) {
        camParam = CameraParam::SHARPNESS;
    } else if (!trimmed.compare("AUTO_EXPOSURE")) {
        camParam = CameraParam::AUTO_EXPOSURE;
    } else if (!trimmed.compare("ABSOLUTE_EXPOSURE")) {
        camParam = CameraParam::ABSOLUTE_EXPOSURE;
    } else if (!trimmed.compare("ABSOLUTE_FOCUS")) {
        camParam = CameraParam::ABSOLUTE_FOCUS;
    } else if (!trimmed.compare("AUTO_FOCUS")) {
        camParam = CameraParam::AUTO_FOCUS;
    } else if (!trimmed.compare("ABSOLUTE_ZOOM")) {
        camParam = CameraParam::ABSOLUTE_ZOOM;
    } else {
        success = false;
    }

    return success;
}

bool ConfigManagerUtil::convertToPixelFormat(const std::string& in, PixelFormat& out) {
    std::string trimmed = ConfigManagerUtil::trimString(in);
    bool success = true;

    if (!trimmed.compare("RGBA_8888")) {
        out = PixelFormat::RGBA_8888;
    } else if (!trimmed.compare("YCRCB_420_SP")) {
        out = PixelFormat::YCRCB_420_SP;
    } else if (!trimmed.compare("YCBCR_422_I")) {
        out = PixelFormat::YCBCR_422_I;
    } else {
        out = PixelFormat::UNSPECIFIED;
        success = false;
    }

    return success;
}

bool ConfigManagerUtil::convertToMetadataTag(const char* name, camera_metadata_tag& aTag) {
    if (!std::strcmp(name, "LENS_DISTORTION")) {
        aTag = ANDROID_LENS_DISTORTION;
    } else if (!std::strcmp(name, "LENS_INTRINSIC_CALIBRATION")) {
        aTag = ANDROID_LENS_INTRINSIC_CALIBRATION;
    } else if (!std::strcmp(name, "LENS_POSE_ROTATION")) {
        aTag = ANDROID_LENS_POSE_ROTATION;
    } else if (!std::strcmp(name, "LENS_POSE_TRANSLATION")) {
        aTag = ANDROID_LENS_POSE_TRANSLATION;
    } else if (!std::strcmp(name, "REQUEST_AVAILABLE_CAPABILITIES")) {
        aTag = ANDROID_REQUEST_AVAILABLE_CAPABILITIES;
    } else if (!std::strcmp(name, "LOGICAL_MULTI_CAMERA_PHYSICAL_IDS")) {
        aTag = ANDROID_LOGICAL_MULTI_CAMERA_PHYSICAL_IDS;
    } else {
        return false;
    }

    return true;
}

bool ConfigManagerUtil::convertToCameraCapability(
        const char* name, camera_metadata_enum_android_request_available_capabilities_t& cap) {
    if (!std::strcmp(name, "DEPTH_OUTPUT")) {
        cap = ANDROID_REQUEST_AVAILABLE_CAPABILITIES_DEPTH_OUTPUT;
    } else if (!std::strcmp(name, "LOGICAL_MULTI_CAMERA")) {
        cap = ANDROID_REQUEST_AVAILABLE_CAPABILITIES_LOGICAL_MULTI_CAMERA;
    } else if (!std::strcmp(name, "MONOCHROME")) {
        cap = ANDROID_REQUEST_AVAILABLE_CAPABILITIES_MONOCHROME;
    } else if (!std::strcmp(name, "SECURE_IMAGE_DATA")) {
        cap = ANDROID_REQUEST_AVAILABLE_CAPABILITIES_SECURE_IMAGE_DATA;
    } else {
        return false;
    }

    return true;
}

float* ConfigManagerUtil::convertFloatArray(const char* sz, const char* vals, size_t& count,
                                            const char delimiter) {
    std::string size_string(sz);
    std::string value_string(vals);

    if (!android::base::ParseUint(size_string, &count)) {
        LOG(ERROR) << "Failed to parse " << size_string;
        return nullptr;
    }
    float* result = new float[count];
    std::stringstream values(value_string);

    int32_t idx = 0;
    std::string token;
    while (getline(values, token, delimiter)) {
        if (!android::base::ParseFloat(token, &result[idx++])) {
            LOG(WARNING) << "Failed to parse " << token;
        }
    }

    return result;
}

std::string ConfigManagerUtil::trimString(const std::string& src, const std::string& ws) {
    const auto s = src.find_first_not_of(ws);
    if (s == std::string::npos) {
        return "";
    }

    const auto e = src.find_last_not_of(ws);
    const auto r = e - s + 1;

    return src.substr(s, r);
}
