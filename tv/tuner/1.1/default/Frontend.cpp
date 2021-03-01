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

#define LOG_TAG "android.hardware.tv.tuner@1.1-Frontend"

#include "Frontend.h"
#include <android/hardware/tv/tuner/1.1/IFrontendCallback.h>
#include <utils/Log.h>

namespace android {
namespace hardware {
namespace tv {
namespace tuner {
namespace V1_0 {
namespace implementation {

Frontend::Frontend(FrontendType type, FrontendId id, sp<Tuner> tuner) {
    mType = type;
    mId = id;
    mTunerService = tuner;
    // Init callback to nullptr
    mCallback = nullptr;
}

Frontend::~Frontend() {}

Return<Result> Frontend::close() {
    ALOGV("%s", __FUNCTION__);
    // Reset callback
    mCallback = nullptr;
    mIsLocked = false;
    mTunerService->removeFrontend(mId);

    return Result::SUCCESS;
}

Return<Result> Frontend::setCallback(const sp<IFrontendCallback>& callback) {
    ALOGV("%s", __FUNCTION__);
    if (callback == nullptr) {
        ALOGW("[   WARN   ] Set Frontend callback with nullptr");
        return Result::INVALID_ARGUMENT;
    }

    mCallback = callback;
    return Result::SUCCESS;
}

Return<Result> Frontend::tune(const FrontendSettings& /* settings */) {
    ALOGV("%s", __FUNCTION__);
    if (mCallback == nullptr) {
        ALOGW("[   WARN   ] Frontend callback is not set when tune");
        return Result::INVALID_STATE;
    }

    mTunerService->frontendStartTune(mId);
    mCallback->onEvent(FrontendEventType::LOCKED);
    mIsLocked = true;
    return Result::SUCCESS;
}

Return<Result> Frontend::tune_1_1(const FrontendSettings& settings,
                                  const V1_1::FrontendSettingsExt1_1& /*settingsExt1_1*/) {
    ALOGV("%s", __FUNCTION__);
    return tune(settings);
}

Return<Result> Frontend::stopTune() {
    ALOGV("%s", __FUNCTION__);

    mTunerService->frontendStopTune(mId);
    mIsLocked = false;

    return Result::SUCCESS;
}

Return<Result> Frontend::scan(const FrontendSettings& settings, FrontendScanType type) {
    ALOGV("%s", __FUNCTION__);
    FrontendScanMessage msg;

    if (mIsLocked) {
        msg.isEnd(true);
        mCallback->onScanMessage(FrontendScanMessageType::END, msg);
        return Result::SUCCESS;
    }

    uint32_t frequency;
    switch (settings.getDiscriminator()) {
        case FrontendSettings::hidl_discriminator::analog:
            frequency = settings.analog().frequency;
            break;
        case FrontendSettings::hidl_discriminator::atsc:
            frequency = settings.atsc().frequency;
            break;
        case FrontendSettings::hidl_discriminator::atsc3:
            frequency = settings.atsc3().frequency;
            break;
        case FrontendSettings::hidl_discriminator::dvbs:
            frequency = settings.dvbs().frequency;
            break;
        case FrontendSettings::hidl_discriminator::dvbc:
            frequency = settings.dvbc().frequency;
            break;
        case FrontendSettings::hidl_discriminator::dvbt:
            frequency = settings.dvbt().frequency;
            break;
        case FrontendSettings::hidl_discriminator::isdbs:
            frequency = settings.isdbs().frequency;
            break;
        case FrontendSettings::hidl_discriminator::isdbs3:
            frequency = settings.isdbs3().frequency;
            break;
        case FrontendSettings::hidl_discriminator::isdbt:
            frequency = settings.isdbt().frequency;
            break;
    }

    if (type == FrontendScanType::SCAN_BLIND) {
        frequency += 100;
    }

    msg.frequencies({frequency});
    mCallback->onScanMessage(FrontendScanMessageType::FREQUENCY, msg);

    msg.progressPercent(20);
    mCallback->onScanMessage(FrontendScanMessageType::PROGRESS_PERCENT, msg);

    msg.symbolRates({30});
    mCallback->onScanMessage(FrontendScanMessageType::SYMBOL_RATE, msg);

    if (mType == FrontendType::DVBT) {
        msg.hierarchy(FrontendDvbtHierarchy::HIERARCHY_NON_NATIVE);
        mCallback->onScanMessage(FrontendScanMessageType::HIERARCHY, msg);
    }

    if (mType == FrontendType::ANALOG) {
        msg.analogType(FrontendAnalogType::PAL);
        mCallback->onScanMessage(FrontendScanMessageType::ANALOG_TYPE, msg);
    }

    msg.plpIds({3});
    mCallback->onScanMessage(FrontendScanMessageType::PLP_IDS, msg);

    msg.groupIds({2});
    mCallback->onScanMessage(FrontendScanMessageType::GROUP_IDS, msg);

    msg.inputStreamIds({1});
    mCallback->onScanMessage(FrontendScanMessageType::INPUT_STREAM_IDS, msg);

    FrontendScanMessage::Standard s;
    switch (mType) {
        case FrontendType::DVBT:
            s.tStd(FrontendDvbtStandard::AUTO);
            msg.std(s);
            mCallback->onScanMessage(FrontendScanMessageType::STANDARD, msg);
            break;
        case FrontendType::DVBS:
            s.sStd(FrontendDvbsStandard::AUTO);
            msg.std(s);
            mCallback->onScanMessage(FrontendScanMessageType::STANDARD, msg);
            break;
        case FrontendType::ANALOG:
            s.sifStd(FrontendAnalogSifStandard::AUTO);
            msg.std(s);
            mCallback->onScanMessage(FrontendScanMessageType::STANDARD, msg);
            break;
        default:
            break;
    }

    FrontendScanAtsc3PlpInfo info{
            .plpId = 1,
            .bLlsFlag = false,
    };
    msg.atsc3PlpInfos({info});
    mCallback->onScanMessage(FrontendScanMessageType::ATSC3_PLP_INFO, msg);

    sp<V1_1::IFrontendCallback> frontendCallback_v1_1 =
            V1_1::IFrontendCallback::castFrom(mCallback);
    if (frontendCallback_v1_1 != NULL) {
        V1_1::FrontendScanMessageExt1_1 msg;
        msg.modulation().dvbc(FrontendDvbcModulation::MOD_16QAM);
        frontendCallback_v1_1->onScanMessageExt1_1(V1_1::FrontendScanMessageTypeExt1_1::MODULATION,
                                                   msg);
        msg.isHighPriority(true);
        frontendCallback_v1_1->onScanMessageExt1_1(
                V1_1::FrontendScanMessageTypeExt1_1::HIGH_PRIORITY, msg);
    } else {
        ALOGD("[Frontend] Couldn't cast to V1_1 IFrontendCallback");
    }

    msg.isLocked(true);
    mCallback->onScanMessage(FrontendScanMessageType::LOCKED, msg);
    mIsLocked = true;

    return Result::SUCCESS;
}

Return<Result> Frontend::scan_1_1(const FrontendSettings& settings, FrontendScanType type,
                                  const V1_1::FrontendSettingsExt1_1& settingsExt1_1) {
    ALOGV("%s", __FUNCTION__);
    ALOGD("[Frontend] scan_1_1 end frequency %d", settingsExt1_1.endFrequency);
    return scan(settings, type);
}

Return<Result> Frontend::stopScan() {
    ALOGV("%s", __FUNCTION__);

    mIsLocked = false;
    return Result::SUCCESS;
}

Return<void> Frontend::getStatus(const hidl_vec<FrontendStatusType>& statusTypes,
                                 getStatus_cb _hidl_cb) {
    ALOGV("%s", __FUNCTION__);

    vector<FrontendStatus> statuses;
    for (int i = 0; i < statusTypes.size(); i++) {
        FrontendStatusType type = statusTypes[i];
        FrontendStatus status;
        // assign randomly selected values for testing.
        switch (type) {
            case FrontendStatusType::DEMOD_LOCK: {
                status.isDemodLocked(true);
                break;
            }
            case FrontendStatusType::SNR: {
                status.snr(221);
                break;
            }
            case FrontendStatusType::BER: {
                status.ber(1);
                break;
            }
            case FrontendStatusType::PER: {
                status.per(2);
                break;
            }
            case FrontendStatusType::PRE_BER: {
                status.preBer(3);
                break;
            }
            case FrontendStatusType::SIGNAL_QUALITY: {
                status.signalQuality(4);
                break;
            }
            case FrontendStatusType::SIGNAL_STRENGTH: {
                status.signalStrength(5);
                break;
            }
            case FrontendStatusType::SYMBOL_RATE: {
                status.symbolRate(6);
                break;
            }
            case FrontendStatusType::FEC: {
                status.innerFec(FrontendInnerFec::FEC_2_9);  // value = 1 << 7
                break;
            }
            case FrontendStatusType::MODULATION: {
                FrontendModulationStatus modulationStatus;
                switch (mType) {
                    case FrontendType::ISDBS: {
                        modulationStatus.isdbs(
                                FrontendIsdbsModulation::MOD_BPSK);  // value = 1 << 1
                        status.modulation(modulationStatus);
                        break;
                    }
                    case FrontendType::DVBC: {
                        modulationStatus.dvbc(FrontendDvbcModulation::MOD_16QAM);  // value = 1 << 1
                        status.modulation(modulationStatus);
                        break;
                    }
                    case FrontendType::DVBS: {
                        modulationStatus.dvbs(FrontendDvbsModulation::MOD_QPSK);  // value = 1 << 1
                        status.modulation(modulationStatus);
                        break;
                    }
                    case FrontendType::ISDBS3: {
                        modulationStatus.isdbs3(
                                FrontendIsdbs3Modulation::MOD_BPSK);  // value = 1 << 1
                        status.modulation(modulationStatus);
                        break;
                    }
                    case FrontendType::ISDBT: {
                        modulationStatus.isdbt(
                                FrontendIsdbtModulation::MOD_DQPSK);  // value = 1 << 1
                        status.modulation(modulationStatus);
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            case FrontendStatusType::SPECTRAL: {
                status.inversion(FrontendDvbcSpectralInversion::NORMAL);
                break;
            }
            case FrontendStatusType::LNB_VOLTAGE: {
                status.lnbVoltage(LnbVoltage::VOLTAGE_5V);
                break;
            }
            case FrontendStatusType::PLP_ID: {
                status.plpId(101);  // type uint8_t
                break;
            }
            case FrontendStatusType::EWBS: {
                status.isEWBS(false);
                break;
            }
            case FrontendStatusType::AGC: {
                status.agc(7);
                break;
            }
            case FrontendStatusType::LNA: {
                status.isLnaOn(false);
                break;
            }
            case FrontendStatusType::LAYER_ERROR: {
                vector<bool> v = {false, true, true};
                status.isLayerError(v);
                break;
            }
            case FrontendStatusType::MER: {
                status.mer(8);
                break;
            }
            case FrontendStatusType::FREQ_OFFSET: {
                status.freqOffset(9);
                break;
            }
            case FrontendStatusType::HIERARCHY: {
                status.hierarchy(FrontendDvbtHierarchy::HIERARCHY_1_NATIVE);
                break;
            }
            case FrontendStatusType::RF_LOCK: {
                status.isRfLocked(false);
                break;
            }
            case FrontendStatusType::ATSC3_PLP_INFO: {
                vector<FrontendStatusAtsc3PlpInfo> v;
                FrontendStatusAtsc3PlpInfo info1{
                        .plpId = 3,
                        .isLocked = false,
                        .uec = 313,
                };
                FrontendStatusAtsc3PlpInfo info2{
                        .plpId = 5,
                        .isLocked = true,
                        .uec = 515,
                };
                v.push_back(info1);
                v.push_back(info2);
                status.plpInfo(v);
                break;
            }
            default: {
                continue;
            }
        }
        statuses.push_back(status);
    }
    _hidl_cb(Result::SUCCESS, statuses);

    return Void();
}

Return<void> Frontend::getStatusExt1_1(const hidl_vec<V1_1::FrontendStatusTypeExt1_1>& statusTypes,
                                       V1_1::IFrontend::getStatusExt1_1_cb _hidl_cb) {
    ALOGV("%s", __FUNCTION__);

    vector<V1_1::FrontendStatusExt1_1> statuses;
    for (int i = 0; i < statusTypes.size(); i++) {
        V1_1::FrontendStatusTypeExt1_1 type = statusTypes[i];
        V1_1::FrontendStatusExt1_1 status;

        switch (type) {
            case V1_1::FrontendStatusTypeExt1_1::MODULATIONS: {
                vector<V1_1::FrontendModulation> modulations;
                V1_1::FrontendModulation modulation;
                switch ((int)mType) {
                    case (int)FrontendType::ISDBS: {
                        modulation.isdbs(FrontendIsdbsModulation::MOD_BPSK);  // value = 1 << 1
                        modulations.push_back(modulation);
                        status.modulations(modulations);
                        break;
                    }
                    case (int)FrontendType::DVBC: {
                        modulation.dvbc(FrontendDvbcModulation::MOD_16QAM);  // value = 1 << 1
                        modulations.push_back(modulation);
                        status.modulations(modulations);
                        break;
                    }
                    case (int)FrontendType::DVBS: {
                        modulation.dvbs(FrontendDvbsModulation::MOD_QPSK);  // value = 1 << 1
                        modulations.push_back(modulation);
                        status.modulations(modulations);
                        break;
                    }
                    case (int)FrontendType::DVBT: {
                        // value = 1 << 16
                        modulation.dvbt(V1_1::FrontendDvbtConstellation::CONSTELLATION_16QAM_R);
                        modulations.push_back(modulation);
                        status.modulations(modulations);
                        break;
                    }
                    case (int)FrontendType::ISDBS3: {
                        modulation.isdbs3(FrontendIsdbs3Modulation::MOD_BPSK);  //  value = 1 << 1
                        modulations.push_back(modulation);
                        status.modulations(modulations);
                        break;
                    }
                    case (int)FrontendType::ISDBT: {
                        modulation.isdbt(FrontendIsdbtModulation::MOD_DQPSK);  // value = 1 << 1
                        modulations.push_back(modulation);
                        status.modulations(modulations);
                        break;
                    }
                    case (int)FrontendType::ATSC: {
                        modulation.atsc(FrontendAtscModulation::MOD_8VSB);  // value = 1 << 2
                        modulations.push_back(modulation);
                        status.modulations(modulations);
                        break;
                    }
                    case (int)FrontendType::ATSC3: {
                        modulation.atsc3(FrontendAtsc3Modulation::MOD_QPSK);  // value = 1 << 1
                        modulations.push_back(modulation);
                        status.modulations(modulations);
                        break;
                    }
                    case (int)V1_1::FrontendType::DTMB: {
                        // value = 1 << 1
                        modulation.dtmb(V1_1::FrontendDtmbModulation::CONSTELLATION_4QAM);
                        modulations.push_back(modulation);
                        status.modulations(modulations);
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            case V1_1::FrontendStatusTypeExt1_1::BERS: {
                vector<uint32_t> bers = {1};
                status.bers(bers);
                break;
            }
            case V1_1::FrontendStatusTypeExt1_1::CODERATES: {
                // value = 1 << 39
                vector<V1_1::FrontendInnerFec> codeRates = {V1_1::FrontendInnerFec::FEC_6_15};
                status.codeRates(codeRates);
                break;
            }
            case V1_1::FrontendStatusTypeExt1_1::BANDWIDTH: {
                V1_1::FrontendBandwidth bandwidth;
                switch ((int)mType) {
                    case (int)FrontendType::DVBC: {
                        // value = 1 << 1
                        bandwidth.dvbc(V1_1::FrontendDvbcBandwidth::BANDWIDTH_6MHZ);
                        status.bandwidth(bandwidth);
                        break;
                    }
                    case (int)FrontendType::DVBT: {
                        // value = 1 << 1
                        bandwidth.dvbt(FrontendDvbtBandwidth::BANDWIDTH_8MHZ);
                        status.bandwidth(bandwidth);
                        break;
                    }
                    case (int)FrontendType::ISDBT: {
                        bandwidth.isdbt(FrontendIsdbtBandwidth::BANDWIDTH_8MHZ);  // value = 1 << 1
                        status.bandwidth(bandwidth);
                        break;
                    }
                    case (int)FrontendType::ATSC3: {
                        bandwidth.atsc3(FrontendAtsc3Bandwidth::BANDWIDTH_6MHZ);  // value = 1 << 1
                        status.bandwidth(bandwidth);
                        break;
                    }
                    case (int)V1_1::FrontendType::DTMB: {
                        // value = 1 << 1
                        bandwidth.dtmb(V1_1::FrontendDtmbBandwidth::BANDWIDTH_8MHZ);
                        status.bandwidth(bandwidth);
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            case V1_1::FrontendStatusTypeExt1_1::GUARD_INTERVAL: {
                V1_1::FrontendGuardInterval interval;
                switch ((int)mType) {
                    case (int)FrontendType::DVBT: {
                        interval.dvbt(FrontendDvbtGuardInterval::INTERVAL_1_32);  // value = 1 << 1
                        status.interval(interval);
                        break;
                    }
                    case (int)FrontendType::ISDBT: {
                        interval.isdbt(FrontendDvbtGuardInterval::INTERVAL_1_32);  // value = 1 << 1
                        status.interval(interval);
                        break;
                    }
                    case (int)V1_1::FrontendType::DTMB: {
                        // value = 1 << 1
                        interval.dtmb(V1_1::FrontendDtmbGuardInterval::PN_420_VARIOUS);
                        status.interval(interval);
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            case V1_1::FrontendStatusTypeExt1_1::TRANSMISSION_MODE: {
                V1_1::FrontendTransmissionMode transMode;
                switch ((int)mType) {
                    case (int)FrontendType::DVBT: {
                        // value = 1 << 8
                        transMode.dvbt(V1_1::FrontendDvbtTransmissionMode::MODE_16K_E);
                        status.transmissionMode(transMode);
                        break;
                    }
                    case (int)FrontendType::ISDBT: {
                        transMode.isdbt(FrontendIsdbtMode::MODE_1);  // value = 1 << 1
                        status.transmissionMode(transMode);
                        break;
                    }
                    case (int)V1_1::FrontendType::DTMB: {
                        transMode.dtmb(V1_1::FrontendDtmbTransmissionMode::C1);  // value = 1 << 1
                        status.transmissionMode(transMode);
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            case V1_1::FrontendStatusTypeExt1_1::UEC: {
                status.uec(4);
                break;
            }
            case V1_1::FrontendStatusTypeExt1_1::T2_SYSTEM_ID: {
                status.systemId(5);
                break;
            }
            case V1_1::FrontendStatusTypeExt1_1::INTERLEAVINGS: {
                V1_1::FrontendInterleaveMode interleave;
                switch ((int)mType) {
                    case (int)FrontendType::DVBC: {
                        // value = 1 << 1
                        interleave.dvbc(
                                V1_1::FrontendCableTimeInterleaveMode::INTERLEAVING_128_1_0);
                        vector<V1_1::FrontendInterleaveMode> interleaving = {interleave};
                        status.interleaving(interleaving);
                        break;
                    }
                    case (int)FrontendType::ATSC3: {
                        // value = 1 << 1
                        interleave.atsc3(FrontendAtsc3TimeInterleaveMode::CTI);
                        vector<V1_1::FrontendInterleaveMode> interleaving = {interleave};
                        status.interleaving(interleaving);
                        break;
                    }
                    case (int)V1_1::FrontendType::DTMB: {
                        // value = 1 << 1
                        interleave.dtmb(V1_1::FrontendDtmbTimeInterleaveMode::TIMER_INT_240);
                        vector<V1_1::FrontendInterleaveMode> interleaving = {interleave};
                        status.interleaving(interleaving);
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            case V1_1::FrontendStatusTypeExt1_1::ISDBT_SEGMENTS: {
                vector<uint8_t> segments = {2, 3};
                status.isdbtSegment(segments);
                break;
            }
            case V1_1::FrontendStatusTypeExt1_1::TS_DATA_RATES: {
                vector<uint32_t> dataRates = {4, 5};
                status.tsDataRate(dataRates);
                break;
            }
            case V1_1::FrontendStatusTypeExt1_1::ROLL_OFF: {
                V1_1::FrontendRollOff rollOff;
                switch (mType) {
                    case FrontendType::DVBS: {
                        // value = 1
                        rollOff.dvbs(FrontendDvbsRolloff::ROLLOFF_0_35);
                        status.rollOff(rollOff);
                        break;
                    }
                    case FrontendType::ISDBS: {
                        // value = 1
                        rollOff.isdbs(FrontendIsdbsRolloff::ROLLOFF_0_35);
                        status.rollOff(rollOff);
                        break;
                    }
                    case FrontendType::ISDBS3: {
                        // value = 1
                        rollOff.isdbs3(FrontendIsdbs3Rolloff::ROLLOFF_0_03);
                        status.rollOff(rollOff);
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            case V1_1::FrontendStatusTypeExt1_1::IS_MISO: {
                status.isMiso(true);
                break;
            }
            case V1_1::FrontendStatusTypeExt1_1::IS_LINEAR: {
                status.isLinear(true);
                break;
            }
            case V1_1::FrontendStatusTypeExt1_1::IS_SHORT_FRAMES: {
                status.isShortFrames(true);
                break;
            }
            default: {
                continue;
            }
        }
        statuses.push_back(status);
    }
    _hidl_cb(Result::SUCCESS, statuses);

    return Void();
}

Return<Result> Frontend::setLna(bool /* bEnable */) {
    ALOGV("%s", __FUNCTION__);

    return Result::SUCCESS;
}

Return<Result> Frontend::setLnb(uint32_t /* lnb */) {
    ALOGV("%s", __FUNCTION__);
    if (!supportsSatellite()) {
        return Result::INVALID_STATE;
    }
    return Result::SUCCESS;
}

Return<void> Frontend::linkCiCam(uint32_t ciCamId, linkCiCam_cb _hidl_cb) {
    ALOGV("%s", __FUNCTION__);

    mCiCamId = ciCamId;
    _hidl_cb(Result::SUCCESS, 0 /*ltsId*/);

    return Void();
}

Return<Result> Frontend::unlinkCiCam(uint32_t /*ciCamId*/) {
    ALOGV("%s", __FUNCTION__);

    mCiCamId = -1;

    return Result::SUCCESS;
}

FrontendType Frontend::getFrontendType() {
    return mType;
}

FrontendId Frontend::getFrontendId() {
    return mId;
}

bool Frontend::supportsSatellite() {
    return mType == FrontendType::DVBS || mType == FrontendType::ISDBS ||
           mType == FrontendType::ISDBS3;
}

bool Frontend::isLocked() {
    return mIsLocked;
}
}  // namespace implementation
}  // namespace V1_0
}  // namespace tuner
}  // namespace tv
}  // namespace hardware
}  // namespace android
