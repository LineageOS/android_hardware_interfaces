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
#ifndef CONFIG_MANAGER_UTIL_H
#define CONFIG_MANAGER_UTIL_H

#include <utility>
#include <string>
#include <system/camera_metadata.h>
#include <android/hardware/automotive/evs/1.1/types.h>

using namespace std;
using ::android::hardware::automotive::evs::V1_1::CameraParam;


class ConfigManagerUtil {
public:
    /**
     * Convert a given string into V4L2_CID_*
     */
    static bool convertToEvsCameraParam(const string &id,
                                        CameraParam &camParam);
    /**
     * Convert a given string into android.hardware.graphics.common.PixelFormat
     */
    static bool convertToPixelFormat(const string &format,
                                     int32_t &pixelFormat);
    /**
     * Convert a given string into corresponding camera metadata data tag defined in
     * system/media/camera/include/system/camera_metadta_tags.h
     */
    static bool convertToMetadataTag(const char *name,
                                     camera_metadata_tag &aTag);
    /**
     * Convert a given string into a floating value array
     */
    static float *convertFloatArray(const char *sz,
                                    const char *vals,
                                    size_t &count,
                                    const char delimiter = ',');
    /**
     * Trim a string
     */
    static string trimString(const string &src,
                             const string &ws = " \n\r\t\f\v");

    /**
     * Convert a given string to corresponding camera capabilities
     */
    static bool convertToCameraCapability(
        const char *name,
        camera_metadata_enum_android_request_available_capabilities_t &cap);

};

#endif // CONFIG_MANAGER_UTIL_H

