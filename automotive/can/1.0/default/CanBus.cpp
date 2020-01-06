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

#include "CanBus.h"

#include "CloseHandle.h"

#include <android-base/logging.h>
#include <libnetdevice/can.h>
#include <libnetdevice/libnetdevice.h>
#include <linux/can.h>
#include <linux/can/error.h>
#include <linux/can/raw.h>

namespace android::hardware::automotive::can::V1_0::implementation {

/** Whether to log sent/received packets. */
static constexpr bool kSuperVerbose = false;

Return<Result> CanBus::send(const CanMessage& message) {
    std::lock_guard<std::mutex> lck(mIsUpGuard);
    if (!mIsUp) return Result::INTERFACE_DOWN;

    if (UNLIKELY(kSuperVerbose)) {
        LOG(VERBOSE) << "Sending " << toString(message);
    }

    if (message.payload.size() > CAN_MAX_DLEN) return Result::PAYLOAD_TOO_LONG;

    struct canfd_frame frame = {};
    frame.can_id = message.id;
    if (message.isExtendedId) frame.can_id |= CAN_EFF_FLAG;
    if (message.remoteTransmissionRequest) frame.can_id |= CAN_RTR_FLAG;
    frame.len = message.payload.size();
    memcpy(frame.data, message.payload.data(), message.payload.size());

    if (!mSocket->send(frame)) return Result::TRANSMISSION_FAILURE;

    return Result::OK;
}

Return<void> CanBus::listen(const hidl_vec<CanMessageFilter>& filter,
                            const sp<ICanMessageListener>& listenerCb, listen_cb _hidl_cb) {
    std::lock_guard<std::mutex> lck(mIsUpGuard);

    if (listenerCb == nullptr) {
        _hidl_cb(Result::INVALID_ARGUMENTS, nullptr);
        return {};
    }
    if (!mIsUp) {
        _hidl_cb(Result::INTERFACE_DOWN, nullptr);
        return {};
    }

    std::lock_guard<std::mutex> lckListeners(mMsgListenersGuard);

    sp<CloseHandle> closeHandle = new CloseHandle([this, listenerCb]() {
        std::lock_guard<std::mutex> lck(mMsgListenersGuard);
        std::erase_if(mMsgListeners, [&](const auto& e) { return e.callback == listenerCb; });
    });
    mMsgListeners.emplace_back(CanMessageListener{listenerCb, filter, closeHandle});
    auto& listener = mMsgListeners.back();

    // fix message IDs to have all zeros on bits not covered by mask
    std::for_each(listener.filter.begin(), listener.filter.end(),
                  [](auto& rule) { rule.id &= rule.mask; });

    _hidl_cb(Result::OK, closeHandle);
    return {};
}

CanBus::CanBus() {}

CanBus::CanBus(const std::string& ifname) : mIfname(ifname) {}

CanBus::~CanBus() {
    std::lock_guard<std::mutex> lck(mIsUpGuard);
    CHECK(!mIsUp) << "Interface is still up while being destroyed";

    std::lock_guard<std::mutex> lckListeners(mMsgListenersGuard);
    CHECK(mMsgListeners.empty()) << "Listener list is not empty while interface is being destroyed";
}

void CanBus::setErrorCallback(ErrorCallback errcb) {
    CHECK(!mIsUp) << "Can't set error callback while interface is up";
    CHECK(mErrCb == nullptr) << "Error callback is already set";
    mErrCb = errcb;
    CHECK(!mIsUp) << "Can't set error callback while interface is up";
}

ICanController::Result CanBus::preUp() {
    return ICanController::Result::OK;
}

bool CanBus::postDown() {
    return true;
}

ICanController::Result CanBus::up() {
    std::lock_guard<std::mutex> lck(mIsUpGuard);

    if (mIsUp) {
        LOG(WARNING) << "Interface is already up";
        return ICanController::Result::INVALID_STATE;
    }

    const auto preResult = preUp();
    if (preResult != ICanController::Result::OK) return preResult;

    const auto isUp = netdevice::isUp(mIfname);
    if (!isUp.has_value()) {
        // preUp() should prepare the interface (either create or make sure it's there)
        LOG(ERROR) << "Interface " << mIfname << " didn't get prepared";
        return ICanController::Result::BAD_ADDRESS;
    }

    if (!*isUp && !netdevice::up(mIfname)) {
        LOG(ERROR) << "Can't bring " << mIfname << " up";
        return ICanController::Result::UNKNOWN_ERROR;
    }
    mDownAfterUse = !*isUp;

    using namespace std::placeholders;
    CanSocket::ReadCallback rdcb = std::bind(&CanBus::onRead, this, _1, _2);
    CanSocket::ErrorCallback errcb = std::bind(&CanBus::onError, this, _1);
    mSocket = CanSocket::open(mIfname, rdcb, errcb);
    if (!mSocket) {
        if (mDownAfterUse) netdevice::down(mIfname);
        return ICanController::Result::UNKNOWN_ERROR;
    }

    mIsUp = true;
    return ICanController::Result::OK;
}

void CanBus::clearMsgListeners() {
    std::vector<wp<ICloseHandle>> listenersToClose;
    {
        std::lock_guard<std::mutex> lck(mMsgListenersGuard);
        std::transform(mMsgListeners.begin(), mMsgListeners.end(),
                       std::back_inserter(listenersToClose),
                       [](const auto& e) { return e.closeHandle; });
    }

    for (auto& weakListener : listenersToClose) {
        /* Between populating listenersToClose and calling close method here, some listeners might
         * have been already removed from the original mMsgListeners list (resulting in a dangling
         * weak pointer here). It's fine - we just want to clean them up. */
        auto listener = weakListener.promote();
        if (listener != nullptr) listener->close();
    }

    std::lock_guard<std::mutex> lck(mMsgListenersGuard);
    CHECK(mMsgListeners.empty()) << "Listeners list wasn't emptied";
}

void CanBus::clearErrListeners() {
    std::lock_guard<std::mutex> lck(mErrListenersGuard);
    mErrListeners.clear();
}

Return<sp<ICloseHandle>> CanBus::listenForErrors(const sp<ICanErrorListener>& listener) {
    if (listener == nullptr) {
        return new CloseHandle();
    }

    std::lock_guard<std::mutex> upLck(mIsUpGuard);
    if (!mIsUp) {
        listener->onError(ErrorEvent::INTERFACE_DOWN, true);
        return new CloseHandle();
    }

    std::lock_guard<std::mutex> errLck(mErrListenersGuard);
    mErrListeners.emplace_back(listener);

    return new CloseHandle([this, listener]() {
        std::lock_guard<std::mutex> lck(mErrListenersGuard);
        std::erase(mErrListeners, listener);
    });
}

bool CanBus::down() {
    std::lock_guard<std::mutex> lck(mIsUpGuard);

    if (!mIsUp) {
        LOG(WARNING) << "Interface is already down";
        return false;
    }
    mIsUp = false;

    clearMsgListeners();
    clearErrListeners();
    mSocket.reset();

    bool success = true;

    if (mDownAfterUse && !netdevice::down(mIfname)) {
        LOG(ERROR) << "Can't bring " << mIfname << " down";
        // don't return yet, let's try to do best-effort cleanup
        success = false;
    }

    if (!postDown()) success = false;

    return success;
}

/**
 * Helper function to determine if a flag meets the requirements of a
 * FilterFlag. See definition of FilterFlag in types.hal
 *
 * \param filterFlag FilterFlag object to match flag against
 * \param flag bool object from CanMessage object
 */
static bool satisfiesFilterFlag(FilterFlag filterFlag, bool flag) {
    // TODO(b/144458917) add testing for this to VTS tests
    if (filterFlag == FilterFlag::DONT_CARE) return true;
    if (filterFlag == FilterFlag::SET) return flag;
    if (filterFlag == FilterFlag::NOT_SET) return !flag;
    return false;
}

/**
 * Match the filter set against message id.
 *
 * For details on the filters syntax, please see CanMessageFilter at
 * the HAL definition (types.hal).
 *
 * \param filter Filter to match against
 * \param id Message id to filter
 * \return true if the message id matches the filter, false otherwise
 */
static bool match(const hidl_vec<CanMessageFilter>& filter, CanMessageId id, bool isRtr,
                  bool isExtendedId) {
    if (filter.size() == 0) return true;

    bool anyNonExcludeRulePresent = false;
    bool anyNonExcludeRuleSatisfied = false;
    for (auto& rule : filter) {
        const bool satisfied = ((id & rule.mask) == rule.id) &&
                               satisfiesFilterFlag(rule.rtr, isRtr) &&
                               satisfiesFilterFlag(rule.extendedFormat, isExtendedId);

        if (rule.exclude) {
            // Any excluded (blacklist) rule not being satisfied invalidates the whole filter set.
            if (satisfied) return false;
        } else {
            anyNonExcludeRulePresent = true;
            if (satisfied) anyNonExcludeRuleSatisfied = true;
        }
    }
    return !anyNonExcludeRulePresent || anyNonExcludeRuleSatisfied;
}

void CanBus::notifyErrorListeners(ErrorEvent err, bool isFatal) {
    std::lock_guard<std::mutex> lck(mErrListenersGuard);
    for (auto& listener : mErrListeners) {
        if (!listener->onError(err, isFatal).isOk()) {
            LOG(WARNING) << "Failed to notify listener about error";
        }
    }
}

static ErrorEvent parseErrorFrame(const struct canfd_frame& frame) {
    // decode error frame (to a degree)
    if ((frame.can_id & (CAN_ERR_BUSERROR | CAN_ERR_BUSOFF)) != 0) {
        return ErrorEvent::BUS_ERROR;
    }
    if ((frame.data[1] & CAN_ERR_CRTL_TX_OVERFLOW) != 0) {
        return ErrorEvent::TX_OVERFLOW;
    }
    if ((frame.data[1] & CAN_ERR_CRTL_RX_OVERFLOW) != 0) {
        return ErrorEvent::RX_OVERFLOW;
    }
    if ((frame.data[2] & CAN_ERR_PROT_OVERLOAD) != 0) {
        return ErrorEvent::BUS_OVERLOAD;
    }
    if ((frame.can_id & CAN_ERR_PROT) != 0) {
        return ErrorEvent::MALFORMED_INPUT;
    }
    if ((frame.can_id & (CAN_ERR_CRTL | CAN_ERR_TRX | CAN_ERR_RESTARTED)) != 0) {
        // "controller restarted" constitutes a HARDWARE_ERROR imo
        return ErrorEvent::HARDWARE_ERROR;
    }
    return ErrorEvent::UNKNOWN_ERROR;
}

void CanBus::onRead(const struct canfd_frame& frame, std::chrono::nanoseconds timestamp) {
    if ((frame.can_id & CAN_ERR_FLAG) != 0) {
        // error bit is set
        LOG(WARNING) << "CAN Error frame received";
        // TODO(b/144458917) consider providing different values for isFatal, depending on error
        notifyErrorListeners(parseErrorFrame(frame), false);
        return;
    }

    CanMessage message = {};
    message.id = frame.can_id & CAN_EFF_MASK;  // mask out eff/rtr/err flags
    message.payload = hidl_vec<uint8_t>(frame.data, frame.data + frame.len);
    message.timestamp = timestamp.count();
    message.isExtendedId = (frame.can_id & CAN_EFF_FLAG) != 0;
    message.remoteTransmissionRequest = (frame.can_id & CAN_RTR_FLAG) != 0;

    if (UNLIKELY(kSuperVerbose)) {
        LOG(VERBOSE) << "Got message " << toString(message);
    }

    std::lock_guard<std::mutex> lck(mMsgListenersGuard);
    for (auto& listener : mMsgListeners) {
        if (!match(listener.filter, message.id, message.remoteTransmissionRequest,
                   message.isExtendedId))
            continue;
        if (!listener.callback->onReceive(message).isOk() && !listener.failedOnce) {
            listener.failedOnce = true;
            LOG(WARNING) << "Failed to notify listener about message";
        }
    }
}

void CanBus::onError(int errnoVal) {
    auto eventType = ErrorEvent::HARDWARE_ERROR;

    if (errnoVal == ENODEV || errnoVal == ENETDOWN) {
        mDownAfterUse = false;
        eventType = ErrorEvent::INTERFACE_DOWN;
    }
    notifyErrorListeners(eventType, true);

    const auto errcb = mErrCb;
    if (errcb != nullptr) errcb();
}

}  // namespace android::hardware::automotive::can::V1_0::implementation
