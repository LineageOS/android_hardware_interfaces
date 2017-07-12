/*
 * Copyright (C) 2017 The Android Open Source Project
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

#define LOG_TAG "BroadcastRadioDefault.tuner"
#define LOG_NDEBUG 0

#include "BroadcastRadio.h"
#include "Tuner.h"

#include <Utils.h>
#include <log/log.h>

namespace android {
namespace hardware {
namespace broadcastradio {
namespace V1_1 {
namespace implementation {

using namespace std::chrono_literals;

using V1_0::Band;
using V1_0::BandConfig;
using V1_0::Direction;

using std::chrono::milliseconds;
using std::lock_guard;
using std::move;
using std::mutex;
using std::sort;
using std::vector;

const struct {
    milliseconds config = 50ms;
    milliseconds scan = 200ms;
    milliseconds step = 100ms;
    milliseconds tune = 150ms;
} gDefaultDelay;

Tuner::Tuner(const sp<V1_0::ITunerCallback>& callback)
    : mCallback(callback),
      mCallback1_1(ITunerCallback::castFrom(callback).withDefault(nullptr)),
      mVirtualFm(make_fm_radio()) {}

void Tuner::forceClose() {
    lock_guard<mutex> lk(mMut);
    mIsClosed = true;
    mThread.cancelAll();
}

Return<Result> Tuner::setConfiguration(const BandConfig& config) {
    ALOGV("%s", __func__);

    if (config.lowerLimit >= config.upperLimit) return Result::INVALID_ARGUMENTS;

    auto task = [this, config]() {
        ALOGI("Setting AM/FM config");
        lock_guard<mutex> lk(mMut);

        mAmfmConfig = move(config);
        mAmfmConfig.antennaConnected = true;
        mCurrentProgram = utils::make_selector(mAmfmConfig.type, mAmfmConfig.lowerLimit);

        mIsAmfmConfigSet = true;
        mCallback->configChange(Result::OK, mAmfmConfig);
    };
    mThread.schedule(task, gDefaultDelay.config);

    return Result::OK;
}

Return<void> Tuner::getConfiguration(getConfiguration_cb _hidl_cb) {
    ALOGV("%s", __func__);

    lock_guard<mutex> lk(mMut);
    if (mIsAmfmConfigSet) {
        _hidl_cb(Result::OK, mAmfmConfig);
    } else {
        _hidl_cb(Result::NOT_INITIALIZED, {});
    }
    return Void();
}

// makes ProgramInfo that points to no program
static ProgramInfo makeDummyProgramInfo(const ProgramSelector& selector) {
    ProgramInfo info11 = {};
    auto& info10 = info11.base;

    utils::getLegacyChannel(selector, info10.channel, info10.subChannel);
    info11.selector = selector;
    info11.flags |= ProgramInfoFlags::MUTED;

    return info11;
}

bool Tuner::isFmLocked() {
    if (!utils::isAmFm(utils::getType(mCurrentProgram))) return false;
    return mAmfmConfig.type == Band::FM_HD || mAmfmConfig.type == Band::FM;
}

void Tuner::tuneInternalLocked(const ProgramSelector& sel) {
    VirtualRadio* virtualRadio = nullptr;
    if (isFmLocked()) {
        virtualRadio = &mVirtualFm;
    }

    VirtualProgram virtualProgram;
    if (virtualRadio != nullptr && virtualRadio->getProgram(sel, virtualProgram)) {
        mCurrentProgram = virtualProgram.selector;
        mCurrentProgramInfo = static_cast<ProgramInfo>(virtualProgram);
    } else {
        mCurrentProgram = sel;
        mCurrentProgramInfo = makeDummyProgramInfo(sel);
    }
    mIsTuneCompleted = true;

    mCallback->tuneComplete(Result::OK, mCurrentProgramInfo.base);
    if (mCallback1_1 != nullptr) {
        mCallback1_1->tuneComplete_1_1(Result::OK, mCurrentProgramInfo);
    }
}

Return<Result> Tuner::scan(Direction direction, bool skipSubChannel __unused) {
    ALOGV("%s", __func__);
    lock_guard<mutex> lk(mMut);
    vector<VirtualProgram> list;

    if (isFmLocked()) {
        list = mVirtualFm.getProgramList();
    }

    if (list.empty()) {
        mIsTuneCompleted = false;
        auto task = [this, direction]() {
            ALOGI("Performing failed scan %s", toString(direction).c_str());

            mCallback->tuneComplete(Result::TIMEOUT, {});
            if (mCallback1_1 != nullptr) {
                mCallback1_1->tuneComplete_1_1(Result::TIMEOUT, {});
            }
        };
        mThread.schedule(task, gDefaultDelay.scan);

        return Result::OK;
    }

    // Not optimal (O(sort) instead of O(n)), but not a big deal here;
    // also, it's likely that list is already sorted (so O(n) anyway).
    sort(list.begin(), list.end());
    auto current = mCurrentProgram;
    auto found = lower_bound(list.begin(), list.end(), VirtualProgram({current}));
    if (direction == Direction::UP) {
        if (found < list.end() - 1) {
            if (utils::tunesTo(current, found->selector)) found++;
        } else {
            found = list.begin();
        }
    } else {
        if (found > list.begin() && found != list.end()) {
            found--;
        } else {
            found = list.end() - 1;
        }
    }
    auto tuneTo = found->selector;

    mIsTuneCompleted = false;
    auto task = [this, tuneTo, direction]() {
        ALOGI("Performing scan %s", toString(direction).c_str());

        lock_guard<mutex> lk(mMut);
        tuneInternalLocked(tuneTo);
    };
    mThread.schedule(task, gDefaultDelay.scan);

    return Result::OK;
}

Return<Result> Tuner::step(Direction direction, bool skipSubChannel) {
    ALOGV("%s", __func__);
    ALOGW_IF(!skipSubChannel, "can't step to next frequency without ignoring subChannel");

    lock_guard<mutex> lk(mMut);

    if (!utils::isAmFm(utils::getType(mCurrentProgram))) {
        ALOGE("Can't step in anything else than AM/FM");
        return Result::NOT_INITIALIZED;
    }

    ALOGW_IF(!mIsAmfmConfigSet, "AM/FM config not set");
    if (!mIsAmfmConfigSet) return Result::INVALID_STATE;
    mIsTuneCompleted = false;

    auto task = [this, direction]() {
        ALOGI("Performing step %s", toString(direction).c_str());

        lock_guard<mutex> lk(mMut);

        auto current = utils::getId(mCurrentProgram, IdentifierType::AMFM_FREQUENCY, 0);

        if (direction == Direction::UP) {
            current += mAmfmConfig.spacings[0];
        } else {
            current -= mAmfmConfig.spacings[0];
        }

        if (current > mAmfmConfig.upperLimit) current = mAmfmConfig.lowerLimit;
        if (current < mAmfmConfig.lowerLimit) current = mAmfmConfig.upperLimit;

        tuneInternalLocked(utils::make_selector(mAmfmConfig.type, current));
    };
    mThread.schedule(task, gDefaultDelay.step);

    return Result::OK;
}

Return<Result> Tuner::tune(uint32_t channel, uint32_t subChannel) {
    ALOGV("%s(%d, %d)", __func__, channel, subChannel);
    Band band;
    {
        lock_guard<mutex> lk(mMut);
        band = mAmfmConfig.type;
    }
    return tune_1_1(utils::make_selector(band, channel, subChannel));
}

Return<Result> Tuner::tune_1_1(const ProgramSelector& sel) {
    ALOGV("%s(%s)", __func__, toString(sel).c_str());

    lock_guard<mutex> lk(mMut);

    if (utils::isAmFm(utils::getType(mCurrentProgram))) {
        ALOGW_IF(!mIsAmfmConfigSet, "AM/FM config not set");
        if (!mIsAmfmConfigSet) return Result::INVALID_STATE;

        auto freq = utils::getId(sel, IdentifierType::AMFM_FREQUENCY);
        if (freq < mAmfmConfig.lowerLimit || freq > mAmfmConfig.upperLimit) {
            return Result::INVALID_ARGUMENTS;
        }
    }

    mIsTuneCompleted = false;
    auto task = [this, sel]() {
        lock_guard<mutex> lk(mMut);
        tuneInternalLocked(sel);
    };
    mThread.schedule(task, gDefaultDelay.tune);

    return Result::OK;
}

Return<Result> Tuner::cancel() {
    ALOGV("%s", __func__);
    mThread.cancelAll();
    return Result::OK;
}

Return<void> Tuner::getProgramInformation(getProgramInformation_cb _hidl_cb) {
    ALOGV("%s", __func__);
    return getProgramInformation_1_1([&](Result result, const ProgramInfo& info) {
        _hidl_cb(result, info.base);
    });
}

Return<void> Tuner::getProgramInformation_1_1(getProgramInformation_1_1_cb _hidl_cb) {
    ALOGV("%s", __func__);

    lock_guard<mutex> lk(mMut);
    if (mIsTuneCompleted) {
        _hidl_cb(Result::OK, mCurrentProgramInfo);
    } else {
        _hidl_cb(Result::NOT_INITIALIZED, makeDummyProgramInfo(mCurrentProgram));
    }
    return Void();
}

Return<ProgramListResult> Tuner::startBackgroundScan() {
    ALOGV("%s", __func__);
    return ProgramListResult::UNAVAILABLE;
}

Return<void> Tuner::getProgramList(const hidl_string& filter __unused, getProgramList_cb _hidl_cb) {
    ALOGV("%s", __func__);
    lock_guard<mutex> lk(mMut);

    auto& virtualRadio = mVirtualFm;
    if (!isFmLocked()) {
        ALOGI("bands other than FM are not supported yet");
        _hidl_cb(ProgramListResult::OK, {});
        return Void();
    }

    auto list = virtualRadio.getProgramList();
    ALOGD("returning a list of %zu programs", list.size());
    _hidl_cb(ProgramListResult::OK, vector<ProgramInfo>(list.begin(), list.end()));
    return Void();
}

Return<void> Tuner::isAnalogForced(isAnalogForced_cb _hidl_cb) {
    ALOGV("%s", __func__);
    // TODO(b/36864090): implement
    _hidl_cb(Result::INVALID_STATE, false);
    return Void();
}

Return<Result> Tuner::setAnalogForced(bool isForced __unused) {
    ALOGV("%s", __func__);
    // TODO(b/36864090): implement
    return Result::INVALID_STATE;
}

}  // namespace implementation
}  // namespace V1_1
}  // namespace broadcastradio
}  // namespace hardware
}  // namespace android
