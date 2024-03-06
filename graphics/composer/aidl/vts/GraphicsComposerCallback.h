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
#pragma once

#include <aidl/android/hardware/graphics/composer3/BnComposerCallback.h>
#include <android-base/thread_annotations.h>
#include <mutex>
#include <vector>

namespace aidl::android::hardware::graphics::composer3::vts {

class GraphicsComposerCallback : public BnComposerCallback {
  public:
    void setVsyncAllowed(bool allowed);

    void setRefreshRateChangedDebugDataEnabledCallbackAllowed(bool allowed);

    std::vector<int64_t> getDisplays() const;

    int32_t getInvalidHotplugCount() const;

    int32_t getInvalidRefreshCount() const;

    int32_t getInvalidVsyncCount() const;

    int32_t getInvalidVsyncPeriodChangeCount() const;

    int32_t getInvalidSeamlessPossibleCount() const;

    int32_t getVsyncIdleCount() const;

    int64_t getVsyncIdleTime() const;

    std::optional<VsyncPeriodChangeTimeline> takeLastVsyncPeriodChangeTimeline();

    std::vector<RefreshRateChangedDebugData> takeListOfRefreshRateChangedDebugData();

    int32_t getInvalidRefreshRateDebugEnabledCallbackCount() const;

  private:
    virtual ::ndk::ScopedAStatus onHotplug(int64_t in_display, bool in_connected) override;
    virtual ::ndk::ScopedAStatus onRefresh(int64_t in_display) override;
    virtual ::ndk::ScopedAStatus onSeamlessPossible(int64_t in_display) override;
    virtual ::ndk::ScopedAStatus onVsync(int64_t in_display, int64_t in_timestamp,
                                         int32_t in_vsyncPeriodNanos) override;
    virtual ::ndk::ScopedAStatus onVsyncPeriodTimingChanged(
            int64_t in_display,
            const ::aidl::android::hardware::graphics::composer3::VsyncPeriodChangeTimeline&
                    in_updatedTimeline) override;
    virtual ::ndk::ScopedAStatus onVsyncIdle(int64_t in_display) override;
    virtual ::ndk::ScopedAStatus onRefreshRateChangedDebug(
            const RefreshRateChangedDebugData&) override;
    virtual ::ndk::ScopedAStatus onHotplugEvent(int64_t in_display,
                                                common::DisplayHotplugEvent) override;

    mutable std::mutex mMutex;
    // the set of all currently connected displays
    std::vector<int64_t> mDisplays GUARDED_BY(mMutex);
    // true only when vsync is enabled
    bool mVsyncAllowed GUARDED_BY(mMutex) = true;
    // true only when RefreshRateChangedCallbackDebugEnabled is set to true.
    bool mRefreshRateChangedDebugDataEnabledCallbackAllowed GUARDED_BY(mMutex) = false;

    std::optional<VsyncPeriodChangeTimeline> mTimeline GUARDED_BY(mMutex);

    std::vector<RefreshRateChangedDebugData> mRefreshRateChangedDebugData GUARDED_BY(mMutex);

    int32_t mVsyncIdleCount GUARDED_BY(mMutex) = 0;
    int64_t mVsyncIdleTime GUARDED_BY(mMutex) = 0;

    // track invalid callbacks
    int32_t mInvalidHotplugCount GUARDED_BY(mMutex) = 0;
    int32_t mInvalidRefreshCount GUARDED_BY(mMutex) = 0;
    int32_t mInvalidVsyncCount GUARDED_BY(mMutex) = 0;
    int32_t mInvalidVsyncPeriodChangeCount GUARDED_BY(mMutex) = 0;
    int32_t mInvalidSeamlessPossibleCount GUARDED_BY(mMutex) = 0;
    int32_t mInvalidRefreshRateDebugEnabledCallbackCount GUARDED_BY(mMutex) = 0;
};

}  // namespace aidl::android::hardware::graphics::composer3::vts
