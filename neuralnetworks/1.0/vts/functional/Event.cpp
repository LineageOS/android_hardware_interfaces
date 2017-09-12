#include "Event.h"
#include <android-base/logging.h>

namespace android {
namespace hardware {
namespace neuralnetworks {
namespace V1_0 {
namespace implementation {

Event::Event() : mStatus(Status::WAITING) {}

Event::~Event() {
    // Note that we cannot call Event::join_thread from here: Event is
    // intended to be reference counted, and it is possible that the
    // reference count drops to zero in the bound thread, causing the
    // bound thread to call this destructor. If a thread tries to join
    // itself, it throws an exception, producing a message like the
    // following:
    //
    //     terminating with uncaught exception of type std::__1::system_error:
    //     thread::join failed: Resource deadlock would occur
}

Return<void> Event::notify(ReturnedStatus status) {
    {
        std::lock_guard<std::mutex> lock(mMutex);
        mStatus = status == ReturnedStatus::SUCCESS ? Status::SUCCESS : Status::ERROR;
        if (mStatus == Status::SUCCESS && mCallback != nullptr) {
            bool success = mCallback();
            if (!success) {
                LOG(ERROR) << "Event::notify -- callback failed";
            }
        }
    }
    mCondition.notify_all();
    return Void();
}

Event::Status Event::poll() {
    std::lock_guard<std::mutex> lock(mMutex);
    return mStatus;
}

Event::Status Event::wait() {
    std::unique_lock<std::mutex> lock(mMutex);
    mCondition.wait(lock, [this]{return mStatus != Status::WAITING;});
    join_thread_locked();
    return mStatus;
}

bool Event::on_finish(std::function<bool(void)> callback) {
    std::lock_guard<std::mutex> lock(mMutex);
    if (mCallback != nullptr) {
        LOG(ERROR) << "Event::on_finish -- a callback has already been bound to this event";
        return false;
    }
    if (callback == nullptr) {
        LOG(ERROR) << "Event::on_finish -- the new callback is invalid";
        return false;
    }
    mCallback = std::move(callback);
    return true;
}

bool Event::bind_thread(std::thread&& asyncThread) {
    std::lock_guard<std::mutex> lock(mMutex);
    if (mThread.joinable()) {
        LOG(ERROR) << "Event::bind_thread -- a thread has already been bound to this event";
        return false;
    }
    if (!asyncThread.joinable()) {
        LOG(ERROR) << "Event::bind_thread -- the new thread is not joinable";
        return false;
    }
    mThread = std::move(asyncThread);
    return true;
}

void Event::join_thread() {
    std::lock_guard<std::mutex> lock(mMutex);
    join_thread_locked();
}

void Event::join_thread_locked() {
    if (mThread.joinable()) {
        mThread.join();
    }
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace neuralnetworks
}  // namespace hardware
}  // namespace android
