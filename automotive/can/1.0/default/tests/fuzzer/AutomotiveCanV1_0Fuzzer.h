/*
 * Copyright (C) 2022 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#ifndef __AUTOMOTIVE_CAN_V1_0_FUZZER_H__
#define __AUTOMOTIVE_CAN_V1_0_FUZZER_H__
#include <CanController.h>
#include <android/hidl/manager/1.2/IServiceManager.h>
#include <fuzzer/FuzzedDataProvider.h>
#include <hidl-utils/hidl-utils.h>

namespace android::hardware::automotive::can::V1_0::implementation::fuzzer {

using ::android::sp;

struct CanMessageListener : public can::V1_0::ICanMessageListener {
    DISALLOW_COPY_AND_ASSIGN(CanMessageListener);

    CanMessageListener() {}

    virtual Return<void> onReceive(const can::V1_0::CanMessage& msg) override {
        std::unique_lock<std::mutex> lock(mMessagesGuard);
        mMessages.push_back(msg);
        mMessagesUpdated.notify_one();
        return {};
    }

    virtual ~CanMessageListener() {
        if (mCloseHandle) {
            mCloseHandle->close();
        }
    }

    void assignCloseHandle(sp<ICloseHandle> closeHandle) { mCloseHandle = closeHandle; }

  private:
    sp<ICloseHandle> mCloseHandle;

    std::mutex mMessagesGuard;
    std::condition_variable mMessagesUpdated GUARDED_BY(mMessagesGuard);
    std::vector<can::V1_0::CanMessage> mMessages GUARDED_BY(mMessagesGuard);
};

struct Bus {
    DISALLOW_COPY_AND_ASSIGN(Bus);

    Bus(sp<ICanController> controller, const ICanController::BusConfig& config)
        : mIfname(config.name), mController(controller) {
        const auto result = controller->upInterface(config);
        const auto manager = hidl::manager::V1_2::IServiceManager::getService();
        const auto service = manager->get(ICanBus::descriptor, config.name);
        mBus = ICanBus::castFrom(service);
    }

    virtual ~Bus() { reset(); }

    void reset() {
        mBus.clear();
        if (mController) {
            mController->downInterface(mIfname);
            mController.clear();
        }
    }

    ICanBus* operator->() const { return mBus.get(); }
    sp<ICanBus> get() { return mBus; }

    sp<CanMessageListener> listen(const hidl_vec<CanMessageFilter>& filter) {
        sp<CanMessageListener> listener = sp<CanMessageListener>::make();

        if (!mBus) {
            return listener;
        }
        Result result;
        sp<ICloseHandle> closeHandle;
        mBus->listen(filter, listener, hidl_utils::fill(&result, &closeHandle)).assertOk();
        listener->assignCloseHandle(closeHandle);

        return listener;
    }

    void send(const CanMessage& msg) {
        if (!mBus) {
            return;
        }
        mBus->send(msg);
    }

  private:
    const std::string mIfname;
    sp<ICanController> mController;
    sp<ICanBus> mBus;
};

class CanFuzzer {
  public:
    ~CanFuzzer() { deInit(); }
    bool init();
    void process(const uint8_t* data, size_t size);
    void deInit();

  private:
    Bus makeBus();
    hidl_vec<hidl_string> getBusNames();
    void getSupportedInterfaceTypes();
    void invokeBus();
    void invokeUpInterface();
    void invokeDownInterface();
    FuzzedDataProvider* mFuzzedDataProvider = nullptr;
    sp<CanController> mCanController = nullptr;
    hidl_vec<hidl_string> mBusNames = {};
    unsigned mLastInterface = 0;
};
}  // namespace android::hardware::automotive::can::V1_0::implementation::fuzzer

#endif  // __AUTOMOTIVE_CAN_V1_0_FUZZER_H__
