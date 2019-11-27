/*
 * Copyright (C) 2019 The Android Open Source Project
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

#include <android/hardware/tests/extension/vibrator/ICustomVibrator.h>
#include <android/hardware/vibrator/IVibrator.h>
#include <binder/IInterface.h>
#include <binder/IServiceManager.h>
#include <gtest/gtest.h>

using android::checked_interface_cast;
using android::IBinder;
using android::IInterface;
using android::OK;
using android::sp;
using android::waitForVintfService;
using android::hardware::tests::extension::vibrator::Directionality;
using android::hardware::tests::extension::vibrator::ICustomVibrator;
using android::hardware::vibrator::IVibrator;

TEST(Cpp, CallRootMethod) {
    sp<IVibrator> vib = waitForVintfService<IVibrator>();
    ASSERT_NE(nullptr, vib.get());
    ASSERT_TRUE(vib->off().isOk());
}

TEST(Cpp, CallExtMethod) {
    // normally you would want to cache this
    sp<IVibrator> vib = waitForVintfService<IVibrator>();
    ASSERT_NE(nullptr, vib.get());

    // getting the extension
    sp<IBinder> ext;
    ASSERT_EQ(OK, IInterface::asBinder(vib)->getExtension(&ext));
    sp<ICustomVibrator> cvib = checked_interface_cast<ICustomVibrator>(ext);
    ASSERT_NE(nullptr, cvib.get());

    // calling extension method
    ASSERT_TRUE(cvib->setDirectionality(Directionality::TRANSVERSE).isOk());
}
