/*
 * Copyright 2021 The Android Open Source Project
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

#include <aidl/android/hardware/tv/tuner/BnFrontend.h>
#include <fstream>
#include <iostream>
#include <thread>
#include "Tuner.h"
#include "dtv_plugin.h"

using namespace std;

namespace aidl {
namespace android {
namespace hardware {
namespace tv {
namespace tuner {

class Tuner;

class Frontend : public BnFrontend {
  public:
    Frontend(FrontendType type, int32_t id);

    ::ndk::ScopedAStatus setCallback(
            const std::shared_ptr<IFrontendCallback>& in_callback) override;
    ::ndk::ScopedAStatus tune(const FrontendSettings& in_settings) override;
    ::ndk::ScopedAStatus stopTune() override;
    ::ndk::ScopedAStatus close() override;
    ::ndk::ScopedAStatus scan(const FrontendSettings& in_settings,
                              FrontendScanType in_type) override;
    ::ndk::ScopedAStatus stopScan() override;
    ::ndk::ScopedAStatus getStatus(const std::vector<FrontendStatusType>& in_statusTypes,
                                   std::vector<FrontendStatus>* _aidl_return) override;
    ::ndk::ScopedAStatus setLnb(int32_t in_lnbId) override;
    ::ndk::ScopedAStatus linkCiCam(int32_t in_ciCamId, int32_t* _aidl_return) override;
    ::ndk::ScopedAStatus unlinkCiCam(int32_t in_ciCamId) override;
    ::ndk::ScopedAStatus getHardwareInfo(std::string* _aidl_return) override;
    ::ndk::ScopedAStatus removeOutputPid(int32_t in_pid) override;
    ::ndk::ScopedAStatus getFrontendStatusReadiness(
            const std::vector<FrontendStatusType>& in_statusTypes,
            std::vector<FrontendStatusReadiness>* _aidl_return) override;

    binder_status_t dump(int fd, const char** args, uint32_t numArgs) override;

    FrontendType getFrontendType();
    int32_t getFrontendId();
    string getSourceFile();
    dtv_plugin* getIptvPluginInterface();
    string getIptvTransportDescription();
    dtv_streamer* getIptvPluginStreamer();
    void readTuneByte(dtv_streamer* streamer, void* buf, size_t size, int timeout_ms);
    bool isLocked();
    void getFrontendInfo(FrontendInfo* _aidl_return);
    void setTunerService(std::shared_ptr<Tuner> tuner);

  private:
    virtual ~Frontend();
    bool supportsSatellite();
    void scanThreadLoop();

    std::shared_ptr<IFrontendCallback> mCallback;
    std::shared_ptr<Tuner> mTuner;
    FrontendType mType = FrontendType::UNDEFINED;
    int32_t mId = 0;
    bool mIsLocked = false;
    int32_t mCiCamId;
    std::thread mScanThread;
    FrontendSettings mFrontendSettings;
    FrontendScanType mFrontendScanType;
    std::ifstream mFrontendData;
    FrontendCapabilities mFrontendCaps;
    vector<FrontendStatusType> mFrontendStatusCaps;
    dtv_plugin* mIptvPluginInterface;
    string mIptvTransportDescription;
    dtv_streamer* mIptvPluginStreamer;
    std::thread mIptvFrontendTuneThread;
};

}  // namespace tuner
}  // namespace tv
}  // namespace hardware
}  // namespace android
}  // namespace aidl
