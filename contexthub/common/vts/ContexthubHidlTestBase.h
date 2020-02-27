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

#include <android/hardware/contexthub/1.0/IContexthubCallback.h>
#include <gtest/gtest.h>

#include <string>
#include <tuple>

namespace android {
namespace hardware {
namespace contexthub {
namespace vts_utils {

// Base fixture for Context Hub HAL tests. Parameterized by service name and hub ID.
template <class IContexthubVersion>
class ContexthubHidlTestBase
    : public ::testing::TestWithParam<std::tuple<std::string, std::string>> {
  public:
    virtual void SetUp() override { fetchHubApi(); }

    void fetchHubApi() {
        hubApi = IContexthubVersion::getService(std::get<0>(GetParam()));
        ASSERT_NE(hubApi, nullptr);
    }

    uint32_t getHubId() { return std::stoi(std::get<1>(GetParam())); }

    V1_0::Result registerCallback(sp<V1_0::IContexthubCallback> cb) {
        return hubApi->registerCallback(getHubId(), cb);
    }

    sp<IContexthubVersion> hubApi;
};

}  // namespace vts_utils
}  // namespace contexthub
}  // namespace hardware
}  // namespace android
