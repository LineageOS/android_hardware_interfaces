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

#define LOG_TAG "AHAL_BluetoothPortProxy"

#include <android-base/logging.h>
#include <android-base/stringprintf.h>
#include <audio_utils/primitives.h>
#include <inttypes.h>
#include <log/log.h>

#include "core-impl/DevicePortProxy.h"

namespace android::bluetooth::audio::aidl {

namespace {

// The maximum time to wait in std::condition_variable::wait_for()
constexpr unsigned int kMaxWaitingTimeMs = 4500;

}  // namespace

using ::aidl::android::hardware::audio::common::SinkMetadata;
using ::aidl::android::hardware::audio::common::SourceMetadata;
using ::aidl::android::hardware::bluetooth::audio::AudioConfiguration;
using ::aidl::android::hardware::bluetooth::audio::BluetoothAudioSessionControl;
using ::aidl::android::hardware::bluetooth::audio::BluetoothAudioStatus;
using ::aidl::android::hardware::bluetooth::audio::ChannelMode;
using ::aidl::android::hardware::bluetooth::audio::PcmConfiguration;
using ::aidl::android::hardware::bluetooth::audio::PortStatusCallbacks;
using ::aidl::android::hardware::bluetooth::audio::PresentationPosition;
using ::aidl::android::hardware::bluetooth::audio::SessionType;
using ::aidl::android::media::audio::common::AudioDeviceDescription;
using ::aidl::android::media::audio::common::AudioDeviceType;
using ::android::base::StringPrintf;

std::ostream& operator<<(std::ostream& os, const BluetoothStreamState& state) {
    switch (state) {
        case BluetoothStreamState::DISABLED:
            return os << "DISABLED";
        case BluetoothStreamState::STANDBY:
            return os << "STANDBY";
        case BluetoothStreamState::STARTING:
            return os << "STARTING";
        case BluetoothStreamState::STARTED:
            return os << "STARTED";
        case BluetoothStreamState::SUSPENDING:
            return os << "SUSPENDING";
        case BluetoothStreamState::UNKNOWN:
            return os << "UNKNOWN";
        default:
            return os << android::base::StringPrintf("%#hhx", state);
    }
}

BluetoothAudioPortAidl::BluetoothAudioPortAidl()
    : mCookie(::aidl::android::hardware::bluetooth::audio::kObserversCookieUndefined),
      mState(BluetoothStreamState::DISABLED),
      mSessionType(SessionType::UNKNOWN) {}

BluetoothAudioPortAidl::~BluetoothAudioPortAidl() {
    unregisterPort();
}

bool BluetoothAudioPortAidl::registerPort(const AudioDeviceDescription& description) {
    if (inUse()) {
        LOG(ERROR) << __func__ << debugMessage() << " already in use";
        return false;
    }

    if (!initSessionType(description)) return false;

    auto control_result_cb = [port = this](uint16_t cookie, bool start_resp,
                                           const BluetoothAudioStatus& status) {
        (void)start_resp;
        port->controlResultHandler(cookie, status);
    };
    auto session_changed_cb = [port = this](uint16_t cookie) {
        port->sessionChangedHandler(cookie);
    };
    // TODO: Add audio_config_changed_cb
    PortStatusCallbacks cbacks = {
            .control_result_cb_ = control_result_cb,
            .session_changed_cb_ = session_changed_cb,
    };
    mCookie = BluetoothAudioSessionControl::RegisterControlResultCback(mSessionType, cbacks);
    auto isOk = (mCookie != ::aidl::android::hardware::bluetooth::audio::kObserversCookieUndefined);
    if (isOk) {
        std::lock_guard guard(mCvMutex);
        mState = BluetoothStreamState::STANDBY;
    }
    LOG(DEBUG) << __func__ << debugMessage();
    return isOk;
}

bool BluetoothAudioPortAidl::initSessionType(const AudioDeviceDescription& description) {
    if (description.connection == AudioDeviceDescription::CONNECTION_BT_A2DP &&
        (description.type == AudioDeviceType::OUT_DEVICE ||
         description.type == AudioDeviceType::OUT_HEADPHONE ||
         description.type == AudioDeviceType::OUT_SPEAKER)) {
        LOG(VERBOSE) << __func__
                     << ": device=AUDIO_DEVICE_OUT_BLUETOOTH_A2DP (HEADPHONES/SPEAKER) ("
                     << description.toString() << ")";
        mSessionType = SessionType::A2DP_SOFTWARE_ENCODING_DATAPATH;
    } else if (description.connection == AudioDeviceDescription::CONNECTION_WIRELESS &&
               description.type == AudioDeviceType::OUT_HEARING_AID) {
        LOG(VERBOSE) << __func__ << ": device=AUDIO_DEVICE_OUT_HEARING_AID (MEDIA/VOICE) ("
                     << description.toString() << ")";
        mSessionType = SessionType::HEARING_AID_SOFTWARE_ENCODING_DATAPATH;
    } else if (description.connection == AudioDeviceDescription::CONNECTION_BT_LE &&
               description.type == AudioDeviceType::OUT_HEADSET) {
        LOG(VERBOSE) << __func__ << ": device=AUDIO_DEVICE_OUT_BLE_HEADSET (MEDIA/VOICE) ("
                     << description.toString() << ")";
        mSessionType = SessionType::LE_AUDIO_SOFTWARE_ENCODING_DATAPATH;
    } else if (description.connection == AudioDeviceDescription::CONNECTION_BT_LE &&
               description.type == AudioDeviceType::OUT_SPEAKER) {
        LOG(VERBOSE) << __func__ << ": device=AUDIO_DEVICE_OUT_BLE_SPEAKER (MEDIA) ("
                     << description.toString() << ")";
        mSessionType = SessionType::LE_AUDIO_SOFTWARE_ENCODING_DATAPATH;
    } else if (description.connection == AudioDeviceDescription::CONNECTION_BT_LE &&
               description.type == AudioDeviceType::IN_HEADSET) {
        LOG(VERBOSE) << __func__ << ": device=AUDIO_DEVICE_IN_BLE_HEADSET (VOICE) ("
                     << description.toString() << ")";
        mSessionType = SessionType::LE_AUDIO_SOFTWARE_DECODING_DATAPATH;
    } else if (description.connection == AudioDeviceDescription::CONNECTION_BT_LE &&
               description.type == AudioDeviceType::OUT_BROADCAST) {
        LOG(VERBOSE) << __func__ << ": device=AUDIO_DEVICE_OUT_BLE_BROADCAST (MEDIA) ("
                     << description.toString() << ")";
        mSessionType = SessionType::LE_AUDIO_BROADCAST_SOFTWARE_ENCODING_DATAPATH;
    } else {
        LOG(ERROR) << __func__ << ": unknown device=" << description.toString();
        return false;
    }

    if (!BluetoothAudioSessionControl::IsSessionReady(mSessionType)) {
        LOG(ERROR) << __func__ << ": device=" << description.toString()
                   << ", session_type=" << toString(mSessionType) << " is not ready";
        return false;
    }
    return true;
}

void BluetoothAudioPortAidl::unregisterPort() {
    if (!inUse()) {
        LOG(WARNING) << __func__ << ": BluetoothAudioPortAidl is not in use";
        return;
    }
    BluetoothAudioSessionControl::UnregisterControlResultCback(mSessionType, mCookie);
    mCookie = ::aidl::android::hardware::bluetooth::audio::kObserversCookieUndefined;
    LOG(VERBOSE) << __func__ << debugMessage() << " port unregistered";
}

void BluetoothAudioPortAidl::controlResultHandler(uint16_t cookie,
                                                  const BluetoothAudioStatus& status) {
    std::lock_guard guard(mCvMutex);
    if (!inUse()) {
        LOG(ERROR) << "control_result_cb: BluetoothAudioPortAidl is not in use";
        return;
    }
    if (mCookie != cookie) {
        LOG(ERROR) << "control_result_cb: proxy of device port (cookie="
                   << StringPrintf("%#hx", cookie) << ") is corrupted";
        return;
    }
    BluetoothStreamState previous_state = mState;
    LOG(INFO) << "control_result_cb:" << debugMessage() << ", previous_state=" << previous_state
              << ", status=" << toString(status);

    switch (previous_state) {
        case BluetoothStreamState::STARTED:
            /* Only Suspend signal can be send in STARTED state*/
            if (status == BluetoothAudioStatus::RECONFIGURATION ||
                status == BluetoothAudioStatus::SUCCESS) {
                mState = BluetoothStreamState::STANDBY;
            } else {
                LOG(WARNING) << StringPrintf(
                        "control_result_cb: status=%s failure for session_type= %s, cookie=%#hx, "
                        "previous_state=%#hhx",
                        toString(status).c_str(), toString(mSessionType).c_str(), mCookie,
                        previous_state);
            }
            break;
        case BluetoothStreamState::STARTING:
            if (status == BluetoothAudioStatus::SUCCESS) {
                mState = BluetoothStreamState::STARTED;
            } else {
                // Set to standby since the stack may be busy switching between outputs
                LOG(WARNING) << StringPrintf(
                        "control_result_cb: status=%s failure for session_type= %s, cookie=%#hx, "
                        "previous_state=%#hhx",
                        toString(status).c_str(), toString(mSessionType).c_str(), mCookie,
                        previous_state);
                mState = BluetoothStreamState::STANDBY;
            }
            break;
        case BluetoothStreamState::SUSPENDING:
            if (status == BluetoothAudioStatus::SUCCESS) {
                mState = BluetoothStreamState::STANDBY;
            } else {
                // It will be failed if the headset is disconnecting, and set to disable
                // to wait for re-init again
                LOG(WARNING) << StringPrintf(
                        "control_result_cb: status=%s failure for session_type= %s, cookie=%#hx, "
                        "previous_state=%#hhx",
                        toString(status).c_str(), toString(mSessionType).c_str(), mCookie,
                        previous_state);
                mState = BluetoothStreamState::DISABLED;
            }
            break;
        default:
            LOG(ERROR) << "control_result_cb: unexpected previous_state="
                       << StringPrintf(
                                  "control_result_cb: status=%s failure for session_type= %s, "
                                  "cookie=%#hx, previous_state=%#hhx",
                                  toString(status).c_str(), toString(mSessionType).c_str(), mCookie,
                                  previous_state);
            return;
    }
    mInternalCv.notify_all();
}

void BluetoothAudioPortAidl::sessionChangedHandler(uint16_t cookie) {
    std::lock_guard guard(mCvMutex);
    if (!inUse()) {
        LOG(ERROR) << "session_changed_cb: BluetoothAudioPortAidl is not in use";
        return;
    }
    if (mCookie != cookie) {
        LOG(ERROR) << "session_changed_cb: proxy of device port (cookie="
                   << StringPrintf("%#hx", cookie) << ") is corrupted";
        return;
    }
    BluetoothStreamState previous_state = mState;
    LOG(VERBOSE) << "session_changed_cb:" << debugMessage()
                 << ", previous_state=" << previous_state;
    mState = BluetoothStreamState::DISABLED;
    mInternalCv.notify_all();
}

bool BluetoothAudioPortAidl::inUse() const {
    return (mCookie != ::aidl::android::hardware::bluetooth::audio::kObserversCookieUndefined);
}

bool BluetoothAudioPortAidl::getPreferredDataIntervalUs(size_t* interval_us) const {
    if (!interval_us) {
        LOG(ERROR) << __func__ << ": bad input arg";
        return false;
    }

    if (!inUse()) {
        LOG(ERROR) << __func__ << ": BluetoothAudioPortAidl is not in use";
        return false;
    }

    const AudioConfiguration& hal_audio_cfg =
            BluetoothAudioSessionControl::GetAudioConfig(mSessionType);
    if (hal_audio_cfg.getTag() != AudioConfiguration::pcmConfig) {
        LOG(ERROR) << __func__ << ": unsupported audio cfg tag";
        return false;
    }

    *interval_us = hal_audio_cfg.get<AudioConfiguration::pcmConfig>().dataIntervalUs;
    return true;
}

bool BluetoothAudioPortAidl::loadAudioConfig(PcmConfiguration* audio_cfg) const {
    if (!audio_cfg) {
        LOG(ERROR) << __func__ << ": bad input arg";
        return false;
    }

    if (!inUse()) {
        LOG(ERROR) << __func__ << ": BluetoothAudioPortAidl is not in use";
        return false;
    }

    const AudioConfiguration& hal_audio_cfg =
            BluetoothAudioSessionControl::GetAudioConfig(mSessionType);
    if (hal_audio_cfg.getTag() != AudioConfiguration::pcmConfig) {
        LOG(ERROR) << __func__ << ": unsupported audio cfg tag";
        return false;
    }
    *audio_cfg = hal_audio_cfg.get<AudioConfiguration::pcmConfig>();
    LOG(VERBOSE) << __func__ << debugMessage() << ", state*=" << getState() << ", PcmConfig=["
                 << audio_cfg->toString() << "]";
    if (audio_cfg->channelMode == ChannelMode::UNKNOWN) {
        return false;
    }
    return true;
}

bool BluetoothAudioPortAidl::standby() {
    if (!inUse()) {
        LOG(ERROR) << __func__ << ": BluetoothAudioPortAidl is not in use";
        return false;
    }
    std::lock_guard guard(mCvMutex);
    LOG(VERBOSE) << __func__ << debugMessage() << ", state=" << getState() << " request";
    if (mState == BluetoothStreamState::DISABLED) {
        mState = BluetoothStreamState::STANDBY;
        LOG(VERBOSE) << __func__ << debugMessage() << ", state=" << getState() << " done";
        return true;
    }
    return false;
}

bool BluetoothAudioPortAidl::condWaitState(BluetoothStreamState state) {
    const auto waitTime = std::chrono::milliseconds(kMaxWaitingTimeMs);
    std::unique_lock lock(mCvMutex);
    base::ScopedLockAssertion lock_assertion(mCvMutex);
    switch (state) {
        case BluetoothStreamState::STARTING: {
            LOG(VERBOSE) << __func__ << debugMessage() << " waiting for STARTED";
            mInternalCv.wait_for(lock, waitTime, [this] {
                base::ScopedLockAssertion lock_assertion(mCvMutex);
                return mState != BluetoothStreamState::STARTING;
            });
            return mState == BluetoothStreamState::STARTED;
        }
        case BluetoothStreamState::SUSPENDING: {
            LOG(VERBOSE) << __func__ << debugMessage() << " waiting for SUSPENDED";
            mInternalCv.wait_for(lock, waitTime, [this] {
                base::ScopedLockAssertion lock_assertion(mCvMutex);
                return mState != BluetoothStreamState::SUSPENDING;
            });
            return mState == BluetoothStreamState::STANDBY;
        }
        default:
            LOG(WARNING) << __func__ << debugMessage() << " waiting for KNOWN";
            return false;
    }
    return false;
}

bool BluetoothAudioPortAidl::start() {
    if (!inUse()) {
        LOG(ERROR) << __func__ << ": BluetoothAudioPortAidl is not in use";
        return false;
    }
    LOG(VERBOSE) << __func__ << debugMessage() << ", state=" << getState()
                 << ", mono=" << (mIsStereoToMono ? "true" : "false") << " request";

    {
        std::unique_lock lock(mCvMutex);
        base::ScopedLockAssertion lock_assertion(mCvMutex);
        if (mState == BluetoothStreamState::STARTED) {
            return true;  // nop, return
        } else if (mState == BluetoothStreamState::SUSPENDING ||
                   mState == BluetoothStreamState::STARTING) {
            /* If port is in transient state, give some time to respond */
            auto state_ = mState;
            lock.unlock();
            if (!condWaitState(state_)) {
                LOG(ERROR) << __func__ << debugMessage() << ", state=" << getState() << " failure";
                return false;
            }
        }
    }

    bool retval = false;
    {
        std::unique_lock lock(mCvMutex);
        base::ScopedLockAssertion lock_assertion(mCvMutex);
        if (mState == BluetoothStreamState::STARTED) {
            retval = true;
        } else if (mState == BluetoothStreamState::STANDBY) {
            mState = BluetoothStreamState::STARTING;
            lock.unlock();
            if (BluetoothAudioSessionControl::StartStream(mSessionType)) {
                retval = condWaitState(BluetoothStreamState::STARTING);
            } else {
                LOG(ERROR) << __func__ << debugMessage() << ", state=" << getState()
                           << " Hal fails";
            }
        }
    }

    if (retval) {
        LOG(INFO) << __func__ << debugMessage() << ", state=" << getState()
                  << ", mono=" << (mIsStereoToMono ? "true" : "false") << " done";
    } else {
        LOG(ERROR) << __func__ << debugMessage() << ", state=" << getState() << " failure";
    }

    return retval;  // false if any failure like timeout
}

bool BluetoothAudioPortAidl::suspend() {
    if (!inUse()) {
        LOG(ERROR) << __func__ << ": BluetoothAudioPortAidl is not in use";
        return false;
    }
    LOG(VERBOSE) << __func__ << debugMessage() << ", state=" << getState() << " request";

    {
        std::unique_lock lock(mCvMutex);
        base::ScopedLockAssertion lock_assertion(mCvMutex);
        if (mState == BluetoothStreamState::STANDBY) {
            return true;  // nop, return
        } else if (mState == BluetoothStreamState::SUSPENDING ||
                   mState == BluetoothStreamState::STARTING) {
            /* If port is in transient state, give some time to respond */
            auto state_ = mState;
            lock.unlock();
            if (!condWaitState(state_)) {
                LOG(ERROR) << __func__ << debugMessage() << ", state=" << getState() << " failure";
                return false;
            }
        }
    }

    bool retval = false;
    {
        std::unique_lock lock(mCvMutex);
        base::ScopedLockAssertion lock_assertion(mCvMutex);
        if (mState == BluetoothStreamState::STANDBY) {
            retval = true;
        } else if (mState == BluetoothStreamState::STARTED) {
            mState = BluetoothStreamState::SUSPENDING;
            lock.unlock();
            if (BluetoothAudioSessionControl::SuspendStream(mSessionType)) {
                retval = condWaitState(BluetoothStreamState::SUSPENDING);
            } else {
                LOG(ERROR) << __func__ << debugMessage() << ", state=" << getState()
                           << " Hal fails";
            }
        }
    }

    if (retval) {
        LOG(INFO) << __func__ << debugMessage() << ", state=" << getState() << " done";
    } else {
        LOG(ERROR) << __func__ << debugMessage() << ", state=" << getState() << " failure";
    }

    return retval;  // false if any failure like timeout
}

void BluetoothAudioPortAidl::stop() {
    if (!inUse()) {
        LOG(ERROR) << __func__ << ": BluetoothAudioPortAidl is not in use";
        return;
    }
    std::lock_guard guard(mCvMutex);
    LOG(VERBOSE) << __func__ << debugMessage() << ", state=" << getState() << " request";
    if (mState != BluetoothStreamState::DISABLED) {
        BluetoothAudioSessionControl::StopStream(mSessionType);
        mState = BluetoothStreamState::DISABLED;
    }
    LOG(VERBOSE) << __func__ << debugMessage() << ", state=" << getState() << " done";
}

size_t BluetoothAudioPortAidlOut::writeData(const void* buffer, size_t bytes) const {
    if (!buffer) {
        LOG(ERROR) << __func__ << ": bad input arg";
        return 0;
    }

    if (!inUse()) {
        LOG(ERROR) << __func__ << ": BluetoothAudioPortAidl is not in use";
        return 0;
    }

    if (!mIsStereoToMono) {
        return BluetoothAudioSessionControl::OutWritePcmData(mSessionType, buffer, bytes);
    }

    // WAR to mix the stereo into Mono (16 bits per sample)
    const size_t write_frames = bytes >> 2;
    if (write_frames == 0) return 0;
    auto src = static_cast<const int16_t*>(buffer);
    std::unique_ptr<int16_t[]> dst{new int16_t[write_frames]};
    downmix_to_mono_i16_from_stereo_i16(dst.get(), src, write_frames);
    // a frame is 16 bits, and the size of a mono frame is equal to half a stereo.
    auto totalWrite = BluetoothAudioSessionControl::OutWritePcmData(mSessionType, dst.get(),
                                                                    write_frames * 2);
    return totalWrite * 2;
}

size_t BluetoothAudioPortAidlIn::readData(void* buffer, size_t bytes) const {
    if (!buffer) {
        LOG(ERROR) << __func__ << ": bad input arg";
        return 0;
    }

    if (!inUse()) {
        LOG(ERROR) << __func__ << ": BluetoothAudioPortAidl is not in use";
        return 0;
    }

    return BluetoothAudioSessionControl::InReadPcmData(mSessionType, buffer, bytes);
}

bool BluetoothAudioPortAidl::getPresentationPosition(
        PresentationPosition& presentation_position) const {
    if (!inUse()) {
        LOG(ERROR) << __func__ << ": BluetoothAudioPortAidl is not in use";
        return false;
    }
    bool retval = BluetoothAudioSessionControl::GetPresentationPosition(mSessionType,
                                                                        presentation_position);
    LOG(VERBOSE) << __func__ << debugMessage() << ", state=" << getState()
                 << presentation_position.toString();

    return retval;
}

bool BluetoothAudioPortAidl::updateSourceMetadata(const SourceMetadata& source_metadata) const {
    if (!inUse()) {
        LOG(ERROR) << __func__ << ": BluetoothAudioPortAidl is not in use";
        return false;
    }
    LOG(DEBUG) << __func__ << debugMessage() << ", state=" << getState() << ", "
               << source_metadata.tracks.size() << " track(s)";
    if (source_metadata.tracks.size() == 0) return true;
    return BluetoothAudioSessionControl::UpdateSourceMetadata(mSessionType, source_metadata);
}

bool BluetoothAudioPortAidl::updateSinkMetadata(const SinkMetadata& sink_metadata) const {
    if (!inUse()) {
        LOG(ERROR) << __func__ << ": BluetoothAudioPortAidl is not in use";
        return false;
    }
    LOG(DEBUG) << __func__ << debugMessage() << ", state=" << getState() << ", "
               << sink_metadata.tracks.size() << " track(s)";
    if (sink_metadata.tracks.size() == 0) return true;
    return BluetoothAudioSessionControl::UpdateSinkMetadata(mSessionType, sink_metadata);
}

BluetoothStreamState BluetoothAudioPortAidl::getState() const {
    return mState;
}

bool BluetoothAudioPortAidl::setState(BluetoothStreamState state) {
    if (!inUse()) {
        LOG(ERROR) << __func__ << ": BluetoothAudioPortAidl is not in use";
        return false;
    }
    std::lock_guard guard(mCvMutex);
    LOG(DEBUG) << __func__ << ": BluetoothAudioPortAidl old state = " << mState
               << " new state = " << state;
    mState = state;
    return true;
}

bool BluetoothAudioPortAidl::isA2dp() const {
    return mSessionType == SessionType::A2DP_SOFTWARE_ENCODING_DATAPATH ||
           mSessionType == SessionType::A2DP_HARDWARE_OFFLOAD_ENCODING_DATAPATH;
}

bool BluetoothAudioPortAidl::isLeAudio() const {
    return mSessionType == SessionType::LE_AUDIO_SOFTWARE_ENCODING_DATAPATH ||
           mSessionType == SessionType::LE_AUDIO_SOFTWARE_DECODING_DATAPATH ||
           mSessionType == SessionType::LE_AUDIO_HARDWARE_OFFLOAD_ENCODING_DATAPATH ||
           mSessionType == SessionType::LE_AUDIO_HARDWARE_OFFLOAD_DECODING_DATAPATH ||
           mSessionType == SessionType::LE_AUDIO_BROADCAST_SOFTWARE_ENCODING_DATAPATH ||
           mSessionType == SessionType::LE_AUDIO_BROADCAST_HARDWARE_OFFLOAD_ENCODING_DATAPATH;
}

std::string BluetoothAudioPortAidl::debugMessage() const {
    return StringPrintf(": session_type=%s, cookie=%#hx", toString(mSessionType).c_str(), mCookie);
}

}  // namespace android::bluetooth::audio::aidl
