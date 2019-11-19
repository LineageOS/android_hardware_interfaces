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

#include <aidl/android/hardware/tests/extension/vibrator/ICustomVibrator.h>
#include <aidl/android/hardware/vibrator/IVibrator.h>
#include <android/binder_manager.h>

#include <gtest/gtest.h>

using aidl::android::hardware::tests::extension::vibrator::Directionality;
using aidl::android::hardware::tests::extension::vibrator::ICustomVibrator;
using aidl::android::hardware::vibrator::IVibrator;
using ndk::SpAIBinder;

static const std::string kInstance = std::string() + IVibrator::descriptor + "/default";

TEST(Ndk, CallRootMethod) {
    SpAIBinder vibBinder = SpAIBinder(AServiceManager_getService(kInstance.c_str()));
    ASSERT_NE(nullptr, vibBinder.get());
    std::shared_ptr<IVibrator> vib = IVibrator::fromBinder(vibBinder);
    ASSERT_NE(nullptr, vib.get());
    ASSERT_TRUE(vib->off().isOk());
}

TEST(Ndk, CallExtMethod) {
    // normally you would want to cache this
    //
    SpAIBinder vibBinder = SpAIBinder(AServiceManager_getService(kInstance.c_str()));
    ASSERT_NE(nullptr, vibBinder.get());
    std::shared_ptr<IVibrator> vib = IVibrator::fromBinder(vibBinder);
    ASSERT_NE(nullptr, vib.get());

    // getting the extension
    SpAIBinder cvibBinder;
    ASSERT_EQ(STATUS_OK, AIBinder_getExtension(vibBinder.get(), cvibBinder.getR()));
    ASSERT_NE(nullptr, cvibBinder.get());
    std::shared_ptr<ICustomVibrator> cvib = ICustomVibrator::fromBinder(cvibBinder);
    ASSERT_NE(nullptr, cvib.get());

    // calling extension method
    ASSERT_TRUE(cvib->setDirectionality(Directionality::TRANSVERSE).isOk());
}
