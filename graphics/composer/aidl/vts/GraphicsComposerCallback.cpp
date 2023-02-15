/**
 * Copyright (c) 2021, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "GraphicsComposerCallback.h"
#include <log/log_main.h>
#include <utils/Timers.h>

#pragma push_macro("LOG_TAG")
#undef LOG_TAG
#define LOG_TAG "GraphicsComposerCallback"

namespace aidl::android::hardware::graphics::composer3::vts {

void GraphicsComposerCallback::setVsyncAllowed(bool allowed) {
    std::scoped_lock lock(mMutex);
    mVsyncAllowed = allowed;
}

std::vector<int64_t> GraphicsComposerCallback::getDisplays() const {
    std::scoped_lock lock(mMutex);
    return mDisplays;
}

int32_t GraphicsComposerCallback::getInvalidHotplugCount() const {
    std::scoped_lock lock(mMutex);
    return mInvalidHotplugCount;
}

int32_t GraphicsComposerCallback::getInvalidRefreshCount() const {
    std::scoped_lock lock(mMutex);
    return mInvalidRefreshCount;
}

int32_t GraphicsComposerCallback::getInvalidVsyncCount() const {
    std::scoped_lock lock(mMutex);
    return mInvalidVsyncCount;
}

int32_t GraphicsComposerCallback::getInvalidVsyncPeriodChangeCount() const {
    std::scoped_lock lock(mMutex);
    return mInvalidVsyncPeriodChangeCount;
}

int32_t GraphicsComposerCallback::getInvalidSeamlessPossibleCount() const {
    std::scoped_lock lock(mMutex);
    return mInvalidSeamlessPossibleCount;
}

int32_t GraphicsComposerCallback::getVsyncIdleCount() const {
    std::scoped_lock lock(mMutex);
    return mVsyncIdleCount;
}

int64_t GraphicsComposerCallback::getVsyncIdleTime() const {
    std::scoped_lock lock(mMutex);
    return mVsyncIdleTime;
}

std::optional<VsyncPeriodChangeTimeline>
GraphicsComposerCallback::takeLastVsyncPeriodChangeTimeline() {
    std::scoped_lock lock(mMutex);

    std::optional<VsyncPeriodChangeTimeline> ret;
    ret.swap(mTimeline);

    return ret;
}

::ndk::ScopedAStatus GraphicsComposerCallback::onHotplug(int64_t in_display, bool in_connected) {
    std::scoped_lock lock(mMutex);

    const auto it = std::find(mDisplays.begin(), mDisplays.end(), in_display);
    if (in_connected) {
        if (it == mDisplays.end()) {
            mDisplays.push_back(in_display);
        } else {
            mInvalidHotplugCount++;
        }
    } else {
        if (it != mDisplays.end()) {
            mDisplays.erase(it);
        } else {
            mInvalidHotplugCount++;
        }
    }
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus GraphicsComposerCallback::onRefresh(int64_t in_display) {
    std::scoped_lock lock(mMutex);

    const auto it = std::find(mDisplays.begin(), mDisplays.end(), in_display);
    if (it == mDisplays.end()) {
        mInvalidRefreshCount++;
    }

    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus GraphicsComposerCallback::onVsync(int64_t in_display, int64_t in_timestamp,
                                                       int32_t in_vsyncPeriodNanos) {
    std::scoped_lock lock(mMutex);

    const auto it = std::find(mDisplays.begin(), mDisplays.end(), in_display);
    if (!mVsyncAllowed || it == mDisplays.end()) {
        mInvalidVsyncCount++;
    }

    ALOGV("%ld, %d", static_cast<long>(in_timestamp), in_vsyncPeriodNanos);

    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus GraphicsComposerCallback::onRefreshRateChangedDebug(
        const RefreshRateChangedDebugData&) {
    // TODO(b/202734676) Add implementation for Vts tests
    return ::ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

::ndk::ScopedAStatus GraphicsComposerCallback::onVsyncPeriodTimingChanged(
        int64_t in_display,
        const ::aidl::android::hardware::graphics::composer3::VsyncPeriodChangeTimeline&
                in_updatedTimeline) {
    std::scoped_lock lock(mMutex);

    const auto it = std::find(mDisplays.begin(), mDisplays.end(), in_display);
    if (it == mDisplays.end()) {
        mInvalidVsyncPeriodChangeCount++;
    }
    mTimeline = in_updatedTimeline;

    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus GraphicsComposerCallback::onSeamlessPossible(int64_t in_display) {
    std::scoped_lock lock(mMutex);

    const auto it = std::find(mDisplays.begin(), mDisplays.end(), in_display);
    if (it != mDisplays.end()) {
        mInvalidSeamlessPossibleCount++;
    }
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus GraphicsComposerCallback::onVsyncIdle(int64_t in_display) {
    std::scoped_lock lock(mMutex);

    const auto it = std::find(mDisplays.begin(), mDisplays.end(), in_display);
    if (it != mDisplays.end()) {
        mVsyncIdleCount++;
        mVsyncIdleTime = systemTime();
    }
    return ::ndk::ScopedAStatus::ok();
}

}  // namespace aidl::android::hardware::graphics::composer3::vts
