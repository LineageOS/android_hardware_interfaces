#include "Event.h"
#include <android-base/logging.h>

namespace android {
namespace hardware {
namespace neuralnetworks {
namespace V1_0 {
namespace implementation {

Event::Event() : mStatus(Status::WAITING) {}

Event::~Event() {
    if (mThread.joinable()) {
        mThread.join();
    }
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

}  // namespace implementation
}  // namespace V1_0
}  // namespace neuralnetworks
}  // namespace hardware
}  // namespace android
