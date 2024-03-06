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

#include "ConfigManager.h"

#include <android-base/parseint.h>
#include <hardware/gralloc.h>
#include <utils/SystemClock.h>

#include <fstream>
#include <sstream>
#include <string_view>
#include <thread>

namespace {

using ::aidl::android::hardware::automotive::evs::CameraParam;
using ::aidl::android::hardware::graphics::common::PixelFormat;
using ::tinyxml2::XMLAttribute;
using ::tinyxml2::XMLDocument;
using ::tinyxml2::XMLElement;

}  // namespace

std::string_view ConfigManager::sConfigDefaultPath =
        "/vendor/etc/automotive/evs/evs_mock_hal_configuration.xml";
std::string_view ConfigManager::sConfigOverridePath =
        "/vendor/etc/automotive/evs/evs_configuration_override.xml";

ConfigManager::CameraInfo::DeviceType ConfigManager::CameraInfo::deviceTypeFromSV(
        const std::string_view sv) {
    using namespace std::string_view_literals;
    static const std::unordered_map<std::string_view, DeviceType> nameToType = {
            {"mock"sv, DeviceType::MOCK},
            {"v4l2"sv, DeviceType::V4L2},
            {"video"sv, DeviceType::VIDEO},
    };
    const auto search = nameToType.find(sv);
    return search == nameToType.end() ? DeviceType::UNKNOWN : search->second;
}

void ConfigManager::printElementNames(const XMLElement* rootElem, const std::string& prefix) const {
    const XMLElement* curElem = rootElem;

    while (curElem != nullptr) {
        LOG(VERBOSE) << "[ELEM] " << prefix << curElem->Name();
        const XMLAttribute* curAttr = curElem->FirstAttribute();
        while (curAttr) {
            LOG(VERBOSE) << "[ATTR] " << prefix << curAttr->Name() << ": " << curAttr->Value();
            curAttr = curAttr->Next();
        }

        /* recursively go down to descendants */
        printElementNames(curElem->FirstChildElement(), prefix + "\t");

        curElem = curElem->NextSiblingElement();
    }
}

void ConfigManager::readCameraInfo(const XMLElement* const aCameraElem) {
    if (aCameraElem == nullptr) {
        LOG(WARNING) << "XML file does not have required camera element";
        return;
    }

    const XMLElement* curElem = aCameraElem->FirstChildElement();
    while (curElem != nullptr) {
        if (!strcmp(curElem->Name(), "group")) {
            /* camera group identifier */
            const char* id = curElem->FindAttribute("id")->Value();

            /* create a camera group to be filled */
            CameraGroupInfo* aCamera = new CameraGroupInfo();

            /* read camera device information */
            if (!readCameraDeviceInfo(aCamera, curElem)) {
                LOG(WARNING) << "Failed to read a camera information of " << id;
                delete aCamera;
                continue;
            }

            /* camera group synchronization */
            const char* sync = curElem->FindAttribute("synchronized")->Value();
            if (!strcmp(sync, "CALIBRATED")) {
                aCamera->synchronized = ANDROID_LOGICAL_MULTI_CAMERA_SENSOR_SYNC_TYPE_CALIBRATED;
            } else if (!strcmp(sync, "APPROXIMATE")) {
                aCamera->synchronized = ANDROID_LOGICAL_MULTI_CAMERA_SENSOR_SYNC_TYPE_APPROXIMATE;
            } else {
                aCamera->synchronized = 0;  // Not synchronized
            }

            /* add a group to hash map */
            mCameraGroups.insert_or_assign(id, std::unique_ptr<CameraGroupInfo>(aCamera));
        } else if (!std::strcmp(curElem->Name(), "device")) {
            /* camera unique identifier */
            const char* id = curElem->FindAttribute("id")->Value();

            /* camera mount location */
            const char* pos = curElem->FindAttribute("position")->Value();

            /* create a camera device to be filled */
            CameraInfo* aCamera = new CameraInfo();

            /* read camera device information */
            if (!readCameraDeviceInfo(aCamera, curElem)) {
                LOG(WARNING) << "Failed to read a camera information of " << id;
                delete aCamera;
                continue;
            }

            /* store read camera module information */
            mCameraInfo.insert_or_assign(id, std::unique_ptr<CameraInfo>(aCamera));

            /* assign a camera device to a position group */
            mCameraPosition[pos].insert(id);
        } else {
            /* ignore other device types */
            LOG(DEBUG) << "Unknown element " << curElem->Name() << " is ignored";
        }

        curElem = curElem->NextSiblingElement();
    }
}

bool ConfigManager::readCameraDeviceInfo(CameraInfo* aCamera, const XMLElement* aDeviceElem) {
    if (aCamera == nullptr || aDeviceElem == nullptr) {
        return false;
    }

    if (const auto typeAttr = aDeviceElem->FindAttribute("type")) {
        aCamera->deviceType = CameraInfo::deviceTypeFromSV(typeAttr->Value());
    }

    /* size information to allocate camera_metadata_t */
    size_t totalEntries = 0;
    size_t totalDataSize = 0;

    /* read device capabilities */
    totalEntries +=
            readCameraCapabilities(aDeviceElem->FirstChildElement("caps"), aCamera, totalDataSize);

    /* read camera metadata */
    totalEntries += readCameraMetadata(aDeviceElem->FirstChildElement("characteristics"), aCamera,
                                       totalDataSize);

    /* construct camera_metadata_t */
    if (!constructCameraMetadata(aCamera, totalEntries, totalDataSize)) {
        LOG(WARNING) << "Either failed to allocate memory or "
                     << "allocated memory was not large enough";
    }

    return true;
}

size_t ConfigManager::readCameraCapabilities(const XMLElement* const aCapElem, CameraInfo* aCamera,
                                             size_t& dataSize) {
    if (aCapElem == nullptr || aCamera == nullptr) {
        return 0;
    }

    std::string token;
    const XMLElement* curElem = nullptr;

    /* a list of supported camera parameters/controls */
    curElem = aCapElem->FirstChildElement("supported_controls");
    if (curElem != nullptr) {
        const XMLElement* ctrlElem = curElem->FirstChildElement("control");
        while (ctrlElem != nullptr) {
            const char* nameAttr = ctrlElem->FindAttribute("name")->Value();
            int32_t minVal = INT32_MIN, maxVal = INT32_MAX;
            if (!android::base::ParseInt(ctrlElem->FindAttribute("min")->Value(), &minVal)) {
                LOG(WARNING) << "Failed to parse " << ctrlElem->FindAttribute("min")->Value();
            }

            if (!android::base::ParseInt(ctrlElem->FindAttribute("max")->Value(), &maxVal)) {
                LOG(WARNING) << "Failed to parse " << ctrlElem->FindAttribute("max")->Value();
            }

            int32_t stepVal = 1;
            const XMLAttribute* stepAttr = ctrlElem->FindAttribute("step");
            if (stepAttr != nullptr) {
                if (!android::base::ParseInt(stepAttr->Value(), &stepVal)) {
                    LOG(WARNING) << "Failed to parse " << stepAttr->Value();
                }
            }

            CameraParam aParam;
            if (ConfigManagerUtil::convertToEvsCameraParam(nameAttr, aParam)) {
                aCamera->controls.insert_or_assign(
                        aParam, std::move(std::make_tuple(minVal, maxVal, stepVal)));
            }

            ctrlElem = ctrlElem->NextSiblingElement("control");
        }
    }

    /* a list of camera stream configurations */
    curElem = aCapElem->FirstChildElement("stream");
    while (curElem != nullptr) {
        /* read 5 attributes */
        const XMLAttribute* idAttr = curElem->FindAttribute("id");
        const XMLAttribute* widthAttr = curElem->FindAttribute("width");
        const XMLAttribute* heightAttr = curElem->FindAttribute("height");
        const XMLAttribute* fmtAttr = curElem->FindAttribute("format");
        const XMLAttribute* fpsAttr = curElem->FindAttribute("framerate");

        int32_t id = -1;
        int32_t framerate = 0;
        if (!android::base::ParseInt(idAttr->Value(), &id)) {
            LOG(WARNING) << "Failed to parse " << idAttr->Value();
        }
        if (fpsAttr != nullptr) {
            if (!android::base::ParseInt(fpsAttr->Value(), &framerate)) {
                LOG(WARNING) << "Failed to parse " << fpsAttr->Value();
            }
        }

        PixelFormat format = PixelFormat::UNSPECIFIED;
        if (ConfigManagerUtil::convertToPixelFormat(fmtAttr->Value(), format)) {
            StreamConfiguration cfg = {
                    .id = id,
                    .format = format,
                    .type = ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT,
                    .framerate = framerate,
            };

            if (!android::base::ParseInt(widthAttr->Value(), &cfg.width) ||
                !android::base::ParseInt(heightAttr->Value(), &cfg.height)) {
                LOG(WARNING) << "Failed to parse " << widthAttr->Value() << " and "
                             << heightAttr->Value();
            }
            aCamera->streamConfigurations.insert_or_assign(id, cfg);
        }

        curElem = curElem->NextSiblingElement("stream");
    }

    dataSize = calculate_camera_metadata_entry_data_size(
            get_camera_metadata_tag_type(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS),
            aCamera->streamConfigurations.size() * sizeof(StreamConfiguration));

    /* a single camera metadata entry contains multiple stream configurations */
    return dataSize > 0 ? 1 : 0;
}

size_t ConfigManager::readCameraMetadata(const XMLElement* const aParamElem, CameraInfo* aCamera,
                                         size_t& dataSize) {
    if (aParamElem == nullptr || aCamera == nullptr) {
        return 0;
    }

    const XMLElement* curElem = aParamElem->FirstChildElement("parameter");
    size_t numEntries = 0;
    camera_metadata_tag_t tag;
    while (curElem != nullptr) {
        if (ConfigManagerUtil::convertToMetadataTag(curElem->FindAttribute("name")->Value(), tag)) {
            switch (tag) {
                case ANDROID_LENS_DISTORTION:
                case ANDROID_LENS_POSE_ROTATION:
                case ANDROID_LENS_POSE_TRANSLATION:
                case ANDROID_LENS_INTRINSIC_CALIBRATION: {
                    /* float[] */
                    size_t count = 0;
                    void* data = ConfigManagerUtil::convertFloatArray(
                            curElem->FindAttribute("size")->Value(),
                            curElem->FindAttribute("value")->Value(), count);

                    aCamera->cameraMetadata.insert_or_assign(tag, std::make_pair(data, count));

                    ++numEntries;
                    dataSize += calculate_camera_metadata_entry_data_size(
                            get_camera_metadata_tag_type(tag), count);

                    break;
                }

                case ANDROID_REQUEST_AVAILABLE_CAPABILITIES: {
                    camera_metadata_enum_android_request_available_capabilities_t* data =
                            new camera_metadata_enum_android_request_available_capabilities_t[1];
                    if (ConfigManagerUtil::convertToCameraCapability(
                                curElem->FindAttribute("value")->Value(), *data)) {
                        aCamera->cameraMetadata.insert_or_assign(tag,
                                                                 std::make_pair((void*)data, 1));

                        ++numEntries;
                        dataSize += calculate_camera_metadata_entry_data_size(
                                get_camera_metadata_tag_type(tag), 1);
                    }
                    break;
                }

                case ANDROID_LOGICAL_MULTI_CAMERA_PHYSICAL_IDS: {
                    /* a comma-separated list of physical camera devices */
                    size_t len = strlen(curElem->FindAttribute("value")->Value());
                    char* data = new char[len + 1];
                    memcpy(data, curElem->FindAttribute("value")->Value(), len * sizeof(char));

                    /* replace commas with null char */
                    char* p = data;
                    while (*p != '\0') {
                        if (*p == ',') {
                            *p = '\0';
                        }
                        ++p;
                    }

                    aCamera->cameraMetadata.insert_or_assign(tag,
                                                             std::make_pair((void*)data, len + 1));

                    ++numEntries;
                    dataSize += calculate_camera_metadata_entry_data_size(
                            get_camera_metadata_tag_type(tag), len);
                    break;
                }

                /* TODO(b/140416878): add vendor-defined/custom tag support */
                default:
                    LOG(WARNING) << "Parameter " << curElem->FindAttribute("name")->Value()
                                 << " is not supported";
                    break;
            }
        } else {
            LOG(WARNING) << "Unsupported metadata tag " << curElem->FindAttribute("name")->Value()
                         << " is found.";
        }

        curElem = curElem->NextSiblingElement("parameter");
    }

    return numEntries;
}

bool ConfigManager::constructCameraMetadata(CameraInfo* aCamera, size_t totalEntries,
                                            size_t totalDataSize) {
    if (aCamera == nullptr || !aCamera->allocate(totalEntries, totalDataSize)) {
        LOG(ERROR) << "Failed to allocate memory for camera metadata";
        return false;
    }

    const size_t numStreamConfigs = aCamera->streamConfigurations.size();
    std::unique_ptr<int32_t[]> data(new int32_t[sizeof(StreamConfiguration) * numStreamConfigs]);
    int32_t* ptr = data.get();
    for (auto& cfg : aCamera->streamConfigurations) {
        memcpy(ptr, &cfg.second, sizeof(StreamConfiguration));
        ptr += sizeof(StreamConfiguration);
    }
    int32_t err = add_camera_metadata_entry(
            aCamera->characteristics, ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS, data.get(),
            numStreamConfigs * sizeof(StreamConfiguration));

    if (err) {
        LOG(ERROR) << "Failed to add stream configurations to metadata, ignored";
        return false;
    }

    bool success = true;
    for (auto& [tag, entry] : aCamera->cameraMetadata) {
        /* try to add new camera metadata entry */
        int32_t err =
                add_camera_metadata_entry(aCamera->characteristics, tag, entry.first, entry.second);
        if (err) {
            LOG(ERROR) << "Failed to add an entry with a tag, " << std::hex << tag;

            /* may exceed preallocated capacity */
            LOG(ERROR) << "Camera metadata has "
                       << get_camera_metadata_entry_count(aCamera->characteristics) << " / "
                       << get_camera_metadata_entry_capacity(aCamera->characteristics)
                       << " entries and "
                       << get_camera_metadata_data_count(aCamera->characteristics) << " / "
                       << get_camera_metadata_data_capacity(aCamera->characteristics)
                       << " bytes are filled.";
            LOG(ERROR) << "\tCurrent metadata entry requires "
                       << calculate_camera_metadata_entry_data_size(tag, entry.second) << " bytes.";

            success = false;
        }
    }

    LOG(VERBOSE) << "Camera metadata has "
                 << get_camera_metadata_entry_count(aCamera->characteristics) << " / "
                 << get_camera_metadata_entry_capacity(aCamera->characteristics) << " entries and "
                 << get_camera_metadata_data_count(aCamera->characteristics) << " / "
                 << get_camera_metadata_data_capacity(aCamera->characteristics)
                 << " bytes are filled.";
    return success;
}

void ConfigManager::readSystemInfo(const XMLElement* const aSysElem) {
    if (aSysElem == nullptr) {
        return;
    }

    /*
     * Please note that this function assumes that a given system XML element
     * and its child elements follow DTD.  If it does not, it will cause a
     * segmentation fault due to the failure of finding expected attributes.
     */

    /* read number of cameras available in the system */
    const XMLElement* xmlElem = aSysElem->FirstChildElement("num_cameras");
    if (xmlElem != nullptr) {
        if (!android::base::ParseInt(xmlElem->FindAttribute("value")->Value(),
                                     &mSystemInfo.numCameras)) {
            LOG(WARNING) << "Failed to parse " << xmlElem->FindAttribute("value")->Value();
        }
    }
}

void ConfigManager::readDisplayInfo(const XMLElement* const aDisplayElem) {
    if (aDisplayElem == nullptr) {
        LOG(WARNING) << "XML file does not have required camera element";
        return;
    }

    const XMLElement* curDev = aDisplayElem->FirstChildElement("device");
    while (curDev != nullptr) {
        const char* id = curDev->FindAttribute("id")->Value();
        std::unique_ptr<DisplayInfo> dpy(new DisplayInfo());
        if (dpy == nullptr) {
            LOG(ERROR) << "Failed to allocate memory for DisplayInfo";
            return;
        }

        const XMLElement* cap = curDev->FirstChildElement("caps");
        if (cap != nullptr) {
            const XMLElement* curStream = cap->FirstChildElement("stream");
            while (curStream != nullptr) {
                /* read 4 attributes */
                const XMLAttribute* idAttr = curStream->FindAttribute("id");
                const XMLAttribute* widthAttr = curStream->FindAttribute("width");
                const XMLAttribute* heightAttr = curStream->FindAttribute("height");
                const XMLAttribute* fmtAttr = curStream->FindAttribute("format");

                int32_t id = -1;
                if (!android::base::ParseInt(idAttr->Value(), &id)) {
                    LOG(WARNING) << "Failed to parse " << idAttr->Value();
                }
                PixelFormat format = PixelFormat::UNSPECIFIED;
                if (ConfigManagerUtil::convertToPixelFormat(fmtAttr->Value(), format)) {
                    StreamConfiguration cfg = {
                            .id = id,
                            .format = format,
                            .type = ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_INPUT,
                    };
                    if (!android::base::ParseInt(widthAttr->Value(), &cfg.width) ||
                        !android::base::ParseInt(heightAttr->Value(), &cfg.height)) {
                        LOG(WARNING) << "Failed to parse " << widthAttr->Value() << " and "
                                     << heightAttr->Value();
                    }
                    dpy->streamConfigurations.insert_or_assign(id, cfg);
                }

                curStream = curStream->NextSiblingElement("stream");
            }
        }

        mDisplayInfo.insert_or_assign(id, std::move(dpy));
        curDev = curDev->NextSiblingElement("device");
    }

    return;
}

bool ConfigManager::readConfigDataFromXML() noexcept {
    XMLDocument xmlDoc;

    const int64_t parsingStart = android::elapsedRealtimeNano();

    /* load and parse a configuration file */
    xmlDoc.LoadFile(sConfigOverridePath.data());
    if (xmlDoc.ErrorID() != tinyxml2::XML_SUCCESS) {
        xmlDoc.LoadFile(sConfigDefaultPath.data());
        if (xmlDoc.ErrorID() != tinyxml2::XML_SUCCESS) {
            LOG(ERROR) << "Failed to load and/or parse a configuration file, " << xmlDoc.ErrorStr();
            return false;
        }
    }

    /* retrieve the root element */
    const XMLElement* rootElem = xmlDoc.RootElement();
    if (std::strcmp(rootElem->Name(), "configuration") != 0) {
        LOG(ERROR) << "A configuration file is not in the required format.  "
                   << "See /etc/automotive/evs/evs_configuration.dtd";
        return false;
    }

    std::unique_lock<std::mutex> lock(mConfigLock);

    /*
     * parse camera information; this needs to be done before reading system
     * information
     */
    readCameraInfo(rootElem->FirstChildElement("camera"));

    /* parse system information */
    readSystemInfo(rootElem->FirstChildElement("system"));

    /* parse display information */
    readDisplayInfo(rootElem->FirstChildElement("display"));

    /* configuration data is ready to be consumed */
    mIsReady = true;

    /* notify that configuration data is ready */
    lock.unlock();
    mConfigCond.notify_all();

    const int64_t parsingEnd = android::elapsedRealtimeNano();
    LOG(INFO) << "Parsing configuration file takes " << std::scientific
              << (double)(parsingEnd - parsingStart) / 1000000.0 << " ms.";

    return true;
}

bool ConfigManager::readConfigDataFromBinary() {
    /* Temporary buffer to hold configuration data read from a binary file */
    char mBuffer[1024];

    std::fstream srcFile;
    const int64_t readStart = android::elapsedRealtimeNano();

    srcFile.open(mBinaryFilePath, std::fstream::in | std::fstream::binary);
    if (!srcFile) {
        LOG(ERROR) << "Failed to open a source binary file, " << mBinaryFilePath;
        return false;
    }

    std::unique_lock<std::mutex> lock(mConfigLock);
    mIsReady = false;

    /* read configuration data into the internal buffer */
    srcFile.read(mBuffer, sizeof(mBuffer));
    LOG(VERBOSE) << __FUNCTION__ << ": " << srcFile.gcount() << " bytes are read.";
    char* p = mBuffer;
    size_t sz = 0;

    /* read number of camera group information entries */
    const size_t ngrps = *(reinterpret_cast<size_t*>(p));
    p += sizeof(size_t);

    /* read each camera information entry */
    for (size_t cidx = 0; cidx < ngrps; ++cidx) {
        /* read camera identifier */
        std::string cameraId = *(reinterpret_cast<std::string*>(p));
        p += sizeof(std::string);

        /* size of camera_metadata_t */
        const size_t num_entry = *(reinterpret_cast<size_t*>(p));
        p += sizeof(size_t);
        const size_t num_data = *(reinterpret_cast<size_t*>(p));
        p += sizeof(size_t);

        /* create CameraInfo and add it to hash map */
        std::unique_ptr<ConfigManager::CameraGroupInfo> aCamera;
        if (aCamera == nullptr || !aCamera->allocate(num_entry, num_data)) {
            LOG(ERROR) << "Failed to create new CameraInfo object";
            mCameraInfo.clear();
            return false;
        }

        /* controls */
        typedef struct {
            CameraParam cid;
            int32_t min;
            int32_t max;
            int32_t step;
        } CameraCtrl;
        sz = *(reinterpret_cast<size_t*>(p));
        p += sizeof(size_t);
        CameraCtrl* ptr = reinterpret_cast<CameraCtrl*>(p);
        for (size_t idx = 0; idx < sz; ++idx) {
            CameraCtrl temp = *ptr++;
            aCamera->controls.insert_or_assign(
                    temp.cid, std::move(std::make_tuple(temp.min, temp.max, temp.step)));
        }
        p = reinterpret_cast<char*>(ptr);

        /* stream configurations */
        sz = *(reinterpret_cast<size_t*>(p));
        p += sizeof(size_t);
        int32_t* i32_ptr = reinterpret_cast<int32_t*>(p);
        for (size_t idx = 0; idx < sz; ++idx) {
            const int32_t id = *i32_ptr++;

            StreamConfiguration temp;
            memcpy(&temp, i32_ptr, sizeof(StreamConfiguration));
            i32_ptr += sizeof(StreamConfiguration);
            aCamera->streamConfigurations.insert_or_assign(id, temp);
        }
        p = reinterpret_cast<char*>(i32_ptr);

        /* synchronization */
        aCamera->synchronized = *(reinterpret_cast<int32_t*>(p));
        p += sizeof(int32_t);

        for (size_t idx = 0; idx < num_entry; ++idx) {
            /* Read camera metadata entries */
            camera_metadata_tag_t tag = *reinterpret_cast<camera_metadata_tag_t*>(p);
            p += sizeof(camera_metadata_tag_t);
            const size_t count = *reinterpret_cast<size_t*>(p);
            p += sizeof(size_t);

            const int32_t type = get_camera_metadata_tag_type(tag);
            switch (type) {
                case TYPE_BYTE: {
                    add_camera_metadata_entry(aCamera->characteristics, tag, p, count);
                    p += count * sizeof(uint8_t);
                    break;
                }
                case TYPE_INT32: {
                    add_camera_metadata_entry(aCamera->characteristics, tag, p, count);
                    p += count * sizeof(int32_t);
                    break;
                }
                case TYPE_FLOAT: {
                    add_camera_metadata_entry(aCamera->characteristics, tag, p, count);
                    p += count * sizeof(float);
                    break;
                }
                case TYPE_INT64: {
                    add_camera_metadata_entry(aCamera->characteristics, tag, p, count);
                    p += count * sizeof(int64_t);
                    break;
                }
                case TYPE_DOUBLE: {
                    add_camera_metadata_entry(aCamera->characteristics, tag, p, count);
                    p += count * sizeof(double);
                    break;
                }
                case TYPE_RATIONAL:
                    p += count * sizeof(camera_metadata_rational_t);
                    break;
                default:
                    LOG(WARNING) << "Type " << type << " is unknown; "
                                 << "data may be corrupted.";
                    break;
            }
        }

        mCameraInfo.insert_or_assign(cameraId, std::move(aCamera));
    }

    /* read number of camera information entries */
    const size_t ncams = *(reinterpret_cast<size_t*>(p));
    p += sizeof(size_t);

    /* read each camera information entry */
    for (size_t cidx = 0; cidx < ncams; ++cidx) {
        /* read camera identifier */
        std::string cameraId = *(reinterpret_cast<std::string*>(p));
        p += sizeof(std::string);

        /* size of camera_metadata_t */
        const size_t num_entry = *(reinterpret_cast<size_t*>(p));
        p += sizeof(size_t);
        const size_t num_data = *(reinterpret_cast<size_t*>(p));
        p += sizeof(size_t);

        /* create CameraInfo and add it to hash map */
        std::unique_ptr<ConfigManager::CameraInfo> aCamera;
        if (aCamera == nullptr || !aCamera->allocate(num_entry, num_data)) {
            LOG(ERROR) << "Failed to create new CameraInfo object";
            mCameraInfo.clear();
            return false;
        }

        /* controls */
        typedef struct {
            CameraParam cid;
            int32_t min;
            int32_t max;
            int32_t step;
        } CameraCtrl;
        sz = *(reinterpret_cast<size_t*>(p));
        p += sizeof(size_t);
        CameraCtrl* ptr = reinterpret_cast<CameraCtrl*>(p);
        for (size_t idx = 0; idx < sz; ++idx) {
            CameraCtrl temp = *ptr++;
            aCamera->controls.insert_or_assign(
                    temp.cid, std::move(std::make_tuple(temp.min, temp.max, temp.step)));
        }
        p = reinterpret_cast<char*>(ptr);

        /* stream configurations */
        sz = *(reinterpret_cast<size_t*>(p));
        p += sizeof(size_t);
        int32_t* i32_ptr = reinterpret_cast<int32_t*>(p);
        for (size_t idx = 0; idx < sz; ++idx) {
            const int32_t id = *i32_ptr++;

            StreamConfiguration temp;
            memcpy(&temp, i32_ptr, sizeof(StreamConfiguration));
            i32_ptr += sizeof(StreamConfiguration);
            aCamera->streamConfigurations.insert_or_assign(id, temp);
        }
        p = reinterpret_cast<char*>(i32_ptr);

        for (size_t idx = 0; idx < num_entry; ++idx) {
            /* Read camera metadata entries */
            camera_metadata_tag_t tag = *reinterpret_cast<camera_metadata_tag_t*>(p);
            p += sizeof(camera_metadata_tag_t);
            const size_t count = *reinterpret_cast<size_t*>(p);
            p += sizeof(size_t);

            const int32_t type = get_camera_metadata_tag_type(tag);
            switch (type) {
                case TYPE_BYTE: {
                    add_camera_metadata_entry(aCamera->characteristics, tag, p, count);
                    p += count * sizeof(uint8_t);
                    break;
                }
                case TYPE_INT32: {
                    add_camera_metadata_entry(aCamera->characteristics, tag, p, count);
                    p += count * sizeof(int32_t);
                    break;
                }
                case TYPE_FLOAT: {
                    add_camera_metadata_entry(aCamera->characteristics, tag, p, count);
                    p += count * sizeof(float);
                    break;
                }
                case TYPE_INT64: {
                    add_camera_metadata_entry(aCamera->characteristics, tag, p, count);
                    p += count * sizeof(int64_t);
                    break;
                }
                case TYPE_DOUBLE: {
                    add_camera_metadata_entry(aCamera->characteristics, tag, p, count);
                    p += count * sizeof(double);
                    break;
                }
                case TYPE_RATIONAL:
                    p += count * sizeof(camera_metadata_rational_t);
                    break;
                default:
                    LOG(WARNING) << "Type " << type << " is unknown; "
                                 << "data may be corrupted.";
                    break;
            }
        }

        mCameraInfo.insert_or_assign(cameraId, std::move(aCamera));
    }

    mIsReady = true;

    /* notify that configuration data is ready */
    lock.unlock();
    mConfigCond.notify_all();

    int64_t readEnd = android::elapsedRealtimeNano();
    LOG(INFO) << __FUNCTION__ << " takes " << std::scientific
              << (double)(readEnd - readStart) / 1000000.0 << " ms.";

    return true;
}

bool ConfigManager::writeConfigDataToBinary() {
    std::fstream outFile;

    const int64_t writeStart = android::elapsedRealtimeNano();

    outFile.open(mBinaryFilePath, std::fstream::out | std::fstream::binary);
    if (!outFile) {
        LOG(ERROR) << "Failed to open a destination binary file, " << mBinaryFilePath;
        return false;
    }

    /* lock a configuration data while it's being written to the filesystem */
    std::lock_guard<std::mutex> lock(mConfigLock);

    /* write camera group information */
    size_t sz = mCameraGroups.size();
    outFile.write(reinterpret_cast<const char*>(&sz), sizeof(size_t));
    for (auto&& [camId, camInfo] : mCameraGroups) {
        LOG(INFO) << "Storing camera group " << camId;

        /* write a camera identifier string */
        outFile.write(reinterpret_cast<const char*>(&camId), sizeof(std::string));

        /* controls */
        sz = camInfo->controls.size();
        outFile.write(reinterpret_cast<const char*>(&sz), sizeof(size_t));
        for (auto&& [ctrl, range] : camInfo->controls) {
            outFile.write(reinterpret_cast<const char*>(&ctrl), sizeof(CameraParam));
            outFile.write(reinterpret_cast<const char*>(&std::get<0>(range)), sizeof(int32_t));
            outFile.write(reinterpret_cast<const char*>(&std::get<1>(range)), sizeof(int32_t));
            outFile.write(reinterpret_cast<const char*>(&std::get<2>(range)), sizeof(int32_t));
        }

        /* stream configurations */
        sz = camInfo->streamConfigurations.size();
        outFile.write(reinterpret_cast<const char*>(&sz), sizeof(size_t));
        for (auto&& [sid, cfg] : camInfo->streamConfigurations) {
            outFile.write(reinterpret_cast<const char*>(sid), sizeof(int32_t));
            outFile.write(reinterpret_cast<const char*>(&cfg), sizeof(cfg));
        }

        /* synchronization */
        outFile.write(reinterpret_cast<const char*>(&camInfo->synchronized), sizeof(int32_t));

        /* size of camera_metadata_t */
        size_t num_entry = 0;
        size_t num_data = 0;
        if (camInfo->characteristics != nullptr) {
            num_entry = get_camera_metadata_entry_count(camInfo->characteristics);
            num_data = get_camera_metadata_data_count(camInfo->characteristics);
        }
        outFile.write(reinterpret_cast<const char*>(&num_entry), sizeof(size_t));
        outFile.write(reinterpret_cast<const char*>(&num_data), sizeof(size_t));

        /* write each camera metadata entry */
        if (num_entry > 0) {
            camera_metadata_entry_t entry;
            for (size_t idx = 0; idx < num_entry; ++idx) {
                if (get_camera_metadata_entry(camInfo->characteristics, idx, &entry)) {
                    LOG(ERROR) << "Failed to retrieve camera metadata entry " << idx;
                    outFile.close();
                    return false;
                }

                outFile.write(reinterpret_cast<const char*>(&entry.tag), sizeof(entry.tag));
                outFile.write(reinterpret_cast<const char*>(&entry.count), sizeof(entry.count));

                int32_t type = get_camera_metadata_tag_type(entry.tag);
                switch (type) {
                    case TYPE_BYTE:
                        outFile.write(reinterpret_cast<const char*>(entry.data.u8),
                                      sizeof(uint8_t) * entry.count);
                        break;
                    case TYPE_INT32:
                        outFile.write(reinterpret_cast<const char*>(entry.data.i32),
                                      sizeof(int32_t) * entry.count);
                        break;
                    case TYPE_FLOAT:
                        outFile.write(reinterpret_cast<const char*>(entry.data.f),
                                      sizeof(float) * entry.count);
                        break;
                    case TYPE_INT64:
                        outFile.write(reinterpret_cast<const char*>(entry.data.i64),
                                      sizeof(int64_t) * entry.count);
                        break;
                    case TYPE_DOUBLE:
                        outFile.write(reinterpret_cast<const char*>(entry.data.d),
                                      sizeof(double) * entry.count);
                        break;
                    case TYPE_RATIONAL:
                        [[fallthrough]];
                    default:
                        LOG(WARNING) << "Type " << type << " is not supported.";
                        break;
                }
            }
        }
    }

    /* write camera device information */
    sz = mCameraInfo.size();
    outFile.write(reinterpret_cast<const char*>(&sz), sizeof(size_t));
    for (auto&& [camId, camInfo] : mCameraInfo) {
        LOG(INFO) << "Storing camera " << camId;

        /* write a camera identifier string */
        outFile.write(reinterpret_cast<const char*>(&camId), sizeof(std::string));

        /* controls */
        sz = camInfo->controls.size();
        outFile.write(reinterpret_cast<const char*>(&sz), sizeof(size_t));
        for (auto& [ctrl, range] : camInfo->controls) {
            outFile.write(reinterpret_cast<const char*>(&ctrl), sizeof(CameraParam));
            outFile.write(reinterpret_cast<const char*>(&std::get<0>(range)), sizeof(int32_t));
            outFile.write(reinterpret_cast<const char*>(&std::get<1>(range)), sizeof(int32_t));
            outFile.write(reinterpret_cast<const char*>(&std::get<2>(range)), sizeof(int32_t));
        }

        /* stream configurations */
        sz = camInfo->streamConfigurations.size();
        outFile.write(reinterpret_cast<const char*>(&sz), sizeof(size_t));
        for (auto&& [sid, cfg] : camInfo->streamConfigurations) {
            outFile.write(reinterpret_cast<const char*>(sid), sizeof(int32_t));
            outFile.write(reinterpret_cast<const char*>(&cfg), sizeof(cfg));
        }

        /* size of camera_metadata_t */
        size_t num_entry = 0;
        size_t num_data = 0;
        if (camInfo->characteristics != nullptr) {
            num_entry = get_camera_metadata_entry_count(camInfo->characteristics);
            num_data = get_camera_metadata_data_count(camInfo->characteristics);
        }
        outFile.write(reinterpret_cast<const char*>(&num_entry), sizeof(size_t));
        outFile.write(reinterpret_cast<const char*>(&num_data), sizeof(size_t));

        /* write each camera metadata entry */
        if (num_entry > 0) {
            camera_metadata_entry_t entry;
            for (size_t idx = 0; idx < num_entry; ++idx) {
                if (get_camera_metadata_entry(camInfo->characteristics, idx, &entry)) {
                    LOG(ERROR) << "Failed to retrieve camera metadata entry " << idx;
                    outFile.close();
                    return false;
                }

                outFile.write(reinterpret_cast<const char*>(&entry.tag), sizeof(entry.tag));
                outFile.write(reinterpret_cast<const char*>(&entry.count), sizeof(entry.count));

                int32_t type = get_camera_metadata_tag_type(entry.tag);
                switch (type) {
                    case TYPE_BYTE:
                        outFile.write(reinterpret_cast<const char*>(entry.data.u8),
                                      sizeof(uint8_t) * entry.count);
                        break;
                    case TYPE_INT32:
                        outFile.write(reinterpret_cast<const char*>(entry.data.i32),
                                      sizeof(int32_t) * entry.count);
                        break;
                    case TYPE_FLOAT:
                        outFile.write(reinterpret_cast<const char*>(entry.data.f),
                                      sizeof(float) * entry.count);
                        break;
                    case TYPE_INT64:
                        outFile.write(reinterpret_cast<const char*>(entry.data.i64),
                                      sizeof(int64_t) * entry.count);
                        break;
                    case TYPE_DOUBLE:
                        outFile.write(reinterpret_cast<const char*>(entry.data.d),
                                      sizeof(double) * entry.count);
                        break;
                    case TYPE_RATIONAL:
                        [[fallthrough]];
                    default:
                        LOG(WARNING) << "Type " << type << " is not supported.";
                        break;
                }
            }
        }
    }

    outFile.close();
    int64_t writeEnd = android::elapsedRealtimeNano();
    LOG(INFO) << __FUNCTION__ << " takes " << std::scientific
              << (double)(writeEnd - writeStart) / 1000000.0 << " ms.";

    return true;
}

std::unique_ptr<ConfigManager> ConfigManager::Create() {
    std::unique_ptr<ConfigManager> cfgMgr(new ConfigManager());

    /*
     * Read a configuration from XML file
     *
     * If this is too slow, ConfigManager::readConfigDataFromBinary() and
     * ConfigManager::writeConfigDataToBinary()can serialize CameraInfo object
     * to the filesystem and construct CameraInfo instead; this was
     * evaluated as 10x faster.
     */
    if (!cfgMgr->readConfigDataFromXML()) {
        return nullptr;
    } else {
        return cfgMgr;
    }
}

ConfigManager::CameraInfo::~CameraInfo() {
    free_camera_metadata(characteristics);

    for (auto&& [tag, val] : cameraMetadata) {
        switch (tag) {
            case ANDROID_LENS_DISTORTION:
            case ANDROID_LENS_POSE_ROTATION:
            case ANDROID_LENS_POSE_TRANSLATION:
            case ANDROID_LENS_INTRINSIC_CALIBRATION: {
                delete[] reinterpret_cast<float*>(val.first);
                break;
            }

            case ANDROID_REQUEST_AVAILABLE_CAPABILITIES: {
                delete[] reinterpret_cast<
                        camera_metadata_enum_android_request_available_capabilities_t*>(val.first);
                break;
            }

            case ANDROID_LOGICAL_MULTI_CAMERA_PHYSICAL_IDS: {
                delete[] reinterpret_cast<char*>(val.first);
                break;
            }

            default:
                LOG(WARNING) << "Tag " << std::hex << tag << " is not supported.  "
                             << "Data may be corrupted?";
                break;
        }
    }
}
