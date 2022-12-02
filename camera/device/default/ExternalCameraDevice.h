/*
 * Copyright (C) 2022 The Android Open Source Project
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

#ifndef HARDWARE_INTERFACES_CAMERA_DEVICE_DEFAULT_EXTERNALCAMERADEVICE_H_
#define HARDWARE_INTERFACES_CAMERA_DEVICE_DEFAULT_EXTERNALCAMERADEVICE_H_

#include <ExternalCameraDeviceSession.h>
#include <ExternalCameraUtils.h>
#include <aidl/android/hardware/camera/device/BnCameraDevice.h>

namespace android {
namespace hardware {
namespace camera {
namespace device {
namespace implementation {

using ::aidl::android::hardware::camera::common::CameraResourceCost;
using ::aidl::android::hardware::camera::device::BnCameraDevice;
using ::aidl::android::hardware::camera::device::CameraMetadata;
using ::aidl::android::hardware::camera::device::ICameraDeviceCallback;
using ::aidl::android::hardware::camera::device::ICameraDeviceSession;
using ::aidl::android::hardware::camera::device::ICameraInjectionSession;
using ::aidl::android::hardware::camera::device::StreamConfiguration;
using ::android::hardware::camera::external::common::ExternalCameraConfig;

class ExternalCameraDevice : public BnCameraDevice {
  public:
    // Called by external camera provider HAL.
    // Provider HAL must ensure the uniqueness of CameraDevice object per cameraId, or there could
    // be multiple CameraDevice trying to access the same physical camera.  Also, provider will have
    // to keep track of all CameraDevice objects in order to notify CameraDevice when the underlying
    // camera is detached.
    ExternalCameraDevice(const std::string& devicePath, const ExternalCameraConfig& config);
    ~ExternalCameraDevice() override;

    ndk::ScopedAStatus getCameraCharacteristics(CameraMetadata* _aidl_return) override;
    ndk::ScopedAStatus getPhysicalCameraCharacteristics(const std::string& in_physicalCameraId,
                                                        CameraMetadata* _aidl_return) override;
    ndk::ScopedAStatus getResourceCost(CameraResourceCost* _aidl_return) override;
    ndk::ScopedAStatus isStreamCombinationSupported(const StreamConfiguration& in_streams,
                                                    bool* _aidl_return) override;
    ndk::ScopedAStatus open(const std::shared_ptr<ICameraDeviceCallback>& in_callback,
                            std::shared_ptr<ICameraDeviceSession>* _aidl_return) override;
    ndk::ScopedAStatus openInjectionSession(
            const std::shared_ptr<ICameraDeviceCallback>& in_callback,
            std::shared_ptr<ICameraInjectionSession>* _aidl_return) override;
    ndk::ScopedAStatus setTorchMode(bool in_on) override;
    ndk::ScopedAStatus turnOnTorchWithStrengthLevel(int32_t in_torchStrength) override;
    ndk::ScopedAStatus getTorchStrengthLevel(int32_t* _aidl_return) override;

    binder_status_t dump(int fd, const char** args, uint32_t numArgs) override;

    // Caller must use this method to check if CameraDevice ctor failed
    bool isInitFailed();

    // Device version to be used by the external camera provider.
    // Should be of the form <major>.<minor>
    static std::string kDeviceVersion;

  private:
    virtual std::shared_ptr<ExternalCameraDeviceSession> createSession(
            const std::shared_ptr<ICameraDeviceCallback>&, const ExternalCameraConfig& cfg,
            const std::vector<SupportedV4L2Format>& sortedFormats, const CroppingType& croppingType,
            const common::V1_0::helper::CameraMetadata& chars, const std::string& cameraId,
            unique_fd v4l2Fd);

    bool isInitFailedLocked();

    // Init supported w/h/format/fps in mSupportedFormats. Caller still owns fd
    void initSupportedFormatsLocked(int fd);

    // Calls into virtual member function. Do not use it in constructor
    status_t initCameraCharacteristics();
    // Init available capabilities keys
    virtual status_t initAvailableCapabilities(
            ::android::hardware::camera::common::V1_0::helper::CameraMetadata*);
    // Init non-device dependent keys
    virtual status_t initDefaultCharsKeys(
            ::android::hardware::camera::common::V1_0::helper::CameraMetadata*);
    // Init camera control chars keys. Caller still owns fd
    status_t initCameraControlsCharsKeys(
            int fd, ::android::hardware::camera::common::V1_0::helper::CameraMetadata*);
    // Init camera output configuration related keys.  Caller still owns fd
    status_t initOutputCharsKeys(
            int fd, ::android::hardware::camera::common::V1_0::helper::CameraMetadata*);

    // Helper function for initOutputCharskeys
    template <size_t SIZE>
    status_t initOutputCharsKeysByFormat(
            ::android::hardware::camera::common::V1_0::helper::CameraMetadata* metadata,
            uint32_t fourcc, const std::array<int, SIZE>& halFormats, int streamConfigTag,
            int streamConfiguration, int minFrameDuration, int stallDuration);

    status_t calculateMinFps(::android::hardware::camera::common::V1_0::helper::CameraMetadata*);

    static void getFrameRateList(int fd, double fpsUpperBound, SupportedV4L2Format* format);

    static void updateFpsBounds(int fd, CroppingType cropType,
                                const std::vector<ExternalCameraConfig::FpsLimitation>& fpsLimits,
                                SupportedV4L2Format format,
                                std::vector<SupportedV4L2Format>& outFmts);

    // Get candidate supported formats list of input cropping type.
    static std::vector<SupportedV4L2Format> getCandidateSupportedFormatsLocked(
            int fd, CroppingType cropType,
            const std::vector<ExternalCameraConfig::FpsLimitation>& fpsLimits,
            const std::vector<ExternalCameraConfig::FpsLimitation>& depthFpsLimits,
            const Size& minStreamSize, bool depthEnabled);
    // Trim supported format list by the cropping type. Also sort output formats by width/height
    static void trimSupportedFormats(CroppingType cropType,
                                     /*inout*/ std::vector<SupportedV4L2Format>* pFmts);

    Mutex mLock;
    bool mInitialized = false;
    bool mInitFailed = false;
    std::string mCameraId;
    std::string mDevicePath;
    const ExternalCameraConfig& mCfg;
    std::vector<SupportedV4L2Format> mSupportedFormats;
    CroppingType mCroppingType;

    std::weak_ptr<ExternalCameraDeviceSession> mSession =
            std::weak_ptr<ExternalCameraDeviceSession>();

    ::android::hardware::camera::common::V1_0::helper::CameraMetadata mCameraCharacteristics;

    const std::vector<int32_t> AVAILABLE_CHARACTERISTICS_KEYS = {
            ANDROID_COLOR_CORRECTION_AVAILABLE_ABERRATION_MODES,
            ANDROID_CONTROL_AE_AVAILABLE_ANTIBANDING_MODES,
            ANDROID_CONTROL_AE_AVAILABLE_MODES,
            ANDROID_CONTROL_AE_AVAILABLE_TARGET_FPS_RANGES,
            ANDROID_CONTROL_AE_COMPENSATION_RANGE,
            ANDROID_CONTROL_AE_COMPENSATION_STEP,
            ANDROID_CONTROL_AE_LOCK_AVAILABLE,
            ANDROID_CONTROL_AF_AVAILABLE_MODES,
            ANDROID_CONTROL_AVAILABLE_EFFECTS,
            ANDROID_CONTROL_AVAILABLE_MODES,
            ANDROID_CONTROL_AVAILABLE_SCENE_MODES,
            ANDROID_CONTROL_AVAILABLE_VIDEO_STABILIZATION_MODES,
            ANDROID_CONTROL_AWB_AVAILABLE_MODES,
            ANDROID_CONTROL_AWB_LOCK_AVAILABLE,
            ANDROID_CONTROL_MAX_REGIONS,
            ANDROID_FLASH_INFO_AVAILABLE,
            ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL,
            ANDROID_JPEG_AVAILABLE_THUMBNAIL_SIZES,
            ANDROID_LENS_FACING,
            ANDROID_LENS_INFO_AVAILABLE_OPTICAL_STABILIZATION,
            ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION,
            ANDROID_NOISE_REDUCTION_AVAILABLE_NOISE_REDUCTION_MODES,
            ANDROID_REQUEST_AVAILABLE_CAPABILITIES,
            ANDROID_REQUEST_MAX_NUM_INPUT_STREAMS,
            ANDROID_REQUEST_MAX_NUM_OUTPUT_STREAMS,
            ANDROID_REQUEST_PARTIAL_RESULT_COUNT,
            ANDROID_REQUEST_PIPELINE_MAX_DEPTH,
            ANDROID_SCALER_AVAILABLE_MAX_DIGITAL_ZOOM,
            ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS,
            ANDROID_SCALER_CROPPING_TYPE,
            ANDROID_SENSOR_INFO_ACTIVE_ARRAY_SIZE,
            ANDROID_SENSOR_INFO_MAX_FRAME_DURATION,
            ANDROID_SENSOR_INFO_PIXEL_ARRAY_SIZE,
            ANDROID_SENSOR_INFO_PRE_CORRECTION_ACTIVE_ARRAY_SIZE,
            ANDROID_SENSOR_INFO_TIMESTAMP_SOURCE,
            ANDROID_SENSOR_ORIENTATION,
            ANDROID_SHADING_AVAILABLE_MODES,
            ANDROID_STATISTICS_INFO_AVAILABLE_FACE_DETECT_MODES,
            ANDROID_STATISTICS_INFO_AVAILABLE_HOT_PIXEL_MAP_MODES,
            ANDROID_STATISTICS_INFO_AVAILABLE_LENS_SHADING_MAP_MODES,
            ANDROID_STATISTICS_INFO_MAX_FACE_COUNT,
            ANDROID_SYNC_MAX_LATENCY};
};

}  // namespace implementation
}  // namespace device
}  // namespace camera
}  // namespace hardware
}  // namespace android

#endif  // HARDWARE_INTERFACES_CAMERA_DEVICE_DEFAULT_EXTERNALCAMERADEVICE_H_
