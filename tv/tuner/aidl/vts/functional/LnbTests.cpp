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

#include <log/log.h>

#include "LnbTests.h"

ndk::ScopedAStatus LnbCallback::onEvent(LnbEventType lnbEventType) {
    android::Mutex::Autolock autoLock(mMsgLock);
    ALOGD("[vts] lnb event received. Type: %d", lnbEventType);
    mEventReceived = true;
    mMsgCondition.signal();
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus LnbCallback::onDiseqcMessage(const vector<uint8_t>& diseqcMessage) {
    string msg(diseqcMessage.begin(), diseqcMessage.end());
    ALOGD("[vts] onDiseqcMessage %s", msg.c_str());
    return ndk::ScopedAStatus::ok();
}

AssertionResult LnbTests::getLnbIds(vector<int32_t>& ids) {
    ndk::ScopedAStatus status;
    status = mService->getLnbIds(&ids);
    return AssertionResult(status.isOk());
}

AssertionResult LnbTests::openLnbById(int32_t lnbId) {
    ndk::ScopedAStatus status;
    status = mService->openLnbById(lnbId, &mLnb);
    return AssertionResult(status.isOk());
}

AssertionResult LnbTests::openLnbByName(string lnbName, int32_t& id) {
    ndk::ScopedAStatus status;
    vector<int32_t> ids;
    status = mService->openLnbByName(lnbName, &ids, &mLnb);
    if (status.isOk()) {
        id = ids[0];
    }
    return AssertionResult(status.isOk());
}

AssertionResult LnbTests::setLnbCallback() {
    if (!mLnb) {
        ALOGW("[vts] Open Lnb first");
        return failure();
    }
    mLnbCallback = ndk::SharedRefBase::make<LnbCallback>();
    auto callbackStatus = mLnb->setCallback(mLnbCallback);
    return AssertionResult(callbackStatus.isOk());
}

AssertionResult LnbTests::setVoltage(LnbVoltage voltage) {
    if (!mLnb) {
        ALOGW("[vts] Open Lnb first");
        return failure();
    }
    ndk::ScopedAStatus status = mLnb->setVoltage(voltage);
    return AssertionResult(status.isOk());
}

AssertionResult LnbTests::setTone(LnbTone tone) {
    if (!mLnb) {
        ALOGW("[vts] Open Lnb first");
        return failure();
    }
    ndk::ScopedAStatus status = mLnb->setTone(tone);
    return AssertionResult(status.isOk());
}

AssertionResult LnbTests::setSatellitePosition(LnbPosition position) {
    if (!mLnb) {
        ALOGW("[vts] Open Lnb first");
        return failure();
    }
    ndk::ScopedAStatus status = mLnb->setSatellitePosition(position);
    return AssertionResult(status.isOk());
}

AssertionResult LnbTests::sendDiseqcMessage(vector<uint8_t> diseqcMsg) {
    if (!mLnb) {
        ALOGW("[vts] Open Lnb first");
        return failure();
    }
    ndk::ScopedAStatus status = mLnb->sendDiseqcMessage(diseqcMsg);
    return AssertionResult(status.isOk());
}

AssertionResult LnbTests::closeLnb() {
    if (!mLnb) {
        ALOGW("[vts] Open Lnb first");
        return failure();
    }
    ndk::ScopedAStatus status = mLnb->close();
    mLnb = nullptr;
    mLnbCallback = nullptr;
    return AssertionResult(status.isOk());
}
