/*
 * Copyright (C) 2016 The Android Open Source Project
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

#define LOG_TAG "graphics_composer_hidl_hal_test"

#include <IComposerCommandBuffer.h>
#include <android-base/logging.h>
#include <android/hardware/graphics/allocator/2.0/IAllocator.h>
#include <android/hardware/graphics/composer/2.1/IComposer.h>
#include <android/hardware/graphics/mapper/2.0/IMapper.h>

#include <VtsHalHidlTargetBaseTest.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <memory>
#include <mutex>
#include <unordered_set>
#include <vector>

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_1 {
namespace tests {
namespace {

using android::hardware::graphics::allocator::V2_0::Buffer;
using android::hardware::graphics::allocator::V2_0::BufferDescriptor;
using android::hardware::graphics::allocator::V2_0::ConsumerUsage;
using android::hardware::graphics::allocator::V2_0::IAllocator;
using android::hardware::graphics::allocator::V2_0::IAllocatorClient;
using android::hardware::graphics::allocator::V2_0::ProducerUsage;
using android::hardware::graphics::common::V1_0::ColorMode;
using android::hardware::graphics::common::V1_0::ColorTransform;
using android::hardware::graphics::common::V1_0::Dataspace;
using android::hardware::graphics::common::V1_0::PixelFormat;
using android::hardware::graphics::common::V1_0::Transform;
using android::hardware::graphics::mapper::V2_0::IMapper;
using GrallocError = android::hardware::graphics::allocator::V2_0::Error;

// IComposerCallback to be installed with IComposerClient::registerCallback.
class GraphicsComposerCallback : public IComposerCallback {
 public:
  void setVsyncAllowed(bool allowed) {
    std::lock_guard<std::mutex> lock(mMutex);
    mVsyncAllowed = allowed;
  }

  std::vector<Display> getDisplays() const {
    std::lock_guard<std::mutex> lock(mMutex);
    return std::vector<Display>(mDisplays.begin(), mDisplays.end());
  }

  int getInvalidHotplugCount() const {
    std::lock_guard<std::mutex> lock(mMutex);
    return mInvalidHotplugCount;
  }

  int getInvalidRefreshCount() const {
    std::lock_guard<std::mutex> lock(mMutex);
    return mInvalidRefreshCount;
  }

  int getInvalidVsyncCount() const {
    std::lock_guard<std::mutex> lock(mMutex);
    return mInvalidVsyncCount;
  }

 private:
  Return<void> onHotplug(Display display, Connection connection) override {
    std::lock_guard<std::mutex> lock(mMutex);

    if (connection == Connection::CONNECTED) {
      if (!mDisplays.insert(display).second) {
        mInvalidHotplugCount++;
      }
    } else if (connection == Connection::DISCONNECTED) {
      if (!mDisplays.erase(display)) {
        mInvalidHotplugCount++;
      }
    }

    return Void();
  }

  Return<void> onRefresh(Display display) override {
    std::lock_guard<std::mutex> lock(mMutex);

    if (mDisplays.count(display) == 0) {
      mInvalidRefreshCount++;
    }

    return Void();
  }

  Return<void> onVsync(Display display, int64_t) override {
    std::lock_guard<std::mutex> lock(mMutex);

    if (!mVsyncAllowed || mDisplays.count(display) == 0) {
      mInvalidVsyncCount++;
    }

    return Void();
  }

  mutable std::mutex mMutex;
  // the set of all currently connected displays
  std::unordered_set<Display> mDisplays;
  // true only when vsync is enabled
  bool mVsyncAllowed = false;

  // track invalid callbacks
  int mInvalidHotplugCount = 0;
  int mInvalidRefreshCount = 0;
  int mInvalidVsyncCount = 0;
};

class GraphicsComposerHidlTest : public ::testing::VtsHalHidlTargetBaseTest {
 protected:
  void SetUp() override {
    mComposer = ::testing::VtsHalHidlTargetBaseTest::getService<IComposer>();
    ASSERT_NE(nullptr, mComposer.get());

    mComposerClient = createClient();
    ASSERT_NE(nullptr, mComposerClient.get());

    initCapabilities();

    mComposerCallback = new GraphicsComposerCallback;
    mComposerClient->registerCallback(mComposerCallback);

    // assume the first display is primary and is never removed
    mPrimaryDisplay = waitForFirstDisplay();
  }

  void TearDown() override {
    if (mComposerCallback != nullptr) {
      EXPECT_EQ(0, mComposerCallback->getInvalidHotplugCount());
      EXPECT_EQ(0, mComposerCallback->getInvalidRefreshCount());
      EXPECT_EQ(0, mComposerCallback->getInvalidVsyncCount());
    }
  }

  /**
   * Initialize the set of supported capabilities.
   */
  void initCapabilities() {
    mComposer->getCapabilities([this](const auto& capabilities) {
      std::vector<IComposer::Capability> caps = capabilities;
      mCapabilities.insert(caps.cbegin(), caps.cend());
    });
  }

  /**
   * Test whether a capability is supported.
   */
  bool hasCapability(IComposer::Capability capability) const {
    return (mCapabilities.count(capability) > 0);
  }

  IComposerClient::DisplayType getDisplayType(Display display) {
    IComposerClient::DisplayType type = IComposerClient::DisplayType::INVALID;
    mComposerClient->getDisplayType(
        display, [&](const auto& tmpError, const auto& tmpType) {
          ASSERT_EQ(Error::NONE, tmpError);
          type = tmpType;
        });
    return type;
  }

  Error createVirtualDisplay(Display* outDisplay) {
    auto ret_count = mComposerClient->getMaxVirtualDisplayCount();
    if (ret_count == 0) {
      return Error::UNSUPPORTED;
    }

    Error err = Error::NO_RESOURCES;
    Display display;
    mComposerClient->createVirtualDisplay(
        64, 64, PixelFormat::IMPLEMENTATION_DEFINED, kBufferSlotCount,
        [&](const auto& tmpError, const auto& tmpDisplay, const auto&) {
          err = tmpError;
          display = tmpDisplay;
        });

    *outDisplay = display;
    return err;
  }

  void destroyVirtualDisplay(Display display) {
    auto ret = mComposerClient->destroyVirtualDisplay(display);
    ASSERT_EQ(Error::NONE, static_cast<Error>(ret));
  }

  Error createLayer(Layer* outLayer) {
    Error err = Error::NO_RESOURCES;
    Layer layer;
    mComposerClient->createLayer(
        mPrimaryDisplay, kBufferSlotCount,
        [&](const auto& tmpError, const auto& tmpLayer) {
          err = tmpError;
          layer = tmpLayer;
        });

    *outLayer = layer;
    return err;
  }

  void destroyLayer(Layer layer) {
    auto ret = mComposerClient->destroyLayer(mPrimaryDisplay, layer);
    ASSERT_EQ(Error::NONE, static_cast<Error>(ret));
  }

  int32_t getDisplayAttribute(Config config,
                              IComposerClient::Attribute attribute) {
    int32_t value = -1;
    mComposerClient->getDisplayAttribute(
        mPrimaryDisplay, config, attribute,
        [&](const auto& tmpError, const auto& tmpValue) {
          ASSERT_EQ(Error::NONE, tmpError);
          value = tmpValue;
        });
    return value;
  }

  std::vector<Config> getDisplayConfigs() {
    std::vector<Config> configs;
    mComposerClient->getDisplayConfigs(
        mPrimaryDisplay, [&](const auto& tmpError, const auto& tmpConfigs) {
          ASSERT_EQ(Error::NONE, tmpError);

          configs = tmpConfigs;
          ASSERT_FALSE(configs.empty());
        });

    return configs;
  }

  std::vector<ColorMode> getColorModes() {
    std::vector<ColorMode> modes;
    mComposerClient->getColorModes(
        mPrimaryDisplay, [&](const auto& tmpError, const auto& tmpModes) {
          ASSERT_EQ(Error::NONE, tmpError);

          modes = tmpModes;
          ASSERT_NE(modes.end(),
                    std::find(modes.begin(), modes.end(), ColorMode::NATIVE));
        });

    return modes;
  }

  std::vector<IComposerClient::PowerMode> getPowerModes() {
    std::vector<IComposerClient::PowerMode> modes;
    modes.push_back(IComposerClient::PowerMode::OFF);

    mComposerClient->getDozeSupport(
        mPrimaryDisplay, [&](const auto& tmpError, const auto& tmpSupport) {
          ASSERT_EQ(Error::NONE, tmpError);
          if (tmpSupport) {
            modes.push_back(IComposerClient::PowerMode::DOZE);
            modes.push_back(IComposerClient::PowerMode::DOZE_SUSPEND);
          }
        });

    // push ON last
    modes.push_back(IComposerClient::PowerMode::ON);

    return modes;
  }

  void setActiveConfig(Config config) {
    auto ret = mComposerClient->setActiveConfig(mPrimaryDisplay, config);
    ASSERT_EQ(Error::NONE, static_cast<Error>(ret));
  }

  void setColorMode(ColorMode mode) {
    auto ret = mComposerClient->setColorMode(mPrimaryDisplay, mode);
    ASSERT_EQ(Error::NONE, static_cast<Error>(ret));
  }

  void setPowerMode(IComposerClient::PowerMode mode) {
    auto ret = mComposerClient->setPowerMode(mPrimaryDisplay, mode);
    ASSERT_EQ(Error::NONE, static_cast<Error>(ret));
  }

  void setVsyncEnabled(bool enable) {
    auto ret = mComposerClient->setVsyncEnabled(
        mPrimaryDisplay,
        enable ? IComposerClient::Vsync::ENABLE
               : IComposerClient::Vsync::DISABLE);
    ASSERT_EQ(Error::NONE, static_cast<Error>(ret));
  }
  // use the slot count usually set by SF
  static constexpr uint32_t kBufferSlotCount = 64;

  sp<IComposer> mComposer;
  sp<IComposerClient> mComposerClient;
  sp<GraphicsComposerCallback> mComposerCallback;
  // the first display and is assumed never to be removed
  Display mPrimaryDisplay;

 private:
  sp<IComposerClient> createClient() {
    sp<IComposerClient> client;
    mComposer->createClient([&](const auto& tmpError, const auto& tmpClient) {
      if (tmpError == Error::NONE) {
        client = tmpClient;
      }
    });

    return client;
  }

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

  // the set of all supported capabilities
  std::unordered_set<IComposer::Capability> mCapabilities;
};

/**
 * Test IComposer::getCapabilities.
 *
 * Test that IComposer::getCapabilities returns no invalid capabilities.
 */
TEST_F(GraphicsComposerHidlTest, GetCapabilities) {
  mComposer->getCapabilities([](const auto& tmpCapabilities) {
    std::vector<IComposer::Capability> capabilities = tmpCapabilities;
    ASSERT_EQ(capabilities.end(),
              std::find(capabilities.begin(), capabilities.end(),
                        IComposer::Capability::INVALID));
  });
}

/**
 * Test IComposer::dumpDebugInfo.
 */
TEST_F(GraphicsComposerHidlTest, DumpDebugInfo) {
  mComposer->dumpDebugInfo([](const auto&) {
    // nothing to do
  });
}

/**
 * Test IComposer::createClient.
 *
 * Test that IComposerClient is a singleton.
 */
TEST_F(GraphicsComposerHidlTest, CreateClientSingleton) {
  mComposer->createClient([&](const auto& tmpError, const auto&) {
    EXPECT_EQ(Error::NO_RESOURCES, tmpError);
  });
}

/**
 * Test IComposerClient::createVirtualDisplay and
 * IComposerClient::destroyVirtualDisplay.
 *
 * Test that virtual displays can be created and has the correct display type.
 */
TEST_F(GraphicsComposerHidlTest, CreateVirtualDisplay) {
  Display display;
  Error err = createVirtualDisplay(&display);
  if (err == Error::UNSUPPORTED) {
    GTEST_SUCCEED() << "no virtual display support";
    return;
  }
  ASSERT_EQ(Error::NONE, err);

  // test display type
  IComposerClient::DisplayType type = getDisplayType(display);
  EXPECT_EQ(IComposerClient::DisplayType::VIRTUAL, type);

  destroyVirtualDisplay(display);
}

/**
 * Test IComposerClient::createLayer and IComposerClient::destroyLayer.
 *
 * Test that layers can be created and destroyed.
 */
TEST_F(GraphicsComposerHidlTest, CreateLayer) {
  Layer layer;
  Error err = createLayer(&layer);
  ASSERT_EQ(Error::NONE, err);

  destroyLayer(layer);
}

/**
 * Test IComposerClient::getDisplayName.
 */
TEST_F(GraphicsComposerHidlTest, GetDisplayName) {
  mComposerClient->getDisplayName(mPrimaryDisplay,
                                  [&](const auto& tmpError, const auto&) {
                                    ASSERT_EQ(Error::NONE, tmpError);
                                  });
}

/**
 * Test IComposerClient::getDisplayType.
 *
 * Test that IComposerClient::getDisplayType returns the correct display type
 * for the primary display.
 */
TEST_F(GraphicsComposerHidlTest, GetDisplayType) {
  IComposerClient::DisplayType type = getDisplayType(mPrimaryDisplay);
  EXPECT_EQ(IComposerClient::DisplayType::PHYSICAL, type);
}

/**
 * Test IComposerClient::getClientTargetSupport.
 *
 * Test that IComposerClient::getClientTargetSupport returns true for the
 * required client targets.
 */
TEST_F(GraphicsComposerHidlTest, GetClientTargetSupport) {
  std::vector<Config> configs = getDisplayConfigs();
  for (auto config : configs) {
    int32_t width =
        getDisplayAttribute(config, IComposerClient::Attribute::WIDTH);
    int32_t height =
        getDisplayAttribute(config, IComposerClient::Attribute::HEIGHT);
    ASSERT_LT(0, width);
    ASSERT_LT(0, height);

    setActiveConfig(config);

    auto ret = mComposerClient->getClientTargetSupport(
        mPrimaryDisplay, width, height, PixelFormat::RGBA_8888,
        Dataspace::UNKNOWN);
    ASSERT_EQ(Error::NONE, static_cast<Error>(ret));
  }
}

/**
 * Test IComposerClient::getDisplayAttribute.
 *
 * Test that IComposerClient::getDisplayAttribute succeeds for the required
 * formats, and succeeds or fails correctly for optional attributes.
 */
TEST_F(GraphicsComposerHidlTest, GetDisplayAttribute) {
  std::vector<Config> configs = getDisplayConfigs();
  for (auto config : configs) {
    const std::array<IComposerClient::Attribute, 3> requiredAttributes = {{
        IComposerClient::Attribute::WIDTH, IComposerClient::Attribute::HEIGHT,
        IComposerClient::Attribute::VSYNC_PERIOD,
    }};
    for (auto attribute : requiredAttributes) {
      getDisplayAttribute(config, attribute);
    }

    const std::array<IComposerClient::Attribute, 2> optionalAttributes = {{
        IComposerClient::Attribute::DPI_X, IComposerClient::Attribute::DPI_Y,
    }};
    for (auto attribute : optionalAttributes) {
      mComposerClient->getDisplayAttribute(
          mPrimaryDisplay, config, attribute,
          [&](const auto& tmpError, const auto&) {
            EXPECT_TRUE(tmpError == Error::NONE ||
                        tmpError == Error::UNSUPPORTED);
          });
    }
  }
}

/**
 * Test IComposerClient::getHdrCapabilities.
 */
TEST_F(GraphicsComposerHidlTest, GetHdrCapabilities) {
  mComposerClient->getHdrCapabilities(
      mPrimaryDisplay,
      [&](const auto& tmpError, const auto&, const auto&, const auto&,
          const auto&) { ASSERT_EQ(Error::NONE, tmpError); });
}

/**
 * Test IComposerClient::setClientTargetSlotCount.
 */
TEST_F(GraphicsComposerHidlTest, SetClientTargetSlotCount) {
  auto ret = mComposerClient->setClientTargetSlotCount(mPrimaryDisplay,
                                                       kBufferSlotCount);
  ASSERT_EQ(Error::NONE, static_cast<Error>(ret));
}

/**
 * Test IComposerClient::setActiveConfig.
 *
 * Test that IComposerClient::setActiveConfig succeeds for all display
 * configs.
 */
TEST_F(GraphicsComposerHidlTest, SetActiveConfig) {
  std::vector<Config> configs = getDisplayConfigs();
  for (auto config : configs) {
    setActiveConfig(config);

    mComposerClient->getActiveConfig(
        mPrimaryDisplay, [&](const auto& tmpError, const auto& tmpConfig) {
          EXPECT_EQ(Error::NONE, tmpError);
          EXPECT_EQ(config, tmpConfig);
        });
  }
}

/**
 * Test IComposerClient::setColorMode.
 *
 * Test that IComposerClient::setColorMode succeeds for all color modes.
 */
TEST_F(GraphicsComposerHidlTest, SetColorMode) {
  std::vector<ColorMode> modes = getColorModes();
  for (auto mode : modes) {
    setColorMode(mode);
  }
}

/**
 * Test IComposerClient::setPowerMode.
 *
 * Test that IComposerClient::setPowerMode succeeds for all power modes.
 */
TEST_F(GraphicsComposerHidlTest, SetPowerMode) {
  std::vector<IComposerClient::PowerMode> modes = getPowerModes();
  for (auto mode : modes) {
    setPowerMode(mode);
  }
}

/**
 * Test IComposerClient::setVsyncEnabled.
 *
 * Test that IComposerClient::setVsyncEnabled succeeds and there is no
 * spurious vsync events.
 */
TEST_F(GraphicsComposerHidlTest, SetVsyncEnabled) {
  mComposerCallback->setVsyncAllowed(true);

  setVsyncEnabled(true);
  usleep(60 * 1000);
  setVsyncEnabled(false);

  mComposerCallback->setVsyncAllowed(false);
}

// Tests for IComposerClient::Command.
class GraphicsComposerHidlCommandTest : public GraphicsComposerHidlTest {
 protected:
  void SetUp() override {
    ASSERT_NO_FATAL_FAILURE(GraphicsComposerHidlTest::SetUp());
    ASSERT_NO_FATAL_FAILURE(SetUpGralloc());

    mWriter = std::make_unique<CommandWriterBase>(1024);
    mReader = std::make_unique<CommandReader>();
  }

  void TearDown() override {
    ASSERT_NO_FATAL_FAILURE(GraphicsComposerHidlTest::TearDown());
  }

  const native_handle_t* cloneBuffer(const native_handle_t* handle) {
    auto clone = native_handle_clone(handle);
    if (!clone) {
      return nullptr;
    }

    GrallocError err = mMapper->retain(clone);
    if (err != GrallocError::NONE) {
      native_handle_close(clone);
      native_handle_delete(const_cast<native_handle_t*>(clone));
      return nullptr;
    }

    return clone;
  }

  const native_handle_t* allocate(
      const IAllocatorClient::BufferDescriptorInfo& info) {
    // create descriptor
    GrallocError err = GrallocError::NO_RESOURCES;
    BufferDescriptor descriptor;
    mAllocatorClient->createDescriptor(
        info, [&](const auto& tmpError, const auto& tmpDescriptor) {
          err = tmpError;
          descriptor = tmpDescriptor;
        });
    if (err != GrallocError::NONE) {
      return nullptr;
    }

    // allocate buffer
    hidl_vec<BufferDescriptor> descriptors;
    hidl_vec<Buffer> buffers;
    descriptors.setToExternal(&descriptor, 1);
    err = GrallocError::NO_RESOURCES;
    mAllocatorClient->allocate(
        descriptors, [&](const auto& tmpError, const auto& tmpBuffers) {
          err = tmpError;
          buffers = tmpBuffers;
        });
    if ((err != GrallocError::NONE && err != GrallocError::NOT_SHARED) ||
        buffers.size() != 1) {
      mAllocatorClient->destroyDescriptor(descriptors[0]);
      return nullptr;
    }

    // export handle
    err = GrallocError::NO_RESOURCES;
    const native_handle_t* handle = nullptr;
    mAllocatorClient->exportHandle(
        descriptors[0], buffers[0],
        [&](const auto& tmpError, const auto& tmpHandle) {
          err = tmpError;
          if (err != GrallocError::NONE) {
            return;
          }

          handle = cloneBuffer(tmpHandle.getNativeHandle());
          if (!handle) {
            err = GrallocError::NO_RESOURCES;
            return;
          }
        });

    mAllocatorClient->destroyDescriptor(descriptors[0]);
    mAllocatorClient->free(buffers[0]);

    if (err != GrallocError::NONE) {
      return nullptr;
    }

    return handle;
  }

  const native_handle_t* allocate() {
    IAllocatorClient::BufferDescriptorInfo info{};
    info.width = 64;
    info.height = 64;
    info.layerCount = 1;
    info.format = PixelFormat::RGBA_8888;
    info.producerUsageMask = static_cast<uint64_t>(ProducerUsage::CPU_WRITE);
    info.consumerUsageMask = static_cast<uint64_t>(ConsumerUsage::CPU_READ);

    return allocate(info);
  }

  void free(const native_handle_t* handle) {
    auto ret = mMapper->release(handle);
    ASSERT_EQ(GrallocError::NONE, static_cast<GrallocError>(ret));
  }

  void execute() {
    bool queueChanged = false;
    uint32_t commandLength = 0;
    hidl_vec<hidl_handle> commandHandles;
    ASSERT_TRUE(
        mWriter->writeQueue(&queueChanged, &commandLength, &commandHandles));

    if (queueChanged) {
      auto ret =
          mComposerClient->setInputCommandQueue(*mWriter->getMQDescriptor());
      ASSERT_EQ(Error::NONE, static_cast<Error>(ret));
      return;
    }

    mComposerClient->executeCommands(
        commandLength, commandHandles,
        [&](const auto& tmpError, const auto& tmpOutQueueChanged,
            const auto& tmpOutLength, const auto& tmpOutHandles) {
          ASSERT_EQ(Error::NONE, tmpError);

          if (tmpOutQueueChanged) {
            mComposerClient->getOutputCommandQueue(
                [&](const auto& tmpError, const auto& tmpDescriptor) {
                  ASSERT_EQ(Error::NONE, tmpError);
                  mReader->setMQDescriptor(tmpDescriptor);
                });
          }

          ASSERT_TRUE(mReader->readQueue(tmpOutLength, tmpOutHandles));
          mReader->parse();
        });
  }

  // A command parser that checks that no error nor unexpected commands are
  // returned.
  class CommandReader : public CommandReaderBase {
   public:
    // Parse all commands in the return command queue.  Call GTEST_FAIL() for
    // unexpected errors or commands.
    void parse() {
      while (!isEmpty()) {
        IComposerClient::Command command;
        uint16_t length;
        ASSERT_TRUE(beginCommand(&command, &length));

        switch (command) {
          case IComposerClient::Command::SET_ERROR: {
            ASSERT_EQ(2, length);
            auto loc = read();
            auto err = readSigned();
            GTEST_FAIL() << "unexpected error " << err << " at location "
                         << loc;
          } break;
          case IComposerClient::Command::SELECT_DISPLAY:
          case IComposerClient::Command::SET_CHANGED_COMPOSITION_TYPES:
          case IComposerClient::Command::SET_DISPLAY_REQUESTS:
          case IComposerClient::Command::SET_PRESENT_FENCE:
          case IComposerClient::Command::SET_RELEASE_FENCES:
            break;
          default:
            GTEST_FAIL() << "unexpected return command " << std::hex
                         << static_cast<int>(command);
            break;
        }

        endCommand();
      }
    }
  };

  std::unique_ptr<CommandWriterBase> mWriter;
  std::unique_ptr<CommandReader> mReader;

 private:
  void SetUpGralloc() {
    mAllocator = ::testing::VtsHalHidlTargetBaseTest::getService<IAllocator>();
    ASSERT_NE(nullptr, mAllocator.get());

    mAllocator->createClient([this](const auto& error, const auto& client) {
      if (error == GrallocError::NONE) {
        mAllocatorClient = client;
      }
    });
    ASSERT_NE(nullptr, mAllocatorClient.get());

    mMapper = ::testing::VtsHalHidlTargetBaseTest::getService<IMapper>();
    ASSERT_NE(nullptr, mMapper.get());
    ASSERT_FALSE(mMapper->isRemote());
  }

  sp<IAllocator> mAllocator;
  sp<IAllocatorClient> mAllocatorClient;
  sp<IMapper> mMapper;
};

/**
 * Test IComposerClient::Command::SET_COLOR_TRANSFORM.
 */
TEST_F(GraphicsComposerHidlCommandTest, SET_COLOR_TRANSFORM) {
  const std::array<float, 16> identity = {{
      1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
      0.0f, 0.0f, 0.0f, 1.0f,
  }};

  mWriter->selectDisplay(mPrimaryDisplay);
  mWriter->setColorTransform(identity.data(), ColorTransform::IDENTITY);

  execute();
}

/**
 * Test IComposerClient::Command::SET_CLIENT_TARGET.
 */
TEST_F(GraphicsComposerHidlCommandTest, SET_CLIENT_TARGET) {
  mComposerClient->setClientTargetSlotCount(mPrimaryDisplay, kBufferSlotCount);

  mWriter->selectDisplay(mPrimaryDisplay);
  mWriter->setClientTarget(0, nullptr, -1, Dataspace::UNKNOWN,
                           std::vector<IComposerClient::Rect>());

  execute();
}

/**
 * Test IComposerClient::Command::SET_OUTPUT_BUFFER.
 */
TEST_F(GraphicsComposerHidlCommandTest, SET_OUTPUT_BUFFER) {
  auto handle = allocate();
  ASSERT_NE(nullptr, handle);

  Display display;
  Error err = createVirtualDisplay(&display);
  if (err == Error::UNSUPPORTED) {
    GTEST_SUCCEED() << "no virtual display support";
    return;
  }
  ASSERT_EQ(Error::NONE, err);

  mWriter->selectDisplay(display);
  mWriter->setOutputBuffer(0, handle, -1);

  destroyVirtualDisplay(display);
  free(handle);
}

/**
 * Test IComposerClient::Command::VALIDATE_DISPLAY.
 */
TEST_F(GraphicsComposerHidlCommandTest, VALIDATE_DISPLAY) {
  mWriter->selectDisplay(mPrimaryDisplay);
  mWriter->validateDisplay();
  execute();
}

/**
 * Test IComposerClient::Command::ACCEPT_DISPLAY_CHANGES.
 */
TEST_F(GraphicsComposerHidlCommandTest, ACCEPT_DISPLAY_CHANGES) {
  mWriter->selectDisplay(mPrimaryDisplay);
  mWriter->validateDisplay();
  mWriter->acceptDisplayChanges();
  execute();
}

/**
 * Test IComposerClient::Command::PRESENT_DISPLAY.
 */
TEST_F(GraphicsComposerHidlCommandTest, PRESENT_DISPLAY) {
  mWriter->selectDisplay(mPrimaryDisplay);
  mWriter->validateDisplay();
  mWriter->presentDisplay();
  execute();
}

/**
 * Test IComposerClient::Command::SET_LAYER_CURSOR_POSITION.
 */
TEST_F(GraphicsComposerHidlCommandTest, SET_LAYER_CURSOR_POSITION) {
  Layer layer;
  Error err = createLayer(&layer);
  ASSERT_EQ(Error::NONE, err);

  mWriter->selectDisplay(mPrimaryDisplay);
  mWriter->selectLayer(layer);
  mWriter->setLayerCursorPosition(1, 1);
  mWriter->setLayerCursorPosition(0, 0);
  execute();

  destroyLayer(layer);
}

/**
 * Test IComposerClient::Command::SET_LAYER_BUFFER.
 */
TEST_F(GraphicsComposerHidlCommandTest, SET_LAYER_BUFFER) {
  auto handle = allocate();
  ASSERT_NE(nullptr, handle);

  Layer layer;
  Error err = createLayer(&layer);
  ASSERT_EQ(Error::NONE, err);

  mWriter->selectDisplay(mPrimaryDisplay);
  mWriter->selectLayer(layer);
  mWriter->setLayerBuffer(0, handle, -1);
  execute();

  destroyLayer(layer);
  free(handle);
}

/**
 * Test IComposerClient::Command::SET_LAYER_SURFACE_DAMAGE.
 */
TEST_F(GraphicsComposerHidlCommandTest, SET_LAYER_SURFACE_DAMAGE) {
  Layer layer;
  Error err = createLayer(&layer);
  ASSERT_EQ(Error::NONE, err);

  IComposerClient::Rect empty{0, 0, 0, 0};
  IComposerClient::Rect unit{0, 0, 1, 1};

  mWriter->selectDisplay(mPrimaryDisplay);
  mWriter->selectLayer(layer);
  mWriter->setLayerSurfaceDamage(std::vector<IComposerClient::Rect>(1, empty));
  mWriter->setLayerSurfaceDamage(std::vector<IComposerClient::Rect>(1, unit));
  mWriter->setLayerSurfaceDamage(std::vector<IComposerClient::Rect>());
  execute();

  destroyLayer(layer);
}

/**
 * Test IComposerClient::Command::SET_LAYER_BLEND_MODE.
 */
TEST_F(GraphicsComposerHidlCommandTest, SET_LAYER_BLEND_MODE) {
  Layer layer;
  Error err = createLayer(&layer);
  ASSERT_EQ(Error::NONE, err);

  mWriter->selectDisplay(mPrimaryDisplay);
  mWriter->selectLayer(layer);
  mWriter->setLayerBlendMode(IComposerClient::BlendMode::NONE);
  mWriter->setLayerBlendMode(IComposerClient::BlendMode::PREMULTIPLIED);
  mWriter->setLayerBlendMode(IComposerClient::BlendMode::COVERAGE);
  execute();

  destroyLayer(layer);
}

/**
 * Test IComposerClient::Command::SET_LAYER_COLOR.
 */
TEST_F(GraphicsComposerHidlCommandTest, SET_LAYER_COLOR) {
  Layer layer;
  Error err = createLayer(&layer);
  ASSERT_EQ(Error::NONE, err);

  mWriter->selectDisplay(mPrimaryDisplay);
  mWriter->selectLayer(layer);
  mWriter->setLayerColor(IComposerClient::Color{0xff, 0xff, 0xff, 0xff});
  mWriter->setLayerColor(IComposerClient::Color{0, 0, 0, 0});
  execute();

  destroyLayer(layer);
}

/**
 * Test IComposerClient::Command::SET_LAYER_COMPOSITION_TYPE.
 */
TEST_F(GraphicsComposerHidlCommandTest, SET_LAYER_COMPOSITION_TYPE) {
  Layer layer;
  Error err = createLayer(&layer);
  ASSERT_EQ(Error::NONE, err);

  mWriter->selectDisplay(mPrimaryDisplay);
  mWriter->selectLayer(layer);
  mWriter->setLayerCompositionType(IComposerClient::Composition::CLIENT);
  mWriter->setLayerCompositionType(IComposerClient::Composition::DEVICE);
  mWriter->setLayerCompositionType(IComposerClient::Composition::SOLID_COLOR);
  mWriter->setLayerCompositionType(IComposerClient::Composition::CURSOR);
  execute();

  destroyLayer(layer);
}

/**
 * Test IComposerClient::Command::SET_LAYER_DATASPACE.
 */
TEST_F(GraphicsComposerHidlCommandTest, SET_LAYER_DATASPACE) {
  Layer layer;
  Error err = createLayer(&layer);
  ASSERT_EQ(Error::NONE, err);

  mWriter->selectDisplay(mPrimaryDisplay);
  mWriter->selectLayer(layer);
  mWriter->setLayerDataspace(Dataspace::UNKNOWN);
  execute();

  destroyLayer(layer);
}

/**
 * Test IComposerClient::Command::SET_LAYER_DISPLAY_FRAME.
 */
TEST_F(GraphicsComposerHidlCommandTest, SET_LAYER_DISPLAY_FRAME) {
  Layer layer;
  Error err = createLayer(&layer);
  ASSERT_EQ(Error::NONE, err);

  mWriter->selectDisplay(mPrimaryDisplay);
  mWriter->selectLayer(layer);
  mWriter->setLayerDisplayFrame(IComposerClient::Rect{0, 0, 1, 1});
  execute();

  destroyLayer(layer);
}

/**
 * Test IComposerClient::Command::SET_LAYER_PLANE_ALPHA.
 */
TEST_F(GraphicsComposerHidlCommandTest, SET_LAYER_PLANE_ALPHA) {
  Layer layer;
  Error err = createLayer(&layer);
  ASSERT_EQ(Error::NONE, err);

  mWriter->selectDisplay(mPrimaryDisplay);
  mWriter->selectLayer(layer);
  mWriter->setLayerPlaneAlpha(0.0f);
  mWriter->setLayerPlaneAlpha(1.0f);
  execute();

  destroyLayer(layer);
}

/**
 * Test IComposerClient::Command::SET_LAYER_SIDEBAND_STREAM.
 */
TEST_F(GraphicsComposerHidlCommandTest, SET_LAYER_SIDEBAND_STREAM) {
  if (!hasCapability(IComposer::Capability::SIDEBAND_STREAM)) {
    GTEST_SUCCEED() << "no sideband stream support";
    return;
  }

  auto handle = allocate();
  ASSERT_NE(nullptr, handle);

  Layer layer;
  Error err = createLayer(&layer);
  ASSERT_EQ(Error::NONE, err);

  mWriter->selectDisplay(mPrimaryDisplay);
  mWriter->selectLayer(layer);
  mWriter->setLayerSidebandStream(handle);
  execute();

  destroyLayer(layer);
  free(handle);
}

/**
 * Test IComposerClient::Command::SET_LAYER_SOURCE_CROP.
 */
TEST_F(GraphicsComposerHidlCommandTest, SET_LAYER_SOURCE_CROP) {
  Layer layer;
  Error err = createLayer(&layer);
  ASSERT_EQ(Error::NONE, err);

  mWriter->selectDisplay(mPrimaryDisplay);
  mWriter->selectLayer(layer);
  mWriter->setLayerSourceCrop(IComposerClient::FRect{0.0f, 0.0f, 1.0f, 1.0f});
  execute();

  destroyLayer(layer);
}

/**
 * Test IComposerClient::Command::SET_LAYER_TRANSFORM.
 */
TEST_F(GraphicsComposerHidlCommandTest, SET_LAYER_TRANSFORM) {
  Layer layer;
  Error err = createLayer(&layer);
  ASSERT_EQ(Error::NONE, err);

  mWriter->selectDisplay(mPrimaryDisplay);
  mWriter->selectLayer(layer);
  mWriter->setLayerTransform(static_cast<Transform>(0));
  mWriter->setLayerTransform(Transform::FLIP_H);
  mWriter->setLayerTransform(Transform::FLIP_V);
  mWriter->setLayerTransform(Transform::ROT_90);
  mWriter->setLayerTransform(Transform::ROT_180);
  mWriter->setLayerTransform(Transform::ROT_270);
  mWriter->setLayerTransform(
      static_cast<Transform>(Transform::FLIP_H | Transform::ROT_90));
  mWriter->setLayerTransform(
      static_cast<Transform>(Transform::FLIP_V | Transform::ROT_90));
  execute();

  destroyLayer(layer);
}

/**
 * Test IComposerClient::Command::SET_LAYER_VISIBLE_REGION.
 */
TEST_F(GraphicsComposerHidlCommandTest, SET_LAYER_VISIBLE_REGION) {
  Layer layer;
  Error err = createLayer(&layer);
  ASSERT_EQ(Error::NONE, err);

  IComposerClient::Rect empty{0, 0, 0, 0};
  IComposerClient::Rect unit{0, 0, 1, 1};

  mWriter->selectDisplay(mPrimaryDisplay);
  mWriter->selectLayer(layer);
  mWriter->setLayerVisibleRegion(std::vector<IComposerClient::Rect>(1, empty));
  mWriter->setLayerVisibleRegion(std::vector<IComposerClient::Rect>(1, unit));
  mWriter->setLayerVisibleRegion(std::vector<IComposerClient::Rect>());
  execute();

  destroyLayer(layer);
}

/**
 * Test IComposerClient::Command::SET_LAYER_Z_ORDER.
 */
TEST_F(GraphicsComposerHidlCommandTest, SET_LAYER_Z_ORDER) {
  Layer layer;
  Error err = createLayer(&layer);
  ASSERT_EQ(Error::NONE, err);

  mWriter->selectDisplay(mPrimaryDisplay);
  mWriter->selectLayer(layer);
  mWriter->setLayerZOrder(10);
  mWriter->setLayerZOrder(0);
  execute();

  destroyLayer(layer);
}

}  // namespace anonymous
}  // namespace tests
}  // namespace V2_1
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);

  int status = RUN_ALL_TESTS();
  LOG(INFO) << "Test result = " << status;

  return status;
}
