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

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_4 {
namespace vts {

using V2_4::Error;

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

int32_t ComposerClient::getDisplayAttribute_2_4(
        android::hardware::graphics::composer::V2_1::Display display,
        android::hardware::graphics::composer::V2_1::Config config,
        IComposerClient::Attribute attribute) {
    int32_t value = 0;
    mClient->getDisplayAttribute_2_4(
            display, config, attribute, [&](const auto& tmpError, const auto& tmpValue) {
                ASSERT_EQ(Error::NONE, tmpError) << "failed to get display attribute";
                value = tmpValue;
            });

    return value;
}

void ComposerClient::registerCallback_2_4(const sp<IComposerCallback>& callback) {
    mClient->registerCallback_2_4(callback);
}

Error ComposerClient::getDisplayVsyncPeriod(Display display, VsyncPeriodNanos* outVsyncPeriod) {
    Error error = Error::NONE;
    mClient->getDisplayVsyncPeriod(display, [&](const auto& tmpError, const auto& tmpVsyncPeriod) {
        error = tmpError;
        *outVsyncPeriod = tmpVsyncPeriod;
    });
    return error;
}

Error ComposerClient::setActiveConfigWithConstraints(
        Display display, Config config,
        const IComposerClient::VsyncPeriodChangeConstraints& vsyncPeriodChangeConstraints,
        VsyncPeriodChangeTimeline* timeline) {
    Error error = Error::NONE;
    mClient->setActiveConfigWithConstraints(display, config, vsyncPeriodChangeConstraints,
                                            [&](const auto& tmpError, const auto& tmpTimeline) {
                                                error = tmpError;
                                                *timeline = tmpTimeline;
                                            });
    return error;
}

Error ComposerClient::setAutoLowLatencyMode(Display display, bool on) {
    return mClient->setAutoLowLatencyMode(display, on);
}

Error ComposerClient::getSupportedContentTypes(
        Display display, std::vector<IComposerClient::ContentType>* outSupportedContentTypes) {
    Error error = Error::NONE;
    mClient->getSupportedContentTypes(
            display, [&](const auto& tmpError, const auto& tmpSupportedContentTypes) {
                error = tmpError;
                *outSupportedContentTypes = tmpSupportedContentTypes;
            });
    return error;
}

Error ComposerClient::setContentType(Display display, IComposerClient::ContentType contentType) {
    return mClient->setContentType(display, contentType);
}

Error ComposerClient::getLayerGenericMetadataKeys(
        std::vector<IComposerClient::LayerGenericMetadataKey>* outKeys) {
    Error error = Error::NONE;
    mClient->getLayerGenericMetadataKeys([&](const auto tmpError, const auto& tmpKeys) {
        error = tmpError;
        *outKeys = tmpKeys;
    });
    return error;
}

void ComposerClient::execute(TestCommandReader* reader, CommandWriterBase* writer) {
    bool queueChanged = false;
    uint32_t commandLength = 0;
    hidl_vec<hidl_handle> commandHandles;
    ASSERT_TRUE(writer->writeQueue(&queueChanged, &commandLength, &commandHandles));

    if (queueChanged) {
        auto ret = mClient->setInputCommandQueue(*writer->getMQDescriptor());
        ASSERT_EQ(V2_1::Error::NONE, ret);
    }

    mClient->executeCommands_2_3(
            commandLength, commandHandles,
            [&](const auto& tmpError, const auto& tmpOutQueueChanged, const auto& tmpOutLength,
                const auto& tmpOutHandles) {
                ASSERT_EQ(V2_1::Error::NONE, tmpError);

                if (tmpOutQueueChanged) {
                    mClient->getOutputCommandQueue(
                            [&](const auto& tmpError, const auto& tmpDescriptor) {
                                ASSERT_EQ(V2_3::Error::NONE, tmpError);
                                reader->setMQDescriptor(tmpDescriptor);
                            });
                }

                ASSERT_TRUE(reader->readQueue(tmpOutLength, tmpOutHandles));
                reader->parse();
            });
    reader->reset();
    writer->reset();
}

}  // namespace vts
}  // namespace V2_4
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android
