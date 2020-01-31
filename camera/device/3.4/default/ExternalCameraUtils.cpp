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
#define LOG_TAG "ExtCamUtils@3.4"
//#define LOG_NDEBUG 0
#include <log/log.h>

#include <cmath>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include "ExternalCameraUtils.h"

namespace android {
namespace hardware {
namespace camera {
namespace device {
namespace V3_4 {
namespace implementation {

V4L2Frame::V4L2Frame(
        uint32_t w, uint32_t h, uint32_t fourcc,
        int bufIdx, int fd, uint32_t dataSize, uint64_t offset) :
        mWidth(w), mHeight(h), mFourcc(fourcc),
        mBufferIndex(bufIdx), mFd(fd), mDataSize(dataSize), mOffset(offset) {}

int V4L2Frame::map(uint8_t** data, size_t* dataSize) {
    if (data == nullptr || dataSize == nullptr) {
        ALOGI("%s: V4L2 buffer map bad argument: data %p, dataSize %p",
                __FUNCTION__, data, dataSize);
        return -EINVAL;
    }

    std::lock_guard<std::mutex> lk(mLock);
    if (!mMapped) {
        void* addr = mmap(NULL, mDataSize, PROT_READ, MAP_SHARED, mFd, mOffset);
        if (addr == MAP_FAILED) {
            ALOGE("%s: V4L2 buffer map failed: %s", __FUNCTION__, strerror(errno));
            return -EINVAL;
        }
        mData = static_cast<uint8_t*>(addr);
        mMapped = true;
    }
    *data = mData;
    *dataSize = mDataSize;
    ALOGV("%s: V4L map FD %d, data %p size %zu", __FUNCTION__, mFd, mData, mDataSize);
    return 0;
}

int V4L2Frame::unmap() {
    std::lock_guard<std::mutex> lk(mLock);
    if (mMapped) {
        ALOGV("%s: V4L unmap data %p size %zu", __FUNCTION__, mData, mDataSize);
        if (munmap(mData, mDataSize) != 0) {
            ALOGE("%s: V4L2 buffer unmap failed: %s", __FUNCTION__, strerror(errno));
            return -EINVAL;
        }
        mMapped = false;
    }
    return 0;
}

V4L2Frame::~V4L2Frame() {
    unmap();
}

AllocatedFrame::AllocatedFrame(
        uint32_t w, uint32_t h) :
        mWidth(w), mHeight(h), mFourcc(V4L2_PIX_FMT_YUV420) {};

AllocatedFrame::~AllocatedFrame() {}

int AllocatedFrame::allocate(YCbCrLayout* out) {
    std::lock_guard<std::mutex> lk(mLock);
    if ((mWidth % 2) || (mHeight % 2)) {
        ALOGE("%s: bad dimension %dx%d (not multiple of 2)", __FUNCTION__, mWidth, mHeight);
        return -EINVAL;
    }

    uint32_t dataSize = mWidth * mHeight * 3 / 2; // YUV420
    if (mData.size() != dataSize) {
        mData.resize(dataSize);
    }

    if (out != nullptr) {
        out->y = mData.data();
        out->yStride = mWidth;
        uint8_t* cbStart = mData.data() + mWidth * mHeight;
        uint8_t* crStart = cbStart + mWidth * mHeight / 4;
        out->cb = cbStart;
        out->cr = crStart;
        out->cStride = mWidth / 2;
        out->chromaStep = 1;
    }
    return 0;
}

int AllocatedFrame::getLayout(YCbCrLayout* out) {
    IMapper::Rect noCrop = {0, 0,
            static_cast<int32_t>(mWidth),
            static_cast<int32_t>(mHeight)};
    return getCroppedLayout(noCrop, out);
}

int AllocatedFrame::getCroppedLayout(const IMapper::Rect& rect, YCbCrLayout* out) {
    if (out == nullptr) {
        ALOGE("%s: null out", __FUNCTION__);
        return -1;
    }

    std::lock_guard<std::mutex> lk(mLock);
    if ((rect.left + rect.width) > static_cast<int>(mWidth) ||
        (rect.top + rect.height) > static_cast<int>(mHeight) ||
            (rect.left % 2) || (rect.top % 2) || (rect.width % 2) || (rect.height % 2)) {
        ALOGE("%s: bad rect left %d top %d w %d h %d", __FUNCTION__,
                rect.left, rect.top, rect.width, rect.height);
        return -1;
    }

    out->y = mData.data() + mWidth * rect.top + rect.left;
    out->yStride = mWidth;
    uint8_t* cbStart = mData.data() + mWidth * mHeight;
    uint8_t* crStart = cbStart + mWidth * mHeight / 4;
    out->cb = cbStart + mWidth * rect.top / 4 + rect.left / 2;
    out->cr = crStart + mWidth * rect.top / 4 + rect.left / 2;
    out->cStride = mWidth / 2;
    out->chromaStep = 1;
    return 0;
}

bool isAspectRatioClose(float ar1, float ar2) {
    const float kAspectRatioMatchThres = 0.025f; // This threshold is good enough to distinguish
                                                // 4:3/16:9/20:9
                                                // 1.33 / 1.78 / 2
    return (std::abs(ar1 - ar2) < kAspectRatioMatchThres);
}

double SupportedV4L2Format::FrameRate::getDouble() const {
    return durationDenominator / static_cast<double>(durationNumerator);
}

}  // namespace implementation
}  // namespace V3_4
}  // namespace device


namespace external {
namespace common {

namespace {
    const int kDefaultJpegBufSize = 5 << 20; // 5MB
    const int kDefaultNumVideoBuffer = 4;
    const int kDefaultNumStillBuffer = 2;
    const int kDefaultOrientation = 0; // suitable for natural landscape displays like tablet/TV
                                       // For phone devices 270 is better
} // anonymous namespace

const char* ExternalCameraConfig::kDefaultCfgPath = "/vendor/etc/external_camera_config.xml";

ExternalCameraConfig ExternalCameraConfig::loadFromCfg(const char* cfgPath) {
    using namespace tinyxml2;
    ExternalCameraConfig ret;

    XMLDocument configXml;
    XMLError err = configXml.LoadFile(cfgPath);
    if (err != XML_SUCCESS) {
        ALOGE("%s: Unable to load external camera config file '%s'. Error: %s",
                __FUNCTION__, cfgPath, XMLDocument::ErrorIDToName(err));
        return ret;
    } else {
        ALOGI("%s: load external camera config succeed!", __FUNCTION__);
    }

    XMLElement *extCam = configXml.FirstChildElement("ExternalCamera");
    if (extCam == nullptr) {
        ALOGI("%s: no external camera config specified", __FUNCTION__);
        return ret;
    }

    XMLElement *providerCfg = extCam->FirstChildElement("Provider");
    if (providerCfg == nullptr) {
        ALOGI("%s: no external camera provider config specified", __FUNCTION__);
        return ret;
    }

    XMLElement *ignore = providerCfg->FirstChildElement("ignore");
    if (ignore == nullptr) {
        ALOGI("%s: no internal ignored device specified", __FUNCTION__);
        return ret;
    }

    XMLElement *id = ignore->FirstChildElement("id");
    while (id != nullptr) {
        const char* text = id->GetText();
        if (text != nullptr) {
            ret.mInternalDevices.insert(text);
            ALOGI("%s: device %s will be ignored by external camera provider",
                    __FUNCTION__, text);
        }
        id = id->NextSiblingElement("id");
    }

    XMLElement *deviceCfg = extCam->FirstChildElement("Device");
    if (deviceCfg == nullptr) {
        ALOGI("%s: no external camera device config specified", __FUNCTION__);
        return ret;
    }

    XMLElement *jpegBufSz = deviceCfg->FirstChildElement("MaxJpegBufferSize");
    if (jpegBufSz == nullptr) {
        ALOGI("%s: no max jpeg buffer size specified", __FUNCTION__);
    } else {
        ret.maxJpegBufSize = jpegBufSz->UnsignedAttribute("bytes", /*Default*/kDefaultJpegBufSize);
    }

    XMLElement *numVideoBuf = deviceCfg->FirstChildElement("NumVideoBuffers");
    if (numVideoBuf == nullptr) {
        ALOGI("%s: no num video buffers specified", __FUNCTION__);
    } else {
        ret.numVideoBuffers =
                numVideoBuf->UnsignedAttribute("count", /*Default*/kDefaultNumVideoBuffer);
    }

    XMLElement *numStillBuf = deviceCfg->FirstChildElement("NumStillBuffers");
    if (numStillBuf == nullptr) {
        ALOGI("%s: no num still buffers specified", __FUNCTION__);
    } else {
        ret.numStillBuffers =
                numStillBuf->UnsignedAttribute("count", /*Default*/kDefaultNumStillBuffer);
    }

    XMLElement *fpsList = deviceCfg->FirstChildElement("FpsList");
    if (fpsList == nullptr) {
        ALOGI("%s: no fps list specified", __FUNCTION__);
    } else {
        if (!updateFpsList(fpsList, ret.fpsLimits)) {
            return ret;
        }
    }

    XMLElement *depth = deviceCfg->FirstChildElement("Depth16Supported");
    if (depth == nullptr) {
        ret.depthEnabled = false;
        ALOGI("%s: depth output is not enabled", __FUNCTION__);
    } else {
        ret.depthEnabled = depth->BoolAttribute("enabled", false);
    }

    if(ret.depthEnabled) {
        XMLElement *depthFpsList = deviceCfg->FirstChildElement("DepthFpsList");
        if (depthFpsList == nullptr) {
            ALOGW("%s: no depth fps list specified", __FUNCTION__);
        } else {
            if(!updateFpsList(depthFpsList, ret.depthFpsLimits)) {
                return ret;
            }
        }
    }

    XMLElement *minStreamSize = deviceCfg->FirstChildElement("MinimumStreamSize");
    if (minStreamSize == nullptr) {
       ALOGI("%s: no minimum stream size specified", __FUNCTION__);
    } else {
        ret.minStreamSize = {
                minStreamSize->UnsignedAttribute("width", /*Default*/0),
                minStreamSize->UnsignedAttribute("height", /*Default*/0)};
    }

    XMLElement *orientation = deviceCfg->FirstChildElement("Orientation");
    if (orientation == nullptr) {
        ALOGI("%s: no sensor orientation specified", __FUNCTION__);
    } else {
        ret.orientation = orientation->IntAttribute("degree", /*Default*/kDefaultOrientation);
    }

    ALOGI("%s: external camera cfg loaded: maxJpgBufSize %d,"
            " num video buffers %d, num still buffers %d, orientation %d",
            __FUNCTION__, ret.maxJpegBufSize,
            ret.numVideoBuffers, ret.numStillBuffers, ret.orientation);
    for (const auto& limit : ret.fpsLimits) {
        ALOGI("%s: fpsLimitList: %dx%d@%f", __FUNCTION__,
                limit.size.width, limit.size.height, limit.fpsUpperBound);
    }
    for (const auto& limit : ret.depthFpsLimits) {
        ALOGI("%s: depthFpsLimitList: %dx%d@%f", __FUNCTION__, limit.size.width, limit.size.height,
              limit.fpsUpperBound);
    }
    ALOGI("%s: minStreamSize: %dx%d" , __FUNCTION__,
         ret.minStreamSize.width, ret.minStreamSize.height);
    return ret;
}

bool ExternalCameraConfig::updateFpsList(tinyxml2::XMLElement* fpsList,
        std::vector<FpsLimitation>& fpsLimits) {
    using namespace tinyxml2;
    std::vector<FpsLimitation> limits;
    XMLElement* row = fpsList->FirstChildElement("Limit");
    while (row != nullptr) {
        FpsLimitation prevLimit{{0, 0}, 1000.0};
        FpsLimitation limit;
        limit.size = {row->UnsignedAttribute("width", /*Default*/ 0),
                      row->UnsignedAttribute("height", /*Default*/ 0)};
        limit.fpsUpperBound = row->DoubleAttribute("fpsBound", /*Default*/ 1000.0);
        if (limit.size.width <= prevLimit.size.width ||
            limit.size.height <= prevLimit.size.height ||
            limit.fpsUpperBound >= prevLimit.fpsUpperBound) {
            ALOGE(
                "%s: FPS limit list must have increasing size and decreasing fps!"
                " Prev %dx%d@%f, Current %dx%d@%f",
                __FUNCTION__, prevLimit.size.width, prevLimit.size.height, prevLimit.fpsUpperBound,
                limit.size.width, limit.size.height, limit.fpsUpperBound);
            return false;
        }
        limits.push_back(limit);
        row = row->NextSiblingElement("Limit");
    }
    fpsLimits = limits;
    return true;
}

ExternalCameraConfig::ExternalCameraConfig() :
        maxJpegBufSize(kDefaultJpegBufSize),
        numVideoBuffers(kDefaultNumVideoBuffer),
        numStillBuffers(kDefaultNumStillBuffer),
        depthEnabled(false),
        orientation(kDefaultOrientation) {
    fpsLimits.push_back({/*Size*/{ 640,  480}, /*FPS upper bound*/30.0});
    fpsLimits.push_back({/*Size*/{1280,  720}, /*FPS upper bound*/7.5});
    fpsLimits.push_back({/*Size*/{1920, 1080}, /*FPS upper bound*/5.0});
    minStreamSize = {0, 0};
}


}  // namespace common
}  // namespace external
}  // namespace camera
}  // namespace hardware
}  // namespace android
