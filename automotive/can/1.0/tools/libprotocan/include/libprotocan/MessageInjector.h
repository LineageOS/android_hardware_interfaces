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

#pragma once

#include <android-base/macros.h>
#include <android/hardware/automotive/can/1.0/ICanBus.h>
#include <libprotocan/MessageCounter.h>
#include <libprotocan/MessageDef.h>
#include <utils/Mutex.h>

#include <queue>

namespace android::hardware::automotive::protocan {

class MessageInjectorManager;

/**
 * Injects CAN messages with a counter to an existing system.
 *
 * This class is NOT meant to use in production - there should be no need to inject counted CAN
 * messages where the other sender is also broadcasting them. If this is the case, it may be a sign
 * your CAN network needs a redesign. This tool is intended for use for testing and demo purposes.
 */
class MessageInjector {
 public:
  MessageInjector(MessageDef msgDef, std::optional<std::chrono::milliseconds> interMessageDelay);

  void inject(const can::V1_0::CanMessage& msg);
  void inject(const std::initializer_list<can::V1_0::CanMessage> msgs);

 private:
  const MessageDef kMsgDef;
  const std::optional<std::chrono::milliseconds> kInterMessageDelay;
  MessageCounter mCounter;

  mutable std::mutex mMessagesGuard;
  std::queue<can::V1_0::CanMessage> mMessages GUARDED_BY(mMessagesGuard);

  void onReceive(can::V1_0::ICanBus& bus, const can::V1_0::CanMessage& msg);
  void processQueueLocked(can::V1_0::ICanBus& bus);

  friend class MessageInjectorManager;

  DISALLOW_COPY_AND_ASSIGN(MessageInjector);
};

/**
 * Routes intercepted messages to MessageInjector instances configured to handle specific CAN
 * message (CAN message ID). Intercepted messages from other nodes in CAN network are used to read
 * current counter value in order to spoof the next packet.
 */
class MessageInjectorManager {
 public:
  MessageInjectorManager(std::initializer_list<std::shared_ptr<MessageInjector>> injectors);

  void onReceive(sp<can::V1_0::ICanBus> bus, const can::V1_0::CanMessage& msg);

 private:
  std::map<can::V1_0::CanMessageId, std::shared_ptr<MessageInjector>> mInjectors;

  DISALLOW_COPY_AND_ASSIGN(MessageInjectorManager);
};

}  // namespace android::hardware::automotive::protocan
