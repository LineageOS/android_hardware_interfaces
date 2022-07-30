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

#include <aidl/android/hardware/tv/tuner/BnLnbCallback.h>
#include <aidl/android/hardware/tv/tuner/ILnb.h>
#include <aidl/android/hardware/tv/tuner/ITuner.h>
#include <gtest/gtest.h>
#include <log/log.h>
#include <utils/Condition.h>
#include <utils/Mutex.h>
#include <map>

#define INVALID_LNB_ID -1

using android::Condition;
using android::Mutex;

using ::testing::AssertionResult;

using namespace aidl::android::hardware::tv::tuner;
using namespace std;

class LnbCallback : public BnLnbCallback {
  public:
    virtual ::ndk::ScopedAStatus onEvent(LnbEventType lnbEventType) override;
    virtual ::ndk::ScopedAStatus onDiseqcMessage(
            const std::vector<uint8_t>& diseqcMessage) override;

  private:
    bool mEventReceived = false;
    android::Mutex mMsgLock;
    android::Condition mMsgCondition;
};

class LnbTests {
  public:
    void setService(std::shared_ptr<ITuner> tuner) { mService = tuner; }

    AssertionResult getLnbIds(vector<int32_t>& ids);
    AssertionResult openLnbById(int32_t lnbId);
    AssertionResult openLnbByName(string lnbName, int32_t& lnbId);
    AssertionResult setLnbCallback();
    AssertionResult setVoltage(LnbVoltage voltage);
    AssertionResult setTone(LnbTone tone);
    AssertionResult setSatellitePosition(LnbPosition position);
    AssertionResult sendDiseqcMessage(vector<uint8_t> diseqcMsg);
    AssertionResult closeLnb();

  protected:
    static AssertionResult failure() { return ::testing::AssertionFailure(); }

    static AssertionResult success() { return ::testing::AssertionSuccess(); }

    std::shared_ptr<ITuner> mService;
    std::shared_ptr<ILnb> mLnb;
    std::shared_ptr<LnbCallback> mLnbCallback;
    vector<int32_t> mLnbIds;
};
