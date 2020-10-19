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

#include "DescramblerTests.h"

AssertionResult DescramblerTests::createCasPlugin(int32_t caSystemId) {
    auto status = mMediaCasService->isSystemIdSupported(caSystemId);
    if (!status.isOk() || !status) {
        ALOGW("[vts] Failed to check isSystemIdSupported.");
        return failure();
    }

    mCasListener = new MediaCasListener();
    auto pluginStatus = mMediaCasService->createPluginExt(caSystemId, mCasListener);
    if (!pluginStatus.isOk()) {
        ALOGW("[vts] Failed to createPluginExt.");
        return failure();
    }
    mCas = ICas::castFrom(pluginStatus);
    if (mCas == nullptr) {
        ALOGW("[vts] Failed to get ICas.");
        return failure();
    }
    return success();
}

AssertionResult DescramblerTests::openCasSession(TunerKeyToken& sessionId,
                                                 vector<uint8_t> hidlPvtData) {
    Status sessionStatus;
    SessionIntent intent = SessionIntent::LIVE;
    ScramblingMode mode = ScramblingMode::RESERVED;
    auto returnVoid =
            mCas->openSession_1_2(intent, mode, [&](Status status, const hidl_vec<uint8_t>& id) {
                sessionStatus = status;
                sessionId = id;
            });
    if (!returnVoid.isOk() || (sessionStatus != Status::OK)) {
        ALOGW("[vts] Failed to open cas session.");
        mCas->closeSession(sessionId);
        return failure();
    }

    auto status = mCas->setSessionPrivateData(sessionId, hidlPvtData);
    if (status != android::hardware::cas::V1_0::Status::OK) {
        ALOGW("[vts] Failed to set session private data");
        mCas->closeSession(sessionId);
        return failure();
    }
    return success();
}

AssertionResult DescramblerTests::getKeyToken(int32_t caSystemId, string provisonStr,
                                              hidl_vec<uint8_t> hidlPvtData, TunerKeyToken& token) {
    if (createCasPlugin(caSystemId) != success()) {
        ALOGW("[vts] createCasPlugin failed.");
        return failure();
    }

    if (provisonStr.size() > 0) {
        auto returnStatus = mCas->provision(hidl_string(provisonStr));
        if (returnStatus != android::hardware::cas::V1_0::Status::OK) {
            ALOGW("[vts] provision failed.");
            return failure();
        }
    }

    return openCasSession(token, hidlPvtData);
}

AssertionResult DescramblerTests::openDescrambler(uint32_t demuxId) {
    Result status;
    mService->openDescrambler([&](Result result, const sp<IDescrambler>& descrambler) {
        mDescrambler = descrambler;
        status = result;
    });
    if (status != Result::SUCCESS) {
        ALOGW("[vts] openDescrambler failed.");
        return failure();
    }

    status = mDescrambler->setDemuxSource(demuxId);
    if (status != Result::SUCCESS) {
        ALOGW("[vts] setDemuxSource failed.");
        return failure();
    }

    return success();
}

AssertionResult DescramblerTests::setKeyToken(TunerKeyToken token) {
    Result status;
    if (!mDescrambler) {
        ALOGW("[vts] Descrambler is not opened yet.");
        return failure();
    }

    status = mDescrambler->setKeyToken(token);
    if (status != Result::SUCCESS) {
        ALOGW("[vts] setKeyToken failed.");
        return failure();
    }

    return success();
}

AssertionResult DescramblerTests::addPid(DemuxPid pid, sp<IFilter> optionalSourceFilter) {
    Result status;
    if (!mDescrambler) {
        ALOGW("[vts] Descrambler is not opened yet.");
        return failure();
    }

    status = mDescrambler->addPid(pid, optionalSourceFilter);
    if (status != Result::SUCCESS) {
        ALOGW("[vts] addPid failed.");
        return failure();
    }

    return success();
}

AssertionResult DescramblerTests::removePid(DemuxPid pid, sp<IFilter> optionalSourceFilter) {
    Result status;
    if (!mDescrambler) {
        ALOGW("[vts] Descrambler is not opened yet.");
        return failure();
    }

    status = mDescrambler->removePid(pid, optionalSourceFilter);
    if (status != Result::SUCCESS) {
        ALOGW("[vts] removePid failed.");
        return failure();
    }

    return success();
}

AssertionResult DescramblerTests::closeDescrambler() {
    Result status;
    if (!mDescrambler) {
        ALOGW("[vts] Descrambler is not opened yet.");
        return failure();
    }

    status = mDescrambler->close();
    mDescrambler = nullptr;
    if (status != Result::SUCCESS) {
        ALOGW("[vts] close Descrambler failed.");
        return failure();
    }

    return success();
}

AssertionResult DescramblerTests::getDemuxPidFromFilterSettings(DemuxFilterType type,
                                                                DemuxFilterSettings settings,
                                                                DemuxPid& pid) {
    switch (type.mainType) {
        case DemuxFilterMainType::TS:
            if (type.subType.tsFilterType() == DemuxTsFilterType::AUDIO ||
                type.subType.tsFilterType() == DemuxTsFilterType::VIDEO) {
                pid.tPid(settings.ts().tpid);
            } else {
                ALOGW("[vts] Not a media ts filter!");
                return failure();
            }
            break;
        case DemuxFilterMainType::MMTP:
            if (type.subType.mmtpFilterType() == DemuxMmtpFilterType::AUDIO ||
                type.subType.mmtpFilterType() == DemuxMmtpFilterType::VIDEO) {
                pid.mmtpPid(settings.mmtp().mmtpPid);
            } else {
                ALOGW("[vts] Not a media mmtp filter!");
                return failure();
            }
            break;
        default:
            ALOGW("[vts] Not a media filter!");
            return failure();
    }
    return success();
}
