/*
 * Copyright (C) 2018 The Android Open Source Project
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

#include <composer-vts/2.3/ComposerVts.h>

#include <VtsHalHidlTargetTestBase.h>

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_3 {
namespace vts {

using V2_1::Error;

Composer::Composer() : Composer(::testing::VtsHalHidlTargetTestBase::getService<IComposer>()) {}

Composer::Composer(const std::string& name)
    : Composer(::testing::VtsHalHidlTargetTestBase::getService<IComposer>(name)) {}

Composer::Composer(const sp<IComposer>& composer)
    : V2_2::vts::Composer(composer), mComposer(composer) {}

std::unique_ptr<ComposerClient> Composer::createClient() {
    std::unique_ptr<ComposerClient> client;
    mComposer->createClient_2_3([&client](const auto& tmpError, const auto& tmpClient) {
        ASSERT_EQ(Error::NONE, tmpError) << "failed to create client";
        client = std::make_unique<ComposerClient>(tmpClient);
    });

    return client;
}

bool ComposerClient::getDisplayIdentificationData(Display display, uint8_t* outPort,
                                                  std::vector<uint8_t>* outData) {
    bool supported = true;
    mClient->getDisplayIdentificationData(
        display, [&](const auto& tmpError, const auto& tmpPort, const auto& tmpData) {
            if (tmpError == Error::UNSUPPORTED) {
                supported = false;
                return;
            }
            ASSERT_EQ(Error::NONE, tmpError) << "failed to get display identification data";

            *outPort = tmpPort;
            *outData = tmpData;
            ASSERT_FALSE(outData->empty()) << "data is empty";
        });

    return supported;
}

}  // namespace vts
}  // namespace V2_3
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android
