/*
 * Copyright (C) 2020 The Android Open Source Project
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

#include <libprotocan/MessageInjector.h>

#include <android-base/logging.h>

#include <thread>

namespace android::hardware::automotive::protocan {

/** Whether to log injected messages. */
static constexpr bool kSuperVerbose = true;

using namespace std::literals::chrono_literals;

using can::V1_0::CanMessage;
using can::V1_0::CanMessageId;
using can::V1_0::ICanBus;
using can::V1_0::Result;

MessageInjector::MessageInjector(MessageDef msgDef,
                                 std::optional<std::chrono::milliseconds> interMessageDelay)
    : kMsgDef(std::move(msgDef)),
      kInterMessageDelay(interMessageDelay),
      mCounter(msgDef.makeCounter()) {}

void MessageInjector::inject(const CanMessage& msg) { inject({msg}); }

void MessageInjector::inject(const std::initializer_list<can::V1_0::CanMessage> msgs) {
  std::lock_guard<std::mutex> lock(mMessagesGuard);
  for (const auto& msg : msgs) {
    if constexpr (kSuperVerbose) {
      LOG(VERBOSE) << "Message scheduled for injection: " << toString(msg);
    }

    mMessages.push(msg);
  }
}

void MessageInjector::processQueueLocked(can::V1_0::ICanBus& bus) {
  if (mMessages.empty() || !mCounter.isReady()) return;

  auto paddingMessagesCount = mCounter.upperBound - (mMessages.size() % mCounter.upperBound);
  auto padMessage = kMsgDef.makeDefault();
  for (unsigned i = 0; i < paddingMessagesCount; i++) {
    mMessages.push(padMessage);
  }

  while (!mMessages.empty()) {
    auto&& outMsg = mMessages.front();

    mCounter.increment(outMsg);
    kMsgDef.updateChecksum(outMsg);

    if constexpr (kSuperVerbose) {
      LOG(VERBOSE) << "Injecting message: " << toString(outMsg);
    }
    auto result = bus.send(outMsg);
    if (result != Result::OK) {
      LOG(ERROR) << "Message injection failed: " << toString(result);
    }

    mMessages.pop();

    // This would block onReceive, but the class is not supposed to be used in production anyway
    // (see MessageInjector docstring).
    if (kInterMessageDelay.has_value()) {
      std::this_thread::sleep_for(*kInterMessageDelay);
    }
  }
}

void MessageInjector::onReceive(ICanBus& bus, const CanMessage& msg) {
    if (!kMsgDef.validate(msg)) return;

    std::lock_guard<std::mutex> lock(mMessagesGuard);

    mCounter.read(msg);
    processQueueLocked(bus);
}

MessageInjectorManager::MessageInjectorManager(
    std::initializer_list<std::shared_ptr<MessageInjector>> injectors) {
  std::transform(injectors.begin(), injectors.end(), std::inserter(mInjectors, mInjectors.end()),
                 [](const std::shared_ptr<MessageInjector>& injector) {
                   return std::make_pair(injector->kMsgDef.id, std::move(injector));
                 });
}

void MessageInjectorManager::onReceive(sp<ICanBus> bus, const CanMessage& msg) {
  auto it = mInjectors.find(msg.id);
  if (it == mInjectors.end()) return;
  it->second->onReceive(*bus, msg);
}

}  // namespace android::hardware::automotive::protocan
