//
// Copyright (C) 2019 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <gtest/gtest.h>

#include "HalProxy.h"
#include "SensorsSubHal.h"

using ::android::hardware::sensors::V2_0::implementation::HalProxy;
using ::android::hardware::sensors::V2_0::subhal::implementation::SensorsSubHal;

// TODO: Add more interesting tests such as
//     - verify setOperationMode invokes all subhals
//     - verify if a subhal fails to change operation mode, that state is reset properly
//     - Available sensors are obtained during initialization
//
// You can run this suite using "atest android.hardware.sensors@2.0-halproxy-unit-tests".
//
// See https://source.android.com/compatibility/tests/development/native-func-e2e.md for more info
// on how tests are set up and for information on the gtest framework itself.
TEST(HalProxyTest, ExampleTest) {
    SensorsSubHal subHal;
    std::vector<ISensorsSubHal*> fakeSubHals;
    fakeSubHals.push_back(&subHal);

    HalProxy proxy(fakeSubHals);
}