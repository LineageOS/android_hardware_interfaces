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

#pragma once

#include <memory>
#include <vector>

#include <VtsHalHidlTargetTestBase.h>
#include <android/hardware/graphics/composer/2.4/IComposer.h>
#include <android/hardware/graphics/composer/2.4/IComposerClient.h>
#include <composer-vts/2.3/ComposerVts.h>
#include <utils/StrongPointer.h>

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_4 {
namespace vts {

using common::V1_1::RenderIntent;
using common::V1_2::ColorMode;
using common::V1_2::Dataspace;
using common::V1_2::Hdr;
using common::V1_2::PixelFormat;
using V2_1::Config;
using V2_1::Display;
using V2_4::Error;
using V2_4::IComposer;
using V2_4::IComposerClient;
using V2_4::VsyncPeriodNanos;

class ComposerClient;

// A wrapper to IComposer.
class Composer : public V2_3::vts::Composer {
  public:
    Composer();
    explicit Composer(const std::string& name);
    explicit Composer(const sp<IComposer>& composer);

    std::unique_ptr<ComposerClient> createClient();

  private:
    const sp<IComposer> mComposer;
};

// A wrapper to IComposerClient.
class ComposerClient : public V2_3::vts::ComposerClient {
  public:
    explicit ComposerClient(const sp<IComposerClient>& client)
        : V2_3::vts::ComposerClient(client), mClient(client) {}

    sp<IComposerClient> getRaw() const;

    Error getDisplayCapabilities(
            Display display,
            std::vector<IComposerClient::DisplayCapability>* outDisplayCapabilities);

    Error getDisplayConnectionType(Display display,
                                   IComposerClient::DisplayConnectionType* outType);

    int32_t getDisplayAttribute_2_4(Display display, Config config,
                                    IComposerClient::Attribute attribute);

    Error getDisplayVsyncPeriod(Display display, VsyncPeriodNanos* outVsyncPeriods);

    Error setActiveConfigWithConstraints(
            Display display, Config config,
            const IComposerClient::VsyncPeriodChangeConstraints& vsyncPeriodChangeConstraints,
            VsyncPeriodChangeTimeline* timeline);

    Error setAutoLowLatencyMode(Display display, bool on);

    Error getSupportedContentTypes(
            Display display, std::vector<IComposerClient::ContentType>* outSupportedContentTypes);

    Error setContentType(Display display, IComposerClient::ContentType contentType);

  private:
    const sp<IComposerClient> mClient;
};

}  // namespace vts
}  // namespace V2_4
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android
