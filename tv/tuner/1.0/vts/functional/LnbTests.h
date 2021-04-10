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

#include <android-base/logging.h>
#include <android/hardware/tv/tuner/1.0/ILnb.h>
#include <android/hardware/tv/tuner/1.0/ITuner.h>
#include <android/hardware/tv/tuner/1.0/types.h>
#include <gtest/gtest.h>
#include <hidl/HidlSupport.h>
#include <hidl/HidlTransportSupport.h>
#include <hidl/ServiceManagement.h>
#include <hidl/Status.h>
#include <hidlmemory/FrameworkUtils.h>
#include <utils/Condition.h>
#include <utils/Mutex.h>
#include <map>

using android::Condition;
using android::Mutex;
using android::sp;
using android::hardware::hidl_vec;
using android::hardware::Return;
using android::hardware::Void;
using android::hardware::tv::tuner::V1_0::ILnb;
using android::hardware::tv::tuner::V1_0::ILnbCallback;
using android::hardware::tv::tuner::V1_0::ITuner;
using android::hardware::tv::tuner::V1_0::LnbEventType;
using android::hardware::tv::tuner::V1_0::LnbPosition;
using android::hardware::tv::tuner::V1_0::LnbTone;
using android::hardware::tv::tuner::V1_0::LnbVoltage;
using android::hardware::tv::tuner::V1_0::Result;

using ::testing::AssertionResult;

using namespace std;

class LnbCallback : public ILnbCallback {
  public:
    virtual Return<void> onEvent(LnbEventType lnbEventType) override;
    virtual Return<void> onDiseqcMessage(const hidl_vec<uint8_t>& diseqcMessage) override;

  private:
    bool mEventReceived = false;
    android::Mutex mMsgLock;
    android::Condition mMsgCondition;
};

class LnbTests {
  public:
    void setService(sp<ITuner> tuner) { mService = tuner; }

    AssertionResult getLnbIds(vector<uint32_t>& ids);
    AssertionResult openLnbById(uint32_t lnbId);
    AssertionResult openLnbByName(string lnbName, uint32_t& lnbId);
    AssertionResult setLnbCallback();
    AssertionResult setVoltage(LnbVoltage voltage);
    AssertionResult setTone(LnbTone tone);
    AssertionResult setSatellitePosition(LnbPosition position);
    AssertionResult sendDiseqcMessage(vector<uint8_t> diseqcMsg);
    AssertionResult closeLnb();

  protected:
    static AssertionResult failure() { return ::testing::AssertionFailure(); }

    static AssertionResult success() { return ::testing::AssertionSuccess(); }

    sp<ITuner> mService;
    sp<ILnb> mLnb;
    sp<LnbCallback> mLnbCallback;
    hidl_vec<uint32_t> mLnbIds;
};
