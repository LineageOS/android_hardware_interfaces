/*
 * Copyright 2020 The Android Open Source Project
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

#include <composer-vts/2.4/GraphicsComposerCallback.h>

namespace android::hardware::graphics::composer::V2_4::vts {

void GraphicsComposerCallback::setVsyncAllowed(bool allowed) {
    std::lock_guard<std::mutex> lock(mMutex);
    mVsyncAllowed = allowed;
}

std::vector<Display> GraphicsComposerCallback::getDisplays() const {
    std::lock_guard<std::mutex> lock(mMutex);
    return mDisplays;
}

int32_t GraphicsComposerCallback::getInvalidHotplugCount() const {
    std::lock_guard<std::mutex> lock(mMutex);
    return mInvalidHotplugCount;
}

int32_t GraphicsComposerCallback::getInvalidRefreshCount() const {
    std::lock_guard<std::mutex> lock(mMutex);
    return mInvalidRefreshCount;
}

int32_t GraphicsComposerCallback::getInvalidVsyncCount() const {
    std::lock_guard<std::mutex> lock(mMutex);
    return mInvalidVsyncCount;
}

int32_t GraphicsComposerCallback::getInvalidVsync_2_4Count() const {
    std::lock_guard<std::mutex> lock(mMutex);
    return mInvalidVsync_2_4Count;
}

int32_t GraphicsComposerCallback::getInvalidVsyncPeriodChangeCount() const {
    std::lock_guard<std::mutex> lock(mMutex);
    return mInvalidVsyncPeriodChangeCount;
}

int32_t GraphicsComposerCallback::getInvalidSeamlessPossibleCount() const {
    std::lock_guard<std::mutex> lock(mMutex);
    return mInvalidSeamlessPossibleCount;
}

std::optional<VsyncPeriodChangeTimeline>
GraphicsComposerCallback::takeLastVsyncPeriodChangeTimeline() {
    std::lock_guard<std::mutex> lock(mMutex);

    std::optional<VsyncPeriodChangeTimeline> ret;
    ret.swap(mTimeline);

    return ret;
}

Return<void> GraphicsComposerCallback::onHotplug(Display display, Connection connection) {
    std::lock_guard<std::mutex> lock(mMutex);

    auto it = std::find(mDisplays.begin(), mDisplays.end(), display);
    if (connection == Connection::CONNECTED) {
        if (it == mDisplays.end()) {
            mDisplays.push_back(display);
        } else {
            mInvalidHotplugCount++;
        }
    } else if (connection == Connection::DISCONNECTED) {
        if (it != mDisplays.end()) {
            mDisplays.erase(it);
        } else {
            mInvalidHotplugCount++;
        }
    }

    return Void();
}

Return<void> GraphicsComposerCallback::onRefresh(Display display) {
    std::lock_guard<std::mutex> lock(mMutex);

    auto it = std::find(mDisplays.begin(), mDisplays.end(), display);
    if (it == mDisplays.end()) {
        mInvalidRefreshCount++;
    }

    return Void();
}

Return<void> GraphicsComposerCallback::onVsync(Display, int64_t) {
    std::lock_guard<std::mutex> lock(mMutex);

    // On composer 2.4, onVsync is not expected at all
    mInvalidVsyncCount++;

    return Void();
}

Return<void> GraphicsComposerCallback::onVsync_2_4(Display display, int64_t, VsyncPeriodNanos) {
    std::lock_guard<std::mutex> lock(mMutex);

    auto it = std::find(mDisplays.begin(), mDisplays.end(), display);
    if (!mVsyncAllowed || it == mDisplays.end()) {
        mInvalidVsync_2_4Count++;
    }

    return Void();
}

Return<void> GraphicsComposerCallback::onVsyncPeriodTimingChanged(
        Display display, const VsyncPeriodChangeTimeline& updatedTimeline) {
    std::lock_guard<std::mutex> lock(mMutex);

    auto it = std::find(mDisplays.begin(), mDisplays.end(), display);
    if (it == mDisplays.end()) {
        mInvalidVsyncPeriodChangeCount++;
    }

    mTimeline = updatedTimeline;

    return Void();
}

Return<void> GraphicsComposerCallback::onSeamlessPossible(Display) {
    std::lock_guard<std::mutex> lock(mMutex);

    mInvalidSeamlessPossibleCount++;

    return Void();
}

}  // namespace android::hardware::graphics::composer::V2_4::vts
