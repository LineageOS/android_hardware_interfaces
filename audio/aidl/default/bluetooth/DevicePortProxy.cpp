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

#define LOG_TAG "AHAL_BluetoothAudioPort"

#include <android-base/logging.h>
#include <android-base/stringprintf.h>
#include <audio_utils/primitives.h>
#include <log/log.h>

#include "BluetoothAudioSessionControl.h"
#include "core-impl/DevicePortProxy.h"

using aidl::android::hardware::audio::common::SinkMetadata;
using aidl::android::hardware::audio::common::SourceMetadata;
using aidl::android::hardware::bluetooth::audio::AudioConfiguration;
using aidl::android::hardware::bluetooth::audio::BluetoothAudioSessionControl;
using aidl::android::hardware::bluetooth::audio::BluetoothAudioStatus;
using aidl::android::hardware::bluetooth::audio::ChannelMode;
using aidl::android::hardware::bluetooth::audio::LatencyMode;
using aidl::android::hardware::bluetooth::audio::PcmConfiguration;
using aidl::android::hardware::bluetooth::audio::PortStatusCallbacks;
using aidl::android::hardware::bluetooth::audio::PresentationPosition;
using aidl::android::hardware::bluetooth::audio::SessionType;
using aidl::android::media::audio::common::AudioDeviceDescription;
using aidl::android::media::audio::common::AudioDeviceType;
using android::base::StringPrintf;

namespace android::bluetooth::audio::aidl {

namespace {

// The maximum time to wait in std::condition_variable::wait_for()
constexpr unsigned int kMaxWaitingTimeMs = 4500;

}  // namespace

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

BluetoothAudioPortAidl::BluetoothAudioPortAidl(std::optional<bool> supportsLowLatency)
    : mCookie(::aidl::android::hardware::bluetooth::audio::kObserversCookieUndefined),
      mState(BluetoothStreamState::DISABLED),
      mSessionType(SessionType::UNKNOWN),
      mSupportsLowLatency(supportsLowLatency) {}

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
    auto low_latency_allowed_cb = [port = this](uint16_t cookie, bool allowed) {
        port->lowLatencyAllowedHandler(cookie, allowed);
    };

    PortStatusCallbacks cbacks = {
            .control_result_cb_ = control_result_cb,
            .session_changed_cb_ = session_changed_cb,
            .low_latency_mode_allowed_cb_ = low_latency_allowed_cb,
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
    ::aidl::android::hardware::bluetooth::audio::SessionType fallbackSessionType =
            SessionType::UNKNOWN;
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
        fallbackSessionType = SessionType::LE_AUDIO_HARDWARE_OFFLOAD_ENCODING_DATAPATH;
    } else if (description.connection == AudioDeviceDescription::CONNECTION_BT_LE &&
               description.type == AudioDeviceType::OUT_SPEAKER) {
        LOG(VERBOSE) << __func__ << ": device=AUDIO_DEVICE_OUT_BLE_SPEAKER (MEDIA) ("
                     << description.toString() << ")";
        mSessionType = SessionType::LE_AUDIO_SOFTWARE_ENCODING_DATAPATH;
        fallbackSessionType = SessionType::LE_AUDIO_HARDWARE_OFFLOAD_ENCODING_DATAPATH;
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
        if (fallbackSessionType != SessionType::UNKNOWN) {
            LOG(WARNING) << __func__
                         << ": Retry fallback session_type=" << toString(fallbackSessionType)
                         << " for session_type=" << toString(mSessionType);
            if (BluetoothAudioSessionControl::IsSessionReady(fallbackSessionType, false)) {
                mSessionType = fallbackSessionType;
                return true;
            } else {
                LOG(ERROR) << __func__
                           << ": fallback session_type=" << toString(fallbackSessionType)
                           << " is not ready";
            }
        }
        LOG(ERROR) << __func__ << ": device=" << description.toString()
                   << ", session_type=" << toString(mSessionType) << " is not ready";
        return false;
    }
    return true;
}

void BluetoothAudioPortAidl::unregisterPort() {
    if (!inUse()) {
        LOG(WARNING) << __func__ << debugMessage() << ": BluetoothAudioPortAidl is not in use";
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
        LOG(ERROR) << "control_result_cb: proxy of device port is corrupted "
                   << "cookie=" << StringPrintf("%#hx", cookie) << ", expected "
                   << StringPrintf("%#hx", mCookie);
        return;
    }
    BluetoothStreamState previous_state = mState;
    ::android::base::LogSeverity severity = ::android::base::FATAL;
    switch (previous_state) {
        case BluetoothStreamState::STARTED:
            /* Only Suspend signal can be send in STARTED state*/
            if (status == BluetoothAudioStatus::RECONFIGURATION ||
                status == BluetoothAudioStatus::SUCCESS) {
                mState = BluetoothStreamState::STANDBY;
                severity = ::android::base::INFO;
            } else {
                severity = ::android::base::WARNING;
            }
            break;
        case BluetoothStreamState::STARTING:
            if (status == BluetoothAudioStatus::SUCCESS) {
                mState = BluetoothStreamState::STARTED;
                severity = ::android::base::INFO;
            } else {
                // Set to standby since the stack may be busy switching between outputs
                mState = BluetoothStreamState::STANDBY;
                severity = ::android::base::WARNING;
            }
            break;
        case BluetoothStreamState::SUSPENDING:
            if (status == BluetoothAudioStatus::SUCCESS) {
                mState = BluetoothStreamState::STANDBY;
                severity = ::android::base::INFO;
            } else {
                // Will fail if the headset is disconnecting, so set to disable
                // to wait for re-init again
                mState = BluetoothStreamState::DISABLED;
                severity = ::android::base::WARNING;
            }
            break;
        default:
            severity = ::android::base::ERROR;
    }
    if (previous_state != mState) {
        LOG(severity) << "control_result_cb" << debugMessage() << ", status=" << toString(status)
                      << ", " << previous_state << " -> " << mState;
    } else {
        LOG(severity) << "control_result_cb" << debugMessage() << ", status=" << toString(status)
                      << ", " << previous_state;
    }
    if (severity != ::android::base::ERROR) {
        mInternalCv.notify_all();
    }
}

void BluetoothAudioPortAidl::lowLatencyAllowedHandler(uint16_t cookie, bool allowed) {
    if (mCookie != cookie) {
        LOG(ERROR) << "low_latency_allowed_cb: proxy of device port (cookie="
                   << StringPrintf("%#hx", cookie) << ") is corrupted";
        return;
    }
    LOG(INFO) << "low_latency_allowed_cb:" << debugMessage() << ", allowed=" << allowed;
    std::vector<LatencyMode> latency_modes;
    if (!getRecommendedLatencyModes(&latency_modes)) return;
    std::shared_ptr<BluetoothAudioPortCallbacks> callbacks;
    {
        std::lock_guard guard(mCvMutex);
        callbacks = mCallbacks;
    }
    if (callbacks) {
        callbacks->onRecommendedLatencyModeChanged(latency_modes);
    }
}

void BluetoothAudioPortAidl::sessionChangedHandler(uint16_t cookie) {
    std::lock_guard guard(mCvMutex);
    if (!inUse()) {
        LOG(ERROR) << "session_changed_cb: BluetoothAudioPortAidl is not in use";
        return;
    }
    if (mCookie != cookie) {
        LOG(ERROR) << "session_changed_cb: proxy of device port is corrupted "
                   << "cookie=" << StringPrintf("%#hx", cookie) << ", expected "
                   << StringPrintf("%#hx", mCookie);
        return;
    }
    BluetoothStreamState previous_state = mState;
    mState = BluetoothStreamState::DISABLED;
    LOG(DEBUG) << "session_changed_cb" << debugMessage() << ", " << previous_state << " -> "
               << mState;
    mInternalCv.notify_all();
}

bool BluetoothAudioPortAidl::inUse() const {
    return (mCookie != ::aidl::android::hardware::bluetooth::audio::kObserversCookieUndefined);
}

bool BluetoothAudioPortAidl::getPreferredDataIntervalUs(size_t& interval_us) const {
    if (!inUse()) {
        LOG(ERROR) << __func__ << debugMessage() << ": BluetoothAudioPortAidl is not in use";
        return false;
    }

    const AudioConfiguration& hal_audio_cfg =
            BluetoothAudioSessionControl::GetAudioConfig(mSessionType);
    if (hal_audio_cfg.getTag() != AudioConfiguration::pcmConfig) {
        LOG(ERROR) << __func__ << debugMessage() << ": unsupported audio cfg tag";
        return false;
    }

    interval_us = hal_audio_cfg.get<AudioConfiguration::pcmConfig>().dataIntervalUs;
    return true;
}

bool BluetoothAudioPortAidl::getRecommendedLatencyModes(std::vector<LatencyMode>* latency_modes) {
    if (!inUse()) {
        LOG(ERROR) << __func__ << debugMessage() << ": BluetoothAudioPortAidl is not in use";
        return false;
    }
    *latency_modes = BluetoothAudioSessionControl::GetSupportedLatencyModes(mSessionType);
    LOG(INFO) << __func__ << debugMessage() << ": "
              << ::android::internal::ToString(*latency_modes);
    {
        std::lock_guard guard(mCvMutex);
        mSupportsLowLatency = std::find(latency_modes->begin(), latency_modes->end(),
                                        LatencyMode::LOW_LATENCY) != latency_modes->end();
    }
    return true;
}

bool BluetoothAudioPortAidl::loadAudioConfig(PcmConfiguration& audio_cfg) {
    if (!inUse()) {
        LOG(ERROR) << __func__ << debugMessage() << ": BluetoothAudioPortAidl is not in use";
        return false;
    }

    const AudioConfiguration& hal_audio_cfg =
            BluetoothAudioSessionControl::GetAudioConfig(mSessionType);
    if (hal_audio_cfg.getTag() != AudioConfiguration::pcmConfig) {
        LOG(ERROR) << __func__ << debugMessage()
                   << ": unsupported audio cfg tag: " << toString(hal_audio_cfg.getTag());
        return false;
    }
    audio_cfg = hal_audio_cfg.get<AudioConfiguration::pcmConfig>();
    LOG(VERBOSE) << __func__ << debugMessage() << ", state=" << getState() << ", PcmConfig=["
                 << audio_cfg.toString() << "]";
    if (audio_cfg.channelMode == ChannelMode::UNKNOWN) {
        LOG(ERROR) << __func__ << debugMessage()
                   << ": unsupported channel mode: " << toString(audio_cfg.channelMode);
        return false;
    }
    return true;
}

bool BluetoothAudioPortAidlOut::loadAudioConfig(PcmConfiguration& audio_cfg) {
    if (!BluetoothAudioPortAidl::loadAudioConfig(audio_cfg)) return false;
    // WAR to support Mono / 16 bits per sample as the Bluetooth stack requires
    if (audio_cfg.channelMode == ChannelMode::MONO && audio_cfg.bitsPerSample == 16) {
        mIsStereoToMono = true;
        audio_cfg.channelMode = ChannelMode::STEREO;
        LOG(INFO) << __func__ << debugMessage()
                  << ": force channels = to be AUDIO_CHANNEL_OUT_STEREO";
    }
    return true;
}

bool BluetoothAudioPortAidl::standby() {
    if (!inUse()) {
        LOG(ERROR) << __func__ << debugMessage() << ": BluetoothAudioPortAidl is not in use";
        return false;
    }
    std::lock_guard guard(mCvMutex);
    BluetoothStreamState previous_state = mState;
    LOG(VERBOSE) << __func__ << debugMessage() << ", state=" << mState << " request";
    if (mState == BluetoothStreamState::DISABLED) {
        mState = BluetoothStreamState::STANDBY;
        LOG(INFO) << __func__ << debugMessage() << ", " << previous_state << " -> " << mState;
        return true;
    }
    return false;
}

bool BluetoothAudioPortAidl::condWaitState(std::unique_lock<std::mutex>* lock) {
    const auto waitTime = std::chrono::milliseconds(kMaxWaitingTimeMs);
    const auto state = mState;
    if (state == BluetoothStreamState::STARTING || state == BluetoothStreamState::SUSPENDING) {
        LOG(DEBUG) << __func__ << debugMessage() << " waiting to change from " << state;
        mInternalCv.wait_for(*lock, waitTime, [this, state] {
            base::ScopedLockAssertion lock_assertion(mCvMutex);
            return mState != state;
        });
        const bool expected =
                mState == (state == BluetoothStreamState::STARTING ? BluetoothStreamState::STARTED
                                                                   : BluetoothStreamState::STANDBY);
        LOG(expected ? INFO : WARNING)
                << __func__ << debugMessage() << ", " << state << " -> " << mState;
        return expected;
    }
    LOG(ERROR) << __func__ << debugMessage() << " called to wait when in " << state;
    return false;
}

bool BluetoothAudioPortAidl::start() {
    if (!inUse()) {
        LOG(ERROR) << __func__ << debugMessage() << ": BluetoothAudioPortAidl is not in use";
        return false;
    }

    bool retval = false;
    {
        std::unique_lock lock(mCvMutex);
        base::ScopedLockAssertion lock_assertion(mCvMutex);
        LOG(VERBOSE) << __func__ << debugMessage() << ", state=" << mState
                     << ", mono=" << (mIsStereoToMono ? "true" : "false") << " request";
        if (mState == BluetoothStreamState::STARTED) {
            return true;  // nop, return
        } else if (mState == BluetoothStreamState::DISABLED) {
            return false; // avoid logspam when called from `transfer`
        } else if (mState == BluetoothStreamState::SUSPENDING ||
                   mState == BluetoothStreamState::STARTING) {
            /* If port is in transient state, give some time to respond */
            if (!condWaitState(&lock)) {
                LOG(ERROR) << __func__ << debugMessage() << ", state=" << mState << " failure";
                return false;
            }
        }
        if (mState == BluetoothStreamState::STARTED) {
            retval = true;
        } else if (mState == BluetoothStreamState::STANDBY) {
            if (!mSupportsLowLatency.has_value()) {
                std::vector<LatencyMode> latency_modes;
                getRecommendedLatencyModes(&latency_modes);
            }
            const bool low_latency = mSupportsLowLatency.value_or(false);
            mState = BluetoothStreamState::STARTING;
            lock.unlock();
            const bool startSuccess =
                    BluetoothAudioSessionControl::StartStream(mSessionType, low_latency);
            lock.lock();
            if (startSuccess && mState == BluetoothStreamState::STARTING) {
                retval = condWaitState(&lock);
            } else if (startSuccess && mState == BluetoothStreamState::STARTED) {
                retval = true;
            } else {
                // !startSuccess => no session instance
                const BluetoothStreamState newState = startSuccess ? BluetoothStreamState::STANDBY
                                                                   : BluetoothStreamState::DISABLED;
                LOG(ERROR) << __func__ << debugMessage() << ", startSuccess=" << startSuccess
                           << ", state=" << mState << " -> " << newState;
                mState = newState;
            }
        }
        if (retval) {
            LOG(INFO) << __func__ << debugMessage() << ", state=" << mState
                      << ", mono=" << (mIsStereoToMono ? "true" : "false") << " done";
        } else {
            LOG(ERROR) << __func__ << debugMessage() << ", state=" << mState << " failure";
        }
    }
    return retval;  // false if any failure like timeout
}

bool BluetoothAudioPortAidl::suspend() {
    if (!inUse()) {
        LOG(ERROR) << __func__ << debugMessage() << ": BluetoothAudioPortAidl is not in use";
        return false;
    }

    bool retval = false;
    {
        std::unique_lock lock(mCvMutex);
        base::ScopedLockAssertion lock_assertion(mCvMutex);
        LOG(VERBOSE) << __func__ << debugMessage() << ", state=" << mState << " request";
        if (mState == BluetoothStreamState::STANDBY) {
            return true;  // nop, return
        } else if (mState == BluetoothStreamState::SUSPENDING ||
                   mState == BluetoothStreamState::STARTING) {
            /* If port is in transient state, give some time to respond */
            if (!condWaitState(&lock)) {
                LOG(ERROR) << __func__ << debugMessage() << ", state=" << mState << " failure";
                return false;
            }
        }
        if (mState == BluetoothStreamState::STANDBY) {
            retval = true;
        } else if (mState == BluetoothStreamState::STARTED) {
            mState = BluetoothStreamState::SUSPENDING;
            lock.unlock();
            const bool suspendSuccess = BluetoothAudioSessionControl::SuspendStream(mSessionType);
            lock.lock();
            if (suspendSuccess && mState == BluetoothStreamState::SUSPENDING) {
                retval = condWaitState(&lock);
            } else if (suspendSuccess && mState == BluetoothStreamState::STANDBY) {
                retval = true;
            } else {
                LOG(ERROR) << __func__ << debugMessage() << ", suspendSuccess=" << suspendSuccess
                           << ", state=" << mState << " -> DISABLED";
                mState = BluetoothStreamState::DISABLED;
            }
        }
        if (retval) {
            LOG(INFO) << __func__ << debugMessage() << ", state=" << mState << " done";
        } else {
            LOG(ERROR) << __func__ << debugMessage() << ", state=" << mState << " failure";
        }
    }
    return retval;  // false if any failure like timeout
}

void BluetoothAudioPortAidl::stop() {
    if (!inUse()) {
        LOG(ERROR) << __func__ << debugMessage() << ": BluetoothAudioPortAidl is not in use";
        return;
    }
    std::lock_guard guard(mCvMutex);
    BluetoothStreamState previous_state = mState;
    LOG(VERBOSE) << __func__ << debugMessage() << ", state=" << mState << " request";
    if (mState != BluetoothStreamState::DISABLED) {
        BluetoothAudioSessionControl::StopStream(mSessionType);
        mState = BluetoothStreamState::DISABLED;
        LOG(INFO) << __func__ << debugMessage() << ", " << previous_state << " -> " << mState;
    }
}

size_t BluetoothAudioPortAidlOut::writeData(const void* buffer, size_t bytes) const {
    if (!buffer) {
        LOG(ERROR) << __func__ << debugMessage() << ": bad input arg";
        return 0;
    }

    if (!inUse()) {
        LOG(ERROR) << __func__ << debugMessage() << ": BluetoothAudioPortAidl is not in use";
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

bool BluetoothAudioPortAidlOut::setLatencyMode(
        ::aidl::android::hardware::bluetooth::audio::LatencyMode latency_mode) {
    if (!inUse()) {
        LOG(ERROR) << __func__ << debugMessage() << ": BluetoothAudioPortAidl is not in use";
        return false;
    }
    LOG(INFO) << __func__ << debugMessage() << ": " << toString(latency_mode);
    BluetoothAudioSessionControl::SetLatencyMode(mSessionType, latency_mode);
    return true;
}

size_t BluetoothAudioPortAidlIn::readData(void* buffer, size_t bytes) const {
    if (!buffer) {
        LOG(ERROR) << __func__ << debugMessage() << ": bad input arg";
        return 0;
    }

    if (!inUse()) {
        LOG(ERROR) << __func__ << debugMessage() << ": BluetoothAudioPortAidl is not in use";
        return 0;
    }

    return BluetoothAudioSessionControl::InReadPcmData(mSessionType, buffer, bytes);
}

bool BluetoothAudioPortAidl::getPresentationPosition(
        PresentationPosition& presentation_position) const {
    if (!inUse()) {
        LOG(ERROR) << __func__ << debugMessage() << ": BluetoothAudioPortAidl is not in use";
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
        LOG(ERROR) << __func__ << debugMessage() << ": BluetoothAudioPortAidl is not in use";
        return false;
    }
    LOG(DEBUG) << __func__ << debugMessage() << ", state=" << getState() << ", "
               << source_metadata.tracks.size() << " track(s)";
    if (source_metadata.tracks.size() == 0) return true;
    return BluetoothAudioSessionControl::UpdateSourceMetadata(mSessionType, source_metadata);
}

bool BluetoothAudioPortAidl::updateSinkMetadata(const SinkMetadata& sink_metadata) const {
    if (!inUse()) {
        LOG(ERROR) << __func__ << debugMessage() << ": BluetoothAudioPortAidl is not in use";
        return false;
    }
    LOG(DEBUG) << __func__ << debugMessage() << ", state=" << getState() << ", "
               << sink_metadata.tracks.size() << " track(s)";
    if (sink_metadata.tracks.size() == 0) return true;
    return BluetoothAudioSessionControl::UpdateSinkMetadata(mSessionType, sink_metadata);
}

BluetoothStreamState BluetoothAudioPortAidl::getState() const {
    std::lock_guard guard(mCvMutex);
    return mState;
}

bool BluetoothAudioPortAidl::setState(BluetoothStreamState state) {
    if (!inUse()) {
        LOG(ERROR) << __func__ << debugMessage() << ": BluetoothAudioPortAidl is not in use";
        return false;
    }
    std::lock_guard guard(mCvMutex);
    LOG(INFO) << __func__ << debugMessage() << ": " << mState << " -> " << state;
    mState = state;
    return true;
}

void BluetoothAudioPortAidl::setCallbacks(
        const std::shared_ptr<BluetoothAudioPortCallbacks>& callbacks) {
    std::lock_guard l(mCvMutex);
    mCallbacks = callbacks;
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

std::string BluetoothAudioPortAidl::getSessionNameForDebug() const {
    return toString(mSessionType);
}

}  // namespace android::bluetooth::audio::aidl
