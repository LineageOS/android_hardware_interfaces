/*
 * Copyright (C) 2022 The Android Open Source Project
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

#include "thread_chip.hpp"

#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <utils/Log.h>

#include "hdlc_interface.hpp"
#include "spi_interface.hpp"

namespace aidl {
namespace android {
namespace hardware {
namespace threadnetwork {

ThreadChip::ThreadChip(char* url) : mUrl(), mRxFrameBuffer(), mCallback(nullptr) {
    static const char kHdlcProtocol[] = "spinel+hdlc";
    static const char kSpiProtocol[] = "spinel+spi";
    const char* protocol;

    CHECK_EQ(mUrl.Init(url), 0);

    protocol = mUrl.GetProtocol();
    CHECK_NE(protocol, nullptr);

    if (memcmp(protocol, kSpiProtocol, strlen(kSpiProtocol)) == 0) {
        mSpinelInterface = std::make_shared<ot::Posix::SpiInterface>(handleReceivedFrameJump, this,
                                                                     mRxFrameBuffer);
    } else if (memcmp(protocol, kHdlcProtocol, strlen(kHdlcProtocol)) == 0) {
        mSpinelInterface = std::make_shared<ot::Posix::HdlcInterface>(handleReceivedFrameJump, this,
                                                                      mRxFrameBuffer);
    } else {
        ALOGE("The protocol \"%s\" is not supported!", protocol);
        exit(1);
    }

    CHECK_NE(mSpinelInterface, nullptr);
}

void ThreadChip::clientDeathCallback(void* context) {
    reinterpret_cast<ThreadChip*>(context)->clientDeathCallback();
}

void ThreadChip::clientDeathCallback(void) {
    ALOGW("Thread Network HAL client is dead.");
    close();
}

void ThreadChip::handleReceivedFrameJump(void* context) {
    static_cast<ThreadChip*>(context)->handleReceivedFrame();
}

void ThreadChip::handleReceivedFrame(void) {
    if (mCallback != nullptr) {
        mCallback->onReceiveSpinelFrame(std::vector<uint8_t>(
                mRxFrameBuffer.GetFrame(), mRxFrameBuffer.GetFrame() + mRxFrameBuffer.GetLength()));
    }

    mRxFrameBuffer.DiscardFrame();
}

ndk::ScopedAStatus ThreadChip::open(const std::shared_ptr<IThreadChipCallback>& in_callback) {
    ndk::ScopedAStatus status;
    AIBinder* binder;

    VerifyOrExit(mCallback == nullptr,
                 status = errorStatus(ERROR_BUSY, "Interface is already opened"));
    VerifyOrExit(in_callback != nullptr,
                 status = errorStatus(ERROR_INVALID_ARGS, "The callback is NULL"));
    binder = in_callback->asBinder().get();
    VerifyOrExit(binder != nullptr,
                 status = errorStatus(ERROR_FAILED, "Failed to get the callback binder"));
    mBinderDeathRecipient = AIBinder_DeathRecipient_new(clientDeathCallback);
    VerifyOrExit(AIBinder_linkToDeath(binder, mBinderDeathRecipient, this) == STATUS_OK,
                 status = errorStatus(ERROR_FAILED, "Failed to link the binder to death"));
    VerifyOrExit(mSpinelInterface->Init(mUrl) == OT_ERROR_NONE,
                 status = errorStatus(ERROR_FAILED, "Failed to initialize the interface"));

    mCallback = in_callback;
    ot::Posix::Mainloop::Manager::Get().Add(*this);
    status = ndk::ScopedAStatus::ok();

exit:
    if (!status.isOk()) {
        if (mBinderDeathRecipient != nullptr) {
            AIBinder_DeathRecipient_delete(mBinderDeathRecipient);
            mBinderDeathRecipient = nullptr;
        }
        ALOGW("Open failed, error: %s", status.getDescription().c_str());
    } else {
        ALOGI("open()");
    }

    return status;
}

ndk::ScopedAStatus ThreadChip::close() {
    VerifyOrExit(mCallback != nullptr);
    mCallback = nullptr;
    mSpinelInterface->Deinit();

    ot::Posix::Mainloop::Manager::Get().Remove(*this);

    AIBinder_DeathRecipient_delete(mBinderDeathRecipient);
    mBinderDeathRecipient = nullptr;

exit:
    ALOGI("close()");
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus ThreadChip::sendSpinelFrame(const std::vector<uint8_t>& in_frame) {
    ndk::ScopedAStatus status;
    otError error;

    VerifyOrExit(mCallback != nullptr,
                 status = errorStatus(ERROR_FAILED, "The interface is not open"));

    error = mSpinelInterface->SendFrame(reinterpret_cast<const uint8_t*>(in_frame.data()),
                                        in_frame.size());
    if (error == OT_ERROR_NONE) {
        status = ndk::ScopedAStatus::ok();
    } else if (error == OT_ERROR_NO_BUFS) {
        status = errorStatus(ERROR_NO_BUFS, "Insufficient buffer space to send");
    } else if (error == OT_ERROR_BUSY) {
        status = errorStatus(ERROR_BUSY, "The interface is busy");
    } else {
        status = errorStatus(ERROR_FAILED, "Failed to send the spinel frame");
    }

exit:
    if (!status.isOk()) {
        ALOGW("Send spinel frame failed, error: %s", status.getDescription().c_str());
    }

    return status;
}

ndk::ScopedAStatus ThreadChip::reset() {
    mSpinelInterface->HardwareReset();
    ALOGI("reset()");
    return ndk::ScopedAStatus::ok();
}

void ThreadChip::Update(otSysMainloopContext& context) {
    if (mCallback != nullptr) {
        mSpinelInterface->UpdateFdSet(&context);
    }
}

void ThreadChip::Process(const otSysMainloopContext& context) {
    if (mCallback != nullptr) {
        mSpinelInterface->Process(&context);
    }
}

ndk::ScopedAStatus ThreadChip::errorStatus(int32_t error, const char* message) {
    return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(error, message));
}
}  // namespace threadnetwork
}  // namespace hardware
}  // namespace android
}  // namespace aidl
