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

#define LOG_TAG "CamComm1.0-Exif"
#define ATRACE_TAG ATRACE_TAG_CAMERA
//#define LOG_NDEBUG 0

#include <android/log.h>

#include <inttypes.h>
#include <math.h>
#include <stdint.h>
#include <string>
#include <vector>

#include "Exif.h"

extern "C" {
#include <libexif/exif-data.h>
}

namespace std {

template <>
struct default_delete<ExifEntry> {
    inline void operator()(ExifEntry* entry) const { exif_entry_unref(entry); }
};

}  // namespace std


namespace android {
namespace hardware {
namespace camera {
namespace common {
namespace V1_0 {
namespace helper {


class ExifUtilsImpl : public ExifUtils {
  public:
    ExifUtilsImpl();

    virtual ~ExifUtilsImpl();

    // Initialize() can be called multiple times. The setting of Exif tags will be
    // cleared.
    virtual bool initialize();

    // set all known fields from a metadata structure
    virtual bool setFromMetadata(const CameraMetadata& metadata,
                                 const size_t imageWidth,
                                 const size_t imageHeight);

    // sets the len aperture.
    // Returns false if memory allocation fails.
    virtual bool setAperture(uint32_t numerator, uint32_t denominator);

    // sets the value of brightness.
    // Returns false if memory allocation fails.
    virtual bool setBrightness(int32_t numerator, int32_t denominator);

    // sets the color space.
    // Returns false if memory allocation fails.
    virtual bool setColorSpace(uint16_t color_space);

    // sets the information to compressed data.
    // Returns false if memory allocation fails.
    virtual bool setComponentsConfiguration(const std::string& components_configuration);

    // sets the compression scheme used for the image data.
    // Returns false if memory allocation fails.
    virtual bool setCompression(uint16_t compression);

    // sets image contrast.
    // Returns false if memory allocation fails.
    virtual bool setContrast(uint16_t contrast);

    // sets the date and time of image last modified. It takes local time. The
    // name of the tag is DateTime in IFD0.
    // Returns false if memory allocation fails.
    virtual bool setDateTime(const struct tm& t);

    // sets the image description.
    // Returns false if memory allocation fails.
    virtual bool setDescription(const std::string& description);

    // sets the digital zoom ratio. If the numerator is 0, it means digital zoom
    // was not used.
    // Returns false if memory allocation fails.
    virtual bool setDigitalZoomRatio(uint32_t numerator, uint32_t denominator);

    // sets the exposure bias.
    // Returns false if memory allocation fails.
    virtual bool setExposureBias(int32_t numerator, int32_t denominator);

    // sets the exposure mode set when the image was shot.
    // Returns false if memory allocation fails.
    virtual bool setExposureMode(uint16_t exposure_mode);

    // sets the program used by the camera to set exposure when the picture is
    // taken.
    // Returns false if memory allocation fails.
    virtual bool setExposureProgram(uint16_t exposure_program);

    // sets the exposure time, given in seconds.
    // Returns false if memory allocation fails.
    virtual bool setExposureTime(uint32_t numerator, uint32_t denominator);

    // sets the status of flash.
    // Returns false if memory allocation fails.
    virtual bool setFlash(uint16_t flash);

    // sets the F number.
    // Returns false if memory allocation fails.
    virtual bool setFNumber(uint32_t numerator, uint32_t denominator);

    // sets the focal length of lens used to take the image in millimeters.
    // Returns false if memory allocation fails.
    virtual bool setFocalLength(uint32_t numerator, uint32_t denominator);

    // sets the degree of overall image gain adjustment.
    // Returns false if memory allocation fails.
    virtual bool setGainControl(uint16_t gain_control);

    // sets the altitude in meters.
    // Returns false if memory allocation fails.
    virtual bool setGpsAltitude(double altitude);

    // sets the latitude with degrees minutes seconds format.
    // Returns false if memory allocation fails.
    virtual bool setGpsLatitude(double latitude);

    // sets the longitude with degrees minutes seconds format.
    // Returns false if memory allocation fails.
    virtual bool setGpsLongitude(double longitude);

    // sets GPS processing method.
    // Returns false if memory allocation fails.
    virtual bool setGpsProcessingMethod(const std::string& method);

    // sets GPS date stamp and time stamp (atomic clock). It takes UTC time.
    // Returns false if memory allocation fails.
    virtual bool setGpsTimestamp(const struct tm& t);

    // sets the length (number of rows) of main image.
    // Returns false if memory allocation fails.
    virtual bool setImageHeight(uint32_t length);

    // sets the width (number of columes) of main image.
    // Returns false if memory allocation fails.
    virtual bool setImageWidth(uint32_t width);

    // sets the ISO speed.
    // Returns false if memory allocation fails.
    virtual bool setIsoSpeedRating(uint16_t iso_speed_ratings);

    // sets the kind of light source.
    // Returns false if memory allocation fails.
    virtual bool setLightSource(uint16_t light_source);

    // sets the smallest F number of the lens.
    // Returns false if memory allocation fails.
    virtual bool setMaxAperture(uint32_t numerator, uint32_t denominator);

    // sets the metering mode.
    // Returns false if memory allocation fails.
    virtual bool setMeteringMode(uint16_t metering_mode);

    // sets image orientation.
    // Returns false if memory allocation fails.
    virtual bool setOrientation(uint16_t orientation);

    // sets the unit for measuring XResolution and YResolution.
    // Returns false if memory allocation fails.
    virtual bool setResolutionUnit(uint16_t resolution_unit);

    // sets image saturation.
    // Returns false if memory allocation fails.
    virtual bool setSaturation(uint16_t saturation);

    // sets the type of scene that was shot.
    // Returns false if memory allocation fails.
    virtual bool setSceneCaptureType(uint16_t type);

    // sets image sharpness.
    // Returns false if memory allocation fails.
    virtual bool setSharpness(uint16_t sharpness);

    // sets the shutter speed.
    // Returns false if memory allocation fails.
    virtual bool setShutterSpeed(int32_t numerator, int32_t denominator);

    // sets the distance to the subject, given in meters.
    // Returns false if memory allocation fails.
    virtual bool setSubjectDistance(uint32_t numerator, uint32_t denominator);

    // sets the fractions of seconds for the <DateTime> tag.
    // Returns false if memory allocation fails.
    virtual bool setSubsecTime(const std::string& subsec_time);

    // sets the white balance mode set when the image was shot.
    // Returns false if memory allocation fails.
    virtual bool setWhiteBalance(uint16_t white_balance);

    // sets the number of pixels per resolution unit in the image width.
    // Returns false if memory allocation fails.
    virtual bool setXResolution(uint32_t numerator, uint32_t denominator);

    // sets the position of chrominance components in relation to the luminance
    // component.
    // Returns false if memory allocation fails.
    virtual bool setYCbCrPositioning(uint16_t ycbcr_positioning);

    // sets the number of pixels per resolution unit in the image length.
    // Returns false if memory allocation fails.
    virtual bool setYResolution(uint32_t numerator, uint32_t denominator);

    // sets the manufacturer of camera.
    // Returns false if memory allocation fails.
    virtual bool setMake(const std::string& make);

    // sets the model number of camera.
    // Returns false if memory allocation fails.
    virtual bool setModel(const std::string& model);

    // Generates APP1 segment.
    // Returns false if generating APP1 segment fails.
    virtual bool generateApp1(const void* thumbnail_buffer, uint32_t size);

    // Gets buffer of APP1 segment. This method must be called only after calling
    // GenerateAPP1().
    virtual const uint8_t* getApp1Buffer();

    // Gets length of APP1 segment. This method must be called only after calling
    // GenerateAPP1().
    virtual unsigned int getApp1Length();

  protected:
    // sets the version of this standard supported.
    // Returns false if memory allocation fails.
    virtual bool setExifVersion(const std::string& exif_version);


    // Resets the pointers and memories.
    virtual void reset();

    // Adds a variable length tag to |exif_data_|. It will remove the original one
    // if the tag exists.
    // Returns the entry of the tag. The reference count of returned ExifEntry is
    // two.
    virtual std::unique_ptr<ExifEntry> addVariableLengthEntry(ExifIfd ifd,
                                                              ExifTag tag,
                                                              ExifFormat format,
                                                              uint64_t components,
                                                              unsigned int size);

    // Adds a entry of |tag| in |exif_data_|. It won't remove the original one if
    // the tag exists.
    // Returns the entry of the tag. It adds one reference count to returned
    // ExifEntry.
    virtual std::unique_ptr<ExifEntry> addEntry(ExifIfd ifd, ExifTag tag);

    // Helpe functions to add exif data with different types.
    virtual bool setShort(ExifIfd ifd,
                          ExifTag tag,
                          uint16_t value,
                          const std::string& msg);

    virtual bool setLong(ExifIfd ifd,
                         ExifTag tag,
                         uint32_t value,
                         const std::string& msg);

    virtual bool setRational(ExifIfd ifd,
                             ExifTag tag,
                             uint32_t numerator,
                             uint32_t denominator,
                             const std::string& msg);

    virtual bool setSRational(ExifIfd ifd,
                              ExifTag tag,
                              int32_t numerator,
                              int32_t denominator,
                              const std::string& msg);

    virtual bool setString(ExifIfd ifd,
                           ExifTag tag,
                           ExifFormat format,
                           const std::string& buffer,
                           const std::string& msg);

    // Destroys the buffer of APP1 segment if exists.
    virtual void destroyApp1();

    // The Exif data (APP1). Owned by this class.
    ExifData* exif_data_;
    // The raw data of APP1 segment. It's allocated by ExifMem in |exif_data_| but
    // owned by this class.
    uint8_t* app1_buffer_;
    // The length of |app1_buffer_|.
    unsigned int app1_length_;

};

#define SET_SHORT(ifd, tag, value)                      \
    do {                                                \
        if (setShort(ifd, tag, value, #tag) == false)   \
            return false;                               \
    } while (0);

#define SET_LONG(ifd, tag, value)                       \
    do {                                                \
        if (setLong(ifd, tag, value, #tag) == false)    \
            return false;                               \
    } while (0);

#define SET_RATIONAL(ifd, tag, numerator, denominator)                      \
    do {                                                                    \
        if (setRational(ifd, tag, numerator, denominator, #tag) == false)   \
            return false;                                                   \
    } while (0);

#define SET_SRATIONAL(ifd, tag, numerator, denominator)                       \
    do {                                                                      \
        if (setSRational(ifd, tag, numerator, denominator, #tag) == false)    \
            return false;                                                     \
    } while (0);

#define SET_STRING(ifd, tag, format, buffer)                                  \
    do {                                                                      \
        if (setString(ifd, tag, format, buffer, #tag) == false)               \
            return false;                                                     \
    } while (0);

// This comes from the Exif Version 2.2 standard table 6.
const char gExifAsciiPrefix[] = {0x41, 0x53, 0x43, 0x49, 0x49, 0x0, 0x0, 0x0};

static void setLatitudeOrLongitudeData(unsigned char* data, double num) {
    // Take the integer part of |num|.
    ExifLong degrees = static_cast<ExifLong>(num);
    ExifLong minutes = static_cast<ExifLong>(60 * (num - degrees));
    ExifLong microseconds =
            static_cast<ExifLong>(3600000000u * (num - degrees - minutes / 60.0));
    exif_set_rational(data, EXIF_BYTE_ORDER_INTEL, {degrees, 1});
    exif_set_rational(data + sizeof(ExifRational), EXIF_BYTE_ORDER_INTEL,
                                        {minutes, 1});
    exif_set_rational(data + 2 * sizeof(ExifRational), EXIF_BYTE_ORDER_INTEL,
                                        {microseconds, 1000000});
}

ExifUtils *ExifUtils::create() {
    return new ExifUtilsImpl();
}

ExifUtils::~ExifUtils() {
}

ExifUtilsImpl::ExifUtilsImpl()
        : exif_data_(nullptr), app1_buffer_(nullptr), app1_length_(0) {}

ExifUtilsImpl::~ExifUtilsImpl() {
    reset();
}


bool ExifUtilsImpl::initialize() {
    reset();
    exif_data_ = exif_data_new();
    if (exif_data_ == nullptr) {
        ALOGE("%s: allocate memory for exif_data_ failed", __FUNCTION__);
        return false;
    }
    // set the image options.
    exif_data_set_option(exif_data_, EXIF_DATA_OPTION_FOLLOW_SPECIFICATION);
    exif_data_set_data_type(exif_data_, EXIF_DATA_TYPE_COMPRESSED);
    exif_data_set_byte_order(exif_data_, EXIF_BYTE_ORDER_INTEL);

    // set exif version to 2.2.
    if (!setExifVersion("0220")) {
        return false;
    }

    return true;
}

bool ExifUtilsImpl::setAperture(uint32_t numerator, uint32_t denominator) {
    SET_RATIONAL(EXIF_IFD_EXIF, EXIF_TAG_APERTURE_VALUE, numerator, denominator);
    return true;
}

bool ExifUtilsImpl::setBrightness(int32_t numerator, int32_t denominator) {
    SET_SRATIONAL(EXIF_IFD_EXIF, EXIF_TAG_BRIGHTNESS_VALUE, numerator,
                                denominator);
    return true;
}

bool ExifUtilsImpl::setColorSpace(uint16_t color_space) {
    SET_SHORT(EXIF_IFD_EXIF, EXIF_TAG_COLOR_SPACE, color_space);
    return true;
}

bool ExifUtilsImpl::setComponentsConfiguration(
        const std::string& components_configuration) {
    SET_STRING(EXIF_IFD_EXIF, EXIF_TAG_COMPONENTS_CONFIGURATION,
                          EXIF_FORMAT_UNDEFINED, components_configuration);
    return true;
}

bool ExifUtilsImpl::setCompression(uint16_t compression) {
    SET_SHORT(EXIF_IFD_0, EXIF_TAG_COMPRESSION, compression);
    return true;
}

bool ExifUtilsImpl::setContrast(uint16_t contrast) {
    SET_SHORT(EXIF_IFD_EXIF, EXIF_TAG_CONTRAST, contrast);
    return true;
}

bool ExifUtilsImpl::setDateTime(const struct tm& t) {
    // The length is 20 bytes including NULL for termination in Exif standard.
    char str[20];
    int result = snprintf(str, sizeof(str), "%04i:%02i:%02i %02i:%02i:%02i",
                                                t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour,
                                                t.tm_min, t.tm_sec);
    if (result != sizeof(str) - 1) {
        ALOGW("%s: Input time is invalid", __FUNCTION__);
        return false;
    }
    std::string buffer(str);
    SET_STRING(EXIF_IFD_0, EXIF_TAG_DATE_TIME, EXIF_FORMAT_ASCII, buffer);
    SET_STRING(EXIF_IFD_EXIF, EXIF_TAG_DATE_TIME_ORIGINAL, EXIF_FORMAT_ASCII,
                          buffer);
    SET_STRING(EXIF_IFD_EXIF, EXIF_TAG_DATE_TIME_DIGITIZED, EXIF_FORMAT_ASCII,
                          buffer);
    return true;
}

bool ExifUtilsImpl::setDescription(const std::string& description) {
    SET_STRING(EXIF_IFD_0, EXIF_TAG_IMAGE_DESCRIPTION, EXIF_FORMAT_ASCII,
                          description);
    return true;
}

bool ExifUtilsImpl::setDigitalZoomRatio(uint32_t numerator, uint32_t denominator) {
    SET_RATIONAL(EXIF_IFD_EXIF, EXIF_TAG_DIGITAL_ZOOM_RATIO, numerator,
                              denominator);
    return true;
}

bool ExifUtilsImpl::setExposureBias(int32_t numerator, int32_t denominator) {
    SET_SRATIONAL(EXIF_IFD_EXIF, EXIF_TAG_EXPOSURE_BIAS_VALUE, numerator,
                                denominator);
    return true;
}

bool ExifUtilsImpl::setExposureMode(uint16_t exposure_mode) {
    SET_SHORT(EXIF_IFD_EXIF, EXIF_TAG_EXPOSURE_MODE, exposure_mode);
    return true;
}

bool ExifUtilsImpl::setExposureProgram(uint16_t exposure_program) {
    SET_SHORT(EXIF_IFD_EXIF, EXIF_TAG_EXPOSURE_PROGRAM, exposure_program);
    return true;
}

bool ExifUtilsImpl::setExposureTime(uint32_t numerator, uint32_t denominator) {
    SET_RATIONAL(EXIF_IFD_EXIF, EXIF_TAG_EXPOSURE_TIME, numerator, denominator);
    return true;
}

bool ExifUtilsImpl::setFlash(uint16_t flash) {
    SET_SHORT(EXIF_IFD_EXIF, EXIF_TAG_FLASH, flash);
    return true;
}

bool ExifUtilsImpl::setFNumber(uint32_t numerator, uint32_t denominator) {
    SET_RATIONAL(EXIF_IFD_EXIF, EXIF_TAG_FNUMBER, numerator, denominator);
    return true;
}

bool ExifUtilsImpl::setFocalLength(uint32_t numerator, uint32_t denominator) {
    SET_RATIONAL(EXIF_IFD_EXIF, EXIF_TAG_FOCAL_LENGTH, numerator, denominator);
    return true;
}

bool ExifUtilsImpl::setGainControl(uint16_t gain_control) {
    SET_SHORT(EXIF_IFD_EXIF, EXIF_TAG_GAIN_CONTROL, gain_control);
    return true;
}

bool ExifUtilsImpl::setGpsAltitude(double altitude) {
    ExifTag refTag = static_cast<ExifTag>(EXIF_TAG_GPS_ALTITUDE_REF);
    std::unique_ptr<ExifEntry> refEntry =
            addVariableLengthEntry(EXIF_IFD_GPS, refTag, EXIF_FORMAT_BYTE, 1, 1);
    if (!refEntry) {
        ALOGE("%s: Adding GPSAltitudeRef exif entry failed", __FUNCTION__);
        return false;
    }
    if (altitude >= 0) {
        *refEntry->data = 0;
    } else {
        *refEntry->data = 1;
        altitude *= -1;
    }

    ExifTag tag = static_cast<ExifTag>(EXIF_TAG_GPS_ALTITUDE);
    std::unique_ptr<ExifEntry> entry = addVariableLengthEntry(
            EXIF_IFD_GPS, tag, EXIF_FORMAT_RATIONAL, 1, sizeof(ExifRational));
    if (!entry) {
        exif_content_remove_entry(exif_data_->ifd[EXIF_IFD_GPS], refEntry.get());
        ALOGE("%s: Adding GPSAltitude exif entry failed", __FUNCTION__);
        return false;
    }
    exif_set_rational(entry->data, EXIF_BYTE_ORDER_INTEL,
                                        {static_cast<ExifLong>(altitude * 1000), 1000});

    return true;
}

bool ExifUtilsImpl::setGpsLatitude(double latitude) {
    const ExifTag refTag = static_cast<ExifTag>(EXIF_TAG_GPS_LATITUDE_REF);
    std::unique_ptr<ExifEntry> refEntry =
            addVariableLengthEntry(EXIF_IFD_GPS, refTag, EXIF_FORMAT_ASCII, 2, 2);
    if (!refEntry) {
        ALOGE("%s: Adding GPSLatitudeRef exif entry failed", __FUNCTION__);
        return false;
    }
    if (latitude >= 0) {
        memcpy(refEntry->data, "N", sizeof("N"));
    } else {
        memcpy(refEntry->data, "S", sizeof("S"));
        latitude *= -1;
    }

    const ExifTag tag = static_cast<ExifTag>(EXIF_TAG_GPS_LATITUDE);
    std::unique_ptr<ExifEntry> entry = addVariableLengthEntry(
            EXIF_IFD_GPS, tag, EXIF_FORMAT_RATIONAL, 3, 3 * sizeof(ExifRational));
    if (!entry) {
        exif_content_remove_entry(exif_data_->ifd[EXIF_IFD_GPS], refEntry.get());
        ALOGE("%s: Adding GPSLatitude exif entry failed", __FUNCTION__);
        return false;
    }
    setLatitudeOrLongitudeData(entry->data, latitude);

    return true;
}

bool ExifUtilsImpl::setGpsLongitude(double longitude) {
    ExifTag refTag = static_cast<ExifTag>(EXIF_TAG_GPS_LONGITUDE_REF);
    std::unique_ptr<ExifEntry> refEntry =
            addVariableLengthEntry(EXIF_IFD_GPS, refTag, EXIF_FORMAT_ASCII, 2, 2);
    if (!refEntry) {
        ALOGE("%s: Adding GPSLongitudeRef exif entry failed", __FUNCTION__);
        return false;
    }
    if (longitude >= 0) {
        memcpy(refEntry->data, "E", sizeof("E"));
    } else {
        memcpy(refEntry->data, "W", sizeof("W"));
        longitude *= -1;
    }

    ExifTag tag = static_cast<ExifTag>(EXIF_TAG_GPS_LONGITUDE);
    std::unique_ptr<ExifEntry> entry = addVariableLengthEntry(
            EXIF_IFD_GPS, tag, EXIF_FORMAT_RATIONAL, 3, 3 * sizeof(ExifRational));
    if (!entry) {
        exif_content_remove_entry(exif_data_->ifd[EXIF_IFD_GPS], refEntry.get());
        ALOGE("%s: Adding GPSLongitude exif entry failed", __FUNCTION__);
        return false;
    }
    setLatitudeOrLongitudeData(entry->data, longitude);

    return true;
}

bool ExifUtilsImpl::setGpsProcessingMethod(const std::string& method) {
    std::string buffer =
            std::string(gExifAsciiPrefix, sizeof(gExifAsciiPrefix)) + method;
    SET_STRING(EXIF_IFD_GPS, static_cast<ExifTag>(EXIF_TAG_GPS_PROCESSING_METHOD),
                          EXIF_FORMAT_UNDEFINED, buffer);
    return true;
}

bool ExifUtilsImpl::setGpsTimestamp(const struct tm& t) {
    const ExifTag dateTag = static_cast<ExifTag>(EXIF_TAG_GPS_DATE_STAMP);
    const size_t kGpsDateStampSize = 11;
    std::unique_ptr<ExifEntry> entry =
            addVariableLengthEntry(EXIF_IFD_GPS, dateTag, EXIF_FORMAT_ASCII,
                                                          kGpsDateStampSize, kGpsDateStampSize);
    if (!entry) {
        ALOGE("%s: Adding GPSDateStamp exif entry failed", __FUNCTION__);
        return false;
    }
    int result =
            snprintf(reinterpret_cast<char*>(entry->data), kGpsDateStampSize,
                              "%04i:%02i:%02i", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);
    if (result != kGpsDateStampSize - 1) {
        ALOGW("%s: Input time is invalid", __FUNCTION__);
        return false;
    }

    const ExifTag timeTag = static_cast<ExifTag>(EXIF_TAG_GPS_TIME_STAMP);
    entry = addVariableLengthEntry(EXIF_IFD_GPS, timeTag, EXIF_FORMAT_RATIONAL, 3,
                                                                  3 * sizeof(ExifRational));
    if (!entry) {
        ALOGE("%s: Adding GPSTimeStamp exif entry failed", __FUNCTION__);
        return false;
    }
    exif_set_rational(entry->data, EXIF_BYTE_ORDER_INTEL,
                                        {static_cast<ExifLong>(t.tm_hour), 1});
    exif_set_rational(entry->data + sizeof(ExifRational), EXIF_BYTE_ORDER_INTEL,
                                        {static_cast<ExifLong>(t.tm_min), 1});
    exif_set_rational(entry->data + 2 * sizeof(ExifRational),
                                        EXIF_BYTE_ORDER_INTEL,
                                        {static_cast<ExifLong>(t.tm_sec), 1});

    return true;
}

bool ExifUtilsImpl::setImageHeight(uint32_t length) {
    SET_SHORT(EXIF_IFD_0, EXIF_TAG_IMAGE_LENGTH, length);
    SET_LONG(EXIF_IFD_EXIF, EXIF_TAG_PIXEL_Y_DIMENSION, length);
    return true;
}

bool ExifUtilsImpl::setImageWidth(uint32_t width) {
    SET_SHORT(EXIF_IFD_0, EXIF_TAG_IMAGE_WIDTH, width);
    SET_LONG(EXIF_IFD_EXIF, EXIF_TAG_PIXEL_X_DIMENSION, width);
    return true;
}

bool ExifUtilsImpl::setIsoSpeedRating(uint16_t iso_speed_ratings) {
    SET_SHORT(EXIF_IFD_EXIF, EXIF_TAG_ISO_SPEED_RATINGS, iso_speed_ratings);
    return true;
}

bool ExifUtilsImpl::setLightSource(uint16_t light_source) {
    SET_SHORT(EXIF_IFD_EXIF, EXIF_TAG_LIGHT_SOURCE, light_source);
    return true;
}

bool ExifUtilsImpl::setMaxAperture(uint32_t numerator, uint32_t denominator) {
    SET_RATIONAL(EXIF_IFD_EXIF, EXIF_TAG_MAX_APERTURE_VALUE, numerator,
                              denominator);
    return true;
}

bool ExifUtilsImpl::setMeteringMode(uint16_t metering_mode) {
    SET_SHORT(EXIF_IFD_EXIF, EXIF_TAG_METERING_MODE, metering_mode);
    return true;
}

bool ExifUtilsImpl::setOrientation(uint16_t orientation) {
    /*
     * Orientation value:
     *  1      2      3      4      5          6          7          8
     *
     *  888888 888888     88 88     8888888888 88                 88 8888888888
     *  88         88     88 88     88  88     88  88         88  88     88  88
     *  8888     8888   8888 8888   88         8888888888 8888888888         88
     *  88         88     88 88
     *  88         88 888888 888888
     */
    int value = 1;
    switch (orientation) {
        case 90:
            value = 6;
            break;
        case 180:
            value = 3;
            break;
        case 270:
            value = 8;
            break;
        default:
            break;
    }
    SET_SHORT(EXIF_IFD_0, EXIF_TAG_ORIENTATION, value);
    return true;
}

bool ExifUtilsImpl::setResolutionUnit(uint16_t resolution_unit) {
    SET_SHORT(EXIF_IFD_EXIF, EXIF_TAG_RESOLUTION_UNIT, resolution_unit);
    return true;
}

bool ExifUtilsImpl::setSaturation(uint16_t saturation) {
    SET_SHORT(EXIF_IFD_EXIF, EXIF_TAG_SATURATION, saturation);
    return true;
}

bool ExifUtilsImpl::setSceneCaptureType(uint16_t type) {
    SET_SHORT(EXIF_IFD_EXIF, EXIF_TAG_SCENE_CAPTURE_TYPE, type);
    return true;
}

bool ExifUtilsImpl::setSharpness(uint16_t sharpness) {
    SET_SHORT(EXIF_IFD_EXIF, EXIF_TAG_SHARPNESS, sharpness);
    return true;
}

bool ExifUtilsImpl::setShutterSpeed(int32_t numerator, int32_t denominator) {
    SET_SRATIONAL(EXIF_IFD_EXIF, EXIF_TAG_SHUTTER_SPEED_VALUE, numerator,
                                denominator);
    return true;
}

bool ExifUtilsImpl::setSubjectDistance(uint32_t numerator, uint32_t denominator) {
    SET_RATIONAL(EXIF_IFD_EXIF, EXIF_TAG_SUBJECT_DISTANCE, numerator,
                              denominator);
    return true;
}

bool ExifUtilsImpl::setSubsecTime(const std::string& subsec_time) {
    SET_STRING(EXIF_IFD_EXIF, EXIF_TAG_SUB_SEC_TIME, EXIF_FORMAT_ASCII,
                          subsec_time);
    SET_STRING(EXIF_IFD_EXIF, EXIF_TAG_SUB_SEC_TIME_ORIGINAL, EXIF_FORMAT_ASCII,
                          subsec_time);
    SET_STRING(EXIF_IFD_EXIF, EXIF_TAG_SUB_SEC_TIME_DIGITIZED, EXIF_FORMAT_ASCII,
                          subsec_time);
    return true;
}

bool ExifUtilsImpl::setWhiteBalance(uint16_t white_balance) {
    SET_SHORT(EXIF_IFD_EXIF, EXIF_TAG_WHITE_BALANCE, white_balance);
    return true;
}

bool ExifUtilsImpl::setXResolution(uint32_t numerator, uint32_t denominator) {
    SET_RATIONAL(EXIF_IFD_EXIF, EXIF_TAG_X_RESOLUTION, numerator, denominator);
    return true;
}

bool ExifUtilsImpl::setYCbCrPositioning(uint16_t ycbcr_positioning) {
    SET_SHORT(EXIF_IFD_0, EXIF_TAG_YCBCR_POSITIONING, ycbcr_positioning);
    return true;
}

bool ExifUtilsImpl::setYResolution(uint32_t numerator, uint32_t denominator) {
    SET_RATIONAL(EXIF_IFD_EXIF, EXIF_TAG_Y_RESOLUTION, numerator, denominator);
    return true;
}

bool ExifUtilsImpl::generateApp1(const void* thumbnail_buffer, uint32_t size) {
    destroyApp1();
    exif_data_->data = const_cast<uint8_t*>(static_cast<const uint8_t*>(thumbnail_buffer));
    exif_data_->size = size;
    // Save the result into |app1_buffer_|.
    exif_data_save_data(exif_data_, &app1_buffer_, &app1_length_);
    if (!app1_length_) {
        ALOGE("%s: Allocate memory for app1_buffer_ failed", __FUNCTION__);
        return false;
    }
    /*
     * The JPEG segment size is 16 bits in spec. The size of APP1 segment should
     * be smaller than 65533 because there are two bytes for segment size field.
     */
    if (app1_length_ > 65533) {
        destroyApp1();
        ALOGE("%s: The size of APP1 segment is too large", __FUNCTION__);
        return false;
    }
    return true;
}

const uint8_t* ExifUtilsImpl::getApp1Buffer() {
    return app1_buffer_;
}

unsigned int ExifUtilsImpl::getApp1Length() {
    return app1_length_;
}

bool ExifUtilsImpl::setExifVersion(const std::string& exif_version) {
    SET_STRING(EXIF_IFD_EXIF, EXIF_TAG_EXIF_VERSION, EXIF_FORMAT_UNDEFINED, exif_version);
    return true;
}

bool ExifUtilsImpl::setMake(const std::string& make) {
    SET_STRING(EXIF_IFD_0, EXIF_TAG_MAKE, EXIF_FORMAT_ASCII, make);
    return true;
}

bool ExifUtilsImpl::setModel(const std::string& model) {
    SET_STRING(EXIF_IFD_0, EXIF_TAG_MODEL, EXIF_FORMAT_ASCII, model);
    return true;
}

void ExifUtilsImpl::reset() {
    destroyApp1();
    if (exif_data_) {
        /*
         * Since we decided to ignore the original APP1, we are sure that there is
         * no thumbnail allocated by libexif. |exif_data_->data| is actually
         * allocated by JpegCompressor. sets |exif_data_->data| to nullptr to
         * prevent exif_data_unref() destroy it incorrectly.
         */
        exif_data_->data = nullptr;
        exif_data_->size = 0;
        exif_data_unref(exif_data_);
        exif_data_ = nullptr;
    }
}

std::unique_ptr<ExifEntry> ExifUtilsImpl::addVariableLengthEntry(ExifIfd ifd,
                                                                 ExifTag tag,
                                                                 ExifFormat format,
                                                                 uint64_t components,
                                                                 unsigned int size) {
    // Remove old entry if exists.
    exif_content_remove_entry(exif_data_->ifd[ifd],
                              exif_content_get_entry(exif_data_->ifd[ifd], tag));
    ExifMem* mem = exif_mem_new_default();
    if (!mem) {
        ALOGE("%s: Allocate memory for exif entry failed", __FUNCTION__);
        return nullptr;
    }
    std::unique_ptr<ExifEntry> entry(exif_entry_new_mem(mem));
    if (!entry) {
        ALOGE("%s: Allocate memory for exif entry failed", __FUNCTION__);
        exif_mem_unref(mem);
        return nullptr;
    }
    void* tmpBuffer = exif_mem_alloc(mem, size);
    if (!tmpBuffer) {
        ALOGE("%s: Allocate memory for exif entry failed", __FUNCTION__);
        exif_mem_unref(mem);
        return nullptr;
    }

    entry->data = static_cast<unsigned char*>(tmpBuffer);
    entry->tag = tag;
    entry->format = format;
    entry->components = components;
    entry->size = size;

    exif_content_add_entry(exif_data_->ifd[ifd], entry.get());
    exif_mem_unref(mem);

    return entry;
}

std::unique_ptr<ExifEntry> ExifUtilsImpl::addEntry(ExifIfd ifd, ExifTag tag) {
    std::unique_ptr<ExifEntry> entry(exif_content_get_entry(exif_data_->ifd[ifd], tag));
    if (entry) {
        // exif_content_get_entry() won't ref the entry, so we ref here.
        exif_entry_ref(entry.get());
        return entry;
    }
    entry.reset(exif_entry_new());
    if (!entry) {
        ALOGE("%s: Allocate memory for exif entry failed", __FUNCTION__);
        return nullptr;
    }
    entry->tag = tag;
    exif_content_add_entry(exif_data_->ifd[ifd], entry.get());
    exif_entry_initialize(entry.get(), tag);
    return entry;
}

bool ExifUtilsImpl::setShort(ExifIfd ifd,
                             ExifTag tag,
                             uint16_t value,
                             const std::string& msg) {
    std::unique_ptr<ExifEntry> entry = addEntry(ifd, tag);
    if (!entry) {
        ALOGE("%s: Adding '%s' entry failed", __FUNCTION__, msg.c_str());
        return false;
    }
    exif_set_short(entry->data, EXIF_BYTE_ORDER_INTEL, value);
    return true;
}

bool ExifUtilsImpl::setLong(ExifIfd ifd,
                            ExifTag tag,
                            uint32_t value,
                            const std::string& msg) {
    std::unique_ptr<ExifEntry> entry = addEntry(ifd, tag);
    if (!entry) {
        ALOGE("%s: Adding '%s' entry failed", __FUNCTION__, msg.c_str());
        return false;
    }
    exif_set_long(entry->data, EXIF_BYTE_ORDER_INTEL, value);
    return true;
}

bool ExifUtilsImpl::setRational(ExifIfd ifd,
                                ExifTag tag,
                                uint32_t numerator,
                                uint32_t denominator,
                                const std::string& msg) {
    std::unique_ptr<ExifEntry> entry = addEntry(ifd, tag);
    if (!entry) {
        ALOGE("%s: Adding '%s' entry failed", __FUNCTION__, msg.c_str());
        return false;
    }
    exif_set_rational(entry->data, EXIF_BYTE_ORDER_INTEL,
                                        {numerator, denominator});
    return true;
}

bool ExifUtilsImpl::setSRational(ExifIfd ifd,
                                 ExifTag tag,
                                 int32_t numerator,
                                 int32_t denominator,
                                 const std::string& msg) {
    std::unique_ptr<ExifEntry> entry = addEntry(ifd, tag);
    if (!entry) {
        ALOGE("%s: Adding '%s' entry failed", __FUNCTION__, msg.c_str());
        return false;
    }
    exif_set_srational(entry->data, EXIF_BYTE_ORDER_INTEL,
                                          {numerator, denominator});
    return true;
}

bool ExifUtilsImpl::setString(ExifIfd ifd,
                              ExifTag tag,
                              ExifFormat format,
                              const std::string& buffer,
                              const std::string& msg) {
    size_t entry_size = buffer.length();
    // Since the exif format is undefined, NULL termination is not necessary.
    if (format == EXIF_FORMAT_ASCII) {
        entry_size++;
    }
    std::unique_ptr<ExifEntry> entry =
            addVariableLengthEntry(ifd, tag, format, entry_size, entry_size);
    if (!entry) {
        ALOGE("%s: Adding '%s' entry failed", __FUNCTION__, msg.c_str());
        return false;
    }
    memcpy(entry->data, buffer.c_str(), entry_size);
    return true;
}

void ExifUtilsImpl::destroyApp1() {
    /*
     * Since there is no API to access ExifMem in ExifData->priv, we use free
     * here, which is the default free function in libexif. See
     * exif_data_save_data() for detail.
     */
    free(app1_buffer_);
    app1_buffer_ = nullptr;
    app1_length_ = 0;
}

bool ExifUtilsImpl::setFromMetadata(const CameraMetadata& metadata,
                                    const size_t imageWidth,
                                    const size_t imageHeight) {
    // How precise the float-to-rational conversion for EXIF tags would be.
    constexpr int kRationalPrecision = 10000;
    if (!setImageWidth(imageWidth) ||
            !setImageHeight(imageHeight)) {
        ALOGE("%s: setting image resolution failed.", __FUNCTION__);
        return false;
    }

    struct timespec tp;
    struct tm time_info;
    bool time_available = clock_gettime(CLOCK_REALTIME, &tp) != -1;
    localtime_r(&tp.tv_sec, &time_info);
    if (!setDateTime(time_info)) {
        ALOGE("%s: setting data time failed.", __FUNCTION__);
        return false;
    }

    float focal_length;
    camera_metadata_ro_entry entry = metadata.find(ANDROID_LENS_FOCAL_LENGTH);
    if (entry.count) {
        focal_length = entry.data.f[0];

        if (!setFocalLength(
                        static_cast<uint32_t>(focal_length * kRationalPrecision),
                        kRationalPrecision)) {
            ALOGE("%s: setting focal length failed.", __FUNCTION__);
            return false;
        }
    } else {
        ALOGV("%s: Cannot find focal length in metadata.", __FUNCTION__);
    }

    if (metadata.exists(ANDROID_JPEG_GPS_COORDINATES)) {
        entry = metadata.find(ANDROID_JPEG_GPS_COORDINATES);
        if (entry.count < 3) {
            ALOGE("%s: Gps coordinates in metadata is not complete.", __FUNCTION__);
            return false;
        }
        if (!setGpsLatitude(entry.data.d[0])) {
            ALOGE("%s: setting gps latitude failed.", __FUNCTION__);
            return false;
        }
        if (!setGpsLongitude(entry.data.d[1])) {
            ALOGE("%s: setting gps longitude failed.", __FUNCTION__);
            return false;
        }
        if (!setGpsAltitude(entry.data.d[2])) {
            ALOGE("%s: setting gps altitude failed.", __FUNCTION__);
            return false;
        }
    }

    if (metadata.exists(ANDROID_JPEG_GPS_PROCESSING_METHOD)) {
        entry = metadata.find(ANDROID_JPEG_GPS_PROCESSING_METHOD);
        std::string method_str(reinterpret_cast<const char*>(entry.data.u8));
        if (!setGpsProcessingMethod(method_str)) {
            ALOGE("%s: setting gps processing method failed.", __FUNCTION__);
            return false;
        }
    }

    if (time_available && metadata.exists(ANDROID_JPEG_GPS_TIMESTAMP)) {
        entry = metadata.find(ANDROID_JPEG_GPS_TIMESTAMP);
        time_t timestamp = static_cast<time_t>(entry.data.i64[0]);
        if (gmtime_r(&timestamp, &time_info)) {
            if (!setGpsTimestamp(time_info)) {
                ALOGE("%s: setting gps timestamp failed.", __FUNCTION__);
                return false;
            }
        } else {
            ALOGE("%s: Time tranformation failed.", __FUNCTION__);
            return false;
        }
    }

    if (metadata.exists(ANDROID_JPEG_ORIENTATION)) {
        entry = metadata.find(ANDROID_JPEG_ORIENTATION);
        if (!setOrientation(entry.data.i32[0])) {
            ALOGE("%s: setting orientation failed.", __FUNCTION__);
            return false;
        }
    }

    if (metadata.exists(ANDROID_SENSOR_EXPOSURE_TIME)) {
        entry = metadata.find(ANDROID_SENSOR_EXPOSURE_TIME);
        // int64_t of nanoseconds
        if (!setExposureTime(entry.data.i64[0],1000000000u)) {
            ALOGE("%s: setting exposure time failed.", __FUNCTION__);
            return false;
        }
    }

    if (metadata.exists(ANDROID_LENS_APERTURE)) {
        const int kAperturePrecision = 10000;
        entry = metadata.find(ANDROID_LENS_APERTURE);
        if (!setFNumber(entry.data.f[0] * kAperturePrecision,
                                                      kAperturePrecision)) {
            ALOGE("%s: setting F number failed.", __FUNCTION__);
            return false;
        }
    }

    if (metadata.exists(ANDROID_FLASH_INFO_AVAILABLE)) {
        entry = metadata.find(ANDROID_FLASH_INFO_AVAILABLE);
        if (entry.data.u8[0] == ANDROID_FLASH_INFO_AVAILABLE_FALSE) {
            const uint32_t kNoFlashFunction = 0x20;
            if (!setFlash(kNoFlashFunction)) {
                ALOGE("%s: setting flash failed.", __FUNCTION__);
                return false;
            }
        } else {
            ALOGE("%s: Unsupported flash info: %d",__FUNCTION__, entry.data.u8[0]);
            return false;
        }
    }

    if (metadata.exists(ANDROID_CONTROL_AWB_MODE)) {
        entry = metadata.find(ANDROID_CONTROL_AWB_MODE);
        if (entry.data.u8[0] == ANDROID_CONTROL_AWB_MODE_AUTO) {
            const uint16_t kAutoWhiteBalance = 0;
            if (!setWhiteBalance(kAutoWhiteBalance)) {
                ALOGE("%s: setting white balance failed.", __FUNCTION__);
                return false;
            }
        } else {
            ALOGE("%s: Unsupported awb mode: %d", __FUNCTION__, entry.data.u8[0]);
            return false;
        }
    }

    if (time_available) {
        char str[4];
        if (snprintf(str, sizeof(str), "%03ld", tp.tv_nsec / 1000000) < 0) {
            ALOGE("%s: Subsec is invalid: %ld", __FUNCTION__, tp.tv_nsec);
            return false;
        }
        if (!setSubsecTime(std::string(str))) {
            ALOGE("%s: setting subsec time failed.", __FUNCTION__);
            return false;
        }
    }

    return true;
}

} // namespace helper
} // namespace V1_0
} // namespace common
} // namespace camera
} // namespace hardware
} // namespace android
