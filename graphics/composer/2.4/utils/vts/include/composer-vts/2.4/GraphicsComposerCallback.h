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
#pragma once

#include <android/hardware/graphics/composer/2.4/IComposerCallback.h>

#include <mutex>
#include <unordered_set>

namespace android::hardware::graphics::composer::V2_4::vts {

using Display = V2_1::Display;

// IComposerCallback to be installed with IComposerClient::registerCallback.
class GraphicsComposerCallback : public IComposerCallback {
  public:
    void setVsyncAllowed(bool allowed);

    std::vector<Display> getDisplays() const;

    int32_t getInvalidHotplugCount() const;

    int32_t getInvalidRefreshCount() const;

    int32_t getInvalidVsyncCount() const;

    int32_t getInvalidVsync_2_4Count() const;

    int32_t getInvalidVsyncPeriodChangeCount() const;

    int32_t getInvalidSeamlessPossibleCount() const;

    std::optional<VsyncPeriodChangeTimeline> takeLastVsyncPeriodChangeTimeline();

  private:
    Return<void> onHotplug(Display display, Connection connection) override;
    Return<void> onRefresh(Display display) override;
    Return<void> onVsync(Display display, int64_t) override;
    Return<void> onVsync_2_4(Display display, int64_t, VsyncPeriodNanos vsyncPeriodNanos) override;
    Return<void> onVsyncPeriodTimingChanged(
            Display display, const VsyncPeriodChangeTimeline& updatedTimeline) override;
    Return<void> onSeamlessPossible(Display display) override;

    mutable std::mutex mMutex;
    // the set of all currently connected displays
    std::unordered_set<Display> mDisplays;
    // true only when vsync is enabled
    bool mVsyncAllowed = true;

    std::optional<VsyncPeriodChangeTimeline> mTimeline;

    // track invalid callbacks
    int32_t mInvalidHotplugCount = 0;
    int32_t mInvalidRefreshCount = 0;
    int32_t mInvalidVsyncCount = 0;
    int32_t mInvalidVsync_2_4Count = 0;
    int32_t mInvalidVsyncPeriodChangeCount = 0;
    int32_t mInvalidSeamlessPossibleCount = 0;
};

}  // namespace android::hardware::graphics::composer::V2_4::vts
