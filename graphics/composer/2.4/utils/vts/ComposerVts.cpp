/*
 * Copyright 2019 The Android Open Source Project
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

#include <composer-vts/2.4/ComposerVts.h>

#include <VtsHalHidlTargetTestBase.h>

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_4 {
namespace vts {

using V2_4::Error;

Composer::Composer() : Composer(::testing::VtsHalHidlTargetTestBase::getService<IComposer>()) {}

Composer::Composer(const std::string& name)
    : Composer(::testing::VtsHalHidlTargetTestBase::getService<IComposer>(name)) {}

Composer::Composer(const sp<IComposer>& composer)
    : V2_3::vts::Composer(composer), mComposer(composer) {}

std::unique_ptr<ComposerClient> Composer::createClient() {
    std::unique_ptr<ComposerClient> client;
    mComposer->createClient_2_4([&client](const auto& tmpError, const auto& tmpClient) {
        ASSERT_EQ(Error::NONE, tmpError) << "failed to create client";
        client = std::make_unique<ComposerClient>(tmpClient);
    });

    return client;
}

sp<IComposerClient> ComposerClient::getRaw() const {
    return mClient;
}

Error ComposerClient::getDisplayCapabilities(
        Display display, std::vector<IComposerClient::DisplayCapability>* outCapabilities) {
    Error error = Error::NONE;
    mClient->getDisplayCapabilities_2_4(display,
                                        [&](const auto& tmpError, const auto& tmpCapabilities) {
                                            error = tmpError;
                                            *outCapabilities = tmpCapabilities;
                                        });
    return error;
}

Error ComposerClient::getDisplayConnectionType(Display display,
                                               IComposerClient::DisplayConnectionType* outType) {
    Error error = Error::NONE;
    mClient->getDisplayConnectionType(display, [&](const auto& tmpError, const auto& tmpType) {
        error = tmpError;
        *outType = tmpType;
    });
    return error;
}

Error ComposerClient::getSupportedDisplayVsyncPeriods(
        Display display, Config config, std::vector<VsyncPeriodNanos>* outSupportedVsyncPeriods) {
    Error error = Error::NONE;
    mClient->getSupportedDisplayVsyncPeriods(
            display, config, [&](const auto& tmpError, const auto& tmpSupportedVsyncPeriods) {
                error = tmpError;
                *outSupportedVsyncPeriods = tmpSupportedVsyncPeriods;
            });
    return error;
}

Error ComposerClient::getDisplayVsyncPeriod(Display display, VsyncPeriodNanos* outVsyncPeriod) {
    Error error = Error::NONE;
    mClient->getDisplayVsyncPeriod(display, [&](const auto& tmpError, const auto& tmpVsyncPeriod) {
        error = tmpError;
        *outVsyncPeriod = tmpVsyncPeriod;
    });
    return error;
}

Error ComposerClient::setActiveConfigAndVsyncPeriod(
        Display display, Config config, VsyncPeriodNanos vsyncPeriodNanos,
        const IComposerClient::VsyncPeriodChangeConstraints& vsyncPeriodChangeConstraints,
        int64_t* outNewVsyncAppliedTime) {
    Error error = Error::NONE;
    mClient->setActiveConfigAndVsyncPeriod(
            display, config, vsyncPeriodNanos, vsyncPeriodChangeConstraints,
            [&](const auto& tmpError, const auto& tmpNewVsyncAppliedTime) {
                error = tmpError;
                *outNewVsyncAppliedTime = tmpNewVsyncAppliedTime;
            });
    return error;
}

}  // namespace vts
}  // namespace V2_4
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android
