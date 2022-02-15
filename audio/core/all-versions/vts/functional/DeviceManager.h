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

#include <unistd.h>

#include <map>

#include <android-base/logging.h>
#include <hwbinder/IPCThreadState.h>
#include <system/audio.h>

// clang-format off
#include PATH(android/hardware/audio/FILE_VERSION/IDevice.h)
#include PATH(android/hardware/audio/FILE_VERSION/IDevicesFactory.h)
#include PATH(android/hardware/audio/FILE_VERSION/IPrimaryDevice.h)
#include PATH(android/hardware/audio/CORE_TYPES_FILE_VERSION/types.h)
#include PATH(android/hardware/audio/common/COMMON_TYPES_FILE_VERSION/types.h)
// clang-format on

#include "utility/ReturnIn.h"

using ::android::sp;
using ::android::hardware::audio::CORE_TYPES_CPP_VERSION::Result;
using namespace ::android::hardware::audio::common::COMMON_TYPES_CPP_VERSION;
using namespace ::android::hardware::audio::common::test::utility;
using namespace ::android::hardware::audio::CPP_VERSION;

template <class Derived, class Key, class Interface>
class InterfaceManager {
  public:
    sp<Interface> getExisting(const Key& name) {
        auto existing = instances.find(name);
        return existing != instances.end() ? existing->second : sp<Interface>();
    }

    sp<Interface> get(const Key& name) {
        auto existing = instances.find(name);
        if (existing != instances.end()) return existing->second;
        auto [inserted, _] = instances.emplace(name, Derived::createInterfaceInstance(name));
        return inserted->second;
    }

    // The test must check that reset was successful. Reset failure means that the test code
    // is holding a strong reference to the device.
    bool reset(const Key& name, bool waitForDestruction) __attribute__((warn_unused_result)) {
        auto iter = instances.find(name);
        if (iter == instances.end()) return true;
        ::android::wp<Interface> weak = iter->second;
        instances.erase(iter);
        if (weak.promote() != nullptr) return false;
        if (waitForDestruction) {
            waitForInstanceDestruction();
        }
        return true;
    }

    static void waitForInstanceDestruction() {
        // FIXME: there is no way to know when the remote IDevice is being destroyed
        //        Binder does not support testing if an object is alive, thus
        //        wait for 100ms to let the binder destruction propagates and
        //        the remote device has the time to be destroyed.
        //        flushCommand makes sure all local command are sent, thus should reduce
        //        the latency between local and remote destruction.
        ::android::hardware::IPCThreadState::self()->flushCommands();
        usleep(100 * 1000);
    }

  protected:
    std::map<Key, sp<Interface>> instances;
};

class DevicesFactoryManager
    : public InterfaceManager<DevicesFactoryManager, std::string, IDevicesFactory> {
  public:
    static DevicesFactoryManager& getInstance() {
        static DevicesFactoryManager instance;
        return instance;
    }
    static sp<IDevicesFactory> createInterfaceInstance(const std::string& name) {
        return IDevicesFactory::getService(name);
    }
};

namespace impl {

class PrimaryDeviceManager
    : public InterfaceManager<PrimaryDeviceManager, std::string, IPrimaryDevice> {
  public:
    static sp<IPrimaryDevice> createInterfaceInstance(const std::string& factoryName) {
        sp<IDevicesFactory> factory = DevicesFactoryManager::getInstance().get(factoryName);
        return openPrimaryDevice(factory);
    }

    bool reset(const std::string& factoryName) __attribute__((warn_unused_result)) {
#if MAJOR_VERSION <= 5
        return InterfaceManager::reset(factoryName, true);
#elif MAJOR_VERSION >= 6
        {
            sp<IPrimaryDevice> device = getExisting(factoryName);
            if (device != nullptr) {
                auto ret = device->close();
                ALOGE_IF(!ret.isOk(), "PrimaryDevice %s close failed: %s", factoryName.c_str(),
                         ret.description().c_str());
            }
        }
        return InterfaceManager::reset(factoryName, false);
#endif
    }

  private:
    static sp<IPrimaryDevice> openPrimaryDevice(const sp<IDevicesFactory>& factory) {
        if (factory == nullptr) return {};
        Result result;
        sp<IPrimaryDevice> primaryDevice;
#if !(MAJOR_VERSION == 7 && MINOR_VERSION == 1)
        sp<IDevice> device;
#if MAJOR_VERSION == 2
        auto ret = factory->openDevice(IDevicesFactory::Device::PRIMARY, returnIn(result, device));
        if (ret.isOk() && result == Result::OK && device != nullptr) {
            primaryDevice = IPrimaryDevice::castFrom(device);
        }
#elif MAJOR_VERSION >= 4
        auto ret = factory->openPrimaryDevice(returnIn(result, device));
        if (ret.isOk() && result == Result::OK && device != nullptr) {
            primaryDevice = IPrimaryDevice::castFrom(device);
        }
#endif
        if (!ret.isOk() || result != Result::OK || primaryDevice == nullptr) {
            ALOGW("Primary device can not be opened, transaction: %s, result %d, device %p",
                  ret.description().c_str(), result, device.get());
            return nullptr;
        }
#else  // V7.1
        auto ret = factory->openPrimaryDevice_7_1(returnIn(result, primaryDevice));
        if (!ret.isOk() || result != Result::OK) {
            ALOGW("Primary device can not be opened, transaction: %s, result %d",
                  ret.description().c_str(), result);
            return nullptr;
        }
#endif
        return primaryDevice;
    }
};

using FactoryAndDevice = std::tuple<std::string, std::string>;
class RegularDeviceManager
    : public InterfaceManager<RegularDeviceManager, FactoryAndDevice, IDevice> {
  public:
    static sp<IDevice> createInterfaceInstance(const FactoryAndDevice& factoryAndDevice) {
        auto [factoryName, name] = factoryAndDevice;
        sp<IDevicesFactory> factory = DevicesFactoryManager::getInstance().get(factoryName);
        return openDevice(factory, name);
    }

    sp<IDevice> get(const std::string& factoryName, const std::string& name) {
        return InterfaceManager::get(std::make_tuple(factoryName, name));
    }

    bool reset(const std::string& factoryName, const std::string& name)
            __attribute__((warn_unused_result)) {
#if MAJOR_VERSION <= 5
        return InterfaceManager::reset(std::make_tuple(factoryName, name), true);
#elif MAJOR_VERSION >= 6
        {
            sp<IDevice> device = getExisting(std::make_tuple(factoryName, name));
            if (device != nullptr) {
                auto ret = device->close();
                ALOGE_IF(!ret.isOk(), "Device %s::%s close failed: %s", factoryName.c_str(),
                         name.c_str(), ret.description().c_str());
            }
        }
        return InterfaceManager::reset(std::make_tuple(factoryName, name), false);
#endif
    }

  private:
    static sp<IDevice> openDevice(const sp<IDevicesFactory>& factory, const std::string& name) {
        if (factory == nullptr) return nullptr;
        Result result;
        sp<IDevice> device;
#if MAJOR_VERSION == 2
        IDevicesFactory::Device dev = IDevicesFactory::IDevicesFactory::Device(-1);
        if (name == AUDIO_HARDWARE_MODULE_ID_A2DP) {
            dev = IDevicesFactory::Device::A2DP;
        } else if (name == AUDIO_HARDWARE_MODULE_ID_USB) {
            dev = IDevicesFactory::Device::USB;
        } else if (name == AUDIO_HARDWARE_MODULE_ID_REMOTE_SUBMIX) {
            dev = IDevicesFactory::Device::R_SUBMIX;
        } else if (name == AUDIO_HARDWARE_MODULE_ID_STUB) {
            dev = IDevicesFactory::Device::STUB;
        }
        auto ret = factory->openDevice(dev, returnIn(result, device));
#elif MAJOR_VERSION >= 4 && (MAJOR_VERSION < 7 || (MAJOR_VERSION == 7 && MINOR_VERSION == 0))
        auto ret = factory->openDevice(name, returnIn(result, device));
#elif MAJOR_VERSION == 7 && MINOR_VERSION == 1
        auto ret = factory->openDevice_7_1(name, returnIn(result, device));
#endif
        if (!ret.isOk() || result != Result::OK || device == nullptr) {
            ALOGW("Device %s can not be opened, transaction: %s, result %d, device %p",
                  name.c_str(), ret.description().c_str(), result, device.get());
            return nullptr;
        }
        return device;
    }
};

}  // namespace impl

class DeviceManager {
  public:
    static DeviceManager& getInstance() {
        static DeviceManager instance;
        return instance;
    }

    static constexpr const char* kPrimaryDevice = "primary";

    sp<IDevice> get(const std::string& factoryName, const std::string& name) {
        if (name == kPrimaryDevice) {
            auto primary = getPrimary(factoryName);
            return primary ? deviceFromPrimary(primary) : nullptr;
        }
        return mDevices.get(factoryName, name);
    }

    sp<IPrimaryDevice> getPrimary(const std::string& factoryName) {
        return mPrimary.get(factoryName);
    }

    bool reset(const std::string& factoryName, const std::string& name)
            __attribute__((warn_unused_result)) {
        return name == kPrimaryDevice ? resetPrimary(factoryName)
                                      : mDevices.reset(factoryName, name);
    }

    bool resetPrimary(const std::string& factoryName) __attribute__((warn_unused_result)) {
        return mPrimary.reset(factoryName);
    }

    static void waitForInstanceDestruction() {
        // Does not matter which device manager to use.
        impl::RegularDeviceManager::waitForInstanceDestruction();
    }

  private:
    sp<IDevice> deviceFromPrimary(const sp<IPrimaryDevice>& primary) {
#if MAJOR_VERSION == 7 && MINOR_VERSION == 1
        auto ret = primary->getDevice();
        if (ret.isOk()) {
            return ret;
        } else {
            ALOGW("Error retrieving IDevice from primary: transaction: %s, primary %p",
                  ret.description().c_str(), primary.get());
            return nullptr;
        }
#else
        return primary;
#endif
    }

    impl::PrimaryDeviceManager mPrimary;
    impl::RegularDeviceManager mDevices;
};
