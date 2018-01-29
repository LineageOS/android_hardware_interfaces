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

#ifndef ANDROID_HARDWARE_INTERFACES_CAMERA_COMMON_1_0_EXIF_H
#define ANDROID_HARDWARE_INTERFACES_CAMERA_COMMON_1_0_EXIF_H

#include "CameraMetadata.h"

namespace android {
namespace hardware {
namespace camera {
namespace common {
namespace V1_0 {
namespace helper {


// This is based on the original ChromeOS ARC implementation of a V4L2 HAL

// ExifUtils can generate APP1 segment with tags which caller set. ExifUtils can
// also add a thumbnail in the APP1 segment if thumbnail size is specified.
// ExifUtils can be reused with different images by calling initialize().
//
// Example of using this class :
//  std::unique_ptr<ExifUtils> utils(ExifUtils::Create());
//  utils->initialize();
//  ...
//  // Call ExifUtils functions to set Exif tags.
//  ...
//  utils->GenerateApp1(thumbnail_buffer, thumbnail_size);
//  unsigned int app1Length = utils->GetApp1Length();
//  uint8_t* app1Buffer = new uint8_t[app1Length];
//  memcpy(app1Buffer, utils->GetApp1Buffer(), app1Length);
class ExifUtils {

 public:
    virtual ~ExifUtils();

    static ExifUtils* create();

    // Initialize() can be called multiple times. The setting of Exif tags will be
    // cleared.
    virtual bool initialize() = 0;

    // Set all known fields from a metadata structure
    virtual bool setFromMetadata(const CameraMetadata& metadata,
                                 const size_t imageWidth,
                                 const size_t imageHeight) = 0;

    // Sets the len aperture.
    // Returns false if memory allocation fails.
    virtual bool setAperture(uint32_t numerator, uint32_t denominator) = 0;

    // Sets the value of brightness.
    // Returns false if memory allocation fails.
    virtual bool setBrightness(int32_t numerator, int32_t denominator) = 0;

    // Sets the color space.
    // Returns false if memory allocation fails.
    virtual bool setColorSpace(uint16_t color_space) = 0;

    // Sets the information to compressed data.
    // Returns false if memory allocation fails.
    virtual bool setComponentsConfiguration(const std::string& components_configuration) = 0;

    // Sets the compression scheme used for the image data.
    // Returns false if memory allocation fails.
    virtual bool setCompression(uint16_t compression) = 0;

    // Sets image contrast.
    // Returns false if memory allocation fails.
    virtual bool setContrast(uint16_t contrast) = 0;

    // Sets the date and time of image last modified. It takes local time. The
    // name of the tag is DateTime in IFD0.
    // Returns false if memory allocation fails.
    virtual bool setDateTime(const struct tm& t) = 0;

    // Sets the image description.
    // Returns false if memory allocation fails.
    virtual bool setDescription(const std::string& description) = 0;

    // Sets the digital zoom ratio. If the numerator is 0, it means digital zoom
    // was not used.
    // Returns false if memory allocation fails.
    virtual bool setDigitalZoomRatio(uint32_t numerator, uint32_t denominator) = 0;

    // Sets the exposure bias.
    // Returns false if memory allocation fails.
    virtual bool setExposureBias(int32_t numerator, int32_t denominator) = 0;

    // Sets the exposure mode set when the image was shot.
    // Returns false if memory allocation fails.
    virtual bool setExposureMode(uint16_t exposure_mode) = 0;

    // Sets the program used by the camera to set exposure when the picture is
    // taken.
    // Returns false if memory allocation fails.
    virtual bool setExposureProgram(uint16_t exposure_program) = 0;

    // Sets the exposure time, given in seconds.
    // Returns false if memory allocation fails.
    virtual bool setExposureTime(uint32_t numerator, uint32_t denominator) = 0;

    // Sets the status of flash.
    // Returns false if memory allocation fails.
    virtual bool setFlash(uint16_t flash) = 0;

    // Sets the F number.
    // Returns false if memory allocation fails.
    virtual bool setFNumber(uint32_t numerator, uint32_t denominator) = 0;

    // Sets the focal length of lens used to take the image in millimeters.
    // Returns false if memory allocation fails.
    virtual bool setFocalLength(uint32_t numerator, uint32_t denominator) = 0;

    // Sets the degree of overall image gain adjustment.
    // Returns false if memory allocation fails.
    virtual bool setGainControl(uint16_t gain_control) = 0;

    // Sets the altitude in meters.
    // Returns false if memory allocation fails.
    virtual bool setGpsAltitude(double altitude) = 0;

    // Sets the latitude with degrees minutes seconds format.
    // Returns false if memory allocation fails.
    virtual bool setGpsLatitude(double latitude) = 0;

    // Sets the longitude with degrees minutes seconds format.
    // Returns false if memory allocation fails.
    virtual bool setGpsLongitude(double longitude) = 0;

    // Sets GPS processing method.
    // Returns false if memory allocation fails.
    virtual bool setGpsProcessingMethod(const std::string& method) = 0;

    // Sets GPS date stamp and time stamp (atomic clock). It takes UTC time.
    // Returns false if memory allocation fails.
    virtual bool setGpsTimestamp(const struct tm& t) = 0;

    // Sets the height (number of rows) of main image.
    // Returns false if memory allocation fails.
    virtual bool setImageHeight(uint32_t length) = 0;

    // Sets the width (number of columns) of main image.
    // Returns false if memory allocation fails.
    virtual bool setImageWidth(uint32_t width) = 0;

    // Sets the ISO speed.
    // Returns false if memory allocation fails.
    virtual bool setIsoSpeedRating(uint16_t iso_speed_ratings) = 0;

    // Sets the kind of light source.
    // Returns false if memory allocation fails.
    virtual bool setLightSource(uint16_t light_source) = 0;

    // Sets the smallest F number of the lens.
    // Returns false if memory allocation fails.
    virtual bool setMaxAperture(uint32_t numerator, uint32_t denominator) = 0;

    // Sets the metering mode.
    // Returns false if memory allocation fails.
    virtual bool setMeteringMode(uint16_t metering_mode) = 0;

    // Sets image orientation.
    // Returns false if memory allocation fails.
    virtual bool setOrientation(uint16_t orientation) = 0;

    // Sets the unit for measuring XResolution and YResolution.
    // Returns false if memory allocation fails.
    virtual bool setResolutionUnit(uint16_t resolution_unit) = 0;

    // Sets image saturation.
    // Returns false if memory allocation fails.
    virtual bool setSaturation(uint16_t saturation) = 0;

    // Sets the type of scene that was shot.
    // Returns false if memory allocation fails.
    virtual bool setSceneCaptureType(uint16_t type) = 0;

    // Sets image sharpness.
    // Returns false if memory allocation fails.
    virtual bool setSharpness(uint16_t sharpness) = 0;

    // Sets the shutter speed.
    // Returns false if memory allocation fails.
    virtual bool setShutterSpeed(int32_t numerator, int32_t denominator) = 0;

    // Sets the distance to the subject, given in meters.
    // Returns false if memory allocation fails.
    virtual bool setSubjectDistance(uint32_t numerator, uint32_t denominator) = 0;

    // Sets the fractions of seconds for the <DateTime> tag.
    // Returns false if memory allocation fails.
    virtual bool setSubsecTime(const std::string& subsec_time) = 0;

    // Sets the white balance mode set when the image was shot.
    // Returns false if memory allocation fails.
    virtual bool setWhiteBalance(uint16_t white_balance) = 0;

    // Sets the number of pixels per resolution unit in the image width.
    // Returns false if memory allocation fails.
    virtual bool setXResolution(uint32_t numerator, uint32_t denominator) = 0;

    // Sets the position of chrominance components in relation to the luminance
    // component.
    // Returns false if memory allocation fails.
    virtual bool setYCbCrPositioning(uint16_t ycbcr_positioning) = 0;

    // Sets the number of pixels per resolution unit in the image length.
    // Returns false if memory allocation fails.
    virtual bool setYResolution(uint32_t numerator, uint32_t denominator) = 0;

    // Sets the manufacturer of camera.
    // Returns false if memory allocation fails.
    virtual bool setMake(const std::string& make) = 0;

    // Sets the model number of camera.
    // Returns false if memory allocation fails.
    virtual bool setModel(const std::string& model) = 0;

    // Generates APP1 segment.
    // Returns false if generating APP1 segment fails.
    virtual bool generateApp1(const void* thumbnail_buffer, uint32_t size) = 0;

    // Gets buffer of APP1 segment. This method must be called only after calling
    // GenerateAPP1().
    virtual const uint8_t* getApp1Buffer() = 0;

    // Gets length of APP1 segment. This method must be called only after calling
    // GenerateAPP1().
    virtual unsigned int getApp1Length() = 0;
};


} // namespace helper
} // namespace V1_0
} // namespace common
} // namespace camera
} // namespace hardware
} // namespace android


#endif  // ANDROID_HARDWARE_INTERFACES_CAMERA_COMMON_1_0_EXIF_H
