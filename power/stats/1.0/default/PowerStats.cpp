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

#define LOG_TAG "android.hardware.power.stats@1.0-service-mock"

#include "PowerStats.h"
#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/properties.h>
#include <android-base/stringprintf.h>
#include <android-base/strings.h>
#include <inttypes.h>
#include <stdlib.h>
#include <algorithm>
#include <exception>
#include <thread>

namespace android {
namespace hardware {
namespace power {
namespace stats {
namespace V1_0 {
namespace implementation {

#define MAX_FILE_PATH_LEN 128
#define MAX_DEVICE_NAME_LEN 64
#define MAX_QUEUE_SIZE 8192

constexpr char kIioDirRoot[] = "/sys/bus/iio/devices/";
constexpr char kDeviceName[] = "pm_device_name";
constexpr char kDeviceType[] = "iio:device";
constexpr uint32_t MAX_SAMPLING_RATE = 10;
constexpr uint64_t WRITE_TIMEOUT_NS = 1000000000;

void PowerStats::findIioPowerMonitorNodes() {
    struct dirent* ent;
    int fd;
    char devName[MAX_DEVICE_NAME_LEN];
    char filePath[MAX_FILE_PATH_LEN];
    DIR* iioDir = opendir(kIioDirRoot);
    if (!iioDir) {
        ALOGE("Error opening directory: %s", kIioDirRoot);
        return;
    }
    while (ent = readdir(iioDir), ent) {
        if (strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0 &&
            strlen(ent->d_name) > strlen(kDeviceType) &&
            strncmp(ent->d_name, kDeviceType, strlen(kDeviceType)) == 0) {
            snprintf(filePath, MAX_FILE_PATH_LEN, "%s/%s", ent->d_name, "name");
            fd = openat(dirfd(iioDir), filePath, O_RDONLY);
            if (fd < 0) {
                ALOGW("Failed to open directory: %s", filePath);
                continue;
            }
            if (read(fd, devName, MAX_DEVICE_NAME_LEN) < 0) {
                ALOGW("Failed to read device name from file: %s(%d)", filePath, fd);
                close(fd);
                continue;
            }

            if (strncmp(devName, kDeviceName, strlen(kDeviceName)) == 0) {
                snprintf(filePath, MAX_FILE_PATH_LEN, "%s/%s", kIioDirRoot, ent->d_name);
                mPm.devicePaths.push_back(filePath);
            }
            close(fd);
        }
    }
    closedir(iioDir);
    return;
}

size_t PowerStats::parsePowerRails() {
    std::string data;
    std::string railFileName;
    std::string spsFileName;
    uint32_t index = 0;
    uint32_t samplingRate;
    for (const auto& path : mPm.devicePaths) {
        railFileName = path + "/enabled_rails";
        spsFileName = path + "/sampling_rate";
        if (!android::base::ReadFileToString(spsFileName, &data)) {
            ALOGW("Error reading file: %s", spsFileName.c_str());
            continue;
        }
        samplingRate = strtoul(data.c_str(), NULL, 10);
        if (!samplingRate || samplingRate == ULONG_MAX) {
            ALOGE("Error parsing: %s", spsFileName.c_str());
            break;
        }
        if (!android::base::ReadFileToString(railFileName, &data)) {
            ALOGW("Error reading file: %s", railFileName.c_str());
            continue;
        }
        std::istringstream railNames(data);
        std::string line;
        while (std::getline(railNames, line)) {
            std::vector<std::string> words = android::base::Split(line, ":");
            if (words.size() == 2) {
                mPm.railsInfo.emplace(words[0], RailData{.devicePath = path,
                                                         .index = index,
                                                         .subsysName = words[1],
                                                         .samplingRate = samplingRate});
                index++;
            } else {
                ALOGW("Unexpected format in file: %s", railFileName.c_str());
            }
        }
    }
    return index;
}

int PowerStats::parseIioEnergyNode(std::string devName) {
    int ret = 0;
    std::string data;
    std::string fileName = devName + "/energy_value";
    if (!android::base::ReadFileToString(fileName, &data)) {
        ALOGE("Error reading file: %s", fileName.c_str());
        return -1;
    }

    std::istringstream energyData(data);
    std::string line;
    uint64_t timestamp = 0;
    bool timestampRead = false;
    while (std::getline(energyData, line)) {
        std::vector<std::string> words = android::base::Split(line, ",");
        if (timestampRead == false) {
            if (words.size() == 1) {
                timestamp = strtoull(words[0].c_str(), NULL, 10);
                if (timestamp == 0 || timestamp == ULLONG_MAX) {
                    ALOGW("Potentially wrong timestamp: %" PRIu64, timestamp);
                }
                timestampRead = true;
            }
        } else if (words.size() == 2) {
            std::string railName = words[0];
            if (mPm.railsInfo.count(railName) != 0) {
                size_t index = mPm.railsInfo[railName].index;
                mPm.reading[index].index = index;
                mPm.reading[index].timestamp = timestamp;
                mPm.reading[index].energy = strtoull(words[1].c_str(), NULL, 10);
                if (mPm.reading[index].energy == ULLONG_MAX) {
                    ALOGW("Potentially wrong energy value: %" PRIu64, mPm.reading[index].energy);
                }
            }
        } else {
            ALOGW("Unexpected format in file: %s", fileName.c_str());
            ret = -1;
            break;
        }
    }
    return ret;
}

Status PowerStats::parseIioEnergyNodes() {
    Status ret = Status::SUCCESS;
    if (mPm.hwEnabled == false) {
        return Status::NOT_SUPPORTED;
    }

    for (const auto& devicePath : mPm.devicePaths) {
        if (parseIioEnergyNode(devicePath) < 0) {
            ALOGE("Error in parsing power stats");
            ret = Status::FILESYSTEM_ERROR;
            break;
        }
    }
    return ret;
}

PowerStats::PowerStats() {
    findIioPowerMonitorNodes();
    size_t numRails = parsePowerRails();
    if (mPm.devicePaths.empty() || numRails == 0) {
        mPm.hwEnabled = false;
    } else {
        mPm.hwEnabled = true;
        mPm.reading.resize(numRails);
    }
}

Return<void> PowerStats::getRailInfo(getRailInfo_cb _hidl_cb) {
    hidl_vec<RailInfo> rInfo;
    Status ret = Status::SUCCESS;
    size_t index;
    std::lock_guard<std::mutex> _lock(mPm.mLock);
    if (mPm.hwEnabled == false) {
        _hidl_cb(rInfo, Status::NOT_SUPPORTED);
        return Void();
    }
    rInfo.resize(mPm.railsInfo.size());
    for (const auto& railData : mPm.railsInfo) {
        index = railData.second.index;
        rInfo[index].railName = railData.first;
        rInfo[index].subsysName = railData.second.subsysName;
        rInfo[index].index = index;
        rInfo[index].samplingRate = railData.second.samplingRate;
    }
    _hidl_cb(rInfo, ret);
    return Void();
}

Return<void> PowerStats::getEnergyData(const hidl_vec<uint32_t>& railIndices,
                                       getEnergyData_cb _hidl_cb) {
    hidl_vec<EnergyData> eVal;
    std::lock_guard<std::mutex> _lock(mPm.mLock);
    Status ret = parseIioEnergyNodes();

    if (ret != Status::SUCCESS) {
        ALOGE("Failed to getEnergyData");
        _hidl_cb(eVal, ret);
        return Void();
    }

    if (railIndices.size() == 0) {
        eVal.resize(mPm.railsInfo.size());
        memcpy(&eVal[0], &mPm.reading[0], mPm.reading.size() * sizeof(EnergyData));
    } else {
        eVal.resize(railIndices.size());
        int i = 0;
        for (const auto& railIndex : railIndices) {
            if (railIndex >= mPm.reading.size()) {
                ret = Status::INVALID_INPUT;
                eVal.resize(0);
                break;
            }
            memcpy(&eVal[i], &mPm.reading[railIndex], sizeof(EnergyData));
            i++;
        }
    }
    _hidl_cb(eVal, ret);
    return Void();
}

Return<void> PowerStats::streamEnergyData(uint32_t timeMs, uint32_t samplingRate,
                                          streamEnergyData_cb _hidl_cb) {
    std::lock_guard<std::mutex> _lock(mPm.mLock);
    if (mPm.fmqSynchronized != nullptr) {
        _hidl_cb(MessageQueueSync::Descriptor(), 0, 0, Status::INSUFFICIENT_RESOURCES);
        return Void();
    }
    uint32_t sps = std::min(samplingRate, MAX_SAMPLING_RATE);
    uint32_t numSamples = timeMs * sps / 1000;
    mPm.fmqSynchronized.reset(new (std::nothrow) MessageQueueSync(MAX_QUEUE_SIZE, true));
    if (mPm.fmqSynchronized == nullptr || mPm.fmqSynchronized->isValid() == false) {
        mPm.fmqSynchronized = nullptr;
        _hidl_cb(MessageQueueSync::Descriptor(), 0, 0, Status::INSUFFICIENT_RESOURCES);
        return Void();
    }
    std::thread pollThread = std::thread([this, sps, numSamples]() {
        uint64_t sleepTimeUs = 1000000 / sps;
        uint32_t currSamples = 0;
        while (currSamples < numSamples) {
            mPm.mLock.lock();
            if (parseIioEnergyNodes() == Status::SUCCESS) {
                mPm.fmqSynchronized->writeBlocking(&mPm.reading[0], mPm.reading.size(),
                                                   WRITE_TIMEOUT_NS);
                mPm.mLock.unlock();
                currSamples++;
                if (usleep(sleepTimeUs) < 0) {
                    ALOGW("Sleep interrupted");
                    break;
                }
            } else {
                mPm.mLock.unlock();
                break;
            }
        }
        mPm.mLock.lock();
        mPm.fmqSynchronized = nullptr;
        mPm.mLock.unlock();
        return;
    });
    pollThread.detach();
    _hidl_cb(*(mPm.fmqSynchronized)->getDesc(), numSamples, mPm.reading.size(), Status::SUCCESS);
    return Void();
}

uint32_t PowerStats::addPowerEntity(const std::string& name, PowerEntityType type) {
    uint32_t id = mPowerEntityInfos.size();
    mPowerEntityInfos.push_back({id, name, type});
    return id;
}

void PowerStats::addStateResidencyDataProvider(std::shared_ptr<IStateResidencyDataProvider> p) {
    std::vector<PowerEntityStateSpace> stateSpaces = p->getStateSpaces();
    for (auto stateSpace : stateSpaces) {
        mPowerEntityStateSpaces.emplace(stateSpace.powerEntityId, stateSpace);
        mStateResidencyDataProviders.emplace(stateSpace.powerEntityId, p);
    }
}

Return<void> PowerStats::getPowerEntityInfo(getPowerEntityInfo_cb _hidl_cb) {
    // If not configured, return NOT_SUPPORTED
    if (mPowerEntityInfos.empty()) {
        _hidl_cb({}, Status::NOT_SUPPORTED);
        return Void();
    }

    _hidl_cb(mPowerEntityInfos, Status::SUCCESS);
    return Void();
}

Return<void> PowerStats::getPowerEntityStateInfo(const hidl_vec<uint32_t>& powerEntityIds,
                                                 getPowerEntityStateInfo_cb _hidl_cb) {
    // If not configured, return NOT_SUPPORTED
    if (mPowerEntityStateSpaces.empty()) {
        _hidl_cb({}, Status::NOT_SUPPORTED);
        return Void();
    }

    std::vector<PowerEntityStateSpace> stateSpaces;

    // If powerEntityIds is empty then return state space info for all entities
    if (powerEntityIds.size() == 0) {
        stateSpaces.reserve(mPowerEntityStateSpaces.size());
        for (auto i : mPowerEntityStateSpaces) {
            stateSpaces.emplace_back(i.second);
        }
        _hidl_cb(stateSpaces, Status::SUCCESS);
        return Void();
    }

    // Return state space information only for valid ids
    auto ret = Status::SUCCESS;
    stateSpaces.reserve(powerEntityIds.size());
    for (const uint32_t id : powerEntityIds) {
        auto stateSpace = mPowerEntityStateSpaces.find(id);
        if (stateSpace != mPowerEntityStateSpaces.end()) {
            stateSpaces.emplace_back(stateSpace->second);
        } else {
            ret = Status::INVALID_INPUT;
        }
    }

    _hidl_cb(stateSpaces, ret);
    return Void();
}

Return<void> PowerStats::getPowerEntityStateResidencyData(
        const hidl_vec<uint32_t>& powerEntityIds, getPowerEntityStateResidencyData_cb _hidl_cb) {
    // If not configured, return NOT_SUPPORTED
    if (mStateResidencyDataProviders.empty() || mPowerEntityStateSpaces.empty()) {
        _hidl_cb({}, Status::NOT_SUPPORTED);
        return Void();
    }

    // If powerEntityIds is empty then return data for all supported entities
    if (powerEntityIds.size() == 0) {
        std::vector<uint32_t> ids;
        for (auto stateSpace : mPowerEntityStateSpaces) {
            ids.emplace_back(stateSpace.first);
        }
        return getPowerEntityStateResidencyData(ids, _hidl_cb);
    }

    std::unordered_map<uint32_t, PowerEntityStateResidencyResult> stateResidencies;
    std::vector<PowerEntityStateResidencyResult> results;
    results.reserve(powerEntityIds.size());

    // return results for only the given powerEntityIds
    bool invalidInput = false;
    bool filesystemError = false;
    for (auto id : powerEntityIds) {
        auto dataProvider = mStateResidencyDataProviders.find(id);
        // skip if the given powerEntityId does not have an associated StateResidencyDataProvider
        if (dataProvider == mStateResidencyDataProviders.end()) {
            invalidInput = true;
            continue;
        }

        // get the results if we have not already done so.
        if (stateResidencies.find(id) == stateResidencies.end()) {
            if (!dataProvider->second->getResults(stateResidencies)) {
                filesystemError = true;
            }
        }

        // append results
        auto stateResidency = stateResidencies.find(id);
        if (stateResidency != stateResidencies.end()) {
            results.emplace_back(stateResidency->second);
        }
    }

    auto ret = Status::SUCCESS;
    if (filesystemError) {
        ret = Status::FILESYSTEM_ERROR;
    } else if (invalidInput) {
        ret = Status::INVALID_INPUT;
    }

    _hidl_cb(results, ret);
    return Void();
}

bool DumpResidencyDataToFd(const hidl_vec<PowerEntityInfo>& infos,
                           const hidl_vec<PowerEntityStateSpace>& stateSpaces,
                           const hidl_vec<PowerEntityStateResidencyResult>& results, int fd) {
    // construct lookup table of powerEntityId to name
    std::unordered_map<uint32_t, std::string> entityNames;
    for (auto info : infos) {
        entityNames.emplace(info.powerEntityId, info.powerEntityName);
    }

    // construct lookup table of powerEntityId, powerEntityStateId to state name
    std::unordered_map<uint32_t, std::unordered_map<uint32_t, std::string>> stateNames;
    for (auto stateSpace : stateSpaces) {
        stateNames.emplace(stateSpace.powerEntityId, std::unordered_map<uint32_t, std::string>());
        for (auto state : stateSpace.states) {
            stateNames.at(stateSpace.powerEntityId)
                    .emplace(state.powerEntityStateId, state.powerEntityStateName);
        }
    }

    std::ostringstream dumpStats;
    dumpStats << "\n========== PowerStats HAL 1.0 state residencies ==========\n";

    const char* headerFormat = "  %14s   %14s   %16s   %15s   %16s\n";
    const char* dataFormat =
            "  %14s   %14s   %13" PRIu64 " ms   %15" PRIu64 "   %13" PRIu64 " ms\n";
    dumpStats << android::base::StringPrintf(headerFormat, "Entity", "State", "Total time",
                                             "Total entries", "Last entry timestamp");

    for (auto result : results) {
        for (auto stateResidency : result.stateResidencyData) {
            dumpStats << android::base::StringPrintf(
                    dataFormat, entityNames.at(result.powerEntityId).c_str(),
                    stateNames.at(result.powerEntityId)
                            .at(stateResidency.powerEntityStateId)
                            .c_str(),
                    stateResidency.totalTimeInStateMs, stateResidency.totalStateEntryCount,
                    stateResidency.lastEntryTimestampMs);
        }
    }

    dumpStats << "========== End of PowerStats HAL 1.0 state residencies ==========\n";

    return android::base::WriteStringToFd(dumpStats.str(), fd);
}

Return<void> PowerStats::debug(const hidl_handle& handle, const hidl_vec<hidl_string>&) {
    if (handle == nullptr || handle->numFds < 1) {
        return Void();
    }

    int fd = handle->data[0];
    Status status;
    hidl_vec<PowerEntityInfo> infos;

    // Get power entity information
    getPowerEntityInfo([&status, &infos](auto rInfos, auto rStatus) {
        status = rStatus;
        infos = rInfos;
    });
    if (status != Status::SUCCESS) {
        LOG(ERROR) << "Error getting power entity info";
        return Void();
    }

    // Get power entity state information
    hidl_vec<PowerEntityStateSpace> stateSpaces;
    getPowerEntityStateInfo({}, [&status, &stateSpaces](auto rStateSpaces, auto rStatus) {
        status = rStatus;
        stateSpaces = rStateSpaces;
    });
    if (status != Status::SUCCESS) {
        LOG(ERROR) << "Error getting state info";
        return Void();
    }

    // Get power entity state residency data
    hidl_vec<PowerEntityStateResidencyResult> results;
    getPowerEntityStateResidencyData({}, [&status, &results](auto rResults, auto rStatus) {
        status = rStatus;
        results = rResults;
    });

    // This implementation of getPowerEntityStateResidencyData supports the
    // return of partial results if status == FILESYSTEM_ERROR.
    if (status != Status::SUCCESS) {
        LOG(ERROR) << "Error getting residency data -- Some results missing";
    }

    if (!DumpResidencyDataToFd(infos, stateSpaces, results, fd)) {
        PLOG(ERROR) << "Failed to dump residency data to fd";
    }

    fsync(fd);

    return Void();
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace stats
}  // namespace power
}  // namespace hardware
}  // namespace android
