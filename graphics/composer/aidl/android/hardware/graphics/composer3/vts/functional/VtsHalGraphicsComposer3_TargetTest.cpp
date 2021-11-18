// TODO(b/129481165): remove the #pragma below and fix conversion issues
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wconversion"

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <aidl/android/hardware/graphics/common/BufferUsage.h>
#include <aidl/android/hardware/graphics/common/FRect.h>
#include <aidl/android/hardware/graphics/common/Rect.h>
#include <aidl/android/hardware/graphics/composer3/BlendMode.h>
#include <aidl/android/hardware/graphics/composer3/Composition.h>
#include <aidl/android/hardware/graphics/composer3/IComposer.h>
#include <android-base/properties.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <android/hardware/graphics/composer3/command-buffer.h>
#include <binder/ProcessState.h>
#include <gtest/gtest.h>
#include <ui/GraphicBuffer.h>
#include <ui/PixelFormat.h>
#include <algorithm>
#include <numeric>
#include <regex>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include "composer-vts/include/GraphicsComposerCallback.h"
#include "composer-vts/include/TestCommandReader.h"

// TODO(b/129481165): remove the #pragma below and fix conversion issues
#pragma clang diagnostic pop  // ignored "-Wconversion

#undef LOG_TAG
#define LOG_TAG "VtsHalGraphicsComposer3_TargetTest"

namespace aidl::android::hardware::graphics::composer3::vts {
namespace {

using namespace std::chrono_literals;

class VtsDisplay {
  public:
    VtsDisplay(int64_t displayId, int32_t displayWidth, int32_t displayHeight)
        : mDisplayId(displayId), mDisplayWidth(displayWidth), mDisplayHeight(displayHeight) {}

    int64_t get() const { return mDisplayId; }

    FRect getCrop() const {
        return {0, 0, static_cast<float>(mDisplayWidth), static_cast<float>(mDisplayHeight)};
    }

    Rect getFrameRect() const { return {0, 0, mDisplayWidth, mDisplayHeight}; }

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

        mPrimaryDisplay = mDisplays[0].get();

        // explicitly disable vsync
        for (const auto& display : mDisplays) {
            EXPECT_TRUE(mComposerClient->setVsyncEnabled(display.get(), false).isOk());
        }
        mComposerCallback->setVsyncAllowed(false);

        mWriter = std::make_unique<CommandWriterBase>(1024);
        mReader = std::make_unique<TestCommandReader>();
    }

    void TearDown() override {
        ASSERT_EQ(0, mReader->mErrors.size());
        ASSERT_EQ(0, mReader->mCompositionChanges.size());

        if (mComposerCallback != nullptr) {
            EXPECT_EQ(0, mComposerCallback->getInvalidHotplugCount());
            EXPECT_EQ(0, mComposerCallback->getInvalidRefreshCount());
            EXPECT_EQ(0, mComposerCallback->getInvalidVsyncCount());
            EXPECT_EQ(0, mComposerCallback->getInvalidVsyncCount());
            EXPECT_EQ(0, mComposerCallback->getInvalidVsyncPeriodChangeCount());
            EXPECT_EQ(0, mComposerCallback->getInvalidSeamlessPossibleCount());
        }
    }

    // returns an invalid display id (one that has not been registered to a
    // display.  Currently assuming that a device will never have close to
    // std::numeric_limit<uint64_t>::max() displays registered while running tests
    int64_t GetInvalidDisplayId() {
        int64_t id = std::numeric_limits<int64_t>::max();
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
                int32_t activeConfig;
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

    // returns an invalid config id which is std::numeric_limit<int32_t>::max()
    int32_t GetInvalidConfigId() { return IComposerClient::INVALID_CONFIGURATION; }

    ndk::ScopedAStatus setActiveConfigWithConstraints(
            VtsDisplay& display, int32_t config, const VsyncPeriodChangeConstraints& constraints,
            VsyncPeriodChangeTimeline* timeline) {
        auto error = mComposerClient->setActiveConfigWithConstraints(display.get(), config,
                                                                     constraints, timeline);
        if (error.isOk()) {
            int32_t displayWidth;
            EXPECT_TRUE(mComposerClient
                                ->getDisplayAttribute(display.get(), config,
                                                      DisplayAttribute::WIDTH, &displayWidth)
                                .isOk());
            int32_t displayHeight;
            EXPECT_TRUE(mComposerClient
                                ->getDisplayAttribute(display.get(), config,
                                                      DisplayAttribute::HEIGHT, &displayHeight)
                                .isOk());
            display.setDimensions(displayWidth, displayHeight);
        }
        return error;
    }

    void Test_setContentTypeForDisplay(const int64_t& display,
                                       const std::vector<ContentType>& capabilities,
                                       const ContentType& contentType, const char* contentTypeStr) {
        const bool contentTypeSupport = std::find(capabilities.begin(), capabilities.end(),
                                                  contentType) != capabilities.end();

        if (!contentTypeSupport) {
            EXPECT_EQ(IComposerClient::EX_UNSUPPORTED,
                      mComposerClient->setContentType(display, contentType)
                              .getServiceSpecificError());
            GTEST_SUCCEED() << contentTypeStr << " content type is not supported on display "
                            << std::to_string(display) << ", skipping test";
            return;
        }

        EXPECT_TRUE(mComposerClient->setContentType(display, contentType).isOk());
        EXPECT_TRUE(mComposerClient->setContentType(display, ContentType::NONE).isOk());
    }

    void Test_setContentType(const ContentType& contentType, const char* contentTypeStr) {
        for (const auto& display : mDisplays) {
            std::vector<ContentType> supportedContentTypes;
            const auto error = mComposerClient->getSupportedContentTypes(display.get(),
                                                                         &supportedContentTypes);
            EXPECT_TRUE(error.isOk());

            Test_setContentTypeForDisplay(display.get(), supportedContentTypes, contentType,
                                          contentTypeStr);
        }
    }

    void execute() {
        TestCommandReader* reader = mReader.get();
        CommandWriterBase* writer = mWriter.get();
        bool queueChanged = false;
        int32_t commandLength = 0;
        std::vector<NativeHandle> commandHandles;
        ASSERT_TRUE(writer->writeQueue(&queueChanged, &commandLength, &commandHandles));

        if (queueChanged) {
            auto ret = mComposerClient->setInputCommandQueue(writer->getMQDescriptor());
            ASSERT_TRUE(ret.isOk());
        }

        ExecuteCommandsStatus commandStatus;
        EXPECT_TRUE(mComposerClient->executeCommands(commandLength, commandHandles, &commandStatus)
                            .isOk());

        if (commandStatus.queueChanged) {
            MQDescriptor<int32_t, SynchronizedReadWrite> outputCommandQueue;
            ASSERT_TRUE(mComposerClient->getOutputCommandQueue(&outputCommandQueue).isOk());
            reader->setMQDescriptor(outputCommandQueue);
        }
        ASSERT_TRUE(reader->readQueue(commandStatus.length, std::move(commandStatus.handles)));
        reader->parse();
        reader->reset();
        writer->reset();
    }

    ::android::sp<::android::GraphicBuffer> allocate(uint32_t width, uint32_t height) {
        ::android::sp<::android::GraphicBuffer> buffer =
                ::android::sp<::android::GraphicBuffer>::make(
                        width, height, ::android::PIXEL_FORMAT_RGBA_8888,
                        /*layerCount*/ 1,
                        static_cast<uint64_t>(
                                static_cast<int>(common::BufferUsage::CPU_WRITE_OFTEN) |
                                static_cast<int>(common::BufferUsage::CPU_READ_OFTEN)),
                        "VtsHalGraphicsComposer3_TargetTest");

        return buffer;
    }

    struct TestParameters {
        nsecs_t delayForChange;
        bool refreshMiss;
    };

    static inline auto toTimePoint(nsecs_t time) {
        return std::chrono::time_point<std::chrono::steady_clock>(std::chrono::nanoseconds(time));
    }

    int64_t createLayer(const VtsDisplay& display) {
        int64_t layer;
        EXPECT_TRUE(mComposerClient->createLayer(display.get(), kBufferSlotCount, &layer).isOk());

        auto resourceIt = mDisplayResources.find(display.get());
        if (resourceIt == mDisplayResources.end()) {
            resourceIt = mDisplayResources.insert({display.get(), DisplayResource(false)}).first;
        }

        EXPECT_TRUE(resourceIt->second.layers.insert(layer).second)
                << "duplicated layer id " << layer;

        return layer;
    }

    void destroyLayer(const VtsDisplay& display, int64_t layer) {
        auto const error = mComposerClient->destroyLayer(display.get(), layer);
        ASSERT_TRUE(error.isOk()) << "failed to destroy layer " << layer;

        auto resourceIt = mDisplayResources.find(display.get());
        ASSERT_NE(mDisplayResources.end(), resourceIt);
        resourceIt->second.layers.erase(layer);
    }

    void forEachTwoConfigs(int64_t display, std::function<void(int32_t, int32_t)> func) {
        std::vector<int32_t> displayConfigs;
        EXPECT_TRUE(mComposerClient->getDisplayConfigs(display, &displayConfigs).isOk());
        for (const int32_t config1 : displayConfigs) {
            for (const int32_t config2 : displayConfigs) {
                if (config1 != config2) {
                    func(config1, config2);
                }
            }
        }
    }

    void setActiveConfig(VtsDisplay& display, int32_t config) {
        EXPECT_TRUE(mComposerClient->setActiveConfig(display.get(), config).isOk());
        int32_t displayWidth;
        EXPECT_TRUE(mComposerClient
                            ->getDisplayAttribute(display.get(), config, DisplayAttribute::WIDTH,
                                                  &displayWidth)
                            .isOk());
        int32_t displayHeight;
        EXPECT_TRUE(mComposerClient
                            ->getDisplayAttribute(display.get(), config, DisplayAttribute::HEIGHT,
                                                  &displayHeight)
                            .isOk());
        display.setDimensions(displayWidth, displayHeight);
    }

    void sendRefreshFrame(const VtsDisplay& display, const VsyncPeriodChangeTimeline* timeline) {
        if (timeline != nullptr) {
            // Refresh time should be before newVsyncAppliedTimeNanos
            EXPECT_LT(timeline->refreshTimeNanos, timeline->newVsyncAppliedTimeNanos);

            std::this_thread::sleep_until(toTimePoint(timeline->refreshTimeNanos));
        }

        mWriter->selectDisplay(display.get());
        EXPECT_TRUE(mComposerClient->setPowerMode(display.get(), PowerMode::ON).isOk());
        EXPECT_TRUE(
                mComposerClient
                        ->setColorMode(display.get(), ColorMode::NATIVE, RenderIntent::COLORIMETRIC)
                        .isOk());

        FRect displayCrop = display.getCrop();
        auto displayWidth = static_cast<uint32_t>(std::ceilf(displayCrop.right - displayCrop.left));
        auto displayHeight =
                static_cast<uint32_t>(std::ceilf(displayCrop.bottom - displayCrop.top));
        int64_t layer = 0;
        ASSERT_NO_FATAL_FAILURE(layer = createLayer(display));
        {
            auto buffer = allocate(displayWidth, displayHeight);
            ASSERT_NE(nullptr, buffer);
            ASSERT_EQ(::android::OK, buffer->initCheck());
            ASSERT_NE(nullptr, buffer->handle);

            mWriter->selectLayer(layer);
            mWriter->setLayerCompositionType(Composition::DEVICE);
            mWriter->setLayerDisplayFrame(display.getFrameRect());
            mWriter->setLayerPlaneAlpha(1);
            mWriter->setLayerSourceCrop(display.getCrop());
            mWriter->setLayerTransform(static_cast<Transform>(0));
            mWriter->setLayerVisibleRegion(std::vector<Rect>(1, display.getFrameRect()));
            mWriter->setLayerZOrder(10);
            mWriter->setLayerBlendMode(BlendMode::NONE);
            mWriter->setLayerSurfaceDamage(std::vector<Rect>(1, display.getFrameRect()));
            mWriter->setLayerBuffer(0, buffer->handle, -1);
            mWriter->setLayerDataspace(common::Dataspace::UNKNOWN);

            mWriter->validateDisplay();
            execute();
            ASSERT_EQ(0, mReader->mErrors.size());
            mReader->mCompositionChanges.clear();

            mWriter->presentDisplay();
            execute();
            ASSERT_EQ(0, mReader->mErrors.size());
        }

        {
            auto buffer = allocate(displayWidth, displayHeight);
            ASSERT_NE(nullptr, buffer->handle);

            mWriter->selectLayer(layer);
            mWriter->setLayerBuffer(0, buffer->handle, -1);
            mWriter->setLayerSurfaceDamage(std::vector<Rect>(1, {0, 0, 10, 10}));
            mWriter->validateDisplay();
            execute();
            ASSERT_EQ(0, mReader->mErrors.size());
            mReader->mCompositionChanges.clear();

            mWriter->presentDisplay();
            execute();
        }

        ASSERT_NO_FATAL_FAILURE(destroyLayer(display, layer));
    }

    void waitForVsyncPeriodChange(int64_t display, const VsyncPeriodChangeTimeline& timeline,
                                  int64_t desiredTimeNanos, int64_t oldPeriodNanos,
                                  int64_t newPeriodNanos) {
        const auto kChangeDeadline = toTimePoint(timeline.newVsyncAppliedTimeNanos) + 100ms;
        while (std::chrono::steady_clock::now() <= kChangeDeadline) {
            int32_t vsyncPeriodNanos;
            EXPECT_TRUE(mComposerClient->getDisplayVsyncPeriod(display, &vsyncPeriodNanos).isOk());
            if (systemTime() <= desiredTimeNanos) {
                EXPECT_EQ(vsyncPeriodNanos, oldPeriodNanos);
            } else if (vsyncPeriodNanos == newPeriodNanos) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::nanoseconds(oldPeriodNanos));
        }
    }

    void Test_setActiveConfigWithConstraints(const TestParameters& params) {
        for (VtsDisplay& display : mDisplays) {
            forEachTwoConfigs(display.get(), [&](int32_t config1, int32_t config2) {
                setActiveConfig(display, config1);
                sendRefreshFrame(display, nullptr);

                int32_t vsyncPeriod1;
                EXPECT_TRUE(mComposerClient
                                    ->getDisplayAttribute(display.get(), config1,
                                                          DisplayAttribute::VSYNC_PERIOD,
                                                          &vsyncPeriod1)
                                    .isOk());
                int32_t configGroup1;
                EXPECT_TRUE(mComposerClient
                                    ->getDisplayAttribute(display.get(), config1,
                                                          DisplayAttribute::CONFIG_GROUP,
                                                          &configGroup1)
                                    .isOk());
                int32_t vsyncPeriod2;
                EXPECT_TRUE(mComposerClient
                                    ->getDisplayAttribute(display.get(), config2,
                                                          DisplayAttribute::VSYNC_PERIOD,
                                                          &vsyncPeriod2)
                                    .isOk());
                int32_t configGroup2;
                EXPECT_TRUE(mComposerClient
                                    ->getDisplayAttribute(display.get(), config2,
                                                          DisplayAttribute::CONFIG_GROUP,
                                                          &configGroup2)
                                    .isOk());

                if (vsyncPeriod1 == vsyncPeriod2) {
                    return;  // continue
                }

                // We don't allow delayed change when changing config groups
                if (params.delayForChange > 0 && configGroup1 != configGroup2) {
                    return;  // continue
                }

                VsyncPeriodChangeTimeline timeline;
                VsyncPeriodChangeConstraints constraints = {
                        .desiredTimeNanos = systemTime() + params.delayForChange,
                        .seamlessRequired = false};
                EXPECT_TRUE(setActiveConfigWithConstraints(display, config2, constraints, &timeline)
                                    .isOk());

                EXPECT_TRUE(timeline.newVsyncAppliedTimeNanos >= constraints.desiredTimeNanos);
                // Refresh rate should change within a reasonable time
                constexpr std::chrono::nanoseconds kReasonableTimeForChange = 1s;  // 1 second
                EXPECT_TRUE(timeline.newVsyncAppliedTimeNanos - constraints.desiredTimeNanos <=
                            kReasonableTimeForChange.count());

                if (timeline.refreshRequired) {
                    if (params.refreshMiss) {
                        // Miss the refresh frame on purpose to make sure the implementation sends a
                        // callback
                        std::this_thread::sleep_until(toTimePoint(timeline.refreshTimeNanos) +
                                                      100ms);
                    }
                    sendRefreshFrame(display, &timeline);
                }
                waitForVsyncPeriodChange(display.get(), timeline, constraints.desiredTimeNanos,
                                         vsyncPeriod1, vsyncPeriod2);

                // At this point the refresh rate should have changed already, however in rare
                // cases the implementation might have missed the deadline. In this case a new
                // timeline should have been provided.
                auto newTimeline = mComposerCallback->takeLastVsyncPeriodChangeTimeline();
                if (timeline.refreshRequired && params.refreshMiss) {
                    EXPECT_TRUE(newTimeline.has_value());
                }

                if (newTimeline.has_value()) {
                    if (newTimeline->refreshRequired) {
                        sendRefreshFrame(display, &newTimeline.value());
                    }
                    waitForVsyncPeriodChange(display.get(), newTimeline.value(),
                                             constraints.desiredTimeNanos, vsyncPeriod1,
                                             vsyncPeriod2);
                }

                int32_t vsyncPeriodNanos;
                EXPECT_TRUE(mComposerClient->getDisplayVsyncPeriod(display.get(), &vsyncPeriodNanos)
                                    .isOk());
                EXPECT_EQ(vsyncPeriodNanos, vsyncPeriod2);
            });
        }
    }

    // Keep track of all virtual displays and layers.  When a test fails with
    // ASSERT_*, the destructor will clean up the resources for the test.
    struct DisplayResource {
        DisplayResource(bool isVirtual_) : isVirtual(isVirtual_) {}

        bool isVirtual;
        std::unordered_set<int64_t> layers;
    };

    std::shared_ptr<IComposer> mComposer;
    std::shared_ptr<IComposerClient> mComposerClient;
    int64_t mInvalidDisplayId;
    int64_t mPrimaryDisplay;
    std::vector<VtsDisplay> mDisplays;
    std::shared_ptr<GraphicsComposerCallback> mComposerCallback;
    std::unique_ptr<CommandWriterBase> mWriter;
    std::unique_ptr<TestCommandReader> mReader;
    // use the slot count usually set by SF
    static constexpr uint32_t kBufferSlotCount = 64;
    std::unordered_map<int64_t, DisplayResource> mDisplayResources;
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

TEST_P(GraphicsComposerAidlTest, getDisplayVsyncPeriod) {
    for (VtsDisplay& display : mDisplays) {
        std::vector<int32_t> configs;
        EXPECT_TRUE(mComposerClient->getDisplayConfigs(display.get(), &configs).isOk());
        for (int32_t config : configs) {
            int32_t expectedVsyncPeriodNanos = -1;
            EXPECT_TRUE(mComposerClient
                                ->getDisplayAttribute(display.get(), config,
                                                      DisplayAttribute::VSYNC_PERIOD,
                                                      &expectedVsyncPeriodNanos)
                                .isOk());

            VsyncPeriodChangeTimeline timeline;
            VsyncPeriodChangeConstraints constraints;

            constraints.desiredTimeNanos = systemTime();
            constraints.seamlessRequired = false;
            EXPECT_TRUE(mComposerClient
                                ->setActiveConfigWithConstraints(display.get(), config, constraints,
                                                                 &timeline)
                                .isOk());

            if (timeline.refreshRequired) {
                sendRefreshFrame(display, &timeline);
            }
            waitForVsyncPeriodChange(display.get(), timeline, constraints.desiredTimeNanos, 0,
                                     expectedVsyncPeriodNanos);

            int32_t vsyncPeriodNanos;
            int retryCount = 100;
            do {
                std::this_thread::sleep_for(10ms);
                vsyncPeriodNanos = 0;
                EXPECT_TRUE(mComposerClient->getDisplayVsyncPeriod(display.get(), &vsyncPeriodNanos)
                                    .isOk());
                --retryCount;
            } while (vsyncPeriodNanos != expectedVsyncPeriodNanos && retryCount > 0);

            EXPECT_EQ(vsyncPeriodNanos, expectedVsyncPeriodNanos);

            // Make sure that the vsync period stays the same if the active config is not
            // changed.
            auto timeout = 1ms;
            for (int i = 0; i < 10; i++) {
                std::this_thread::sleep_for(timeout);
                timeout *= 2;
                vsyncPeriodNanos = 0;
                EXPECT_TRUE(mComposerClient->getDisplayVsyncPeriod(display.get(), &vsyncPeriodNanos)
                                    .isOk());
                EXPECT_EQ(vsyncPeriodNanos, expectedVsyncPeriodNanos);
            }
        }
    }
}

TEST_P(GraphicsComposerAidlTest, setActiveConfigWithConstraints_SeamlessNotAllowed) {
    VsyncPeriodChangeTimeline timeline;
    VsyncPeriodChangeConstraints constraints;

    constraints.seamlessRequired = true;
    constraints.desiredTimeNanos = systemTime();

    for (VtsDisplay& display : mDisplays) {
        forEachTwoConfigs(display.get(), [&](int32_t config1, int32_t config2) {
            int32_t configGroup1;
            EXPECT_TRUE(mComposerClient
                                ->getDisplayAttribute(display.get(), config1,
                                                      DisplayAttribute::CONFIG_GROUP, &configGroup1)
                                .isOk());
            int32_t configGroup2;
            EXPECT_TRUE(mComposerClient
                                ->getDisplayAttribute(display.get(), config2,
                                                      DisplayAttribute::CONFIG_GROUP, &configGroup2)
                                .isOk());
            if (configGroup1 != configGroup2) {
                setActiveConfig(display, config1);
                sendRefreshFrame(display, nullptr);
                EXPECT_EQ(IComposerClient::EX_SEAMLESS_NOT_ALLOWED,
                          setActiveConfigWithConstraints(display, config2, constraints, &timeline)
                                  .getServiceSpecificError());
            }
        });
    }
}

TEST_P(GraphicsComposerAidlTest, setActiveConfigWithConstraints) {
    Test_setActiveConfigWithConstraints({.delayForChange = 0, .refreshMiss = false});
}

TEST_P(GraphicsComposerAidlTest, GetDisplayIdentificationData) {
    DisplayIdentification displayIdentification0;

    const auto error =
            mComposerClient->getDisplayIdentificationData(mPrimaryDisplay, &displayIdentification0);
    if (error.getServiceSpecificError() == IComposerClient::EX_UNSUPPORTED) {
        return;
    }
    ASSERT_TRUE(error.isOk()) << "failed to get display identification data";
    ASSERT_FALSE(displayIdentification0.data.empty());

    constexpr size_t kEdidBlockSize = 128;
    ASSERT_TRUE(displayIdentification0.data.size() % kEdidBlockSize == 0)
            << "EDID blob length is not a multiple of " << kEdidBlockSize;

    const uint8_t kEdidHeader[] = {0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00};
    ASSERT_TRUE(std::equal(std::begin(kEdidHeader), std::end(kEdidHeader),
                           displayIdentification0.data.begin()))
            << "EDID blob doesn't start with the fixed EDID header";
    ASSERT_EQ(0, std::accumulate(displayIdentification0.data.begin(),
                                 displayIdentification0.data.begin() + kEdidBlockSize,
                                 static_cast<uint8_t>(0)))
            << "EDID base block doesn't checksum";

    DisplayIdentification displayIdentification1;
    ASSERT_TRUE(
            mComposerClient->getDisplayIdentificationData(mPrimaryDisplay, &displayIdentification1)
                    .isOk());

    ASSERT_EQ(displayIdentification0.port, displayIdentification1.port) << "ports are not stable";
    ASSERT_TRUE(displayIdentification0.data.size() == displayIdentification1.data.size() &&
                std::equal(displayIdentification0.data.begin(), displayIdentification0.data.end(),
                           displayIdentification1.data.begin()))
            << "data is not stable";
}

TEST_P(GraphicsComposerAidlTest, GetHdrCapabilities) {
    HdrCapabilities hdrCapabilities;
    const auto error = mComposerClient->getHdrCapabilities(mPrimaryDisplay, &hdrCapabilities);

    ASSERT_TRUE(error.isOk());
    ASSERT_TRUE(hdrCapabilities.maxLuminance >= hdrCapabilities.minLuminance);
}

TEST_P(GraphicsComposerAidlTest, GetPerFrameMetadataKeys) {
    std::vector<PerFrameMetadataKey> keys;
    const auto error = mComposerClient->getPerFrameMetadataKeys(mPrimaryDisplay, &keys);

    if (error.getServiceSpecificError() == IComposerClient::EX_UNSUPPORTED) {
        GTEST_SUCCEED() << "getPerFrameMetadataKeys is not supported";
        return;
    }
    EXPECT_TRUE(error.isOk());
    ASSERT_TRUE(keys.size() >= 0);
}

TEST_P(GraphicsComposerAidlTest, GetReadbackBufferAttributes) {
    ReadbackBufferAttributes readBackBufferAttributes;
    const auto error = mComposerClient->getReadbackBufferAttributes(mPrimaryDisplay,
                                                                    &readBackBufferAttributes);

    if (error.isOk()) {
        EXPECT_EQ(EX_NONE, error.getServiceSpecificError());
    }
}

TEST_P(GraphicsComposerAidlTest, GetRenderIntents) {
    std::vector<ColorMode> modes;
    EXPECT_TRUE(mComposerClient->getColorModes(mPrimaryDisplay, &modes).isOk());
    for (auto mode : modes) {
        std::vector<RenderIntent> intents;
        EXPECT_TRUE(mComposerClient->getRenderIntents(mPrimaryDisplay, mode, &intents).isOk());

        bool isHdr;
        switch (mode) {
            case ColorMode::BT2100_PQ:
            case ColorMode::BT2100_HLG:
                isHdr = true;
                break;
            default:
                isHdr = false;
                break;
        }
        RenderIntent requiredIntent =
                isHdr ? RenderIntent::TONE_MAP_COLORIMETRIC : RenderIntent::COLORIMETRIC;

        auto iter = std::find(intents.cbegin(), intents.cend(), requiredIntent);
        EXPECT_NE(intents.cend(), iter);
    }
}

TEST_P(GraphicsComposerAidlTest, GetRenderIntentsBadDisplay) {
    std::vector<ColorMode> modes;
    EXPECT_TRUE(mComposerClient->getColorModes(mPrimaryDisplay, &modes).isOk());
    for (auto mode : modes) {
        std::vector<RenderIntent> renderIntents;
        const auto error =
                mComposerClient->getRenderIntents(mInvalidDisplayId, mode, &renderIntents);
        EXPECT_FALSE(error.isOk());
        EXPECT_EQ(IComposerClient::EX_BAD_DISPLAY, error.getServiceSpecificError());
    }
}

TEST_P(GraphicsComposerAidlTest, GetRenderIntentsBadParameter) {
    std::vector<RenderIntent> renderIntents;
    const auto error = mComposerClient->getRenderIntents(
            mPrimaryDisplay, static_cast<ColorMode>(-1), &renderIntents);
    EXPECT_FALSE(error.isOk());
    EXPECT_EQ(IComposerClient::EX_BAD_PARAMETER, error.getServiceSpecificError());
}

TEST_P(GraphicsComposerAidlTest, GetColorModes) {
    std::vector<ColorMode> colorModes;
    EXPECT_TRUE(mComposerClient->getColorModes(mPrimaryDisplay, &colorModes).isOk());

    auto native = std::find(colorModes.cbegin(), colorModes.cend(), ColorMode::NATIVE);
    ASSERT_NE(colorModes.cend(), native);
}

TEST_P(GraphicsComposerAidlTest, GetColorModeBadDisplay) {
    std::vector<ColorMode> colorModes;
    const auto error = mComposerClient->getColorModes(mInvalidDisplayId, &colorModes);

    EXPECT_FALSE(error.isOk());
    EXPECT_EQ(IComposerClient::EX_BAD_DISPLAY, error.getServiceSpecificError());
}

TEST_P(GraphicsComposerAidlTest, SetColorMode) {
    std::vector<ColorMode> colorModes;
    EXPECT_TRUE(mComposerClient->getColorModes(mPrimaryDisplay, &colorModes).isOk());
    for (auto mode : colorModes) {
        std::vector<RenderIntent> intents;
        EXPECT_TRUE(mComposerClient->getRenderIntents(mPrimaryDisplay, mode, &intents).isOk())
                << "failed to get render intents";
        for (auto intent : intents) {
            const auto error = mComposerClient->setColorMode(mPrimaryDisplay, mode, intent);
            EXPECT_TRUE(error.isOk() ||
                        IComposerClient::EX_UNSUPPORTED == error.getServiceSpecificError())
                    << "failed to set color mode";
        }
    }

    const auto error = mComposerClient->setColorMode(mPrimaryDisplay, ColorMode::NATIVE,
                                                     RenderIntent::COLORIMETRIC);
    EXPECT_TRUE(error.isOk() || IComposerClient::EX_UNSUPPORTED == error.getServiceSpecificError())
            << "failed to set color mode";
}

TEST_P(GraphicsComposerAidlTest, SetColorModeBadDisplay) {
    auto const error = mComposerClient->setColorMode(mInvalidDisplayId, ColorMode::NATIVE,
                                                     RenderIntent::COLORIMETRIC);

    EXPECT_FALSE(error.isOk());
    ASSERT_EQ(IComposerClient::EX_BAD_DISPLAY, error.getServiceSpecificError());
}

TEST_P(GraphicsComposerAidlTest, SetColorModeBadParameter) {
    const auto colorModeError = mComposerClient->setColorMode(
            mPrimaryDisplay, static_cast<ColorMode>(-1), RenderIntent::COLORIMETRIC);

    EXPECT_FALSE(colorModeError.isOk());
    EXPECT_EQ(IComposerClient::EX_BAD_PARAMETER, colorModeError.getServiceSpecificError());

    const auto renderIntentError = mComposerClient->setColorMode(mPrimaryDisplay, ColorMode::NATIVE,
                                                                 static_cast<RenderIntent>(-1));

    EXPECT_FALSE(renderIntentError.isOk());
    EXPECT_EQ(IComposerClient::EX_BAD_PARAMETER, renderIntentError.getServiceSpecificError());
}

TEST_P(GraphicsComposerAidlTest, GetDisplayedContentSamplingAttributes) {
    int constexpr invalid = -1;

    DisplayContentSamplingAttributes format;
    auto error = mComposerClient->getDisplayedContentSamplingAttributes(mPrimaryDisplay, &format);

    if (error.getServiceSpecificError() == IComposerClient::EX_UNSUPPORTED) {
        SUCCEED() << "Device does not support optional extension. Test skipped";
        return;
    }

    EXPECT_TRUE(error.isOk());
    EXPECT_NE(format.format, static_cast<common::PixelFormat>(invalid));
    EXPECT_NE(format.dataspace, static_cast<common::Dataspace>(invalid));
    EXPECT_NE(format.componentMask, static_cast<FormatColorComponent>(invalid));
};

TEST_P(GraphicsComposerAidlTest, SetDisplayedContentSamplingEnabled) {
    auto const maxFrames = 10;
    FormatColorComponent enableAllComponents = FormatColorComponent::FORMAT_COMPONENT_0;
    auto error = mComposerClient->setDisplayedContentSamplingEnabled(
            mPrimaryDisplay, true, enableAllComponents, maxFrames);
    if (error.getServiceSpecificError() == IComposerClient::EX_UNSUPPORTED) {
        SUCCEED() << "Device does not support optional extension. Test skipped";
        return;
    }
    EXPECT_TRUE(error.isOk());

    error = mComposerClient->setDisplayedContentSamplingEnabled(mPrimaryDisplay, false,
                                                                enableAllComponents, maxFrames);
    EXPECT_TRUE(error.isOk());
}

TEST_P(GraphicsComposerAidlTest, GetDisplayedContentSample) {
    DisplayContentSamplingAttributes displayContentSamplingAttributes;
    int constexpr invalid = -1;
    displayContentSamplingAttributes.format = static_cast<common::PixelFormat>(invalid);
    displayContentSamplingAttributes.dataspace = static_cast<common::Dataspace>(invalid);
    displayContentSamplingAttributes.componentMask = static_cast<FormatColorComponent>(invalid);
    auto error = mComposerClient->getDisplayedContentSamplingAttributes(
            mPrimaryDisplay, &displayContentSamplingAttributes);
    if (error.getServiceSpecificError() == IComposerClient::EX_UNSUPPORTED) {
        SUCCEED() << "Sampling attributes aren't supported on this device, test skipped";
        return;
    }

    int64_t maxFrames = 10;
    int64_t timestamp = 0;
    int64_t frameCount = 0;
    DisplayContentSample displayContentSample;
    error = mComposerClient->getDisplayedContentSample(mPrimaryDisplay, maxFrames, timestamp,
                                                       &displayContentSample);
    if (error.getServiceSpecificError() == IComposerClient::EX_UNSUPPORTED) {
        SUCCEED() << "Device does not support optional extension. Test skipped";
        return;
    }

    EXPECT_TRUE(error.isOk());
    EXPECT_LE(frameCount, maxFrames);
    std::vector<std::vector<int64_t>> histogram = {
            displayContentSample.sampleComponent0, displayContentSample.sampleComponent1,
            displayContentSample.sampleComponent2, displayContentSample.sampleComponent3};

    for (size_t i = 0; i < histogram.size(); i++) {
        const bool shouldHaveHistogram =
                static_cast<int>(displayContentSamplingAttributes.componentMask) & (1 << i);
        EXPECT_EQ(shouldHaveHistogram, !histogram[i].empty());
    }
}

TEST_P(GraphicsComposerAidlTest, getDisplayCapabilitiesBasic) {
    std::vector<DisplayCapability> capabilities;
    const auto error = mComposerClient->getDisplayCapabilities(mPrimaryDisplay, &capabilities);
    ASSERT_TRUE(error.isOk());
    const bool hasDozeSupport = std::find(capabilities.begin(), capabilities.end(),
                                          DisplayCapability::DOZE) != capabilities.end();
    bool isDozeSupported = false;
    EXPECT_TRUE(mComposerClient->getDozeSupport(mPrimaryDisplay, &isDozeSupported).isOk());
    EXPECT_EQ(hasDozeSupport, isDozeSupported);

    bool hasBrightnessSupport = std::find(capabilities.begin(), capabilities.end(),
                                          DisplayCapability::BRIGHTNESS) != capabilities.end();
    bool isBrightnessSupported = false;
    EXPECT_TRUE(
            mComposerClient->getDisplayBrightnessSupport(mPrimaryDisplay, &isBrightnessSupported)
                    .isOk());
    EXPECT_EQ(isBrightnessSupported, hasBrightnessSupport);
}

/*
 * Test that if brightness operations are supported, setDisplayBrightness works as expected.
 */
TEST_P(GraphicsComposerAidlTest, setDisplayBrightness) {
    std::vector<DisplayCapability> capabilities;
    auto error = mComposerClient->getDisplayCapabilities(mPrimaryDisplay, &capabilities);
    ASSERT_TRUE(error.isOk());
    bool brightnessSupport = std::find(capabilities.begin(), capabilities.end(),
                                       DisplayCapability::BRIGHTNESS) != capabilities.end();
    if (!brightnessSupport) {
        EXPECT_EQ(mComposerClient->setDisplayBrightness(mPrimaryDisplay, 0.5f)
                          .getServiceSpecificError(),
                  IComposerClient::EX_UNSUPPORTED);
        GTEST_SUCCEED() << "Brightness operations are not supported";
        return;
    }

    EXPECT_TRUE(mComposerClient->setDisplayBrightness(mPrimaryDisplay, 0.0f).isOk());
    EXPECT_TRUE(mComposerClient->setDisplayBrightness(mPrimaryDisplay, 0.5f).isOk());
    EXPECT_TRUE(mComposerClient->setDisplayBrightness(mPrimaryDisplay, 1.0f).isOk());
    EXPECT_TRUE(mComposerClient->setDisplayBrightness(mPrimaryDisplay, -1.0f).isOk());

    error = mComposerClient->setDisplayBrightness(mPrimaryDisplay, +2.0f);
    EXPECT_FALSE(error.isOk());
    EXPECT_EQ(error.getServiceSpecificError(), IComposerClient::EX_BAD_PARAMETER);

    error = mComposerClient->setDisplayBrightness(mPrimaryDisplay, -2.0f);
    EXPECT_FALSE(error.isOk());
    EXPECT_EQ(error.getServiceSpecificError(), IComposerClient::EX_BAD_PARAMETER);
}

TEST_P(GraphicsComposerAidlTest, getDisplayConnectionType) {
    DisplayConnectionType type;
    EXPECT_FALSE(mComposerClient->getDisplayConnectionType(mInvalidDisplayId, &type).isOk());
    for (const auto& display : mDisplays) {
        EXPECT_TRUE(mComposerClient->getDisplayConnectionType(display.get(), &type).isOk());
    }
}

TEST_P(GraphicsComposerAidlTest, getDisplayAttribute) {
    for (const auto& display : mDisplays) {
        std::vector<int32_t> configs;
        EXPECT_TRUE(mComposerClient->getDisplayConfigs(display.get(), &configs).isOk());
        for (const auto& config : configs) {
            const std::array<DisplayAttribute, 4> requiredAttributes = {{
                    DisplayAttribute::WIDTH,
                    DisplayAttribute::HEIGHT,
                    DisplayAttribute::VSYNC_PERIOD,
                    DisplayAttribute::CONFIG_GROUP,
            }};
            int32_t value;
            for (const auto& attribute : requiredAttributes) {
                EXPECT_TRUE(mComposerClient
                                    ->getDisplayAttribute(display.get(), config, attribute, &value)
                                    .isOk());
                EXPECT_NE(-1, value);
            }

            const std::array<DisplayAttribute, 2> optionalAttributes = {{
                    DisplayAttribute::DPI_X,
                    DisplayAttribute::DPI_Y,
            }};
            for (const auto& attribute : optionalAttributes) {
                const auto error = mComposerClient->getDisplayAttribute(display.get(), config,
                                                                        attribute, &value);
                if (error.isOk()) {
                    EXPECT_EQ(EX_NONE, error.getServiceSpecificError());
                } else {
                    EXPECT_EQ(IComposerClient::EX_UNSUPPORTED, error.getServiceSpecificError());
                }
            }
        }
    }
}

TEST_P(GraphicsComposerAidlTest, checkConfigsAreValid) {
    for (const auto& display : mDisplays) {
        std::vector<int32_t> configs;
        EXPECT_TRUE(mComposerClient->getDisplayConfigs(display.get(), &configs).isOk());

        EXPECT_FALSE(std::any_of(configs.begin(), configs.end(), [](auto config) {
            return config == IComposerClient::INVALID_CONFIGURATION;
        }));
    }
}

TEST_P(GraphicsComposerAidlTest, getDisplayAttributeConfigsInAGroupDifferOnlyByVsyncPeriod) {
    struct Resolution {
        int32_t width;
        int32_t height;
    };
    struct Dpi {
        int32_t x;
        int32_t y;
    };
    for (const auto& display : mDisplays) {
        std::vector<int32_t> configs;
        EXPECT_TRUE(mComposerClient->getDisplayConfigs(display.get(), &configs).isOk());
        std::unordered_map<int32_t, Resolution> configGroupToResolutionMap;
        std::unordered_map<int32_t, Dpi> configGroupToDpiMap;
        for (const auto& config : configs) {
            int32_t configGroup = -1;
            EXPECT_TRUE(mComposerClient
                                ->getDisplayAttribute(display.get(), config,
                                                      DisplayAttribute::CONFIG_GROUP, &configGroup)
                                .isOk());
            int32_t width = -1;
            EXPECT_TRUE(mComposerClient
                                ->getDisplayAttribute(display.get(), config,
                                                      DisplayAttribute::WIDTH, &width)
                                .isOk());
            int32_t height = -1;
            EXPECT_TRUE(mComposerClient
                                ->getDisplayAttribute(display.get(), config,
                                                      DisplayAttribute::HEIGHT, &height)
                                .isOk());
            if (configGroupToResolutionMap.find(configGroup) == configGroupToResolutionMap.end()) {
                configGroupToResolutionMap[configGroup] = {width, height};
            }
            EXPECT_EQ(configGroupToResolutionMap[configGroup].width, width);
            EXPECT_EQ(configGroupToResolutionMap[configGroup].height, height);

            int32_t dpiX = -1;
            mComposerClient->getDisplayAttribute(display.get(), config, DisplayAttribute::DPI_X,
                                                 &dpiX);

            int32_t dpiY = -1;
            mComposerClient->getDisplayAttribute(display.get(), config, DisplayAttribute::DPI_Y,
                                                 &dpiY);
            if (dpiX == -1 && dpiY == -1) {
                continue;
            }

            if (configGroupToDpiMap.find(configGroup) == configGroupToDpiMap.end()) {
                configGroupToDpiMap[configGroup] = {dpiX, dpiY};
            }
            EXPECT_EQ(configGroupToDpiMap[configGroup].x, dpiX);
            EXPECT_EQ(configGroupToDpiMap[configGroup].y, dpiY);
        }
    }
}

TEST_P(GraphicsComposerAidlTest, getDisplayVsyncPeriod_BadDisplay) {
    int32_t vsyncPeriodNanos;
    const auto error = mComposerClient->getDisplayVsyncPeriod(mInvalidDisplayId, &vsyncPeriodNanos);
    EXPECT_FALSE(error.isOk());
    EXPECT_EQ(IComposerClient::EX_BAD_DISPLAY, error.getServiceSpecificError());
}

TEST_P(GraphicsComposerAidlTest, setActiveConfigWithConstraints_BadDisplay) {
    VsyncPeriodChangeTimeline timeline;
    VsyncPeriodChangeConstraints constraints;

    constraints.seamlessRequired = false;
    constraints.desiredTimeNanos = systemTime();
    int32_t config = 0;
    auto const error = mComposerClient->setActiveConfigWithConstraints(mInvalidDisplayId, config,
                                                                       constraints, &timeline);

    EXPECT_FALSE(error.isOk());
    EXPECT_EQ(IComposerClient::EX_BAD_DISPLAY, error.getServiceSpecificError());
}

TEST_P(GraphicsComposerAidlTest, setActiveConfigWithConstraints_BadConfig) {
    VsyncPeriodChangeTimeline timeline;
    VsyncPeriodChangeConstraints constraints;

    constraints.seamlessRequired = false;
    constraints.desiredTimeNanos = systemTime();

    for (VtsDisplay& display : mDisplays) {
        int32_t invalidConfigId = GetInvalidConfigId();
        const auto error =
                setActiveConfigWithConstraints(display, invalidConfigId, constraints, &timeline);
        EXPECT_FALSE(error.isOk());
        EXPECT_EQ(IComposerClient::EX_BAD_CONFIG, error.getServiceSpecificError());
    }
}

TEST_P(GraphicsComposerAidlTest, setAutoLowLatencyModeBadDisplay) {
    EXPECT_EQ(IComposerClient::EX_BAD_DISPLAY,
              mComposerClient->setAutoLowLatencyMode(mInvalidDisplayId, true)
                      .getServiceSpecificError());
    EXPECT_EQ(IComposerClient::EX_BAD_DISPLAY,
              mComposerClient->setAutoLowLatencyMode(mInvalidDisplayId, false)
                      .getServiceSpecificError());
}

TEST_P(GraphicsComposerAidlTest, setAutoLowLatencyMode) {
    for (const auto& display : mDisplays) {
        std::vector<DisplayCapability> capabilities;
        const auto error = mComposerClient->getDisplayCapabilities(display.get(), &capabilities);
        EXPECT_TRUE(error.isOk());

        const bool allmSupport =
                std::find(capabilities.begin(), capabilities.end(),
                          DisplayCapability::AUTO_LOW_LATENCY_MODE) != capabilities.end();

        if (!allmSupport) {
            const auto errorIsOn = mComposerClient->setAutoLowLatencyMode(display.get(), true);
            EXPECT_FALSE(errorIsOn.isOk());
            EXPECT_EQ(IComposerClient::EX_UNSUPPORTED, errorIsOn.getServiceSpecificError());
            const auto errorIsOff = mComposerClient->setAutoLowLatencyMode(display.get(), false);
            EXPECT_FALSE(errorIsOff.isOk());
            EXPECT_EQ(IComposerClient::EX_UNSUPPORTED, errorIsOff.getServiceSpecificError());
            GTEST_SUCCEED() << "Auto Low Latency Mode is not supported on display "
                            << std::to_string(display.get()) << ", skipping test";
            return;
        }

        EXPECT_TRUE(mComposerClient->setAutoLowLatencyMode(display.get(), true).isOk());
        EXPECT_TRUE(mComposerClient->setAutoLowLatencyMode(display.get(), false).isOk());
    }
}

TEST_P(GraphicsComposerAidlTest, getSupportedContentTypesBadDisplay) {
    std::vector<ContentType> supportedContentTypes;
    const auto error =
            mComposerClient->getSupportedContentTypes(mInvalidDisplayId, &supportedContentTypes);
    EXPECT_FALSE(error.isOk());
    EXPECT_EQ(IComposerClient::EX_BAD_DISPLAY, error.getServiceSpecificError());
}

TEST_P(GraphicsComposerAidlTest, getSupportedContentTypes) {
    std::vector<ContentType> supportedContentTypes;
    for (const auto& display : mDisplays) {
        supportedContentTypes.clear();
        const auto error =
                mComposerClient->getSupportedContentTypes(display.get(), &supportedContentTypes);

        ASSERT_TRUE(error.isOk());

        const bool noneSupported =
                std::find(supportedContentTypes.begin(), supportedContentTypes.end(),
                          ContentType::NONE) != supportedContentTypes.end();
        EXPECT_FALSE(noneSupported);
    }
}

TEST_P(GraphicsComposerAidlTest, setContentTypeNoneAlwaysAccepted) {
    for (const auto& display : mDisplays) {
        const auto error = mComposerClient->setContentType(display.get(), ContentType::NONE);
        EXPECT_TRUE(error.isOk());
    }
}

TEST_P(GraphicsComposerAidlTest, setContentTypeBadDisplay) {
    constexpr ContentType types[] = {ContentType::NONE, ContentType::GRAPHICS, ContentType::PHOTO,
                                     ContentType::CINEMA, ContentType::GAME};
    for (const auto& type : types) {
        auto const error = mComposerClient->setContentType(mInvalidDisplayId, type);

        EXPECT_FALSE(error.isOk());
        EXPECT_EQ(IComposerClient::EX_BAD_DISPLAY, error.getServiceSpecificError());
    }
}

TEST_P(GraphicsComposerAidlTest, setGraphicsContentType) {
    Test_setContentType(ContentType::GRAPHICS, "GRAPHICS");
}

TEST_P(GraphicsComposerAidlTest, setPhotoContentType) {
    Test_setContentType(ContentType::PHOTO, "PHOTO");
}

TEST_P(GraphicsComposerAidlTest, setCinemaContentType) {
    Test_setContentType(ContentType::CINEMA, "CINEMA");
}

TEST_P(GraphicsComposerAidlTest, setGameContentType) {
    Test_setContentType(ContentType::GAME, "GAME");
}

TEST_P(GraphicsComposerAidlTest, getLayerGenericMetadataKeys) {
    std::vector<LayerGenericMetadataKey> keys;
    EXPECT_TRUE(mComposerClient->getLayerGenericMetadataKeys(&keys).isOk());

    std::regex reverseDomainName("^[a-zA-Z-]{2,}(\\.[a-zA-Z0-9-]+)+$");
    std::unordered_set<std::string> uniqueNames;
    for (const auto& key : keys) {
        std::string name(key.name);

        // Keys must not start with 'android' or 'com.android'
        EXPECT_FALSE(name.find("android") == 0);
        EXPECT_FALSE(name.find("com.android") == 0);

        // Keys must be in reverse domain name format
        EXPECT_TRUE(std::regex_match(name, reverseDomainName));

        // Keys must be unique within this list
        const auto& [iter, inserted] = uniqueNames.insert(name);
        EXPECT_TRUE(inserted);
    }
}

TEST_P(GraphicsComposerAidlTest, CreateVirtualDisplay) {
    int32_t maxVirtualDisplayCount;
    EXPECT_TRUE(mComposerClient->getMaxVirtualDisplayCount(&maxVirtualDisplayCount).isOk());
    if (maxVirtualDisplayCount == 0) {
        GTEST_SUCCEED() << "no virtual display support";
        return;
    }

    VirtualDisplay virtualDisplay;

    EXPECT_TRUE(mComposerClient
                        ->createVirtualDisplay(64, 64, common::PixelFormat::IMPLEMENTATION_DEFINED,
                                               kBufferSlotCount, &virtualDisplay)
                        .isOk());

    ASSERT_TRUE(mDisplayResources.insert({virtualDisplay.display, DisplayResource(true)}).second)
            << "duplicated virtual display id " << virtualDisplay.display;

    EXPECT_TRUE(mComposerClient->destroyVirtualDisplay(virtualDisplay.display).isOk());
}

TEST_P(GraphicsComposerAidlTest, SetPowerMode) {
    std::vector<PowerMode> modes;
    modes.push_back(PowerMode::OFF);
    modes.push_back(PowerMode::ON_SUSPEND);
    modes.push_back(PowerMode::ON);

    for (auto mode : modes) {
        EXPECT_TRUE(mComposerClient->setPowerMode(mPrimaryDisplay, mode).isOk());
    }
}

TEST_P(GraphicsComposerAidlTest, SetPowerModeVariations) {
    std::vector<PowerMode> modes;

    modes.push_back(PowerMode::OFF);
    modes.push_back(PowerMode::OFF);

    for (auto mode : modes) {
        EXPECT_TRUE(mComposerClient->setPowerMode(mPrimaryDisplay, mode).isOk());
    }

    modes.clear();

    modes.push_back(PowerMode::ON);
    modes.push_back(PowerMode::ON);

    for (auto mode : modes) {
        EXPECT_TRUE(mComposerClient->setPowerMode(mPrimaryDisplay, mode).isOk());
    }

    modes.clear();

    modes.push_back(PowerMode::ON_SUSPEND);
    modes.push_back(PowerMode::ON_SUSPEND);

    for (auto mode : modes) {
        EXPECT_TRUE(mComposerClient->setPowerMode(mPrimaryDisplay, mode).isOk());
    }

    bool isDozeSupported = false;
    ASSERT_TRUE(mComposerClient->getDozeSupport(mPrimaryDisplay, &isDozeSupported).isOk());
    if (isDozeSupported) {
        modes.clear();

        modes.push_back(PowerMode::DOZE);
        modes.push_back(PowerMode::DOZE);

        for (auto mode : modes) {
            EXPECT_TRUE(mComposerClient->setPowerMode(mPrimaryDisplay, mode).isOk());
        }

        modes.clear();

        modes.push_back(PowerMode::DOZE_SUSPEND);
        modes.push_back(PowerMode::DOZE_SUSPEND);

        for (auto mode : modes) {
            EXPECT_TRUE(mComposerClient->setPowerMode(mPrimaryDisplay, mode).isOk());
        }
    }
}

TEST_P(GraphicsComposerAidlTest, SetPowerModeBadDisplay) {
    const auto error = mComposerClient->setPowerMode(mInvalidDisplayId, PowerMode::ON);

    EXPECT_FALSE(error.isOk());
    ASSERT_EQ(IComposerClient::EX_BAD_DISPLAY, error.getServiceSpecificError());
}

TEST_P(GraphicsComposerAidlTest, SetPowerModeBadParameter) {
    const auto error = mComposerClient->setPowerMode(mPrimaryDisplay, static_cast<PowerMode>(-1));

    EXPECT_FALSE(error.isOk());
    ASSERT_EQ(IComposerClient::EX_BAD_PARAMETER, error.getServiceSpecificError());
}

TEST_P(GraphicsComposerAidlTest, SetPowerModeUnsupported) {
    bool isDozeSupported = false;
    EXPECT_TRUE(mComposerClient->getDozeSupport(mPrimaryDisplay, &isDozeSupported).isOk());
    if (!isDozeSupported) {
        auto error = mComposerClient->setPowerMode(mPrimaryDisplay, PowerMode::DOZE);
        EXPECT_FALSE(error.isOk());
        EXPECT_EQ(IComposerClient::EX_UNSUPPORTED, error.getServiceSpecificError());

        error = mComposerClient->setPowerMode(mPrimaryDisplay, PowerMode::DOZE_SUSPEND);
        EXPECT_FALSE(error.isOk());
        EXPECT_EQ(IComposerClient::EX_UNSUPPORTED, error.getServiceSpecificError());
    }
}

TEST_P(GraphicsComposerAidlTest, GetDataspaceSaturationMatrix) {
    std::vector<float> matrix;
    EXPECT_TRUE(
            mComposerClient->getDataspaceSaturationMatrix(common::Dataspace::SRGB_LINEAR, &matrix)
                    .isOk());

    // the last row is known
    ASSERT_EQ(0.0f, matrix[12]);
    ASSERT_EQ(0.0f, matrix[13]);
    ASSERT_EQ(0.0f, matrix[14]);
    ASSERT_EQ(1.0f, matrix[15]);
}

TEST_P(GraphicsComposerAidlTest, GetDataspaceSaturationMatrixBadParameter) {
    std::vector<float> matrix;
    const auto error =
            mComposerClient->getDataspaceSaturationMatrix(common::Dataspace::UNKNOWN, &matrix);

    EXPECT_FALSE(error.isOk());
    EXPECT_EQ(IComposerClient::EX_BAD_PARAMETER, error.getServiceSpecificError());
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
