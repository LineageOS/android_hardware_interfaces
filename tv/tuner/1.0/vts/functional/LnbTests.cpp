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

#include "LnbTests.h"

Return<void> LnbCallback::onEvent(LnbEventType lnbEventType) {
    android::Mutex::Autolock autoLock(mMsgLock);
    ALOGD("[vts] lnb event received. Type: %d", lnbEventType);
    mEventReceived = true;
    mMsgCondition.signal();
    return Void();
}

Return<void> LnbCallback::onDiseqcMessage(const hidl_vec<uint8_t>& diseqcMessage) {
    string msg(diseqcMessage.begin(), diseqcMessage.end());
    ALOGD("[vts] onDiseqcMessage %s", msg.c_str());
    return Void();
}

AssertionResult LnbTests::getLnbIds(vector<uint32_t>& ids) {
    Result status;
    mService->getLnbIds([&](Result result, const hidl_vec<uint32_t>& lnbIds) {
        status = result;
        ids = lnbIds;
    });
    return AssertionResult(status == Result::SUCCESS);
}

AssertionResult LnbTests::openLnbById(uint32_t lnbId) {
    Result status;
    mService->openLnbById(lnbId, [&](Result result, const sp<ILnb>& lnb) {
        mLnb = lnb;
        status = result;
    });
    return AssertionResult(status == Result::SUCCESS);
}

AssertionResult LnbTests::openLnbByName(string lnbName, uint32_t& id) {
    Result status;
    mService->openLnbByName(lnbName, [&](Result result, uint32_t lnbId, const sp<ILnb>& lnb) {
        mLnb = lnb;
        id = lnbId;
        status = result;
    });

    return AssertionResult(status == Result::SUCCESS);
}

AssertionResult LnbTests::setLnbCallback() {
    if (!mLnb) {
        ALOGW("[vts] Open Lnb first");
        return failure();
    }
    mLnbCallback = new LnbCallback();
    auto callbackStatus = mLnb->setCallback(mLnbCallback);
    return AssertionResult(callbackStatus.isOk());
}

AssertionResult LnbTests::setVoltage(LnbVoltage voltage) {
    if (!mLnb) {
        ALOGW("[vts] Open Lnb first");
        return failure();
    }
    Result status = mLnb->setVoltage(voltage);
    return AssertionResult(status == Result::SUCCESS);
}

AssertionResult LnbTests::setTone(LnbTone tone) {
    if (!mLnb) {
        ALOGW("[vts] Open Lnb first");
        return failure();
    }
    Result status = mLnb->setTone(tone);
    return AssertionResult(status == Result::SUCCESS);
}

AssertionResult LnbTests::setSatellitePosition(LnbPosition position) {
    if (!mLnb) {
        ALOGW("[vts] Open Lnb first");
        return failure();
    }
    Result status = mLnb->setSatellitePosition(position);
    return AssertionResult(status == Result::SUCCESS);
}

AssertionResult LnbTests::sendDiseqcMessage(vector<uint8_t> diseqcMsg) {
    if (!mLnb) {
        ALOGW("[vts] Open Lnb first");
        return failure();
    }
    Result status = mLnb->sendDiseqcMessage(diseqcMsg);
    return AssertionResult(status == Result::SUCCESS);
}

AssertionResult LnbTests::closeLnb() {
    if (!mLnb) {
        ALOGW("[vts] Open Lnb first");
        return failure();
    }
    Result status = mLnb->close();
    mLnb = nullptr;
    mLnbCallback = nullptr;
    return AssertionResult(status == Result::SUCCESS);
}
