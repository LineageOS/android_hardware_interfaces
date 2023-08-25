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

#pragma once

#include "VirtualRadio.h"

#include <aidl/android/hardware/broadcastradio/AmFmBandRange.h>
#include <aidl/android/hardware/broadcastradio/AmFmRegionConfig.h>
#include <aidl/android/hardware/broadcastradio/AnnouncementType.h>
#include <aidl/android/hardware/broadcastradio/BnBroadcastRadio.h>
#include <aidl/android/hardware/broadcastradio/DabTableEntry.h>
#include <aidl/android/hardware/broadcastradio/IAnnouncementListener.h>
#include <aidl/android/hardware/broadcastradio/ICloseHandle.h>
#include <aidl/android/hardware/broadcastradio/ITunerCallback.h>
#include <aidl/android/hardware/broadcastradio/Properties.h>
#include <broadcastradio-utils/WorkerThread.h>

#include <android-base/thread_annotations.h>

#include <optional>

namespace aidl::android::hardware::broadcastradio {

class BroadcastRadio final : public BnBroadcastRadio {
  public:
    explicit BroadcastRadio(const VirtualRadio& virtualRadio);
    ~BroadcastRadio();
    ndk::ScopedAStatus getAmFmRegionConfig(bool full, AmFmRegionConfig* returnConfigs) override;
    ndk::ScopedAStatus getDabRegionConfig(std::vector<DabTableEntry>* returnConfigs) override;
    ndk::ScopedAStatus getImage(int32_t id, std::vector<uint8_t>* returnImage) override;
    ndk::ScopedAStatus getProperties(Properties* returnProperties) override;

    ndk::ScopedAStatus setTunerCallback(const std::shared_ptr<ITunerCallback>& callback) override;
    ndk::ScopedAStatus unsetTunerCallback() override;
    ndk::ScopedAStatus tune(const ProgramSelector& program) override;
    ndk::ScopedAStatus seek(bool directionUp, bool skipSubChannel) override;
    ndk::ScopedAStatus step(bool directionUp) override;
    ndk::ScopedAStatus cancel() override;
    ndk::ScopedAStatus startProgramListUpdates(const ProgramFilter& filter) override;
    ndk::ScopedAStatus stopProgramListUpdates() override;
    ndk::ScopedAStatus isConfigFlagSet(ConfigFlag flag, bool* returnIsSet) override;
    ndk::ScopedAStatus setConfigFlag(ConfigFlag flag, bool in_value) override;
    ndk::ScopedAStatus setParameters(const std::vector<VendorKeyValue>& parameters,
                                     std::vector<VendorKeyValue>* returnParameters) override;
    ndk::ScopedAStatus getParameters(const std::vector<std::string>& keys,
                                     std::vector<VendorKeyValue>* returnParameters) override;
    ndk::ScopedAStatus registerAnnouncementListener(
            const std::shared_ptr<IAnnouncementListener>& listener,
            const std::vector<AnnouncementType>& enabled,
            std::shared_ptr<ICloseHandle>* returnCloseHandle) override;
    binder_status_t dump(int fd, const char** args, uint32_t numArgs) override;

  private:
    const VirtualRadio& mVirtualRadio;
    std::mutex mMutex;
    AmFmRegionConfig mAmFmConfig GUARDED_BY(mMutex);
    std::unique_ptr<::android::WorkerThread> mThread GUARDED_BY(mMutex) =
            std::unique_ptr<::android::WorkerThread>(new ::android::WorkerThread());
    bool mIsTuneCompleted GUARDED_BY(mMutex) = true;
    Properties mProperties GUARDED_BY(mMutex);
    ProgramSelector mCurrentProgram GUARDED_BY(mMutex) = {};
    std::shared_ptr<ITunerCallback> mCallback GUARDED_BY(mMutex);

    std::optional<AmFmBandRange> getAmFmRangeLocked() const;
    void cancelLocked();
    ProgramInfo tuneInternalLocked(const ProgramSelector& sel);

    binder_status_t cmdHelp(int fd) const;
    binder_status_t cmdTune(int fd, const char** args, uint32_t numArgs);
    binder_status_t cmdSeek(int fd, const char** args, uint32_t numArgs);
    binder_status_t cmdStep(int fd, const char** args, uint32_t numArgs);
    binder_status_t cmdCancel(int fd, uint32_t numArgs);
    binder_status_t cmdStartProgramListUpdates(int fd, const char** args, uint32_t numArgs);
    binder_status_t cmdStopProgramListUpdates(int fd, uint32_t numArgs);

    binder_status_t dumpsys(int fd);
};

}  // namespace aidl::android::hardware::broadcastradio
