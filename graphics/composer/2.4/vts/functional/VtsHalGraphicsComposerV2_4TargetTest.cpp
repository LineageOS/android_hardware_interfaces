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

#define LOG_TAG "graphics_composer_hidl_hal_test@2.4"

#include <algorithm>

#include <VtsHalHidlTargetTestBase.h>
#include <android-base/logging.h>
#include <android/hardware/graphics/mapper/2.0/IMapper.h>
#include <composer-command-buffer/2.4/ComposerCommandBuffer.h>
#include <composer-vts/2.1/GraphicsComposerCallback.h>
#include <composer-vts/2.1/TestCommandReader.h>
#include <composer-vts/2.4/ComposerVts.h>
#include <mapper-vts/2.0/MapperVts.h>

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_4 {
namespace vts {
namespace {

using common::V1_0::BufferUsage;
using common::V1_1::RenderIntent;
using common::V1_2::ColorMode;
using common::V1_2::Dataspace;
using common::V1_2::PixelFormat;
using mapper::V2_0::IMapper;
using mapper::V2_0::vts::Gralloc;

// Test environment for graphics.composer
class GraphicsComposerHidlEnvironment : public ::testing::VtsHalHidlTargetTestEnvBase {
  public:
    // get the test environment singleton
    static GraphicsComposerHidlEnvironment* Instance() {
        static GraphicsComposerHidlEnvironment* instance = new GraphicsComposerHidlEnvironment;
        return instance;
    }

    virtual void registerTestServices() override { registerTestService<IComposer>(); }

  private:
    GraphicsComposerHidlEnvironment() {}

    GTEST_DISALLOW_COPY_AND_ASSIGN_(GraphicsComposerHidlEnvironment);
};

class GraphicsComposerHidlTest : public ::testing::VtsHalHidlTargetTestBase {
  protected:
    void SetUp() override {
        ASSERT_NO_FATAL_FAILURE(
                mComposer = std::make_unique<Composer>(
                        GraphicsComposerHidlEnvironment::Instance()->getServiceName<IComposer>()));
        ASSERT_NO_FATAL_FAILURE(mComposerClient = mComposer->createClient());

        mComposerCallback = new V2_1::vts::GraphicsComposerCallback;
        mComposerClient->registerCallback(mComposerCallback);

        // assume the first display is primary and is never removed
        mPrimaryDisplay = waitForFirstDisplay();

        mInvalidDisplayId = GetInvalidDisplayId();

        // explicitly disable vsync
        mComposerClient->setVsyncEnabled(mPrimaryDisplay, false);
        mComposerCallback->setVsyncAllowed(false);

        mWriter = std::make_unique<CommandWriterBase>(1024);
        mReader = std::make_unique<V2_1::vts::TestCommandReader>();
    }

    void TearDown() override {
        ASSERT_EQ(0, mReader->mErrors.size());
        ASSERT_EQ(0, mReader->mCompositionChanges.size());
        if (mComposerCallback != nullptr) {
            EXPECT_EQ(0, mComposerCallback->getInvalidHotplugCount());
            EXPECT_EQ(0, mComposerCallback->getInvalidRefreshCount());
            EXPECT_EQ(0, mComposerCallback->getInvalidVsyncCount());
        }
    }

    // returns an invalid display id (one that has not been registered to a
    // display.  Currently assuming that a device will never have close to
    // std::numeric_limit<uint64_t>::max() displays registered while running tests
    Display GetInvalidDisplayId() {
        std::vector<Display> validDisplays = mComposerCallback->getDisplays();
        uint64_t id = std::numeric_limits<uint64_t>::max();
        while (id > 0) {
            if (std::find(validDisplays.begin(), validDisplays.end(), id) == validDisplays.end()) {
                return id;
            }
            id--;
        }

        return 0;
    }

    void execute() { mComposerClient->execute(mReader.get(), mWriter.get()); }

    std::unique_ptr<Composer> mComposer;
    std::unique_ptr<ComposerClient> mComposerClient;
    sp<V2_1::vts::GraphicsComposerCallback> mComposerCallback;
    // the first display and is assumed never to be removed
    Display mPrimaryDisplay;
    Display mInvalidDisplayId;
    std::unique_ptr<CommandWriterBase> mWriter;
    std::unique_ptr<V2_1::vts::TestCommandReader> mReader;

  private:
    Display waitForFirstDisplay() {
        while (true) {
            std::vector<Display> displays = mComposerCallback->getDisplays();
            if (displays.empty()) {
                usleep(5 * 1000);
                continue;
            }

            return displays[0];
        }
    }
};

// Tests for IComposerClient::Command.
class GraphicsComposerHidlCommandTest : public GraphicsComposerHidlTest {
  protected:
    void SetUp() override {
        ASSERT_NO_FATAL_FAILURE(GraphicsComposerHidlTest::SetUp());

        ASSERT_NO_FATAL_FAILURE(mGralloc = std::make_unique<Gralloc>());

        mWriter = std::make_unique<CommandWriterBase>(1024);
        mReader = std::make_unique<V2_1::vts::TestCommandReader>();
    }

    void TearDown() override {
        ASSERT_EQ(0, mReader->mErrors.size());
        ASSERT_NO_FATAL_FAILURE(GraphicsComposerHidlTest::TearDown());
    }

    const native_handle_t* allocate() {
        IMapper::BufferDescriptorInfo info{};
        info.width = 64;
        info.height = 64;
        info.layerCount = 1;
        info.format = static_cast<common::V1_0::PixelFormat>(PixelFormat::RGBA_8888);
        info.usage =
                static_cast<uint64_t>(BufferUsage::CPU_WRITE_OFTEN | BufferUsage::CPU_READ_OFTEN);

        return mGralloc->allocate(info);
    }

    void execute() { mComposerClient->execute(mReader.get(), mWriter.get()); }

    std::unique_ptr<CommandWriterBase> mWriter;
    std::unique_ptr<V2_1::vts::TestCommandReader> mReader;

  private:
    std::unique_ptr<Gralloc> mGralloc;
};

TEST_F(GraphicsComposerHidlTest, getDisplayCapabilitiesBadDisplay) {
    std::vector<IComposerClient::DisplayCapability> capabilities;
    const auto error = mComposerClient->getDisplayCapabilities(mInvalidDisplayId, &capabilities);
    EXPECT_EQ(Error::BAD_DISPLAY, error);
}

TEST_F(GraphicsComposerHidlTest, getDisplayConnectionType) {
    IComposerClient::DisplayConnectionType type;
    EXPECT_EQ(Error::BAD_DISPLAY,
              mComposerClient->getDisplayConnectionType(mInvalidDisplayId, &type));

    for (Display display : mComposerCallback->getDisplays()) {
        EXPECT_EQ(Error::NONE, mComposerClient->getDisplayConnectionType(display, &type));
    }
}

}  // namespace
}  // namespace vts
}  // namespace V2_4
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android

int main(int argc, char** argv) {
    using android::hardware::graphics::composer::V2_4::vts::GraphicsComposerHidlEnvironment;
    ::testing::AddGlobalTestEnvironment(GraphicsComposerHidlEnvironment::Instance());
    ::testing::InitGoogleTest(&argc, argv);
    GraphicsComposerHidlEnvironment::Instance()->init(&argc, argv);
    int status = RUN_ALL_TESTS();
    return status;
}
