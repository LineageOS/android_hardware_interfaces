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

#include <sstream>
#include <fstream>
#include <thread>

#include <hardware/gralloc.h>
#include <utils/SystemClock.h>
#include <android/hardware/camera/device/3.2/ICameraDevice.h>

#include "ConfigManager.h"

using ::android::hardware::camera::device::V3_2::StreamRotation;


ConfigManager::~ConfigManager() {
    /* Nothing to do */
}


void ConfigManager::readCameraInfo(const XMLElement * const aCameraElem) {
    if (aCameraElem == nullptr) {
        ALOGW("XML file does not have required camera element");
        return;
    }

    const XMLElement *curElem = aCameraElem->FirstChildElement();
    while (curElem != nullptr) {
        if (!strcmp(curElem->Name(), "group")) {
            /* camera group identifier */
            const char *id = curElem->FindAttribute("id")->Value();

            /* create a camera group to be filled */
            CameraGroupInfo *aCamera = new CameraGroupInfo();

            /* read camera device information */
            if (!readCameraDeviceInfo(aCamera, curElem)) {
                ALOGW("Failed to read a camera information of %s", id);
                delete aCamera;
                continue;
            }

            /* camera group synchronization */
            const char *sync = curElem->FindAttribute("synchronized")->Value();
            if (!strcmp(sync, "CALIBRATED")) {
                aCamera->synchronized =
                    ANDROID_LOGICAL_MULTI_CAMERA_SENSOR_SYNC_TYPE_CALIBRATED;
            } else if (!strcmp(sync, "APPROXIMATE")) {
                aCamera->synchronized =
                    ANDROID_LOGICAL_MULTI_CAMERA_SENSOR_SYNC_TYPE_APPROXIMATE;
            } else {
                aCamera->synchronized = 0; // Not synchronized
            }

            /* add a group to hash map */
            mCameraGroupInfos.insert_or_assign(id, unique_ptr<CameraGroupInfo>(aCamera));
        } else if (!strcmp(curElem->Name(), "device")) {
            /* camera unique identifier */
            const char *id = curElem->FindAttribute("id")->Value();

            /* camera mount location */
            const char *pos = curElem->FindAttribute("position")->Value();

            /* create a camera device to be filled */
            CameraInfo *aCamera = new CameraInfo();

            /* read camera device information */
            if (!readCameraDeviceInfo(aCamera, curElem)) {
                ALOGW("Failed to read a camera information of %s", id);
                delete aCamera;
                continue;
            }

            /* store read camera module information */
            mCameraInfo.insert_or_assign(id, unique_ptr<CameraInfo>(aCamera));

            /* assign a camera device to a position group */
            mCameraPosition[pos].emplace(id);
        } else {
            /* ignore other device types */
            ALOGD("Unknown element %s is ignored", curElem->Name());
        }

        curElem = curElem->NextSiblingElement();
    }
}


bool
ConfigManager::readCameraDeviceInfo(CameraInfo *aCamera,
                                    const XMLElement *aDeviceElem) {
    if (aCamera == nullptr || aDeviceElem == nullptr) {
        return false;
    }

    /* size information to allocate camera_metadata_t */
    size_t totalEntries = 0;
    size_t totalDataSize = 0;

    /* read device capabilities */
    totalEntries +=
        readCameraCapabilities(aDeviceElem->FirstChildElement("caps"),
                               aCamera,
                               totalDataSize);


    /* read camera metadata */
    totalEntries +=
        readCameraMetadata(aDeviceElem->FirstChildElement("characteristics"),
                           aCamera,
                           totalDataSize);

    /* construct camera_metadata_t */
    if (!constructCameraMetadata(aCamera, totalEntries, totalDataSize)) {
        ALOGW("Either failed to allocate memory or "
              "allocated memory was not large enough");
    }

    return true;
}


size_t
ConfigManager::readCameraCapabilities(const XMLElement * const aCapElem,
                                      CameraInfo *aCamera,
                                      size_t &dataSize) {
    if (aCapElem == nullptr || aCamera == nullptr) {
        return 0;
    }

    string token;
    const XMLElement *curElem = nullptr;

    /* a list of supported camera parameters/controls */
    curElem = aCapElem->FirstChildElement("supported_controls");
    if (curElem != nullptr) {
        const XMLElement *ctrlElem = curElem->FirstChildElement("control");
        while (ctrlElem != nullptr) {
            const char *nameAttr = ctrlElem->FindAttribute("name")->Value();;
            const int32_t minVal = stoi(ctrlElem->FindAttribute("min")->Value());
            const int32_t maxVal = stoi(ctrlElem->FindAttribute("max")->Value());

            int32_t stepVal = 1;
            const XMLAttribute *stepAttr = ctrlElem->FindAttribute("step");
            if (stepAttr != nullptr) {
                stepVal = stoi(stepAttr->Value());
            }

            CameraParam aParam;
            if (ConfigManagerUtil::convertToEvsCameraParam(nameAttr,
                                                           aParam)) {
                aCamera->controls.emplace(
                    aParam,
                    make_tuple(minVal, maxVal, stepVal)
                );
            }

            ctrlElem = ctrlElem->NextSiblingElement("control");
        }
    }

    /* a list of camera stream configurations */
    curElem = aCapElem->FirstChildElement("stream");
    while (curElem != nullptr) {
        /* read 5 attributes */
        const XMLAttribute *idAttr     = curElem->FindAttribute("id");
        const XMLAttribute *widthAttr  = curElem->FindAttribute("width");
        const XMLAttribute *heightAttr = curElem->FindAttribute("height");
        const XMLAttribute *fmtAttr    = curElem->FindAttribute("format");
        const XMLAttribute *fpsAttr    = curElem->FindAttribute("framerate");

        const int32_t id = stoi(idAttr->Value());
        int32_t framerate = 0;
        if (fpsAttr != nullptr) {
            framerate = stoi(fpsAttr->Value());
        }

        int32_t pixFormat;
        if (ConfigManagerUtil::convertToPixelFormat(fmtAttr->Value(),
                                                    pixFormat)) {
            RawStreamConfiguration cfg = {
                id,
                stoi(widthAttr->Value()),
                stoi(heightAttr->Value()),
                pixFormat,
                ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT,
                framerate
            };
            aCamera->streamConfigurations.insert_or_assign(id, cfg);
        }

        curElem = curElem->NextSiblingElement("stream");
    }

    dataSize = calculate_camera_metadata_entry_data_size(
                   get_camera_metadata_tag_type(
                       ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS
                   ),
                   aCamera->streamConfigurations.size() * kStreamCfgSz
               );

    /* a single camera metadata entry contains multiple stream configurations */
    return dataSize > 0 ? 1 : 0;
}


size_t
ConfigManager::readCameraMetadata(const XMLElement * const aParamElem,
                                  CameraInfo *aCamera,
                                  size_t &dataSize) {
    if (aParamElem == nullptr || aCamera == nullptr) {
        return 0;
    }

    const XMLElement *curElem = aParamElem->FirstChildElement("parameter");
    size_t numEntries = 0;
    camera_metadata_tag_t tag;
    while (curElem != nullptr) {
        if (!ConfigManagerUtil::convertToMetadataTag(curElem->FindAttribute("name")->Value(),
                                                     tag)) {
            switch(tag) {
                case ANDROID_LENS_DISTORTION:
                case ANDROID_LENS_POSE_ROTATION:
                case ANDROID_LENS_POSE_TRANSLATION:
                case ANDROID_LENS_INTRINSIC_CALIBRATION: {
                    /* float[] */
                    size_t count = 0;
                    void   *data = ConfigManagerUtil::convertFloatArray(
                                        curElem->FindAttribute("size")->Value(),
                                        curElem->FindAttribute("value")->Value(),
                                        count
                                   );

                    aCamera->cameraMetadata.insert_or_assign(
                        tag, make_pair(make_unique<void *>(data), count)
                    );

                    ++numEntries;
                    dataSize += calculate_camera_metadata_entry_data_size(
                                    get_camera_metadata_tag_type(tag), count
                                );

                    break;
                }

                case ANDROID_REQUEST_AVAILABLE_CAPABILITIES: {
                    camera_metadata_enum_android_request_available_capabilities_t *data =
                        new camera_metadata_enum_android_request_available_capabilities_t[1];
                    if (ConfigManagerUtil::convertToCameraCapability(
                            curElem->FindAttribute("value")->Value(), *data)) {
                                        curElem->FindAttribute("value")->Value(),
                        aCamera->cameraMetadata.insert_or_assign(
                            tag, make_pair(make_unique<void *>(data), 1)
                        );

                        ++numEntries;
                        dataSize += calculate_camera_metadata_entry_data_size(
                                        get_camera_metadata_tag_type(tag), 1
                                    );
                    }
                    break;
                }

                case ANDROID_LOGICAL_MULTI_CAMERA_PHYSICAL_IDS: {
                    /* a comma-separated list of physical camera devices */
                    size_t len = strlen(curElem->FindAttribute("value")->Value());
                    char *data = new char[len + 1];
                    memcpy(data,
                           curElem->FindAttribute("value")->Value(),
                           len * sizeof(char));

                    /* replace commas with null char */
                    char *p = data;
                    while (*p != '\0') {
                        if (*p == ',') {
                            *p = '\0';
                        }
                        ++p;
                    }

                    aCamera->cameraMetadata.insert_or_assign(
                        tag, make_pair(make_unique<void *>(data), len)
                    );

                    ++numEntries;
                    dataSize += calculate_camera_metadata_entry_data_size(
                                    get_camera_metadata_tag_type(tag), len
                                );
                    break;
                }

                default:
                    ALOGW("Parameter %s is not supported",
                          curElem->FindAttribute("name")->Value());
                    break;
            }
        }

        curElem = curElem->NextSiblingElement("parameter");
    }

    return numEntries;
}


bool
ConfigManager::constructCameraMetadata(CameraInfo *aCamera,
                                       const size_t totalEntries,
                                       const size_t totalDataSize) {
    if (aCamera == nullptr || !aCamera->allocate(totalEntries, totalDataSize)) {
        ALOGE("Failed to allocate memory for camera metadata");
        return false;
    }

    const size_t numStreamConfigs = aCamera->streamConfigurations.size();
    unique_ptr<int32_t[]> data(new int32_t[kStreamCfgSz * numStreamConfigs]);
    int32_t *ptr = data.get();
    for (auto &cfg : aCamera->streamConfigurations) {
        for (auto i = 0; i < kStreamCfgSz; ++i) {
          *ptr++ = cfg.second[i];
        }
    }
    int32_t err = add_camera_metadata_entry(aCamera->characteristics,
                                            ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS,
                                            data.get(),
                                            numStreamConfigs * kStreamCfgSz);

    if (err) {
        ALOGE("Failed to add stream configurations to metadata, ignored");
        return false;
    }

    bool success = true;
    for (auto &[tag, entry] : aCamera->cameraMetadata) {
        /* try to add new camera metadata entry */
        int32_t err = add_camera_metadata_entry(aCamera->characteristics,
                                                tag,
                                                entry.first.get(),
                                                entry.second);
        if (err) {
            ALOGE("Failed to add an entry with a tag 0x%X", tag);

            /* may exceed preallocated capacity */
            ALOGE("Camera metadata has %ld / %ld entries and %ld / %ld bytes are filled",
                  (long)get_camera_metadata_entry_count(aCamera->characteristics),
                  (long)get_camera_metadata_entry_capacity(aCamera->characteristics),
                  (long)get_camera_metadata_data_count(aCamera->characteristics),
                  (long)get_camera_metadata_data_capacity(aCamera->characteristics));
            ALOGE("\tCurrent metadata entry requires %ld bytes",
                  (long)calculate_camera_metadata_entry_data_size(tag, entry.second));

            success = false;
        }
    }

    ALOGV("Camera metadata has %ld / %ld entries and %ld / %ld bytes are filled",
          (long)get_camera_metadata_entry_count(aCamera->characteristics),
          (long)get_camera_metadata_entry_capacity(aCamera->characteristics),
          (long)get_camera_metadata_data_count(aCamera->characteristics),
          (long)get_camera_metadata_data_capacity(aCamera->characteristics));

    return success;
}


void ConfigManager::readSystemInfo(const XMLElement * const aSysElem) {
    if (aSysElem == nullptr) {
        return;
    }

    /*
     * Please note that this function assumes that a given system XML element
     * and its child elements follow DTD.  If it does not, it will cause a
     * segmentation fault due to the failure of finding expected attributes.
     */

    /* read number of cameras available in the system */
    const XMLElement *xmlElem = aSysElem->FirstChildElement("num_cameras");
    if (xmlElem != nullptr) {
        mSystemInfo.numCameras =
            stoi(xmlElem->FindAttribute("value")->Value());
    }
}


void ConfigManager::readDisplayInfo(const XMLElement * const aDisplayElem) {
    if (aDisplayElem == nullptr) {
        ALOGW("XML file does not have required camera element");
        return;
    }

    const XMLElement *curDev = aDisplayElem->FirstChildElement("device");
    while (curDev != nullptr) {
        const char *id = curDev->FindAttribute("id")->Value();
        //const char *pos = curDev->FirstAttribute("position")->Value();

        unique_ptr<DisplayInfo> dpy(new DisplayInfo());
        if (dpy == nullptr) {
            ALOGE("Failed to allocate memory for DisplayInfo");
            return;
        }

        const XMLElement *cap = curDev->FirstChildElement("caps");
        if (cap != nullptr) {
            const XMLElement *curStream = cap->FirstChildElement("stream");
            while (curStream != nullptr) {
                /* read 4 attributes */
                const XMLAttribute *idAttr     = curStream->FindAttribute("id");
                const XMLAttribute *widthAttr  = curStream->FindAttribute("width");
                const XMLAttribute *heightAttr = curStream->FindAttribute("height");
                const XMLAttribute *fmtAttr    = curStream->FindAttribute("format");

                const int32_t id = stoi(idAttr->Value());
                int32_t pixFormat;
                if (ConfigManagerUtil::convertToPixelFormat(fmtAttr->Value(),
                                                            pixFormat)) {
                    RawStreamConfiguration cfg = {
                        id,
                        stoi(widthAttr->Value()),
                        stoi(heightAttr->Value()),
                        pixFormat,
                        ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_INPUT,
                        0   // unused
                    };
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
    xmlDoc.LoadFile(mConfigFilePath);
    if (xmlDoc.ErrorID() != XML_SUCCESS) {
        ALOGE("Failed to load and/or parse a configuration file, %s", xmlDoc.ErrorStr());
        return false;
    }

    /* retrieve the root element */
    const XMLElement *rootElem = xmlDoc.RootElement();
    if (strcmp(rootElem->Name(), "configuration")) {
        ALOGE("A configuration file is not in the required format.  "
              "See /etc/automotive/evs/evs_configuration.dtd");
        return false;
    }

    /*
     * parse camera information; this needs to be done before reading system
     * information
     */
    readCameraInfo(rootElem->FirstChildElement("camera"));

    /* parse system information */
    readSystemInfo(rootElem->FirstChildElement("system"));

    /* parse display information */
    readDisplayInfo(rootElem->FirstChildElement("display"));

    const int64_t parsingEnd = android::elapsedRealtimeNano();
    ALOGI("Parsing configuration file takes %lf (ms)",
          (double)(parsingEnd - parsingStart) / 1000000.0);


    return true;
}


std::unique_ptr<ConfigManager> ConfigManager::Create(const char *path) {
    unique_ptr<ConfigManager> cfgMgr(new ConfigManager(path));

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

