/*
 * Copyright 2023 The Android Open Source Project
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

#pragma once

#include <condition_variable>
#include <mutex>

#include <android-base/thread_annotations.h>

#include <aidl/android/hardware/audio/common/SinkMetadata.h>
#include <aidl/android/hardware/audio/common/SourceMetadata.h>
#include <aidl/android/hardware/bluetooth/audio/BluetoothAudioStatus.h>
#include <aidl/android/hardware/bluetooth/audio/PcmConfiguration.h>
#include <aidl/android/hardware/bluetooth/audio/SessionType.h>
#include <aidl/android/media/audio/common/AudioDeviceDescription.h>

#include "BluetoothAudioSessionControl.h"

namespace android::bluetooth::audio::aidl {

enum class BluetoothStreamState : uint8_t {
    DISABLED = 0,  // This stream is closing or Bluetooth profiles (A2DP/LE) is disabled
    STANDBY,
    STARTING,
    STARTED,
    SUSPENDING,
    UNKNOWN,
};

std::ostream& operator<<(std::ostream& os, const BluetoothStreamState& state);

/**
 * Proxy for Bluetooth Audio HW Module to communicate with Bluetooth Audio
 * Session Control. All methods are not thread safe, so users must acquire a
 * lock. Note: currently, getState() of DevicePortProxy is only used for
 * verbose logging, it is not locked, so the state may not be synchronized.
 */
class BluetoothAudioPort {
  public:
    BluetoothAudioPort() = default;
    virtual ~BluetoothAudioPort() = default;

    /**
     * Fetch output control / data path of BluetoothAudioPort and setup
     * callbacks into BluetoothAudioProvider. If registerPort() returns false, the audio
     * HAL must delete this BluetoothAudioPort and return EINVAL to caller
     */
    virtual bool registerPort(
            const ::aidl::android::media::audio::common::AudioDeviceDescription&) = 0;

    /**
     * Unregister this BluetoothAudioPort from BluetoothAudioSessionControl.
     * Audio HAL must delete this BluetoothAudioPort after calling this.
     */
    virtual void unregisterPort() = 0;

    /**
     * When the Audio framework / HAL tries to query audio config about format,
     * channel mask and sample rate, it uses this function to fetch from the
     * Bluetooth stack
     */
    virtual bool loadAudioConfig(
            ::aidl::android::hardware::bluetooth::audio::PcmConfiguration*) const = 0;

    /**
     * WAR to support Mono mode / 16 bits per sample
     */
    virtual void forcePcmStereoToMono(bool) = 0;

    /**
     * When the Audio framework / HAL wants to change the stream state, it invokes
     * these 4 functions to control the Bluetooth stack (Audio Control Path).
     * Note: standby(), start() and suspend() will return true when there are no errors.

     * Called by Audio framework / HAL to change the state to stand by. When A2DP/LE profile is
     * disabled, the port is first set to STANDBY by calling suspend and then mState is set to
     * DISABLED. To reset the state back to STANDBY this method is called.
     */
    virtual bool standby() = 0;

    /**
     * Called by Audio framework / HAL to start the stream
     */
    virtual bool start() = 0;

    /**
     * Called by Audio framework / HAL to suspend the stream
     */
    virtual bool suspend() = 0;

    /**
     * Called by Audio framework / HAL to stop the stream
     */
    virtual void stop() = 0;

    /**
     * Called by the Audio framework / HAL to fetch information about audio frames
     * presented to an external sink, or frames presented fror an internal sink
     */
    virtual bool getPresentationPosition(
            ::aidl::android::hardware::bluetooth::audio::PresentationPosition&) const = 0;

    /**
     * Called by the Audio framework / HAL when the metadata of the stream's
     * source has been changed.
     */
    virtual bool updateSourceMetadata(
            const ::aidl::android::hardware::audio::common::SourceMetadata&) const {
        return false;
    }

    /**
     * Called by the Audio framework / HAL when the metadata of the stream's
     * sink has been changed.
     */
    virtual bool updateSinkMetadata(
            const ::aidl::android::hardware::audio::common::SinkMetadata&) const {
        return false;
    }

    /**
     * Return the current BluetoothStreamState
     */
    virtual BluetoothStreamState getState() const = 0;

    /**
     * Set the current BluetoothStreamState
     */
    virtual bool setState(BluetoothStreamState) = 0;

    virtual bool isA2dp() const = 0;

    virtual bool isLeAudio() const = 0;

    virtual bool getPreferredDataIntervalUs(size_t*) const = 0;

    virtual size_t writeData(const void*, size_t) const { return 0; }

    virtual size_t readData(void*, size_t) const { return 0; }
};

class BluetoothAudioPortAidl : public BluetoothAudioPort {
  public:
    BluetoothAudioPortAidl();
    virtual ~BluetoothAudioPortAidl();

    bool registerPort(const ::aidl::android::media::audio::common::AudioDeviceDescription&
                              description) override;

    void unregisterPort() override;

    bool loadAudioConfig(::aidl::android::hardware::bluetooth::audio::PcmConfiguration* audio_cfg)
            const override;

    void forcePcmStereoToMono(bool force) override { mIsStereoToMono = force; }

    bool standby() override;
    bool start() override;
    bool suspend() override;
    void stop() override;

    bool getPresentationPosition(::aidl::android::hardware::bluetooth::audio::PresentationPosition&
                                         presentation_position) const override;

    bool updateSourceMetadata(const ::aidl::android::hardware::audio::common::SourceMetadata&
                                      sourceMetadata) const override;

    bool updateSinkMetadata(const ::aidl::android::hardware::audio::common::SinkMetadata&
                                    sinkMetadata) const override;

    /**
     * Return the current BluetoothStreamState
     * Note: This method is used for logging, does not lock, so value returned may not be latest
     */
    BluetoothStreamState getState() const override NO_THREAD_SAFETY_ANALYSIS;

    bool setState(BluetoothStreamState state) override;

    bool isA2dp() const override;

    bool isLeAudio() const override;

    bool getPreferredDataIntervalUs(size_t* interval_us) const override;

  protected:
    uint16_t mCookie;
    BluetoothStreamState mState GUARDED_BY(mCvMutex);
    ::aidl::android::hardware::bluetooth::audio::SessionType mSessionType;
    // WR to support Mono: True if fetching Stereo and mixing into Mono
    bool mIsStereoToMono = false;

    bool inUse() const;

    std::string debugMessage() const;

  private:
    // start()/suspend() report state change status via callback. Wait until kMaxWaitingTimeMs or a
    // state change after a call to start()/suspend() and analyse the returned status. Below mutex,
    // conditional variable serves this purpose.
    mutable std::mutex mCvMutex;
    std::condition_variable mInternalCv GUARDED_BY(mCvMutex);

    // Check and initialize session type for |devices| If failed, this
    // BluetoothAudioPortAidl is not initialized and must be deleted.
    bool initSessionType(
            const ::aidl::android::media::audio::common::AudioDeviceDescription& description);

    bool condWaitState(BluetoothStreamState state);

    void controlResultHandler(
            uint16_t cookie,
            const ::aidl::android::hardware::bluetooth::audio::BluetoothAudioStatus& status);
    void sessionChangedHandler(uint16_t cookie);
};

class BluetoothAudioPortAidlOut : public BluetoothAudioPortAidl {
  public:
    // The audio data path to the Bluetooth stack (Software encoding)
    size_t writeData(const void* buffer, size_t bytes) const override;
};

class BluetoothAudioPortAidlIn : public BluetoothAudioPortAidl {
  public:
    // The audio data path from the Bluetooth stack (Software decoded)
    size_t readData(void* buffer, size_t bytes) const override;
};

}  // namespace android::bluetooth::audio::aidl