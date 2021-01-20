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

#define LOG_TAG "ExecutionBurstController"

#include "ExecutionBurstController.h"

#include <android-base/logging.h>

#include <algorithm>
#include <cstring>
#include <limits>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "ExecutionBurstUtils.h"
#include "HalInterfaces.h"
#include "Tracing.h"
#include "Utils.h"

namespace android::nn {
namespace {

class BurstContextDeathHandler : public hardware::hidl_death_recipient {
  public:
    using Callback = std::function<void()>;

    BurstContextDeathHandler(const Callback& onDeathCallback) : mOnDeathCallback(onDeathCallback) {
        CHECK(onDeathCallback != nullptr);
    }

    void serviceDied(uint64_t /*cookie*/, const wp<hidl::base::V1_0::IBase>& /*who*/) override {
        LOG(ERROR) << "BurstContextDeathHandler::serviceDied -- service unexpectedly died!";
        mOnDeathCallback();
    }

  private:
    const Callback mOnDeathCallback;
};

}  // anonymous namespace

hardware::Return<void> ExecutionBurstController::ExecutionBurstCallback::getMemories(
        const hardware::hidl_vec<int32_t>& slots, getMemories_cb cb) {
    std::lock_guard<std::mutex> guard(mMutex);

    // get all memories
    hardware::hidl_vec<hardware::hidl_memory> memories(slots.size());
    std::transform(slots.begin(), slots.end(), memories.begin(), [this](int32_t slot) {
        return slot < mMemoryCache.size() ? mMemoryCache[slot] : hardware::hidl_memory{};
    });

    // ensure all memories are valid
    if (!std::all_of(memories.begin(), memories.end(),
                     [](const hardware::hidl_memory& memory) { return memory.valid(); })) {
        cb(V1_0::ErrorStatus::INVALID_ARGUMENT, {});
        return hardware::Void();
    }

    // return successful
    cb(V1_0::ErrorStatus::NONE, std::move(memories));
    return hardware::Void();
}

std::vector<int32_t> ExecutionBurstController::ExecutionBurstCallback::getSlots(
        const hardware::hidl_vec<hardware::hidl_memory>& memories,
        const std::vector<intptr_t>& keys) {
    std::lock_guard<std::mutex> guard(mMutex);

    // retrieve (or bind) all slots corresponding to memories
    std::vector<int32_t> slots;
    slots.reserve(memories.size());
    for (size_t i = 0; i < memories.size(); ++i) {
        slots.push_back(getSlotLocked(memories[i], keys[i]));
    }
    return slots;
}

std::pair<bool, int32_t> ExecutionBurstController::ExecutionBurstCallback::freeMemory(
        intptr_t key) {
    std::lock_guard<std::mutex> guard(mMutex);

    auto iter = mMemoryIdToSlot.find(key);
    if (iter == mMemoryIdToSlot.end()) {
        return {false, 0};
    }
    const int32_t slot = iter->second;
    mMemoryIdToSlot.erase(key);
    mMemoryCache[slot] = {};
    mFreeSlots.push(slot);
    return {true, slot};
}

int32_t ExecutionBurstController::ExecutionBurstCallback::getSlotLocked(
        const hardware::hidl_memory& memory, intptr_t key) {
    auto iter = mMemoryIdToSlot.find(key);
    if (iter == mMemoryIdToSlot.end()) {
        const int32_t slot = allocateSlotLocked();
        mMemoryIdToSlot[key] = slot;
        mMemoryCache[slot] = memory;
        return slot;
    } else {
        const int32_t slot = iter->second;
        return slot;
    }
}

int32_t ExecutionBurstController::ExecutionBurstCallback::allocateSlotLocked() {
    constexpr size_t kMaxNumberOfSlots = std::numeric_limits<int32_t>::max();

    // if there is a free slot, use it
    if (mFreeSlots.size() > 0) {
        const int32_t slot = mFreeSlots.top();
        mFreeSlots.pop();
        return slot;
    }

    // otherwise use a slot for the first time
    CHECK(mMemoryCache.size() < kMaxNumberOfSlots) << "Exceeded maximum number of slots!";
    const int32_t slot = static_cast<int32_t>(mMemoryCache.size());
    mMemoryCache.emplace_back();

    return slot;
}

std::unique_ptr<ExecutionBurstController> ExecutionBurstController::create(
        const sp<V1_2::IPreparedModel>& preparedModel,
        std::chrono::microseconds pollingTimeWindow) {
    // check inputs
    if (preparedModel == nullptr) {
        LOG(ERROR) << "ExecutionBurstController::create passed a nullptr";
        return nullptr;
    }

    // create callback object
    sp<ExecutionBurstCallback> callback = new ExecutionBurstCallback();

    // create FMQ objects
    auto [requestChannelSenderTemp, requestChannelDescriptor] =
            RequestChannelSender::create(kExecutionBurstChannelLength);
    auto [resultChannelReceiverTemp, resultChannelDescriptor] =
            ResultChannelReceiver::create(kExecutionBurstChannelLength, pollingTimeWindow);
    std::shared_ptr<RequestChannelSender> requestChannelSender =
            std::move(requestChannelSenderTemp);
    std::shared_ptr<ResultChannelReceiver> resultChannelReceiver =
            std::move(resultChannelReceiverTemp);

    // check FMQ objects
    if (!requestChannelSender || !resultChannelReceiver || !requestChannelDescriptor ||
        !resultChannelDescriptor) {
        LOG(ERROR) << "ExecutionBurstController::create failed to create FastMessageQueue";
        return nullptr;
    }

    // configure burst
    V1_0::ErrorStatus errorStatus;
    sp<IBurstContext> burstContext;
    const hardware::Return<void> ret = preparedModel->configureExecutionBurst(
            callback, *requestChannelDescriptor, *resultChannelDescriptor,
            [&errorStatus, &burstContext](V1_0::ErrorStatus status,
                                          const sp<IBurstContext>& context) {
                errorStatus = status;
                burstContext = context;
            });

    // check burst
    if (!ret.isOk()) {
        LOG(ERROR) << "IPreparedModel::configureExecutionBurst failed with description "
                   << ret.description();
        return nullptr;
    }
    if (errorStatus != V1_0::ErrorStatus::NONE) {
        LOG(ERROR) << "IPreparedModel::configureExecutionBurst failed with status "
                   << toString(errorStatus);
        return nullptr;
    }
    if (burstContext == nullptr) {
        LOG(ERROR) << "IPreparedModel::configureExecutionBurst returned nullptr for burst";
        return nullptr;
    }

    // create death handler object
    BurstContextDeathHandler::Callback onDeathCallback = [requestChannelSender,
                                                          resultChannelReceiver] {
        requestChannelSender->invalidate();
        resultChannelReceiver->invalidate();
    };
    const sp<BurstContextDeathHandler> deathHandler = new BurstContextDeathHandler(onDeathCallback);

    // linkToDeath registers a callback that will be invoked on service death to
    // proactively handle service crashes. If the linkToDeath call fails,
    // asynchronous calls are susceptible to hangs if the service crashes before
    // providing the response.
    const hardware::Return<bool> deathHandlerRet = burstContext->linkToDeath(deathHandler, 0);
    if (!deathHandlerRet.isOk() || deathHandlerRet != true) {
        LOG(ERROR) << "ExecutionBurstController::create -- Failed to register a death recipient "
                      "for the IBurstContext object.";
        return nullptr;
    }

    // make and return controller
    return std::make_unique<ExecutionBurstController>(requestChannelSender, resultChannelReceiver,
                                                      burstContext, callback, deathHandler);
}

ExecutionBurstController::ExecutionBurstController(
        const std::shared_ptr<RequestChannelSender>& requestChannelSender,
        const std::shared_ptr<ResultChannelReceiver>& resultChannelReceiver,
        const sp<IBurstContext>& burstContext, const sp<ExecutionBurstCallback>& callback,
        const sp<hardware::hidl_death_recipient>& deathHandler)
    : mRequestChannelSender(requestChannelSender),
      mResultChannelReceiver(resultChannelReceiver),
      mBurstContext(burstContext),
      mMemoryCache(callback),
      mDeathHandler(deathHandler) {}

ExecutionBurstController::~ExecutionBurstController() {
    // It is safe to ignore any errors resulting from this unlinkToDeath call
    // because the ExecutionBurstController object is already being destroyed
    // and its underlying IBurstContext object is no longer being used by the NN
    // runtime.
    if (mDeathHandler) {
        mBurstContext->unlinkToDeath(mDeathHandler).isOk();
    }
}

static std::tuple<int, std::vector<V1_2::OutputShape>, V1_2::Timing, bool> getExecutionResult(
        V1_0::ErrorStatus status, std::vector<V1_2::OutputShape> outputShapes, V1_2::Timing timing,
        bool fallback) {
    auto [n, checkedOutputShapes, checkedTiming] =
            getExecutionResult(convertToV1_3(status), std::move(outputShapes), timing);
    return {n, convertToV1_2(checkedOutputShapes), convertToV1_2(checkedTiming), fallback};
}

std::tuple<int, std::vector<V1_2::OutputShape>, V1_2::Timing, bool>
ExecutionBurstController::compute(const V1_0::Request& request, V1_2::MeasureTiming measure,
                                  const std::vector<intptr_t>& memoryIds) {
    // This is the first point when we know an execution is occurring, so begin
    // to collect systraces. Note that the first point we can begin collecting
    // systraces in ExecutionBurstServer is when the RequestChannelReceiver
    // realizes there is data in the FMQ, so ExecutionBurstServer collects
    // systraces at different points in the code.
    NNTRACE_FULL(NNTRACE_LAYER_IPC, NNTRACE_PHASE_EXECUTION, "ExecutionBurstController::compute");

    std::lock_guard<std::mutex> guard(mMutex);

    // send request packet
    const std::vector<int32_t> slots = mMemoryCache->getSlots(request.pools, memoryIds);
    const bool success = mRequestChannelSender->send(request, measure, slots);
    if (!success) {
        LOG(ERROR) << "Error sending FMQ packet";
        // only use fallback execution path if the packet could not be sent
        return getExecutionResult(V1_0::ErrorStatus::GENERAL_FAILURE, {}, kNoTiming12,
                                  /*fallback=*/true);
    }

    // get result packet
    const auto result = mResultChannelReceiver->getBlocking();
    if (!result) {
        LOG(ERROR) << "Error retrieving FMQ packet";
        // only use fallback execution path if the packet could not be sent
        return getExecutionResult(V1_0::ErrorStatus::GENERAL_FAILURE, {}, kNoTiming12,
                                  /*fallback=*/false);
    }

    // unpack results and return (only use fallback execution path if the
    // packet could not be sent)
    auto [status, outputShapes, timing] = std::move(*result);
    return getExecutionResult(status, std::move(outputShapes), timing, /*fallback=*/false);
}

void ExecutionBurstController::freeMemory(intptr_t key) {
    std::lock_guard<std::mutex> guard(mMutex);

    bool valid;
    int32_t slot;
    std::tie(valid, slot) = mMemoryCache->freeMemory(key);
    if (valid) {
        mBurstContext->freeMemory(slot).isOk();
    }
}

}  // namespace android::nn
