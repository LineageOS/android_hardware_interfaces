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

#define LOG_TAG "BcRadioDef.tuner"
#define LOG_NDEBUG 0

#include "TunerSession.h"

#include "BroadcastRadio.h"

#include <broadcastradio-utils-2x/Utils.h>
#include <log/log.h>

namespace android {
namespace hardware {
namespace broadcastradio {
namespace V2_0 {
namespace implementation {

using namespace std::chrono_literals;

using utils::tunesTo;

using std::lock_guard;
using std::move;
using std::mutex;
using std::sort;
using std::vector;

namespace delay {

static constexpr auto seek = 200ms;
static constexpr auto step = 100ms;
static constexpr auto tune = 150ms;
static constexpr auto list = 1s;

}  // namespace delay

TunerSession::TunerSession(BroadcastRadio& module, const sp<ITunerCallback>& callback)
    : mCallback(callback), mModule(module) {
    auto&& ranges = module.getAmFmConfig().ranges;
    if (ranges.size() > 0) {
        tuneInternalLocked(utils::make_selector_amfm(ranges[0].lowerBound));
    }
}

// makes ProgramInfo that points to no program
static ProgramInfo makeDummyProgramInfo(const ProgramSelector& selector) {
    ProgramInfo info = {};
    info.selector = selector;
    info.logicallyTunedTo = utils::make_identifier(
        IdentifierType::AMFM_FREQUENCY, utils::getId(selector, IdentifierType::AMFM_FREQUENCY));
    info.physicallyTunedTo = info.logicallyTunedTo;
    return info;
}

void TunerSession::tuneInternalLocked(const ProgramSelector& sel) {
    ALOGV("%s(%s)", __func__, toString(sel).c_str());

    VirtualProgram virtualProgram;
    ProgramInfo programInfo;
    if (virtualRadio().getProgram(sel, virtualProgram)) {
        mCurrentProgram = virtualProgram.selector;
        programInfo = virtualProgram;
    } else {
        mCurrentProgram = sel;
        programInfo = makeDummyProgramInfo(sel);
    }
    mIsTuneCompleted = true;

    mCallback->onCurrentProgramInfoChanged(programInfo);
}

const BroadcastRadio& TunerSession::module() const {
    return mModule.get();
}

const VirtualRadio& TunerSession::virtualRadio() const {
    return module().mVirtualRadio;
}

Return<Result> TunerSession::tune(const ProgramSelector& sel) {
    ALOGV("%s(%s)", __func__, toString(sel).c_str());
    lock_guard<mutex> lk(mMut);
    if (mIsClosed) return Result::INVALID_STATE;

    if (!utils::isSupported(module().mProperties, sel)) {
        ALOGW("Selector not supported");
        return Result::NOT_SUPPORTED;
    }

    if (!utils::isValid(sel)) {
        ALOGE("ProgramSelector is not valid");
        return Result::INVALID_ARGUMENTS;
    }

    cancelLocked();

    mIsTuneCompleted = false;
    auto task = [this, sel]() {
        lock_guard<mutex> lk(mMut);
        tuneInternalLocked(sel);
    };
    mThread.schedule(task, delay::tune);

    return Result::OK;
}

Return<Result> TunerSession::scan(bool directionUp, bool /* skipSubChannel */) {
    ALOGV("%s", __func__);
    lock_guard<mutex> lk(mMut);
    if (mIsClosed) return Result::INVALID_STATE;

    cancelLocked();

    auto list = virtualRadio().getProgramList();

    if (list.empty()) {
        mIsTuneCompleted = false;
        auto task = [this, directionUp]() {
            ALOGI("Performing failed seek up=%d", directionUp);

            mCallback->onTuneFailed(Result::TIMEOUT, {});
        };
        mThread.schedule(task, delay::seek);

        return Result::OK;
    }

    // Not optimal (O(sort) instead of O(n)), but not a big deal here;
    // also, it's likely that list is already sorted (so O(n) anyway).
    sort(list.begin(), list.end());
    auto current = mCurrentProgram;
    auto found = lower_bound(list.begin(), list.end(), VirtualProgram({current}));
    if (directionUp) {
        if (found < list.end() - 1) {
            if (tunesTo(current, found->selector)) found++;
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
    auto task = [this, tuneTo, directionUp]() {
        ALOGI("Performing seek up=%d", directionUp);

        lock_guard<mutex> lk(mMut);
        tuneInternalLocked(tuneTo);
    };
    mThread.schedule(task, delay::seek);

    return Result::OK;
}

Return<Result> TunerSession::step(bool directionUp) {
    ALOGV("%s", __func__);
    lock_guard<mutex> lk(mMut);
    if (mIsClosed) return Result::INVALID_STATE;

    cancelLocked();

    if (!utils::hasId(mCurrentProgram, IdentifierType::AMFM_FREQUENCY)) {
        ALOGE("Can't step in anything else than AM/FM");
        return Result::NOT_SUPPORTED;
    }

    auto stepTo = utils::getId(mCurrentProgram, IdentifierType::AMFM_FREQUENCY);
    auto range = getAmFmRangeLocked();
    if (!range) {
        ALOGE("Can't find current band");
        return Result::INTERNAL_ERROR;
    }

    if (directionUp) {
        stepTo += range->spacing;
    } else {
        stepTo -= range->spacing;
    }
    if (stepTo > range->upperBound) stepTo = range->lowerBound;
    if (stepTo < range->lowerBound) stepTo = range->upperBound;

    mIsTuneCompleted = false;
    auto task = [this, stepTo]() {
        ALOGI("Performing step to %s", std::to_string(stepTo).c_str());

        lock_guard<mutex> lk(mMut);

        tuneInternalLocked(utils::make_selector_amfm(stepTo));
    };
    mThread.schedule(task, delay::step);

    return Result::OK;
}

void TunerSession::cancelLocked() {
    ALOGV("%s", __func__);

    mThread.cancelAll();
    if (utils::getType(mCurrentProgram.primaryId) != IdentifierType::INVALID) {
        mIsTuneCompleted = true;
    }
}

Return<void> TunerSession::cancel() {
    ALOGV("%s", __func__);
    lock_guard<mutex> lk(mMut);
    if (mIsClosed) return {};

    cancelLocked();

    return {};
}

Return<Result> TunerSession::startProgramListUpdates(const ProgramFilter& filter) {
    ALOGV("%s(%s)", __func__, toString(filter).c_str());
    lock_guard<mutex> lk(mMut);
    if (mIsClosed) return Result::INVALID_STATE;

    auto list = virtualRadio().getProgramList();
    vector<VirtualProgram> filteredList;
    auto filterCb = [&filter](const VirtualProgram& program) {
        return utils::satisfies(filter, program.selector);
    };
    std::copy_if(list.begin(), list.end(), std::back_inserter(filteredList), filterCb);

    auto task = [this, list]() {
        lock_guard<mutex> lk(mMut);

        ProgramListChunk chunk = {};
        chunk.purge = true;
        chunk.complete = true;
        chunk.modified = hidl_vec<ProgramInfo>(list.begin(), list.end());

        mCallback->onProgramListUpdated(chunk);
    };
    mThread.schedule(task, delay::list);

    return Result::OK;
}

Return<void> TunerSession::stopProgramListUpdates() {
    ALOGV("%s", __func__);
    return {};
}

Return<void> TunerSession::isConfigFlagSet(ConfigFlag flag, isConfigFlagSet_cb _hidl_cb) {
    ALOGV("%s(%s)", __func__, toString(flag).c_str());

    _hidl_cb(Result::NOT_SUPPORTED, false);
    return {};
}

Return<Result> TunerSession::setConfigFlag(ConfigFlag flag, bool value) {
    ALOGV("%s(%s, %d)", __func__, toString(flag).c_str(), value);

    return Result::NOT_SUPPORTED;
}

Return<void> TunerSession::setParameters(const hidl_vec<VendorKeyValue>& /* parameters */,
                                         setParameters_cb _hidl_cb) {
    ALOGV("%s", __func__);

    _hidl_cb({});
    return {};
}

Return<void> TunerSession::getParameters(const hidl_vec<hidl_string>& /* keys */,
                                         getParameters_cb _hidl_cb) {
    ALOGV("%s", __func__);

    _hidl_cb({});
    return {};
}

Return<void> TunerSession::close() {
    ALOGV("%s", __func__);
    lock_guard<mutex> lk(mMut);
    if (mIsClosed) return {};

    mIsClosed = true;
    mThread.cancelAll();
    return {};
}

std::optional<AmFmBandRange> TunerSession::getAmFmRangeLocked() const {
    if (!mIsTuneCompleted) {
        ALOGW("tune operation in process");
        return {};
    }
    if (!utils::hasId(mCurrentProgram, IdentifierType::AMFM_FREQUENCY)) return {};

    auto freq = utils::getId(mCurrentProgram, IdentifierType::AMFM_FREQUENCY);
    for (auto&& range : module().getAmFmConfig().ranges) {
        if (range.lowerBound <= freq && range.upperBound >= freq) return range;
    }

    return {};
}

}  // namespace implementation
}  // namespace V2_0
}  // namespace broadcastradio
}  // namespace hardware
}  // namespace android
