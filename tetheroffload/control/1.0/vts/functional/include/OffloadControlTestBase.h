/*
 * Copyright (C) 2020 The Android Open Source Project
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

#include <OffloadControlTestUtils.h>
#include <VtsHalHidlTargetCallbackBase.h>
#include <android-base/stringprintf.h>
#include <android-base/unique_fd.h>
#include <android/hardware/tetheroffload/config/1.0/IOffloadConfig.h>
#include <android/hardware/tetheroffload/control/1.0/IOffloadControl.h>
#include <android/hardware/tetheroffload/control/1.0/types.h>
#include <gtest/gtest.h>
#include <linux/netfilter/nfnetlink.h>
#include <log/log.h>

using android::sp;
using android::base::StringPrintf;
using android::base::unique_fd;
using android::hardware::hidl_handle;
using android::hardware::hidl_string;
using android::hardware::hidl_vec;
using android::hardware::Return;
using android::hardware::Void;
using android::hardware::tetheroffload::config::V1_0::IOffloadConfig;
using android::hardware::tetheroffload::control::V1_0::ITetheringOffloadCallback;
using android::hardware::tetheroffload::control::V1_0::NatTimeoutUpdate;
using android::hardware::tetheroffload::control::V1_0::OffloadCallbackEvent;

constexpr char kCallbackOnEvent[] = "onEvent";
constexpr char kCallbackUpdateTimeout[] = "updateTimeout";

enum class ExpectBoolean {
    Ignored = -1,
    False = 0,
    True = 1,
};

class TetheringOffloadCallbackArgs {
  public:
    OffloadCallbackEvent last_event;
    NatTimeoutUpdate last_params;
};

class OffloadControlTestBase : public testing::TestWithParam<std::tuple<std::string, std::string>> {
  public:
    virtual void SetUp() = 0;

    virtual void TearDown();

    // Called once in setup stage to retrieve correct version of
    // IOffloadControl object.
    virtual sp<android::hardware::tetheroffload::control::V1_0::IOffloadControl> createControl(
            const std::string& serviceName) = 0;

    // The IOffloadConfig HAL is tested more thoroughly elsewhere. Here the
    // class just setup everything correctly and verify basic readiness.
    void setupConfigHal();

    virtual void prepareControlHal() = 0;

    virtual void initOffload(const bool expected_result) = 0;

    void setupControlHal() {
        prepareControlHal();
        initOffload(true);
    };

    void stopOffload(const ExpectBoolean value);

    // Callback class for both events and NAT timeout updates.
    class TetheringOffloadCallback
        : public testing::VtsHalHidlTargetCallbackBase<TetheringOffloadCallbackArgs>,
          public ITetheringOffloadCallback {
      public:
        TetheringOffloadCallback() = default;
        virtual ~TetheringOffloadCallback() = default;

        Return<void> onEvent(OffloadCallbackEvent event) override {
            const TetheringOffloadCallbackArgs args{.last_event = event};
            NotifyFromCallback(kCallbackOnEvent, args);
            return Void();
        };

        Return<void> updateTimeout(const NatTimeoutUpdate& params) override {
            const TetheringOffloadCallbackArgs args{.last_params = params};
            NotifyFromCallback(kCallbackUpdateTimeout, args);
            return Void();
        };
    };

    sp<IOffloadConfig> config;
    sp<android::hardware::tetheroffload::control::V1_0::IOffloadControl> control;
    sp<TetheringOffloadCallback> control_cb;
};