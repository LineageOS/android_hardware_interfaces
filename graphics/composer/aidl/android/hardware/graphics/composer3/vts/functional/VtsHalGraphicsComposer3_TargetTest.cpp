
#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <aidl/android/hardware/graphics/composer3/IComposer.h>
#include <android-base/properties.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <binder/ProcessState.h>
#include <composer-vts/include/GraphicsComposerCallback.h>
#include <gtest/gtest.h>
#include <string>
#include <thread>

#pragma push_macro("LOG_TAG")
#undef LOG_TAG
#define LOG_TAG "VtsHalGraphicsComposer3_TargetTest"

typedef int32_t Config;

namespace aidl::android::hardware::graphics::composer3::vts {
namespace {

using namespace std::chrono_literals;

class VtsDisplay {
  public:
    VtsDisplay(int64_t displayId, int32_t displayWidth, int32_t displayHeight)
        : mDisplayId(displayId), mDisplayWidth(displayWidth), mDisplayHeight(displayHeight) {}

    int64_t get() const { return mDisplayId; }

    void setDimensions(int32_t displayWidth, int32_t displayHeight) {
        mDisplayWidth = displayWidth;
        mDisplayHeight = displayHeight;
    }

  private:
    const int64_t mDisplayId;
    int32_t mDisplayWidth;
    int32_t mDisplayHeight;
};

class GraphicsComposerAidlTest : public ::testing::TestWithParam<std::string> {
  protected:
    void SetUp() override {
        std::string name = GetParam();
        ndk::SpAIBinder binder(AServiceManager_waitForService(name.c_str()));
        ASSERT_NE(binder, nullptr);
        ASSERT_NO_FATAL_FAILURE(mComposer = IComposer::fromBinder(binder));
        ASSERT_NE(mComposer, nullptr);
        ASSERT_NO_FATAL_FAILURE(mComposer->createClient(&mComposerClient));
        mInvalidDisplayId = GetInvalidDisplayId();

        mComposerCallback = ::ndk::SharedRefBase::make<GraphicsComposerCallback>();
        EXPECT_TRUE(mComposerClient->registerCallback(mComposerCallback).isOk());

        // assume the first displays are built-in and are never removed
        mDisplays = waitForDisplays();
    }

    // returns an invalid display id (one that has not been registered to a
    // display.  Currently assuming that a device will never have close to
    // std::numeric_limit<uint64_t>::max() displays registered while running tests
    uint64_t GetInvalidDisplayId() {
        uint64_t id = std::numeric_limits<uint64_t>::max();
        while (id > 0) {
            if (std::none_of(mDisplays.begin(), mDisplays.end(),
                             [&](const VtsDisplay& display) { return id == display.get(); })) {
                return id;
            }
            id--;
        }

        return 0;
    }

    std::vector<VtsDisplay> waitForDisplays() {
        while (true) {
            // Sleep for a small period of time to allow all built-in displays
            // to post hotplug events
            std::this_thread::sleep_for(5ms);
            std::vector<int64_t> displays = mComposerCallback->getDisplays();
            if (displays.empty()) {
                continue;
            }

            std::vector<VtsDisplay> vtsDisplays;
            vtsDisplays.reserve(displays.size());
            for (int64_t display : displays) {
                Config activeConfig;
                EXPECT_TRUE(mComposerClient->getActiveConfig(display, &activeConfig).isOk());
                int32_t displayWidth;
                EXPECT_TRUE(mComposerClient
                                    ->getDisplayAttribute(display, activeConfig,
                                                          DisplayAttribute::WIDTH, &displayWidth)
                                    .isOk());
                int32_t displayHeight;
                EXPECT_TRUE(mComposerClient
                                    ->getDisplayAttribute(display, activeConfig,
                                                          DisplayAttribute::HEIGHT, &displayHeight)
                                    .isOk());
                vtsDisplays.emplace_back(VtsDisplay{display, displayWidth, displayHeight});
            }

            return vtsDisplays;
        }
    }

    std::shared_ptr<IComposer> mComposer;
    std::shared_ptr<IComposerClient> mComposerClient;
    int64_t mInvalidDisplayId;
    std::vector<VtsDisplay> mDisplays;
    std::shared_ptr<GraphicsComposerCallback> mComposerCallback;
};

TEST_P(GraphicsComposerAidlTest, getDisplayCapabilitiesBadDisplay) {
    std::vector<DisplayCapability> capabilities;
    const auto error = mComposerClient->getDisplayCapabilities(mInvalidDisplayId, &capabilities);

    EXPECT_FALSE(error.isOk());
    EXPECT_EQ(IComposerClient::EX_BAD_DISPLAY, error.getServiceSpecificError());
}

TEST_P(GraphicsComposerAidlTest, getDisplayCapabilities) {
    for (const auto& display : mDisplays) {
        std::vector<DisplayCapability> capabilities;

        EXPECT_TRUE(mComposerClient->getDisplayCapabilities(display.get(), &capabilities).isOk());
    }
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(GraphicsComposerAidlTest);
INSTANTIATE_TEST_SUITE_P(
        PerInstance, GraphicsComposerAidlTest,
        testing::ValuesIn(::android::getAidlHalInstanceNames(IComposer::descriptor)),
        ::android::PrintInstanceNameToString);
}  // namespace
}  // namespace aidl::android::hardware::graphics::composer3::vts

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);

    using namespace std::chrono_literals;
    if (!android::base::WaitForProperty("init.svc.surfaceflinger", "stopped", 10s)) {
        ALOGE("Failed to stop init.svc.surfaceflinger");
        return -1;
    }
    return RUN_ALL_TESTS();
}
