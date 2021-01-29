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

#include <OffloadControlTestV1_0.h>
#include <android/hardware/tetheroffload/control/1.1/IOffloadControl.h>
#include <android/hardware/tetheroffload/control/1.1/ITetheringOffloadCallback.h>
#include <gtest/gtest.h>

constexpr char kCallbackOnEvent_1_1[] = "onEvent_1_1";

class TetheringOffloadCallbackArgsV1_1 {
  public:
    android::hardware::tetheroffload::control::V1_1::OffloadCallbackEvent last_event;
};

class OffloadControlTestV1_1_HalNotStarted : public OffloadControlTestV1_0_HalNotStarted {
  public:
    virtual sp<android::hardware::tetheroffload::control::V1_0::IOffloadControl> createControl(
            const std::string& serviceName) override {
        return android::hardware::tetheroffload::control::V1_1::IOffloadControl::getService(
                serviceName);
    };

    void prepareControlHal() override {
        control = createControl(std::get<1>(GetParam()));
        ASSERT_NE(nullptr, control.get()) << "Could not get HIDL instance";

        control_cb_1_1 = new TetheringOffloadCallbackV1_1();
        ASSERT_NE(nullptr, control_cb_1_1.get()) << "Could not get offload callback";
    };

    void initOffload(const bool expected_result) override {
        auto init_cb = [&](bool success, std::string errMsg) {
            std::string msg = StringPrintf("Unexpectedly %s to init offload: %s",
                                           success ? "succeeded" : "failed", errMsg.c_str());
            ASSERT_EQ(expected_result, success) << msg;
        };
        auto control = getControlV1_1();
        ASSERT_NE(control, nullptr);
        const Return<void> ret = control->initOffload(control_cb_1_1, init_cb);
        ASSERT_TRUE(ret.isOk());
    };

    sp<android::hardware::tetheroffload::control::V1_1::IOffloadControl> getControlV1_1() {
        // The cast is safe since only devices with V1.1+ HAL will be enumerated and pass in to the
        // test.
        return android::hardware::tetheroffload::control::V1_1::IOffloadControl::castFrom(control)
                .withDefault(nullptr);
    };

    // Callback class for both new events.
    class TetheringOffloadCallbackV1_1
        : public testing::VtsHalHidlTargetCallbackBase<TetheringOffloadCallbackArgsV1_1>,
          public android::hardware::tetheroffload::control::V1_1::ITetheringOffloadCallback {
      public:
        Return<void> onEvent_1_1(
                android::hardware::tetheroffload::control::V1_1::OffloadCallbackEvent event)
                override {
            const TetheringOffloadCallbackArgsV1_1 args{.last_event = event};
            NotifyFromCallback(kCallbackOnEvent_1_1, args);
            return Void();
        };

        Return<void> onEvent([[maybe_unused]] OffloadCallbackEvent event) override {
            // Tested only in IOffloadControl 1.0.
            return Void();
        };

        Return<void> updateTimeout([[maybe_unused]] const NatTimeoutUpdate& params) override {
            // Tested only in IOffloadControl 1.0.
            return Void();
        };
    };

    sp<TetheringOffloadCallbackV1_1> control_cb_1_1;
};

class OffloadControlTestV1_1_HalStarted : public OffloadControlTestV1_1_HalNotStarted {
  public:
    virtual void SetUp() override {
        setupConfigHal();
        setupControlHal();
    }
};