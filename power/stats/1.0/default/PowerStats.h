/*
 * Copyright (C) 2018 The Android Open Source Project
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

#ifndef ANDROID_HARDWARE_POWERSTATS_V1_0_POWERSTATS_H
#define ANDROID_HARDWARE_POWERSTATS_V1_0_POWERSTATS_H

#include <android/hardware/power/stats/1.0/IPowerStats.h>
#include <fmq/MessageQueue.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>

namespace android {
namespace hardware {
namespace power {
namespace stats {
namespace V1_0 {
namespace implementation {

using ::android::hardware::hidl_vec;
using ::android::hardware::kSynchronizedReadWrite;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::power::stats::V1_0::EnergyData;
using ::android::hardware::power::stats::V1_0::PowerEntityInfo;
using ::android::hardware::power::stats::V1_0::PowerEntityStateInfo;
using ::android::hardware::power::stats::V1_0::PowerEntityStateResidencyData;
using ::android::hardware::power::stats::V1_0::PowerEntityStateResidencyResult;
using ::android::hardware::power::stats::V1_0::PowerEntityStateSpace;
using ::android::hardware::power::stats::V1_0::PowerEntityType;
using ::android::hardware::power::stats::V1_0::RailInfo;
using ::android::hardware::power::stats::V1_0::Status;

typedef MessageQueue<EnergyData, kSynchronizedReadWrite> MessageQueueSync;
struct RailData {
    std::string devicePath;
    uint32_t index;
    std::string subsysName;
    uint32_t samplingRate;
};

struct OnDeviceMmt {
    std::mutex mLock;
    bool hwEnabled;
    std::vector<std::string> devicePaths;
    std::map<std::string, RailData> railsInfo;
    std::vector<EnergyData> reading;
    std::unique_ptr<MessageQueueSync> fmqSynchronized;
};

struct PowerStats : public IPowerStats {
    PowerStats();
    // Methods from ::android::hardware::power::stats::V1_0::IPowerStats follow.
    Return<void> getRailInfo(getRailInfo_cb _hidl_cb) override;
    Return<void> getEnergyData(const hidl_vec<uint32_t>& railIndices,
                               getEnergyData_cb _hidl_cb) override;
    Return<void> streamEnergyData(uint32_t timeMs, uint32_t samplingRate,
                                  streamEnergyData_cb _hidl_cb) override;
    Return<void> getPowerEntityInfo(getPowerEntityInfo_cb _hidl_cb) override;
    Return<void> getPowerEntityStateInfo(const hidl_vec<uint32_t>& powerEntityIds,
                                         getPowerEntityStateInfo_cb _hidl_cb) override;
    Return<void> getPowerEntityStateResidencyData(
        const hidl_vec<uint32_t>& powerEntityIds,
        getPowerEntityStateResidencyData_cb _hidl_cb) override;

   private:
    OnDeviceMmt mPm;
    void findIioPowerMonitorNodes();
    size_t parsePowerRails();
    int parseIioEnergyNode(std::string devName);
    Status parseIioEnergyNodes();
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace stats
}  // namespace power
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_POWERSTATS_V1_0_POWERSTATS_H
