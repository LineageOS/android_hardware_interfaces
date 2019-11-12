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

#define LOG_TAG "VibratorService"

#include <log/log.h>

#include "Vibrator.h"

namespace android {
namespace hardware {
namespace vibrator {
namespace V1_3 {
namespace implementation {

static constexpr uint32_t MS_PER_S = 1000;
static constexpr uint32_t NS_PER_MS = 1000000;

Vibrator::Vibrator() {
    sigevent se{};
    se.sigev_notify = SIGEV_THREAD;
    se.sigev_value.sival_ptr = this;
    se.sigev_notify_function = timerCallback;
    se.sigev_notify_attributes = nullptr;

    if (timer_create(CLOCK_REALTIME, &se, &mTimer) < 0) {
        ALOGE("Can not create timer!%s", strerror(errno));
    }
}

// Methods from ::android::hardware::vibrator::V1_0::IVibrator follow.

Return<Status> Vibrator::on(uint32_t timeoutMs) {
    return activate(timeoutMs);
}

Return<Status> Vibrator::off() {
    return activate(0);
}

Return<bool> Vibrator::supportsAmplitudeControl() {
    return true;
}

Return<Status> Vibrator::setAmplitude(uint8_t amplitude) {
    if (!amplitude) {
        return Status::BAD_VALUE;
    }
    ALOGI("Amplitude: %u -> %u\n", mAmplitude, amplitude);
    mAmplitude = amplitude;
    return Status::OK;
}

Return<void> Vibrator::perform(V1_0::Effect effect, EffectStrength strength, perform_cb _hidl_cb) {
    return perform<decltype(effect)>(effect, strength, _hidl_cb);
}

// Methods from ::android::hardware::vibrator::V1_1::IVibrator follow.

Return<void> Vibrator::perform_1_1(V1_1::Effect_1_1 effect, EffectStrength strength,
                                   perform_cb _hidl_cb) {
    return perform<decltype(effect)>(effect, strength, _hidl_cb);
}

// Methods from ::android::hardware::vibrator::V1_2::IVibrator follow.

Return<void> Vibrator::perform_1_2(V1_2::Effect effect, EffectStrength strength,
                                   perform_cb _hidl_cb) {
    return perform<decltype(effect)>(effect, strength, _hidl_cb);
}

// Methods from ::android::hardware::vibrator::V1_3::IVibrator follow.

Return<bool> Vibrator::supportsExternalControl() {
    return true;
}

Return<Status> Vibrator::setExternalControl(bool enabled) {
    if (mEnabled) {
        ALOGW("Setting external control while the vibrator is enabled is unsupported!");
        return Status::UNSUPPORTED_OPERATION;
    } else {
        ALOGI("ExternalControl: %s -> %s\n", mExternalControl ? "true" : "false",
              enabled ? "true" : "false");
        mExternalControl = enabled;
        return Status::OK;
    }
}

Return<void> Vibrator::perform_1_3(Effect effect, EffectStrength strength, perform_cb _hidl_cb) {
    return perform<decltype(effect)>(effect, strength, _hidl_cb);
}

// Private methods follow.

Return<void> Vibrator::perform(Effect effect, EffectStrength strength, perform_cb _hidl_cb) {
    uint8_t amplitude;
    uint32_t ms;
    Status status = Status::OK;

    ALOGI("Perform: Effect %s\n", effectToName(effect).c_str());

    amplitude = strengthToAmplitude(strength, &status);
    if (status != Status::OK) {
        _hidl_cb(status, 0);
        return Void();
    }
    setAmplitude(amplitude);

    ms = effectToMs(effect, &status);
    if (status != Status::OK) {
        _hidl_cb(status, 0);
        return Void();
    }
    status = activate(ms);

    _hidl_cb(status, ms);

    return Void();
}

template <typename T>
Return<void> Vibrator::perform(T effect, EffectStrength strength, perform_cb _hidl_cb) {
    auto validRange = hidl_enum_range<T>();
    if (effect < *validRange.begin() || effect > *std::prev(validRange.end())) {
        _hidl_cb(Status::UNSUPPORTED_OPERATION, 0);
        return Void();
    }
    return perform(static_cast<Effect>(effect), strength, _hidl_cb);
}

Status Vibrator::enable(bool enabled) {
    if (mExternalControl) {
        ALOGW("Enabling/disabling while the vibrator is externally controlled is unsupported!");
        return Status::UNSUPPORTED_OPERATION;
    } else {
        ALOGI("Enabled: %s -> %s\n", mEnabled ? "true" : "false", enabled ? "true" : "false");
        mEnabled = enabled;
        return Status::OK;
    }
}

Status Vibrator::activate(uint32_t ms) {
    std::lock_guard<std::mutex> lock{mMutex};
    Status status = Status::OK;

    if (ms > 0) {
        status = enable(true);
        if (status != Status::OK) {
            return status;
        }
    }

    itimerspec ts{};
    ts.it_value.tv_sec = ms / MS_PER_S;
    ts.it_value.tv_nsec = ms % MS_PER_S * NS_PER_MS;

    if (timer_settime(mTimer, 0, &ts, nullptr) < 0) {
        ALOGE("Can not set timer!");
        status = Status::UNKNOWN_ERROR;
    }

    if ((status != Status::OK) || !ms) {
        Status _status;

        _status = enable(false);

        if (status == Status::OK) {
            status = _status;
        }
    }

    return status;
}

void Vibrator::timeout() {
    std::lock_guard<std::mutex> lock{mMutex};
    itimerspec ts{};

    if (timer_gettime(mTimer, &ts) < 0) {
        ALOGE("Can not read timer!");
    }

    if (ts.it_value.tv_sec == 0 && ts.it_value.tv_nsec == 0) {
        enable(false);
    }
}

void Vibrator::timerCallback(union sigval sigval) {
    static_cast<Vibrator*>(sigval.sival_ptr)->timeout();
}

const std::string Vibrator::effectToName(Effect effect) {
    return toString(effect);
}

uint32_t Vibrator::effectToMs(Effect effect, Status* status) {
    switch (effect) {
        case Effect::CLICK:
            return 10;
        case Effect::DOUBLE_CLICK:
            return 15;
        case Effect::TICK:
        case Effect::TEXTURE_TICK:
            return 5;
        case Effect::THUD:
            return 5;
        case Effect::POP:
            return 5;
        case Effect::HEAVY_CLICK:
            return 10;
        case Effect::RINGTONE_1:
            return 30000;
        case Effect::RINGTONE_2:
            return 30000;
        case Effect::RINGTONE_3:
            return 30000;
        case Effect::RINGTONE_4:
            return 30000;
        case Effect::RINGTONE_5:
            return 30000;
        case Effect::RINGTONE_6:
            return 30000;
        case Effect::RINGTONE_7:
            return 30000;
        case Effect::RINGTONE_8:
            return 30000;
        case Effect::RINGTONE_9:
            return 30000;
        case Effect::RINGTONE_10:
            return 30000;
        case Effect::RINGTONE_11:
            return 30000;
        case Effect::RINGTONE_12:
            return 30000;
        case Effect::RINGTONE_13:
            return 30000;
        case Effect::RINGTONE_14:
            return 30000;
        case Effect::RINGTONE_15:
            return 30000;
    }
    *status = Status::UNSUPPORTED_OPERATION;
    return 0;
}

uint8_t Vibrator::strengthToAmplitude(EffectStrength strength, Status* status) {
    switch (strength) {
        case EffectStrength::LIGHT:
            return 128;
        case EffectStrength::MEDIUM:
            return 192;
        case EffectStrength::STRONG:
            return 255;
    }
    *status = Status::UNSUPPORTED_OPERATION;
    return 0;
}

}  // namespace implementation
}  // namespace V1_3
}  // namespace vibrator
}  // namespace hardware
}  // namespace android
