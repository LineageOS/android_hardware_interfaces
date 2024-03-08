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
#include <android/binder_auto_utils.h>
#include <android/binder_ibinder.h>
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
    const char* interfaceName;

    CHECK_EQ(mUrl.Init(url), 0);

    interfaceName = mUrl.GetProtocol();
    CHECK_NE(interfaceName, nullptr);

    if (ot::Posix::SpiInterface::IsInterfaceNameMatch(interfaceName)) {
        mSpinelInterface = std::make_shared<ot::Posix::SpiInterface>(mUrl);
    } else if (ot::Posix::HdlcInterface::IsInterfaceNameMatch(interfaceName)) {
        mSpinelInterface = std::make_shared<ot::Posix::HdlcInterface>(mUrl);
    } else {
        ALOGE("The interface \"%s\" is not supported", interfaceName);
        exit(EXIT_FAILURE);
    }

    CHECK_NE(mSpinelInterface, nullptr);

    mDeathRecipient = ndk::ScopedAIBinder_DeathRecipient(
            AIBinder_DeathRecipient_new(ThreadChip::onBinderDiedJump));
    AIBinder_DeathRecipient_setOnUnlinked(mDeathRecipient.get(), ThreadChip::onBinderUnlinkedJump);
}

void ThreadChip::onBinderDiedJump(void* context) {
    reinterpret_cast<ThreadChip*>(context)->onBinderDied();
}

void ThreadChip::onBinderDied(void) {
    ALOGW("Thread Network HAL client is dead");
}

void ThreadChip::onBinderUnlinkedJump(void* context) {
    reinterpret_cast<ThreadChip*>(context)->onBinderUnlinked();
}

void ThreadChip::onBinderUnlinked(void) {
    ALOGW("ThreadChip binder is unlinked");
    deinitChip();
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
    ndk::ScopedAStatus status = initChip(in_callback);

    if (status.isOk()) {
        AIBinder_linkToDeath(in_callback->asBinder().get(), mDeathRecipient.get(), this);
        ALOGI("Open IThreadChip successfully");
    } else {
        ALOGW("Failed to open IThreadChip: %s", status.getDescription().c_str());
    }

    return status;
}

ndk::ScopedAStatus ThreadChip::initChip(const std::shared_ptr<IThreadChipCallback>& in_callback) {
    if (in_callback == nullptr) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    } else if (mCallback == nullptr) {
        if (mSpinelInterface->Init(handleReceivedFrameJump, this, mRxFrameBuffer) !=
            OT_ERROR_NONE) {
            return errorStatus(ERROR_FAILED, "Failed to initialize the interface");
        }

        mCallback = in_callback;
        ot::Posix::Mainloop::Manager::Get().Add(*this);
        return ndk::ScopedAStatus::ok();
    } else {
        return errorStatus(ERROR_BUSY, "Interface has been opened");
    }
}

ndk::ScopedAStatus ThreadChip::close() {
    ndk::ScopedAStatus status;
    std::shared_ptr<IThreadChipCallback> callback = mCallback;

    status = deinitChip();
    if (status.isOk()) {
        if (callback != nullptr) {
            AIBinder_unlinkToDeath(callback->asBinder().get(), mDeathRecipient.get(), this);
        }

        ALOGI("Close IThreadChip successfully");
    } else {
        ALOGW("Failed to close IThreadChip: %s", status.getDescription().c_str());
    }

    return status;
}

ndk::ScopedAStatus ThreadChip::deinitChip() {
    if (mCallback != nullptr) {
        mSpinelInterface->Deinit();
        ot::Posix::Mainloop::Manager::Get().Remove(*this);
        mCallback = nullptr;
        return ndk::ScopedAStatus::ok();
    }

    return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
}

ndk::ScopedAStatus ThreadChip::sendSpinelFrame(const std::vector<uint8_t>& in_frame) {
    ndk::ScopedAStatus status;
    otError error;

    if (mCallback == nullptr) {
        status = errorStatus(ERROR_FAILED, "The interface is not open");
    } else {
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
    }

    if (!status.isOk()) {
        ALOGW("Send spinel frame failed, error: %s", status.getDescription().c_str());
    }

    return status;
}

ndk::ScopedAStatus ThreadChip::hardwareReset() {
    if (mSpinelInterface->HardwareReset() == OT_ERROR_NOT_IMPLEMENTED) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }

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
