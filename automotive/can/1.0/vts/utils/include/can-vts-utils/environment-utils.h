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

#pragma once

#include <VtsHalHidlTargetTestEnvBase.h>

namespace android::hardware::automotive::can::V1_0::vts::utils {

/**
 * Simple test environment.
 *
 * This is a helper class to instantiate a test environment without boilerplate code for cases where
 * there is no need to pass more parameters than just HIDL service instance name.
 *
 * The class implements registerTestServices() by calling registerTestService() on every HIDL
 * interface provided as parameter to this template.
 *
 * Example usage:
 *     static utils::SimpleHidlEnvironment<IMyService>* gEnv = nullptr;
 *
 *     void CanBusHalTest::SetUp() {
 *         const auto serviceName = gEnv->getServiceName<IMyService>();
 *         (...)
 *     }
 *
 *     int main(int argc, char** argv) {
 *         gEnv = new SimpleHidlEnvironment<IMyService>;
 *         ::testing::AddGlobalTestEnvironment(gEnv);
 *         ::testing::InitGoogleTest(&argc, argv);
 *         gEnv->init(&argc, argv);
 *         return RUN_ALL_TESTS();
 *     }
 *
 * \param T... HIDL interface names to register for a test service
 */
template <typename... T>
class SimpleHidlEnvironment : public ::testing::VtsHalHidlTargetTestEnvBase {
  public:
    virtual void registerTestServices() override {
        // Call registerTestService() for every HIDL interface using this template.
        using expander = int[];
        (void)expander{0, (registerTestService<T>(), 0)...};
    }
};

}  // namespace android::hardware::automotive::can::V1_0::vts::utils
