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

#include "BroadcastRadio.h"
#include <broadcastradio-utils-aidl/Utils.h>
#include "resources.h"

#include <aidl/android/hardware/broadcastradio/IdentifierType.h>
#include <aidl/android/hardware/broadcastradio/Result.h>

#include <android-base/logging.h>

namespace aidl::android::hardware::broadcastradio {

using ::aidl::android::hardware::broadcastradio::utils::resultToInt;
using ::aidl::android::hardware::broadcastradio::utils::tunesTo;
using ::ndk::ScopedAStatus;
using ::std::literals::chrono_literals::operator""ms;
using ::std::literals::chrono_literals::operator""s;
using ::std::lock_guard;
using ::std::mutex;
using ::std::string;
using ::std::vector;

namespace {

inline constexpr std::chrono::milliseconds kSeekDelayTimeMs = 200ms;
inline constexpr std::chrono::milliseconds kStepDelayTimeMs = 100ms;
inline constexpr std::chrono::milliseconds kTuneDelayTimeMs = 150ms;
inline constexpr std::chrono::seconds kListDelayTimeS = 1s;

// clang-format off
const AmFmRegionConfig kDefaultAmFmConfig = {
        {
                {87500, 108000, 100, 100},  // FM
                {153, 282, 3, 9},           // AM LW
                {531, 1620, 9, 9},          // AM MW
                {1600, 30000, 1, 5},        // AM SW
        },
        AmFmRegionConfig::DEEMPHASIS_D50,
        AmFmRegionConfig::RDS};
// clang-format on

Properties initProperties(const VirtualRadio& virtualRadio) {
    Properties prop = {};

    prop.maker = "Android";
    prop.product = virtualRadio.getName();
    prop.supportedIdentifierTypes = vector<IdentifierType>({
            IdentifierType::AMFM_FREQUENCY_KHZ,
            IdentifierType::RDS_PI,
            IdentifierType::HD_STATION_ID_EXT,
            IdentifierType::DAB_SID_EXT,
    });
    prop.vendorInfo = vector<VendorKeyValue>({
            {"com.android.sample", "sample"},
    });

    return prop;
}

// Makes ProgramInfo that does not point to any particular program
ProgramInfo makeSampleProgramInfo(const ProgramSelector& selector) {
    ProgramInfo info = {};
    info.selector = selector;
    info.logicallyTunedTo =
            utils::makeIdentifier(IdentifierType::AMFM_FREQUENCY_KHZ,
                                  utils::getId(selector, IdentifierType::AMFM_FREQUENCY_KHZ));
    info.physicallyTunedTo = info.logicallyTunedTo;
    return info;
}

}  // namespace

BroadcastRadio::BroadcastRadio(const VirtualRadio& virtualRadio)
    : mVirtualRadio(virtualRadio),
      mAmFmConfig(kDefaultAmFmConfig),
      mProperties(initProperties(virtualRadio)) {
    const auto& ranges = kDefaultAmFmConfig.ranges;
    if (ranges.size() > 0) {
        ProgramSelector sel = utils::makeSelectorAmfm(ranges[0].lowerBound);
        VirtualProgram virtualProgram = {};
        if (mVirtualRadio.getProgram(sel, &virtualProgram)) {
            mCurrentProgram = virtualProgram.selector;
        } else {
            mCurrentProgram = sel;
        }
    }
}

BroadcastRadio::~BroadcastRadio() {
    mThread.reset();
}

ScopedAStatus BroadcastRadio::getAmFmRegionConfig(bool full, AmFmRegionConfig* returnConfigs) {
    if (full) {
        *returnConfigs = {};
        returnConfigs->ranges = vector<AmFmBandRange>({
                {65000, 108000, 10, 0},  // FM
                {150, 30000, 1, 0},      // AM
        });
        returnConfigs->fmDeemphasis =
                AmFmRegionConfig::DEEMPHASIS_D50 | AmFmRegionConfig::DEEMPHASIS_D75;
        returnConfigs->fmRds = AmFmRegionConfig::RDS | AmFmRegionConfig::RBDS;
        return ScopedAStatus::ok();
    }
    lock_guard<mutex> lk(mMutex);
    *returnConfigs = mAmFmConfig;
    return ScopedAStatus::ok();
}

ScopedAStatus BroadcastRadio::getDabRegionConfig(vector<DabTableEntry>* returnConfigs) {
    *returnConfigs = {
            {"5A", 174928},  {"7D", 194064},  {"8A", 195936},  {"8B", 197648},  {"9A", 202928},
            {"9B", 204640},  {"9C", 206352},  {"10B", 211648}, {"10C", 213360}, {"10D", 215072},
            {"11A", 216928}, {"11B", 218640}, {"11C", 220352}, {"11D", 222064}, {"12A", 223936},
            {"12B", 225648}, {"12C", 227360}, {"12D", 229072},
    };
    return ScopedAStatus::ok();
}

ScopedAStatus BroadcastRadio::getImage(int32_t id, vector<uint8_t>* returnImage) {
    LOG(DEBUG) << __func__ << ": fetching image " << std::hex << id;

    if (id == resources::kDemoPngId) {
        *returnImage = vector<uint8_t>(resources::kDemoPng, std::end(resources::kDemoPng));
        return ScopedAStatus::ok();
    }

    LOG(WARNING) << __func__ << ": image of id " << std::hex << id << " doesn't exist";
    *returnImage = {};
    return ScopedAStatus::ok();
}

ScopedAStatus BroadcastRadio::getProperties(Properties* returnProperties) {
    lock_guard<mutex> lk(mMutex);
    *returnProperties = mProperties;
    return ScopedAStatus::ok();
}

ProgramInfo BroadcastRadio::tuneInternalLocked(const ProgramSelector& sel) {
    LOG(DEBUG) << __func__ << ": tune (internal) to " << sel.toString();

    VirtualProgram virtualProgram = {};
    ProgramInfo programInfo;
    if (mVirtualRadio.getProgram(sel, &virtualProgram)) {
        mCurrentProgram = virtualProgram.selector;
        programInfo = virtualProgram;
    } else {
        mCurrentProgram = sel;
        programInfo = makeSampleProgramInfo(sel);
    }
    mIsTuneCompleted = true;

    return programInfo;
}

ScopedAStatus BroadcastRadio::setTunerCallback(const std::shared_ptr<ITunerCallback>& callback) {
    LOG(DEBUG) << __func__ << ": setTunerCallback";

    if (callback == nullptr) {
        return ScopedAStatus::fromServiceSpecificErrorWithMessage(
                resultToInt(Result::INVALID_ARGUMENTS), "cannot set tuner callback to null");
    }

    lock_guard<mutex> lk(mMutex);
    mCallback = callback;

    return ScopedAStatus::ok();
}

ScopedAStatus BroadcastRadio::unsetTunerCallback() {
    LOG(DEBUG) << __func__ << ": unsetTunerCallback";

    lock_guard<mutex> lk(mMutex);
    mCallback = nullptr;

    return ScopedAStatus::ok();
}

ScopedAStatus BroadcastRadio::tune(const ProgramSelector& program) {
    LOG(DEBUG) << __func__ << ": tune to " << program.toString() << "...";

    lock_guard<mutex> lk(mMutex);
    if (mCallback == nullptr) {
        LOG(ERROR) << __func__ << ": callback is not registered.";
        return ScopedAStatus::fromServiceSpecificErrorWithMessage(
                resultToInt(Result::INVALID_STATE), "callback is not registered");
    }

    if (!utils::isSupported(mProperties, program)) {
        LOG(WARNING) << __func__ << ": selector not supported: " << program.toString();
        return ScopedAStatus::fromServiceSpecificErrorWithMessage(
                resultToInt(Result::NOT_SUPPORTED), "selector is not supported");
    }

    if (!utils::isValid(program)) {
        LOG(ERROR) << __func__ << ": selector is not valid: " << program.toString();
        return ScopedAStatus::fromServiceSpecificErrorWithMessage(
                resultToInt(Result::INVALID_ARGUMENTS), "selector is not valid");
    }

    cancelLocked();

    mIsTuneCompleted = false;
    std::shared_ptr<ITunerCallback> callback = mCallback;
    auto task = [this, program, callback]() {
        ProgramInfo programInfo = {};
        {
            lock_guard<mutex> lk(mMutex);
            programInfo = tuneInternalLocked(program);
        }
        callback->onCurrentProgramInfoChanged(programInfo);
    };
    mThread->schedule(task, kTuneDelayTimeMs);

    return ScopedAStatus::ok();
}

ScopedAStatus BroadcastRadio::seek(bool directionUp, bool skipSubChannel) {
    LOG(DEBUG) << __func__ << ": seek " << (directionUp ? "up" : "down") << " with skipSubChannel? "
               << (skipSubChannel ? "yes" : "no") << "...";

    lock_guard<mutex> lk(mMutex);
    if (mCallback == nullptr) {
        LOG(ERROR) << __func__ << ": callback is not registered.";
        return ScopedAStatus::fromServiceSpecificErrorWithMessage(
                resultToInt(Result::INVALID_STATE), "callback is not registered");
    }

    cancelLocked();

    const auto& list = mVirtualRadio.getProgramList();
    std::shared_ptr<ITunerCallback> callback = mCallback;
    if (list.empty()) {
        mIsTuneCompleted = false;
        auto task = [callback]() {
            LOG(DEBUG) << "seek: program list is empty, seek couldn't stop";

            callback->onTuneFailed(Result::TIMEOUT, {});
        };
        mThread->schedule(task, kSeekDelayTimeMs);

        return ScopedAStatus::ok();
    }

    // The list is not sorted here since it has already stored in VirtualRadio.
    // If the list is not sorted in advance, it should be sorted here.
    const auto& current = mCurrentProgram;
    auto found = std::lower_bound(list.begin(), list.end(), VirtualProgram({current}));
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
    const ProgramSelector tuneTo = found->selector;

    mIsTuneCompleted = false;
    auto task = [this, tuneTo, callback]() {
        ProgramInfo programInfo = {};
        {
            lock_guard<mutex> lk(mMutex);
            programInfo = tuneInternalLocked(tuneTo);
        }
        callback->onCurrentProgramInfoChanged(programInfo);
    };
    mThread->schedule(task, kSeekDelayTimeMs);

    return ScopedAStatus::ok();
}

ScopedAStatus BroadcastRadio::step(bool directionUp) {
    LOG(DEBUG) << __func__ << ": step " << (directionUp ? "up" : "down") << "...";

    lock_guard<mutex> lk(mMutex);
    if (mCallback == nullptr) {
        LOG(ERROR) << __func__ << ": callback is not registered.";
        return ScopedAStatus::fromServiceSpecificErrorWithMessage(
                resultToInt(Result::INVALID_STATE), "callback is not registered");
    }

    cancelLocked();

    if (!utils::hasId(mCurrentProgram, IdentifierType::AMFM_FREQUENCY_KHZ)) {
        LOG(WARNING) << __func__ << ": can't step in anything else than AM/FM";
        return ScopedAStatus::fromServiceSpecificErrorWithMessage(
                resultToInt(Result::NOT_SUPPORTED), "cannot step in anything else than AM/FM");
    }

    int64_t stepTo = utils::getId(mCurrentProgram, IdentifierType::AMFM_FREQUENCY_KHZ);
    std::optional<AmFmBandRange> range = getAmFmRangeLocked();
    if (!range) {
        LOG(ERROR) << __func__ << ": can't find current band or tune operation is in process";
        ScopedAStatus::fromServiceSpecificErrorWithMessage(
                resultToInt(Result::INTERNAL_ERROR),
                "can't find current band or tune operation is in process");
    }

    if (directionUp) {
        stepTo += range->spacing;
    } else {
        stepTo -= range->spacing;
    }
    if (stepTo > range->upperBound) {
        stepTo = range->lowerBound;
    }
    if (stepTo < range->lowerBound) {
        stepTo = range->upperBound;
    }

    mIsTuneCompleted = false;
    std::shared_ptr<ITunerCallback> callback = mCallback;
    auto task = [this, stepTo, callback]() {
        ProgramInfo programInfo;
        {
            lock_guard<mutex> lk(mMutex);
            programInfo = tuneInternalLocked(utils::makeSelectorAmfm(stepTo));
        }
        callback->onCurrentProgramInfoChanged(programInfo);
    };
    mThread->schedule(task, kStepDelayTimeMs);

    return ScopedAStatus::ok();
}

void BroadcastRadio::cancelLocked() {
    LOG(DEBUG) << __func__ << ": cancelling current operations...";

    mThread->cancelAll();
    if (mCurrentProgram.primaryId.type != IdentifierType::INVALID) {
        mIsTuneCompleted = true;
    }
}

ScopedAStatus BroadcastRadio::cancel() {
    LOG(DEBUG) << __func__ << ": cancel pending tune, seek and step...";

    lock_guard<mutex> lk(mMutex);
    cancelLocked();

    return ScopedAStatus::ok();
}

ScopedAStatus BroadcastRadio::startProgramListUpdates(const ProgramFilter& filter) {
    LOG(DEBUG) << __func__ << ": requested program list updates, filter = " << filter.toString()
               << "...";

    auto filterCb = [&filter](const VirtualProgram& program) {
        return utils::satisfies(filter, program.selector);
    };

    lock_guard<mutex> lk(mMutex);

    const auto& list = mVirtualRadio.getProgramList();
    vector<VirtualProgram> filteredList;
    std::copy_if(list.begin(), list.end(), std::back_inserter(filteredList), filterCb);

    auto task = [this, filteredList]() {
        std::shared_ptr<ITunerCallback> callback;
        {
            lock_guard<mutex> lk(mMutex);
            if (mCallback == nullptr) {
                LOG(WARNING) << "Callback is null when updating program List";
                return;
            }
            callback = mCallback;
        }

        ProgramListChunk chunk = {};
        chunk.purge = true;
        chunk.complete = true;
        chunk.modified = vector<ProgramInfo>(filteredList.begin(), filteredList.end());

        callback->onProgramListUpdated(chunk);
    };
    mThread->schedule(task, kListDelayTimeS);

    return ScopedAStatus::ok();
}

ScopedAStatus BroadcastRadio::stopProgramListUpdates() {
    LOG(DEBUG) << __func__ << ": requested program list updates to stop...";
    // TODO(b/243681584) Implement stop program list updates method
    return ScopedAStatus::ok();
}

ScopedAStatus BroadcastRadio::isConfigFlagSet(ConfigFlag flag, [[maybe_unused]] bool* returnIsSet) {
    LOG(DEBUG) << __func__ << ": flag = " << toString(flag);

    LOG(INFO) << __func__ << ": getting ConfigFlag is not supported";
    return ScopedAStatus::fromServiceSpecificErrorWithMessage(
            resultToInt(Result::NOT_SUPPORTED), "getting ConfigFlag is not supported");
}

ScopedAStatus BroadcastRadio::setConfigFlag(ConfigFlag flag, bool value) {
    LOG(DEBUG) << __func__ << ": flag = " << toString(flag) << ", value = " << value;

    LOG(INFO) << __func__ << ": setting ConfigFlag is not supported";
    return ScopedAStatus::fromServiceSpecificErrorWithMessage(
            resultToInt(Result::NOT_SUPPORTED), "setting ConfigFlag is not supported");
}

ScopedAStatus BroadcastRadio::setParameters(
        [[maybe_unused]] const vector<VendorKeyValue>& parameters,
        vector<VendorKeyValue>* returnParameters) {
    // TODO(b/243682330) Support vendor parameter functionality
    *returnParameters = {};
    return ScopedAStatus::ok();
}

ScopedAStatus BroadcastRadio::getParameters([[maybe_unused]] const vector<string>& keys,
                                            vector<VendorKeyValue>* returnParameters) {
    // TODO(b/243682330) Support vendor parameter functionality
    *returnParameters = {};
    return ScopedAStatus::ok();
}

std::optional<AmFmBandRange> BroadcastRadio::getAmFmRangeLocked() const {
    if (!mIsTuneCompleted) {
        LOG(WARNING) << __func__ << ": tune operation is in process";
        return {};
    }
    if (!utils::hasId(mCurrentProgram, IdentifierType::AMFM_FREQUENCY_KHZ)) {
        LOG(WARNING) << __func__ << ": current program does not has AMFM_FREQUENCY_KHZ identifier";
        return {};
    }

    int64_t freq = utils::getId(mCurrentProgram, IdentifierType::AMFM_FREQUENCY_KHZ);
    for (const auto& range : mAmFmConfig.ranges) {
        if (range.lowerBound <= freq && range.upperBound >= freq) {
            return range;
        }
    }

    return {};
}

ScopedAStatus BroadcastRadio::registerAnnouncementListener(
        [[maybe_unused]] const std::shared_ptr<IAnnouncementListener>& listener,
        const vector<AnnouncementType>& enabled, std::shared_ptr<ICloseHandle>* returnCloseHandle) {
    LOG(DEBUG) << __func__ << ": registering announcement listener for "
               << utils::vectorToString(enabled);

    // TODO(b/243683842) Support announcement listener
    *returnCloseHandle = nullptr;
    LOG(INFO) << __func__ << ": registering announcementListener is not supported";
    return ScopedAStatus::fromServiceSpecificErrorWithMessage(
            resultToInt(Result::NOT_SUPPORTED),
            "registering announcementListener is not supported");
}

}  // namespace aidl::android::hardware::broadcastradio
