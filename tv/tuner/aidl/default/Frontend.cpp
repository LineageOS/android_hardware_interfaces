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

//#define LOG_NDEBUG 0
#define LOG_TAG "android.hardware.tv.tuner-service.example-Frontend"

#include <aidl/android/hardware/tv/tuner/Result.h>
#include <utils/Log.h>

#include "Frontend.h"

namespace aidl {
namespace android {
namespace hardware {
namespace tv {
namespace tuner {

Frontend::Frontend(FrontendType type, int32_t id) {
    mType = type;
    mId = id;
    mTuner = nullptr;
    // Init callback to nullptr
    mCallback = nullptr;
    mIptvPluginInterface = nullptr;
    mIptvPluginStreamer = nullptr;

    switch (mType) {
        case FrontendType::ISDBS: {
            mFrontendCaps.set<FrontendCapabilities::Tag::isdbsCaps>(FrontendIsdbsCapabilities());
            mFrontendStatusCaps = {
                    FrontendStatusType::DEMOD_LOCK,
                    FrontendStatusType::SNR,
                    FrontendStatusType::FEC,
                    FrontendStatusType::MODULATION,
                    FrontendStatusType::MODULATIONS,
                    FrontendStatusType::ROLL_OFF,
                    FrontendStatusType::STREAM_ID_LIST,
            };
            break;
        }
        case FrontendType::ATSC3: {
            mFrontendCaps.set<FrontendCapabilities::Tag::atsc3Caps>(FrontendAtsc3Capabilities());
            mFrontendStatusCaps = {
                    FrontendStatusType::BER,
                    FrontendStatusType::PER,
                    FrontendStatusType::ATSC3_PLP_INFO,
                    FrontendStatusType::MODULATIONS,
                    FrontendStatusType::BERS,
                    FrontendStatusType::INTERLEAVINGS,
                    FrontendStatusType::BANDWIDTH,
                    FrontendStatusType::ATSC3_ALL_PLP_INFO,
            };
            break;
        }
        case FrontendType::DVBC: {
            mFrontendCaps.set<FrontendCapabilities::Tag::dvbcCaps>(FrontendDvbcCapabilities());
            mFrontendStatusCaps = {
                    FrontendStatusType::PRE_BER,       FrontendStatusType::SIGNAL_QUALITY,
                    FrontendStatusType::MODULATION,    FrontendStatusType::SPECTRAL,
                    FrontendStatusType::MODULATIONS,   FrontendStatusType::CODERATES,
                    FrontendStatusType::INTERLEAVINGS, FrontendStatusType::BANDWIDTH,
            };
            break;
        }
        case FrontendType::DVBS: {
            mFrontendCaps.set<FrontendCapabilities::Tag::dvbsCaps>(FrontendDvbsCapabilities());
            mFrontendStatusCaps = {
                    FrontendStatusType::SIGNAL_STRENGTH, FrontendStatusType::SYMBOL_RATE,
                    FrontendStatusType::MODULATION,      FrontendStatusType::MODULATIONS,
                    FrontendStatusType::ROLL_OFF,        FrontendStatusType::IS_MISO,
            };
            break;
        }
        case FrontendType::DVBT: {
            mFrontendCaps.set<FrontendCapabilities::Tag::dvbtCaps>(FrontendDvbtCapabilities());
            mFrontendStatusCaps = {
                    FrontendStatusType::EWBS,
                    FrontendStatusType::PLP_ID,
                    FrontendStatusType::HIERARCHY,
                    FrontendStatusType::MODULATIONS,
                    FrontendStatusType::BANDWIDTH,
                    FrontendStatusType::GUARD_INTERVAL,
                    FrontendStatusType::TRANSMISSION_MODE,
                    FrontendStatusType::T2_SYSTEM_ID,
                    FrontendStatusType::DVBT_CELL_IDS,
            };
            break;
        }
        case FrontendType::ISDBT: {
            FrontendIsdbtCapabilities isdbtCaps{
                    .modeCap = (int)FrontendIsdbtMode::MODE_1 | (int)FrontendIsdbtMode::MODE_2,
                    .bandwidthCap = (int)FrontendIsdbtBandwidth::BANDWIDTH_6MHZ,
                    .modulationCap = (int)FrontendIsdbtModulation::MOD_16QAM,
                    .coderateCap = (int)FrontendIsdbtCoderate::CODERATE_4_5 |
                                   (int)FrontendIsdbtCoderate::CODERATE_6_7,
                    .guardIntervalCap = (int)FrontendIsdbtGuardInterval::INTERVAL_1_128,
                    .timeInterleaveCap = (int)FrontendIsdbtTimeInterleaveMode::AUTO |
                                         (int)FrontendIsdbtTimeInterleaveMode::INTERLEAVE_1_0,
                    .isSegmentAuto = true,
                    .isFullSegment = true,
            };
            mFrontendCaps.set<FrontendCapabilities::Tag::isdbtCaps>(isdbtCaps);
            mFrontendStatusCaps = {
                    FrontendStatusType::AGC,
                    FrontendStatusType::LNA,
                    FrontendStatusType::MODULATION,
                    FrontendStatusType::MODULATIONS,
                    FrontendStatusType::BANDWIDTH,
                    FrontendStatusType::GUARD_INTERVAL,
                    FrontendStatusType::TRANSMISSION_MODE,
                    FrontendStatusType::ISDBT_SEGMENTS,
                    FrontendStatusType::ISDBT_MODE,
                    FrontendStatusType::ISDBT_PARTIAL_RECEPTION_FLAG,
                    FrontendStatusType::INTERLEAVINGS,
            };
            break;
        }
        case FrontendType::ANALOG: {
            mFrontendCaps.set<FrontendCapabilities::Tag::analogCaps>(FrontendAnalogCapabilities());
            mFrontendStatusCaps = {
                    FrontendStatusType::LAYER_ERROR,
                    FrontendStatusType::MER,
                    FrontendStatusType::UEC,
                    FrontendStatusType::TS_DATA_RATES,
            };
            break;
        }
        case FrontendType::ATSC: {
            mFrontendCaps.set<FrontendCapabilities::Tag::atscCaps>(FrontendAtscCapabilities());
            mFrontendStatusCaps = {
                    FrontendStatusType::FREQ_OFFSET,
                    FrontendStatusType::RF_LOCK,
                    FrontendStatusType::MODULATIONS,
                    FrontendStatusType::IS_LINEAR,
            };
            break;
        }
        case FrontendType::ISDBS3: {
            mFrontendCaps.set<FrontendCapabilities::Tag::isdbs3Caps>(FrontendIsdbs3Capabilities());
            mFrontendStatusCaps = {
                    FrontendStatusType::DEMOD_LOCK,      FrontendStatusType::MODULATION,
                    FrontendStatusType::MODULATIONS,     FrontendStatusType::ROLL_OFF,
                    FrontendStatusType::IS_SHORT_FRAMES, FrontendStatusType::STREAM_ID_LIST,
            };
            break;
        }
        case FrontendType::DTMB: {
            mFrontendCaps.set<FrontendCapabilities::Tag::dtmbCaps>(FrontendDtmbCapabilities());
            mFrontendStatusCaps = {
                    FrontendStatusType::MODULATIONS,       FrontendStatusType::INTERLEAVINGS,
                    FrontendStatusType::BANDWIDTH,         FrontendStatusType::GUARD_INTERVAL,
                    FrontendStatusType::TRANSMISSION_MODE,
            };
            break;
        }
        case FrontendType::IPTV: {
            mFrontendCaps.set<FrontendCapabilities::Tag::iptvCaps>(FrontendIptvCapabilities());
            mFrontendStatusCaps = {
                    FrontendStatusType::IPTV_CONTENT_URL,
                    FrontendStatusType::IPTV_PACKETS_LOST,
                    FrontendStatusType::IPTV_PACKETS_RECEIVED,
                    FrontendStatusType::IPTV_AVERAGE_JITTER_MS,
                    FrontendStatusType::IPTV_WORST_JITTER_MS,
            };
            break;
        }
        default: {
            break;
        }
    }
}

Frontend::~Frontend() {
    ALOGV("%s", __FUNCTION__);
    mCallback = nullptr;
    mIsLocked = false;
    mTuner = nullptr;
}

::ndk::ScopedAStatus Frontend::close() {
    ALOGV("%s", __FUNCTION__);
    // Reset callback
    mCallback = nullptr;
    mIsLocked = false;
    if (mTuner != nullptr) {
        mTuner->removeFrontend(mId);
    }
    mTuner = nullptr;

    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Frontend::setCallback(const std::shared_ptr<IFrontendCallback>& in_callback) {
    ALOGV("%s", __FUNCTION__);
    if (in_callback == nullptr) {
        ALOGW("[   WARN   ] Set Frontend callback with nullptr");
        return ::ndk::ScopedAStatus::fromServiceSpecificError(
                static_cast<int32_t>(Result::INVALID_ARGUMENT));
    }

    mCallback = in_callback;
    return ::ndk::ScopedAStatus::ok();
}

void Frontend::readTuneByte(dtv_streamer* streamer, void* buf, size_t buf_size, int timeout_ms) {
    ssize_t bytes_read = mIptvPluginInterface->read_stream(streamer, buf, buf_size, timeout_ms);
    if (bytes_read <= 0) {
        ALOGI("[   ERROR   ] Tune byte couldn't be read.");
        return;
    }
    mCallback->onEvent(FrontendEventType::LOCKED);
    mIsLocked = true;
}

::ndk::ScopedAStatus Frontend::tune(const FrontendSettings& in_settings) {
    if (mCallback == nullptr) {
        ALOGW("[   WARN   ] Frontend callback is not set for tunin0g");
        return ::ndk::ScopedAStatus::fromServiceSpecificError(
                static_cast<int32_t>(Result::INVALID_STATE));
    }

    if (mType != FrontendType::IPTV) {
        mTuner->frontendStartTune(mId);
        mCallback->onEvent(FrontendEventType::LOCKED);
        mIsLocked = true;
    } else {
        // This is a reference implementation for IPTV. It uses an additional socket buffer.
        // Vendors can use hardware memory directly to make the implementation more performant.
        ALOGI("[   INFO   ] Frontend type is set to IPTV, tag = %d id=%d", in_settings.getTag(),
              mId);

        // load udp plugin for reading TS data
        const char* path = "/vendor/lib/iptv_udp_plugin.so";
        DtvPlugin* plugin = new DtvPlugin(path);
        if (!plugin) {
            ALOGE("Failed to create DtvPlugin, plugin_path is invalid");
            return ::ndk::ScopedAStatus::fromServiceSpecificError(
                    static_cast<int32_t>(Result::INVALID_ARGUMENT));
        }
        bool plugin_loaded = plugin->load();
        if (!plugin_loaded) {
            ALOGE("Failed to load plugin");
            return ::ndk::ScopedAStatus::fromServiceSpecificError(
                    static_cast<int32_t>(Result::INVALID_ARGUMENT));
        }
        mIptvPluginInterface = plugin->interface();

        // validate content_url format
        std::string content_url = in_settings.get<FrontendSettings::Tag::iptv>()->contentUrl;
        std::string transport_desc = "{ \"uri\": \"" + content_url + "\"}";
        ALOGI("[   INFO   ] transport_desc: %s", transport_desc.c_str());
        bool is_transport_desc_valid = plugin->validate(transport_desc.c_str());
        if (!is_transport_desc_valid) {  // not of format protocol://ip:port
            ALOGE("[   INFO   ] transport_desc is not valid");
            return ::ndk::ScopedAStatus::fromServiceSpecificError(
                    static_cast<int32_t>(Result::INVALID_ARGUMENT));
        }
        mIptvTransportDescription = transport_desc;

        // create a streamer and open it for reading data
        dtv_streamer* streamer = mIptvPluginInterface->create_streamer();
        mIptvPluginStreamer = streamer;
        int open_fd = mIptvPluginInterface->open_stream(streamer, transport_desc.c_str());
        if (open_fd < 0) {
            ALOGE("[   INFO   ] could not open stream");
            return ::ndk::ScopedAStatus::fromServiceSpecificError(
                    static_cast<int32_t>(Result::INVALID_ARGUMENT));
        }
        ALOGI("[   INFO   ] open_stream successful, open_fd=%d", open_fd);

        size_t buf_size = 1;
        int timeout_ms = 2000;
        void* buf = malloc(sizeof(char) * buf_size);
        if (buf == nullptr) ALOGI("malloc buf failed [TUNE]");
        ALOGI("[   INFO   ] [Tune] Allocated buffer of size %zu", buf_size);
        mIptvFrontendTuneThread =
                std::thread(&Frontend::readTuneByte, this, streamer, buf, buf_size, timeout_ms);
        if (mIptvFrontendTuneThread.joinable()) mIptvFrontendTuneThread.join();
        free(buf);
    }

    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Frontend::stopTune() {
    ALOGV("%s", __FUNCTION__);

    mTuner->frontendStopTune(mId);
    mIsLocked = false;

    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Frontend::scan(const FrontendSettings& in_settings, FrontendScanType in_type) {
    ALOGV("%s", __FUNCTION__);

    // If it's in middle of scanning, stop it first.
    if (mScanThread.joinable()) {
        mScanThread.join();
    }

    mFrontendSettings = in_settings;
    mFrontendScanType = in_type;
    mScanThread = std::thread(&Frontend::scanThreadLoop, this);

    return ::ndk::ScopedAStatus::ok();
}

void Frontend::setTunerService(std::shared_ptr<Tuner> tuner) {
    mTuner = tuner;
}

void Frontend::scanThreadLoop() {
    if (mIsLocked) {
        FrontendScanMessage msg;
        msg.set<FrontendScanMessage::Tag::isEnd>(true);
        mCallback->onScanMessage(FrontendScanMessageType::END, msg);
        return;
    }

    int64_t frequency = 0;
    switch (mFrontendSettings.getTag()) {
        case FrontendSettings::Tag::analog:
            frequency = mFrontendSettings.get<FrontendSettings::Tag::analog>().frequency;
            break;
        case FrontendSettings::Tag::atsc:
            frequency = mFrontendSettings.get<FrontendSettings::Tag::atsc>().frequency;
            break;
        case FrontendSettings::Tag::atsc3:
            frequency = mFrontendSettings.get<FrontendSettings::Tag::atsc3>().frequency;
            break;
        case FrontendSettings::Tag::dvbs:
            frequency = mFrontendSettings.get<FrontendSettings::Tag::dvbs>().frequency;
            break;
        case FrontendSettings::Tag::dvbc:
            frequency = mFrontendSettings.get<FrontendSettings::Tag::dvbc>().frequency;
            break;
        case FrontendSettings::Tag::dvbt:
            frequency = mFrontendSettings.get<FrontendSettings::Tag::dvbt>().frequency;
            break;
        case FrontendSettings::Tag::isdbs:
            frequency = mFrontendSettings.get<FrontendSettings::Tag::isdbs>().frequency;
            break;
        case FrontendSettings::Tag::isdbs3:
            frequency = mFrontendSettings.get<FrontendSettings::Tag::isdbs3>().frequency;
            break;
        case FrontendSettings::Tag::isdbt:
            frequency = mFrontendSettings.get<FrontendSettings::Tag::isdbt>().frequency;
            break;
        default:
            break;
    }

    if (mFrontendScanType == FrontendScanType::SCAN_BLIND) {
        frequency += 100 * 1000;
    }

    {
        FrontendScanMessage msg;
        vector<int64_t> frequencies = {frequency};
        msg.set<FrontendScanMessage::Tag::frequencies>(frequencies);
        mCallback->onScanMessage(FrontendScanMessageType::FREQUENCY, msg);
    }

    {
        FrontendScanMessage msg;
        msg.set<FrontendScanMessage::Tag::progressPercent>(20);
        mCallback->onScanMessage(FrontendScanMessageType::PROGRESS_PERCENT, msg);
    }

    {
        FrontendScanMessage msg;
        vector<int32_t> symbolRates = {30};
        msg.set<FrontendScanMessage::Tag::symbolRates>(symbolRates);
        mCallback->onScanMessage(FrontendScanMessageType::SYMBOL_RATE, msg);
    }

    if (mType == FrontendType::DVBT) {
        FrontendScanMessage msg;
        msg.set<FrontendScanMessage::Tag::hierarchy>(FrontendDvbtHierarchy::HIERARCHY_NON_NATIVE);
        mCallback->onScanMessage(FrontendScanMessageType::HIERARCHY, msg);
    }

    if (mType == FrontendType::ANALOG) {
        FrontendScanMessage msg;
        msg.set<FrontendScanMessage::Tag::analogType>(FrontendAnalogType::PAL);
        mCallback->onScanMessage(FrontendScanMessageType::ANALOG_TYPE, msg);
    }

    {
        FrontendScanMessage msg;
        vector<int32_t> plpIds = {2};
        msg.set<FrontendScanMessage::Tag::plpIds>(plpIds);
        mCallback->onScanMessage(FrontendScanMessageType::PLP_IDS, msg);
    }

    {
        FrontendScanMessage msg;
        vector<int32_t> groupIds = {3};
        msg.set<FrontendScanMessage::Tag::groupIds>(groupIds);
        mCallback->onScanMessage(FrontendScanMessageType::GROUP_IDS, msg);
    }

    {
        FrontendScanMessage msg;
        vector<int32_t> inputStreamIds = {1};
        msg.set<FrontendScanMessage::Tag::inputStreamIds>(inputStreamIds);
        mCallback->onScanMessage(FrontendScanMessageType::INPUT_STREAM_IDS, msg);
    }

    switch (mType) {
        case FrontendType::DVBT: {
            FrontendScanMessage msg;
            FrontendScanMessageStandard std;
            std.set<FrontendScanMessageStandard::Tag::tStd>(FrontendDvbtStandard::AUTO);
            msg.set<FrontendScanMessage::Tag::std>(std);
            mCallback->onScanMessage(FrontendScanMessageType::STANDARD, msg);
            break;
        }
        case FrontendType::DVBS: {
            FrontendScanMessage msg;
            FrontendScanMessageStandard std;
            std.set<FrontendScanMessageStandard::Tag::sStd>(FrontendDvbsStandard::AUTO);
            msg.set<FrontendScanMessage::Tag::std>(std);
            mCallback->onScanMessage(FrontendScanMessageType::STANDARD, msg);
            break;
        }
        case FrontendType::ANALOG: {
            FrontendScanMessage msg;
            FrontendScanMessageStandard std;
            std.set<FrontendScanMessageStandard::Tag::sifStd>(FrontendAnalogSifStandard::AUTO);
            msg.set<FrontendScanMessage::Tag::std>(std);
            mCallback->onScanMessage(FrontendScanMessageType::STANDARD, msg);
            break;
        }
        default:
            break;
    }

    {
        FrontendScanMessage msg;
        FrontendScanAtsc3PlpInfo info;
        info.plpId = 1;
        info.bLlsFlag = false;
        vector<FrontendScanAtsc3PlpInfo> atsc3PlpInfos = {info};
        msg.set<FrontendScanMessage::Tag::atsc3PlpInfos>(atsc3PlpInfos);
        mCallback->onScanMessage(FrontendScanMessageType::ATSC3_PLP_INFO, msg);
    }

    {
        FrontendScanMessage msg;
        FrontendModulation modulation;
        modulation.set<FrontendModulation::Tag::dvbc>(FrontendDvbcModulation::MOD_16QAM);
        msg.set<FrontendScanMessage::Tag::modulation>(modulation);
        mCallback->onScanMessage(FrontendScanMessageType::MODULATION, msg);
    }

    {
        FrontendScanMessage msg;
        msg.set<FrontendScanMessage::Tag::isHighPriority>(true);
        mCallback->onScanMessage(FrontendScanMessageType::HIGH_PRIORITY, msg);
    }

    if (mType == FrontendType::DVBT) {
        FrontendScanMessage msg;
        vector<int32_t> dvbtCellIds = {0, 1};
        msg.set<FrontendScanMessage::Tag::dvbtCellIds>(dvbtCellIds);
        mCallback->onScanMessage(FrontendScanMessageType::DVBT_CELL_IDS, msg);
    }

    {
        FrontendScanMessage msg;
        msg.set<FrontendScanMessage::Tag::isLocked>(false);
        mCallback->onScanMessage(FrontendScanMessageType::LOCKED, msg);
        mIsLocked = false;
    }

    {
        FrontendScanMessage msg;
        msg.set<FrontendScanMessage::Tag::isLocked>(true);
        mCallback->onScanMessage(FrontendScanMessageType::LOCKED, msg);
        mIsLocked = true;
    }
}

::ndk::ScopedAStatus Frontend::stopScan() {
    ALOGV("%s", __FUNCTION__);

    if (mScanThread.joinable()) {
        mScanThread.join();
    }

    mIsLocked = false;
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Frontend::getStatus(const std::vector<FrontendStatusType>& in_statusTypes,
                                         std::vector<FrontendStatus>* _aidl_return) {
    ALOGV("%s", __FUNCTION__);

    for (int i = 0; i < in_statusTypes.size(); i++) {
        FrontendStatusType type = in_statusTypes[i];
        FrontendStatus status;
        // assign randomly selected values for testing.
        switch (type) {
            case FrontendStatusType::DEMOD_LOCK: {
                status.set<FrontendStatus::isDemodLocked>(true);
                break;
            }
            case FrontendStatusType::SNR: {
                status.set<FrontendStatus::snr>(221);
                break;
            }
            case FrontendStatusType::BER: {
                status.set<FrontendStatus::ber>(1);
                break;
            }
            case FrontendStatusType::PER: {
                status.set<FrontendStatus::per>(2);
                break;
            }
            case FrontendStatusType::PRE_BER: {
                status.set<FrontendStatus::preBer>(3);
                break;
            }
            case FrontendStatusType::SIGNAL_QUALITY: {
                status.set<FrontendStatus::signalQuality>(4);
                break;
            }
            case FrontendStatusType::SIGNAL_STRENGTH: {
                status.set<FrontendStatus::signalStrength>(5);
                break;
            }
            case FrontendStatusType::SYMBOL_RATE: {
                status.set<FrontendStatus::symbolRate>(6);
                break;
            }
            case FrontendStatusType::FEC: {
                status.set<FrontendStatus::innerFec>(FrontendInnerFec::FEC_2_9);  // value = 1 << 7
                break;
            }
            case FrontendStatusType::MODULATION: {
                switch (mType) {
                    case FrontendType::ISDBS: {
                        FrontendModulationStatus modulationStatus;
                        modulationStatus.set<FrontendModulationStatus::Tag::isdbs>(
                                FrontendIsdbsModulation::MOD_BPSK);  // value = 1 << 1
                        status.set<FrontendStatus::modulationStatus>(modulationStatus);
                        break;
                    }
                    case FrontendType::DVBC: {
                        FrontendModulationStatus modulationStatus;
                        modulationStatus.set<FrontendModulationStatus::Tag::dvbc>(
                                FrontendDvbcModulation::MOD_16QAM);  // value = 1 << 1
                        status.set<FrontendStatus::modulationStatus>(modulationStatus);
                        break;
                    }
                    case FrontendType::DVBS: {
                        FrontendModulationStatus modulationStatus;
                        modulationStatus.set<FrontendModulationStatus::Tag::dvbs>(
                                FrontendDvbsModulation::MOD_QPSK);  // value = 1 << 1
                        status.set<FrontendStatus::modulationStatus>(modulationStatus);
                        break;
                    }
                    case FrontendType::ISDBS3: {
                        FrontendModulationStatus modulationStatus;
                        modulationStatus.set<FrontendModulationStatus::Tag::isdbs3>(
                                FrontendIsdbs3Modulation::MOD_BPSK);  // value = 1 << 1
                        status.set<FrontendStatus::modulationStatus>(modulationStatus);
                        break;
                    }
                    case FrontendType::ISDBT: {
                        FrontendModulationStatus modulationStatus;
                        modulationStatus.set<FrontendModulationStatus::Tag::isdbt>(
                                FrontendIsdbtModulation::MOD_DQPSK);  // value = 1 << 1
                        status.set<FrontendStatus::modulationStatus>(modulationStatus);
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            case FrontendStatusType::SPECTRAL: {
                status.set<FrontendStatus::inversion>(FrontendSpectralInversion::NORMAL);
                break;
            }
            case FrontendStatusType::LNB_VOLTAGE: {
                status.set<FrontendStatus::lnbVoltage>(LnbVoltage::VOLTAGE_5V);
                break;
            }
            case FrontendStatusType::PLP_ID: {
                status.set<FrontendStatus::plpId>(101);
                break;
            }
            case FrontendStatusType::EWBS: {
                status.set<FrontendStatus::isEWBS>(false);
                break;
            }
            case FrontendStatusType::AGC: {
                status.set<FrontendStatus::agc>(7);
                break;
            }
            case FrontendStatusType::LNA: {
                status.set<FrontendStatus::isLnaOn>(false);
                break;
            }
            case FrontendStatusType::LAYER_ERROR: {
                vector<bool> v = {false, true, true};
                status.set<FrontendStatus::isLayerError>(v);
                break;
            }
            case FrontendStatusType::MER: {
                status.set<FrontendStatus::mer>(8);
                break;
            }
            case FrontendStatusType::FREQ_OFFSET: {
                status.set<FrontendStatus::freqOffset>(9);
                break;
            }
            case FrontendStatusType::HIERARCHY: {
                status.set<FrontendStatus::hierarchy>(FrontendDvbtHierarchy::HIERARCHY_1_NATIVE);
                break;
            }
            case FrontendStatusType::RF_LOCK: {
                status.set<FrontendStatus::isRfLocked>(false);
                break;
            }
            case FrontendStatusType::ATSC3_PLP_INFO: {
                FrontendStatusAtsc3PlpInfo info1;
                info1.plpId = 3;
                info1.isLocked = false;
                info1.uec = 313;
                FrontendStatusAtsc3PlpInfo info2;
                info2.plpId = 5;
                info2.isLocked = true;
                info2.uec = 515;
                vector<FrontendStatusAtsc3PlpInfo> infos = {info1, info2};
                status.set<FrontendStatus::plpInfo>(infos);
                break;
            }
            case FrontendStatusType::MODULATIONS: {
                FrontendModulation modulation;
                vector<FrontendModulation> modulations;
                switch (mType) {
                    case FrontendType::ISDBS: {
                        modulation.set<FrontendModulation::Tag::isdbs>(
                                FrontendIsdbsModulation::MOD_BPSK);  // value = 1 << 1
                        modulations.push_back(modulation);
                        status.set<FrontendStatus::modulations>(modulations);
                        break;
                    }
                    case FrontendType::DVBC: {
                        modulation.set<FrontendModulation::Tag::dvbc>(
                                FrontendDvbcModulation::MOD_16QAM);  // value = 1 << 1
                        modulations.push_back(modulation);
                        status.set<FrontendStatus::modulations>(modulations);
                        break;
                    }
                    case FrontendType::DVBS: {
                        modulation.set<FrontendModulation::Tag::dvbs>(
                                FrontendDvbsModulation::MOD_QPSK);  // value = 1 << 1
                        modulations.push_back(modulation);
                        status.set<FrontendStatus::modulations>(modulations);
                        break;
                    }
                    case FrontendType::DVBT: {
                        modulation.set<FrontendModulation::Tag::dvbt>(
                                FrontendDvbtConstellation::CONSTELLATION_16QAM_R);  // value = 1 <<
                                                                                    // 16
                        modulations.push_back(modulation);
                        status.set<FrontendStatus::modulations>(modulations);
                        break;
                    }
                    case FrontendType::ISDBS3: {
                        modulation.set<FrontendModulation::Tag::isdbs3>(
                                FrontendIsdbs3Modulation::MOD_BPSK);  //  value = 1 << 1
                        modulations.push_back(modulation);
                        status.set<FrontendStatus::modulations>(modulations);
                        break;
                    }
                    case FrontendType::ISDBT: {
                        modulation.set<FrontendModulation::Tag::isdbt>(
                                FrontendIsdbtModulation::MOD_DQPSK);  // value = 1 << 1
                        modulations.push_back(modulation);
                        status.set<FrontendStatus::modulations>(modulations);
                        break;
                    }
                    case FrontendType::ATSC: {
                        modulation.set<FrontendModulation::Tag::atsc>(
                                FrontendAtscModulation::MOD_8VSB);  // value = 1 << 2
                        modulations.push_back(modulation);
                        status.set<FrontendStatus::modulations>(modulations);
                        break;
                    }
                    case FrontendType::ATSC3: {
                        modulation.set<FrontendModulation::Tag::atsc3>(
                                FrontendAtsc3Modulation::MOD_QPSK);  // value = 1 << 1
                        modulations.push_back(modulation);
                        status.set<FrontendStatus::modulations>(modulations);
                        break;
                    }
                    case FrontendType::DTMB: {
                        modulation.set<FrontendModulation::Tag::dtmb>(
                                FrontendDtmbModulation::CONSTELLATION_4QAM);  // value = 1 << 1
                        modulations.push_back(modulation);
                        status.set<FrontendStatus::modulations>(modulations);
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            case FrontendStatusType::BERS: {
                vector<int32_t> bers = {1};
                status.set<FrontendStatus::bers>(bers);
                break;
            }
            case FrontendStatusType::CODERATES: {
                vector<FrontendInnerFec> rates;
                rates.push_back(FrontendInnerFec::FEC_6_15);  // value = 1 << 39
                status.set<FrontendStatus::codeRates>(rates);
                break;
            }
            case FrontendStatusType::BANDWIDTH: {
                FrontendBandwidth bandwidth;
                switch (mType) {
                    case FrontendType::DVBC: {
                        bandwidth.set<FrontendBandwidth::Tag::dvbc>(
                                FrontendDvbcBandwidth::BANDWIDTH_6MHZ);  // value = 1 << 1
                        status.set<FrontendStatus::bandwidth>(bandwidth);
                        break;
                    }
                    case FrontendType::DVBT: {
                        bandwidth.set<FrontendBandwidth::Tag::dvbt>(
                                FrontendDvbtBandwidth::BANDWIDTH_8MHZ);  // value = 1 << 1
                        status.set<FrontendStatus::bandwidth>(bandwidth);
                        break;
                    }
                    case FrontendType::ISDBT: {
                        bandwidth.set<FrontendBandwidth::Tag::isdbt>(
                                FrontendIsdbtBandwidth::BANDWIDTH_8MHZ);  // value = 1 << 1
                        status.set<FrontendStatus::bandwidth>(bandwidth);
                        break;
                    }
                    case FrontendType::ATSC3: {
                        bandwidth.set<FrontendBandwidth::Tag::atsc3>(
                                FrontendAtsc3Bandwidth::BANDWIDTH_6MHZ);  // value = 1 << 1
                        status.set<FrontendStatus::bandwidth>(bandwidth);
                        break;
                    }
                    case FrontendType::DTMB: {
                        bandwidth.set<FrontendBandwidth::Tag::dtmb>(
                                FrontendDtmbBandwidth::BANDWIDTH_8MHZ);  // value = 1 << 1
                        status.set<FrontendStatus::bandwidth>(bandwidth);
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            case FrontendStatusType::GUARD_INTERVAL: {
                FrontendGuardInterval interval;
                switch (mType) {
                    case FrontendType::DVBT: {
                        interval.set<FrontendGuardInterval::Tag::dvbt>(
                                FrontendDvbtGuardInterval::INTERVAL_1_32);  // value = 1 << 1
                        status.set<FrontendStatus::interval>(interval);
                        break;
                    }
                    case FrontendType::ISDBT: {
                        interval.set<FrontendGuardInterval::Tag::isdbt>(
                                FrontendIsdbtGuardInterval::INTERVAL_1_32);  // value = 1 << 1
                        status.set<FrontendStatus::interval>(interval);
                        break;
                    }
                    case FrontendType::DTMB: {
                        interval.set<FrontendGuardInterval::Tag::dtmb>(
                                FrontendDtmbGuardInterval::PN_420_VARIOUS);  // value = 1 << 1
                        status.set<FrontendStatus::interval>(interval);
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            case FrontendStatusType::TRANSMISSION_MODE: {
                FrontendTransmissionMode transMode;
                switch (mType) {
                    case FrontendType::DVBT: {
                        transMode.set<FrontendTransmissionMode::Tag::dvbt>(
                                FrontendDvbtTransmissionMode::MODE_16K_E);  // value = 1 << 8
                        status.set<FrontendStatus::transmissionMode>(transMode);
                        break;
                    }
                    case FrontendType::ISDBT: {
                        transMode.set<FrontendTransmissionMode::Tag::isdbt>(
                                FrontendIsdbtMode::MODE_1);  // value = 1 << 1
                        status.set<FrontendStatus::transmissionMode>(transMode);
                        break;
                    }
                    case FrontendType::DTMB: {
                        transMode.set<FrontendTransmissionMode::Tag::dtmb>(
                                FrontendDtmbTransmissionMode::C1);  // value = 1 << 1
                        status.set<FrontendStatus::transmissionMode>(transMode);
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            case FrontendStatusType::UEC: {
                status.set<FrontendStatus::uec>(4);
                break;
            }
            case FrontendStatusType::T2_SYSTEM_ID: {
                status.set<FrontendStatus::systemId>(5);
                break;
            }
            case FrontendStatusType::INTERLEAVINGS: {
                FrontendInterleaveMode interleave;
                vector<FrontendInterleaveMode> interleaves;
                switch (mType) {
                    case FrontendType::DVBC: {
                        // value = 1 << 1
                        interleave.set<FrontendInterleaveMode::Tag::dvbc>(
                                FrontendCableTimeInterleaveMode::INTERLEAVING_128_1_0);
                        interleaves.push_back(interleave);
                        status.set<FrontendStatus::interleaving>(interleaves);
                        break;
                    }
                    case FrontendType::ATSC3: {
                        interleave.set<FrontendInterleaveMode::Tag::atsc3>(
                                FrontendAtsc3TimeInterleaveMode::CTI);  // value = 1 << 1
                        interleaves.push_back(interleave);
                        status.set<FrontendStatus::interleaving>(interleaves);
                        break;
                    }
                    case FrontendType::DTMB: {
                        interleave.set<FrontendInterleaveMode::Tag::dtmb>(
                                FrontendDtmbTimeInterleaveMode::TIMER_INT_240);  // value = 1 << 1
                        interleaves.push_back(interleave);
                        status.set<FrontendStatus::interleaving>(interleaves);
                        break;
                    }
                    case FrontendType::ISDBT: {
                        interleave.set<FrontendInterleaveMode::Tag::isdbt>(
                                FrontendIsdbtTimeInterleaveMode::INTERLEAVE_1_0); // value = 1 << 1
                        interleaves.push_back(interleave);
                        status.set<FrontendStatus::interleaving>(interleaves);
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            case FrontendStatusType::ISDBT_SEGMENTS: {
                vector<int32_t> segments = {2, 3};
                status.set<FrontendStatus::isdbtSegment>(segments);
                break;
            }
            case FrontendStatusType::TS_DATA_RATES: {
                vector<int32_t> dataRates = {4, 5};
                status.set<FrontendStatus::tsDataRate>(dataRates);
                break;
            }
            case FrontendStatusType::ROLL_OFF: {
                FrontendRollOff rollOff;
                switch (mType) {
                    case FrontendType::DVBS: {
                        rollOff.set<FrontendRollOff::Tag::dvbs>(
                                FrontendDvbsRolloff::ROLLOFF_0_35);  // value = 1
                        status.set<FrontendStatus::rollOff>(rollOff);
                        break;
                    }
                    case FrontendType::ISDBS: {
                        rollOff.set<FrontendRollOff::Tag::isdbs>(
                                FrontendIsdbsRolloff::ROLLOFF_0_35);  // value = 1
                        status.set<FrontendStatus::rollOff>(rollOff);
                        break;
                    }
                    case FrontendType::ISDBS3: {
                        rollOff.set<FrontendRollOff::Tag::isdbs3>(
                                FrontendIsdbs3Rolloff::ROLLOFF_0_03);  // value = 1
                        status.set<FrontendStatus::rollOff>(rollOff);
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            case FrontendStatusType::IS_MISO: {
                status.set<FrontendStatus::isMiso>(true);
                break;
            }
            case FrontendStatusType::IS_LINEAR: {
                status.set<FrontendStatus::isLinear>(true);
                break;
            }
            case FrontendStatusType::IS_SHORT_FRAMES: {
                status.set<FrontendStatus::isShortFrames>(true);
                break;
            }
            case FrontendStatusType::ISDBT_MODE: {
                status.set<FrontendStatus::isdbtMode>(FrontendIsdbtMode::AUTO);
                break;
            }
            case FrontendStatusType::ISDBT_PARTIAL_RECEPTION_FLAG: {
                status.set<FrontendStatus::partialReceptionFlag>(
                        FrontendIsdbtPartialReceptionFlag::AUTO);
                break;
            }
            case FrontendStatusType::STREAM_ID_LIST: {
                vector<int32_t> streamIds = {0, 1};
                status.set<FrontendStatus::streamIdList>(streamIds);
                break;
            }
            case FrontendStatusType::DVBT_CELL_IDS: {
                vector<int32_t> dvbtCellIds = {0, 1};
                status.set<FrontendStatus::dvbtCellIds>(dvbtCellIds);
                break;
            }
            case FrontendStatusType::ATSC3_ALL_PLP_INFO: {
                FrontendScanAtsc3PlpInfo info1;
                info1.plpId = 1;
                info1.bLlsFlag = false;
                FrontendScanAtsc3PlpInfo info2;
                info2.plpId = 2;
                info2.bLlsFlag = true;
                FrontendScanAtsc3PlpInfo info3;
                info3.plpId = 3;
                info3.bLlsFlag = false;
                vector<FrontendScanAtsc3PlpInfo> infos = {info1, info2, info3};
                status.set<FrontendStatus::allPlpInfo>(infos);
                break;
            }
            case FrontendStatusType::IPTV_CONTENT_URL: {
                status.set<FrontendStatus::iptvContentUrl>("");
                break;
            }
            case FrontendStatusType::IPTV_PACKETS_LOST: {
                status.set<FrontendStatus::iptvPacketsLost>(5);
                break;
            }
            case FrontendStatusType::IPTV_PACKETS_RECEIVED: {
                status.set<FrontendStatus::iptvPacketsReceived>(5);
                break;
            }
            case FrontendStatusType::IPTV_WORST_JITTER_MS: {
                status.set<FrontendStatus::iptvWorstJitterMs>(5);
                break;
            }
            case FrontendStatusType::IPTV_AVERAGE_JITTER_MS: {
                status.set<FrontendStatus::iptvAverageJitterMs>(5);
                break;
            }
            default: {
                continue;
            }
        }
        _aidl_return->push_back(status);
    }

    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Frontend::setLnb(int32_t /* in_lnbId */) {
    ALOGV("%s", __FUNCTION__);
    if (!supportsSatellite()) {
        return ::ndk::ScopedAStatus::fromServiceSpecificError(
                static_cast<int32_t>(Result::INVALID_STATE));
    }
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Frontend::linkCiCam(int32_t in_ciCamId, int32_t* _aidl_return) {
    ALOGV("%s", __FUNCTION__);

    mCiCamId = in_ciCamId;
    *_aidl_return = 0;

    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Frontend::unlinkCiCam(int32_t /* in_ciCamId */) {
    ALOGV("%s", __FUNCTION__);

    mCiCamId = -1;

    return ::ndk::ScopedAStatus::ok();
}

binder_status_t Frontend::dump(int fd, const char** /* args */, uint32_t /* numArgs */) {
    dprintf(fd, "  Frontend %d\n", mId);
    dprintf(fd, "    mType: %d\n", mType);
    dprintf(fd, "    mIsLocked: %d\n", mIsLocked);
    dprintf(fd, "    mCiCamId: %d\n", mCiCamId);
    dprintf(fd, "    mFrontendStatusCaps:");
    for (int i = 0; i < mFrontendStatusCaps.size(); i++) {
        dprintf(fd, "        %d\n", mFrontendStatusCaps[i]);
    }
    return STATUS_OK;
}

::ndk::ScopedAStatus Frontend::getHardwareInfo(std::string* _aidl_return) {
    ALOGV("%s", __FUNCTION__);

    *_aidl_return = "Sample Frontend";
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Frontend::removeOutputPid(int32_t /* in_pid */) {
    ALOGV("%s", __FUNCTION__);

    return ::ndk::ScopedAStatus::fromServiceSpecificError(
            static_cast<int32_t>(Result::UNAVAILABLE));
}

::ndk::ScopedAStatus Frontend::getFrontendStatusReadiness(
        const std::vector<FrontendStatusType>& in_statusTypes,
        std::vector<FrontendStatusReadiness>* _aidl_return) {
    ALOGV("%s", __FUNCTION__);

    _aidl_return->resize(in_statusTypes.size());
    for (int i = 0; i < in_statusTypes.size(); i++) {
        int j = 0;
        while (j < mFrontendStatusCaps.size()) {
            if (in_statusTypes[i] == mFrontendStatusCaps[j]) {
                (*_aidl_return)[i] = FrontendStatusReadiness::STABLE;
                break;
            }
            j++;
        }
        if (j >= mFrontendStatusCaps.size()) {
            (*_aidl_return)[i] = FrontendStatusReadiness::UNSUPPORTED;
        }
    }

    return ::ndk::ScopedAStatus::ok();
}

FrontendType Frontend::getFrontendType() {
    return mType;
}

int32_t Frontend::getFrontendId() {
    return mId;
}

dtv_plugin* Frontend::getIptvPluginInterface() {
    return mIptvPluginInterface;
}

string Frontend::getIptvTransportDescription() {
    return mIptvTransportDescription;
}

dtv_streamer* Frontend::getIptvPluginStreamer() {
    return mIptvPluginStreamer;
}

bool Frontend::supportsSatellite() {
    return mType == FrontendType::DVBS || mType == FrontendType::ISDBS ||
           mType == FrontendType::ISDBS3;
}

bool Frontend::isLocked() {
    return mIsLocked;
}

void Frontend::getFrontendInfo(FrontendInfo* _aidl_return) {
    // assign randomly selected values for testing.
    *_aidl_return = {
            .type = mType,
            .minFrequency = 139000000,
            .maxFrequency = 1139000000,
            .minSymbolRate = 45,
            .maxSymbolRate = 1145,
            .acquireRange = 30,
            .exclusiveGroupId = 57,
            .statusCaps = mFrontendStatusCaps,
            .frontendCaps = mFrontendCaps,
    };
}

}  // namespace tuner
}  // namespace tv
}  // namespace hardware
}  // namespace android
}  // namespace aidl
