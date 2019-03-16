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

#ifndef ANDROID_HARDWARE_CAMERA_DEVICE_V3_4_EXTCAMUTIL_H
#define ANDROID_HARDWARE_CAMERA_DEVICE_V3_4_EXTCAMUTIL_H

#include <android/hardware/graphics/mapper/2.0/IMapper.h>
#include <inttypes.h>
#include <mutex>
#include <unordered_set>
#include <vector>
#include "tinyxml2.h"  // XML parsing
#include "utils/LightRefBase.h"

using android::hardware::graphics::mapper::V2_0::IMapper;
using android::hardware::graphics::mapper::V2_0::YCbCrLayout;

namespace android {
namespace hardware {
namespace camera {

namespace external {
namespace common {

struct Size {
    uint32_t width;
    uint32_t height;

    bool operator==(const Size& other) const {
        return (width == other.width && height == other.height);
    }
};

struct SizeHasher {
    size_t operator()(const Size& sz) const {
        size_t result = 1;
        result = 31 * result + sz.width;
        result = 31 * result + sz.height;
        return result;
    }
};

struct ExternalCameraConfig {
    static const char* kDefaultCfgPath;
    static ExternalCameraConfig loadFromCfg(const char* cfgPath = kDefaultCfgPath);

    // List of internal V4L2 video nodes external camera HAL must ignore.
    std::unordered_set<std::string> mInternalDevices;

    // Maximal size of a JPEG buffer, in bytes
    uint32_t maxJpegBufSize;

    // Maximum Size that can sustain 30fps streaming
    Size maxVideoSize;

    // Size of v4l2 buffer queue when streaming <= kMaxVideoSize
    uint32_t numVideoBuffers;

    // Size of v4l2 buffer queue when streaming > kMaxVideoSize
    uint32_t numStillBuffers;

    // Indication that the device connected supports depth output
    bool depthEnabled;

    struct FpsLimitation {
        Size size;
        double fpsUpperBound;
    };
    std::vector<FpsLimitation> fpsLimits;
    std::vector<FpsLimitation> depthFpsLimits;

    // Minimum output stream size
    Size minStreamSize;

    // The value of android.sensor.orientation
    int32_t orientation;

private:
    ExternalCameraConfig();
    static bool updateFpsList(tinyxml2::XMLElement* fpsList, std::vector<FpsLimitation>& fpsLimits);
};

} // common
} // external

namespace device {
namespace V3_4 {
namespace implementation {

struct SupportedV4L2Format {
    uint32_t width;
    uint32_t height;
    uint32_t fourcc;
    // All supported frame rate for this w/h/fourcc combination
    struct FrameRate {
        uint32_t durationNumerator;   // frame duration numerator.   Ex: 1
        uint32_t durationDenominator; // frame duration denominator. Ex: 30
        double getDouble() const;     // FrameRate in double.        Ex: 30.0
    };
    std::vector<FrameRate> frameRates;
};

// A class provide access to a dequeued V4L2 frame buffer (mostly in MJPG format)
// Also contains necessary information to enqueue the buffer back to V4L2 buffer queue
class V4L2Frame : public virtual VirtualLightRefBase {
public:
    V4L2Frame(uint32_t w, uint32_t h, uint32_t fourcc, int bufIdx, int fd,
              uint32_t dataSize, uint64_t offset);
    ~V4L2Frame() override;
    const uint32_t mWidth;
    const uint32_t mHeight;
    const uint32_t mFourcc;
    const int mBufferIndex; // for later enqueue
    int map(uint8_t** data, size_t* dataSize);
    int unmap();
private:
    std::mutex mLock;
    const int mFd; // used for mmap but doesn't claim ownership
    const size_t mDataSize;
    const uint64_t mOffset; // used for mmap
    uint8_t* mData = nullptr;
    bool  mMapped = false;
};

// A RAII class representing a CPU allocated YUV frame used as intermeidate buffers
// when generating output images.
class AllocatedFrame : public virtual VirtualLightRefBase {
public:
    AllocatedFrame(uint32_t w, uint32_t h); // TODO: use Size?
    ~AllocatedFrame() override;
    const uint32_t mWidth;
    const uint32_t mHeight;
    const uint32_t mFourcc; // Only support YU12 format for now
    int allocate(YCbCrLayout* out = nullptr);
    int getLayout(YCbCrLayout* out);
    int getCroppedLayout(const IMapper::Rect&, YCbCrLayout* out); // return non-zero for bad input
private:
    std::mutex mLock;
    std::vector<uint8_t> mData;
};

enum CroppingType {
    HORIZONTAL = 0,
    VERTICAL = 1
};

// Aspect ratio is defined as width/height here and ExternalCameraDevice
// will guarantee all supported sizes has width >= height (so aspect ratio >= 1.0)
#define ASPECT_RATIO(sz) (static_cast<float>((sz).width) / (sz).height)
const float kMaxAspectRatio = std::numeric_limits<float>::max();
const float kMinAspectRatio = 1.f;

bool isAspectRatioClose(float ar1, float ar2);

}  // namespace implementation
}  // namespace V3_4
}  // namespace device
}  // namespace camera
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_CAMERA_DEVICE_V3_4_EXTCAMUTIL_H
